/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "config.h"
#include "debug.h"
#include "panic.h"
#include "id3.h"
#include "mpeg.h"
#include "ata.h"
#include "string.h"
#include <kernel.h>
#include "thread.h"
#include "errno.h"
#include "mp3data.h"
#include "buffer.h"
#ifndef SIMULATOR
#include "i2c.h"
#include "mas.h"
#include "dac.h"
#include "system.h"
#include "usb.h"
#include "file.h"
#include "hwcompat.h"
#endif

extern void bitswap(unsigned char *data, int length);

#ifdef HAVE_MAS3587F
static void init_recording(void);
static void init_playback(void);
static void start_recording(void);
static void stop_recording(void);
static int get_unsaved_space(void);
#endif

#ifndef SIMULATOR
static int get_unplayed_space(void);
static int get_playable_space(void);
static int get_unswapped_space(void);
#endif

#define MPEG_PLAY         1
#define MPEG_STOP         2
#define MPEG_PAUSE        3
#define MPEG_RESUME       4
#define MPEG_NEXT         5
#define MPEG_PREV         6
#define MPEG_FF_REWIND    7
#define MPEG_FLUSH_RELOAD 8
#define MPEG_RECORD 9
#define MPEG_INIT_RECORDING 10
#define MPEG_INIT_PLAYBACK 11
#define MPEG_NEW_FILE    12
#define MPEG_NEED_DATA    100
#define MPEG_TRACK_CHANGE 101
#define MPEG_SAVE_DATA    102
#define MPEG_STOP_DONE    103

#ifdef HAVE_MAS3587F
static enum
{
    MPEG_DECODER,
    MPEG_ENCODER
} mpeg_mode;
#endif

extern char* playlist_peek(int steps);
extern bool playlist_check(int steps);
extern int playlist_next(int steps);
extern int playlist_amount(void);
extern void update_file_pos( int id, int pos );

static char *units[] =
{
    "%",    /* Volume */
    "dB",   /* Bass */
    "dB",   /* Treble */
    "%",    /* Balance */
    "dB",   /* Loudness */
    "%",    /* Bass boost */
    "",     /* AVC */
    "",     /* Channels */
    "dB",   /* Left gain */
    "dB",   /* Right gain */
    "dB",   /* Mic gain */
};

static int numdecimals[] =
{
    0,    /* Volume */
    0,    /* Bass */
    0,    /* Treble */
    0,    /* Balance */
    0,    /* Loudness */
    0,    /* Bass boost */
    0,    /* AVC */
    0,    /* Channels */
    1,    /* Left gain */
    1,    /* Right gain */
    1,    /* Mic gain */
};

static int minval[] =
{
    0,    /* Volume */
    0,    /* Bass */
    0,    /* Treble */
    -50,  /* Balance */
    0,    /* Loudness */
    0,    /* Bass boost */
    -1,   /* AVC */
    0,    /* Channels */
    0,    /* Left gain */
    0,    /* Right gain */
    0,    /* Mic gain */
};

static int maxval[] =
{
    100,  /* Volume */
#ifdef HAVE_MAS3587F
    24,   /* Bass */
    24,   /* Treble */
#else
    30,   /* Bass */
    30,   /* Treble */
#endif
    50,   /* Balance */
    17,   /* Loudness */
    10,   /* Bass boost */
    3,    /* AVC */
    6,    /* Channels */
    15,   /* Left gain */
    15,   /* Right gain */
    15,   /* Mic gain */
};

static int defaultval[] =
{
    70,   /* Volume */
#ifdef HAVE_MAS3587F
    12+6, /* Bass */
    12+6, /* Treble */
#else
    15+7, /* Bass */
    15+7, /* Treble */
#endif
    0,    /* Balance */
    0,    /* Loudness */
    0,    /* Bass boost */
    0,    /* AVC */
    0,    /* Channels */
    8,    /* Left gain */
    8,    /* Right gain */
    2,    /* Mic gain */
};

char *mpeg_sound_unit(int setting)
{
    return units[setting];
}

int mpeg_sound_numdecimals(int setting)
{
    return numdecimals[setting];
}

int mpeg_sound_min(int setting)
{
    return minval[setting];
}

int mpeg_sound_max(int setting)
{
    return maxval[setting];
}

int mpeg_sound_default(int setting)
{
    return defaultval[setting];
}

/* list of tracks in memory */
#define MAX_ID3_TAGS (1<<4) /* Must be power of 2 */
#define MAX_ID3_TAGS_MASK (MAX_ID3_TAGS - 1)

struct id3tag
{
    struct mp3entry id3;
    int mempos;
    bool used;
};

static struct id3tag *id3tags[MAX_ID3_TAGS];
static struct id3tag _id3tags[MAX_ID3_TAGS];

static unsigned int current_track_counter = 0;
static unsigned int last_track_counter = 0;

#ifndef SIMULATOR

static bool mpeg_is_initialized = false;

static int tag_read_idx = 0;
static int tag_write_idx = 0;

static int num_tracks_in_memory(void)
{
    return (tag_write_idx - tag_read_idx) & MAX_ID3_TAGS_MASK;
}
#endif

#ifndef SIMULATOR
static void debug_tags(void)
{
#ifdef DEBUG_TAGS
    int i;

    for(i = 0;i < MAX_ID3_TAGS;i++)
    {
        DEBUGF("id3tags[%d]: %08x", i, id3tags[i]);
        if(id3tags[i])
            DEBUGF(" - %s", id3tags[i]->id3.path);
        DEBUGF("\n");
    }
    DEBUGF("read: %d, write :%d\n", tag_read_idx, tag_write_idx);
    DEBUGF("num_tracks_in_memory: %d\n", num_tracks_in_memory());
#endif
}

static bool append_tag(struct id3tag *tag)
{
    if(num_tracks_in_memory() < MAX_ID3_TAGS - 1)
    {
        id3tags[tag_write_idx] = tag;
        tag_write_idx = (tag_write_idx+1) & MAX_ID3_TAGS_MASK;
        debug_tags();
        return true;
    }
    else
    {
        DEBUGF("Tag memory is full\n");
        return false;
    }
}

static void remove_current_tag(void)
{
    int oldidx = tag_read_idx;
    
    if(num_tracks_in_memory() > 0)
    {
        /* First move the index, so nobody tries to access the tag */
        tag_read_idx = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;

        /* Now delete it */
        id3tags[oldidx]->used = false;
        id3tags[oldidx] = NULL;
        debug_tags();
    }
}

static void remove_all_non_current_tags(void)
{
    int i = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;

    while (i != tag_write_idx)
    {
        id3tags[i]->used = false;
        id3tags[i] = NULL;

        i = (i+1) & MAX_ID3_TAGS_MASK;
    }

    tag_write_idx = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;
    debug_tags();
}

static void remove_all_tags(void)
{
    int i;
    
    for(i = 0;i < MAX_ID3_TAGS;i++)
        remove_current_tag();

    tag_write_idx = tag_read_idx;

    debug_tags();
}
#endif

static void set_elapsed(struct mp3entry* id3)
{
    if ( id3->vbr ) {
        if ( id3->has_toc ) {
            /* calculate elapsed time using TOC */
            int i;
            unsigned int remainder, plen, relpos, nextpos;

            /* find wich percent we're at */
            for (i=0; i<100; i++ )
            {
                if ( id3->offset < (int)(id3->toc[i] * (id3->filesize / 256)) )
                {
                    break;
                }
            }
            
            i--;
            if (i < 0)
                i = 0;

            relpos = id3->toc[i];

            if (i < 99)
            {
                nextpos = id3->toc[i+1];
            }
            else
            {
                nextpos = 256; 
            }

            remainder = id3->offset - (relpos * (id3->filesize / 256));

            /* set time for this percent (divide before multiply to prevent
               overflow on long files. loss of precision is negligible on
               short files) */
            id3->elapsed = i * (id3->length / 100);

            /* calculate remainder time */
            plen = (nextpos - relpos) * (id3->filesize / 256);
            id3->elapsed += (((remainder * 100) / plen) *
                             (id3->length / 10000));
        }
        else {
            /* no TOC exists. set a rough estimate using average bitrate */
            int tpk = id3->length / (id3->filesize / 1024);
            id3->elapsed = id3->offset / 1024 * tpk;
        }
    }
    else
        /* constant bitrate == simple frame calculation */
        id3->elapsed = id3->offset / id3->bpf * id3->tpf;
}

static bool paused; /* playback is paused */

static unsigned int mpeg_errno;

#ifdef SIMULATOR
static bool is_playing = false;
static bool playing = false;
#else
static int last_dma_tick = 0;

static unsigned long mas_version_code;

#ifdef HAVE_MAS3507D

static unsigned int bass_table[] =
{
    0x9e400, /* -15dB */
    0xa2800, /* -14dB */
    0xa7400, /* -13dB */
    0xac400, /* -12dB */
    0xb1800, /* -11dB */
    0xb7400, /* -10dB */
    0xbd400, /* -9dB */
    0xc3c00, /* -8dB */
    0xca400, /* -7dB */
    0xd1800, /* -6dB */
    0xd8c00, /* -5dB */
    0xe0400, /* -4dB */
    0xe8000, /* -3dB */
    0xefc00, /* -2dB */
    0xf7c00, /* -1dB */
    0,
    0x800,   /* 1dB */
    0x10000, /* 2dB */
    0x17c00, /* 3dB */
    0x1f800, /* 4dB */
    0x27000, /* 5dB */
    0x2e400, /* 6dB */
    0x35800, /* 7dB */
    0x3c000, /* 8dB */
    0x42800, /* 9dB */
    0x48800, /* 10dB */
    0x4e400, /* 11dB */
    0x53800, /* 12dB */
    0x58800, /* 13dB */
    0x5d400, /* 14dB */
    0x61800  /* 15dB */
};

static unsigned int treble_table[] =
{
    0xb2c00, /* -15dB */
    0xbb400, /* -14dB */
    0xc1800, /* -13dB */
    0xc6c00, /* -12dB */
    0xcbc00, /* -11dB */
    0xd0400, /* -10dB */
    0xd5000, /* -9dB */
    0xd9800, /* -8dB */
    0xde000, /* -7dB */
    0xe2800, /* -6dB */
    0xe7e00, /* -5dB */
    0xec000, /* -4dB */
    0xf0c00, /* -3dB */
    0xf5c00, /* -2dB */
    0xfac00, /* -1dB */
    0,
    0x5400,  /* 1dB */
    0xac00,  /* 2dB */
    0x10400, /* 3dB */
    0x16000, /* 4dB */
    0x1c000, /* 5dB */
    0x22400, /* 6dB */
    0x28400, /* 7dB */
    0x2ec00, /* 8dB */
    0x35400, /* 9dB */
    0x3c000, /* 10dB */
    0x42c00, /* 11dB */
    0x49c00, /* 12dB */
    0x51800, /* 13dB */
    0x58400, /* 14dB */
    0x5f800  /* 15dB */
};

