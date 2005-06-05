/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "sound.h"
#ifndef SIMULATOR
#include "i2c.h"
#include "mas.h"
#include "dac.h"
#include "system.h"
#include "hwcompat.h"
#if CONFIG_HWCODEC == MASNONE
#include "pcm_playback.h"
#endif
#endif

#ifndef SIMULATOR
extern bool audio_is_initialized;
#endif

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
extern unsigned long shadow_io_control_main;
extern unsigned shadow_codec_reg0;
#endif

static const char* const units[] =
{
    "%",    /* Volume */
    "dB",   /* Bass */
    "dB",   /* Treble */
    "%",    /* Balance */
    "dB",   /* Loudness */
    "",     /* AVC */
    "",     /* Channels */
    "%",    /* Stereo width */
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
    0,    /* Stereo width */
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
    1,    /* Stereo width */
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
    0,    /* Stereo width */
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
    5,    /* Channels */
    255,  /* Stereo width */
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
    100,  /* Stereo width */
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

const char *sound_unit(int setting)
{
    return units[setting];
}

int sound_numdecimals(int setting)
{
    return numdecimals[setting];
}

int sound_steps(int setting)
{
    return steps[setting];
}

int sound_min(int setting)
{
    return minval[setting];
}

int sound_max(int setting)
{
    return maxval[setting];
}

int sound_default(int setting)
{
    return defaultval[setting];
}

#ifndef SIMULATOR
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

/* all values in tenth of dB */
int current_volume = 0;   /* -780..+180 */
int current_balance = 0;  /* -960..+960 */
int current_treble = 0;   /* -150..+150 */
int current_bass = 0;     /* -150..+150 */

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

    /* gain up the analog volume to compensate the prescale reduction gain,
     * but limit to +18 dB (the maximum the DAC can do */
    if (current_volume + prescale > 180)
        prescale = 180 - current_volume;
    l = r = current_volume + prescale;

    if (current_balance > 0)
    {
        l -= current_balance;
        if (l < -780)
            l = -780;
    }
    if (current_balance < 0)
    {
        r += current_balance;
        if (r < -780)
            r = -780;
    }

    dac_volume(tenthdb2reg(l), tenthdb2reg(r), false);
}
#endif /* MAS3507D */
#endif /* !SIMULATOR */

int channel_configuration = SOUND_CHAN_STEREO;
int stereo_width = 100;

