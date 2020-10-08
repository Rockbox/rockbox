/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _AUDIOHW_H_
#define _AUDIOHW_H_

#include "config.h"
#include <stdbool.h>
#include <inttypes.h>

/* define some audiohw caps */
#define TREBLE_CAP            (1 << 0)
#define BASS_CAP              (1 << 1)
#define BALANCE_CAP           (1 << 2)
#define CLIPPING_CAP          (1 << 3)
#define PRESCALER_CAP         (1 << 4)
#define BASS_CUTOFF_CAP       (1 << 5)
#define TREBLE_CUTOFF_CAP     (1 << 6)
#define EQ_CAP                (1 << 7)
#define DEPTH_3D_CAP          (1 << 8)
#define LINEOUT_CAP           (1 << 9)
#define MONO_VOL_CAP          (1 << 10)
#define LIN_GAIN_CAP          (1 << 11)
#define MIC_GAIN_CAP          (1 << 12)
#define FILTER_ROLL_OFF_CAP   (1 << 13)

/* Used by every driver to export its min/max/default values for its audio
   settings. */
#ifdef AUDIOHW_IS_SOUND_C
/* This is the master file with the settings table... */
struct sound_settings_info
{
    const char *unit;
    char numdecimals;
    char steps;
    short minval;
    short maxval;
    short defaultval;
};

