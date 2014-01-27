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

/* swcodec: cap thumbnail buffer to MAX_THUMNAIL_BUFSIZE since audio keeps
 * playing while voice
 * hwcodec: just use whatever is left in the audiobuffer, music
 * playback is impossible => no cap */
#if CONFIG_CODEC == SWCODEC    
#define MAX_THUMBNAIL_BUFSIZE 0x10000
#endif

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
#define TALK_PARTIAL_LOAD
#endif

#ifdef TALK_PARTIAL_LOAD
static long           max_clipsize; /* size of the biggest clip */
static long           buffered_id[QUEUE_SIZE];  /* IDs of the talk clips */
static uint8_t        clip_age[QUEUE_SIZE];
#if QUEUE_SIZE > 255
#   error clip_age[] type too small
#endif
#endif

/* Multiple thumbnails can be loaded back-to-back in this buffer. */
static volatile int thumbnail_buf_used SHAREDBSS_ATTR; /* length of data in
                                                          thumbnail buffer */
static long size_for_thumbnail; /* total thumbnail buffer size */
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
static int silence_offset; /* VOICE_PAUSE clip, used for termination */
static long silence_length; /* length of the VOICE_PAUSE clip */
static unsigned long lastclip_offset; /* address of latest clip, for silence add */
static unsigned char last_lang[MAX_FILENAME+1]; /* name of last used lang file (in talk_init) */
static bool talk_initialized; /* true if talk_init has been called */
static bool give_buffer_away; /* true if we should give the buffers away in shrink_callback if requested */
static int talk_temp_disable_count; /* if positive, temporarily disable voice UI (not saved) */
 /* size of the loaded voice file
  * offsets smaller than this denote a clip from teh voice file,
  * offsets larger than this denote a thumbnail clip */
static unsigned long voicefile_size;

struct queue_entry /* one entry of the internal queue */
{
    int offset, length;
};

static struct queue_entry queue[QUEUE_SIZE]; /* queue of scheduled clips */


/***************** Private implementation *****************/

static int thumb_handle;
static int talk_handle, talk_handle_locked;

#if CONFIG_CODEC != SWCODEC

/* on HWCODEC only voice xor audio can be active at a time */
static bool check_audio_status(void)
{
    if (audio_status()) /* busy, buffer in use */
        return false;
    /* ensure playback is given up on the buffer */
    audio_hard_stop();
    return true;
}

/* ISR (mp3_callback()) must not run during moving of the clip buffer,
 * because the MAS may get out-of-sync */
static void sync_callback(int handle, bool sync_on)
{
    (void) handle;
    (void) sync_on;
#if CONFIG_CPU == SH7034
    if (sync_on)
        CHCR3 &= ~0x0001; /* disable the DMA (and therefore the interrupt also) */
    else
        CHCR3 |=  0x0001; /* re-enable the DMA */
#endif
}
#else
#define check_audio_status() (true)
#endif

static int move_callback(int handle, void *current, void *new)
{
    (void)handle;(void)current;(void)new;
    if (UNLIKELY(talk_handle_locked))
        return BUFLIB_CB_CANNOT_MOVE;
    return BUFLIB_CB_OK;
}

static int clip_shrink_callback(int handle, unsigned hints, void *start, size_t old_size)
{
    (void)start;(void)old_size;

    if (LIKELY(!talk_handle_locked)
            && give_buffer_away
            && (hints & BUFLIB_SHRINK_POS_MASK) == BUFLIB_SHRINK_POS_MASK)
    {
        talk_handle = core_free(handle);
        return BUFLIB_CB_OK;
    }
    return BUFLIB_CB_CANNOT_SHRINK;
}

static int thumb_shrink_callback(int handle, unsigned hints, void *start, size_t old_size)
{
    (void)start;(void)old_size;(void)hints;

    /* be generous about the thumbnail buffer unless currently used */
    if (LIKELY(!talk_handle_locked) && thumbnail_buf_used == 0)
    {
        thumb_handle = core_free(handle);
        return BUFLIB_CB_OK;
    }
    return BUFLIB_CB_CANNOT_SHRINK;
}