static unsigned int prescale_table[] =
{
    0x80000,  /* 0db */
    0x8e000,  /* 1dB */
    0x9a400,  /* 2dB */
    0xa5800, /* 3dB */
    0xaf400, /* 4dB */
    0xb8000, /* 5dB */
    0xbfc00, /* 6dB */
    0xc6c00, /* 7dB */
    0xcd000, /* 8dB */
    0xd25c0, /* 9dB */
    0xd7800, /* 10dB */
    0xdc000, /* 11dB */
    0xdfc00, /* 12dB */
    0xe3400, /* 13dB */
    0xe6800, /* 14dB */
    0xe9400  /* 15dB */
};
#endif

static struct event_queue mpeg_queue;
static char mpeg_stack[DEFAULT_STACK_SIZE + 0x1000];
static char mpeg_thread_name[] = "mpeg";

static int mp3buflen;
static int mp3buf_write;
static int mp3buf_swapwrite;
static int mp3buf_read;

static int last_dma_chunk_size;

static bool dma_on;  /* The DMA is active */
static bool playing; /* We are playing an MP3 stream */
static bool play_pending; /* We are about to start playing */
static bool is_playing; /* We are (attempting to) playing MP3 files */
static bool filling; /* We are filling the buffer with data from disk */
static bool dma_underrun; /* True when the DMA has stopped because of
                             slow disk reading (read error, shaking) */
static int low_watermark; /* Dynamic low watermark level */
static int low_watermark_margin; /* Extra time in seconds for watermark */
static int lowest_watermark_level; /* Debug value to observe the buffer
                                      usage */
#ifdef HAVE_MAS3587F
static bool is_recording; /* We are recording */
static bool stop_pending;
unsigned long record_start_time; /* Value of current_tick when recording
                                    was started */
static bool saving; /* We are saving the buffer to disk */
static char recording_filename[MAX_PATH];
static int rec_frequency_index; /* For create_xing_header() calls */
static int rec_version_index;   /* For create_xing_header() calls */
static bool disable_xing_header; /* When splitting files */
#endif

static int mpeg_file;

/* Synchronization variables */
#ifdef HAVE_MAS3587F
static bool init_recording_done;
static bool init_playback_done;
#endif
static bool mpeg_stop_done;

static void recalculate_watermark(int bitrate)
{
    int bytes_per_sec;
    int time = ata_spinup_time;

    /* A bitrate of 0 probably means empty VBR header. We play safe
       and set a high threshold */
    if(bitrate == 0)
        bitrate = 320;
    
    bytes_per_sec = bitrate * 1000 / 8;
    
    if(time)
    {
        /* No drive spins up faster than 3.5s */
        if(time < 350)
            time = 350;

        time = time * 3;
        low_watermark = ((low_watermark_margin * HZ + time) *
                         bytes_per_sec) / HZ;
    }
    else
    {
        low_watermark = MPEG_LOW_WATER;
    }
}

void mpeg_set_buffer_margin(int seconds)
{
    low_watermark_margin = seconds;
}

void mpeg_get_debugdata(struct mpeg_debug *dbgdata)
{
    dbgdata->mp3buflen = mp3buflen;
    dbgdata->mp3buf_write = mp3buf_write;
    dbgdata->mp3buf_swapwrite = mp3buf_swapwrite;
    dbgdata->mp3buf_read = mp3buf_read;

    dbgdata->last_dma_chunk_size = last_dma_chunk_size;

    dbgdata->dma_on = dma_on;
    dbgdata->playing = playing;
    dbgdata->play_pending = play_pending;
    dbgdata->is_playing = is_playing;
    dbgdata->filling = filling;
    dbgdata->dma_underrun = dma_underrun;

    dbgdata->unplayed_space = get_unplayed_space();
    dbgdata->playable_space = get_playable_space();
    dbgdata->unswapped_space = get_unswapped_space();

    dbgdata->low_watermark_level = low_watermark;
    dbgdata->lowest_watermark_level = lowest_watermark_level;
}

#ifdef HAVE_MAS3507D
static void mas_poll_start(int interval_in_ms)
{
    unsigned int count;

    count = (FREQ * interval_in_ms) / 1000 / 8;

    if(count > 0xffff)
    {
        panicf("Error! The MAS poll interval is too long (%d ms)\n",
               interval_in_ms);
        return;
    }
    
    /* We are using timer 1 */
    
    TSTR &= ~0x02; /* Stop the timer */
    TSNC &= ~0x02; /* No synchronization */
    TMDR &= ~0x02; /* Operate normally */

    TCNT1 = 0;   /* Start counting at 0 */
    GRA1 = count;
    TCR1 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 5 */
    IPRC = (IPRC & ~0x000f) | 0x0005;
    
    TSR1 &= ~0x02;
    TIER1 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x02; /* Start timer 1 */
}
#else
static void postpone_dma_tick(void)
{
    unsigned int count;

    count = FREQ / 1000 / 8;

    /* We are using timer 1 */
    
    TSTR &= ~0x02; /* Stop the timer */
    TSNC &= ~0x02; /* No synchronization */
    TMDR &= ~0x02; /* Operate normally */

    TCNT1 = 0;   /* Start counting at 0 */
    GRA1 = count;
    TCR1 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 5 */
    IPRC = (IPRC & ~0x000f) | 0x0005;
    
    TSR1 &= ~0x02;
    TIER1 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x02; /* Start timer 1 */
}
#endif

#ifdef DEBUG
static void dbg_timer_start(void)
{
    /* We are using timer 2 */
    
    TSTR &= ~0x04; /* Stop the timer */
    TSNC &= ~0x04; /* No synchronization */
    TMDR &= ~0x44; /* Operate normally */

    TCNT2 = 0;   /* Start counting at 0 */
    TCR2 = 0x03; /* Sysclock/8 */

    TSTR |= 0x04; /* Start timer 2 */
}

static int dbg_cnt2us(unsigned int cnt)
{
    return (cnt * 10000) / (FREQ/800);
}
#endif

static int get_unplayed_space(void)
{
    int space = mp3buf_write - mp3buf_read;
    if (space < 0)
        space += mp3buflen;
    return space;
}

static int get_playable_space(void)
{
    int space = mp3buf_swapwrite - mp3buf_read;
    if (space < 0)
        space += mp3buflen;
    return space;
}

static int get_unplayed_space_current_song(void)
{
    int space;

    if (num_tracks_in_memory() > 1)
    {
        int track_offset = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;

        space = id3tags[track_offset]->mempos - mp3buf_read;
    }
    else
    {
        space = mp3buf_write - mp3buf_read;
    }

    if (space < 0)
        space += mp3buflen;

    return space;
}

static int get_unswapped_space(void)
{
    int space = mp3buf_write - mp3buf_swapwrite;
    if (space < 0)
        space += mp3buflen;
    return space;
}

#ifdef HAVE_MAS3587F
static int get_unsaved_space(void)
{
    int space = mp3buf_write - mp3buf_read;
    if (space < 0)
        space += mp3buflen;
    return space;
}
#endif

static void init_dma(void)
{
    SAR3 = (unsigned int) mp3buf + mp3buf_read;
    DAR3 = 0x5FFFEC3;
    CHCR3 &= ~0x0002; /* Clear interrupt */
    CHCR3 = 0x1504; /* Single address destination, TXI0, IE=1 */
    last_dma_chunk_size = MIN(0x2000, get_unplayed_space_current_song());
    DTCR3 = last_dma_chunk_size & 0xffff;
    DMAOR = 0x0001; /* Enable DMA */
    CHCR3 |= 0x0001; /* Enable DMA IRQ */
    dma_underrun = false;
}

static void start_dma(void)
{
    SCR0 |= 0x80;
    dma_on = true;
}

static void stop_dma(void)
{
    SCR0 &= 0x7f;
    dma_on = false;
}

#ifdef HAVE_MAS3587F
#ifdef DEBUG
static long timing_info_index = 0;
static long timing_info[1024];
#endif
static bool inverted_pr;
static unsigned long num_rec_bytes;
static unsigned long num_recorded_frames;

void drain_dma_buffer(void)
{
    if(inverted_pr)
    {
        while((*((volatile unsigned char *)PBDR_ADDR) & 0x40))
        {
            or_b(0x08, &PADRH);
            
            while(*((volatile unsigned char *)PBDR_ADDR) & 0x80);
                    
            /* It must take at least 5 cycles before
               the data is read */
            asm(" nop\n nop\n nop\n");
            asm(" nop\n nop\n nop\n");
            and_b(~0x08, &PADRH);

            while(!(*((volatile unsigned char *)PBDR_ADDR) & 0x80));
        }
    }
    else
    {
        while((*((volatile unsigned char *)PBDR_ADDR) & 0x40))
        {
            and_b(~0x08, &PADRH);
            
            while(*((volatile unsigned char *)PBDR_ADDR) & 0x80);
            
            /* It must take at least 5 cycles before
               the data is read */
            asm(" nop\n nop\n nop\n");
            asm(" nop\n nop\n nop\n");
                    
            or_b(0x08, &PADRH);

            while(!(*((volatile unsigned char *)PBDR_ADDR) & 0x80));
        }
    }
}
#endif

static void dma_tick (void) __attribute__ ((section (".icode")));
static void dma_tick(void)
{
#ifdef HAVE_MAS3587F
    if(mpeg_mode == MPEG_DECODER)
    {
#endif
        if(playing && !paused)
        {
            /* Start DMA if it is disabled and the DEMAND pin is high */
            if(!dma_on && (PBDR & 0x4000))
            {
                if(!(SCR0 & 0x80))
                    start_dma();
            }
            id3tags[tag_read_idx]->id3.elapsed +=
                (current_tick - last_dma_tick) * 1000 / HZ;
            last_dma_tick = current_tick;
        }
#ifdef HAVE_MAS3587F
    }
    else
    {
        int i;
        int num_bytes;
        if(is_recording && (PBDR & 0x4000))
        {
#ifdef DEBUG
            timing_info[timing_info_index++] = current_tick;
            TCNT2 = 0;
#endif
            /* We read as long as EOD is high, but max 30 bytes.
               This code is optimized, and should probably be
               written in assembler instead. */
            if(inverted_pr)
            {
                i = 0;
                while((*((volatile unsigned char *)PBDR_ADDR) & 0x40)
                      && i < 30)
                {
                    or_b(0x08, &PADRH);

                    while(*((volatile unsigned char *)PBDR_ADDR) & 0x80);
                    
                    /* It must take at least 5 cycles before
                       the data is read */
                    asm(" nop\n nop\n nop\n");
                    mp3buf[mp3buf_write++] = *(unsigned char *)0x4000000;
                    
                    if(mp3buf_write >= mp3buflen)
                        mp3buf_write = 0;

                    i++;
                    
                    and_b(~0x08, &PADRH);

                    /* No wait for /RTW, cause it's not necessary */
                }
            }
            else
            {
                i = 0;
                while((*((volatile unsigned char *)PBDR_ADDR) & 0x40)
                      && i < 30)
                {
                    and_b(~0x08, &PADRH);
                    
                    while(*((volatile unsigned char *)PBDR_ADDR) & 0x80);
                    
                    /* It must take at least 5 cycles before
                       the data is read */
                    asm(" nop\n nop\n nop\n");
                    mp3buf[mp3buf_write++] = *(unsigned char *)0x4000000;
                    
                    if(mp3buf_write >= mp3buflen)
                        mp3buf_write = 0;

                    i++;
                    
                    or_b(0x08, &PADRH);

                    /* No wait for /RTW, cause it's not necessary */
                }
            }
#ifdef DEBUG
            timing_info[timing_info_index++] = TCNT2 + (i << 16);
            timing_info_index &= 0x3ff;
#endif

            num_rec_bytes += i;
            
            /* Signal to save the data if we are running out of buffer
               space */
            num_bytes = mp3buf_write - mp3buf_read;
            if(num_bytes < 0)
                num_bytes += mp3buflen;

            if(mp3buflen - num_bytes < MPEG_RECORDING_LOW_WATER && !saving)
            {
                saving = true;
                queue_post(&mpeg_queue, MPEG_SAVE_DATA, 0);
                wake_up_thread();
            }
        }
    }
#endif
}