#undef AUDIOHW_SETTING /* will have been #defined in config.h as empty */
/* Use AUDIOHW_SETTING to create an audio setting. There are two ways to use this
 * macro:
 * AUDIOHW_SETTING(name, unit, nr_decimals, step, min, max, default)
 * AUDIOHW_SETTING(name, unit, nr_decimals, step, min, max, default, expr)
 *
 * It is important to understand that each setting has two scales: the hardware
 * scale and the user scale. In the first form of the macro, they coincide.
 * In the second form, the conversion from hardware to user is done by the
 * expression [expr] provided in the extra argument (see examples below). The
 * hardware scale ranges from [min] to [max], in steps of [step]. The default value
 * is [default]. Furthermore, when displaying the value to the user, [nr_decimals]
 * gives the number of decimal points to display. Thus if [nr_decimals] is 0 then
 * a value of x means x [unit]. If [nr_decimals] is 1 then a value of x means
 * x/10 [unit] and so on. Note that both [nr_decimals] and [unit] are irrelevant
 * to the hardware, they simply provide a flexible way to show natural value to
 * the user. When you want the user scale to be different than the hardware scale,
 * you must provide [expr], an expression that can use the variable "val", which
 * represents the hardware value, and converts it to the user value. The [expr]
 * can involved a function call in very complicated/nonlinear cases, as long as
 * the function does not have any side-effect. Finally, the [name] parameter
 * must be one of the settings listed in audiohw_setting.h
 *
 * Examples:
 *
 *   AUDIOHW_SETTING(VOLUME, "dB", 0, 1, -100,  12,  -25)
 * This describes the volume setting. The values are in dB (no decimal). The
 * minimum value is -100 and the maximum is 12, with a step of 1 and a default
 * value of -25. This means that the hardware can take any of the following value:
 *   -100, -99, -98, ..., 11,12
 * Since there is are decimals and no conversion expression, a hardware value of
 * x means x dB. So a value of -25 means -25 dB, a value of 5 means 5 dB.
 * WARNING VOLUME is actually special: whatever scale you choose, the sound code
 * will always set the volume by calling audiohw_set_volume() with a centibel
 * value (ie it will not perform any conversion). Thus it is strongly advised
 * that you always choose a VOLUME scale with precision 1 (centibels) and no
 * hardware conversion.
 *
 *   AUDIOHW_SETTING(MDB_CENTER,   "Hz",  0, 10,  20, 300,  60)
 * This describes the MDB (dynbamic bass) center. The values are in Hz (no
 * decimal). The minimum value is 20 and the maximum is 300, in steps of 10
 * and a default of 60. Thus hardware can take any of the following value:
 *   20, 30, 40, ... 290, 300
 * Since there are no decimals and no conversion expression, a hardware of x
 * means x Hz. So a value of 60 means 60 Hz.
 *
 *   AUDIOHW_SETTING(BASS, "dB", 1, 15,  -60,   90,    0)
 * This describes the bass control. Since there is one decimal, the values are
 * in tenth of dB. The minimum value is -60 and the maximum is 90, in steps of
 * 15 and a default value of 0. Thus hardware can take any of the following value:
 *   -60, -45, -30, ... 60, 75, 90
 * Since there is one decimal, a hardware value of x means x/10 Hz. So a value
 * of 60 means 60/10 = 6 dB. A value of -45 means -45/10 = -4.5 dB.
 *
 *   AUDIOHW_SETTING(DEPTH_3D, "%", 0,  1, 0,   15,    0, (100 * val + 8) / 15)
 * This describes 3D enhancement control. The values are in percentage (no
 * decimal). This setting makes a difference between hardware and user scale.
 * The minimal hardware value is 0 and the maximum is 15, in steps of 1 and
 * a default of 0. Thus hardware can take any of the following value:
 *   0, 1, 2, ... 14, 15
 * Because of the conversion expression, a hardware value of x means
 * (100 * val + 8) / 15) %. A hardware value of 0 means (100 * 0 + 8) / 15) = 0 %
 * because the result must be an integer (8 / 15 = 0). A hardware value of 1
 * means (100 * 1 + 8) / 15 = 7 %. A hardware value of 15 means
 * (100 * 15 + 8) / 15 = 100 %. In fact, from the user point of view, the range
 * of available values is:
 *   0%, 7%, 13%, 20%, ..., 93%, 100%
 *
 *   AUDIOHW_SETTING(LEFT_GAIN, "dB", 2, 15,-345, 1200, 0, val * 5)
 * This describes the left gain. Since there are two decimals, the values are in
 * hundredth of dB. This setting makes a difference between hardware and user scale.
 * The minimal hardware value is -345 and the maximum is 1200, in steps of 15 and
 * a default of 0. Thus hardware can take any of the following value:
 *   -345, -330, -315, ..., 1185, 1200
 * Because of the conversion expression, a hardware value of x means
 * val * 5 hundredth of dB or, in other words, (val * 5)/100 dB (where we keep two
 * decimals). A hardware value of -345 means -345 * 5 = -1725 hundredth of dB
 * = -17,25 dB. A value of -330 means -330*5 = -1650 hundredth of dB = -16,50 dB.
 * A value of 1200 means 1200 * 5 = 6000 hundredth of dB = 60 dB. In fact,
 * from the user point of view, the range of available values is:
 *   -17.25 dB, -16.60 dB, ..., 59.25 dB, 60dB.
 *
 *   AUDIOHW_SETTING(DEPTH_3D, "dB", 0, 1, 0, 3, 0, depth3d_phys2_val(val))
 * This describes 3D enhancement control. The values are in dB (no
 * decimal). This setting makes a difference between hardware and user scale.
 * The minimal hardware value is 0 and the maximum is 3, in steps of 1 and
 * a default of 0. Thus hardware can take any of the following value:
 *   0, 1, 2, 3
 * Because of the conversion expression, a hardware value of x means
 * depth3d_phys2_val(x) dB. If for example the conversion functions is:
 *   int depth3d_phys2_val(int val)
 *   {
 *     return val == 0 ? 0 : val * 5 + 30;
 *   }
 * then from the user point of view, the range of available values is:
 *   0 dB, 35 dB, 40 dB, 45 dB
 */
