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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "file.h"
#include "buffer.h"
#include "system.h"
#include "kernel.h"
#include "settings.h"
#include "settings_list.h"
#include "mp3_playback.h"
#include "audio.h"
#include "lang.h"
#include "talk.h"
#include "id3.h"
#include "logf.h"
#include "bitswap.h"
#include "structec.h"
#if CONFIG_CODEC == SWCODEC
#include "playback.h"
#endif
#include "debug.h"


/* Memory layout varies between targets because the
   Archos (MASCODEC) devices cannot mix voice and audio playback
 
             MASCODEC  | MASCODEC  | SWCODEC
             (playing) | (stopped) |
    audiobuf-----------+-----------+------------
              audio    | voice     | thumbnail
                       |-----------|------------
                       | thumbnail | voice
                       |           |------------
                       |           | filebuf
                       |           |------------
                       |           | audio
  audiobufend----------+-----------+------------

  SWCODEC allocates dedicated buffers, MASCODEC reuses audiobuf. */


/***************** Constants *****************/

#define QUEUE_SIZE 64 /* must be a power of two */
#define QUEUE_MASK (QUEUE_SIZE-1)
const char* const dir_thumbnail_name = "_dirname.talk";
const char* const file_thumbnail_ext = ".talk";

/***************** Functional Macros *****************/

#define QUEUE_LEVEL ((queue_write - queue_read) & QUEUE_MASK)

#define LOADED_MASK 0x80000000 /* MSB */

#if CONFIG_CODEC == SWCODEC    
#define MAX_THUMBNAIL_BUFSIZE 0x10000
#endif

/***************** Data types *****************/

struct clip_entry /* one entry of the index table */
{
    int offset; /* offset from start of voicefile file */
    int size; /* size of the clip */
};

struct voicefile /* file format of our voice file */
{
    int version; /* version of the voicefile */
    int target_id; /* the rockbox target the file was made for */
    int table;   /* offset to index table, (=header size) */
    int id1_max; /* number of "normal" clips contained in above index */
    int id2_max; /* number of "voice only" clips contained in above index */
    struct clip_entry index[]; /* followed by the index tables */
    /* and finally the mp3 clips, not visible here, bitswapped 
       for SH based players */
};

struct queue_entry /* one entry of the internal queue */
{
    unsigned char* buf;
    long len;
};


/***************** Globals *****************/

static unsigned char* p_thumbnail = NULL; /* buffer for thumbnail */
static long size_for_thumbnail; /* leftover buffer size for it */
static struct voicefile* p_voicefile; /* loaded voicefile */
static bool has_voicefile; /* a voicefile file is present */
static struct queue_entry queue[QUEUE_SIZE]; /* queue of scheduled clips */
static bool force_enqueue_next; /* enqueue next utterance even if enqueue is false */
static int queue_write; /* write index of queue, by application */
static int queue_read; /* read index of queue, by ISR context */
static int sent; /* how many bytes handed over to playback, owned by ISR */
static unsigned char curr_hd[3]; /* current frame header, for re-sync */
static int filehandle = -1; /* global, so the MMC variant can keep the file open */
static unsigned char* p_silence; /* VOICE_PAUSE clip, used for termination */
static long silence_len; /* length of the VOICE_PAUSE clip */
static unsigned char* p_lastclip; /* address of latest clip, for silence add */
static unsigned long voicefile_size = 0; /* size of the loaded voice file */
static unsigned char last_lang[MAX_FILENAME+1]; /* name of last used lang file (in talk_init) */
static bool talk_initialized; /* true if talk_init has been called */
static int talk_temp_disable_count; /* if positive, temporarily disable voice UI (not saved) */


/***************** Private implementation *****************/

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
static unsigned char* get_clip(long id, long* p_size)
{
    long clipsize;
    unsigned char* clipbuf;
    
    if (id > VOICEONLY_DELIMITER)
    {   /* voice-only entries use the second part of the table */
        id -= VOICEONLY_DELIMITER + 1;
        if (id >= p_voicefile->id2_max)
            return NULL; /* must be newer than we have */
        id += p_voicefile->id1_max; /* table 2 is behind table 1 */
    }
    else
    {   /* normal use of the first table */
        if (id >= p_voicefile->id1_max)
            return NULL; /* must be newer than we have */
    }
    
    clipsize = p_voicefile->index[id].size;
    if (clipsize == 0) /* clip not included in voicefile */
        return NULL;
    clipbuf = (unsigned char *) p_voicefile + p_voicefile->index[id].offset;

#ifdef HAVE_MMC /* dynamic loading, on demand */
    if (!(clipsize & LOADED_MASK))
    {   /* clip used for the first time, needs loading */
        lseek(filehandle, p_voicefile->index[id].offset, SEEK_SET);
        if (read(filehandle, clipbuf, clipsize) != clipsize)
            return NULL; /* read error */

        p_voicefile->index[id].size |= LOADED_MASK; /* mark as loaded */
    }
    else
    {   /* clip is in memory already */
        clipsize &= ~LOADED_MASK; /* without the extra bit gives true size */
    }
#endif

    *p_size = clipsize;
    return clipbuf;
}