static void reset_mp3_buffer(void)
{
    mp3buf_read = 0;
    mp3buf_write = 0;
    mp3buf_swapwrite = 0;
    lowest_watermark_level = mp3buflen;
}

#pragma interrupt
void DEI3(void)
{
    if(playing && !paused)
    {
        int unplayed_space_left;
        int space_until_end_of_buffer;
        int track_offset = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;

        mp3buf_read += last_dma_chunk_size;
        if(mp3buf_read >= mp3buflen)
            mp3buf_read = 0;
    
        /* First, check if we are on a track boundary */
        if (num_tracks_in_memory() > 0)
        {
            if (mp3buf_read == id3tags[track_offset]->mempos)
            {
                queue_post(&mpeg_queue, MPEG_TRACK_CHANGE, 0);
                track_offset = (track_offset+1) & MAX_ID3_TAGS_MASK;
            }
        }
        
        unplayed_space_left = get_unplayed_space();
        
        space_until_end_of_buffer = mp3buflen - mp3buf_read;
        
        if(!filling && unplayed_space_left < low_watermark)
        {
            filling = true;
            queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
        }
        
        if(unplayed_space_left)
        {
            last_dma_chunk_size = MIN(0x2000, unplayed_space_left);
            last_dma_chunk_size = MIN(last_dma_chunk_size,
                                      space_until_end_of_buffer);

            /* several tracks loaded? */
            if (num_tracks_in_memory() > 1)
            {
                /* will we move across the track boundary? */
                if (( mp3buf_read < id3tags[track_offset]->mempos ) &&
                    ((mp3buf_read+last_dma_chunk_size) >
                     id3tags[track_offset]->mempos ))
                {
                    /* Make sure that we end exactly on the boundary */
                    last_dma_chunk_size = id3tags[track_offset]->mempos
                        - mp3buf_read;
                }
            }

            DTCR3 = last_dma_chunk_size & 0xffff;
            SAR3 = (unsigned int)mp3buf + mp3buf_read;
            id3tags[tag_read_idx]->id3.offset += last_dma_chunk_size;

            /* Update the watermark debug level */
            if(unplayed_space_left < lowest_watermark_level)
                lowest_watermark_level = unplayed_space_left;
        }
        else
        {
            /* Check if the end of data is because of a hard disk error.
               If there is an open file handle, we are still playing music.
               If not, the last file has been loaded, and the file handle is
               closed. */
            if(mpeg_file >= 0)
            {
                /* Update the watermark debug level */
                if(unplayed_space_left < lowest_watermark_level)
                    lowest_watermark_level = unplayed_space_left;
                
                DEBUGF("DMA underrun.\n");
                dma_underrun = true;
            }
            else
            {
                DEBUGF("No more MP3 data. Stopping.\n");

                queue_post(&mpeg_queue, MPEG_TRACK_CHANGE, 0);
                playing = false;
                is_playing = false;
            }
            CHCR3 &= ~0x0001; /* Disable the DMA interrupt */
        }
    }

    CHCR3 &= ~0x0002; /* Clear DMA interrupt */
    wake_up_thread();
}

#ifdef HAVE_MAS3587F
static void demand_irq_enable(bool on)
{
    int oldlevel = set_irq_level(15);
    
    if(on)
    {
        IPRA = (IPRA & 0xfff0) | 0x000b;
        ICR &= ~0x0010; /* IRQ3 level sensitive */
    }
    else
        IPRA &= 0xfff0;

    set_irq_level(oldlevel);
}
#endif

#pragma interrupt
void IMIA1(void)
{
    dma_tick();
    TSR1 &= ~0x01;
#ifdef HAVE_MAS3587F
    /* Disable interrupt */
    IPRC &= ~0x000f;
#endif
}

#pragma interrupt
void IRQ6(void)
{
    stop_dma();
}

#ifdef HAVE_MAS3587F
#pragma interrupt
void IRQ3(void)
{
    /* Begin with setting the IRQ to edge sensitive */
    ICR |= 0x0010;
    
    if(mpeg_mode == MPEG_ENCODER)
        dma_tick();
    else
        postpone_dma_tick();
}
#endif

static int add_track_to_tag_list(char *filename)
{
    struct id3tag *t = NULL;
    int i;

    /* find a free tag */
    for (i=0; i < MAX_ID3_TAGS_MASK; i++ )
        if ( !_id3tags[i].used )
            t = &_id3tags[i];
    if(t)
    {
        /* grab id3 tag of new file and
           remember where in memory it starts */
        if(mp3info(&(t->id3), filename))
        {
            DEBUGF("Bad mp3\n");
            return -1;
        }
        t->mempos = mp3buf_write;
        t->id3.elapsed = 0;
        if(!append_tag(t))
        {
            DEBUGF("Tag list is full\n");
        }
        else
            t->used = true;
    }
    else
    {
        DEBUGF("No memory available for id3 tag");
    }
    return 0;
}

/* If next_track is true, opens the next track, if false, opens prev track */
static int new_file(int steps)
{
    int max_steps = playlist_amount();
    int start = num_tracks_in_memory() - 1;

    if (start < 0)
        start = 0;

    do {
        char *trackname;

        trackname = playlist_peek( start + steps );
        if ( !trackname )
            return -1;
        
        DEBUGF("playing %s\n", trackname);
        
        mpeg_file = open(trackname, O_RDONLY);
        if(mpeg_file < 0) {
            DEBUGF("Couldn't open file: %s\n",trackname);
            steps++;

            /* Bail out if no file could be opened */
            if(steps > max_steps)
                return -1;
        }
        else
        {
            int new_tag_idx = tag_write_idx;

            if(add_track_to_tag_list(trackname))
            {
                /* Bad mp3 file */
                steps++;
                close(mpeg_file);
                mpeg_file = -1;
            }
            else
            {
                /* skip past id3v2 tag */
                lseek(mpeg_file, 
                      id3tags[new_tag_idx]->id3.first_frame_offset,
                      SEEK_SET);
                id3tags[new_tag_idx]->id3.index = steps;
                id3tags[new_tag_idx]->id3.offset = 0;

                if(id3tags[new_tag_idx]->id3.vbr)
                    /* Average bitrate * 1.5 */
                    recalculate_watermark(
                        (id3tags[new_tag_idx]->id3.bitrate * 3) / 2);
                else
                    recalculate_watermark(
                        id3tags[new_tag_idx]->id3.bitrate);
                    
            }
        }
    } while ( mpeg_file < 0 );

    return 0;
}

static void stop_playing(void)
{
    /* Stop the current stream */
#ifdef HAVE_MAS3587F
    demand_irq_enable(false);
#endif
    playing = false;
    filling = false;
    if(mpeg_file >= 0)
        close(mpeg_file);
    mpeg_file = -1;
    stop_dma();
    remove_all_tags();
}

static void update_playlist(void)
{
    int index;

    if (num_tracks_in_memory() > 0)
    {
        index = playlist_next(id3tags[tag_read_idx]->id3.index);
        id3tags[tag_read_idx]->id3.index = index;
    }
}

static void track_change(void)
{
    DEBUGF("Track change\n");

#ifdef HAVE_MAS3587F
    /* Reset the AVC */
    mpeg_sound_set(SOUND_AVC, -1);
#endif
    remove_current_tag();

    update_playlist();
    current_track_counter++;
}

#ifdef DEBUG
void hexdump(unsigned char *buf, int len)
{
    int i;

    for(i = 0;i < len;i++)
    {
        if(i && (i & 15) == 0)
        {
            DEBUGF("\n");
        }
        DEBUGF("%02x ", buf[i]);
    }
    DEBUGF("\n");
}
#endif

static void start_playback_if_ready(void)
{
    int playable_space;

    playable_space = mp3buf_swapwrite - mp3buf_read;
    if(playable_space < 0)
        playable_space += mp3buflen;
    
    /* See if we have started playing yet. If not, do it. */
    if(play_pending || dma_underrun)
    {
        /* If the filling has stopped, and we still haven't reached
           the watermark, the file must be smaller than the
           watermark. We must still play it. */
        if((playable_space >= MPEG_PLAY_PENDING_THRESHOLD) ||
           !filling || dma_underrun)
        {
            DEBUGF("P\n");
            play_pending = false;
            playing = true;
			
            init_dma();
            if (!paused)
            {
                last_dma_tick = current_tick;
                start_dma();
#ifdef HAVE_MAS3587F
                demand_irq_enable(true);
#endif
            }

            /* Tell ourselves that we need more data */
            queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
        }
    }
}

static bool swap_one_chunk(void)
{
    int free_space_left;
    int amount_to_swap;

    free_space_left = get_unswapped_space();

    if(free_space_left == 0 && !play_pending)
        return false;

    /* Swap in larger chunks when the user is waiting for the playback
       to start, or when there is dangerously little playable data left */
    if(play_pending)
        amount_to_swap = MIN(MPEG_PLAY_PENDING_SWAPSIZE, free_space_left);
    else
    {
        if(get_playable_space() < low_watermark)
            amount_to_swap = MIN(MPEG_LOW_WATER_SWAP_CHUNKSIZE,
                                 free_space_left);
        else
            amount_to_swap = MIN(MPEG_SWAP_CHUNKSIZE, free_space_left);
    }
    
    if(mp3buf_write < mp3buf_swapwrite)
        amount_to_swap = MIN(mp3buflen - mp3buf_swapwrite,
                             amount_to_swap);
    else
        amount_to_swap = MIN(mp3buf_write - mp3buf_swapwrite,
                             amount_to_swap);

    bitswap(mp3buf + mp3buf_swapwrite, amount_to_swap);

    mp3buf_swapwrite += amount_to_swap;
    if(mp3buf_swapwrite >= mp3buflen)
    {
        mp3buf_swapwrite = 0;
    }

    return true;
}

