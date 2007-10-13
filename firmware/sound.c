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
 * Copyright (C) 2007 by Christian Gmeiner
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
#include "logf.h"
#ifndef SIMULATOR
#include "i2c.h"
#include "mas.h"
#if CONFIG_CPU == PNX0101
#include "pnx0101.h"
#endif
#include "dac.h"
#include "system.h"
#if CONFIG_CODEC == SWCODEC
#include "pcm.h"
#endif
#endif

#ifndef SIMULATOR
extern bool audio_is_initialized;

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
extern unsigned long shadow_io_control_main;
extern unsigned shadow_codec_reg0;
#endif
#endif /* SIMULATOR */

#ifdef SIMULATOR
/* dummy for sim */
const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, VOLUME_MIN / 10, VOLUME_MAX / 10, -25},
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#if CONFIG_CODEC == MAS3587F || defined(HAVE_UDA1380) || defined(HAVE_TLV320)\
    || defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8731)
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,-128,  96,   0},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,-128,  96,   0},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,-128, 108,  16},
#endif
};
#endif

const char *sound_unit(int setting)
{
    return audiohw_settings[setting].unit;
}

int sound_numdecimals(int setting)
{
    return audiohw_settings[setting].numdecimals;
}

int sound_steps(int setting)
{
    return audiohw_settings[setting].steps;
}

int sound_min(int setting)
{
    return audiohw_settings[setting].minval;
}

int sound_max(int setting)
{
    return audiohw_settings[setting].maxval;
}

int sound_default(int setting)
{
    return audiohw_settings[setting].defaultval;
}

sound_set_type* sound_get_fn(int setting)
{
    sound_set_type* result = NULL;

    switch (setting) {
        case SOUND_VOLUME:
            result = sound_set_volume;
            break;

        case SOUND_BASS:
            result = sound_set_bass;
            break;

        case SOUND_TREBLE:
            result = sound_set_treble;
            break;

        case SOUND_BALANCE:
            result = sound_set_balance;
            break;

        case SOUND_CHANNELS:
            result = sound_set_channels;
            break;

        case SOUND_STEREO_WIDTH:
            result = sound_set_stereo_width;
            break;

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
        case SOUND_LOUDNESS:
            result = sound_set_loudness;
            break;

        case SOUND_AVC:
            result = sound_set_avc;
            break;

        case SOUND_MDB_STRENGTH:
            result = sound_set_mdb_strength;
            break;

        case SOUND_MDB_HARMONICS:
            result = sound_set_mdb_harmonics;
            break;

        case SOUND_MDB_CENTER:
            result = sound_set_mdb_center;
            break;

        case SOUND_MDB_SHAPE:
            result = sound_set_mdb_shape;
            break;

        case SOUND_MDB_ENABLE:
            result = sound_set_mdb_enable;
            break;

        case SOUND_SUPERBASS:
            result = sound_set_superbass;
            break;
#endif
    }

    return result;
}

#if CONFIG_CODEC == SWCODEC
/* Copied from dsp.h, nasty nasty, but we don't want to include dsp.h */
enum {
    DSP_CALLBACK_SET_PRESCALE = 0,
    DSP_CALLBACK_SET_BASS,
    DSP_CALLBACK_SET_TREBLE,
    DSP_CALLBACK_SET_CHANNEL_CONFIG,
    DSP_CALLBACK_SET_STEREO_WIDTH
};

static int (*dsp_callback)(int, intptr_t) = NULL;

void sound_set_dsp_callback(int (*func)(int, intptr_t))
{
    dsp_callback = func;
}
#endif

#ifndef SIMULATOR
#if CONFIG_CODEC == MAS3507D /* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -780
#define VOLUME_MAX  180

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

/* convert tenth of dB volume (-780..+180) to dac3550 register value */
static int tenthdb2reg(int db)
{
    if (db < -540)                  /* 3 dB steps */
        return (db + 780) / 30;
    else                            /* 1.5 dB steps */
        return (db + 660) / 15;
}
#endif

#if (CONFIG_CODEC == MAS3507D) || defined HAVE_UDA1380 \
    || defined HAVE_WM8975 || defined HAVE_WM8758 || defined(HAVE_WM8731) \
    || defined(HAVE_WM8721) || defined(HAVE_TLV320) || defined(HAVE_WM8751) \
    || defined(HAVE_AS3514)
 /* volume/balance/treble/bass interdependency main part */
