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
#include "file.h"
#include "buffer.h"
#include "system.h"
#include "settings.h"
#include "mp3_playback.h"
#include "mpeg.h"
#include "lang.h"
#include "talk.h"
#include "id3.h"
#include "bitswap.h"

/***************** Constants *****************/

#define QUEUE_SIZE 64 /* must be a power of two */
#define QUEUE_MASK (QUEUE_SIZE-1)
const char* const dir_thumbnail_name = "_dirname.talk";
const char* const file_thumbnail_ext = ".talk";

/***************** Functional Macros *****************/

#define QUEUE_LEVEL ((queue_write - queue_read) & QUEUE_MASK)

#define LOADED_MASK 0x80000000 /* MSB */


/***************** Data types *****************/

struct clip_entry /* one entry of the index table */
{
    int offset; /* offset from start of voicefile file */
    int size; /* size of the clip */
};

struct voicefile /* file format of our voice file */
{
    int version; /* version of the voicefile */
    int table;   /* offset to index table, (=header size) */
    int id1_max; /* number of "normal" clips contained in above index */
    int id2_max; /* number of "voice only" clips contained in above index */
    struct clip_entry index[]; /* followed by the index tables */
    /* and finally the bitswapped mp3 clips, not visible here */
};

struct queue_entry /* one entry of the internal queue */
{
    unsigned char* buf;
    int len;
};


/***************** Globals *****************/

static unsigned char* p_thumbnail; /* buffer for thumbnail */
static long size_for_thumbnail; /* leftover buffer size for it */
static struct voicefile* p_voicefile; /* loaded voicefile */
static bool has_voicefile; /* a voicefile file is present */
static struct queue_entry queue[QUEUE_SIZE]; /* queue of scheduled clips */
static int queue_write; /* write index of queue, by application */
static int queue_read; /* read index of queue, by ISR context */
static int sent; /* how many bytes handed over to playback, owned by ISR */
static unsigned char curr_hd[3]; /* current frame header, for re-sync */
static int filehandle; /* global, so the MMC variant can keep the file open */


/***************** Private prototypes *****************/

static void load_voicefile(void);
static void mp3_callback(unsigned char** start, int* size);
static int shutup(void);
static int queue_clip(unsigned char* buf, int size, bool enqueue);
static int open_voicefile(void);


/***************** Private implementation *****************/

static int open_voicefile(void)
{
    char buf[64];
    char* p_lang = "english"; /* default */

    if ( global_settings.lang_file[0] &&
         global_settings.lang_file[0] != 0xff ) 
    {   /* try to open the voice file of the selected language */
        p_lang = global_settings.lang_file;
    }

    snprintf(buf, sizeof(buf), ROCKBOX_DIR LANG_DIR "/%s.voice", p_lang);

    return open(buf, O_RDONLY);
}