static const unsigned char empty_id3_header[] =
{
    'I', 'D', '3', 0x04, 0x00, 0x00,
    0x00, 0x00, 0x1f, 0x76 /* Size is 4096 minus 10 bytes for the header */
};

static void mpeg_thread(void)
{
    static int pause_tick = 0;
    static unsigned int pause_track = 0;
    struct event ev;
    int len;
    int free_space_left;
    int unplayed_space_left;
    int amount_to_read;
    int t1, t2;
    int start_offset;
#ifdef HAVE_MAS3587F
    int amount_to_save;
    int writelen;
    int framelen;
    unsigned long saved_header;
    int startpos;
    int rc;
    int offset;
    int countdown;
#endif
    
    is_playing = false;
    play_pending = false;
    playing = false;
    mpeg_file = -1;

    while(1)
    {
#ifdef HAVE_MAS3587F
        if(mpeg_mode == MPEG_DECODER)
        {
#endif
        yield();

        /* Swap if necessary, and don't block on the queue_wait() */
        if(swap_one_chunk())
        {
            queue_wait_w_tmo(&mpeg_queue, &ev, 0);
        }
        else
        {
            DEBUGF("S R:%x W:%x SW:%x\n",
                   mp3buf_read, mp3buf_write, mp3buf_swapwrite);
            queue_wait(&mpeg_queue, &ev);
        }

        start_playback_if_ready();
        
        switch(ev.id)
        {
            case MPEG_PLAY:
                DEBUGF("MPEG_PLAY\n");

#ifdef HAVE_FMRADIO
                /* Silence the A/D input, it may be on because the radio
                   may be playing */
                mas_codec_writereg(6, 0x0000);
#endif

                /* Stop the current stream */
                play_pending = false;
                playing = false;
                paused = false;
                stop_dma();

                reset_mp3_buffer();
                remove_all_tags();

                if(mpeg_file >= 0)
                    close(mpeg_file);

                if ( new_file(0) == -1 )
                {
                    is_playing = false;
                    track_change();
                    break;
                }

                start_offset = (int)ev.data;

                /* mid-song resume? */
                if (start_offset) {
                    struct mp3entry* id3 = &id3tags[tag_read_idx]->id3;
                    lseek(mpeg_file, start_offset, SEEK_SET);
                    id3->offset = start_offset;
                    set_elapsed(id3);
                }
                else {
                    /* skip past id3v2 tag */
                    lseek(mpeg_file, 
                          id3tags[tag_read_idx]->id3.first_frame_offset, 
                          SEEK_SET);

                }

                /* Make it read more data */
                filling = true;
                queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                /* Tell the file loading code that we want to start playing
                   as soon as we have some data */
                play_pending = true;

                update_playlist();
                current_track_counter++;
                break;

            case MPEG_STOP:
                DEBUGF("MPEG_STOP\n");
                is_playing = false;
                paused = false;
                stop_playing();
                mpeg_stop_done = true;
                break;

            case MPEG_PAUSE:
                DEBUGF("MPEG_PAUSE\n");
                /* Stop the current stream */
                paused = true;
                playing = false;
                pause_tick = current_tick;
                pause_track = current_track_counter;
                stop_dma();
                break;

            case MPEG_RESUME:
                DEBUGF("MPEG_RESUME\n");
                /* Continue the current stream */
                paused = false;
                if (!play_pending)
                {
                    playing = true;
                    if ( current_track_counter == pause_track )
                        last_dma_tick += current_tick - pause_tick;
                    else
                        last_dma_tick = current_tick;                        
                    pause_tick = 0;
                    start_dma();
                }
                break;

            case MPEG_NEXT:
                DEBUGF("MPEG_NEXT\n");
                /* is next track in ram? */
                if ( num_tracks_in_memory() > 1 ) {
                    int unplayed_space_left, unswapped_space_left;

                    /* stop the current stream */
                    play_pending = false;
                    playing = false;
                    stop_dma();

                    track_change();
                    mp3buf_read = id3tags[tag_read_idx]->mempos;
                    init_dma();
                    last_dma_tick = current_tick;

                    unplayed_space_left  = get_unplayed_space();
                    unswapped_space_left = get_unswapped_space();

                    /* should we start reading more data? */
                    if(!filling && (unplayed_space_left < low_watermark)) {
                        filling = true;
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                        play_pending = true;
                    } else if(unswapped_space_left &&
                              unswapped_space_left > unplayed_space_left) {
                        /* Stop swapping the data from the previous file */
                        mp3buf_swapwrite = mp3buf_read;
                        play_pending = true;
                    } else {
                        playing = true;
                        if (!paused)
                            start_dma();
                    }
                }
                else {
                    if (!playlist_check(1))
                        break;

                    /* stop the current stream */
                    play_pending = false;
                    playing = false;
                    stop_dma();

                    reset_mp3_buffer();
                    remove_all_tags();

                    /* Open the next file */
                    if (mpeg_file >= 0)
                        close(mpeg_file);
                    
                    if (new_file(1) < 0) {
                        DEBUGF("No more files to play\n");
                        filling = false;
                    } else {
                        /* Make it read more data */
                        filling = true;
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                        
                        /* Tell the file loading code that we want
                           to start playing as soon as we have some data */
                        play_pending = true;

                        update_playlist();
                        current_track_counter++;
                    }
                }
                break;

            case MPEG_PREV: {
                DEBUGF("MPEG_PREV\n");

                if (!playlist_check(-1))
                    break;
                
                /* stop the current stream */
                play_pending = false;
                playing = false;
                stop_dma();

                reset_mp3_buffer();
                remove_all_tags();

                /* Open the next file */
                if (mpeg_file >= 0)
                    close(mpeg_file);

                if (new_file(-1) < 0) {
                    DEBUGF("No more files to play\n");
                    filling = false;
                } else {
                    /* Make it read more data */
                    filling = true;
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                    /* Tell the file loading code that we want to
                       start playing as soon as we have some data */
                    play_pending = true;

                    update_playlist();
                    current_track_counter++;
                }
                break;
            }

            case MPEG_FF_REWIND: {
                struct mp3entry *id3  = mpeg_current_track();
                unsigned int oldtime  = id3->elapsed;
                unsigned int newtime  = (unsigned int)ev.data;
                int curpos, newpos, diffpos;
                DEBUGF("MPEG_FF_REWIND\n");

                id3->elapsed = newtime;

                if (id3->vbr)
                {
                    if (id3->has_toc)
                    {
                        /* Use the TOC to find the new position */
                        unsigned int percent, remainder;
                        int curtoc, nexttoc, plen;
                        
                        percent = (newtime*100)/id3->length; 
                        if (percent > 99)
                            percent = 99;
                        
                        curtoc = id3->toc[percent];
                        
                        if (percent < 99)
                            nexttoc = id3->toc[percent+1];
                        else
                            nexttoc = 256;
                        
                        newpos = (id3->filesize/256)*curtoc;
                        
                        /* Use the remainder to get a more accurate position */
                        remainder   = (newtime*100)%id3->length;
                        remainder   = (remainder*100)/id3->length;
                        plen        = (nexttoc - curtoc)*(id3->filesize/256);
                        newpos     += (plen/100)*remainder;
                    }
                    else
                    {
                        /* No TOC exists, estimate the new position */
                        newpos = (id3->filesize / (id3->length / 1000)) *
                            (newtime / 1000);
                    }
                }
                else if (id3->bpf && id3->tpf)
                    newpos = (newtime/id3->tpf)*id3->bpf;
                else
                {
                    /* Not enough information to FF/Rewind */
                    id3->elapsed = oldtime;
                    break;
                }

                if (newpos >= (int)(id3->filesize - id3->id3v1len))
                {
                    /* Don't seek right to the end of the file so that we can
                       transition properly to the next song */
                    newpos = id3->filesize - id3->id3v1len - 1;
                }
                else if (newpos < (int)id3->first_frame_offset)
                {
                    /* skip past id3v2 tag and other leading garbage */
                    newpos = id3->first_frame_offset;
                }

                if (mpeg_file >= 0)
                    curpos = lseek(mpeg_file, 0, SEEK_CUR);
                else
                    curpos = id3->filesize;

                if (num_tracks_in_memory() > 1)
                {
                    /* We have started loading other tracks that need to be
                       accounted for */
                    int i = tag_read_idx;
                    int j = tag_write_idx - 1;

                    if (j < 0)
                        j = MAX_ID3_TAGS - 1;

                    while (i != j)
                    {
                        curpos += id3tags[i]->id3.filesize;
                        i = (i+1) & MAX_ID3_TAGS_MASK;
                    }
                }

                diffpos = curpos - newpos;

                if(!filling && diffpos >= 0 && diffpos < mp3buflen)
                {
                    int unplayed_space_left, unswapped_space_left;

                    /* We are changing to a position that's already in
                       memory, so we just move the DMA read pointer. */
                    mp3buf_read = mp3buf_write - diffpos;
                    if (mp3buf_read < 0)
                    {
                        mp3buf_read += mp3buflen;
                    }

                    unplayed_space_left  = get_unplayed_space();
                    unswapped_space_left = get_unswapped_space();

                    /* If unswapped_space_left is larger than
                       unplayed_space_left, it means that the swapwrite pointer
                       hasn't yet advanced up to the new location of the read
                       pointer. We just move it, there is no need to swap
                       data that won't be played anyway. */
                    
                    if (unswapped_space_left > unplayed_space_left)
                    {
                        DEBUGF("Moved swapwrite\n");
                        mp3buf_swapwrite = mp3buf_read;
                        play_pending = true;
                    }

                    if (mpeg_file>=0 && unplayed_space_left < low_watermark)
                    {
                        /* We need to load more data before starting */
                        filling = true;
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                        play_pending = true;
                    }
                    else
                    {
                        /* resume will start at new position */
                        init_dma();
                    }
                }
                else
                {
                    /* Move to the new position in the file and start
                       loading data */
                    reset_mp3_buffer();

                    if (num_tracks_in_memory() > 1)
                    {
                        /* We have to reload the current track */
                        close(mpeg_file);
                        remove_all_non_current_tags();
                        mpeg_file = -1;
                    }

                    if (mpeg_file < 0)
                    {
                        mpeg_file = open(id3->path, O_RDONLY);
                        if (mpeg_file < 0)
                        {
                            id3->elapsed = oldtime;
                            break;
                        }
                    }

                    if(-1 == lseek(mpeg_file, newpos, SEEK_SET))
                    {
                        id3->elapsed = oldtime;
                        break;
                    }

                    filling = true;
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                    /* Tell the file loading code that we want to start playing
                       as soon as we have some data */
                    play_pending = true;
                }

                id3->offset = newpos;

                break;
            }

            case MPEG_FLUSH_RELOAD: {
                int  numtracks    = num_tracks_in_memory();
                bool reload_track = false;

                if (numtracks > 1)
                {
                    /* Reload songs */
                    int next = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;

                    /* Reset the buffer */
                    mp3buf_write = id3tags[next]->mempos;

                    /* Reset swapwrite unless we're still swapping current
                       track */
                    if (get_unplayed_space() <= get_playable_space())
                        mp3buf_swapwrite = mp3buf_write;

                    close(mpeg_file);
                    remove_all_non_current_tags();
                    mpeg_file = -1;
                    reload_track = true;
                }
                else if (numtracks == 1 && mpeg_file < 0)
                {
                    reload_track = true;
                }

                if(reload_track && new_file(1) >= 0)
                {
                    /* Tell ourselves that we want more data */
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                    filling = true;
                }

                break;
            }

            case MPEG_NEED_DATA:
                free_space_left = mp3buf_read - mp3buf_write;

                /* We interpret 0 as "empty buffer" */
                if(free_space_left <= 0)
                    free_space_left = mp3buflen + free_space_left;

                unplayed_space_left = mp3buflen - free_space_left;

                /* Make sure that we don't fill the entire buffer */
                free_space_left -= MPEG_HIGH_WATER;

                /* do we have any more buffer space to fill? */
                if(free_space_left <= MPEG_HIGH_WATER)
                {
                    DEBUGF("0\n");
                    filling = false;
                    ata_sleep();
                    break;
                }

                /* Read small chunks while we are below the low water mark */
                if(unplayed_space_left < low_watermark)
                    amount_to_read = MIN(MPEG_LOW_WATER_CHUNKSIZE,
                                         free_space_left);
                else
                    amount_to_read = free_space_left;

                /* Don't read more than until the end of the buffer */
                amount_to_read = MIN(mp3buflen - mp3buf_write, amount_to_read);
#if MEM == 8    
                amount_to_read = MIN(0x100000, amount_to_read);
#endif

                /* Read as much mpeg data as we can fit in the buffer */
                if(mpeg_file >= 0)
                {
                    DEBUGF("R\n");
                    t1 = current_tick;
                    len = read(mpeg_file, mp3buf+mp3buf_write, amount_to_read);
                    if(len > 0)
                    {
                        t2 = current_tick;
                        DEBUGF("time: %d\n", t2 - t1);
                        DEBUGF("R: %x\n", len);

                        /* Now make sure that we don't feed the MAS with ID3V1
                           data */
                        if (len < amount_to_read)
                        {
                            int tagptr = mp3buf_write + len - 128;
                            int i;
                            char *tag = "TAG";
                            int taglen = 128;
                            
                            for(i = 0;i < 3;i++)
                            {
                                if(tagptr >= mp3buflen)
                                    tagptr -= mp3buflen;

                                if(mp3buf[tagptr] != tag[i])
                                    taglen = 0;

                                tagptr++;
                            }

                            if(taglen)
                            {
                                /* Skip id3v1 tag */
                                DEBUGF("Skipping ID3v1 tag\n");
                                len -= taglen;

                                /* The very rare case when the entire tag
                                   wasn't read in this read() call must be
                                   taken care of */
                                if(len < 0)
                                    len = 0;
                            }
                        }

                        mp3buf_write += len;
                        
                        if(mp3buf_write >= mp3buflen)
                        {
                            mp3buf_write = 0;
                            DEBUGF("W\n");
                        }

                        /* Tell ourselves that we want more data */
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                    }
                    else
                    {
                        if(len < 0)
                        {
                            DEBUGF("MPEG read error\n");
                        }
                        
                        close(mpeg_file);
                        mpeg_file = -1;
                    
                        if(new_file(1) < 0)
                        {
                            /* No more data to play */
                            DEBUGF("No more files to play\n");
                            filling = false;
                        }
                        else
                        {
                            /* Tell ourselves that we want more data */
                            queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                        }
                    }
                }
                break;
                
            case MPEG_TRACK_CHANGE:
                track_change();
                break;
                
            case SYS_USB_CONNECTED:
                is_playing = false;
                paused = false;
                stop_playing();
#ifndef SIMULATOR
                
                /* Tell the USB thread that we are safe */
                DEBUGF("mpeg_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&mpeg_queue);
#endif
                break;
                
#ifdef HAVE_MAS3587F
            case MPEG_INIT_RECORDING:
                init_recording();
                init_recording_done = true;
                break;
#endif
            }
#ifdef HAVE_MAS3587F
        }
        else
        {
            queue_wait(&mpeg_queue, &ev);
            switch(ev.id)
            {
                case MPEG_RECORD:
                    DEBUGF("Recording...\n");
                    reset_mp3_buffer();

                    /* Advance the write pointer to make
                       room for an ID3 tag plus a VBR header */
                    mp3buf_write = MPEG_RESERVED_HEADER_SPACE;
                    memset(mp3buf, 0, MPEG_RESERVED_HEADER_SPACE);

                    /* Insert the ID3 header */
                    memcpy(mp3buf, empty_id3_header, sizeof(empty_id3_header));

                    start_recording();
                    demand_irq_enable(true);
                    
                    mpeg_file = open(recording_filename, O_WRONLY|O_CREAT);
                    
                    if(mpeg_file < 0)
                        panicf("recfile: %d", mpeg_file);

                    break;

                case MPEG_STOP:
                    DEBUGF("MPEG_STOP\n");
                    demand_irq_enable(false);
                    stop_recording();
                    
                    /* Save the remaining data in the buffer */
                    stop_pending = true;
                    queue_post(&mpeg_queue, MPEG_SAVE_DATA, 0);
                    break;
                    
                case MPEG_STOP_DONE:
                    DEBUGF("MPEG_STOP_DONE\n");
                    
                    if(mpeg_file >= 0)
                        close(mpeg_file);

                    if(!disable_xing_header)
                    {
                        /* Create the Xing header */
                        mpeg_file = open(recording_filename, O_RDWR);
                        if(mpeg_file < 0)
                            panicf("rec upd: %d (%s)", mpeg_file,
                                   recording_filename);

                        /* If the number of recorded frames have
                           reached 0x7ffff, we can no longer trust it */
                        if(num_recorded_frames == 0x7ffff)
                            num_recorded_frames = 0;
                        
                        /* Read the first MP3 frame from the recorded stream */
                        lseek(mpeg_file, MPEG_RESERVED_HEADER_SPACE, SEEK_SET);
                        rc = read(mpeg_file, &saved_header, 4);
                        if(rc <= 0)
                        {
                            close(mpeg_file);
                            mpeg_file = -1;
                            mpeg_stop_done = true;
                            break;
                        }
                        
                        framelen = create_xing_header(mpeg_file, 0,
                                                      num_rec_bytes, mp3buf,
                                                      num_recorded_frames,
                                                      saved_header, NULL,
                                                      false);
                        
                        lseek(mpeg_file, MPEG_RESERVED_HEADER_SPACE-framelen,
                              SEEK_SET);
                        write(mpeg_file, mp3buf, framelen);
                        close(mpeg_file);
                    }
                    mpeg_file = -1;
                    
#ifdef DEBUG1
                    {
                        int i;
                        for(i = 0;i < 512;i++)
                        {
                            DEBUGF("%d - %d us (%d bytes)\n",
                                   timing_info[i*2],
                                   (timing_info[i*2+1] & 0xffff) *
                                   10000 / 13824,
                                   timing_info[i*2+1] >> 16);
                        }
                    }
#endif
                    mpeg_stop_done = true;
                    break;

                case MPEG_NEW_FILE:
                    /* Make sure we have at least one complete frame
                       in the buffer. If we haven't recorded a single
                       frame within 200ms, the MAS is probably not recording
                       anything, and we bail out. */
                    countdown = 20;
                    amount_to_save = get_unsaved_space();
                    while(countdown-- && amount_to_save < 1800)
                    {
                        sleep(HZ/10);
                        amount_to_save = get_unsaved_space();
                    }

                    if(amount_to_save >= 1800)
                    {
                        /* Now find a frame boundary to split at */
                        startpos = mp3buf_write - 1800;
                        if(startpos < 0)
                            startpos += mp3buflen;
                        
                        {
                            unsigned long tmp[2];
                            /* Find out how the mp3 header should look like */
                            mas_readmem(MAS_BANK_D0, 0xfd1, tmp, 2);
                            saved_header = 0xffe00000 |
                                ((tmp[0] & 0x7c00) << 6) |
                                (tmp[1] & 0xffff);
                            DEBUGF("Header: %08x\n", saved_header);
                        }
                        
                        rc = mem_find_next_frame(startpos, &offset, 1800,
                                                 saved_header);
                        if(rc) /* Header found? */
                        {
                            /* offset will now contain the number of bytes to
                               add to startpos to find the frame boundary */
                            startpos += offset;
                            if(startpos >= mp3buflen)
                                startpos -= mp3buflen;
                        }
                        else
                        {
                            /* No header found. Let's save the whole buffer. */
                            startpos = mp3buf_write;
                        }
                    }
                    else
                    {
                        /* Too few bytes recorded, timeout */
                        startpos = mp3buf_write;
                    }
                    
                    amount_to_save = startpos - mp3buf_read;
                        if(amount_to_save < 0)
                            amount_to_save += mp3buflen;

                    /* First save up to the end of the buffer */
                    writelen = MIN(amount_to_save,
                                   mp3buflen - mp3buf_read);
                    
                    if(writelen)
                    {
                        rc = write(mpeg_file, mp3buf + mp3buf_read, writelen);
                        if(rc < 0)
                        {
                            if(errno == ENOSPC)
                            {
                                mpeg_errno = MPEGERR_DISK_FULL;
                                demand_irq_enable(false);
                                stop_recording();
                                queue_post(&mpeg_queue, MPEG_STOP_DONE, 0);
                                break;
                            }
                            else
                            {
                                panicf("spt wrt: %d", rc);
                            }
                        }
                    }

                    /* Then save the rest */
                    writelen =  amount_to_save - writelen;
                    if(writelen)
                    {
                        rc = write(mpeg_file, mp3buf, writelen);
                        if(rc < 0)
                        {
                            if(errno == ENOSPC)
                            {
                                mpeg_errno = MPEGERR_DISK_FULL;
                                demand_irq_enable(false);
                                stop_recording();
                                queue_post(&mpeg_queue, MPEG_STOP_DONE, 0);
                                break;
                            }
                            else
                            {
                                panicf("spt wrt: %d", rc);
                            }
                        }
                    }

                    /* Advance the buffer pointers */
                    mp3buf_read += amount_to_save;
                    if(mp3buf_read >= mp3buflen)
                        mp3buf_read -= mp3buflen;

                    /* Close the current file */
                    rc = close(mpeg_file);
                    if(rc < 0)
                        panicf("spt cls: %d", rc);

                    /* Open the new file */
                    mpeg_file = open(recording_filename, O_WRONLY|O_CREAT);
                    if(mpeg_file < 0)
                        panicf("sptfile: %d", mpeg_file);
                    break;
                    
                case MPEG_SAVE_DATA:
                    amount_to_save = get_unsaved_space();
                    
                    /* If the result is negative, the write index has
                       wrapped */
                    if(amount_to_save < 0)
                    {
                        amount_to_save += mp3buflen;
                    }
                    
                    DEBUGF("r: %x w: %x\n", mp3buf_read, mp3buf_write);
                    DEBUGF("ats: %x\n", amount_to_save);
                    /* Save data only if the buffer is getting full,
                       or if we should stop recording */
                    if(amount_to_save)
                    {
                        if(mp3buflen -
                           amount_to_save < MPEG_RECORDING_LOW_WATER ||
                           stop_pending)
                        {
                            /* Only save up to the end of the buffer */
                            writelen = MIN(amount_to_save,
                                           mp3buflen - mp3buf_read);
                            
                            DEBUGF("wrl: %x\n", writelen);
                            
                            rc = write(mpeg_file, mp3buf + mp3buf_read,
                                       writelen);
                            
                            if(rc < 0)
                            {
                                if(errno == ENOSPC)
                                {
                                    mpeg_errno = MPEGERR_DISK_FULL;
                                    demand_irq_enable(false);
                                    stop_recording();
                                    queue_post(&mpeg_queue, MPEG_STOP_DONE, 0);
                                    break;
                                }
                                else
                                {
                                    panicf("rec wrt: %d", rc);
                                }
                            }
                            
                            mp3buf_read += amount_to_save;
                            if(mp3buf_read >= mp3buflen)
                                mp3buf_read = 0;
                            
                            rc = fsync(mpeg_file);
                            if(rc < 0)
                                panicf("rec fls: %d", rc);

                            queue_post(&mpeg_queue, MPEG_SAVE_DATA, 0);
                        }
                        else
                        {
                            saving = false;
                            ata_sleep();
                        }
                    }
                    else
                    {
                        /* We have saved all data,
                           time to stop for real */
                        if(stop_pending)
                            queue_post(&mpeg_queue, MPEG_STOP_DONE, 0);
                        saving = false;
                        ata_sleep();
                    }
                    break;
                    
                case MPEG_INIT_PLAYBACK:
                    init_playback();
                    init_playback_done = true;
                    break;

                case SYS_USB_CONNECTED:
                    is_playing = false;
                    paused = false;
                    stop_playing();
#ifndef SIMULATOR
                
                    /* Tell the USB thread that we are safe */
                    DEBUGF("mpeg_thread got SYS_USB_CONNECTED\n");
                    usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    
                    /* Wait until the USB cable is extracted again */
                    usb_wait_for_disconnect(&mpeg_queue);
#endif
                    break;
            }
        }
#endif
    }
}

static void setup_sci0(void)
{
    /* PB15 is I/O, PB14 is IRQ6, PB12 is SCK0, PB9 is TxD0 */
    PBCR1 = (PBCR1 & 0x0cff) | 0x1208;
    
    /* Set PB12 to output */
    or_b(0x10, &PBIORH);

    /* Disable serial port */
    SCR0 = 0x00;

    /* Synchronous, no prescale */
    SMR0 = 0x80;

    /* Set baudrate 1Mbit/s */
    BRR0 = 0x03;

    /* use SCK as serial clock output */
    SCR0 = 0x01;

    /* Clear FER and PER */
    SSR0 &= 0xe7;

    /* Set interrupt ITU2 and SCI0 priority to 0 */
    IPRD &= 0x0ff0;

    /* set PB15 and PB14 to inputs */
     and_b(~0x80, &PBIORH);
     and_b(~0x40, &PBIORH);

    /* Enable End of DMA interrupt at prio 8 */
    IPRC = (IPRC & 0xf0ff) | 0x0800;
    
    /* Enable Tx (only!) */
    SCR0 |= 0x20;
}
#endif /* SIMULATOR */

#ifdef SIMULATOR
static struct mp3entry taginfo;
#endif

struct mp3entry* mpeg_current_track()
{
#ifdef SIMULATOR
    return &taginfo;
#else
    if(num_tracks_in_memory())
        return &(id3tags[tag_read_idx]->id3);
    else
        return NULL;
#endif
}

bool mpeg_has_changed_track(void)
{
    if(last_track_counter != current_track_counter)
    {
        last_track_counter = current_track_counter;
        return true;
    }
    return false;
}

#ifdef HAVE_MAS3587F
void mpeg_init_recording(void)
{
    init_recording_done = false;
    queue_post(&mpeg_queue, MPEG_INIT_RECORDING, NULL);

    while(!init_recording_done)
        sleep_thread();
    wake_up_thread();
}

void mpeg_init_playback(void)
{
    init_playback_done = false;
    queue_post(&mpeg_queue, MPEG_INIT_PLAYBACK, NULL);

    while(!init_playback_done)
        sleep_thread();
    wake_up_thread();
}

static void init_recording(void)
{
    unsigned long val;
    int rc;

    stop_playing();
    is_playing = false;
    paused = false;

    reset_mp3_buffer();
    remove_all_tags();
    
    if(mpeg_file >= 0)
        close(mpeg_file);
    mpeg_file = -1;

    /* Init the recording variables */
    is_recording = false;
    
    mas_reset();
    
    /* Enable the audio CODEC and the DSP core, max analog voltage range */
    rc = mas_direct_config_write(MAS_CONTROL, 0x8c00);
    if(rc < 0)
        panicf("mas_ctrl_w: %d", rc);

    /* Stop the current application */
    val = 0;
    mas_writemem(MAS_BANK_D0,0x7f6,&val,1);    
    do
    {
        mas_readmem(MAS_BANK_D0, 0x7f7, &val, 1);
    } while(val);

    /* Perform black magic as described by the data sheet */
    if((mas_version_code & 0xff) == 2)
    {
        DEBUGF("Performing MAS black magic for B2 version\n");
        mas_writereg(0xa3, 0x98);
        mas_writereg(0x94, 0xfffff);
        val = 0;
        mas_writemem(MAS_BANK_D1, 0, &val, 1);
        mas_writereg(0xa3, 0x90);
    }

    /* Enable A/D Converters */
    mas_codec_writereg(0x0, 0xcccd);

    /* Copy left channel to right (mono mode) */
    mas_codec_writereg(8, 0x8000);
    
    /* ADC scale 0%, DSP scale 100%
       We use the DSP output for monitoring, because it works with all
       sources including S/PDIF */
    mas_codec_writereg(6, 0x0000);
    mas_codec_writereg(7, 0x4000);

    /* No mute */
    val = 0;
    mas_writemem(MAS_BANK_D0, 0x7f9, &val, 1);    
    
    /* Set Demand mode, no monitoring and validate all settings */
    val = 0x125;
    mas_writemem(MAS_BANK_D0, 0x7f1, &val, 1);

    /* Start the encoder application */
    val = 0x40;
    mas_writemem(MAS_BANK_D0, 0x7f6, &val, 1);
    do
    {
        mas_readmem(MAS_BANK_D0, 0x7f7, &val, 1);
    } while(!(val & 0x40));

    /* We have started the recording application with monitoring OFF.
       This is because we want to record at least one frame to fill the DMA
       buffer, because the silly MAS will not negate EOD until at least one
       DMA transfer has taken place.
       Now let's wait for some data to be encoded. */
    sleep(20);
    
    /* Disable IRQ6 */
    IPRB &= 0xff0f;

    mpeg_mode = MPEG_ENCODER;

    DEBUGF("MAS Recording application started\n");
}

static void init_playback(void)
{
    unsigned long val;
    int rc;

    stop_dma();
    
    mas_reset();
    
    /* Enable the audio CODEC and the DSP core, max analog voltage range */
    rc = mas_direct_config_write(MAS_CONTROL, 0x8c00);
    if(rc < 0)
        panicf("mas_ctrl_w: %d", rc);

    /* Stop the current application */
    val = 0;
    mas_writemem(MAS_BANK_D0,0x7f6,&val,1);    
    do
    {
        mas_readmem(MAS_BANK_D0, 0x7f7, &val, 1);
    } while(val);
    
    /* Enable the D/A Converter */
    mas_codec_writereg(0x0, 0x0001);

    /* ADC scale 0%, DSP scale 100% */
    mas_codec_writereg(6, 0x0000);
    mas_codec_writereg(7, 0x4000);

    /* Disable SDO and SDI */
    val = 0x0d;
    mas_writemem(MAS_BANK_D0,0x7f2,&val,1);

    /* Set Demand mode and validate all settings */
    val = 0x25;
    mas_writemem(MAS_BANK_D0,0x7f1,&val,1);

    /* Start the Layer2/3 decoder applications */
    val = 0x0c;
    mas_writemem(MAS_BANK_D0,0x7f6,&val,1);
    do
    {
        mas_readmem(MAS_BANK_D0, 0x7f7, &val, 1);
    } while((val & 0x0c) != 0x0c);

    mpeg_sound_channel_config(MPEG_SOUND_STEREO);

    mpeg_mode = MPEG_DECODER;

    /* set IRQ6 to edge detect */
    ICR |= 0x02;

    /* set IRQ6 prio 8 */
    IPRB = ( IPRB & 0xff0f ) | 0x0080;

    DEBUGF("MAS Decoding application started\n");
}

void mpeg_record(char *filename)
{
    mpeg_errno = 0;
    
    strncpy(recording_filename, filename, MAX_PATH - 1);
    recording_filename[MAX_PATH - 1] = 0;
    
    num_rec_bytes = 0;
    disable_xing_header = false;
    queue_post(&mpeg_queue, MPEG_RECORD, NULL);
}

static void start_recording(void)
{
    unsigned long val;

    num_recorded_frames = 0;
    
    /* Stop monitoring and record for real */
    mas_readmem(MAS_BANK_D0, 0x7f1, &val, 1);
    val &= ~(1 << 10);
    val |= 1;
    mas_writemem(MAS_BANK_D0, 0x7f1, &val, 1);

    /* Wait until the DSP has accepted the settings */
    do
    {
        mas_readmem(MAS_BANK_D0, 0x7f1, &val,1);
    } while(val & 1);

    sleep(20);
    
    /* Store the current time */
    record_start_time = current_tick;

    is_recording = true;
    stop_pending = false;
    saving = false;
}

static void stop_recording(void)
{
    unsigned long val;

    is_recording = false;

    /* Read the number of frames recorded */
    mas_readmem(MAS_BANK_D0, 0xfd0, &num_recorded_frames, 1);
    
    /* Start monitoring */
    mas_readmem(MAS_BANK_D0, 0x7f1, &val, 1);
    val |= (1 << 10) | 1;
    mas_writemem(MAS_BANK_D0, 0x7f1, &val, 1);

    /* Wait until the DSP has accepted the settings */
    do
    {
        mas_readmem(MAS_BANK_D0, 0x7f1, &val,1);
    } while(val & 1);
    
    drain_dma_buffer();
}

void mpeg_new_file(char *filename)
{
    mpeg_errno = 0;
    
    strncpy(recording_filename, filename, MAX_PATH - 1);
    recording_filename[MAX_PATH - 1] = 0;

    disable_xing_header = true;

    /* Store the current time */
    record_start_time = current_tick;

    queue_post(&mpeg_queue, MPEG_NEW_FILE, NULL);
}

unsigned long mpeg_recorded_time(void)
{
    if(is_recording)
        return current_tick - record_start_time;
    else
        return 0;
}

unsigned long mpeg_num_recorded_bytes(void)
{
    if(is_recording)
        return num_rec_bytes;
    else
        return 0;
}

#endif

void mpeg_play(int offset)
{
#ifdef SIMULATOR
    char* trackname;
    int steps=0;

    is_playing = true;
    
    do {
        trackname = playlist_peek( steps );
        if (!trackname)
            break;
        if(mp3info(&taginfo, trackname)) {
            /* bad mp3, move on */
            if(++steps > playlist_amount())
                break;
            continue;
        }
        playlist_next(steps);
        taginfo.offset = offset;
        set_elapsed(&taginfo);
        is_playing = true;
        playing = true;
        break;
    } while(1);
#else
    is_playing = true;
    
    queue_post(&mpeg_queue, MPEG_PLAY, (void*)offset);
#endif

    mpeg_errno = 0;
}

void mpeg_stop(void)
{
#ifndef SIMULATOR
    mpeg_stop_done = false;
    queue_post(&mpeg_queue, MPEG_STOP, NULL);
    while(!mpeg_stop_done)
        yield();
#else
    is_playing = false;
    playing = false;
#endif
    
}

void mpeg_pause(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_PAUSE, NULL);
#else
    is_playing = true;
    playing = false;
    paused = true;
#endif
}