#define VOLUME_RANGE (VOLUME_MAX - VOLUME_MIN)

/* all values in tenth of dB    MAS3507D    UDA1380  */
int current_volume = 0;    /* -780..+180  -840..   0 */
int current_balance = 0;   /* -960..+960  -840..+840 */
int current_treble = 0;    /* -150..+150     0.. +60 */
int current_bass = 0;      /* -150..+150     0..+240 */

static void set_prescaled_volume(void)
{
    int prescale = 0;
    int l, r;

/* The WM codecs listed don't have suitable prescaler functionality, so we let
 * the prescaler stay at 0 for these unless SW tone controls are in use */
#if defined(HAVE_SW_TONE_CONTROLS) || !(defined(HAVE_WM8975) \
    || defined(HAVE_WM8731) || defined(HAVE_WM8721) || defined(HAVE_WM8751))

    prescale = MAX(current_bass, current_treble);
    if (prescale < 0)
        prescale = 0;  /* no need to prescale if we don't boost
                          bass or treble */

    /* Gain up the analog volume to compensate the prescale gain reduction,
     * but if this would push the volume over the top, reduce prescaling
     * instead (might cause clipping). */
    if (current_volume + prescale > VOLUME_MAX)
        prescale = VOLUME_MAX - current_volume;
#endif

#if defined(HAVE_SW_TONE_CONTROLS)
    dsp_callback(DSP_CALLBACK_SET_PRESCALE, prescale);
#elif CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KPRESCALE, prescale_table[prescale/10]);
#elif defined(HAVE_UDA1380) || defined(HAVE_WM8758)
    audiohw_set_mixer_vol(tenthdb2mixer(-prescale), tenthdb2mixer(-prescale));
#endif

    if (current_volume == VOLUME_MIN)
        prescale = 0;  /* Make sure the chip gets muted at VOLUME_MIN */

    l = r = current_volume + prescale;

    if (current_balance > 0)
    {
        l -= current_balance;
        if (l < VOLUME_MIN)
            l = VOLUME_MIN;
    }
    if (current_balance < 0)
    {
        r += current_balance;
        if (r < VOLUME_MIN)
            r = VOLUME_MIN;
    }

#if CONFIG_CODEC == MAS3507D
    dac_volume(tenthdb2reg(l), tenthdb2reg(r), false);
#elif defined(HAVE_UDA1380) || defined(HAVE_WM8975) || defined(HAVE_WM8758) \
   || defined(HAVE_WM8731) || defined(HAVE_WM8721) || defined(HAVE_WM8751) \
   || defined(HAVE_AS3514)
    audiohw_set_master_vol(tenthdb2master(l), tenthdb2master(r));
#if defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8751)
    audiohw_set_lineout_vol(tenthdb2master(0), tenthdb2master(0));
#endif

#elif defined(HAVE_TLV320)
    audiohw_set_headphone_vol(tenthdb2master(l), tenthdb2master(r));
#endif
}
#endif /* (CONFIG_CODEC == MAS3507D) || defined HAVE_UDA1380 */
#endif /* !SIMULATOR */

#if CONFIG_CODEC != SWCODEC
int channel_configuration = SOUND_CHAN_STEREO;
int stereo_width = 100;
#endif

#ifndef SIMULATOR

#if CONFIG_CODEC != SWCODEC
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
                    /* straight = - (1 + width) / (2 * width) */
                    fp_straight = - ((((1<<19) + fp_width) / (fp_width >> 9)) << 9);
                    fp_cross = (1<<19) + fp_straight;
                }
                val_ll = val_rr = fp_straight & 0xfffff;
                val_lr = val_rl = fp_cross & 0xfffff;
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
            val_ll = 0xc0000;
            val_lr = 0x40000;
            val_rl = 0x40000;
            val_rr = 0xc0000;
            break;
    }

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_LL, &val_ll, 1); /* LL */
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_LR, &val_lr, 1); /* LR */
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_RL, &val_rl, 1); /* RL */
    mas_writemem(MAS_BANK_D0, MAS_D0_OUT_RR, &val_rr, 1); /* RR */
#elif CONFIG_CODEC == MAS3507D
    mas_writemem(MAS_BANK_D1, 0x7f8, &val_ll, 1); /* LL */
    mas_writemem(MAS_BANK_D1, 0x7f9, &val_lr, 1); /* LR */
    mas_writemem(MAS_BANK_D1, 0x7fa, &val_rl, 1); /* RL */
    mas_writemem(MAS_BANK_D1, 0x7fb, &val_rr, 1); /* RR */
