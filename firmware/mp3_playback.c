/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Code that has been in mpeg.c before, now creating an encapsulated play
 * data module, to be used by other sources than file playback as well.
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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
#include <kernel.h>
#include "mpeg.h" /* ToDo: remove crosslinks */
#include "mp3_playback.h"
#ifndef SIMULATOR
#include "i2c.h"
#include "mas.h"
#include "dac.h"
#include "system.h"
#include "hwcompat.h"
#endif

/* hacking into mpeg.c, recording is still there */
#if CONFIG_HWCODEC == MAS3587F
enum
{
    MPEG_DECODER,
    MPEG_ENCODER
} mpeg_mode;
#endif /* #ifdef MAS3587F */

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
extern unsigned long shadow_io_control_main;
#endif

/**** globals ****/

/* own version, independent of mpeg.c */
static bool paused; /* playback is paused */
static bool playing; /* We are playing an MP3 stream */

#ifndef SIMULATOR
/* for measuring the play time */
static long playstart_tick;
static long cumulative_ticks;

/* the registered callback function to ask for more mp3 data */
static void (*callback_for_more)(unsigned char**, int*);
#endif /* #ifndef SIMULATOR */

static const char* const units[] =
{
    "%",    /* Volume */
    "dB",   /* Bass */
    "dB",   /* Treble */
    "%",    /* Balance */
    "dB",   /* Loudness */
    "",     /* AVC */
    "",     /* Channels */
    "dB",   /* Left gain */
    "dB",   /* Right gain */
    "dB",   /* Mic gain */
    "dB",   /* MDB Strength */
    "%",    /* MDB Harmonics */
    "Hz",   /* MDB Center */
    "Hz",   /* MDB Shape */
    "",     /* MDB Enable */
    "",     /* Super bass */
};

static const int numdecimals[] =
{
    0,    /* Volume */
    0,    /* Bass */
    0,    /* Treble */
    0,    /* Balance */
    0,    /* Loudness */
    0,    /* AVC */
    0,    /* Channels */
    1,    /* Left gain */
    1,    /* Right gain */
    1,    /* Mic gain */
    0,    /* MDB Strength */
    0,    /* MDB Harmonics */
    0,    /* MDB Center */
    0,    /* MDB Shape */
    0,    /* MDB Enable */
    0,    /* Super bass */
};

static const int steps[] =
{
    1,    /* Volume */
    1,    /* Bass */
    1,    /* Treble */
    1,    /* Balance */
    1,    /* Loudness */
    1,    /* AVC */
    1,    /* Channels */
    1,    /* Left gain */
    1,    /* Right gain */
    1,    /* Mic gain */
    1,    /* MDB Strength */
    1,    /* MDB Harmonics */
    10,   /* MDB Center */
    10,   /* MDB Shape */
    1,    /* MDB Enable */
    1,    /* Super bass */
};

static const int minval[] =
{
    0,    /* Volume */
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    -12,  /* Bass */
    -12,  /* Treble */
#else
    -15,  /* Bass */
    -15,  /* Treble */
#endif
    -100,  /* Balance */
    0,    /* Loudness */
    -1,   /* AVC */
    0,    /* Channels */
    0,    /* Left gain */
    0,    /* Right gain */
    0,    /* Mic gain */
    0,    /* MDB Strength */
    0,    /* MDB Harmonics */
    20,   /* MDB Center */
    50,   /* MDB Shape */
    0,    /* MDB Enable */
    0,    /* Super bass */
};

static const int maxval[] =
{
    100,  /* Volume */
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    12,   /* Bass */
    12,   /* Treble */
#else
    15,   /* Bass */
    15,   /* Treble */
#endif
    100,   /* Balance */
    17,   /* Loudness */
    4,    /* AVC */
    6,    /* Channels */
    15,   /* Left gain */
    15,   /* Right gain */
    15,   /* Mic gain */
    127,  /* MDB Strength */
    100,  /* MDB Harmonics */
    300,  /* MDB Center */
    300,  /* MDB Shape */
    1,    /* MDB Enable */
    1,    /* Super bass */
};