void mpeg_resume(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_RESUME, NULL);
#else
    is_playing = true;
    playing = true;
    paused = false;
#endif
}

void mpeg_next(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_NEXT, NULL);
#else
    char* file;
    int steps = 1;
    int index;

    do {
        file = playlist_peek(steps);
        if(!file)
            break;
        if(mp3info(&taginfo, file)) {
            if(++steps > playlist_amount())
                break;
            continue;
        }
        index = playlist_next(steps);
        taginfo.index = index;
        current_track_counter++;
        is_playing = true;
        playing = true;
        break;
    } while(1);
#endif
}

void mpeg_prev(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_PREV, NULL);
#else
    char* file;
    int steps = -1;
    int index;

    do {
        file = playlist_peek(steps);
        if(!file)
            break;
        if(mp3info(&taginfo, file)) {
            steps--;
            continue;
        }
        index = playlist_next(steps);
        taginfo.index = index;
        current_track_counter++;
        is_playing = true;
        playing = true;
        break;
    } while(1);
#endif
}

void mpeg_ff_rewind(int newtime)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_FF_REWIND, (void *)newtime);
#else
    (void)newtime;
#endif
}

void mpeg_flush_and_reload_tracks(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_FLUSH_RELOAD, NULL);
#endif
}

int mpeg_status(void)
{
    int ret = 0;

    if(is_playing)
        ret |= MPEG_STATUS_PLAY;

    if(paused)
        ret |= MPEG_STATUS_PAUSE;
    
#ifdef HAVE_MAS3587F
    if(is_recording)
        ret |= MPEG_STATUS_RECORD;
#endif

    if(mpeg_errno)
        ret |= MPEG_STATUS_ERROR;
    
    return ret;
}