#define AUDIOHW_SETTING(name, us, nd, st, minv, maxv, defv, expr...) \
    static const struct sound_settings_info _audiohw_setting_##name = \
    { .unit = us, .numdecimals = nd, .steps = st, \
      .minval = minv, .maxval = maxv, .defaultval = defv }; \
    static inline int _sound_val2phys_##name(int val) \
    { return #expr[0] ? expr : val; }
#endif

#ifdef HAVE_UDA1380
#include "uda1380.h"
#elif defined(HAVE_UDA1341)
#include "uda1341.h"
#elif defined(HAVE_WM8740)
#include "wm8740.h"
#elif defined(HAVE_WM8750) || defined(HAVE_WM8751)
#include "wm8751.h"
#elif defined(HAVE_WM8978)
#include "wm8978.h"
#elif defined(HAVE_WM8975)
#include "wm8975.h"
#elif defined(HAVE_WM8985)
#include "wm8985.h"
#elif defined(HAVE_WM8758)
#include "wm8758.h"
#elif defined(HAVE_WM8711) || defined(HAVE_WM8721) || \
      defined(HAVE_WM8731)
#include "wm8731.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#elif defined(HAVE_AS3514)
#include "as3514.h"
#if defined(HAVE_DAC3550A)
#include "dac3550a.h"
#endif /* HAVE_DAC3550A */
#elif defined(HAVE_TSC2100)
#include "tsc2100.h"
#elif defined(HAVE_JZ4740_CODEC)
#include "jz4740-codec.h"
#elif defined(HAVE_AK4537)
#include "ak4537.h"
#elif defined(HAVE_RK27XX_CODEC)
#include "rk27xx_codec.h"
#elif defined(HAVE_AIC3X)
#include "aic3x.h"
#elif defined(HAVE_CS42L55)
#include "cs42l55.h"
#elif defined(HAVE_IMX233_CODEC)
#include "imx233-codec.h"
#elif defined(HAVE_DUMMY_CODEC)
#include "dummy_codec.h"
#elif defined(HAVE_DF1704_CODEC)
#include "df1704.h"
#elif defined(HAVE_PCM1792_CODEC)
#include "pcm1792.h"
#elif defined(HAVE_NWZ_LINUX_CODEC)
#include "nwzlinux_codec.h"
#elif defined(HAVE_CS4398)
#include "cs4398.h"
#elif defined(HAVE_ES9018)
#include "es9018.h"
#elif (CONFIG_PLATFORM & (PLATFORM_ANDROID | PLATFORM_MAEMO \
       | PLATFORM_PANDORA | PLATFORM_SDL))
#include "hosted_codec.h"
#elif defined(DX50)
#include "codec-dx50.h"
#elif defined(DX90)
#include "codec-dx90.h"
#elif defined(HAVE_ROCKER_CODEC)
#include "rocker_codec.h"
#elif defined(HAVE_XDUOO_LINUX_CODEC)
#include "xduoolinux_codec.h"
#elif defined(HAVE_FIIO_LINUX_CODEC)
#include "fiiolinux_codec.h"
#elif defined(HAVE_EROSQ_LINUX_CODEC)
#include "erosqlinux_codec.h"
#endif

/* convert caps into defines */
#ifdef AUDIOHW_CAPS
/* Tone controls */
#if (AUDIOHW_CAPS & TREBLE_CAP)
#define AUDIOHW_HAVE_TREBLE
#endif

#if (AUDIOHW_CAPS & BASS_CAP)
#define AUDIOHW_HAVE_BASS
#endif

#if (AUDIOHW_CAPS & BASS_CUTOFF_CAP)
#define AUDIOHW_HAVE_BASS_CUTOFF
#endif

#if (AUDIOHW_CAPS & TREBLE_CUTOFF_CAP)
#define AUDIOHW_HAVE_TREBLE_CUTOFF
#endif

#if (AUDIOHW_CAPS & BALANCE_CAP)
#define AUDIOHW_HAVE_BALANCE
#endif

#if (AUDIOHW_CAPS & CLIPPING_CAP)
#define AUDIOHW_HAVE_CLIPPING
#endif

#if (AUDIOHW_CAPS & PRESCALER_CAP)
#define AUDIOHW_HAVE_PRESCALER
#endif

/* Hardware EQ tone controls */
#if (AUDIOHW_CAPS & EQ_CAP)
/* A hardware equalizer is present (or perhaps some tone control variation
 * that is not Bass and/or Treble) */
#define AUDIOHW_HAVE_EQ

/* Defined band indexes for supported bands */
enum
{
    /* Band 1 is implied; bands must be contiguous, 1 to N */
    AUDIOHW_EQ_BAND1,
#define AUDIOHW_HAVE_EQ_BAND1
#if (AUDIOHW_EQ_BAND_CAPS & (EQ_CAP << 1))
    AUDIOHW_EQ_BAND2,
#define AUDIOHW_HAVE_EQ_BAND2
#if (AUDIOHW_EQ_BAND_CAPS & (EQ_CAP << 2))
    AUDIOHW_EQ_BAND3,
#define AUDIOHW_HAVE_EQ_BAND3
#if (AUDIOHW_EQ_BAND_CAPS & (EQ_CAP << 3))
    AUDIOHW_EQ_BAND4,
#define AUDIOHW_HAVE_EQ_BAND4
#if (AUDIOHW_EQ_BAND_CAPS & (EQ_CAP << 4))
    AUDIOHW_EQ_BAND5,
#define AUDIOHW_HAVE_EQ_BAND5
#endif /* 5 */
#endif /* 4 */
#endif /* 3 */
#endif /* 2 */
    AUDIOHW_EQ_BAND_NUM, /* Keep last */
};