static const int defaultval[] =
{
    70,   /* Volume */
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    6,    /* Bass */
    6,    /* Treble */
#else
    7,    /* Bass */
    7,    /* Treble */
#endif
    0,    /* Balance */
    0,    /* Loudness */
    0,    /* AVC */
    0,    /* Channels */
    8,    /* Left gain */
    8,    /* Right gain */
    2,    /* Mic gain */
    50,   /* MDB Strength */
    48,   /* MDB Harmonics */
    60,   /* MDB Center */
    90,   /* MDB Shape */
    0,    /* MDB Enable */
    0,    /* Super bass */
};

const char *mpeg_sound_unit(int setting)
{
    return units[setting];
}

int mpeg_sound_numdecimals(int setting)
{
    return numdecimals[setting];
}

int mpeg_sound_steps(int setting)
{
    return steps[setting];
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

#ifndef SIMULATOR
static bool mpeg_is_initialized = false;
#endif

#ifndef SIMULATOR

unsigned long mas_version_code;

#if CONFIG_HWCODEC == MAS3507D

static const unsigned int bass_table[] =
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

static const unsigned int treble_table[] =
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

static const unsigned int prescale_table[] =
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

bool dma_on;  /* The DMA is active */

#if CONFIG_HWCODEC == MAS3507D
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

    count = FREQ / 2000 / 8;

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


#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
void demand_irq_enable(bool on)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    
    if(on)
    {
        IPRA = (IPRA & 0xfff0) | 0x000b;
        ICR &= ~0x0010; /* IRQ3 level sensitive */
    }
    else
        IPRA &= 0xfff0;

    set_irq_level(oldlevel);
}
#endif /* #if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F) */


void play_tick(void)
{
    if(playing && !paused)
    {
        /* Start DMA if it is disabled and the DEMAND pin is high */
        if(!(SCR0 & 0x80) && (PBDR & 0x4000))
        {
            SCR0 |= 0x80;
        }

        playback_tick(); /* dirty call to mpeg.c */
    }
}

#pragma interrupt
void DEI3(void)
{
    unsigned char* start;
    int size = 0;

    if (callback_for_more != NULL)
    {
        callback_for_more(&start, &size);
    }
    
    if (size > 0)
    {
        DTCR3 = size & 0xffff;
        SAR3 = (unsigned int) start;
    }
    else
    {
        CHCR3 &= ~0x0001; /* Disable the DMA interrupt */
    }

    CHCR3 &= ~0x0002; /* Clear DMA interrupt */
}

#pragma interrupt
void IMIA1(void) /* Timer 1 interrupt */
{
    if(playing)
        play_tick();
    TSR1 &= ~0x01;
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    /* Disable interrupt */
    IPRC &= ~0x000f;
#endif
}

#pragma interrupt
void IRQ6(void) /* PB14: MAS stop demand IRQ */
{
    SCR0 &= ~0x80;
}

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
#pragma interrupt
void IRQ3(void) /* PA15: MAS demand IRQ */
{
    /* Begin with setting the IRQ to edge sensitive */
    ICR |= 0x0010;

#if CONFIG_HWCODEC == MAS3587F
    if(mpeg_mode == MPEG_ENCODER)
        rec_tick();
    else
#endif
        postpone_dma_tick();
}
#endif /* #if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F) */

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

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
static void init_playback(void)
{
    unsigned long val;
    int rc;

    mp3_play_pause(false);
    
    mas_reset();
    
    /* Enable the audio CODEC and the DSP core, max analog voltage range */
    rc = mas_direct_config_write(MAS_CONTROL, 0x8c00);
    if(rc < 0)
        panicf("mas_ctrl_w: %d", rc);

    /* Stop the current application */
    val = 0;
    mas_writemem(MAS_BANK_D0, MAS_D0_APP_SELECT, &val, 1);
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_APP_RUNNING, &val, 1);
    } while(val);
    
    /* Enable the D/A Converter */
    mas_codec_writereg(0x0, 0x0001);

    /* ADC scale 0%, DSP scale 100% */
    mas_codec_writereg(6, 0x0000);
    mas_codec_writereg(7, 0x4000);

    /* Disable SDO and SDI */
    val = 0x0d;
    mas_writemem(MAS_BANK_D0, MAS_D0_INTERFACE_CONTROL, &val, 1);

    /* Set Demand mode and validate all settings */
    shadow_io_control_main = 0x25;
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);

    /* Start the Layer2/3 decoder applications */
    val = 0x0c;
    mas_writemem(MAS_BANK_D0, MAS_D0_APP_SELECT, &val, 1);
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_APP_RUNNING, &val, 1);
    } while((val & 0x0c) != 0x0c);

    mpeg_sound_channel_config(MPEG_SOUND_STEREO);