static struct buflib_callbacks clip_ops = {
    .move_callback = move_callback,
#if CONFIG_CODEC != SWCODEC
    .sync_callback = sync_callback,
#endif
    .shrink_callback = clip_shrink_callback,
};

static struct buflib_callbacks thumb_ops = {
    .move_callback = move_callback,
#if CONFIG_CODEC != SWCODEC
    .sync_callback = sync_callback,
#endif
    .shrink_callback = thumb_shrink_callback,
};


static int index_handle, index_handle_locked;
static int index_move_callback(int handle, void *current, void *new)
{
    (void)handle;(void)current;(void)new;
    if (UNLIKELY(index_handle_locked))
        return BUFLIB_CB_CANNOT_MOVE;
    return BUFLIB_CB_OK;
}

static int index_shrink_callback(int handle, unsigned hints, void *start, size_t old_size)
{
    (void)start;(void)old_size;
    if (LIKELY(!index_handle_locked)
            && give_buffer_away
            && (hints & BUFLIB_SHRINK_POS_MASK) == BUFLIB_SHRINK_POS_MASK)
    {
        index_handle = core_free(handle);
        /* the clip buffer isn't usable without index table */
        if (LIKELY(!talk_handle_locked))
            talk_handle = core_free(talk_handle);
        return BUFLIB_CB_OK;
    }
    return BUFLIB_CB_CANNOT_SHRINK;
}

static struct buflib_callbacks index_ops = {
    .move_callback = index_move_callback,
    .shrink_callback = index_shrink_callback,
};

static int open_voicefile(void)
{
    char buf[64];
    char* p_lang = "english"; /* default */

    if ( global_settings.lang_file[0] &&
         global_settings.lang_file[0] != 0xff ) 
    {   /* try to open the voice file of the selected language */
        p_lang = (char *)global_settings.lang_file;
    }

    snprintf(buf, sizeof(buf), LANG_DIR "/%s.voice", p_lang);
    
    return open(buf, O_RDONLY);
}


/* fetch a clip from the voice file */
static int get_clip(long id, long* p_size)
{
    int retval = -1;
    struct clip_entry* clipbuf;
    size_t clipsize;

    if (id > VOICEONLY_DELIMITER)
    {   /* voice-only entries use the second part of the table.
           The first string comes after VOICEONLY_DELIMITER so we need to
           substract VOICEONLY_DELIMITER + 1 */
        id -= VOICEONLY_DELIMITER + 1;
        if (id >= voicefile.id2_max)
            return -1; /* must be newer than we have */
        id += voicefile.id1_max; /* table 2 is behind table 1 */
    }
    else
    {   /* normal use of the first table */
        if (id >= voicefile.id1_max)
            return -1; /* must be newer than we have */
    }

    clipbuf = core_get_data(index_handle);
    clipsize = clipbuf[id].size;
    if (clipsize == 0) /* clip not included in voicefile */
        return -1;

#ifndef TALK_PARTIAL_LOAD
    retval = clipbuf[id].offset;

#else
    if (!(clipsize & LOADED_MASK))
    {   /* clip needs loading */
        ssize_t ret;
        int fd, idx = 0;
        unsigned char *voicebuf;
        if (id == VOICE_PAUSE) {
            idx = QUEUE_SIZE;   /* we keep VOICE_PAUSE loaded */
        } else {
            int oldest = 0, i;
            for(i=0; i<QUEUE_SIZE; i++) {
                if (buffered_id[i] < 0) {
                    /* found a free entry, that means the buffer isn't
                     * full yet. */
                    idx = i;
                    break;
                }

                /* find the oldest clip */
                if(clip_age[i] > oldest) {
                    idx = i;
                    oldest = clip_age[i];
                }

                /* increment age of each loaded clip */
                clip_age[i]++;
            }
            clip_age[idx] = 0; /* reset clip's age */
        }
        retval = idx * max_clipsize;
        fd = open_voicefile();
        if (fd < 0)
            return -1; /* open error */

        talk_handle_locked++;
        voicebuf = core_get_data(talk_handle);
        clipbuf = core_get_data(index_handle);
        lseek(fd, clipbuf[id].offset, SEEK_SET);
        ret = read(fd, &voicebuf[retval], clipsize);
        close(fd);
        talk_handle_locked--;

        if (ret < 0 || clipsize != (size_t)ret)
            return -1; /* read error */

        clipbuf = core_get_data(index_handle);
        clipbuf[id].size |= LOADED_MASK; /* mark as loaded */

        if (id != VOICE_PAUSE) {
            if (buffered_id[idx] >= 0) {
                /* mark previously loaded clip as unloaded */
                clipbuf[buffered_id[idx]].size &= ~LOADED_MASK;
            }
            buffered_id[idx] = id;
        }
    }
    else
    {   /* clip is in memory already */
        /* Find where it was loaded */
        if (id == VOICE_PAUSE) {
            retval = QUEUE_SIZE * max_clipsize;
        } else {
            int idx;
            for (idx=0; idx<QUEUE_SIZE; idx++)
                if (buffered_id[idx] == id) {
                    retval = idx * max_clipsize;
                    clip_age[idx] = 0; /* reset clip's age */
                    break;
                }
        }
        clipsize &= ~LOADED_MASK; /* without the extra bit gives true size */
    }
#endif /* TALK_PARTIAL_LOAD */

    *p_size = clipsize;
    return retval;
}

