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
 * Copyright (C) 2013 by Michael Sevakis
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
 /* Indicate it's the sound.c file which affects compilation of audiohw.h */
#define AUDIOHW_IS_SOUND_C
#include "config.h"
#include "system.h"
#include "sound.h"
#ifdef HAVE_SW_VOLUME_CONTROL
#include "pcm_sw_volume.h"
#endif /* HAVE_SW_VOLUME_CONTROL */

/* Define sound_setting_entries table */
#define AUDIOHW_SOUND_SETTINGS_ENTRIES
#include "audiohw_settings.h"

/* Implements sound_val2phys */
#define AUDIOHW_SOUND_SETTINGS_VAL2PHYS
#include "audiohw_settings.h"

extern bool audio_is_initialized;

static const struct sound_setting_entry * get_setting_entry(int setting)
{
    static const struct sound_settings_info default_info =
        { "", 0, 0, 0, 0, 0 };

    static const struct sound_setting_entry default_entry =
        { &default_info, NULL }; 

    if ((unsigned)setting >= ARRAYLEN(sound_setting_entries))
        return &default_entry;

    const struct sound_setting_entry *e = &sound_setting_entries[setting];
    return e->info ? e : &default_entry; /* setting valid but not in table? */
}

static const struct sound_settings_info * get_settings_info(int setting)
{
    return get_setting_entry(setting)->info;
}

const char * sound_unit(int setting)
{
    return get_settings_info(setting)->unit;
}

int sound_numdecimals(int setting)
{
    return get_settings_info(setting)->numdecimals;
}

int sound_steps(int setting)
{
    return get_settings_info(setting)->steps;
}

int sound_min(int setting)
{
    return get_settings_info(setting)->minval;
}

int sound_max(int setting)
{
    return get_settings_info(setting)->maxval;
}

int sound_default(int setting)
{
    return get_settings_info(setting)->defaultval;
}

sound_set_type * sound_get_fn(int setting)
{
    return get_setting_entry(setting)->function;
}

void sound_set(int setting, int value)
{
    sound_set_type *sound_set_val = sound_get_fn(setting);

    if (sound_set_val)
        sound_set_val(value);
}

#if !defined(AUDIOHW_HAVE_CLIPPING)
/*
 * The prescaler compensates for any kind of boosts, to prevent clipping.
 *
 * It's basically just a measure to make sure that audio does not clip during
 * tone controls processing, like if i want to boost bass 12 dB, i can decrease
 * the audio amplitude by -12 dB before processing, then increase master gain
 * by 12 dB after processing.
 */

static int current_volume  = 0; /* tenth dB */
static int current_balance = 0; /* percent  */
#ifdef AUDIOHW_HAVE_TREBLE
static int current_treble  = 0; /* tenth dB */
#endif
#ifdef AUDIOHW_HAVE_BASS
static int current_bass    = 0; /* tenth dB */
#endif
#ifdef AUDIOHW_HAVE_EQ
static int current_eq_band_gain[AUDIOHW_EQ_BAND_NUM]; /* tenth dB */
#endif

/* Return the sound value scaled to centibels (tenth-decibels) */
static int sound_value_to_cb(int setting, int value)
{
    int shift = 1 - sound_numdecimals(setting);
    if (shift < 0) do { value /= 10; } while (++shift);
    if (shift > 0) do { value *= 10; } while (--shift);
    return value;
}