#ifdef AUDIOHW_EQ_FREQUENCY_CAPS
/* One or more bands supports frequency cutoff or center adjustment */
#define AUDIOHW_HAVE_EQ_FREQUENCY
enum
{
#if defined(AUDIOHW_HAVE_EQ_BAND1) && \
        (AUDIOHW_EQ_FREQUENCY_CAPS & (EQ_CAP << 0))
    AUDIOHW_EQ_BAND1_FREQUENCY,
#define AUDIOHW_HAVE_EQ_BAND1_FREQUENCY
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2) && \
        (AUDIOHW_EQ_FREQUENCY_CAPS & (EQ_CAP << 1))
    AUDIOHW_EQ_BAND2_FREQUENCY,
#define AUDIOHW_HAVE_EQ_BAND2_FREQUENCY
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3) && \
        (AUDIOHW_EQ_FREQUENCY_CAPS & (EQ_CAP << 2))
    AUDIOHW_EQ_BAND3_FREQUENCY,
#define AUDIOHW_HAVE_EQ_BAND3_FREQUENCY
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND4) && \
        (AUDIOHW_EQ_FREQUENCY_CAPS & (EQ_CAP << 3))
    AUDIOHW_EQ_BAND4_FREQUENCY,
#define AUDIOHW_HAVE_EQ_BAND4_FREQUENCY
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND5) && \
        (AUDIOHW_EQ_FREQUENCY_CAPS & (EQ_CAP << 4))
    AUDIOHW_EQ_BAND5_FREQUENCY,
#define AUDIOHW_HAVE_EQ_BAND5_FREQUENCY
#endif
    AUDIOHW_EQ_FREQUENCY_NUM, /* Keep last */
};
#endif /* AUDIOHW_EQ_FREQUENCY_CAPS */

#ifdef AUDIOHW_EQ_WIDTH_CAPS
/* One or more bands supports bandwidth adjustment */
#define AUDIOHW_HAVE_EQ_WIDTH
enum
{
#if defined(AUDIOHW_HAVE_EQ_BAND1) && \
        (AUDIOHW_EQ_WIDTH_CAPS & (EQ_CAP << 1))
    AUDIOHW_EQ_BAND2_WIDTH,
#define AUDIOHW_HAVE_EQ_BAND2_WIDTH
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2) && \
        (AUDIOHW_EQ_WIDTH_CAPS & (EQ_CAP << 2))
    AUDIOHW_EQ_BAND3_WIDTH,
#define AUDIOHW_HAVE_EQ_BAND3_WIDTH
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3) && \
        (AUDIOHW_EQ_WIDTH_CAPS & (EQ_CAP << 3))
    AUDIOHW_EQ_BAND4_WIDTH,
#define AUDIOHW_HAVE_EQ_BAND4_WIDTH
#endif
    AUDIOHW_EQ_WIDTH_NUM, /* Keep last */
};
#endif /* AUDIOHW_EQ_WIDTH_CAPS */

/* Types and number of settings types (gain, frequency, width) */
enum AUDIOHW_EQ_SETTINGS
{
    AUDIOHW_EQ_GAIN,
#ifdef AUDIOHW_HAVE_EQ_FREQUENCY
    AUDIOHW_EQ_FREQUENCY,
#endif
#ifdef AUDIOHW_HAVE_EQ_WIDTH
    AUDIOHW_EQ_WIDTH,
#endif
    AUDIOHW_EQ_SETTING_NUM
};

#endif /* (AUDIOHW_CAPS & EQ_CAP) */

#if (AUDIOHW_CAPS & DEPTH_3D_CAP)
#define AUDIOHW_HAVE_DEPTH_3D
#endif

#if (AUDIOHW_CAPS & LINEOUT_CAP)
#define AUDIOHW_HAVE_LINEOUT
#endif

#if (AUDIOHW_CAPS & MONO_VOL_CAP)
#define AUDIOHW_HAVE_MONO_VOLUME
#endif

#ifdef HAVE_RECORDING
#if (AUDIOHW_CAPS & LIN_GAIN_CAP)
#define AUDIOHW_HAVE_LIN_GAIN
#endif