#endif
}

#endif /* CONFIG_CODEC != SWCODEC */

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
unsigned long mdb_shape_shadow = 0;
unsigned long loudness_shadow = 0;
#endif

void sound_set_volume(int value)
{
    if(!audio_is_initialized)
        return;
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(value + 115) & 0xff) << 8;
    mas_codec_writereg(0x10, tmp);
#elif (CONFIG_CODEC == MAS3507D) || defined HAVE_UDA1380 \
   || defined HAVE_WM8975 || defined HAVE_WM8758 || defined HAVE_WM8731 \
   || defined(HAVE_WM8721) || defined(HAVE_TLV320) || defined(HAVE_WM8751) \
   || defined(HAVE_AS3514)
    current_volume = value * 10;     /* tenth of dB */
    set_prescaled_volume();
#elif CONFIG_CPU == PNX0101
    int tmp = (60 - value * 4) & 0xff;
    CODECVOL = tmp | (tmp << 8);
#endif
}

void sound_set_balance(int value)
{
    if(!audio_is_initialized)
        return;
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(value * 127 / 100) & 0xff) << 8;
    mas_codec_writereg(0x11, tmp);
#elif CONFIG_CODEC == MAS3507D || defined HAVE_UDA1380 \
   || defined HAVE_WM8975 || defined HAVE_WM8758 || defined HAVE_WM8731 \
   || defined(HAVE_WM8721) || defined(HAVE_TLV320) || defined(HAVE_WM8751) \
   || defined(HAVE_AS3514)
    current_balance = value * VOLUME_RANGE / 100; /* tenth of dB */
    set_prescaled_volume();
#elif CONFIG_CPU == PNX0101
    /* TODO: implement for iFP */
    (void)value;
#endif
}

void sound_set_bass(int value)
{
    if(!audio_is_initialized)
        return;
#if defined(HAVE_SW_TONE_CONTROLS)
    current_bass = value * 10;
    dsp_callback(DSP_CALLBACK_SET_BASS, current_bass);
    set_prescaled_volume();
#elif (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(value * 8) & 0xff) << 8;
    mas_codec_writereg(0x14, tmp);
#elif CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KBASS, bass_table[value+15]);
    current_bass = value * 10;
    set_prescaled_volume();
#elif defined(HAVE_WM8751)
    current_bass = value;
    audiohw_set_bass(value);
    set_prescaled_volume();
#elif defined HAVE_WM8975 || defined HAVE_WM8758 || defined(HAVE_UDA1380) \
    || defined HAVE_WM8731 || defined(HAVE_WM8721)
    current_bass = value * 10;
    audiohw_set_bass(value);
    set_prescaled_volume();
#elif CONFIG_CPU == PNX0101
    /* TODO: implement for iFP */
    (void)value;
#endif               
}

void sound_set_treble(int value)
{
    if(!audio_is_initialized)
        return;
#if defined(HAVE_SW_TONE_CONTROLS)
    current_treble = value * 10;
    dsp_callback(DSP_CALLBACK_SET_TREBLE, current_treble);
    set_prescaled_volume();
#elif (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(value * 8) & 0xff) << 8;
    mas_codec_writereg(0x15, tmp);
#elif CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KTREBLE, treble_table[value+15]);
    current_treble = value * 10;
    set_prescaled_volume();
#elif defined(HAVE_WM8751)
    audiohw_set_treble(value);
    current_treble = value;
    set_prescaled_volume();
#elif defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_UDA1380) \
   || defined(HAVE_WM8731) || defined(HAVE_WM8721)
    audiohw_set_treble(value);
    current_treble = value * 10;
    set_prescaled_volume();
#elif CONFIG_CPU == PNX0101 
    /* TODO: implement for iFP */
    (void)value;
#endif    
}

void sound_set_channels(int value)
{
#if CONFIG_CODEC == SWCODEC
    dsp_callback(DSP_CALLBACK_SET_CHANNEL_CONFIG, value);
#else
    if(!audio_is_initialized)
        return;
    channel_configuration = value;
    set_channel_config();
#endif
}

