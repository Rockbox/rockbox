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
#include "logf.h"
#ifndef SIMULATOR
#include "i2c.h"
#include "mas.h"
#ifdef HAVE_UDA1380
#include "uda1380.h"
#elif defined(HAVE_WM8975)
#include "wm8975.h"
#elif defined(HAVE_WM8758)
#include "wm8758.h"
#elif defined(HAVE_WM8731)
#include "wm8731l.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#endif
#include "dac.h"
#include "system.h"
#include "hwcompat.h"
#if CONFIG_CODEC == SWCODEC
#include "pcm_playback.h"
#endif
#endif

#ifndef SIMULATOR
extern bool audio_is_initialized;

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
extern unsigned long shadow_io_control_main;
extern unsigned shadow_codec_reg0;
#endif
#endif /* SIMULATOR */

struct sound_settings_info {
    const char *unit;
    int numdecimals;
    int steps;
    int minval;
    int maxval;
    int defaultval;
    sound_set_type *setfn;
};

static const struct sound_settings_info sound_settings_table[] = {
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_VOLUME]        = {"dB", 0,  1,-100,  12, -25, sound_set_volume},
    [SOUND_BASS]          = {"dB", 0,  1, -12,  12,   6, sound_set_bass},
    [SOUND_TREBLE]        = {"dB", 0,  1, -12,  12,   6, sound_set_treble},
#elif defined(HAVE_UDA1380)
    [SOUND_VOLUME]        = {"dB", 0,  1, -84,   0, -25, sound_set_volume},
    [SOUND_BASS]          = {"dB", 0,  2,   0,  24,   0, sound_set_bass},
    [SOUND_TREBLE]        = {"dB", 0,  2,   0,   6,   0, sound_set_treble},
#elif defined(HAVE_WM8975)
    [SOUND_VOLUME]        = {"dB", 0,  1, -73,   6, -25, sound_set_volume},
    [SOUND_BASS]          = {"dB", 0,  1,  -6,   9,   0, sound_set_bass},
    [SOUND_TREBLE]        = {"dB", 0,  1,  -6,   9,   0, sound_set_treble},
#elif defined(HAVE_WM8758)
    [SOUND_VOLUME]        = {"dB", 0,  1, -57,   6, -25, sound_set_volume},
    [SOUND_BASS]          = {"dB", 0,  1,  -6,   9,   0, sound_set_bass},
    [SOUND_TREBLE]        = {"dB", 0,  1,  -6,   9,   0, sound_set_treble},
#elif defined(HAVE_WM8731)
    [SOUND_VOLUME]        = {"dB", 0,  1, -73,   6, -25, sound_set_volume},
    [SOUND_BASS]          = {"dB", 0,  1,  -6,   9,   0, sound_set_bass},
    [SOUND_TREBLE]        = {"dB", 0,  1,  -6,   9,   0, sound_set_treble},
#else /* MAS3507D */
    [SOUND_VOLUME]        = {"dB", 0,  1, -78,  18, -18, sound_set_volume},
    [SOUND_BASS]          = {"dB", 0,  1, -15,  15,   7, sound_set_bass},
    [SOUND_TREBLE]        = {"dB", 0,  1, -15,  15,   7, sound_set_treble},
#endif
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0, sound_set_balance},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0, sound_set_channels},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  1,   0, 255, 100, sound_set_stereo_width},
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_LOUDNESS]      = {"dB", 0,  1,   0,  17,   0, sound_set_loudness},
    [SOUND_AVC]           = {"",   0,  1,  -1,   4,   0, sound_set_avc},
    [SOUND_MDB_STRENGTH]  = {"dB", 0,  1,   0, 127,  48, sound_set_mdb_strength},
    [SOUND_MDB_HARMONICS] = {"%",  0,  1,   0, 100,  50, sound_set_mdb_harmonics},
    [SOUND_MDB_CENTER]    = {"Hz", 0, 10,  20, 300,  60, sound_set_mdb_center},
    [SOUND_MDB_SHAPE]     = {"Hz", 0, 10,  50, 300,  90, sound_set_mdb_shape},
    [SOUND_MDB_ENABLE]    = {"",   0,  1,   0,   1,   0, sound_set_mdb_enable},
    [SOUND_SUPERBASS]     = {"",   0,  1,   0,   1,   0, sound_set_superbass},
#endif
#if CONFIG_CODEC == MAS3587F
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  15,   8, NULL},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  15,   8, NULL},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,  15,   2, NULL},
#elif defined(HAVE_UDA1380)
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,   8,   8, NULL},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,   8,   8, NULL},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,  15,   2, NULL},
    [SOUND_DECIMATOR_LEFT_GAIN] = {"dB", 1,  1,-128,  48,   0, NULL},
    [SOUND_DECIMATOR_RIGHT_GAIN]= {"dB", 1,  1,-128,  48,   0, NULL},