#if (AUDIOHW_CAPS & MIC_GAIN_CAP)
#define AUDIOHW_HAVE_MIC_GAIN
#endif
#endif /* HAVE_RECORDING */

#if (AUDIOHW_CAPS & FILTER_ROLL_OFF_CAP)
#define AUDIOHW_HAVE_FILTER_ROLL_OFF
#endif

#endif /* AUDIOHW_CAPS */

#ifdef HAVE_SW_TONE_CONTROLS
/* Needed for proper sound support */
#define AUDIOHW_HAVE_BASS
#define AUDIOHW_HAVE_TREBLE
#endif /* HAVE_SW_TONE_CONTROLS */

/* Generate enumeration of SOUND_xxx constants */
#include "audiohw_settings.h"

/* All usable functions implemented by a audio codec drivers. Most of
 * the function in sound settings are only called, when in audio codecs
 * .h file suitable defines are added.
 */

/**
 * Initialize audio codec to a well defined state. Includes SoC-specific
 * setup.
 */
void audiohw_init(void);

/**
 * Do initial audio codec setup. Usually called from audiohw_init.
 */
void audiohw_preinit(void);

/**
 * Do some stuff (codec related) after audiohw_init that needs to be
 * delayed such as enabling outputs to prevent popping. This lets
 * other inits in the system complete in the meantime.
 */
void audiohw_postinit(void);

/**
 * Close audio codec.
 */
void audiohw_close(void);

#ifdef AUDIOHW_HAVE_MONO_VOLUME
  /**
 * Set new volume value
 * @param val to set in centibels.
 * NOTE: AUDIOHW_CAPS need to contain
 *          CLIPPING_CAP
 */
void audiohw_set_volume(int val);
#else /* Stereo volume */
/**
 * Set new volume value for each channel
 * @param vol_l sets left channel volume in centibels.
 * @param vol_r sets right channel volume in centibels.
 */
void audiohw_set_volume(int vol_l, int vol_r);
#endif /* AUDIOHW_HAVE_MONO_VOLUME */

#ifdef AUDIOHW_HAVE_LINEOUT
 /**
 * Set new volume value for each channel
 * @param vol_l sets left channel volume
 * @param vol_r sets right channel volume
 */
void audiohw_set_lineout_volume(int vol_l, int vol_r);
#endif

#ifndef AUDIOHW_HAVE_CLIPPING
#if defined(AUDIOHW_HAVE_BASS) || defined(AUDIOHW_HAVE_TREBLE) \
    || defined(AUDIOHW_HAVE_EQ)
/**
 * Set new prescaler value.
 * @param val to set in centibels.
 * NOTE: AUDIOHW_CAPS need to contain
 *          PRESCALER_CAP
 */
void audiohw_set_prescaler(int val);
#endif
#endif /* !AUDIOHW_HAVE_CLIPPING */

#ifdef AUDIOHW_HAVE_BALANCE
/**
 * Set new balance value
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          BALANCE_CAP
 */
void audiohw_set_balance(int val);
#endif

#ifdef AUDIOHW_HAVE_TREBLE
/**
 * Set new treble value.
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          TREBLE_CAP
 */
void audiohw_set_treble(int val);
#endif

#ifdef AUDIOHW_HAVE_BASS
/**
 * Set new bass value.
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          BASS_CAP
 */
void audiohw_set_bass(int val);
#endif

#ifdef AUDIOHW_HAVE_BASS_CUTOFF
/**
 * Set new bass cut off value.
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          BASS_CUTOFF_CAP
 */
void audiohw_set_bass_cutoff(int val);
#endif

#ifdef AUDIOHW_HAVE_TREBLE_CUTOFF
/**
 * Set new treble cut off value.
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          TREBLE_CUTOFF_CAP
 */
void audiohw_set_treble_cutoff(int val);
#endif

#ifdef AUDIOHW_HAVE_EQ
/**
 * Set new band gain value.
 * @param band index to which val is set
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          EQ_CAP
 *
 *       AUDIOHW_EQ_BAND_CAPS must be defined as a bitmask
 *       of EQ_CAP each shifted by the zero-based band number
 *       for each band. Bands 1 to N are indexed 0 to N-1.
 */
void audiohw_set_eq_band_gain(unsigned int band, int val);
#endif