/* load the voice file into the mp3 buffer */
static void load_voicefile(void)
{
    int load_size;
    int got_size;
    int file_size;
#ifdef ROCKBOX_LITTLE_ENDIAN
    int i;
#endif

    filehandle = open_voicefile();
    if (filehandle < 0) /* failed to open */
        goto load_err;

    file_size = filesize(filehandle);
    if (file_size > audiobufend - audiobuf) /* won't fit? */
        goto load_err;

#ifdef HAVE_MMC /* load only the header for now */
    load_size = offsetof(struct voicefile, index);
#else /* load the full file */
    load_size = file_size; 
#endif

    got_size = read(filehandle, audiobuf, load_size);
    if (got_size != load_size /* failure */)
        goto load_err;

#ifdef ROCKBOX_LITTLE_ENDIAN
    logf("Byte swapping voice file");
    structec_convert(audiobuf, "lllll", 1, true);
#endif

    if (((struct voicefile*)audiobuf)->table /* format check */
           == offsetof(struct voicefile, index))
    {
        p_voicefile = (struct voicefile*)audiobuf;

        if (p_voicefile->version != VOICE_VERSION ||
            p_voicefile->target_id != TARGET_ID)
        {
            logf("Incompatible voice file");
            goto load_err;
        }
#if CONFIG_CODEC != SWCODEC
        /* MASCODEC: now use audiobuf for voice then thumbnail */
        p_thumbnail = audiobuf + file_size;
        p_thumbnail += (long)p_thumbnail % 2; /* 16-bit align */
        size_for_thumbnail = audiobufend - p_thumbnail;
#endif
    }
    else
        goto load_err;

#ifdef ROCKBOX_LITTLE_ENDIAN
    for (i = 0; i < p_voicefile->id1_max + p_voicefile->id2_max; i++)
        structec_convert(&p_voicefile->index[i], "ll", 1, true);
#endif

#ifdef HAVE_MMC
    /* load the index table, now that we know its size from the header */
    load_size = (p_voicefile->id1_max + p_voicefile->id2_max)
                * sizeof(struct clip_entry);
    got_size = read(filehandle, 
                    (unsigned char *) p_voicefile + offsetof(struct voicefile, index), load_size);
    if (got_size != load_size) /* read error */
        goto load_err;
#else
    close(filehandle); /* only the MMC variant leaves it open */
    filehandle = -1;
#endif

    /* make sure to have the silence clip, if available */
    p_silence = get_clip(VOICE_PAUSE, &silence_len);

    return;

load_err:
    p_voicefile = NULL;
    has_voicefile = false; /* don't try again */
    if (filehandle >= 0)
    {
        close(filehandle);
        filehandle = -1;
    }
    return;
}


/* called in ISR context if mp3 data got consumed */
static void mp3_callback(unsigned char** start, size_t* size)
{
    queue[queue_read].len -= sent; /* we completed this */
    queue[queue_read].buf += sent;

    if (queue[queue_read].len > 0) /* current clip not finished? */
    {   /* feed the next 64K-1 chunk */
#if CONFIG_CODEC != SWCODEC
        sent = MIN(queue[queue_read].len, 0xFFFF);
#else
        sent = queue[queue_read].len;
#endif
        *start = queue[queue_read].buf;
        *size = sent;
        return;
    }
    else if (sent > 0) /* go to next entry */
    {
        queue_read = (queue_read + 1) & QUEUE_MASK;
    }

re_check:

    if (QUEUE_LEVEL != 0) /* queue is not empty? */
    {   /* start next clip */
#if CONFIG_CODEC != SWCODEC
        sent = MIN(queue[queue_read].len, 0xFFFF);
#else
        sent = queue[queue_read].len;
#endif
        *start = p_lastclip = queue[queue_read].buf;
        *size = sent;
        curr_hd[0] = p_lastclip[1];
        curr_hd[1] = p_lastclip[2];
        curr_hd[2] = p_lastclip[3];
    }
    else if (p_silence != NULL             /* silence clip available */
             && p_lastclip != p_silence    /* previous clip wasn't silence */
             && p_lastclip != p_thumbnail) /* ..or thumbnail */
    {   /* add silence clip when queue runs empty playing a voice clip */
        queue[queue_write].buf = p_silence;
        queue[queue_write].len = silence_len;
        queue_write = (queue_write + 1) & QUEUE_MASK;

        goto re_check;
    }
    else
    {
        *size = 0; /* end of data */
    }
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
    if (QUEUE_LEVEL == 0) /* has ended anyway */
        return;

#if CONFIG_CPU == SH7034
    CHCR3 &= ~0x0001; /* disable the DMA (and therefore the interrupt also) */
#endif /* CONFIG_CPU == SH7034 */
    /* search next frame boundary and continue up to there */
    pos = search = mp3_get_pos();
    end = queue[queue_read].buf + queue[queue_read].len;

    if (pos >= queue[queue_read].buf
        && pos <= end) /* really our clip? */
    { /* (for strange reasons this isn't nesessarily the case) */
        /* find the next frame boundary */
        while (search < end) /* search the remaining data */
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
            queue[queue_read].len = sent; /* current one ends after this */

#if CONFIG_CPU == SH7034
            DTCR3 = sent; /* let the DMA finish this frame */
            CHCR3 |= 0x0001; /* re-enable DMA */
#endif /* CONFIG_CPU == SH7034 */
            return;
        }
    }