unsigned int mpeg_error(void)
{
    return mpeg_errno;
}

void mpeg_error_clear(void)
{
    mpeg_errno = 0;
}

#ifndef SIMULATOR
#ifdef HAVE_MAS3507D
int current_left_volume = 0;  /* all values in tenth of dB */
int current_right_volume = 0;  /* all values in tenth of dB */
int current_treble = 0;
int current_bass = 0;
int current_balance = 0;

/* convert tenth of dB volume to register value */
static int tenthdb2reg(int db) {
    if (db < -540)
        return (db + 780) / 30;
    else
        return (db + 660) / 15;
}

void set_prescaled_volume(void)
{
    int prescale;
    int l, r;

    prescale = MAX(current_bass, current_treble);
    if (prescale < 0)
        prescale = 0;  /* no need to prescale if we don't boost
                          bass or treble */

    mas_writereg(MAS_REG_KPRESCALE, prescale_table[prescale/10]);
    
    /* gain up the analog volume to compensate the prescale reduction gain */
    l = current_left_volume + prescale;
    r = current_right_volume + prescale;

    dac_volume(tenthdb2reg(l), tenthdb2reg(r), false);
}
#endif /* HAVE_MAS3507D */
#endif /* !SIMULATOR */

void mpeg_sound_set(int setting, int value)
{
#ifdef SIMULATOR
    setting = value;
#else
#ifdef HAVE_MAS3507D
    int l, r;
#else
    int tmp;
#endif

    if(!mpeg_is_initialized)
        return;
    
    switch(setting)
    {
        case SOUND_VOLUME:
#ifdef HAVE_MAS3587F
            tmp = 0x7f00 * value / 100;
            mas_codec_writereg(0x10, tmp & 0xff00);
#else
            l = value;
            r = value;
            
            if(current_balance > 0)
            {
                l -= current_balance;
                if(l < 0)
                    l = 0;
            }
            
            if(current_balance < 0)
            {
                r += current_balance;
                if(r < 0)
                    r = 0;
            }
            
            l = 0x38 * l / 100;
            r = 0x38 * r / 100;

            /* store volume in tenth of dB */
            current_left_volume = ( l < 0x08 ? l*30 - 780 : l*15 - 660 );
            current_right_volume = ( r < 0x08 ? r*30 - 780 : r*15 - 660 );

            set_prescaled_volume();
#endif
            break;

        case SOUND_BALANCE:
#ifdef HAVE_MAS3587F
            tmp = ((value * 127 / 100) & 0xff) << 8;
            mas_codec_writereg(0x11, tmp & 0xff00);
#else
            /* Convert to percent */
            current_balance = value * 2;
#endif
            break;

        case SOUND_BASS:
#ifdef HAVE_MAS3587F
            tmp = (((value-12) * 8) & 0xff) << 8;
            mas_codec_writereg(0x14, tmp & 0xff00);
#else    
            mas_writereg(MAS_REG_KBASS, bass_table[value]);
            current_bass = (value-15) * 10;
            set_prescaled_volume();
#endif
            break;

        case SOUND_TREBLE:
#ifdef HAVE_MAS3587F
            tmp = (((value-12) * 8) & 0xff) << 8;
            mas_codec_writereg(0x15, tmp & 0xff00);
#else    
            mas_writereg(MAS_REG_KTREBLE, treble_table[value]);
            current_treble = (value-15) * 10;
            set_prescaled_volume();
#endif
            break;
            
#ifdef HAVE_MAS3587F
        case SOUND_SUPERBASS:
            if (value) {
                tmp = MAX(MIN(value * 12, 0x7f), 0);
                mas_codec_writereg(MAS_REG_KMDB_STR, (tmp & 0xff) << 8);
                tmp = 0x30; /* MDB_HAR: Space for experiment here */
                mas_codec_writereg(MAS_REG_KMDB_HAR, (tmp & 0xff) << 8);
                tmp = 60 / 10; /* calculate MDB_FC, 60hz - experiment here,
                                  this would depend on the earphones...
                                  perhaps make it tunable? */
                mas_codec_writereg(MAS_REG_KMDB_FC, (tmp & 0xff) << 8);
                tmp = (3 * tmp) / 2; /* calculate MDB_SHAPE */
                mas_codec_writereg(MAS_REG_KMDB_SWITCH,
                    ((tmp & 0xff) << 8) /* MDB_SHAPE */
                    | 2);  /* MDB_SWITCH enable */
            } else {
                mas_codec_writereg(MAS_REG_KMDB_STR, 0);
                mas_codec_writereg(MAS_REG_KMDB_HAR, 0);
                mas_codec_writereg(MAS_REG_KMDB_SWITCH, 0); /* MDB_SWITCH disable */
            }
            break;
            
        case SOUND_LOUDNESS:
            tmp = MAX(MIN(value * 4, 0x44), 0);
            mas_codec_writereg(MAS_REG_KLOUDNESS, (tmp & 0xff) << 8);
            break;
            
        case SOUND_AVC:
            switch (value) {
                case 1: /* 2s */
                    tmp = (0x2 << 8) | (0x8 << 12);
                    break;
                case 2: /* 4s */
                    tmp = (0x4 << 8) | (0x8 << 12);
                    break;
                case 3: /* 8s */
                    tmp = (0x8 << 8) | (0x8 << 12);
                    break;
                case -1: /* turn off and then turn on again to decay quickly */
                    tmp = mas_codec_readreg(MAS_REG_KAVC);
                    mas_codec_writereg(MAS_REG_KAVC, 0);
                    break;
                default: /* off */
                    tmp = 0;
                    break;  
            }
            mas_codec_writereg(MAS_REG_KAVC, tmp);
            break;
#endif
        case SOUND_CHANNELS:
            mpeg_sound_channel_config(value);
            break;
    }
#endif /* SIMULATOR */
}