/* load the voice file into the mp3 buffer */
static void load_voicefile(void)
{
    int load_size;
    int got_size;
    int file_size;

    filehandle = open_voicefile();
    if (filehandle < 0) /* failed to open */
        goto load_err;

    file_size = filesize(filehandle);
    if (file_size > mp3end - mp3buf) /* won't fit? */
       goto load_err;

#ifdef HAVE_MMC /* load only the header for now */
    load_size = offsetof(struct voicefile, index);
#else /* load the full file */
    load_size = file_size; 
#endif

    got_size = read(filehandle, mp3buf, load_size);
    if (got_size == load_size /* success */
        && ((struct voicefile*)mp3buf)->table /* format check */
           == offsetof(struct voicefile, index))
    {
        p_voicefile = (struct voicefile*)mp3buf;

        /* thumbnail buffer is the remaining space behind */
        p_thumbnail = mp3buf + file_size;
        p_thumbnail += (int)p_thumbnail % 2; /* 16-bit align */
        size_for_thumbnail = mp3end - p_thumbnail;
    }
    else
       goto load_err;

#ifdef HAVE_MMC 
    /* load the index table, now that we know its size from the header */
    load_size = (p_voicefile->id1_max + p_voicefile->id2_max)
                * sizeof(struct clip_entry);
    got_size = read(filehandle, 
                    mp3buf + offsetof(struct voicefile, index), load_size);
    if (got_size != load_size) /* read error */
       goto load_err;
#else
    close(filehandle); /* only the MMC variant leaves it open */
    filehandle = -1;
#endif

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
static void mp3_callback(unsigned char** start, int* size)
{
    queue[queue_read].len -= sent; /* we completed this */
    queue[queue_read].buf += sent;

    if (queue[queue_read].len > 0) /* current clip not finished? */
    {   /* feed the next 64K-1 chunk */
        sent = MIN(queue[queue_read].len, 0xFFFF);
        *start = queue[queue_read].buf;
        *size = sent;
        return;
    }
    else /* go to next entry */
    {
        queue_read++;
        if (queue_read >= QUEUE_SIZE)
            queue_read = 0;
    }

    if (QUEUE_LEVEL) /* queue is not empty? */
    {   /* start next clip */
        sent = MIN(queue[queue_read].len, 0xFFFF);
        *start = queue[queue_read].buf;
        *size = sent;
        curr_hd[0] = *start[1];
        curr_hd[1] = *start[2];
        curr_hd[2] = *start[3];
    }
    else
    {
        *size = 0; /* end of data */
        mp3_play_stop(); /* fixme: should be done by caller */
    }
}

/* stop the playback and the pending clips, but at frame boundary */
static int shutup(void)
{
    unsigned char* pos;
    unsigned char* search;
    unsigned char* end;

    if (QUEUE_LEVEL == 0) /* has ended anyway */
    {
        return 0;
    }

    CHCR3 &= ~0x0001; /* disable the DMA (and therefore the interrupt also) */

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

            queue_write = queue_read + 1; /* will be empty after next callback */
            if (queue_write >= QUEUE_SIZE)
                queue_write = 0;
            queue[queue_read].len = sent; /* current one ends after this */

            DTCR3 = sent; /* let the DMA finish this frame */
            CHCR3 |= 0x0001; /* re-enable DMA */
            return 0;
        }
    }

    /* nothing to do, was frame boundary or not our clip */
    mp3_play_stop();
    queue_write = queue_read = 0; /* reset the queue */

    return 0;
}


/* schedule a clip, at the end or discard the existing queue */
static int queue_clip(unsigned char* buf, int size, bool enqueue)
{
    int queue_level;

    if (!enqueue)
        shutup(); /* cut off all the pending stuff */
    
    if (!size)
        return 0; /* safety check */

    /* disable the DMA temporarily, to be safe of race condition */
    CHCR3 &= ~0x0001;

    queue_level = QUEUE_LEVEL; /* check old level */

    if (queue_level < QUEUE_SIZE - 1) /* space left? */
    {
        queue[queue_write].buf = buf; /* populate an entry */
        queue[queue_write].len = size;
    
        queue_write++; /* increase queue */
        if (queue_write >= QUEUE_SIZE)
            queue_write = 0;
    }
        
    if (queue_level == 0)
    {   /* queue was empty, we have to do the initial start */
        sent = MIN(size, 0xFFFF); /* DMA can do no more */
        mp3_play_data(buf, sent, mp3_callback);
        curr_hd[0] = buf[1];
        curr_hd[1] = buf[2];
        curr_hd[2] = buf[3];
        mp3_play_pause(true); /* kickoff audio */
    }
    else
    {
        CHCR3 |= 0x0001; /* re-enable DMA */
    }

    return 0;
}

/* common code for talk_init() and talk_buffer_steal() */
static void reset_state(void)
{
    queue_write = queue_read = 0; /* reset the queue */
    p_voicefile = NULL; /* indicate no voicefile (trashed) */
    p_thumbnail = mp3buf; /*  whole space for thumbnail */
    size_for_thumbnail = mp3end - mp3buf;
}

/***************** Public implementation *****************/

void talk_init(void)
{
    reset_state(); /* use this for most of our inits */

#ifdef HAVE_MMC
    load_voicefile(); /* load the tables right away */
    has_voicefile = (p_voicefile != NULL);
#else
    filehandle = open_voicefile();
    has_voicefile = (filehandle >= 0); /* test if we can open it */
    if (has_voicefile)
    {
        close(filehandle); /* close again, this was just to detect presence */
        filehandle = -1;
    }
#endif
}


/* somebody else claims the mp3 buffer, e.g. for regular play/record */
int talk_buffer_steal(void)
{
    mp3_play_stop();
#ifdef HAVE_MMC
    if (filehandle >= 0) /* only relevant for MMC */
    {
        close(filehandle);
        filehandle = -1;
    }
#endif
    reset_state();
    return 0;
}


