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
#include "i2c.h"
#include "mas.h"
#include "dac.h"
#include "sh7034.h"
#include "system.h"
#include "debug.h"
#include "kernel.h"
#include "thread.h"
#include "panic.h"
#include "file.h"
#include "settings.h"
#include "id3.h"

#define MPEG_STACK_SIZE 0x2000
#define MPEG_CHUNKSIZE  0x20000
#define MPEG_LOW_WATER  0x30000

#define MPEG_PLAY         1
#define MPEG_STOP         2
#define MPEG_PAUSE        3
#define MPEG_RESUME       4
#define MPEG_NEED_DATA    100

extern char* peek_next_track(int type);

#ifndef ARCHOS_RECORDER
static unsigned int bass_table[] =
{
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
static int mpeg_stack[MPEG_STACK_SIZE/sizeof(int)];

/* defined in linker script */
extern unsigned char mp3buf[];
extern unsigned char mp3end[];

static int mp3buflen;
static int mp3buf_write;
static int mp3buf_read;

static int last_dma_chunk_size;

static bool dma_on;  /* The DMA is active */
static bool playing; /* We are playing an MP3 stream */
static bool filling; /* We are filling the buffer with data from disk */

static int mpeg_file;

/* list of tracks in memory */
#define MAX_ID3_TAGS 4
static struct {
    struct mp3entry id3;
    int mempos;
} id3tags[MAX_ID3_TAGS];
static int last_tag = 0;

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
    /* Start DMA if it isn't running */
    if(playing && !dma_on)
    {
        if(PBDR & 0x4000)
        {
            if(!(SCR0 & 0x80))
                start_dma();
        }
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

    if(playing)
    {
        mp3buf_read += last_dma_chunk_size;
        if(mp3buf_read >= mp3buflen)
            mp3buf_read = 0;
    
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
            last_dma_chunk_size = MIN(last_dma_chunk_size, space_until_end_of_buffer);
            DTCR3 = last_dma_chunk_size & 0xffff;
            SAR3 = (unsigned int)mp3buf + mp3buf_read;

            /* several tracks loaded? */
            if ( last_tag>1 ) {
                /* will we move across the track boundary? */
                if (( mp3buf_read <= id3tags[1].mempos ) &&
                    ((mp3buf_read+last_dma_chunk_size) > id3tags[1].mempos )) {
                    /* shift array so index 0 is current track */
                    int i;
                    for (i=0; i<last_tag-1; i++)
                        id3tags[i] = id3tags[i+1];
                    last_tag--;
                    DEBUGF("Track change\n");
                }
            }
        }
        else
        {
            DEBUGF("No more MP3 data. Stopping.\n");
            CHCR3 = 0; /* Stop DMA interrupt */
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

static int new_file(void)
{
    char *trackname;

    trackname = peek_next_track(0);
    if ( !trackname )
        return -1;

    DEBUGF("playing %s\n", trackname);

    /* grab id3 tag of new file and remember where in memory it starts */
    if ( last_tag < MAX_ID3_TAGS ) {
        mp3info(&(id3tags[last_tag].id3), trackname);
        id3tags[last_tag].mempos = mp3buf_write;
        last_tag++;
    }

    mpeg_file = open(trackname, O_RDONLY);
    if(mpeg_file < 0)
    {
        debugf("Couldn't open file\n");
        return -1;
    }
    return 0;
}

struct mp3entry* mpeg_current_track(void)
{
    return &(id3tags[0].id3);
}

static void mpeg_thread(void)
{
    struct event ev;
    int len;
    int free_space_left;
    int amount_to_read;
    bool play_pending;

    play_pending = false;
    playing = false;
    mpeg_file = -1;

    while(1)
    {
        DEBUGF("S\n");
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

                if(mpeg_file >= 0)
                    close(mpeg_file);

                mpeg_file = open((char*)ev.data, O_RDONLY);
                if(mpeg_file < 0)
                {
                    DEBUGF("Couldn't open %s\n",ev.data);
                    break;
                }
                
                /* grab id3 tag of new file and
                   remember where in memory it starts */
                mp3info(&(id3tags[0].id3), ev.data);
                id3tags[0].mempos = mp3buf_write;
                last_tag=1;

                /* Make it read more data */
                filling = true;
                queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                /* Tell the file loading code that we want to start playing
                   as soon as we have some data */
                play_pending = true;
                break;

            case MPEG_STOP:
                DEBUGF("MPEG_STOP\n");
                /* Stop the current stream */
                playing = false;
                filling = false;
                if(mpeg_file >= 0)
                    close(mpeg_file);
                mpeg_file = -1;
                stop_dma();
                break;

            case MPEG_PAUSE:
                DEBUGF("MPEG_PAUSE\n");
                /* Stop the current stream */
                playing = false;
                stop_dma();
                break;

            case MPEG_RESUME:
                DEBUGF("MPEG_RESUME\n");
                /* Stop the current stream */
                playing = true;
                start_dma();
                break;

            case MPEG_NEED_DATA:
                free_space_left = mp3buf_read - mp3buf_write;

                /* We interpret 0 as "empty buffer" */
                if(free_space_left <= 0)
                    free_space_left = mp3buflen + free_space_left;

                if(free_space_left <= MPEG_CHUNKSIZE)
                {
                    DEBUGF("0\n");
                    filling = false;
                    break;;
                }
            
                amount_to_read = MIN(MPEG_CHUNKSIZE, free_space_left);
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
                        DEBUGF("B\n");

                        bitswap((unsigned short *)(mp3buf + mp3buf_write),
                                (len+1)/2);
                    
                        mp3buf_write += len;
                        if(mp3buf_write >= mp3buflen)
                        {
                            mp3buf_write = 0;
                            DEBUGF("W\n");
                        }

                        /* Tell ourselves that we want more data */
                        queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);

                        /* And while we're at it, see if we have started
                           playing yet. If not, do it. */
                        if(play_pending)
                        {
                            play_pending = false;
                            playing = true;
                
                            init_dma();
                            start_dma();
                        }
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

                        if(new_file() < 0)
                        {
                            /* No more data to play */
                            DEBUGF("Finished playing\n");
                            filling = false;
                        }
                        else
                        {
                            /* Tell ourselves that we want more data */
                            queue_post(&mpeg_queue, MPEG_NEED_DATA, 0);
                        }
                    }
                    yield(); /* To be safe */
                }
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


void mpeg_play(char* trackname)
{
    queue_post(&mpeg_queue, MPEG_PLAY, trackname);
}

void mpeg_stop(void)
{
    queue_post(&mpeg_queue, MPEG_STOP, NULL);
}

void mpeg_pause(void)
{
    queue_post(&mpeg_queue, MPEG_PAUSE, NULL);
}

void mpeg_resume(void)
{
    queue_post(&mpeg_queue, MPEG_RESUME, NULL);
}

void mpeg_volume(int percent)
{
    int volume;
    
#ifdef ARCHOS_RECORDER
    volume = 0x7f00 * percent / 100;
    mas_codec_writereg(0x10, volume & 0xff00);
#else
    volume = 0x38 * percent / 100;
    dac_volume(volume);
#endif
}

void mpeg_bass(int percent)
{
    int bass;
    
#ifdef ARCHOS_RECORDER
    bass = 0x6000 * percent / 100;
    mas_codec_writereg(0x14, bass & 0xff00);
#else    
    bass = 15 * percent / 100;
    mas_writereg(MAS_REG_KBASS, bass_table[bass]);
#endif
}

void mpeg_treble(int percent)
{
    int treble;

#ifdef ARCHOS_RECORDER
    treble = 0x6000 * percent / 100;
    mas_codec_writereg(0x15, treble & 0xff00);
#else    
    treble = 15 * percent / 100;
    mas_writereg(MAS_REG_KTREBLE, treble_table[treble]);
#endif
}

void mpeg_init(void)
{
#ifdef ARCHOS_RECORDER
    unsigned long val;
#endif

    setup_sci0();
    i2c_init();

#ifdef ARCHOS_RECORDER
    /* Reset the MAS */
    PAIOR |= 0x100;
    PADR &= ~0x100;
    sleep(HZ/100);
    PADR |= 0x100;
    sleep(HZ/10);

    /* Enable the audio CODEC and the DSP core, max analog voltage range */
    mas_direct_config_write(MAS_CONTROL, 0x8c00);

    /* Max volume on both ears */
    val = 0x80000;
    mas_writemem(MAS_BANK_D0,0x7fc,&val,1);
    mas_writemem(MAS_BANK_D0,0x7ff,&val,1);

    /* Enable the D/A Converter */
    mas_codec_writereg(0x0, 0x0001);

    /* DSP scale 100% */
    mas_codec_writereg(7, 0x4000);

    /* Disable S/PDIF, SDO and SDI */
    val = 0x2d;
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
    sleep(HZ/10);
#endif
    
    mp3buflen = mp3end - mp3buf;

    create_fliptable();
    
    queue_init(&mpeg_queue);
    create_thread(mpeg_thread, mpeg_stack, sizeof(mpeg_stack));
    mas_poll_start(2);

#ifndef ARCHOS_RECORDER
    mas_writereg(MAS_REG_KPRESCALE, 0xe9400);
    dac_config(0x04); /* DAC on, all else off */
#endif
    
    mpeg_bass(DEFAULT_BASS_SETTING);
    mpeg_treble(DEFAULT_TREBLE_SETTING);
    mpeg_volume(DEFAULT_VOLUME_SETTING);

}
