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
#include "malloc.h"
#include "string.h"
#ifndef SIMULATOR
#include "i2c.h"
#include "mas.h"
#include "dac.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "usb.h"
#include "file.h"
#endif

#define MPEG_CHUNKSIZE  0x180000
#define MPEG_SWAP_CHUNKSIZE  0x8000
#define MPEG_HIGH_WATER  2
#define MPEG_LOW_WATER  0x40000
#define MPEG_LOW_WATER_CHUNKSIZE  0x10000

#define MPEG_PLAY         1
#define MPEG_STOP         2
#define MPEG_PAUSE        3
#define MPEG_RESUME       4
#define MPEG_NEXT         5
#define MPEG_PREV         6
#define MPEG_NEED_DATA    100
#define MPEG_SWAP_DATA    101
#define MPEG_TRACK_CHANGE 102

extern char* peek_next_track(int type);
extern char* peek_prev_track(int type);

static char *units[] =
{
    "%",    /* Volume */
    "dB",   /* Bass */
    "dB",   /* Treble */
    "",     /* Balance */
    "dB",   /* Loudness */
    "%"     /* Bass boost */
};

static int numdecimals[] =
{
    0,    /* Volume */
    0,    /* Bass */
    0,    /* Treble */
    0,    /* Balance */
    0,    /* Loudness */
    0     /* Bass boost */
};

static int minval[] =
{
    0,    /* Volume */
    0,    /* Bass */
    0,    /* Treble */
    0,    /* Balance */
    0,    /* Loudness */
    0     /* Bass boost */
};

static int maxval[] =
{
    50,    /* Volume */
#ifdef ARCHOS_RECORDER
    24,    /* Bass */
    24,    /* Treble */
#else
    30,    /* Bass */
    30,    /* Treble */
#endif
    100,   /* Balance */
    17,    /* Loudness */
    10     /* Bass boost */
};

static int defaultval[] =
{
    70/2,    /* Volume */
#ifdef ARCHOS_RECORDER
    12+6,    /* Bass */
    12+6,    /* Treble */
#else
    15+7,    /* Bass */
    15+7,    /* Treble */
#endif
    50,      /* Balance */
    0,       /* Loudness */
    0        /* Bass boost */
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
};

static struct id3tag *id3tags[MAX_ID3_TAGS];

static unsigned int current_track_counter = 0;
static unsigned int last_track_counter = 0;

#ifndef SIMULATOR

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
#ifdef DEBUG
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
    struct id3tag *tag = id3tags[tag_read_idx];
    
    if(num_tracks_in_memory() > 0)
    {
        /* First move the index, so nobody tries to access the tag */
        tag_read_idx = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;

        /* Now delete it */
        id3tags[oldidx] = NULL;
        free(tag);
        debug_tags();
    }
}

static void remove_all_tags(void)
{
    int i;
    
    for(i = 0;i < MAX_ID3_TAGS;i++)
        remove_current_tag();
    debug_tags();
}
#endif

#ifndef SIMULATOR
static int last_dma_tick = 0;
static int pause_tick = 0;

#ifndef ARCHOS_RECORDER
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

static unsigned char fliptable[] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static unsigned short big_fliptable[65536];

static struct event_queue mpeg_queue;
static char mpeg_stack[DEFAULT_STACK_SIZE + 0x1000];
static char mpeg_thread_name[] = "mpeg";

/* defined in linker script */
extern unsigned char mp3buf[];
extern unsigned char mp3end[];

static int mp3buflen;
static int mp3buf_write;
static int mp3buf_swapwrite;
static int mp3buf_read;

static int last_dma_chunk_size;

static bool dma_on;  /* The DMA is active */
static bool playing; /* We are playing an MP3 stream */
static bool play_pending; /* We are about to start playing */
static bool filling; /* We are filling the buffer with data from disk */

static int mpeg_file;

static void create_fliptable(void)
{
    int i;

    for(i = 0;i < 65536;i++)
    {
        big_fliptable[i] = fliptable[i & 0xff] | (fliptable[i >> 8] << 8);
    }
}