#ifdef AUDIOHW_HAVE_EQ_FREQUENCY
/**
 * Set new band cutoff or center frequency value.
 * @param band index to which val is set
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          EQ_CAP
 *
 *       AUDIOHW_EQ_FREQUENCY_CAPS must be defined as a bitmask
 *       of EQ_CAP each shifted by the zero-based band number
 *       for each band that supports frequency adjustment.
 *       Bands 1 to N are indexed 0 to N-1.
 */
void audiohw_set_eq_band_frequency(unsigned int band, int val);
#endif

#ifdef AUDIOHW_HAVE_EQ_WIDTH
/**
 * Set new band cutoff or center frequency value.
 * @param band index to which val is set
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          EQ_CAP
 *
 *       AUDIOHW_EQ_WIDTH_CAPS must be defined as a bitmask
 *       of EQ_CAP each shifted by the zero-based band number
 *       for each band that supports width adjustment.
 *       Bands 1 to N are indexed 0 to N-1.
 */
void audiohw_set_eq_band_width(unsigned int band, int val);
#endif

#ifdef AUDIOHW_HAVE_DEPTH_3D
/**
 * Set new 3-d enhancement (stereo expansion) effect value.
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          DEPTH_3D_CAP
 */
void audiohw_set_depth_3d(int val);
#endif

#ifdef AUDIOHW_HAVE_FILTER_ROLL_OFF
/**
 * Set DAC's oversampling filter roll-off.
 * @param val 0 - sharp roll-off, 1 - slow roll-off, 2 - short roll-off, 3 - bypass.
 * NOTE: AUDIOHW_CAPS need to contain
 *          FILTER_ROLL_OFF_CAP
 */
void audiohw_set_filter_roll_off(int val);
#endif

void audiohw_set_frequency(int fsel);

#ifdef HAVE_RECORDING

/**
 * Enable recording.
 * @param source_mic if this is true, we want to record from microphone,
 *                   else we want to record FM/LineIn.
 */
void audiohw_enable_recording(bool source_mic);

/**
 * Disable recording.
 */
void audiohw_disable_recording(void);

/**
 * Set gain of recording source.
 * @param left gain value.
 * @param right will not be used if recording from micophone (mono).
 * @param type AUDIO_GAIN_MIC, AUDIO_GAIN_LINEIN.
 */
void audiohw_set_recvol(int left, int right, int type);

#endif /* HAVE_RECORDING */

#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)
/**
 * Enable or disable recording monitor.
 * @param enable true or false.
 */
void audiohw_set_monitor(bool enable);
#endif

/**
 * Set channel configuration.
 * @param val new channel value (see enum below).
 */
enum AUDIOHW_CHANNEL_CONFIG
{
    SOUND_CHAN_STEREO,
    SOUND_CHAN_MONO,
    SOUND_CHAN_CUSTOM,
    SOUND_CHAN_MONO_LEFT,
    SOUND_CHAN_MONO_RIGHT,
    SOUND_CHAN_KARAOKE,
    SOUND_CHAN_NUM_MODES,
};

void audiohw_set_channel(int val);

#ifdef HAVE_PITCHCONTROL
/**
 * Set the pitch ratio
 * @param ratio to set in .01% units
 */
void audiohw_set_pitch(int32_t val);

/**
 * Return the set pitch ratio
 */
int32_t audiohw_get_pitch(void);
#endif /* HAVE_PITCHCONTROL */

/**
 * Set stereo width.
 * @param val new stereo width value.
 */
void audiohw_set_stereo_width(int val);

#ifdef HAVE_SPEAKER
void audiohw_enable_speaker(bool on);
#endif /* HAVE_SPEAKER */

/**
 * Some setting are the same for every codec and can be defined here.
 */
#ifdef HAVE_SW_TONE_CONTROLS
AUDIOHW_SETTING(BASS,        "dB", 0, 1,  -24,  24,   0)
AUDIOHW_SETTING(TREBLE,      "dB", 0, 1,  -24,  24,   0)
#endif /* HAVE_SW_TONE_CONTROLS */
AUDIOHW_SETTING(BALANCE,      "%", 0, 1, -100, 100,   0)
AUDIOHW_SETTING(CHANNELS,      "", 0, 1,    0,   5,   0)
AUDIOHW_SETTING(STEREO_WIDTH, "%", 0, 5,    0, 250, 100)

#endif /* _AUDIOHW_H_ */