#endif /* CONFIG_CODEC != SWCODEC */

    /* Either SWCODEC, or MAS had nothing to do (was frame boundary or not our clip) */
    mp3_play_stop();
    queue_write = queue_read = 0; /* reset the queue */
    return;
}

/* Shutup the voice, except if force_enqueue_next is set. */
void talk_shutup(void)
{
    if (!force_enqueue_next)
        talk_force_shutup();
}

/* schedule a clip, at the end or discard the existing queue */
static void queue_clip(unsigned char* buf, long size, bool enqueue)
{
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
    queue_level = QUEUE_LEVEL; /* check old level */

    if (queue_level < QUEUE_SIZE - 1) /* space left? */
    {
        queue[queue_write].buf = buf; /* populate an entry */
        queue[queue_write].len = size;
        queue_write = (queue_write + 1) & QUEUE_MASK;
    }
        
    if (queue_level == 0)
    {   /* queue was empty, we have to do the initial start */
        p_lastclip = buf;
#if CONFIG_CODEC != SWCODEC
        sent = MIN(size, 0xFFFF); /* DMA can do no more */
#else
        sent = size;
#endif
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

    return;
}


/* common code for talk_init() and talk_buffer_steal() */
static void reset_state(void)
{
    queue_write = queue_read = 0; /* reset the queue */
    p_voicefile = NULL; /* indicate no voicefile (trashed) */
#if CONFIG_CODEC == SWCODEC
    /* Allocate a dedicated thumbnail buffer - once */
    if (p_thumbnail == NULL)
    {
        size_for_thumbnail = audiobufend - audiobuf;
        if (size_for_thumbnail > MAX_THUMBNAIL_BUFSIZE)
            size_for_thumbnail = MAX_THUMBNAIL_BUFSIZE;
        p_thumbnail = buffer_alloc(size_for_thumbnail);
    }
#else
    /* Just use the audiobuf, without allocating anything */
    p_thumbnail = audiobuf;
    size_for_thumbnail = audiobufend - audiobuf;
#endif
    p_silence = NULL; /* pause clip not accessible */
}


/***************** Public implementation *****************/

void talk_init(void)
{
    talk_temp_disable_count = 0;
    if (talk_initialized && !strcasecmp(last_lang, global_settings.lang_file))
    {
        /* not a new file, nothing to do */
        return;
    }

#ifdef HAVE_MMC
    if (filehandle >= 0) /* MMC: An old voice file might still be open */
    {
        close(filehandle);
        filehandle = -1;
    }
#endif

    talk_initialized = true;
    strncpy((char *) last_lang, (char *)global_settings.lang_file,
        MAX_FILENAME);

#if CONFIG_CODEC == SWCODEC
    audio_get_buffer(false, NULL);  /* Must tell audio to reinitialize */
#endif
    reset_state(); /* use this for most of our inits */

    filehandle = open_voicefile();
    has_voicefile = (filehandle >= 0); /* test if we can open it */
    voicefile_size = 0;

    if (has_voicefile)
    {
        voicefile_size = filesize(filehandle);
        close(filehandle); /* close again, this was just to detect presence */
        filehandle = -1;
    }

}

#if CONFIG_CODEC == SWCODEC
/* return if a voice codec is required or not */
bool talk_voice_required(void)
{
    return (voicefile_size != 0) /* Voice file is available */
        || (global_settings.talk_dir_clip)  /* Thumbnail clips are required */
        || (global_settings.talk_file_clip);
}
#endif

/* return size of voice file */
int talk_get_bufsize(void)
{
    return voicefile_size;
}

/* somebody else claims the mp3 buffer, e.g. for regular play/record */
void talk_buffer_steal(void)
{
#if CONFIG_CODEC != SWCODEC
    mp3_play_stop();
#endif
#ifdef HAVE_MMC
    if (filehandle >= 0) /* only relevant for MMC */
    {
        close(filehandle);
        filehandle = -1;
    }
#endif
    reset_state();
}


/* play a voice ID from voicefile */
int talk_id(int32_t id, bool enqueue)
{
    long clipsize;
    unsigned char* clipbuf;
    int32_t unit;
    int decimals;

    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
#if CONFIG_CODEC != SWCODEC
    if (audio_status()) /* busy, buffer in use */
        return -1;
#endif

    if (p_voicefile == NULL && has_voicefile)
        load_voicefile(); /* reload needed */

    if (p_voicefile == NULL) /* still no voices? */
        return -1;

    if (id == -1) /* -1 is an indication for silence */
        return -1;

    decimals = (((uint32_t)id) >> DECIMAL_SHIFT) & 0x7;
    DEBUGF("decimals %d\n", decimals);
    /* check if this is a special ID, with a value */
    unit = ((uint32_t)id) >> UNIT_SHIFT;
    if (unit || decimals)
    {   /* sign-extend the value */
        id = (uint32_t)id << (32-DECIMAL_SHIFT);
        id >>= (32-DECIMAL_SHIFT);

        talk_value_decimal(id, unit, decimals, enqueue); /* speak it */
        return 0; /* and stop, end of special case */
    }

    clipbuf = get_clip(id, &clipsize);
    if (clipbuf == NULL)
        return -1; /* not present */

    queue_clip(clipbuf, clipsize, enqueue);

    return 0;
}
/* Speaks zero or more IDs (from an array). */
int talk_idarray(long *ids, bool enqueue)
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
int talk_file(const char* filename, bool enqueue)
{
    int fd;
    int size;
#if CONFIG_CODEC != SWCODEC
    struct mp3entry info;
#endif

    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
#if CONFIG_CODEC != SWCODEC
    if (audio_status()) /* busy, buffer in use */
        return -1; 
#endif

    if (p_thumbnail == NULL || size_for_thumbnail <= 0)
        return -1;

#if CONFIG_CODEC != SWCODEC
    if(mp3info(&info, filename)) /* use this to find real start */
    {   
        return 0; /* failed to open, or invalid */
    }
#endif

    fd = open(filename, O_RDONLY);
    if (fd < 0) /* failed to open */
    {
        return 0;
    }

#if CONFIG_CODEC != SWCODEC
    lseek(fd, info.first_frame_offset, SEEK_SET); /* behind ID data */
#endif

    size = read(fd, p_thumbnail, size_for_thumbnail);
    close(fd);

    /* ToDo: find audio, skip ID headers and trailers */

    if (size > 0 && size != size_for_thumbnail)    /* Don't play missing or truncated clips */
    {
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
        bitswap(p_thumbnail, size);
#endif
        queue_clip(p_thumbnail, size, enqueue);
    }

    return size;
}


/* say a numeric value, this word ordering works for english,
   but not necessarily for other languages (e.g. german) */
int talk_number(long n, bool enqueue)
{
    int level = 2; /* mille count */
    long mil = 1000000000; /* highest possible "-illion" */

    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
#if CONFIG_CODEC != SWCODEC
    if (audio_status()) /* busy, buffer in use */
        return -1; 
#endif

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
static int talk_time_unit(long secs, bool exact, bool enqueue)
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
        if(exact || !hours)
            talk_value(mins, UNIT_MIN, true);
        else talk_number(mins, true); /* don't say "minutes" */
    }
    if((exact && secs) || (!hours && !mins))
        talk_value(secs, UNIT_SEC, true);
    else if(!hours && secs)
        talk_number(secs, true);
    return 0;
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

    if (talk_temp_disable_count > 0)
        return -1;  /* talking has been disabled */
#if CONFIG_CODEC != SWCODEC
    if (audio_status()) /* busy, buffer in use */
        return -1; 
#endif

    /* special case for time duration */
    if (unit == UNIT_TIME || unit == UNIT_TIME_EXACT)
        return talk_time_unit(n, unit == UNIT_TIME_EXACT, enqueue);

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
            enqueue = true;
        }

        talk_number(n / (10*decimals), enqueue);
        talk_id(LANG_POINT, true);
        n %= (10*decimals);
        enqueue = true;
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
#if CONFIG_CODEC != SWCODEC
    if (audio_status()) /* busy, buffer in use */
        return -1; 
#endif

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
void talk_date(struct tm *tm, bool enqueue)
{
    talk_id(LANG_MONTH_JANUARY + tm->tm_mon, enqueue);
    talk_number(tm->tm_mday, true);
    talk_number(1900 + tm->tm_year, true);
}

void talk_time(struct tm *tm, bool enqueue)
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

#endif