static void mas_poll_start(int interval_in_ms)
{
    unsigned int count;

    count = FREQ / 1000 / 8 * interval_in_ms;

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

    TSTR |= 0x02; /* Start timer 2 */
}

static void init_dma(void)
{
    SAR3 = (unsigned int) mp3buf + mp3buf_read;
    DAR3 = 0x5FFFEC3;
    CHCR3 &= ~0x0002; /* Clear interrupt */
    CHCR3 = 0x1504; /* Single address destination, TXI0, IE=1 */
    last_dma_chunk_size = MIN(65536, mp3buf_write - mp3buf_read);
    DTCR3 = last_dma_chunk_size & 0xffff;
    DMAOR = 0x0001; /* Enable DMA */
    CHCR3 |= 0x0001; /* Enable DMA IRQ */
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

static void dma_tick(void)
{
    if(playing)
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
}

static void bitswap(unsigned short *data, int length)
{
    int i = length;
    while(i--)
    {
        data[i] = big_fliptable[data[i]];
    }
}

static void reset_mp3_buffer(void)
{
    mp3buf_read = 0;
    mp3buf_write = 0;
    mp3buf_swapwrite = 0;
}

#pragma interrupt
void IRQ6(void)
{
    stop_dma();
}

#pragma interrupt
void DEI3(void)
{
    int unplayed_space_left;
    int space_until_end_of_buffer;
    int track_offset = (tag_read_idx+1) & MAX_ID3_TAGS_MASK;

    if(playing)
    {
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
        
        unplayed_space_left = mp3buf_write - mp3buf_read;
        if(unplayed_space_left < 0)
            unplayed_space_left = mp3buflen + unplayed_space_left;

        space_until_end_of_buffer = mp3buflen - mp3buf_read;
        
        if(!filling && unplayed_space_left < MPEG_LOW_WATER)
        {
            filling = true;
            queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
        }
        
        if(unplayed_space_left)
        {
            last_dma_chunk_size = MIN(65536, unplayed_space_left);
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
        }
        else
        {
            DEBUGF("No more MP3 data. Stopping.\n");
            queue_post(&mpeg_queue, MPEG_TRACK_CHANGE, 0);
            CHCR3 = 0; /* Stop DMA interrupt */
            playing = false;
        }
    }

    CHCR3 &= ~0x0002; /* Clear DMA interrupt */
}

#pragma interrupt
void IMIA1(void)
{
    dma_tick();
    TSR1 &= ~0x01;
}

static void add_track_to_tag_list(char *filename)
{
    struct id3tag *t;

    /* grab id3 tag of new file and
       remember where in memory it starts */
    t = malloc(sizeof(struct id3tag));
    if(t)
    {
        mp3info(&(t->id3), filename);
        t->mempos = mp3buf_write;
        t->id3.elapsed = 0;
        if(!append_tag(t))
        {
            free(t);
            DEBUGF("Tag list is full\n");
        }
    }
    else
    {
        DEBUGF("No memory available for id3 tag");
    }
}

/* If next_track is true, opens the next track, if false, opens prev track */
static int new_file(bool next_track)
{
    char *trackname;

    do {
        trackname = peek_next_track( next_track ? 1 : -1 );
        if ( !trackname )
            return -1;
        
        DEBUGF("playing %s\n", trackname);
        
        mpeg_file = open(trackname, O_RDONLY);
        if(mpeg_file < 0) {
            DEBUGF("Couldn't open file: %s\n",trackname);
        }
        else
        {
            add_track_to_tag_list(trackname);
        }
    } while ( mpeg_file < 0 );

    return 0;
}

static void stop_playing(void)
{
    /* Stop the current stream */
    playing = false;
    filling = false;
    if(mpeg_file >= 0)
        close(mpeg_file);
    mpeg_file = -1;
    stop_dma();
    remove_all_tags();
}

static void mpeg_thread(void)
{
    struct event ev;
    int len;
    int free_space_left;
    int unplayed_space_left;
    int amount_to_read;
    int amount_to_swap;
    
    play_pending = false;
    playing = false;
    mpeg_file = -1;

    while(1)
    {
        DEBUGF("S R:%x W:%x SW:%x\n",
               mp3buf_read, mp3buf_write, mp3buf_swapwrite);
        yield();
        queue_wait(&mpeg_queue, &ev);
        switch(ev.id)
        {
            case MPEG_PLAY:
                DEBUGF("MPEG_PLAY %s\n",ev.data);
                /* Stop the current stream */
                play_pending = false;
                playing = false;
                stop_dma();

                reset_mp3_buffer();
                remove_all_tags();

                if(mpeg_file >= 0)
                    close(mpeg_file);

                mpeg_file = open((char*)ev.data, O_RDONLY);
                while (mpeg_file < 0) {
                    DEBUGF("Couldn't open file: %s\n",ev.data);
                    if ( new_file(true) == -1 )
                        return;
                }

                add_track_to_tag_list((char *)ev.data);

                /* Make it read more data */
                filling = true;
                queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                /* Tell the file loading code that we want to start playing
                   as soon as we have some data */
                play_pending = true;

                current_track_counter++;
                break;

            case MPEG_STOP:
                DEBUGF("MPEG_STOP\n");
                stop_playing();
                break;

            case MPEG_PAUSE:
                DEBUGF("MPEG_PAUSE\n");
                /* Stop the current stream */
                playing = false;
                pause_tick = current_tick;
                stop_dma();
                break;

            case MPEG_RESUME:
                DEBUGF("MPEG_RESUME\n");
                /* Continue the current stream */
                playing = true;
                last_dma_tick += current_tick - pause_tick;
                pause_tick = 0;
                start_dma();
                break;

            case MPEG_NEXT:
                DEBUGF("MPEG_NEXT\n");
                /* stop the current stream */
                play_pending = false;
                playing = false;
                stop_dma();

                reset_mp3_buffer();
                remove_all_tags();
                
                /* Open the next file */
                if (mpeg_file >= 0)
                    close(mpeg_file);

                if (new_file(true) < 0) {
                    DEBUGF("No more files to play\n");
                    filling = false;
                } else {
                    /* Make it read more data */
                    filling = true;
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                    /* Tell the file loading code that we want to start playing
                       as soon as we have some data */
                    play_pending = true;

                    current_track_counter++;
                }
                break;

            case MPEG_PREV:
                DEBUGF("MPEG_PREV\n");
                /* stop the current stream */
                play_pending = false;
                playing = false;
                stop_dma();

                reset_mp3_buffer();
                remove_all_tags();

                /* Open the next file */
                if (mpeg_file >= 0)
                    close(mpeg_file);

                if (new_file(false) < 0) {
                    DEBUGF("No more files to play\n");
                    filling = false;
                } else {
                    /* Make it read more data */
                    filling = true;
                    queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                    /* Tell the file loading code that we want to start playing
                       as soon as we have some data */
                    play_pending = true;

                    current_track_counter++;
                }
                break; 

            case MPEG_SWAP_DATA:
                free_space_left = mp3buf_write - mp3buf_swapwrite;

                if(free_space_left == 0)
                    break;
                
                if(free_space_left < 0)
                    free_space_left = mp3buflen + free_space_left;

                amount_to_swap = MIN(MPEG_SWAP_CHUNKSIZE, free_space_left);
                if(mp3buf_write < mp3buf_swapwrite)
                    amount_to_swap = MIN(mp3buflen - mp3buf_swapwrite,
                                         amount_to_swap);
                else
                    amount_to_swap = MIN(mp3buf_write - mp3buf_swapwrite,
                                         amount_to_swap);
                
                DEBUGF("B %x\n", amount_to_swap);
                bitswap((unsigned short *)(mp3buf + mp3buf_swapwrite),
                        (amount_to_swap+1)/2);

                mp3buf_swapwrite += amount_to_swap;
                if(mp3buf_swapwrite >= mp3buflen)
                {
                    mp3buf_swapwrite = 0;
                    DEBUGF("BW\n");
                }

                /* Tell ourselves that we must swap more data */
                queue_post(&mpeg_queue, MPEG_SWAP_DATA, 0);

                /* And while we're at it, see if we have started
                   playing yet. If not, do it. */
                if(play_pending)
                {
                    /* If the filling has stopped, and we still haven't reached
                       the watermark, the file must be smaller than the
                       watermark. We must still play it. */
                    if(((mp3buf_swapwrite - mp3buf_read) >= MPEG_LOW_WATER) ||
                       !filling)
                    {
                        DEBUGF("P\n");
                        play_pending = false;
                        playing = true;
			
                        last_dma_tick = current_tick;
                        init_dma();
                        start_dma();

                        /* Tell ourselves that we need more data */
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                    }
                }
                break;

            case MPEG_NEED_DATA:
                free_space_left = mp3buf_read - mp3buf_write;

                /* We interpret 0 as "empty buffer" */
                if(free_space_left <= 0)
                    free_space_left = mp3buflen + free_space_left;

                unplayed_space_left = mp3buflen - free_space_left;

                /* Make sure that we don't fill the entire buffer */
                free_space_left -= 2;

                /* do we have any more buffer space to fill? */
                if(free_space_left <= MPEG_HIGH_WATER)
                {
                    DEBUGF("0\n");
                    filling = false;
                    ata_sleep();
                    break;
                }

                if(play_pending)
                {
                    amount_to_read = MIN(MPEG_LOW_WATER, free_space_left);
                }
                else
                {
                    if(unplayed_space_left < MPEG_LOW_WATER)
                        amount_to_read = MIN(MPEG_LOW_WATER_CHUNKSIZE,
                                             free_space_left);
                    else
                        amount_to_read = MIN(MPEG_CHUNKSIZE, free_space_left);
                }
                amount_to_read = MIN(mp3buflen - mp3buf_write, amount_to_read);
                
                /* Read in a few seconds worth of MP3 data. We don't want to
                   read too large chunks because the bitswapping will take
                   too much time. We must keep the DMA happy and also give
                   the other threads a chance to run. */
                if(mpeg_file >= 0)
                {
                    DEBUGF("R\n");
                    len = read(mpeg_file, mp3buf+mp3buf_write, amount_to_read);
                    if(len > 0)
                    {
                        DEBUGF("R: %x\n", len);
                        /* Tell ourselves that we need to swap some data */
                        queue_post(&mpeg_queue, MPEG_SWAP_DATA, 0);

                        /* Make sure that the write pointer is at a word
                           boundary when we reach the end of the file */
                        if(len < amount_to_read)
                            len = (len + 1) & 0xfffffffe;
                        
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
                    
                        /* Make sure that the write pointer is at a word
                           boundary */
                        mp3buf_write = (mp3buf_write + 1) & 0xfffffffe;

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
                DEBUGF("Track change\n");

#ifdef ARCHOS_RECORDER
                /* Reset the AVC */
                mpeg_sound_set(SOUND_AVC, -1);
#endif
                remove_current_tag();

                current_track_counter++;
                break;
                
            case SYS_USB_CONNECTED:
                stop_playing();
                
                /* Tell the USB thread that we are safe */
                DEBUGF("mpeg_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&mpeg_queue);
                break;
        }
    }
}

static void setup_sci0(void)
{
    /* PB15 is I/O, PB14 is IRQ6, PB12 is SCK0 */
    PBCR1 = (PBCR1 & 0x0cff) | 0x1200;
    
    /* Set PB12 to output */
    PBIOR |= 0x1000;

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

    /* set IRQ6 and IRQ7 to edge detect */
    ICR |= 0x03;

    /* set PB15 and PB14 to inputs */
    PBIOR &= 0x7fff;
    PBIOR &= 0xbfff;

    /* set IRQ6 prio 8 and IRQ7 prio 0 */
    IPRB = ( IPRB & 0xff00 ) | 0x0080;

    /* Enable End of DMA interrupt at prio 8 */
    IPRC = (IPRC & 0xf0ff) | 0x0800;
    
    /* Enable Tx (only!) */
    SCR0 |= 0x20;
}
#endif /* SIMULATOR */

#ifdef SIMULATOR
static struct mp3entry taginfo;
#endif

struct mp3entry* mpeg_current_track(void)
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

void mpeg_play(char* trackname)
{
#ifdef SIMULATOR
    mp3info(&taginfo, trackname);
#else
    queue_post(&mpeg_queue, MPEG_PLAY, trackname);
#endif
}

void mpeg_stop(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_STOP, NULL);
#endif
}

void mpeg_pause(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_PAUSE, NULL);
#endif
}

void mpeg_resume(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_RESUME, NULL);
#endif
}

void mpeg_next(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_NEXT, NULL);
#endif
}