/* play a voice ID from voicefile */
int talk_id(int id, bool enqueue)
{
    int clipsize;
    unsigned char* clipbuf;
    int unit;

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (p_voicefile == NULL && has_voicefile)
        load_voicefile(); /* reload needed */

    if (p_voicefile == NULL) /* still no voices? */
        return -1;

    if (id == -1) /* -1 is an indication for silence */
        return -1;

    /* check if this is a special ID, with a value */
    unit = ((unsigned)id) >> UNIT_SHIFT;
    if (unit)
    {   /* sign-extend the value */
        id = (unsigned)id << (32-UNIT_SHIFT);
        id >>= (32-UNIT_SHIFT);
        talk_value(id, unit, enqueue); /* speak it */
        return 0; /* and stop, end of special case */
    }

    if (id > VOICEONLY_DELIMITER)
    {   /* voice-only entries use the second part of the table */
        id -= VOICEONLY_DELIMITER + 1;
        if (id >= p_voicefile->id2_max)
            return -1; /* must be newer than we have */
        id += p_voicefile->id1_max; /* table 2 is behind table 1 */
    }
    else
    {   /* normal use of the first table */
        if (id >= p_voicefile->id1_max)
            return -1; /* must be newer than we have */
    }
    
    clipsize = p_voicefile->index[id].size;
    if (clipsize == 0) /* clip not included in voicefile */
        return -1;
    clipbuf = mp3buf + p_voicefile->index[id].offset;

#ifdef HAVE_MMC /* dynamic loading, on demand */
    if (!(clipsize & LOADED_MASK))
    {   /* clip used for the first time, needs loading */
        lseek(filehandle, p_voicefile->index[id].offset, SEEK_SET);
        if (read(filehandle, clipbuf, clipsize) != clipsize)
            return -1; /* read error */

        p_voicefile->index[id].size |= LOADED_MASK; /* mark as loaded */
    }
    else
    {   /* clip is in memory already */
        clipsize &= ~LOADED_MASK; /* without the extra bit gives true size */
    }
#endif

    queue_clip(clipbuf, clipsize, enqueue);

    return 0;
}


/* play a thumbnail from file */
int talk_file(const char* filename, bool enqueue)
{
    int fd;
    int size;
    struct mp3entry info;

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (p_thumbnail == NULL || size_for_thumbnail <= 0)
        return -1;

    if(mp3info(&info, filename, false)) /* use this to find real start */
    {   
        return 0; /* failed to open, or invalid */
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0) /* failed to open */
    {
        return 0;
    }

    lseek(fd, info.first_frame_offset, SEEK_SET); /* behind ID data */

    size = read(fd, p_thumbnail, size_for_thumbnail);
    close(fd);

    /* ToDo: find audio, skip ID headers and trailers */

    if (size)
    {
        bitswap(p_thumbnail, size);
        queue_clip(p_thumbnail, size, enqueue);
    }

    return size;
}


/* say a numeric value, this word ordering works for english,
   but not necessarily for other languages (e.g. german) */
int talk_number(int n, bool enqueue)
{
    int level = 0; /* mille count */
    int mil = 1000000000; /* highest possible "-illion" */

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (!enqueue)
        shutup(); /* cut off all the pending stuff */
    
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
                talk_id(VOICE_BILLION + level, true);
        }
        level++;
    }

    return 0;
}

/* singular/plural aware saying of a value */
int talk_value(int n, int unit, bool enqueue)
{
    int unit_id;
    static const int unit_voiced[] = 
    {   /* lookup table for the voice ID of the units */
        -1, -1, -1, /* regular ID, int, signed */
        VOICE_MILLISECONDS, /* here come the "real" units */
        VOICE_SECONDS, 
        VOICE_MINUTES, 
        VOICE_HOURS, 
        VOICE_KHZ, 
        VOICE_DB, 
        VOICE_PERCENT, 
        VOICE_MEGABYTE, 
        VOICE_GIGABYTE,
        VOICE_MILLIAMPHOURS,
        VOICE_PIXEL,
        VOICE_PER_SEC,
        VOICE_HERTZ,
    };

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

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

    talk_number(n, enqueue); /* say the number */
    talk_id(unit_id, true); /* say the unit, if any */
        
    return 0;
}

/* spell a string */
int talk_spell(const char* spell, bool enqueue)
{
    char c; /* currently processed char */
    
    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (!enqueue)
        shutup(); /* cut off all the pending stuff */
    
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
            talk_id(VOICE_POINT, true); 
            /* fixme: change to VOICE_DOT when settled in the voice files */
        else if (c == ' ')
            talk_id(VOICE_PAUSE, true);
    }

    return 0;
}