#if CONFIG_HWCODEC == MAS3587F
    mpeg_mode = MPEG_DECODER;
#endif

    /* set IRQ6 to edge detect */
    ICR |= 0x02;

    /* set IRQ6 prio 8 */
    IPRB = ( IPRB & 0xff0f ) | 0x0080;

    DEBUGF("MAS Decoding application started\n");
}
#endif /* #if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F) */

#ifndef SIMULATOR
#if CONFIG_HWCODEC == MAS3507D
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
#endif /* MAS3507D */
#endif /* !SIMULATOR */

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
unsigned long mdb_shape_shadow = 0;
unsigned long loudness_shadow = 0;
#endif

void mpeg_sound_set(int setting, int value)
{
#ifdef SIMULATOR
    setting = value;
#else
#if CONFIG_HWCODEC == MAS3507D
    int l, r;
#else
    int tmp;
#endif

    if(!mpeg_is_initialized)
        return;
    
    switch(setting)
    {
        case SOUND_VOLUME:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
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
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
            tmp = ((value * 127 / 100) & 0xff) << 8;
            mas_codec_writereg(0x11, tmp & 0xff00);
#else
            current_balance = value;
#endif
            break;

        case SOUND_BASS:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
            tmp = ((value * 8) & 0xff) << 8;
            mas_codec_writereg(0x14, tmp & 0xff00);
#else    
            mas_writereg(MAS_REG_KBASS, bass_table[value+15]);
            current_bass = (value) * 10;
            set_prescaled_volume();
#endif
            break;

        case SOUND_TREBLE:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
            tmp = ((value * 8) & 0xff) << 8;
            mas_codec_writereg(0x15, tmp & 0xff00);
#else    
            mas_writereg(MAS_REG_KTREBLE, treble_table[value+15]);
            current_treble = (value) * 10;
            set_prescaled_volume();
#endif
            break;
            
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
        case SOUND_LOUDNESS:
            loudness_shadow = (loudness_shadow & 0x04) |
                (MAX(MIN(value * 4, 0x44), 0) << 8);
            mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
            break;
            
        case SOUND_AVC:
            switch (value) {
                case 1: /* 20ms */
                    tmp = (0x1 << 8) | (0x8 << 12);
                    break;
                case 2: /* 2s */
                    tmp = (0x2 << 8) | (0x8 << 12);
                    break;
                case 3: /* 4s */
                    tmp = (0x4 << 8) | (0x8 << 12);
                    break;
                case 4: /* 8s */
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

        case SOUND_MDB_STRENGTH:
            mas_codec_writereg(MAS_REG_KMDB_STR, (value & 0x7f) << 8);
            break;
          
        case SOUND_MDB_HARMONICS:
            tmp = value * 127 / 100;
            mas_codec_writereg(MAS_REG_KMDB_HAR, (tmp & 0x7f) << 8);
            break;
          
        case SOUND_MDB_CENTER:
            mas_codec_writereg(MAS_REG_KMDB_FC, (value/10) << 8);
            break;
          
        case SOUND_MDB_SHAPE:
            mdb_shape_shadow = (mdb_shape_shadow & 0x02) | ((value/10) << 8);
            mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
            break;
          
        case SOUND_MDB_ENABLE:
            mdb_shape_shadow = (mdb_shape_shadow & ~0x02) | (value?2:0);
            mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
            break;
          
        case SOUND_SUPERBASS:
            loudness_shadow = (loudness_shadow & ~0x04) |
                (value?4:0);
            mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
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
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    int result = 0;
    
    switch(setting)
    {
        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
            result = (value - 2) * 15;
            break;

        case SOUND_MIC_GAIN:
            result = value * 15 + 210;
            break;

       default:
            result = value;
            break;
    }
    return result;
#else
    (void)setting;
    return value;
#endif
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

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_LL, &val_ll, 1); /* LL */
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_LR, &val_lr, 1); /* LR */
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_RL, &val_rl, 1); /* RL */
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_RR, &val_rr, 1); /* RR */
#else
    mas_writemem(MAS_BANK_D1, 0x7f8, &val_ll, 1); /* LL */
    mas_writemem(MAS_BANK_D1, 0x7f9, &val_lr, 1); /* LR */
    mas_writemem(MAS_BANK_D1, 0x7fa, &val_rl, 1); /* RL */
    mas_writemem(MAS_BANK_D1, 0x7fb, &val_rr, 1); /* RR */
