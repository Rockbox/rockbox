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

#ifndef SIMULATOR
static bool mpeg_is_initialized = false;
#endif

#ifndef SIMULATOR

unsigned long mas_version_code;

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

bool dma_on;  /* The DMA is active */

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
#endif

/* the registered callback function ta ask for more mp3 data */
static void (*callback_for_more)(unsigned char**, int*);

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
void IRQ6(void) /* PB14: MAS stop demand IRQ */
{
    mp3_play_pause(false);
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

#ifdef HAVE_MAS3587F
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

    /* set IRQ6 to edge detect */
    ICR |= 0x02;

    /* set IRQ6 prio 8 */
    IPRB = ( IPRB & 0xff0f ) | 0x0080;

    DEBUGF("MAS Decoding application started\n");
}
#endif /* #ifdef HAVE_MAS3587F */

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

void mp3_init(int volume, int bass, int treble, int balance, int loudness, 
    int bass_boost, int avc, int channel_config)
{
#ifdef SIMULATOR
    volume = bass = treble = balance = loudness
        = bass_boost = avc = channel_config;
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
}


/* new functions, to be exported to plugin API */

void mp3_play_init(void)
{
#ifdef HAVE_MAS3587F
    init_playback();
#endif
    callback_for_more = NULL;
}

void mp3_play_data(unsigned char* start, int size,
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

    CHCR3 |= 0x0001; /* Enable DMA IRQ */
}

void mp3_play_pause(bool play)
{
    if (play)
        SCR0 |= 0x80;
    else
        SCR0 &= 0x7f;
}

void mp3_play_stop(void)
{
    mp3_play_pause(false);
    CHCR3 &= ~0x0001; /* Disable the DMA interrupt */
}