static void set_prescaled_volume(void)
{
    int prescale = 0;

#if defined(AUDIOHW_HAVE_BASS) || defined(AUDIOHW_HAVE_TREBLE) \
    || defined(AUDIOHW_HAVE_EQ)

    /* Note: Having Tone + EQ isn't prohibited */
#if defined(AUDIOHW_HAVE_BASS) && defined(AUDIOHW_HAVE_TREBLE)
    prescale = MAX(current_bass, current_treble);
#endif
#if defined(AUDIOHW_HAVE_EQ)
    for (int i = 0; i < AUDIOHW_EQ_BAND_NUM; i++)
        prescale = MAX(current_eq_band_gain[i], prescale);
#endif

    if (prescale < 0)
        prescale = 0;  /* no need to prescale if we don't boost
                          bass, treble or eq band */

    /* Gain up the analog volume to compensate the prescale gain reduction,
     * but if this would push the volume over the top, reduce prescaling
     * instead (might cause clipping). */
    if (current_volume + prescale > VOLUME_MAX)
        prescale = VOLUME_MAX - current_volume;

    audiohw_set_prescaler(prescale);

    if (current_volume < VOLUME_MIN)
        prescale = 0;  /* Make sure the audio gets muted */
#endif /* AUDIOHW_HAVE_BASS || AUDIOHW_HAVE_TREBLE || AUDIOHW_HAVE_EQ */

#if defined(AUDIOHW_HAVE_MONO_VOLUME)
    audiohw_set_volume(current_volume);
#else /* Stereo volume */
    int l = current_volume + prescale, r = l;

    /* Balance the channels scaled by the current volume and min volume. */
    /* Subtract a dB from VOLUME_MIN to get it to a mute level */
    int volshift = current_balance * VOLUME_RANGE / 100; /* tenth of dB */

    if (volshift > 0)
    {
        l -= ((l - (VOLUME_MIN - ONE_DB)) * volshift) / VOLUME_RANGE;
    }
    else if (volshift < 0)
    {
        r += ((r - (VOLUME_MIN - ONE_DB)) * volshift) / VOLUME_RANGE;
    }

    audiohw_set_volume(l, r);
#endif /* AUDIOHW_HAVE_MONO_VOLUME */

#if defined(AUDIOHW_HAVE_LINEOUT)
    /* For now, lineout stays at unity */
    audiohw_set_lineout_volume(0, 0);
#endif /* AUDIOHW_HAVE_LINEOUT */

    (void)prescale; /* In case of no tone controls + mono volume */
}
#endif /* AUDIOIHW_HAVE_CLIPPING */

void sound_set_volume(int value)
{
    if (!audio_is_initialized)
        return;

#if defined(AUDIOHW_HAVE_CLIPPING)
    audiohw_set_volume(value);
#else
    current_volume = sound_value_to_cb(SOUND_VOLUME, value);
    set_prescaled_volume();
#endif
}

void sound_set_balance(int value)
{
    if (!audio_is_initialized)
        return;

#if defined(AUDIOHW_HAVE_BALANCE)
    audiohw_set_balance(value);
#else
    current_balance = value;
    set_prescaled_volume();
#endif
}

#ifdef AUDIOHW_HAVE_BASS
void sound_set_bass(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_bass(value);

#if !defined(AUDIOHW_HAVE_CLIPPING)
    current_bass = sound_value_to_cb(SOUND_BASS, value);
    set_prescaled_volume();
#endif
}
#endif /* AUDIOHW_HAVE_BASS */

#ifdef AUDIOHW_HAVE_TREBLE
void sound_set_treble(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_treble(value);

#if !defined(AUDIOHW_HAVE_CLIPPING)
    current_treble = sound_value_to_cb(SOUND_TREBLE, value);
    set_prescaled_volume();
#endif
}
#endif /* AUDIOHW_HAVE_TREBLE */

#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
void sound_set_bass_cutoff(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_bass_cutoff(value);
}
#endif

#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
void sound_set_treble_cutoff(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_treble_cutoff(value);
}
#endif

void sound_set_channels(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_channel(value);
}

void sound_set_stereo_width(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_stereo_width(value);
}

