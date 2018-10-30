/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Jörg Hohensohn
 *
 * This module collects the Talkbox and voice UI functions.
 * (Talkbox reads directory names from mp3 clips called thumbnails,
 *  the voice UI lets menus and screens "talk" from a voicefile in memory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "string-extra.h"
#include "file.h"
#include "system.h"
#include "kernel.h"
#include "settings.h"
#include "settings_list.h"
#if CONFIG_CODEC == SWCODEC
#include "voice_thread.h"
#else
#include "mp3_playback.h"
#endif
#include "audio.h"
#include "lang.h"
#include "talk.h"
#include "metadata.h"
/*#define LOGF_ENABLE*/
#include "logf.h"
#include "bitswap.h"
#include "structec.h"
#include "plugin.h" /* plugin_get_buffer() */
#include "debug.h"
#include "panic.h"

/* Memory layout varies between targets because the
   Archos (MASCODEC) devices cannot mix voice and audio playback
 
             MASCODEC  | MASCODEC  | SWCODEC
             (playing) | (stopped) |
    voicebuf-----------+-----------+------------
              audio    | voice     | voice
                       |-----------|------------
                       | thumbnail | thumbnail 
                       |           |------------
                       |           | filebuf
                       |           |------------
                       |           | audio
  voicebufend----------+-----------+------------

  SWCODEC allocates dedicated buffers (except voice and thumbnail are together
  in the talkbuf), MASCODEC reuses audiobuf. */


/***************** Constants *****************/

#define QUEUE_SIZE 64 /* must be a power of two */
#define QUEUE_MASK (QUEUE_SIZE-1)
const char* const dir_thumbnail_name = "_dirname.talk";
const char* const file_thumbnail_ext = ".talk";

/***************** Functional Macros *****************/

#define QUEUE_LEVEL ((queue_write - queue_read) & QUEUE_MASK)

#define LOADED_MASK 0x80000000 /* MSB */

#define DEFAULT_VOICE_LANG "english"

/***************** Data types *****************/

struct clip_entry /* one entry of the index table */
{
    int offset; /* offset from start of voicefile file */
    int size; /* size of the clip */
};

struct voicefile_header /* file format of our voice file */
{
    int version; /* version of the voicefile */
    int target_id; /* the rockbox target the file was made for */
    int table;   /* offset to index table, (=header size) */
    int id1_max; /* number of "normal" clips contained in above index */
    int id2_max; /* number of "voice only" clips contained in above index */
    /* The header is folled by the index tables (n*struct clip_entry),
     * which is followed by the mp3/speex encoded clip data */
};

/***************** Globals *****************/

#if (CONFIG_CODEC == SWCODEC && MEMORYSIZE <= 2) || defined(ONDIO_SERIES)
/* On low memory swcodec targets the entire voice file wouldn't fit in memory
 * together with codecs, so we load clips each time they are accessed.
 * The Ondios have slow storage access and loading the entire voice file would
 * take several seconds, so we use the same mechanism. */
#define TALK_PROGRESSIVE_LOAD
#if !defined(ONDIO_SERIES)
/* 70+ clips should fit into 100k */
#define MAX_CLIP_BUFFER_SIZE (100000)
#endif
#endif

#ifndef MAX_CLIP_BUFFER_SIZE
/* 1GB should be enough for everybody; will be capped to voicefile size */
#define MAX_CLIP_BUFFER_SIZE (1<<30)
#endif
#define THUMBNAIL_RESERVE (50000)

/* Multiple thumbnails can be loaded back-to-back in this buffer. */
static volatile int thumbnail_buf_used SHAREDBSS_ATTR; /* length of data in
                                                          thumbnail buffer */
static struct voicefile_header voicefile; /* loaded voicefile */
static bool has_voicefile; /* a voicefile file is present */
static bool need_shutup; /* is there possibly any voice playing to be shutup */
static bool force_enqueue_next; /* enqueue next utterance even if enqueue is false */
static int queue_write; /* write index of queue, by application */
static int queue_read; /* read index of queue, by ISR context */
#if CONFIG_CODEC == SWCODEC
/* protects queue_read, queue_write and thumbnail_buf_used */
static struct mutex queue_mutex SHAREDBSS_ATTR; 
#define talk_queue_lock() ({ mutex_lock(&queue_mutex); })
#define talk_queue_unlock() ({ mutex_unlock(&queue_mutex); })
#else
#define talk_queue_lock() ({ })
#define talk_queue_unlock() ({ })
#endif /* CONFIG_CODEC */
static int sent; /* how many bytes handed over to playback, owned by ISR */
static unsigned char curr_hd[3]; /* current frame header, for re-sync */
static unsigned char last_lang[MAX_FILENAME+1]; /* name of last used lang file (in talk_init) */
static bool talk_initialized; /* true if talk_init has been called */
static bool give_buffer_away; /* true if we should give the buffers away in shrink_callback if requested */
static int talk_temp_disable_count; /* if positive, temporarily disable voice UI (not saved) */
 /* size of the voice data in the voice file and the actually allocated buffer
  * for it. voicebuf_size is always smaller or equal to voicefile_size */
static unsigned long voicefile_size, voicebuf_size;

struct queue_entry /* one entry of the internal queue */
{
    int handle;    /* buflib handle to the clip data */
    int length;    /* total length of the clip */
    int remaining; /* bytes that still need to be deoded */
};

static struct buflib_context clip_ctx;

struct clip_cache_metadata {
    long tick;
    int handle, voice_id;
};

static int metadata_table_handle;
static unsigned max_clips;
static int cache_hits, cache_misses;

static struct queue_entry queue[QUEUE_SIZE]; /* queue of scheduled clips */
static struct queue_entry silence, *last_clip;

/***************** Private implementation *****************/

static int index_handle, talk_handle;

static int move_callback(int handle, void *current, void *new)
{
    (void)handle; (void)current; (void)new;
    if (handle == talk_handle && !buflib_context_relocate(&clip_ctx, new))
        return BUFLIB_CB_CANNOT_MOVE;
    return BUFLIB_CB_OK;
}


static struct mutex read_buffer_mutex;


/* on HWCODEC only voice xor audio can be active at a time */
static bool check_audio_status(void)
{
#if CONFIG_CODEC != SWCODEC
    if (audio_status()) /* busy, buffer in use */
        return false;
    /* ensure playback is given up on the buffer */
    audio_hard_stop();
#endif
    return true;
}

/* ISR (mp3_callback()) must not run during moving of the clip buffer,
 * because the MAS may get out-of-sync */
static void sync_callback(int handle, bool sync_on)
{
    (void) handle;
    if (sync_on)
        mutex_lock(&read_buffer_mutex);
    else
        mutex_unlock(&read_buffer_mutex);
#if CONFIG_CPU == SH7034
    /* DMA must not interrupt during buffer move or commit_buffer copies
     * from inconsistent buflib buffer */
    if (sync_on)
        CHCR3 &= ~0x0001; /* disable the DMA (and therefore the interrupt also) */
    else
        CHCR3 |=  0x0001; /* re-enable the DMA */
#endif
}

static ssize_t read_to_handle_ex(int fd, struct buflib_context *ctx, int handle,
                                 int handle_offset, size_t count)
{
    unsigned char *buf;
    ssize_t ret;
    mutex_lock(&read_buffer_mutex);

    if (!ctx)
        buf = core_get_data(handle);
    else
        buf = buflib_get_data(ctx, handle);

    buf += handle_offset;
    ret = read(fd, buf, count);

    mutex_unlock(&read_buffer_mutex);

    return ret;
}

static ssize_t read_to_handle(int fd, int handle, int handle_offset, size_t count)
{
    return read_to_handle_ex(fd, NULL, handle, handle_offset, count);
}


static int shrink_callback(int handle, unsigned hints, void *start, size_t old_size)
{
    (void)start;(void)old_size;(void)hints;
    int *h;
#if (MAX_CLIP_BUFFER_SIZE < (MEMORYSIZE<<20) || (MEMORYSIZE > 2))
    /* on low-mem and when the voice buffer size is not limited (i.e.
     * on 2MB HWCODEC) we effectively own the entire buffer because
     * the voicefile takes up all RAM. This blocks other Rockbox parts
     * from allocating, especially during bootup. Therefore always give
     * up the buffer and reload when clips are played back. On high-mem
     * or when the clip buffer is limited to a few 100K this provision is
     * not necessary. */
    if (give_buffer_away
            && (hints & BUFLIB_SHRINK_POS_MASK) == BUFLIB_SHRINK_POS_MASK)
#endif
    {
        if (handle == talk_handle)
            h = &talk_handle;
        else //if (handle == index_handle)
            h = &index_handle;

        mutex_lock(&read_buffer_mutex);
        /* the clip buffer isn't usable without index table */
        if (handle == index_handle && talk_handle > 0)
            talk_handle = core_free(talk_handle);
        *h = core_free(handle);
        mutex_unlock(&read_buffer_mutex);

        return BUFLIB_CB_OK;
    }
    return BUFLIB_CB_CANNOT_SHRINK;
}

static struct buflib_callbacks talk_ops = {
    .move_callback = move_callback,
    .sync_callback = sync_callback,
    .shrink_callback = shrink_callback,
};


static int open_voicefile(void)
{
    char buf[64];
    char* p_lang = DEFAULT_VOICE_LANG; /* default */

    if ( global_settings.lang_file[0] &&
         global_settings.lang_file[0] != 0xff ) 
    {   /* try to open the voice file of the selected language */
        p_lang = (char *)global_settings.lang_file;
    }

    snprintf(buf, sizeof(buf), LANG_DIR "/%s.voice", p_lang);
    
    return open(buf, O_RDONLY);
}


static int id2index(int id)
{
    int index = id;
    if (id > VOICEONLY_DELIMITER)
    {   /* voice-only entries use the second part of the table.
           The first string comes after VOICEONLY_DELIMITER so we need to
           substract VOICEONLY_DELIMITER + 1 */
        index -= VOICEONLY_DELIMITER + 1;
        if (index >= voicefile.id2_max)
            return -1; /* must be newer than we have */
        index += voicefile.id1_max; /* table 2 is behind table 1 */
    }
    else
    {   /* normal use of the first table */
        if (id >= voicefile.id1_max)
            return -1; /* must be newer than we have */
    }
    return index;
}

#ifndef TALK_PROGRESSIVE_LOAD
static int index2id(int index)
{
    int id = index;

    if (index >= voicefile.id2_max + voicefile.id1_max)
        return -1;

    if (index >= voicefile.id1_max)
    {   /* must be voice-only if it exceeds table 1 */
        id -= voicefile.id1_max;
        /* The first string comes after VOICEONLY_DELIMITER so we need to
           add VOICEONLY_DELIMITER + 1 */
        id += VOICEONLY_DELIMITER + 1;
    }

    return id;
}
#endif

static int free_oldest_clip(void)
{
    unsigned i;
    int oldest = 0;
    long age, now;
    struct clip_entry* clipbuf;
    struct clip_cache_metadata *cc = buflib_get_data(&clip_ctx, metadata_table_handle);
    for(age = i = 0, now = current_tick; i < max_clips; i++)
    {
        if (cc[i].handle)
        {
            if ((now - cc[i].tick) > age && cc[i].voice_id != VOICE_PAUSE)
            {
                /* find the last-used clip but never consider silence */
                age = now - cc[i].tick;
                oldest = i;
            }
            else if (cc[i].voice_id == VOICEONLY_DELIMITER)
            {
                /* thumb clips are freed immediately */
                oldest = i;
                break;
            }
        }
    }
    /* free the last one if no oldest one could be determined */
    cc = &cc[oldest];
    cc->handle = buflib_free(&clip_ctx, cc->handle);
    /* need to clear the LOADED bit too (not for thumb clips) */
    if (cc->voice_id != VOICEONLY_DELIMITER)
    {
        clipbuf = core_get_data(index_handle);
        clipbuf[id2index(cc->voice_id)].size &= ~LOADED_MASK;
    }
    return oldest;
}


/* common code for load_initial_clips() and get_clip() */
static void add_cache_entry(int clip_handle, int table_index, int id)
{
    unsigned i;
    struct clip_cache_metadata *cc = buflib_get_data(&clip_ctx, metadata_table_handle);

    if (table_index != -1)
    {
        /* explicit slot; use that */
        cc = &cc[table_index];
        if (cc->handle > 0) panicf("%s(): Slot already used", __func__);
    }
    else
    {   /* find an empty slot */
        for(i = 0; cc[i].handle && i < max_clips; i++) ;
        if (i == max_clips) /* no free slot in the cache table? */
            i = free_oldest_clip();
        cc = &cc[i];
    }
    cc->handle = clip_handle;
    cc->tick = current_tick;
    cc->voice_id = id;
}

static ssize_t read_clip_data(int fd, int index, int clip_handle)
{
    struct clip_entry* clipbuf;
    size_t clipsize;
    ssize_t ret;

    if (fd < 0)
    {
        buflib_free(&clip_ctx, clip_handle);
        return -1; /* open error */
    }

    clipbuf = core_get_data(index_handle);
    /* this must not be called with LOADED_MASK set in clipsize */
    clipsize = clipbuf[index].size;
    lseek(fd, clipbuf[index].offset, SEEK_SET);
    ret = read_to_handle_ex(fd, &clip_ctx, clip_handle, 0, clipsize);

    if (ret < 0 || clipsize != (size_t)ret)
    {
        buflib_free(&clip_ctx, clip_handle);
        return -2; /* read error */
    }

    clipbuf = core_get_data(index_handle);
    clipbuf[index].size |= LOADED_MASK; /* mark as loaded */

    return ret;
}

static void load_initial_clips(int fd)
{
    (void) fd;
#ifndef TALK_PROGRESSIVE_LOAD
    unsigned index, i;
    unsigned num_clips = voicefile.id1_max + voicefile.id2_max;

    for(index = i = 0; index < num_clips && i < max_clips; index++)
    {
        int handle;
        struct clip_entry* clipbuf = core_get_data(index_handle);
        size_t clipsize = clipbuf[index].size;
        ssize_t ret;

        if (clipsize == 0) /* clip not included in voicefile */
            continue;

        handle = buflib_alloc(&clip_ctx, clipsize);
        if (handle < 0)
            break;

        ret = read_clip_data(fd, index, handle);
        if (ret < 0)
            break;

        add_cache_entry(handle, i++, index2id(index));
    }
#endif
}

/* fetch a clip from the voice file */
static int get_clip(long id, struct queue_entry *q)
{
    int index;
    int retval = -1;
    struct clip_entry* clipbuf;
    size_t clipsize;

    index = id2index(id);
    if (index == -1)
        return -1;

    clipbuf = core_get_data(index_handle);
    clipsize = clipbuf[index].size;
    if (clipsize == 0) /* clip not included in voicefile */
        return -1;

    if (!(clipsize & LOADED_MASK))
    {   /* clip needs loading */
        int fd, handle, oldest = -1;
        ssize_t ret;
        cache_misses++;
        /* free clips from cache until this one succeeds to allocate */
        while ((handle = buflib_alloc(&clip_ctx, clipsize)) < 0)
            oldest = free_oldest_clip();
        /* handle should now hold a valid alloc. Load from disk
         * and insert into cache */
        fd = open_voicefile();
        ret = read_clip_data(fd, index, handle);
        close(fd);
        if (ret < 0)
            return ret;
        /* finally insert into metadata table */
        add_cache_entry(handle, oldest, id);
        retval = handle;
    }
    else
    {   /* clip is in memory already; find where it was loaded */
        cache_hits++;
        struct clip_cache_metadata *cc;
        static int i;
        cc = buflib_get_data(&clip_ctx, metadata_table_handle);
        for (i = 0; cc[i].voice_id != id || !cc[i].handle; i++) ;
        cc[i].tick = current_tick; /* reset age */
        clipsize &= ~LOADED_MASK; /* without the extra bit gives true size */
        retval = cc[i].handle;
    }

    q->handle    = retval;
    q->length    = clipsize;
    q->remaining = clipsize;
    return 0;
}

static bool load_index_table(int fd, const struct voicefile_header *hdr)
{
    ssize_t ret;
    struct clip_entry *buf;

    if (index_handle > 0) /* nothing to do? */
        return true;

    ssize_t alloc_size = (hdr->id1_max + hdr->id2_max) * sizeof(struct clip_entry);
    index_handle = core_alloc_ex("voice index", alloc_size, &talk_ops);
    if (index_handle < 0)
        return false;

    ret = read_to_handle(fd, index_handle, 0, alloc_size);

    if (ret == alloc_size)
    {
        buf = core_get_data(index_handle);
        for (int i = 0; i < hdr->id1_max + hdr->id2_max; i++)
        {
#ifdef ROCKBOX_LITTLE_ENDIAN
            /* doesn't yield() */
            structec_convert(&buf[i], "ll", 1, true);
#endif
        }
    }
    else
        index_handle = core_free(index_handle);

    return ret == alloc_size;
}

static bool load_header(int fd, struct voicefile_header *hdr)
{
    ssize_t got_size = read(fd, hdr, sizeof(*hdr));
    if (got_size != sizeof(*hdr))
        return false;

#ifdef ROCKBOX_LITTLE_ENDIAN
    logf("Byte swapping voice file");
    structec_convert(&voicefile, "lllll", 1, true);
#endif
    return true;
}

static bool create_clip_buffer(size_t max_size)
{
    size_t alloc_size;
    /* just allocate, populate on an as-needed basis later */
    talk_handle = core_alloc_ex("voice data", max_size, &talk_ops);
    if (talk_handle < 0)
        goto alloc_err;

    buflib_init(&clip_ctx, core_get_data(talk_handle), max_size);

    /* the first alloc is the clip metadata table */
    alloc_size = max_clips * sizeof(struct clip_cache_metadata);
    metadata_table_handle = buflib_alloc(&clip_ctx, alloc_size);
    memset(buflib_get_data(&clip_ctx, metadata_table_handle), 0, alloc_size);

    return true;

alloc_err:
    index_handle = core_free(index_handle);
    return false;
}

/* load the voice file into the mp3 buffer */
static bool load_voicefile_index(int fd)
{
    if (fd < 0) /* failed to open */
        return false;

    /* load the header first */
    if (!load_header(fd, &voicefile))
        return false;

    /* format check */
    if (voicefile.table == sizeof(struct voicefile_header))
    {
        if (voicefile.version == VOICE_VERSION &&
            voicefile.target_id == TARGET_ID)
        {
            if (load_index_table(fd, &voicefile))
                return true;
        }
    }

    logf("Incompatible voice file");
    return false;
}

/* this function caps the voicefile buffer and allocates it. It can
 * be called after talk_init(), e.g. when the voice was temporarily disabled.
 * The buffer size has to be capped again each time because the available
 * audio buffer changes over time */
static bool load_voicefile_data(int fd)
{
    voicebuf_size = voicefile_size;
    /* cap to the max. number of clips or the size of the available audio
     * buffer which we grab. We leave some to the rest of the system.
     * While that reduces our buffer size it improves the chance that
     * other allocs succeed without disabling voice which would require
     * reloading the voice from disk (as we do not shrink our buffer when
     * other code attempts new allocs these would fail) */
    ssize_t cap = MIN(MAX_CLIP_BUFFER_SIZE, audio_buffer_available() - (64<<10));
    if (UNLIKELY(cap < 0))
    {
        logf("Not enough memory for voice. Disabling...\n");
        return false;
    }
    else if (voicebuf_size > (size_t)cap)
        voicebuf_size = cap;

    /* just allocate, populate on an as-needed basis later
     * re-create the clip buffer to ensure clip_ctx is up-to-date */
    if (talk_handle > 0)
        talk_handle = core_free(talk_handle);
    if (!create_clip_buffer(voicebuf_size))
        return false;

    load_initial_clips(fd);
    /* make sure to have the silence clip, if available return value can
     * be cached globally even for TALK_PROGRESSIVE_LOAD because the
     * VOICE_PAUSE clip is specially handled */
    get_clip(VOICE_PAUSE, &silence);

    return true;
}

/* Use a static buffer to avoid difficulties with buflib during DMA
 * (hwcodec)/buffer passing to the voice_thread (swcodec). Clips
 * can be played in chunks so the size is not that important */
static unsigned char commit_buffer[2<<10];

static void* commit_transfer(struct queue_entry *qe, size_t *size)
{
    void *buf = NULL; /* shut up gcc */
    static unsigned char *bufpos = commit_buffer;
#if CONFIG_CODEC != SWCODEC
    sent = MIN(qe->remaining, 0xFFFF);
#else
    sent = qe->remaining;
#endif
    sent = MIN((size_t)sent, sizeof(commit_buffer));
    buf = buflib_get_data(&clip_ctx, qe->handle);
    /* adjust buffer position to what has been played already */
    buf += (qe->length - qe->remaining);
    memcpy(bufpos, buf, sent);
    *size = sent;

    return commit_buffer;
}

static inline bool is_silence(struct queue_entry *qe)
{
    if (silence.handle > 0) /* silence clip available? */
        return (qe->handle == silence.handle);
    else
        return false;
}

/* called in ISR context (on HWCODEC) if mp3 data got consumed */
static void mp3_callback(const void** start, size_t* size)
{
    struct queue_entry *qe = &queue[queue_read];
#if CONFIG_CODEC == SWCODEC
    /* voice_thread.c hints us how many of the buffer we provided it actually
     * consumed. Because buffers have to be frame-aligned for speex
     * it might be less than what we presented */
    if (*size)
        sent = *size;
#endif
    qe->remaining -= sent; /* we completed this */

    if (qe->remaining > 0) /* current clip not finished? */
    {   /* feed the next 64K-1 chunk */
        *start = commit_transfer(qe, size);
        return;
    }

    talk_queue_lock();

    /* increment read position for the just played clip */
    queue_read = (queue_read + 1) & QUEUE_MASK;

    if (QUEUE_LEVEL == 0)
    {
        if (!is_silence(last_clip))
        {   /* add silence clip when queue runs empty playing a voice clip,
             * only if the previous clip wasn't already silence */
            queue[queue_write] = silence;
            queue_write = (queue_write + 1) & QUEUE_MASK;
        }
        else
        {
            *size = 0; /* end of data */
        }
    }

    if (QUEUE_LEVEL != 0) /* queue is not empty? */
    {   /* start next clip */
        last_clip = &queue[queue_read];
        *start = commit_transfer(last_clip, size);
        curr_hd[0] = commit_buffer[1];
        curr_hd[1] = commit_buffer[2];
        curr_hd[2] = commit_buffer[3];
    }
    
    talk_queue_unlock();
}

/***************** Public routines *****************/

/* stop the playback and the pending clips */
void talk_force_shutup(void)
{
    /* Most of this is MAS only */
#if CONFIG_CODEC != SWCODEC
#ifdef SIMULATOR
    return;
#endif
    unsigned char* pos;
    unsigned char* search;
    unsigned char* end;
    int len;
    if (QUEUE_LEVEL == 0) /* has ended anyway */
        return;

#if CONFIG_CPU == SH7034
    CHCR3 &= ~0x0001; /* disable the DMA (and therefore the interrupt also) */
#endif /* CONFIG_CPU == SH7034 */
    /* search next frame boundary and continue up to there */
    pos = search = mp3_get_pos();
    end = buflib_get_data(&clip_ctx, queue[queue_read].handle);
    len = queue[queue_read].length;

    if (pos >= end && pos <= (end+len)) /* really our clip? */
    { /* (for strange reasons this isn't nesessarily the case) */
        /* find the next frame boundary */
        while (search < (end+len)) /* search the remaining data */
        {
            if (*search++ != 0xFF) /* quick search for frame sync byte */
                continue; /* (this does the majority of the job) */
            
            /* look at the (bitswapped) rest of header candidate */
            if (search[0] == curr_hd[0] /* do the quicker checks first */
             && search[2] == curr_hd[2]
             && (search[1] & 0x30) == (curr_hd[1] & 0x30)) /* sample rate */
            {
                search--; /* back to the sync byte */
                break; /* From looking at it, this is our header. */
            }
        }
    
        if (search-pos)
        {   /* play old data until the frame end, to keep the MAS in sync */
            sent = search-pos;

            queue_write = (queue_read + 1) & QUEUE_MASK; /* will be empty after next callback */
            queue[queue_read].length = sent; /* current one ends after this */

#if CONFIG_CPU == SH7034
            DTCR3 = sent; /* let the DMA finish this frame */
            CHCR3 |= 0x0001; /* re-enable DMA */
#endif /* CONFIG_CPU == SH7034 */
            thumbnail_buf_used = 0;
            return;
        }
    }
#endif /* CONFIG_CODEC != SWCODEC */

    /* Either SWCODEC, or MAS had nothing to do (was frame boundary or not our clip) */
    mp3_play_stop();
    talk_queue_lock();
    queue_write = queue_read = 0; /* reset the queue */
    thumbnail_buf_used = 0;
    talk_queue_unlock();
    need_shutup = false;
}

/* Shutup the voice, except if force_enqueue_next is set. */
void talk_shutup(void)
{
    if (need_shutup && !force_enqueue_next)
        talk_force_shutup();
}

/* schedule a clip, at the end or discard the existing queue */
static void queue_clip(struct queue_entry *clip, bool enqueue)
{
    struct queue_entry *qe;
    int queue_level;

    if (!enqueue)
        talk_shutup(); /* cut off all the pending stuff */
    /* Something is being enqueued, force_enqueue_next override is no
       longer in effect. */
    force_enqueue_next = false;
    
    if (!clip->length)
        return; /* safety check */
#if CONFIG_CPU == SH7034
    /* disable the DMA temporarily, to be safe of race condition */
    CHCR3 &= ~0x0001;
#endif
    talk_queue_lock();
    queue_level = QUEUE_LEVEL; /* check old level */
    qe = &queue[queue_write];

    if (queue_level < QUEUE_SIZE - 1) /* space left? */
    {
        queue[queue_write] = *clip;
        queue_write = (queue_write + 1) & QUEUE_MASK;
    }
    talk_queue_unlock();

    if (queue_level == 0)
    {   /* queue was empty, we have to do the initial start */
        size_t size;
        void *buf = commit_transfer(qe, &size);
        last_clip = qe;
        mp3_play_data(buf, size, mp3_callback);
        curr_hd[0] = commit_buffer[1];
        curr_hd[1] = commit_buffer[2];
        curr_hd[2] = commit_buffer[3];
        mp3_play_pause(true); /* kickoff audio */
    }
    else
    {
#if CONFIG_CPU == SH7034
        CHCR3 |= 0x0001; /* re-enable DMA */
#endif
    }

    need_shutup = true;

    return;
}

/* return if a voice codec is required or not */
static bool talk_voice_required(void)
{
    return (has_voicefile) /* Voice file is available */
        || (global_settings.talk_dir_clip)  /* Thumbnail clips are required */
        || (global_settings.talk_file_clip);
}

/***************** Public implementation *****************/

void talk_init(void)
{
    int filehandle;
    talk_temp_disable_count = 0;
    if (talk_initialized && !strcasecmp(last_lang, global_settings.lang_file))
    {
        /* not a new file, nothing to do */
        return;
    }

    if(!talk_initialized)
    {
#if CONFIG_CODEC == SWCODEC
        mutex_init(&queue_mutex);
#endif /* CONFIG_CODEC == SWCODEC */
        mutex_init(&read_buffer_mutex);
    }
    talk_initialized = true;
    strlcpy((char *)last_lang, (char *)global_settings.lang_file,
        MAX_FILENAME);

    /* reset some states */
    queue_write = queue_read = 0; /* reset the queue */
    memset(&voicefile, 0, sizeof(voicefile));

    silence.handle = -1; /* pause clip not accessible */
    voicefile_size = has_voicefile = 0;
    /* need to free these as their size depends on the voice file, and
     * this function is called when the talk voice file changes */
    if (index_handle > 0) index_handle = core_free(index_handle);
    if (talk_handle  > 0) talk_handle  = core_free(talk_handle);
    /* don't free thumb handle, it doesn't depend on the actual voice file
     * and so we can re-use it if it's already allocated in any event */

    filehandle = open_voicefile();
    if (filehandle > 0)
    {
        if (!load_voicefile_index(filehandle))
            goto out;

        /* Now determine the maximum buffer size needed for the voicefile.
         * The below pretends the entire voicefile would be loaded. The buffer
         * size is eventually capped later on in load_voicefile_data() */
        int num_clips = voicefile.id1_max + voicefile.id2_max;
        int non_empty = num_clips;
        int total_size = 0, avg_size;
        struct clip_entry *clips = core_get_data(index_handle);
        /* check for the average clip size to estimate the maximum number of
         * clips the buffer can hold */
        for (int i = 0; i<num_clips; i++)
        {
            if (clips[i].size)
                total_size += ALIGN_UP(clips[i].size, sizeof(void *));
            else
                non_empty -= 1;
        }
        avg_size = total_size / non_empty;
        max_clips = MIN((int)(MAX_CLIP_BUFFER_SIZE/avg_size) + 1, non_empty);
        /* account for possible thumb clips */        
        total_size += THUMBNAIL_RESERVE;
        max_clips += 16;
        voicefile_size = total_size;
        has_voicefile = true;
    }
    else if (talk_voice_required())
    {
        /* create buffer just for thumb clips */
        max_clips = 16;
        voicefile_size = THUMBNAIL_RESERVE;
    }
    /* additionally to the clip we need a table to record the age of the clips
     * so that, when memory is tight, only the most recently used ones are kept */
    voicefile_size += sizeof(struct clip_cache_metadata) * max_clips;
    /* compensate a bit for buflib alloc overhead. */
    voicefile_size += BUFLIB_ALLOC_OVERHEAD * max_clips + 64;

    load_voicefile_data(filehandle);

#if CONFIG_CODEC == SWCODEC
    /* Initialize the actual voice clip playback engine as well */
    if (talk_voice_required())
        voice_thread_init();
#endif

out:
    close(filehandle); /* close again, this was just to detect presence */
}

/* somebody else claims the mp3 buffer, e.g. for regular play/record */
void talk_buffer_set_policy(int policy)
{
    switch(policy)
    {
        case TALK_BUFFER_DEFAULT:
        case TALK_BUFFER_HOLD:  give_buffer_away = false;            break;
        case TALK_BUFFER_LOOSE: give_buffer_away = true;             break;
        default:           DEBUGF("Ignoring unknown policy\n"); break;
    }
}

/* play a voice ID from voicefile */
int talk_id(int32_t id, bool enqueue)
{
    int32_t unit;
    int decimals;
    struct queue_entry clip;

    if (!has_voicefile)
        return 0; /* no voicefile loaded, not an error -> pretent success */
    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
    if (!check_audio_status())
        return -1;

    if (talk_handle <= 0 || index_handle <= 0) /* reload needed? */
    {
        int fd = open_voicefile();
        if (fd < 0 || !load_voicefile_index(fd))
            return -1;
        load_voicefile_data(fd);
        close(fd);
    }

    if (id == -1) /* -1 is an indication for silence */
        return -1;

    decimals = (((uint32_t)id) >> DECIMAL_SHIFT) & 0x7;

    /* check if this is a special ID, with a value */
    unit = ((uint32_t)id) >> UNIT_SHIFT;
    if (unit || decimals)
    {   /* sign-extend the value */
        id = (uint32_t)id << (32-DECIMAL_SHIFT);
        id >>= (32-DECIMAL_SHIFT);

        talk_value_decimal(id, unit, decimals, enqueue); /* speak it */
        return 0; /* and stop, end of special case */
    }

    if (get_clip(id, &clip) < 0)
        return -1; /* not present */

#ifdef LOGF_ENABLE
    if (id > VOICEONLY_DELIMITER)
        logf("\ntalk_id: Say voice clip 0x%x\n", id);
    else
        logf("\ntalk_id: Say '%s'\n", str(id));
#endif

    queue_clip(&clip, enqueue);

    return 0;
}
/* Speaks zero or more IDs (from an array). */
int talk_idarray(const long *ids, bool enqueue)
{
    int r;
    if(!ids)
        return 0;
    while(*ids != TALK_FINAL_ID)
    {
        if((r = talk_id(*ids++, enqueue)) <0)
            return r;
        enqueue = true;
    }
    return 0;
}

/* Make sure the current utterance is not interrupted by the next one. */
void talk_force_enqueue_next(void)
{
    force_enqueue_next = true;
}

/* play a thumbnail from file */
/* Returns size of spoken thumbnail, so >0 means something is spoken,
   <=0 means something went wrong. */
static int _talk_file(const char* filename,
                      const long *prefix_ids, bool enqueue)
{
    int fd;
    int size;
    int handle, oldest = -1;
#if CONFIG_CODEC != SWCODEC
    struct mp3entry info;
#endif

    /* reload needed? */
    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
    if (!check_audio_status())
        return -1;
    if (talk_handle <= 0)
        load_voicefile_data(-1);

#if CONFIG_CODEC != SWCODEC
    if(mp3info(&info, filename)) /* use this to find real start */
    {   
        return 0; /* failed to open, or invalid */
    }
#endif

    if (!enqueue)
        /* shutup now to free the thumbnail buffer */
        talk_shutup();

    fd = open(filename, O_RDONLY);
    if (fd < 0) /* failed to open */
    {
        return 0;
    }
    size = filesize(fd);

#if CONFIG_CODEC != SWCODEC
    size -= lseek(fd, info.first_frame_offset, SEEK_SET); /* behind ID data */
#endif

    /* free clips from cache until this one succeeds to allocate */
    while ((handle = buflib_alloc(&clip_ctx, size)) < 0)
        oldest = free_oldest_clip();

    size = read_to_handle_ex(fd, &clip_ctx, handle, 0, size);
    close(fd);

    /* ToDo: find audio, skip ID headers and trailers */

    if (size > 0)    /* Don't play missing clips */
    {
        struct queue_entry clip;
        clip.handle = handle;
        clip.length = clip.remaining = size;
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
        /* bitswap doesnt yield() */
        bitswap(buflib_get_data(&clip_ctx, handle), size);
#endif
        if(prefix_ids)
            /* prefix thumbnail by speaking these ids, but only now
               that we know there's actually a thumbnail to be
               spoken. */
            talk_idarray(prefix_ids, true);
        /* finally insert into metadata table. thumb clips go under the
         * VOICEONLY_DELIMITER id so the cache can distinguish them from
         * normal clips */
        add_cache_entry(handle, oldest, VOICEONLY_DELIMITER);
        queue_clip(&clip, true);
    }
    else
        buflib_free(&clip_ctx, handle);

    return size;
}

int talk_file(const char *root, const char *dir, const char *file,
              const char *ext, const long *prefix_ids, bool enqueue)
/* Play a thumbnail file */
{
    char buf[MAX_PATH];
    /* Does root end with a slash */
    char *slash = (root && root[0]
                   && root[strlen(root)-1] != '/') ? "/" : "";
    snprintf(buf, MAX_PATH, "%s%s%s%s%s%s",
             root ? root : "", slash,
             dir ? dir : "", dir ? "/" : "",
             file ? file : "",
             ext ? ext : "");
    return _talk_file(buf, prefix_ids, enqueue);
}

static int talk_spell_basename(const char *path,
                               const long *prefix_ids, bool enqueue)
{
    if(prefix_ids)
    {
        talk_idarray(prefix_ids, enqueue);
        enqueue = true;
    }
    char buf[MAX_PATH];
    /* Spell only the path component after the last slash */
    strlcpy(buf, path, sizeof(buf));
    if(strlen(buf) >1 && buf[strlen(buf)-1] == '/')
        /* strip trailing slash */
        buf[strlen(buf)-1] = '\0';
    char *ptr = strrchr(buf, '/');
    if(ptr && strlen(buf) >1)
        ++ptr;
    else ptr = buf;
    return talk_spell(ptr, enqueue);
}

/* Play a file's .talk thumbnail, fallback to spelling the filename, or
   go straight to spelling depending on settings. */
int talk_file_or_spell(const char *dirname, const char *filename,
                       const long *prefix_ids, bool enqueue)
{
    if (global_settings.talk_file_clip)
    {   /* .talk clips enabled */
        if(talk_file(dirname, NULL, filename, file_thumbnail_ext,
                              prefix_ids, enqueue) >0)
            return 0;
    }
    if (global_settings.talk_file == 2)
        /* Either .talk clips are disabled, or as a fallback */
        return talk_spell_basename(filename, prefix_ids, enqueue);
    return 0;
}

#if CONFIG_CODEC == SWCODEC
/* Play a directory's .talk thumbnail, fallback to spelling the filename, or
   go straight to spelling depending on settings. */
int talk_dir_or_spell(const char* dirname,
                      const long *prefix_ids, bool enqueue)
{
    if (global_settings.talk_dir_clip)
    {   /* .talk clips enabled */
        if(talk_file(dirname, NULL, dir_thumbnail_name, NULL,
                              prefix_ids, enqueue) >0)
            return 0;
    }
    if (global_settings.talk_dir == 2)
        /* Either .talk clips disabled or as a fallback */
        return talk_spell_basename(dirname, prefix_ids, enqueue);
    return 0;
}
#endif

/* Speak thumbnail for each component of a full path, again falling
   back or going straight to spelling depending on settings. */
int talk_fullpath(const char* path, bool enqueue)
{
    if (!enqueue)
        talk_shutup();
    if(path[0] != '/')
        /* path ought to start with /... */
        return talk_spell(path, true);
    talk_id(VOICE_CHAR_SLASH, true);
    char buf[MAX_PATH];
    strlcpy(buf, path, MAX_PATH);
    char *start = buf+1; /* start of current component */
    char *ptr = strchr(start, '/'); /* end of current component */
    while(ptr) { /* There are more slashes ahead */
        /* temporarily poke a NULL at end of component to truncate string */
        *ptr = '\0';
        talk_dir_or_spell(buf, NULL, true);
        *ptr = '/'; /* restore string */
        talk_id(VOICE_CHAR_SLASH, true);
        start = ptr+1; /* setup for next component */
        ptr = strchr(start, '/');
    }
    /* no more slashes, final component is a filename */
    return talk_file_or_spell(NULL, buf, NULL, true);
}

/* say a numeric value, this word ordering works for english,
   but not necessarily for other languages (e.g. german) */
int talk_number(long n, bool enqueue)
{
    int level = 2; /* mille count */
    long mil = 1000000000; /* highest possible "-illion" */

    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
    if (!check_audio_status())
        return -1;

    if (!enqueue)
        talk_shutup(); /* cut off all the pending stuff */
    
    if (n==0)
    {   /* special case */
        talk_id(VOICE_ZERO, true);
        return 0;
    }
    
    if (n<0)
    {
        talk_id(VOICE_MINUS, true);
        n = -n;
    }
    
    while (n)
    {
        int segment = n / mil; /* extract in groups of 3 digits */
        n -= segment * mil; /* remove the used digits from number */
        mil /= 1000; /* digit place for next round */

        if (segment)
        {
            int hundreds = segment / 100;
            int ones = segment % 100;

            if (hundreds)
            {
                talk_id(VOICE_ZERO + hundreds, true);
                talk_id(VOICE_HUNDRED, true);
            }

            /* combination indexing */
            if (ones > 20)
            {
               int tens = ones/10 + 18;
               talk_id(VOICE_ZERO + tens, true);
               ones %= 10;
            }

            /* direct indexing */
            if (ones)
                talk_id(VOICE_ZERO + ones, true);
 
            /* add billion, million, thousand */
            if (mil)
                talk_id(VOICE_THOUSAND + level, true);
        }
        level--;
    }

    return 0;
}

/* Say year like "nineteen ninety nine" instead of "one thousand 9
   hundred ninety nine". */
static int talk_year(long year, bool enqueue)
{
    int rem;
    if(year < 1100 || year >=2000)
        /* just say it as a regular number */
        return talk_number(year, enqueue);
    /* Say century */
    talk_number(year/100, enqueue);
    rem = year%100;
    if(rem == 0)
        /* as in 1900 */
        return talk_id(VOICE_HUNDRED, true);
    if(rem <10)
        /* as in 1905 */
        talk_id(VOICE_ZERO, true);
    /* sub-century year */
    return talk_number(rem, true);
}

/* Say time duration/interval. Input is time in seconds,
   say hours,minutes,seconds. */
static int talk_time_unit(long secs, bool enqueue)
{
    int hours, mins;
    if (!enqueue)
        talk_shutup();
    if((hours = secs/3600)) {
        secs %= 3600;
        talk_value(hours, UNIT_HOUR, true);
    }
    if((mins = secs/60)) {
        secs %= 60;
        talk_value(mins, UNIT_MIN, true);
    }
    if((secs) || (!hours && !mins))
        talk_value(secs, UNIT_SEC, true);
    else if(!hours && secs)
        talk_number(secs, true);
    return 0;
}

void talk_fractional(char *tbuf, int value, int unit)
{
    int i;
    /* strip trailing zeros from the fraction */
    for (i = strlen(tbuf) - 1; (i >= 0) && (tbuf[i] == '0'); i--)
        tbuf[i] = '\0';

    talk_number(value, true);
    if (tbuf[0] != 0)
    {
        talk_id(LANG_POINT, true);
        talk_spell(tbuf, true);
    }
    talk_id(unit, true);
}

int talk_value(long n, int unit, bool enqueue)
{
    return talk_value_decimal(n, unit, 0, enqueue);
}

/* singular/plural aware saying of a value */
int talk_value_decimal(long n, int unit, int decimals, bool enqueue)
{
    int unit_id;
    static const int unit_voiced[] = 
    {   /* lookup table for the voice ID of the units */
        [0 ... UNIT_LAST-1] = -1, /* regular ID, int, signed */
        [UNIT_MS]
            = VOICE_MILLISECONDS, /* here come the "real" units */
        [UNIT_SEC]
            = VOICE_SECONDS, 
        [UNIT_MIN]
            = VOICE_MINUTES, 
        [UNIT_HOUR]
            = VOICE_HOURS, 
        [UNIT_KHZ]
            = VOICE_KHZ, 
        [UNIT_DB]
            = VOICE_DB, 
        [UNIT_PERCENT]
            = VOICE_PERCENT,
        [UNIT_MAH]
            = VOICE_MILLIAMPHOURS,
        [UNIT_PIXEL]
            = VOICE_PIXEL,
        [UNIT_PER_SEC]
            = VOICE_PER_SEC,
        [UNIT_HERTZ]
            = VOICE_HERTZ,
        [UNIT_MB]
            = LANG_MEGABYTE,
        [UNIT_KBIT]
            = VOICE_KBIT_PER_SEC,
        [UNIT_PM_TICK]
            = VOICE_PM_UNITS_PER_TICK,
    };

    static const int pow10[] = { /* 10^0 - 10^7 */
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
    };

    char tbuf[8];
    char fmt[] = "%0nd";

    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
    if (!check_audio_status())
        return -1;

    /* special pronounciation for year number */
    if (unit == UNIT_DATEYEAR)
        return talk_year(n, enqueue);
    /* special case for time duration */
    if (unit == UNIT_TIME)
        return talk_time_unit(n, enqueue);

    if (unit < 0 || unit >= UNIT_LAST)
        unit_id = -1;
    else
        unit_id = unit_voiced[unit];

    if ((n==1 || n==-1) /* singular? */
        && unit_id >= VOICE_SECONDS && unit_id <= VOICE_HOURS)
    {
        unit_id--; /* use the singular for those units which have */
    }

    /* special case with a "plus" before */
    if (n > 0 && (unit == UNIT_SIGNED || unit == UNIT_DB))
    {
        talk_id(VOICE_PLUS, enqueue);
        enqueue = true;
    }

    if (decimals)
    {
        /* needed for the "-0.5" corner case */
        if (n < 0)
        {
            talk_id(VOICE_MINUS, enqueue);
            n = -n;
        }

        fmt[2] = '0' + decimals;

        snprintf(tbuf, sizeof(tbuf), fmt, n % pow10[decimals]);
        talk_fractional(tbuf, n / pow10[decimals], unit_id);

        return 0;
    }

    talk_number(n, enqueue); /* say the number */
    talk_id(unit_id, true); /* say the unit, if any */

    return 0;
}

/* spell a string */
int talk_spell(const char* spell, bool enqueue)
{
    char c; /* currently processed char */
    
    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
    if (!check_audio_status())
        return -1;

    if (!enqueue)
        talk_shutup(); /* cut off all the pending stuff */
    
    while ((c = *spell++) != '\0')
    {
        /* if this grows into too many cases, I should use a table */
        if (c >= 'A' && c <= 'Z')
            talk_id(VOICE_CHAR_A + c - 'A', true);
        else if (c >= 'a' && c <= 'z')
            talk_id(VOICE_CHAR_A + c - 'a', true);
        else if (c >= '0' && c <= '9')
            talk_id(VOICE_ZERO + c - '0', true);
        else if (c == '-')
            talk_id(VOICE_MINUS, true);
        else if (c == '+')
            talk_id(VOICE_PLUS, true);
        else if (c == '.')
            talk_id(VOICE_DOT, true); 
        else if (c == ' ')
            talk_id(VOICE_PAUSE, true);
        else if (c == '/')
            talk_id(VOICE_CHAR_SLASH, true);
    }

    return 0;
}

void talk_disable(bool disable)
{
    if (disable)
        talk_temp_disable_count++;
    else 
        talk_temp_disable_count--;
}

void talk_setting(const void *global_settings_variable)
{
    const struct settings_list *setting;
    if (!global_settings.talk_menu)
        return;
    setting = find_setting(global_settings_variable, NULL);
    if (setting == NULL)
        return;
    if (setting->lang_id)
        talk_id(setting->lang_id,false);
}


#if CONFIG_RTC
void talk_date(const struct tm *tm, bool enqueue)
{
    talk_id(LANG_MONTH_JANUARY + tm->tm_mon, enqueue);
    talk_number(tm->tm_mday, true);
    talk_number(1900 + tm->tm_year, true);
}

void talk_time(const struct tm *tm, bool enqueue)
{
    if (global_settings.timeformat == 1)
    {
        /* Voice the hour */
        long am_pm_id = VOICE_AM;
        int hour = tm->tm_hour;
        if (hour >= 12)
        {
            am_pm_id = VOICE_PM;
            hour -= 12;
        }
        if (hour == 0)
            hour = 12;
        talk_number(hour, enqueue);

        /* Voice the minutes */
        if (tm->tm_min == 0)
        {
            /* Say o'clock if the minute is 0. */
            talk_id(VOICE_OCLOCK, true);
        }
        else
        {
            /* Pronounce the leading 0 */
            if(tm->tm_min < 10)
                talk_id(VOICE_OH, true);
            talk_number(tm->tm_min, true);
        }
        talk_id(am_pm_id, true);
    }
    else
    {
        /* Voice the time in 24 hour format */
        talk_number(tm->tm_hour, enqueue);
        if (tm->tm_min == 0)
            talk_ids(true, VOICE_HUNDRED, VOICE_HOUR);
        else
        {
            /* Pronounce the leading 0 */
            if(tm->tm_min < 10)
                talk_id(VOICE_OH, true);
            talk_number(tm->tm_min, true);
        }
    }
}

#endif /* CONFIG_RTC */

bool talk_get_debug_data(struct talk_debug_data *data)
{
    char* p_lang = DEFAULT_VOICE_LANG; /* default */
    struct clip_cache_metadata *cc;

    memset(data, 0, sizeof(*data));

    if (!has_voicefile || index_handle <= 0)
        return false;

    if (global_settings.lang_file[0] && global_settings.lang_file[0] != 0xff)
        p_lang = (char *)global_settings.lang_file;

    struct clip_entry *clips = core_get_data(index_handle);
    int cached = 0;
    int real_clips = 0;

    strlcpy(data->voicefile, p_lang, sizeof(data->voicefile));
    data->num_clips = voicefile.id1_max + voicefile.id2_max;
    data->avg_clipsize = data->max_clipsize = 0;
    data->min_clipsize = INT_MAX;
    for(int i = 0; i < data->num_clips; i++)
    {
        int size = clips[i].size & (~LOADED_MASK);
        if (!size) continue;
        real_clips += 1;
        if (size < data->min_clipsize)
            data->min_clipsize = size;
        if (size > data->max_clipsize)
            data->max_clipsize = size;
        data->avg_clipsize += size;
    }
    cc = buflib_get_data(&clip_ctx, metadata_table_handle);
    for (int i = 0; i < (int) max_clips; i++)
    {
        if (cc[i].handle > 0)
            cached += 1;
    }
    data->avg_clipsize /= real_clips;
    data->num_empty_clips = data->num_clips - real_clips;
    data->memory_allocated = sizeof(commit_buffer) + sizeof(voicefile)
                         + data->num_clips * sizeof(struct clip_entry)
                         + voicebuf_size;
    data->memory_used = 0;
    if (talk_handle > 0)
        data->memory_used = data->memory_allocated - buflib_available(&clip_ctx);
    data->cached_clips = cached;
    data->cache_hits   = cache_hits;
    data->cache_misses = cache_misses;

    return true;
}