static bool load_index_table(int fd, const struct voicefile_header *hdr)
{
    ssize_t ret;
    struct clip_entry *buf;

    if (index_handle > 0) /* nothing to do? */
        return true;

    ssize_t alloc_size = (hdr->id1_max + hdr->id2_max) * sizeof(struct clip_entry);
    index_handle = core_alloc_ex("voice index", alloc_size, &index_ops);
    if (index_handle < 0)
        return false;

    index_handle_locked++;
    buf = core_get_data(index_handle);
    ret = read(fd, buf, alloc_size);

#ifndef TALK_PARTIAL_LOAD
    int clips_offset, num_clips;
    /* adjust the offsets of the clips, they are relative to the file
     * TALK_PARTUAL_LOAD needs the file offset instead as it loads
     * the clips later */
    clips_offset = hdr->table;
    num_clips = hdr->id1_max + hdr->id2_max;
    clips_offset += num_clips * sizeof(struct clip_entry); /* skip index */
#endif
    if (ret == alloc_size)
        for (int i = 0; i < hdr->id1_max + hdr->id2_max; i++)
        {
#ifdef ROCKBOX_LITTLE_ENDIAN
            structec_convert(&buf[i], "ll", 1, true);
#endif
#ifndef TALK_PARTIAL_LOAD
            buf[i].offset -= clips_offset;
#endif
        }

    index_handle_locked--;

    if (ret != alloc_size)
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

#ifndef TALK_PARTIAL_LOAD
static bool load_data(int fd, ssize_t size_to_read)
{
    unsigned char *buf;
    ssize_t ret;

    if (size_to_read < 0)
        return false;

    talk_handle = core_alloc_ex("voice data", size_to_read, &clip_ops);
    if (talk_handle < 0)
        return false;

    talk_handle_locked++;
    buf = core_get_data(talk_handle);
    ret = read(fd, buf, size_to_read);
    talk_handle_locked--;

    if (ret != size_to_read)
        talk_handle = core_free(talk_handle);

    return ret == size_to_read;
}
#endif

static bool alloc_thumbnail_buf(void)
{
    int handle;
    size_t size;
    if (thumb_handle > 0)
        return true; /* nothing to do? */
#if CONFIG_CODEC == SWCODEC
    /* try to allocate the max. first, and take whatever we can get if that
     * fails */
    size = MAX_THUMBNAIL_BUFSIZE;
    handle = core_alloc_ex("voice thumb", MAX_THUMBNAIL_BUFSIZE, &thumb_ops);
    if (handle < 0)
    {
        size = core_allocatable();
        handle = core_alloc_ex("voice thumb", size, &thumb_ops);
    }
#else
    /* on HWCODEC, just use the rest of the remaining buffer,
     * normal playback cannot happen anyway */
    handle = core_alloc_maximum("voice thumb", &size, &thumb_ops);
#endif
    thumb_handle = handle;
    size_for_thumbnail = (handle > 0) ? size : 0;
    return handle > 0;
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

static bool load_voicefile_data(int fd, size_t max_size)
{
#ifdef TALK_PARTIAL_LOAD
    (void)fd;
    /* just allocate, populate on an as-needed basis later */
    talk_handle = core_alloc_ex("voice data", max_size, &clip_ops);
    if (talk_handle < 0)
        goto load_err_free;
#else
    size_t load_size, clips_size;
    /* load the entire file into memory */
    clips_size = (voicefile.id1_max+voicefile.id2_max) * sizeof(struct clip_entry);
    load_size  = max_size - voicefile.table - clips_size;
    if (!load_data(fd, load_size))
        goto load_err_free;
#endif

    /* make sure to have the silence clip, if available
     * return value can be cached globally even for TALK_PARTIAL_LOAD because
     * the VOICE_PAUSE clip is specially handled */
    silence_offset = get_clip(VOICE_PAUSE, &silence_length);

    /* not an error if this fails here, might try again when the
     * actual thumbnails are attempted to be played back */
    alloc_thumbnail_buf();

    return true;

load_err_free:
    index_handle = core_free(index_handle);
    return false;
}


/* called in ISR context (on HWCODEC) if mp3 data got consumed */
static void mp3_callback(const void** start, size_t* size)
{
    queue[queue_read].length -= sent; /* we completed this */
    queue[queue_read].offset += sent;

    if (queue[queue_read].length > 0) /* current clip not finished? */
    {   /* feed the next 64K-1 chunk */
        int offset;
#if CONFIG_CODEC != SWCODEC
        sent = MIN(queue[queue_read].length, 0xFFFF);
#else
        sent = queue[queue_read].length;
#endif
        offset = queue[queue_read].offset;
        if ((unsigned long)offset >= voicefile_size)
            *start = core_get_data(thumb_handle) + offset - voicefile_size;
        else
            *start = core_get_data(talk_handle) + offset;
        *size = sent;
        return;
    }
    talk_queue_lock();
    if(thumb_handle && (unsigned long)queue[queue_read].offset == voicefile_size+thumbnail_buf_used)
        thumbnail_buf_used = 0;
    if (sent > 0) /* go to next entry */
    {
        queue_read = (queue_read + 1) & QUEUE_MASK;
    }

re_check:

    if (QUEUE_LEVEL != 0) /* queue is not empty? */
    {   /* start next clip */
        unsigned char *buf;
#if CONFIG_CODEC != SWCODEC
        sent = MIN(queue[queue_read].length, 0xFFFF);
#else
        sent = queue[queue_read].length;
#endif
        lastclip_offset = queue[queue_read].offset;
        /* offsets larger than voicefile_size denote thumbnail clips */
        if (lastclip_offset >= voicefile_size)
            buf = core_get_data(thumb_handle) + lastclip_offset - voicefile_size;
        else
            buf = core_get_data(talk_handle) + lastclip_offset;
        *start = buf;
        *size = sent;
        curr_hd[0] = buf[1];
        curr_hd[1] = buf[2];
        curr_hd[2] = buf[3];
    }
    else if (silence_offset > 0               /* silence clip available */
             && lastclip_offset != (unsigned long)silence_offset  /* previous clip wasn't silence */
             && !(lastclip_offset >= voicefile_size   /* ..or thumbnail */
                 && lastclip_offset < voicefile_size +size_for_thumbnail))
    {   /* add silence clip when queue runs empty playing a voice clip */
        queue[queue_write].offset = silence_offset;
        queue[queue_write].length = silence_length;
        queue_write = (queue_write + 1) & QUEUE_MASK;

        goto re_check;
    }
    else
    {
        *size = 0; /* end of data */
        talk_handle_locked--;
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
    unsigned clip_offset;
    if (QUEUE_LEVEL == 0) /* has ended anyway */
        return;

#if CONFIG_CPU == SH7034
    CHCR3 &= ~0x0001; /* disable the DMA (and therefore the interrupt also) */
#endif /* CONFIG_CPU == SH7034 */
    /* search next frame boundary and continue up to there */
    pos = search = mp3_get_pos();
    clip_offset = queue[queue_read].offset;
    if (clip_offset >= voicefile_size)
        end = core_get_data(thumb_handle) + clip_offset - voicefile_size;
    else
        end = core_get_data(talk_handle) + clip_offset;
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
    talk_handle_locked = MAX(talk_handle_locked-1, 0);
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
static void queue_clip(unsigned long clip_offset, long size, bool enqueue)
{
    unsigned char *buf;
    int queue_level;

    if (!enqueue)
        talk_shutup(); /* cut off all the pending stuff */
    /* Something is being enqueued, force_enqueue_next override is no
       longer in effect. */
    force_enqueue_next = false;
    
    if (!size)
        return; /* safety check */
#if CONFIG_CPU == SH7034
    /* disable the DMA temporarily, to be safe of race condition */
    CHCR3 &= ~0x0001;
#endif
    talk_queue_lock();
    queue_level = QUEUE_LEVEL; /* check old level */

    if (queue_level < QUEUE_SIZE - 1) /* space left? */
    {
        queue[queue_write].offset = clip_offset; /* populate an entry */
        queue[queue_write].length = size;
        queue_write = (queue_write + 1) & QUEUE_MASK;
    }
    talk_queue_unlock();

    if (queue_level == 0)
    {   /* queue was empty, we have to do the initial start */
        lastclip_offset = clip_offset;
#if CONFIG_CODEC != SWCODEC
        sent = MIN(size, 0xFFFF); /* DMA can do no more */
#else
        sent = size;
#endif
        talk_handle_locked++;
        if (clip_offset >= voicefile_size)
            buf = core_get_data(thumb_handle) + clip_offset - voicefile_size;
        else
            buf = core_get_data(talk_handle) + clip_offset;
        mp3_play_data(buf, sent, mp3_callback);
        curr_hd[0] = buf[1];
        curr_hd[1] = buf[2];
        curr_hd[2] = buf[3];
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

#if CONFIG_CODEC == SWCODEC
    if(!talk_initialized)
        mutex_init(&queue_mutex);
#endif /* CONFIG_CODEC == SWCODEC */

    talk_initialized = true;
    strlcpy((char *)last_lang, (char *)global_settings.lang_file,
        MAX_FILENAME);

    /* reset some states */
    queue_write = queue_read = 0; /* reset the queue */
    memset(&voicefile, 0, sizeof(voicefile));

#ifdef TALK_PARTIAL_LOAD
    for(int i=0; i<QUEUE_SIZE; i++)
        buffered_id[i] = -1;
#endif

    silence_offset = -1; /* pause clip not accessible */
    voicefile_size = has_voicefile = 0;
    /* need to free these as their size depends on the voice file, and
     * this function is called when the talk voice file changes */
    if (index_handle > 0) index_handle = core_free(index_handle);
    if (talk_handle  > 0) talk_handle  = core_free(talk_handle);
    /* don't free thumb handle, it doesn't depend on the actual voice file
     * and so we can re-use it if it's already allocated in any event */

    filehandle = open_voicefile();
    if (filehandle < 0)
        return;

    if (!load_voicefile_index(filehandle))
        goto out;

#ifdef TALK_PARTIAL_LOAD
    /* TALK_PARTIAL_LOAD loads the actual clip data later, and not all
     * at once */
    unsigned num_clips = voicefile.id1_max + voicefile.id2_max;
    struct clip_entry *clips = core_get_data(index_handle);
    int silence_size = 0;

    for(unsigned i=0; i<num_clips; i++) {
        int size = clips[i].size;
        if (size > max_clipsize)
            max_clipsize = size;
        if (i == VOICE_PAUSE)
            silence_size = size;
    }

    voicefile_size = voicefile.table + num_clips * sizeof(struct clip_entry);
    voicefile_size += max_clipsize * QUEUE_SIZE + silence_size;

    /* test if we can open and if it fits in the audiobuffer */
    size_t audiobufsz = audio_buffer_available();
    has_voicefile = audiobufsz >= voicefile_size;

#else
    /* load the compressed clip data into memory, in its entirety */
    voicefile_size = filesize(filehandle);
    if (!load_voicefile_data(filehandle, voicefile_size))
    {
        voicefile_size = 0;
        goto out;
    }
    has_voicefile = true;
#endif

#if CONFIG_CODEC == SWCODEC
    /* Safe to init voice playback engine now since we now know if talk is
       required or not */
    voice_thread_init();
#endif

out:
    close(filehandle); /* close again, this was just to detect presence */
    filehandle = -1;
}

#if CONFIG_CODEC == SWCODEC
/* return if a voice codec is required or not */
bool talk_voice_required(void)
{
    return (has_voicefile) /* Voice file is available */
        || (global_settings.talk_dir_clip)  /* Thumbnail clips are required */
        || (global_settings.talk_file_clip);
}
#endif

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
    int clip;
    long clipsize;
    int32_t unit;
    int decimals;

    if (!has_voicefile)
        return 0; /* no voicefile loaded, not an error -> pretent success */
    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
    if (!check_audio_status())
        return -1;

    if (talk_handle <= 0 || index_handle <= 0) /* reload needed? */
    {
        int fd = open_voicefile();
        if (fd < 0
                || !load_voicefile_index(fd)
                || !load_voicefile_data(fd, voicefile_size))
            return -1;
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

    clip = get_clip(id, &clipsize);
    if (clip < 0)
        return -1; /* not present */

#ifdef LOGF_ENABLE
    if (id > VOICEONLY_DELIMITER)
        logf("\ntalk_id: Say voice clip 0x%x\n", id);
    else
        logf("\ntalk_id: Say '%s'\n", str(id));
#endif

    queue_clip(clip, clipsize, enqueue);

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
    int thumb_used;
    char *buf;
#if CONFIG_CODEC != SWCODEC
    struct mp3entry info;
#endif

    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
    if (!check_audio_status())
        return -1;

    if (!alloc_thumbnail_buf())
        return -1;

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

    thumb_used = thumbnail_buf_used;
    if(filesize(fd) > size_for_thumbnail -thumb_used)
    {   /* Don't play truncated  clips */
        close(fd);
        return 0;
    }

#if CONFIG_CODEC != SWCODEC
    lseek(fd, info.first_frame_offset, SEEK_SET); /* behind ID data */
#endif

    talk_handle_locked++;
    buf = core_get_data(thumb_handle);
    size = read(fd, buf+thumb_used, size_for_thumbnail - thumb_used);
    talk_handle_locked--;
    close(fd);

    /* ToDo: find audio, skip ID headers and trailers */

    if (size > 0)    /* Don't play missing clips */
    {
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
        /* bitswap doesnt yield() */
        bitswap(core_get_data(thumb_handle), size);
#endif
        if(prefix_ids)
            /* prefix thumbnail by speaking these ids, but only now
               that we know there's actually a thumbnail to be
               spoken. */
            talk_idarray(prefix_ids, true);
        talk_queue_lock();
        thumbnail_buf_used = thumb_used + size;
        talk_queue_unlock();
        queue_clip(voicefile_size + thumb_used, size, true);
    }

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
        {
            talk_id(VOICE_HUNDRED, true);
            talk_id(VOICE_HOUR, true);
        }
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