#endif
};

const char *sound_unit(int setting)
{
    return sound_settings_table[setting].unit;
}

int sound_numdecimals(int setting)
{
    return sound_settings_table[setting].numdecimals;
}

int sound_steps(int setting)
{
    return sound_settings_table[setting].steps;
}

int sound_min(int setting)
{
    return sound_settings_table[setting].minval;
}

int sound_max(int setting)
{
    return sound_settings_table[setting].maxval;
}

int sound_default(int setting)
{
    return sound_settings_table[setting].defaultval;
}

sound_set_type* sound_get_fn(int setting)
{
    if ((unsigned)setting < (sizeof(sound_settings_table)
                             / sizeof(struct sound_settings_info)))
        return sound_settings_table[setting].setfn;
    else
        return NULL;
}

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

#ifdef HAVE_UDA1380  /* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -840
#define VOLUME_MAX  0

/* convert tenth of dB volume (-840..0) to master volume register value */
static int tenthdb2master(int db)
{
    if (db < -720)                  /* 1.5 dB steps */
        return (2940 - db) / 15;
    else if (db < -660)             /* 0.75 dB steps */
        return (1110 - db) * 2 / 15;
    else if (db < -520)             /* 0.5 dB steps */
        return (520 - db) / 5;
    else                            /* 0.25 dB steps */
        return -db * 2 / 5;
}

/* convert tenth of dB volume (-780..0) to mixer volume register value */
static int tenthdb2mixer(int db)
{
    if (db < -660)                 /* 1.5 dB steps */
        return (2640 - db) / 15;
    else if (db < -600)            /* 0.75 dB steps */
        return (990 - db) * 2 / 15;
    else if (db < -460)            /* 0.5 dB steps */
        return (460 - db) / 5; 
    else                           /* 0.25 dB steps */
        return -db * 2 / 5;
}

#elif defined(HAVE_WM8975) 
/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -730
#define VOLUME_MAX  60

/* convert tenth of dB volume (-730..60) to master volume register value */
static int tenthdb2master(int db)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB  (0x7f) */
    /* 1111001 == 0dB   (0x79) */
    /* 0110000 == -73dB (0x30 */
    /* 0101111 == mute  (0x2f) */

    if (db <= -730) {
        return 0x0;
    } else {
        return((db/10)+73+0x2f);
    }
}

/* convert tenth of dB volume (-780..0) to mixer volume register value */
static int tenthdb2mixer(int db)
{
    if (db < -660)                 /* 1.5 dB steps */
        return (2640 - db) / 15;
    else if (db < -600)            /* 0.75 dB steps */
        return (990 - db) * 2 / 15;
    else if (db < -460)            /* 0.5 dB steps */
        return (460 - db) / 5; 
    else                           /* 0.25 dB steps */
        return -db * 2 / 5;
}

#elif defined(HAVE_WM8758) 
/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -730
#define VOLUME_MAX  60

/* convert tenth of dB volume (-730..60) to master volume register value */
static int tenthdb2master(int db)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB  (0x7f) */
    /* 1111001 == 0dB   (0x79) */
    /* 0110000 == -73dB (0x30 */
    /* 0101111 == mute  (0x2f) */

    if (db <= -570) {
        return 0x0;
    } else {
        return((db/10)+57);
    }
}

/* convert tenth of dB volume (-780..0) to mixer volume register value */
static int tenthdb2mixer(int db)
{
    if (db < -660)                 /* 1.5 dB steps */
        return (2640 - db) / 15;
    else if (db < -600)            /* 0.75 dB steps */
        return (990 - db) * 2 / 15;
    else if (db < -460)            /* 0.5 dB steps */
        return (460 - db) / 5; 
    else                           /* 0.25 dB steps */
        return -db * 2 / 5;
}

#elif defined(HAVE_WM8731) 
/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -730
#define VOLUME_MAX  60

/* convert tenth of dB volume (-730..60) to master volume register value */
static int tenthdb2master(int db)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB  (0x7f) */
    /* 1111001 == 0dB   (0x79) */
    /* 0110000 == -73dB (0x30 */
    /* 0101111 == mute  (0x2f) */

    if (db <= -570) {
        return 0x0;
    } else {
        return((db/10)+57);
    }
}

/* convert tenth of dB volume (-780..0) to mixer volume register value */
static int tenthdb2mixer(int db)
{
    if (db < -660)                 /* 1.5 dB steps */
        return (2640 - db) / 15;
    else if (db < -600)            /* 0.75 dB steps */
        return (990 - db) * 2 / 15;
    else if (db < -460)            /* 0.5 dB steps */
        return (460 - db) / 5; 
    else                           /* 0.25 dB steps */
        return -db * 2 / 5;
}