void sound_set_stereo_width(int value)
{
#if CONFIG_CODEC == SWCODEC
    dsp_callback(DSP_CALLBACK_SET_STEREO_WIDTH, value);
#else
    if(!audio_is_initialized)
        return;
    stereo_width = value;
    if (channel_configuration == SOUND_CHAN_CUSTOM)
        set_channel_config();
#endif
}

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void sound_set_loudness(int value)
{
    if(!audio_is_initialized)
        return;
    loudness_shadow = (loudness_shadow & 0x04) |
                       (MAX(MIN(value * 4, 0x44), 0) << 8);
    mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
}

void sound_set_avc(int value)
{
    if(!audio_is_initialized)
        return;
    int tmp;
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
}

void sound_set_mdb_strength(int value)
{
    if(!audio_is_initialized)
        return;
    mas_codec_writereg(MAS_REG_KMDB_STR, (value & 0x7f) << 8); 
}

void sound_set_mdb_harmonics(int value)
{
    if(!audio_is_initialized)
        return;
    int tmp = value * 127 / 100;
    mas_codec_writereg(MAS_REG_KMDB_HAR, (tmp & 0x7f) << 8);
}

void sound_set_mdb_center(int value)
{
    if(!audio_is_initialized)
        return;
    mas_codec_writereg(MAS_REG_KMDB_FC, (value/10) << 8);
}

void sound_set_mdb_shape(int value)
{
    if(!audio_is_initialized)
        return;
    mdb_shape_shadow = (mdb_shape_shadow & 0x02) | ((value/10) << 8);
    mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
}

void sound_set_mdb_enable(int value)
{
    if(!audio_is_initialized)
        return;
    mdb_shape_shadow = (mdb_shape_shadow & ~0x02) | (value?2:0);
    mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
}

void sound_set_superbass(int value)
{
    if(!audio_is_initialized)
        return;
    loudness_shadow = (loudness_shadow & ~0x04) | (value?4:0);
    mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
}
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */

#else /* SIMULATOR */
int sim_volume;
void sound_set_volume(int value)
{
    /* 128 is SDL_MIX_MAXVOLUME */
    sim_volume = 128 * (value - VOLUME_MIN / 10) / ((VOLUME_MAX - VOLUME_MIN) / 10);
}

void sound_set_balance(int value)
{
    (void)value;
}

void sound_set_bass(int value)
{
    (void)value;
}

void sound_set_treble(int value)
{
    (void)value;
}

void sound_set_channels(int value)
{
    (void)value;
}

void sound_set_stereo_width(int value)
{
    (void)value;
}

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void sound_set_loudness(int value)
{
    (void)value;
}

void sound_set_avc(int value)
{
    (void)value;
}

void sound_set_mdb_strength(int value)
{
    (void)value;
}

void sound_set_mdb_harmonics(int value)
{
    (void)value;
}

void sound_set_mdb_center(int value)
{
    (void)value;
}

void sound_set_mdb_shape(int value)
{
    (void)value;
}

void sound_set_mdb_enable(int value)
{
    (void)value;
}

void sound_set_superbass(int value)
{
    (void)value;
}
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
#endif /* SIMULATOR */

void sound_set(int setting, int value)
{
    sound_set_type* sound_set_val = sound_get_fn(setting);
    if (sound_set_val)
        sound_set_val(value);
}

#if (!defined(HAVE_AS3514) && !defined (HAVE_WM8731)) || defined(SIMULATOR)
int sound_val2phys(int setting, int value)
{
#if CONFIG_CODEC == MAS3587F
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
#elif defined(HAVE_UDA1380)
    int result = 0;
    
    switch(setting)
    {
        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
        case SOUND_MIC_GAIN:
            result = value * 5;         /* (1/2) * 10 */
            break;

        default:
            result = value;
            break;
    }
    return result;
#elif defined(HAVE_TLV320) || defined(HAVE_WM8731)
    int result = 0;
    
    switch(setting)
    {
        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
            result = (value - 23) * 15;    /* (x - 23)/1.5 *10 */
            break;

        case SOUND_MIC_GAIN:
            result = value * 200;          /* 0 or 20 dB */
            break;

       default:
            result = value;
            break;
    }
    return result;
#elif defined(HAVE_AS3514)
    /* This is here for the sim only and the audio driver has its own */
    int result;

    switch(setting)
    {
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = (value - 23) * 15;
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
#endif /* !defined(HAVE_AS3514) || defined(SIMULATOR) */

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
#ifndef SIMULATOR
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
#else /* SIMULATOR */
void sound_set_pitch(int pitch)
{
    (void)pitch;
}

int sound_get_pitch(void)
{
    return 1000;
}
#endif /* SIMULATOR */
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