int mpeg_val2phys(int setting, int value)
{
    int result = 0;
    
    switch(setting)
    {
        case SOUND_VOLUME:
            result = value;
            break;
        
        case SOUND_BALANCE:
            result = value * 2;
            break;
        
        case SOUND_BASS:
#ifdef HAVE_MAS3587F
            result = value - 12;
#else
            result = value - 15;
#endif
            break;
        
        case SOUND_TREBLE:
#ifdef HAVE_MAS3587F
            result = value - 12;
#else
            result = value - 15;
#endif
            break;

#ifdef HAVE_MAS3587F
        case SOUND_LOUDNESS:
            result = value;
            break;
            
        case SOUND_SUPERBASS:
            result = value * 10;
            break;

        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
            result = (value - 2) * 15;
            break;

        case SOUND_MIC_GAIN:
            result = value * 15 + 210;
            break;
#endif
    }
    return result;
}

int mpeg_phys2val(int setting, int value)
{
    int result = 0;
    
    switch(setting)
    {
        case SOUND_VOLUME:
            result = value;
            break;
        
        case SOUND_BALANCE:
            result = value / 2;
            break;
        
        case SOUND_BASS:
#ifdef HAVE_MAS3587F
            result = value + 12;
#else
            result = value + 15;
#endif
            break;
        
        case SOUND_TREBLE:
#ifdef HAVE_MAS3587F
            result = value + 12;
#else
            result = value + 15;
#endif
            break;

#ifdef HAVE_MAS3587F
        case SOUND_SUPERBASS:
            result = value / 10;
            break;

        case SOUND_LOUDNESS:
        case SOUND_AVC:
        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
        case SOUND_MIC_GAIN:
            result = value;
            break;
#endif
    }

    return result;
}