#ifndef SIMULATOR
static void set_channel_config(void)
{
    /* default values: stereo */
    unsigned long val_ll = 0x80000;
    unsigned long val_lr = 0;
    unsigned long val_rl = 0;
    unsigned long val_rr = 0x80000;
    
    switch(channel_configuration)
    {
        /* case SOUND_CHAN_STEREO unnecessary */

        case SOUND_CHAN_MONO:
            val_ll = 0xc0000;
            val_lr = 0xc0000;
            val_rl = 0xc0000;
            val_rr = 0xc0000;
            break;

        case SOUND_CHAN_CUSTOM:
            {
                /* fixed point variables (matching MAS internal format)
                   integer part: upper 13 bits (inlcuding sign)
                   fractional part: lower 19 bits */
                long fp_width, fp_straight, fp_cross;
                
                fp_width = (stereo_width << 19) / 100;
                if (stereo_width <= 100)
                {
                    fp_straight = - ((1<<19) + fp_width) / 2;
                    fp_cross = fp_straight + fp_width;
                }
                else
                {
                    fp_straight = - (1<<19);
                    fp_cross = ((2 * fp_width / (((1<<19) + fp_width) >> 10))
                                << 9) - (1<<19);
                }
                val_ll = val_rr = fp_straight & 0xFFFFF;
                val_lr = val_rl = fp_cross & 0xFFFFF;
            }
            break;

        case SOUND_CHAN_MONO_LEFT:
            val_ll = 0x80000;
            val_lr = 0x80000;
            val_rl = 0;
            val_rr = 0;
            break;

        case SOUND_CHAN_MONO_RIGHT:
            val_ll = 0;
            val_lr = 0;
            val_rl = 0x80000;
            val_rr = 0x80000;
            break;

        case SOUND_CHAN_KARAOKE:
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
#elif CONFIG_HWCODEC == MAS3507D
    mas_writemem(MAS_BANK_D1, 0x7f8, &val_ll, 1); /* LL */
    mas_writemem(MAS_BANK_D1, 0x7f9, &val_lr, 1); /* LR */
    mas_writemem(MAS_BANK_D1, 0x7fa, &val_rl, 1); /* RL */
    mas_writemem(MAS_BANK_D1, 0x7fb, &val_rr, 1); /* RR */
#endif
}
#endif /* !SIMULATOR */

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
unsigned long mdb_shape_shadow = 0;
unsigned long loudness_shadow = 0;
#endif

void sound_set(int setting, int value)
{
#ifdef SIMULATOR
    setting = value;
#else
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    int tmp;
#endif

    if(!audio_is_initialized)
        return;
    
    switch(setting)
    {
        case SOUND_VOLUME:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
            tmp = 0x7f00 * value / 100;
            mas_codec_writereg(0x10, tmp & 0xff00);
#elif CONFIG_HWCODEC == MAS3507D
            current_volume = -780 + (value * 960 / 100); /* tenth of dB */
            set_prescaled_volume();
#elif CONFIG_HWCODEC == MASNONE
	    pcm_set_volume((value*167117) >> 16);
#endif
            break;

        case SOUND_BALANCE:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
            tmp = ((value * 127 / 100) & 0xff) << 8;
            mas_codec_writereg(0x11, tmp & 0xff00);
#elif CONFIG_HWCODEC == MAS3507D
            current_balance = value * 960 / 100; /* tenth of dB */
            set_prescaled_volume();
#endif
            break;

        case SOUND_BASS:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
            tmp = ((value * 8) & 0xff) << 8;
            mas_codec_writereg(0x14, tmp & 0xff00);
#elif CONFIG_HWCODEC == MAS3507D
            mas_writereg(MAS_REG_KBASS, bass_table[value+15]);
            current_bass = value * 10;
            set_prescaled_volume();
#endif
            break;

        case SOUND_TREBLE:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
            tmp = ((value * 8) & 0xff) << 8;
            mas_codec_writereg(0x15, tmp & 0xff00);
#elif CONFIG_HWCODEC == MAS3507D
            mas_writereg(MAS_REG_KTREBLE, treble_table[value+15]);
            current_treble = value * 10;
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
            channel_configuration = value;
            set_channel_config();
            break;
        
        case SOUND_STEREO_WIDTH:
            stereo_width = value;
            if (channel_configuration == SOUND_CHAN_CUSTOM)
                set_channel_config();
            break;
    }
#endif /* SIMULATOR */
}

int sound_val2phys(int setting, int value)
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

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
/* This function works by telling the decoder that we have another
   crystal frequency than we actually have. It will adjust its internal
   parameters and the result is that the audio is played at another pitch.

   The pitch value is in tenths of percent.
*/
static int last_pitch = 1000;

void sound_set_pitch(int pitch)
{
    unsigned long val;

    if (pitch != last_pitch)
    {
        /* Calculate the new (bogus) frequency */
        val = 18432 * 1000 / pitch;
    
        mas_writemem(MAS_BANK_D0, MAS_D0_OFREQ_CONTROL, &val, 1);

        /* We must tell the MAS that the frequency has changed.
         * This will unfortunately cause a short silence. */
        mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);
        
        last_pitch = pitch;
    }
}

int sound_get_pitch(void)
{
    return last_pitch;
}
#elif defined SIMULATOR
void sound_set_pitch(int pitch)
{
    (void)pitch;
}

int sound_get_pitch(void)
{
    return 1000;
}
#endif

#if CONFIG_HWCODEC == MASNONE
bool mp3_is_playing(void)
{
    /* a dummy */
    return false;
}
#endif