#endif
#endif
}

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
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
    
    mas_writemem(MAS_BANK_D0, MAS_D0_OFREQ_CONTROL, &val, 1);

    /* We must tell the MAS that the frequency has changed.
       This will unfortunately cause a short silence. */
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);
}
#endif

void mp3_init(int volume, int bass, int treble, int balance, int loudness, 
              int avc, int channel_config,
              int mdb_strength, int mdb_harmonics,
              int mdb_center, int mdb_shape, bool mdb_enable,
              bool superbass)
{
#ifdef SIMULATOR
    (void)volume;
    (void)bass;
    (void)treble;
    (void)balance;
    (void)loudness;
    (void)avc;
    (void)channel_config;
    (void)mdb_strength;
    (void)mdb_harmonics;
    (void)mdb_center;
    (void)mdb_shape;
    (void)mdb_enable;
    (void)superbass;
#else
#if CONFIG_HWCODEC == MAS3507D
    unsigned long val;
    (void)loudness;
    (void)avc;
    (void)mdb_strength;
    (void)mdb_harmonics;
    (void)mdb_center;
    (void)mdb_shape;
    (void)mdb_enable;
    (void)superbass;
#endif

    setup_sci0();

#ifdef HAVE_MAS_SIBI_CONTROL
    and_b(~0x01, &PBDRH); /* drive SIBI low */
    and_b(~0x01, &PBIORH); /* output for PB8 */
#endif

#if CONFIG_HWCODEC == MAS3587F
    or_b(0x08, &PAIORH); /* output for /PR */
    init_playback();
    
    mas_version_code = mas_readver();
    DEBUGF("MAS3587 derivate %d, version %c%d\n",
           (mas_version_code & 0xf000) >> 12,
           'A' + ((mas_version_code & 0x0f00) >> 8), mas_version_code & 0xff);
#elif CONFIG_HW_CODEC == MAS3539F
    or_b(0x08, &PAIORH); /* output for /PR */
    init_playback();
    
    mas_version_code = mas_readver();
    DEBUGF("MAS3539 derivate %d, version %c%d\n",
           (mas_version_code & 0xf000) >> 12,
           'A' + ((mas_version_code & 0x0f00) >> 8), mas_version_code & 0xff);
#endif

#ifdef HAVE_DAC3550A
    dac_init();
#endif
    
#if CONFIG_HWCODEC == MAS3507D
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

#if CONFIG_HWCODEC == MAS3507D
    mas_poll_start(1);

    mas_writereg(MAS_REG_KPRESCALE, 0xe9400);
    dac_enable(true);

    mpeg_sound_channel_config(channel_config);
#endif

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    ICR &= ~0x0010; /* IRQ3 level sensitive */
    PACR1 = (PACR1 & 0x3fff) | 0x4000; /* PA15 is IRQ3 */
#endif

    /* Must be done before calling mpeg_sound_set() */
    mpeg_is_initialized = true;

    mpeg_sound_set(SOUND_BASS, bass);
    mpeg_sound_set(SOUND_TREBLE, treble);
    mpeg_sound_set(SOUND_BALANCE, balance);
    mpeg_sound_set(SOUND_VOLUME, volume);
    
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    mpeg_sound_channel_config(channel_config);
    mpeg_sound_set(SOUND_LOUDNESS, loudness);
    mpeg_sound_set(SOUND_AVC, avc);
    mpeg_sound_set(SOUND_MDB_STRENGTH, mdb_strength);
    mpeg_sound_set(SOUND_MDB_HARMONICS, mdb_harmonics);
    mpeg_sound_set(SOUND_MDB_CENTER, mdb_center);
    mpeg_sound_set(SOUND_MDB_SHAPE, mdb_shape);
    mpeg_sound_set(SOUND_MDB_ENABLE, mdb_enable);
    mpeg_sound_set(SOUND_SUPERBASS, superbass);
#endif
#endif /* !SIMULATOR */

    playing = false;
    paused = true;
}