void mpeg_sound_channel_config(int configuration)
{
#ifdef SIMULATOR
    (void)configuration;
#else
    unsigned long val_ll = 0x80000;
    unsigned long val_lr = 0;
    unsigned long val_rl = 0;
    unsigned long val_rr = 0x80000;
    
    switch(configuration)
    {
        case MPEG_SOUND_STEREO:
            val_ll = 0x80000;
            val_lr = 0;
            val_rl = 0;
            val_rr = 0x80000;
            break;

        case MPEG_SOUND_MONO:
            val_ll = 0xc0000;
            val_lr = 0xc0000;
            val_rl = 0xc0000;
            val_rr = 0xc0000;
            break;

        case MPEG_SOUND_MONO_LEFT:
            val_ll = 0x80000;
            val_lr = 0x80000;
            val_rl = 0;
            val_rr = 0;
            break;

        case MPEG_SOUND_MONO_RIGHT:
            val_ll = 0;
            val_lr = 0;
            val_rl = 0x80000;
            val_rr = 0x80000;
            break;

        case MPEG_SOUND_STEREO_NARROW:
            val_ll = 0xa0000;
            val_lr = 0xe0000;
            val_rl = 0xe0000;
            val_rr = 0xa0000;
            break;

        case MPEG_SOUND_STEREO_WIDE:
            val_ll = 0x80000;
            val_lr = 0x40000;
            val_rl = 0x40000;
            val_rr = 0x80000;
            break;

        case MPEG_SOUND_KARAOKE:
            val_ll = 0x80001;
            val_lr = 0x7ffff;
            val_rl = 0x7ffff;
            val_rr = 0x80001;
            break;
    }

#ifdef HAVE_MAS3587F
    mas_writemem(MAS_BANK_D0, 0x7fc, &val_ll, 1); /* LL */
    mas_writemem(MAS_BANK_D0, 0x7fd, &val_lr, 1); /* LR */
    mas_writemem(MAS_BANK_D0, 0x7fe, &val_rl, 1); /* RL */
    mas_writemem(MAS_BANK_D0, 0x7ff, &val_rr, 1); /* RR */
#else
    mas_writemem(MAS_BANK_D1, 0x7f8, &val_ll, 1); /* LL */
    mas_writemem(MAS_BANK_D1, 0x7f9, &val_lr, 1); /* LR */
    mas_writemem(MAS_BANK_D1, 0x7fa, &val_rl, 1); /* RL */
    mas_writemem(MAS_BANK_D1, 0x7fb, &val_rr, 1); /* RR */
#endif
#endif
}

#ifdef HAVE_MAS3587F
/* This function works by telling the decoder that we have another
   crystal frequency than we actually have. It will adjust its internal
   parameters and the result is that the audio is played at another pitch.

   The pitch value is in tenths of percent.
*/
void mpeg_set_pitch(int pitch)
{
    unsigned long val;

    /* invert pitch value */
    pitch = 1000000/pitch;

    /* Calculate the new (bogus) frequency */
    val = 18432*pitch/1000;
    
    mas_writemem(MAS_BANK_D0,0x7f3,&val,1);

    /* We must tell the MAS that the frequency has changed.
       This will unfortunately cause a short silence. */
    val = 0x25;
    mas_writemem(MAS_BANK_D0,0x7f1,&val,1);
}
#endif

#ifdef HAVE_MAS3587F
void mpeg_set_recording_options(int frequency, int quality,
                                int source, int channel_mode,
                                bool editable)
{
    bool is_mpeg1;
    unsigned long val;

    is_mpeg1 = (frequency < 3)?true:false;

    rec_version_index = is_mpeg1?3:2;
    rec_frequency_index = frequency % 3;
    
    val = (quality << 17) |
        (rec_frequency_index << 10) |
        ((is_mpeg1?1:0) << 9) |
        (1 << 8) | /* CRC on */
        (((channel_mode * 2 + 1) & 3) << 6) |
        (1 << 5) /* MS-stereo */ |
        (1 << 2) /* Is an original */;
    mas_writemem(MAS_BANK_D0, 0x7f0, &val,1);

    DEBUGF("mas_writemem(MAS_BANK_D0, 0x7f0, %x)\n", val);

    val = editable?4:0;
    mas_writemem(MAS_BANK_D0, 0x7f9, &val,1);

    DEBUGF("mas_writemem(MAS_BANK_D0, 0x7f9, %x)\n", val);

    val = ((!is_recording << 10) | /* Monitoring */
        ((source < 2)?1:2) << 8) | /* Input select */
        (1 << 5) | /* SDO strobe invert */
        ((is_mpeg1?0:1) << 3) |
        (1 << 2) | /* Inverted SIBC clock signal */
        1; /* Validate */
    mas_writemem(MAS_BANK_D0, 0x7f1, &val,1);

    DEBUGF("mas_writemem(MAS_BANK_D0, 0x7f1, %x)\n", val);

    drain_dma_buffer();
    
    if(source == 0) /* Mic */
    {
        /* Copy left channel to right (mono mode) */
        mas_codec_writereg(8, 0x8000);
    }
    else
    {
        /* Stereo input mode */
        mas_codec_writereg(8, 0);
    }
}

/* If use_mic is true, the left gain is used */
void mpeg_set_recording_gain(int left, int right, bool use_mic)
{
    /* Enable both left and right A/D */
    mas_codec_writereg(0x0,
                       (left << 12) |
                       (right << 8) |
                       (left << 4) |
                       (use_mic?0x0008:0) | /* Connect left A/D to mic */
                       0x0007);
}

#endif

#ifdef SIMULATOR
static char mpeg_stack[DEFAULT_STACK_SIZE];
static char mpeg_thread_name[] = "mpeg";
static void mpeg_thread(void)
{
    struct mp3entry* id3;
    while ( 1 ) {
        if (is_playing) {
            id3 = mpeg_current_track();
            if (!paused)
            {
                id3->elapsed+=1000;
                id3->offset+=1000;
            }
            if (id3->elapsed>=id3->length)
                mpeg_next();
        }
        sleep(HZ);
    }
}
#endif

void mpeg_init(int volume, int bass, int treble, int balance, int loudness, 
    int bass_boost, int avc, int channel_config)
{
    mpeg_errno = 0;
    
#ifdef SIMULATOR
    volume = bass = treble = balance = loudness
        = bass_boost = avc = channel_config;
    create_thread(mpeg_thread, mpeg_stack,
                  sizeof(mpeg_stack), mpeg_thread_name);
#else
#ifdef HAVE_MAS3507D
    unsigned long val;
    loudness = bass_boost = avc;
#endif

    setup_sci0();

#ifdef HAVE_MAS3587F
    or_b(0x08, &PAIORH); /* output for /PR */
    init_playback();
    
    mas_version_code = mas_readver();
    DEBUGF("MAS3587 derivate %d, version B%d\n",
           (mas_version_code & 0xff00) >> 8, mas_version_code & 0xff);
#endif

#ifdef HAVE_DAC3550A
    dac_init();
#endif
    
#ifdef HAVE_MAS3507D
    and_b(~0x20, &PBDRL);
    sleep(HZ/5);
    or_b(0x20, &PBDRL);
    sleep(HZ/5);
    
    /* set IRQ6 to edge detect */
    ICR |= 0x02;

    /* set IRQ6 prio 8 */
    IPRB = ( IPRB & 0xff0f ) | 0x0080;

    mas_readmem(MAS_BANK_D1, 0xff7, &mas_version_code, 1);
    
    mas_writereg(0x3b, 0x20); /* Don't ask why. The data sheet doesn't say */
    mas_run(1);
    sleep(HZ);

    /* Clear the upper 12 bits of the 32-bit samples */
    mas_writereg(0xc5, 0);
    mas_writereg(0xc6, 0);
    
    /* We need to set the PLL for a 14.1318MHz crystal */
    if(mas_version_code == 0x0601) /* Version F10? */
    {
        val = 0x5d9d0;
        mas_writemem(MAS_BANK_D0, 0x32d, &val, 1);
        val = 0xfffceceb;
        mas_writemem(MAS_BANK_D0, 0x32e, &val, 1);
        val = 0x0;
        mas_writemem(MAS_BANK_D0, 0x32f, &val, 1);
        mas_run(0x475);
    }
    else
    {
        val = 0x5d9d0;
        mas_writemem(MAS_BANK_D0, 0x36d, &val, 1);
        val = 0xfffceceb;
        mas_writemem(MAS_BANK_D0, 0x36e, &val, 1);
        val = 0x0;
        mas_writemem(MAS_BANK_D0, 0x36f, &val, 1);
        mas_run(0xfcb);
    }
    
#endif

    mp3buflen = mp3end - mp3buf;

    queue_init(&mpeg_queue);
    create_thread(mpeg_thread, mpeg_stack,
                  sizeof(mpeg_stack), mpeg_thread_name);

#ifdef HAVE_MAS3507D
    mas_poll_start(1);

    mas_writereg(MAS_REG_KPRESCALE, 0xe9400);
    dac_enable(true);

    mpeg_sound_channel_config(channel_config);
#endif

#ifdef HAVE_MAS3587F
    ICR &= ~0x0010; /* IRQ3 level sensitive */
    PACR1 = (PACR1 & 0x3fff) | 0x4000; /* PA15 is IRQ3 */
#endif

    /* Must be done before calling mpeg_sound_set() */
    mpeg_is_initialized = true;
    
    mpeg_sound_set(SOUND_BASS, bass);
    mpeg_sound_set(SOUND_TREBLE, treble);
    mpeg_sound_set(SOUND_BALANCE, balance);
    mpeg_sound_set(SOUND_VOLUME, volume);
    
#ifdef HAVE_MAS3587F
    mpeg_sound_channel_config(channel_config);
    mpeg_sound_set(SOUND_LOUDNESS, loudness);
    mpeg_sound_set(SOUND_SUPERBASS, bass_boost);
    mpeg_sound_set(SOUND_AVC, avc);
#endif
#endif /* !SIMULATOR */

    memset(id3tags, sizeof(id3tags), 0);
    memset(_id3tags, sizeof(id3tags), 0);

#ifdef HAVE_MAS3587F
    if(read_hw_mask() & PR_ACTIVE_HIGH)
        inverted_pr = true;
    else
        inverted_pr = false;
#endif
    
#ifdef DEBUG
    dbg_timer_start();
    dbg_cnt2us(0);
#endif
}