void mpeg_prev(void)
{
#ifndef SIMULATOR
    queue_post(&mpeg_queue, MPEG_PREV, NULL);
#endif
}

bool mpeg_is_playing(void)
{
#ifndef SIMULATOR
    return playing || play_pending;
#else
    return false;
#endif
}

#ifndef SIMULATOR
#ifndef ARCHOS_RECORDER
int current_volume=0;  /* all values in tenth of dB */
int current_treble=0;
int current_bass=0;
int current_balance=0;

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
    r = l = current_volume + prescale;

    /* balance */
    if (current_balance >= 0)
        l -= current_balance;
    else
        r += current_balance;
    
    dac_volume(tenthdb2reg(l), tenthdb2reg(r), false);
}
#endif /* ARCHOS_RECORDER */
#endif /* SIMULATOR */

void mpeg_sound_set(int setting, int value)
{
#ifdef SIMULATOR
    setting = value;
#else
    int tmp;
    
    switch(setting)
    {
        case SOUND_VOLUME:
            value *= 2; /* Convert to percent */
            
#ifdef ARCHOS_RECORDER
            tmp = 0x7f00 * value / 100;
            mas_codec_writereg(0x10, tmp & 0xff00);
#else
            tmp = 0x38 * value / 100;

            /* store volume in tenth of dB */
            current_volume = ( tmp < 0x08 ? tmp*30 - 780 : tmp*15 - 660 );

            set_prescaled_volume();
#endif
            break;

        case SOUND_BASS:
#ifdef ARCHOS_RECORDER
            tmp = (((value-12) * 8) & 0xff) << 8;
            mas_codec_writereg(0x14, tmp & 0xff00);
#else    
            mas_writereg(MAS_REG_KBASS, bass_table[value]);
            current_bass = (value-15) * 10;
            set_prescaled_volume();
#endif
            break;

        case SOUND_TREBLE:
#ifdef ARCHOS_RECORDER
            tmp = (((value-12) * 8) & 0xff) << 8;
            mas_codec_writereg(0x15, tmp & 0xff00);
#else    
            mas_writereg(MAS_REG_KTREBLE, treble_table[value]);
            current_treble = (value-15) * 10;
            set_prescaled_volume();
#endif
            break;
            
#ifdef ARCHOS_RECORDER
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
    }
#endif /* SIMULATOR */
}

