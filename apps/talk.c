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
 *  the voice UI lets menus and screens "talk" from a voicefont in memory.
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
extern void bitswap(unsigned char *data, int length); /* no header for this */

/***************** Constants *****************/

#define QUEUE_SIZE 50
const char* dir_thumbnail_name = ".dirname.tbx";


/***************** Data types *****************/

struct clip_entry /* one entry of the index table */
{
    int offset; /* offset from start of voicefont file */
    int size; /* size of the clip */
};

struct voicefont /* file format of our "voicefont" */
{
    int version; /* version of the voicefont */
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
static struct voicefont* p_voicefont; /* loaded voicefont */
static bool has_voicefont; /* a voicefont file is present */
static bool is_playing; /* we're currently playing */
static struct queue_entry queue[QUEUE_SIZE]; /* queue of scheduled clips */
static int queue_write; /* write index of queue, by application */
static int queue_read; /* read index of queue, by ISR context */

/***************** Private prototypes *****************/

static int load_voicefont(void);
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



static int load_voicefont(void)
{
    int fd;
    int size;

    p_voicefont = NULL; /* indicate no voicefont if we fail below */

    fd = open_voicefile();
    if (fd < 0) /* failed to open */
    {
        p_voicefont = NULL; /* indicate no voicefont */
        has_voicefont = false; /* don't try again */
        return 0;
    }

    size = read(fd, mp3buf, mp3end - mp3buf);
    if (size > 1000
        && ((struct voicefont*)mp3buf)->table
           == offsetof(struct voicefont, index))
    {
        p_voicefont = (struct voicefont*)mp3buf;

        /* thumbnail buffer is the remaining space behind */
        p_thumbnail = mp3buf + size;
        p_thumbnail += (int)p_thumbnail % 2; /* 16-bit align */
        size_for_thumbnail = mp3end - p_thumbnail;
    }
    else
    {
        has_voicefont = false; /* don't try again */
    }
    close(fd);

    return size;
}


/* called in ISR context if mp3 data got consumed */
static void mp3_callback(unsigned char** start, int* size)
{
    int play_now;

    if (queue[queue_read].len > 0) /* current clip not finished? */
    {   /* feed the next 64K-1 chunk */
        play_now = MIN(queue[queue_read].len, 0xFFFF);
        *start = queue[queue_read].buf;
        *size = play_now;
        queue[queue_read].buf += play_now;
        queue[queue_read].len -= play_now;
        return;
    }
    else /* go to next entry */
    {
        queue_read++;
        if (queue_read >= QUEUE_SIZE)
            queue_read = 0;
    }

    if (queue_read != queue_write) /* queue is not empty? */
    {   /* start next clip */
        play_now = MIN(queue[queue_read].len, 0xFFFF);
        *start = queue[queue_read].buf;
        *size = play_now;
        queue[queue_read].buf += play_now;
        queue[queue_read].len -= play_now;
    }
    else
    {
        *size = 0; /* end of data */
        is_playing = false;
        mp3_play_stop(); /* fixme: should be done by caller */
    }
}

/* stop the playback and the pending clips, but at frame boundary */
static int shutup(void)
{
    unsigned char* pos;
    unsigned char* search;
    unsigned char* end;
    /* one silent bitswapped mp3 frame (22kHz), without bit reservoir */
    static const unsigned char silent_frame[] = {
        0xFF,0xCF,0x08,0x23,0x00,0x00,0x00,0xC0,0x12,0x80,0x01,0x00,0x00,
        0x32,0x82,0xB2,0xA2,0xCC,0x74,0x9C,0xCC,0xAA,0xAA,0xAA,0xAA,0xAA,
    };

    mp3_play_pause(false); /* pause */

    if (!is_playing) /* has ended anyway */
        return 0;
    
    /* search next frame boundary and continue up to there */
    pos = search = mp3_get_pos();
    end = queue[queue_read].buf + queue[queue_read].len;

    /* Find the next frame boundary */
    while (search < end) /* search the remaining data */
    {
        if (*search++ != 0xFF) /* search for frame sync byte */
        {
            continue;
        }
            
        /* look at the (bitswapped) 2nd byte of header candidate */
        if ((*search & 0x07) == 0x07  /* rest of frame sync */
         && (*search & 0x18) != 0x10  /* version != reserved */
         && (*search & 0x60) != 0x00) /* layer != reserved */
        {
            search--; /* back to the sync byte */
            break; /* From looking at the first 2 bytes, this is a header. */
            /* this is not a sufficient condition to find header,
               may give "false alert" (end too early), but a start */
        }
    }

    queue_write = queue_read; /* reset the queue */
    is_playing = false;

    /* play old data until the frame end, to keep the MAS in sync */
    if (search-pos)
        queue_clip(pos, search-pos, true);

    /* If the voice clips contain dependent frames (currently they don't),
       it may be a good idea to insert an independent dummy frame here. */
    queue_clip((unsigned char*)silent_frame, sizeof(silent_frame), true);
    
    return 0;
}


/* schedule a clip, at the end or discard the existing queue */
static int queue_clip(unsigned char* buf, int size, bool enqueue)
{
    if (!enqueue)
        shutup(); /* cut off all the pending stuff */
    
    queue[queue_write].buf = buf;
    queue[queue_write].len = size;
    
    /* FixMe: make this IRQ-safe */
        
    if (!is_playing)
    {   /* queue empty, we have to do the initial start */
        int size_now = MIN(size, 0xFFFF); /* DMA can do no more */
        is_playing = true;
        mp3_play_data(buf, size_now, mp3_callback);
        mp3_play_pause(true); /* kickoff audio */
        queue[queue_write].buf += size_now;
        queue[queue_write].len -= size_now;
    }

    queue_write++;
    if (queue_write >= QUEUE_SIZE)
        queue_write = 0;

    return 0;
}


/***************** Public implementation *****************/

void talk_init(void)
{
    int fd;

    fd = open_voicefile();
    if (fd >= 0) /* success */
    {
        close(fd);
        has_voicefont = true;
    }
    else
    {
        has_voicefont = false; /* no voice file available */
    }
    
    talk_buffer_steal(); /* abuse this for most of our inits */
    queue_write = queue_read = 0; 
}


/* somebody else claims the mp3 buffer, e.g. for regular play/record */
int talk_buffer_steal(void)
{
    p_voicefont = NULL; /* indicate no voicefont (trashed) */
    p_thumbnail = mp3buf; /*  whole space for thumbnail */
    size_for_thumbnail = mp3end - mp3buf;
    return 0;
}


/* play a voice ID from voicefont */
int talk_id(int id, bool enqueue)
{
    int clipsize;
    unsigned char* clipbuf;
    int unit;

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (p_voicefont == NULL && has_voicefont)
        load_voicefont(); /* reload needed */

    if (p_voicefont == NULL) /* still no voices? */
        return -1;

    if (id == -1) /* -1 is an indication for silence */
        return -1;

    /* check if this is a special ID, with a value */
    unit = ((unsigned)id) >> UNIT_SHIFT;
    if (unit)
    {   /* sign-extend the value */
        //splash(200, true,"unit=%d", unit);
        id = (unsigned)id << (32-UNIT_SHIFT);
        id >>= (32-UNIT_SHIFT);
        talk_value(id, unit, enqueue); /* speak it */
        return 0; /* and stop, end of special case */
    }

    if (id > VOICEONLY_DELIMITER)
    {   /* voice-only entries use the second part of the table */
        id -= VOICEONLY_DELIMITER + 1;
        if (id >= p_voicefont->id2_max)
            return -1; /* must be newer than we have */
        id += p_voicefont->id1_max; /* table 2 is behind table 1 */
    }
    else
    {   /* normal use of the first table */
        if (id >= p_voicefont->id1_max)
            return -1; /* must be newer than we have */
    }
    
    clipsize = p_voicefont->index[id].size;
    if (clipsize == 0) /* clip not included in voicefont */
        return -1;

    clipbuf = mp3buf + p_voicefont->index[id].offset;

    queue_clip(clipbuf, clipsize, enqueue);

    return 0;
}


/* play a thumbnail from file */
int talk_file(char* filename, bool enqueue)
{
    int fd;
    int size;
    struct mp3entry info;

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (p_thumbnail == NULL || size_for_thumbnail <= 0)
        return -1;

    if(mp3info(&info, filename)) /* use this to find real start */
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

int talk_value(int n, int unit, bool enqueue)
{
    int unit_id;
    const int unit_voiced[] = 
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
int talk_spell(char* spell, bool enqueue) 
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
    }

    return 0;
}
