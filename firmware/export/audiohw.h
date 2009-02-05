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

/* define some audiohw caps */
#define TREBLE_CAP          (1 << 0)
#define BASS_CAP            (1 << 1)
#define BALANCE_CAP         (1 << 2)
#define CLIPPING_CAP        (1 << 3)
#define PRESCALER_CAP       (1 << 4)
#define BASS_CUTOFF_CAP     (1 << 5)
#define TREBLE_CUTOFF_CAP   (1 << 6)

#ifdef HAVE_UDA1380
#include "uda1380.h"
#elif defined(HAVE_WM8751)
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
#elif defined(HAVE_MAS35XX)
#include "mas35xx.h"
#elif defined(HAVE_TSC2100)
#include "tsc2100.h"
#endif

/* convert caps into defines */
#ifdef AUDIOHW_CAPS
#if (AUDIOHW_CAPS & TREBLE_CAP)
#define AUDIOHW_HAVE_TREBLE
#endif

#if (AUDIOHW_CAPS & BASS_CAP)
#define AUDIOHW_HAVE_BASS
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

#if (AUDIOHW_CAPS & BASS_CUTOFF_CAP)
#define AUDIOHW_HAVE_BASS_CUTOFF
#endif

#if (AUDIOHW_CAPS & TREBLE_CUTOFF_CAP)
#define AUDIOHW_HAVE_TREBLE_CUTOFF
#endif
#endif /* AUDIOHW_CAPS */

enum {
    SOUND_VOLUME = 0,
    SOUND_BASS,
    SOUND_TREBLE,
    SOUND_BALANCE,
    SOUND_CHANNELS,
    SOUND_STEREO_WIDTH,
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    SOUND_LOUDNESS,
    SOUND_AVC,
    SOUND_MDB_STRENGTH,
    SOUND_MDB_HARMONICS,
    SOUND_MDB_CENTER,
    SOUND_MDB_SHAPE,
    SOUND_MDB_ENABLE,
    SOUND_SUPERBASS,
#endif
#if defined(HAVE_RECORDING)
    SOUND_LEFT_GAIN,
    SOUND_RIGHT_GAIN,
    SOUND_MIC_GAIN,
#endif
#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
    SOUND_BASS_CUTOFF,
#endif
#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
    SOUND_TREBLE_CUTOFF,
#endif
    SOUND_LAST_SETTING, /* Keep this last */
};

enum Channel {
    SOUND_CHAN_STEREO,
    SOUND_CHAN_MONO,
    SOUND_CHAN_CUSTOM,
    SOUND_CHAN_MONO_LEFT,
    SOUND_CHAN_MONO_RIGHT,
    SOUND_CHAN_KARAOKE,
    SOUND_CHAN_NUM_MODES,
};

struct sound_settings_info {
    const char *unit;
    char numdecimals;
    char steps;
    short minval;
    short maxval;
    short defaultval;
};

/* This struct is used by every driver to export its min/max/default values for
 * its audio settings. Keep in mind that the order must be correct! */
extern const struct sound_settings_info audiohw_settings[];

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

#ifdef AUDIOHW_HAVE_CLIPPING
 /**
 * Set new volume value
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          CLIPPING_CAP
 */
void audiohw_set_volume(int val);
#endif

#ifdef AUDIOHW_HAVE_PRESCALER
/**
 * Set new prescaler value.
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          PRESCALER_CAP
 */
void audiohw_set_prescaler(int val);
#endif

#ifdef AUDIOHW_HAVE_BALANCE
/**
 * Set new balance value
 * @param val to set.
 * NOTE: AUDIOHW_CAPS need to contain
 *          BALANCE_CAP
 */
void audiohw_set_balance(int val);
#endif

/**
 * Mute or enable sound.
 * @param mute true or false.
 */
void audiohw_mute(bool mute);

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

#endif /*HAVE_RECORDING*/

#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)

/**
 * Enable or disable recording monitor.
 * @param enable ture or false.
 */
void audiohw_set_monitor(bool enable);

#endif /* HAVE_RECORDING || HAVE_FMRADIO_IN */

#if CONFIG_CODEC != SWCODEC

/* functions which are only used by mas35xx codecs, but are also
   aviable on SWCODECS through dsp */

/**
 * Set channel configuration.
 * @param val new channel value (see enum Channel).
 */
void audiohw_set_channel(int val);

/**
 * Set stereo width.
 * @param val new stereo width value.
 */
void audiohw_set_stereo_width(int val);

#endif /* CONFIG_CODEC != SWCODEC */

#endif /* _AUDIOHW_H_ */