int mpeg_val2phys(int setting, int value)
{
    int result = 0;
    
    switch(setting)
    {
        case SOUND_VOLUME:
            result = value * 2;
            break;
        
        case SOUND_BASS:
#ifdef ARCHOS_RECORDER
            result = value - 12;
#else
            result = value - 15;
#endif
            break;
        
        case SOUND_TREBLE:
#ifdef ARCHOS_RECORDER
            result = value - 12;
#else
            result = value - 15;
#endif
            break;

#ifdef ARCHOS_RECORDER            
        case SOUND_LOUDNESS:
            result = value;
            break;
            
        case SOUND_SUPERBASS:
            result = value * 10;
            break;
#endif
    }
    return result;
}

static unsigned long mas_version_code;

void mpeg_init(int volume, int bass, int treble, int loudness, int bass_boost, int avc)
{
#ifdef SIMULATOR
    volume = bass = treble = loudness = bass_boost = avc;
#else
    unsigned long val;
#ifdef ARCHOS_RECORDER
    int rc;
#else
    loudness = bass_boost = avc;
#endif

    setup_sci0();

#ifdef ARCHOS_RECORDER
    mas_reset();
    
    /* Enable the audio CODEC and the DSP core, max analog voltage range */
    rc = mas_direct_config_write(MAS_CONTROL, 0x8c00);
    if(rc < 0)
        panicf("mas_ctrl_w: %d", rc);

    rc = mas_direct_config_read(MAS_CONTROL);
    if(rc < 0)
        panicf("mas_ctrl_r: %d", rc);

    /* Max volume on both ears */
    val = 0x80000;
    mas_writemem(MAS_BANK_D0,0x7fc,&val,1);
    mas_writemem(MAS_BANK_D0,0x7ff,&val,1);

    /* Enable the D/A Converter */
    mas_codec_writereg(0x0, 0x0001);

    /* DSP scale 100% */
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
#endif
    
#ifndef ARCHOS_RECORDER
    mas_writereg(0x3b, 0x20); /* Don't ask why. The data sheet doesn't say */
    mas_run(1);
    sleep(HZ);

    mas_readmem(MAS_BANK_D1, 0xff7, &mas_version_code, 1);

    /* Clear the upper 12 bits of the 32-bit samples */
    mas_writereg(0xc5, 0);
    mas_writereg(0xc6, 0);
    
    /* We need to set the PLL for a 14.1318MHz crystal */
    if(mas_version_code == 0x0601) /* Version F10? */
    {
        val = 0x5d9e8;
        mas_writemem(MAS_BANK_D0, 0x32d, &val, 1);
        val = 0xfffceb8d;
        mas_writemem(MAS_BANK_D0, 0x32e, &val, 1);
        val = 0x0;
        mas_writemem(MAS_BANK_D0, 0x32f, &val, 1);
        mas_run(0x475);
    }
    else
    {
        val = 0x5d9e8;
        mas_writemem(MAS_BANK_D0, 0x36d, &val, 1);
        val = 0xfffceb8d;
        mas_writemem(MAS_BANK_D0, 0x36e, &val, 1);
        val = 0x0;
        mas_writemem(MAS_BANK_D0, 0x36f, &val, 1);
        mas_run(0xfcb);
    }
#endif
    
    mp3buflen = mp3end - mp3buf;

    create_fliptable();
    
    queue_init(&mpeg_queue);
    create_thread(mpeg_thread, mpeg_stack,
                  sizeof(mpeg_stack), mpeg_thread_name);
    mas_poll_start(2);

#ifndef ARCHOS_RECORDER
    mas_writereg(MAS_REG_KPRESCALE, 0xe9400);
    dac_config(0x04); /* DAC on, all else off */

    val = 0x80000;
    mas_writemem(MAS_BANK_D1, 0x7f8, &val, 1);
    mas_writemem(MAS_BANK_D1, 0x7fb, &val, 1);
#endif
    
    mpeg_sound_set(SOUND_BASS, bass);
    mpeg_sound_set(SOUND_TREBLE, treble);
    mpeg_sound_set(SOUND_VOLUME, volume);
#ifdef ARCHOS_RECORDER
    mpeg_sound_set(SOUND_LOUDNESS, loudness);
    mpeg_sound_set(SOUND_SUPERBASS, bass_boost);
    mpeg_sound_set(SOUND_AVC, avc);
#endif
#endif /* !SIMULATOR */

    memset(id3tags, sizeof(id3tags), 0);
}