void mp3_shutdown(void)
{
#ifndef SIMULATOR
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    unsigned long val = 1;
    mas_writemem(MAS_BANK_D0, MAS_D0_SOFT_MUTE, &val, 1); /* Mute */
#endif

#if CONFIG_HWCODEC == MAS3507D
    dac_volume(0, 0, false);
#endif

#endif
}

/* new functions, to be exported to plugin API */

#ifndef SIMULATOR

void mp3_play_init(void)
{
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    init_playback();
#endif
    playing = false;
    paused = true;
    callback_for_more = NULL;
    mp3_reset_playtime();
}

void mp3_play_data(const unsigned char* start, int size,
    void (*get_more)(unsigned char** start, int* size) /* callback fn */
)
{
    /* init DMA */
    DAR3 = 0x5FFFEC3;
    CHCR3 &= ~0x0002; /* Clear interrupt */
    CHCR3 = 0x1504; /* Single address destination, TXI0, IE=1 */
    DMAOR = 0x0001; /* Enable DMA */
    
    callback_for_more = get_more;

    SAR3 = (unsigned int)start;
    DTCR3 = size & 0xffff;

    playing = true;
    paused = true;

    CHCR3 |= 0x0001; /* Enable DMA IRQ */

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    demand_irq_enable(true);
#endif
}

void mp3_play_pause(bool play)
{
    if (paused && play)
    {   /* resume playback */
        SCR0 |= 0x80;
        paused = false;
        playstart_tick = current_tick;
    }
    else if (!paused && !play)
    {   /* stop playback */
        SCR0 &= 0x7f;
        paused = true;
        cumulative_ticks += current_tick - playstart_tick;
    }
}

void mp3_play_stop(void)
{
    playing = false;
    mp3_play_pause(false);
    CHCR3 &= ~0x0001; /* Disable the DMA interrupt */
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    demand_irq_enable(false);
#endif
}

long mp3_get_playtime(void)
{
    if (paused)
        return cumulative_ticks;
    else
        return cumulative_ticks + current_tick - playstart_tick;
}

void mp3_reset_playtime(void)
{
    cumulative_ticks = 0;
    playstart_tick = current_tick;
}


bool mp3_is_playing(void)
{
    return playing;
}


/* returns the next byte position which would be transferred */
unsigned char* mp3_get_pos(void)
{
    return (unsigned char*)SAR3;
}


#endif /* #ifndef SIMULATOR */