#if defined(AUDIOHW_HAVE_DEPTH_3D)
void sound_set_depth_3d(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_depth_3d(value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ)
int sound_enum_hw_eq_band_setting(unsigned int band,
                                  unsigned int band_setting)
{
    static const int8_t
    sound_hw_eq_band_setting[AUDIOHW_EQ_SETTING_NUM][AUDIOHW_EQ_BAND_NUM] =
    {
        [AUDIOHW_EQ_GAIN] =
        {
            [0 ... AUDIOHW_EQ_BAND_NUM-1] = -1,
            [AUDIOHW_EQ_BAND1] = SOUND_EQ_BAND1_GAIN,
        #ifdef AUDIOHW_HAVE_EQ_BAND2
            [AUDIOHW_EQ_BAND2] = SOUND_EQ_BAND2_GAIN,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND3
            [AUDIOHW_EQ_BAND3] = SOUND_EQ_BAND3_GAIN,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND4
            [AUDIOHW_EQ_BAND4] = SOUND_EQ_BAND4_GAIN,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND5
            [AUDIOHW_EQ_BAND5] = SOUND_EQ_BAND5_GAIN,
        #endif
        },
#ifdef AUDIOHW_HAVE_EQ_FREQUENCY
        [AUDIOHW_EQ_FREQUENCY] =
        {
            [0 ... AUDIOHW_EQ_BAND_NUM-1] = -1,
        #ifdef AUDIOHW_HAVE_EQ_BAND1_FREQUENCY
            [AUDIOHW_EQ_BAND1] = SOUND_EQ_BAND1_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND2_FREQUENCY
            [AUDIOHW_EQ_BAND2] = SOUND_EQ_BAND2_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND3_FREQUENCY
            [AUDIOHW_EQ_BAND3] = SOUND_EQ_BAND3_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND4_FREQUENCY
            [AUDIOHW_EQ_BAND4] = SOUND_EQ_BAND4_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND5_FREQUENCY
            [AUDIOHW_EQ_BAND5] = SOUND_EQ_BAND5_FREQUENCY,
        #endif
        },
#endif /* AUDIOHW_HAVE_EQ_FREQUENCY */
#ifdef AUDIOHW_HAVE_EQ_WIDTH
        [AUDIOHW_EQ_WIDTH] =
        {
            [0 ... AUDIOHW_EQ_BAND_NUM-1] = -1,
        #ifdef AUDIOHW_HAVE_EQ_BAND2_WIDTH
            [AUDIOHW_EQ_BAND2] = SOUND_EQ_BAND2_WIDTH,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND3_WIDTH
            [AUDIOHW_EQ_BAND3] = SOUND_EQ_BAND3_WIDTH,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND4_WIDTH
            [AUDIOHW_EQ_BAND4] = SOUND_EQ_BAND4_WIDTH,
        #endif
        },
#endif /* AUDIOHW_HAVE_EQ_WIDTH */
    };

    if (band < AUDIOHW_EQ_BAND_NUM && band_setting < AUDIOHW_EQ_SETTING_NUM)
        return sound_hw_eq_band_setting[band_setting][band];

    return -1;
}

static void sound_set_hw_eq_band_gain(unsigned int band, int value)
{
    if (!audio_is_initialized)
        return;

    int setting = sound_enum_hw_eq_band_setting(band, AUDIOHW_EQ_GAIN);
    current_eq_band_gain[band] = sound_value_to_cb(setting, value);

    audiohw_set_eq_band_gain(band, value);
    set_prescaled_volume();
}

void sound_set_hw_eq_band1_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND1, value);
}

#if defined(AUDIOHW_HAVE_EQ_BAND2)
void sound_set_hw_eq_band2_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND2, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND3)
void sound_set_hw_eq_band3_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND3, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND4)
void sound_set_hw_eq_band4_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND4, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND5)
void sound_set_hw_eq_band5_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND5, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND1_FREQUENCY)
void sound_set_hw_eq_band1_frequency(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND1, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND2_FREQUENCY)
void sound_set_hw_eq_band2_frequency(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND2, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND3_FREQUENCY)
void sound_set_hw_eq_band3_frequency(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND3, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND4_FREQUENCY)
void sound_set_hw_eq_band4_frequency(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND4, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND5_FREQUENCY)
void sound_set_hw_eq_band5_frequency(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND5, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND2_WIDTH)
void sound_set_hw_eq_band2_width(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_width(AUDIOHW_EQ_BAND2, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND3_WIDTH)
void sound_set_hw_eq_band3_width(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_width(AUDIOHW_EQ_BAND3, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND4_WIDTH)
void sound_set_hw_eq_band4_width(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_eq_band_width(AUDIOHW_EQ_BAND4, value);
}
#endif
#endif /* AUDIOHW_HAVE_EQ */

#if CONFIG_CODEC == MAS3587F || CONFIG_CODEC == MAS3539F
void sound_set_loudness(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_loudness(value);
}

void sound_set_avc(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_avc(value);
}

void sound_set_mdb_strength(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_mdb_strength(value);
}

void sound_set_mdb_harmonics(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_mdb_harmonics(value);
}

void sound_set_mdb_center(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_mdb_center(value);
}

void sound_set_mdb_shape(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_mdb_shape(value);
}

void sound_set_mdb_enable(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_mdb_enable(value);
}

void sound_set_superbass(int value)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_superbass(value);
}
#endif /* CONFIG_CODEC == MAS3587F || CONFIG_CODEC == MAS3539F */

#ifdef HAVE_PITCHCONTROL
void sound_set_pitch(int32_t pitch)
{
    if (!audio_is_initialized)
        return;

    audiohw_set_pitch(pitch);
}

int32_t sound_get_pitch(void)
{
    if (!audio_is_initialized)
        return PITCH_SPEED_100;

    return audiohw_get_pitch();
}
#endif /* HAVE_PITCHCONTROL */