#endif

#if (CONFIG_CODEC == MAS3507D) || defined HAVE_UDA1380 || \
    defined HAVE_WM8975 || defined HAVE_WM8758 || defined(HAVE_WM8731)
 /* volume/balance/treble/bass interdependency main part */
#define VOLUME_RANGE (VOLUME_MAX - VOLUME_MIN)

/* all values in tenth of dB    MAS3507D    UDA1380  */
int current_volume = 0;    /* -780..+180  -840..   0 */
int current_balance = 0;   /* -960..+960  -840..+840 */
int current_treble = 0;    /* -150..+150     0.. +60 */
int current_bass = 0;      /* -150..+150     0..+240 */

static void set_prescaled_volume(void)
{
    int prescale;
    int l, r;

    prescale = MAX(current_bass, current_treble);
    if (prescale < 0)
        prescale = 0;  /* no need to prescale if we don't boost
                          bass or treble */

    /* Gain up the analog volume to compensate the prescale gain reduction,
     * but if this would push the volume over the top, reduce prescaling
     * instead (might cause clipping). */
    if (current_volume + prescale > VOLUME_MAX)
        prescale = VOLUME_MAX - current_volume;

#if CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KPRESCALE, prescale_table[prescale/10]);
#elif defined(HAVE_UDA1380)
    uda1380_set_mixer_vol(tenthdb2mixer(-prescale), tenthdb2mixer(-prescale));
#elif defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8731)
    wmcodec_set_mixer_vol(tenthdb2mixer(-prescale), tenthdb2mixer(-prescale));
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
#elif defined(HAVE_UDA1380)
    uda1380_set_master_vol(tenthdb2master(l), tenthdb2master(r));
#elif defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8731)
    wmcodec_set_master_vol(tenthdb2master(l), tenthdb2master(r));
#endif
}
#endif /* (CONFIG_CODEC == MAS3507D) || defined HAVE_UDA1380 */
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
#elif (CONFIG_CODEC == MAS3507D) || defined HAVE_UDA1380 || \
      defined HAVE_WM8975 || defined HAVE_WM8758 || defined HAVE_WM8731
    current_volume = value * 10;     /* tenth of dB */
    set_prescaled_volume();                          
#elif CONFIG_CPU == PNX0101
    /* TODO: implement for iFP */
    (void)value;
#endif
}

void sound_set_balance(int value)
{
    if(!audio_is_initialized)
        return;
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(value * 127 / 100) & 0xff) << 8;
    mas_codec_writereg(0x11, tmp);
#elif CONFIG_CODEC == MAS3507D || defined HAVE_UDA1380 || \
      defined HAVE_WM8975 || defined HAVE_WM8758 || defined HAVE_WM8731
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
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(value * 8) & 0xff) << 8;
    mas_codec_writereg(0x14, tmp);
#elif CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KBASS, bass_table[value+15]);
    current_bass = value * 10;
    set_prescaled_volume();
#elif defined(HAVE_UDA1380)
    uda1380_set_bass(value >> 1);
    current_bass = value * 10;
    set_prescaled_volume();
#elif defined HAVE_WM8975 || defined HAVE_WM8758 || defined HAVE_WM8731
    current_bass = value * 10;
    wmcodec_set_bass(value);
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
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(value * 8) & 0xff) << 8;
    mas_codec_writereg(0x15, tmp);
#elif CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KTREBLE, treble_table[value+15]);
    current_treble = value * 10;
    set_prescaled_volume();
#elif defined(HAVE_UDA1380)
    uda1380_set_treble(value >> 1);
    current_treble = value * 10;
    set_prescaled_volume();
#elif defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8731)
    wmcodec_set_treble(value);
    current_treble = value * 10;
    set_prescaled_volume();
#elif CONFIG_CPU == PNX0101
    /* TODO: implement for iFP */
    (void)value;
#endif    
}

void sound_set_channels(int value)
{
    if(!audio_is_initialized)
        return;
    channel_configuration = value;
    set_channel_config();  
}

void sound_set_stereo_width(int value)
{
    if(!audio_is_initialized)
        return;
    stereo_width = value;
    if (channel_configuration == SOUND_CHAN_CUSTOM)
        set_channel_config();
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
void sound_set_volume(int value)
{
    (void)value;
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
            result = value * 30;        /* (24/8) *10 */
            break;

        case SOUND_MIC_GAIN:
            result = value * 20;        /* (30/15) *10 */
            break;
            
        case SOUND_DECIMATOR_LEFT_GAIN:
        case SOUND_DECIMATOR_RIGHT_GAIN:
            result = value * 5;         /* (1/2) *10 */
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
