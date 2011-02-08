/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2010 Thomas Martitz
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

#include <SDL_audio.h>
#include "config.h"
#include "audiohw.h"
#include "pcm_sampr.h"

/**
 * Audio Hardware api. Make them do nothing as we cannot properly simulate with
 * SDL. if we used DSP we would run code that doesn't actually run on the target
 **/

extern void pcm_set_mixer_volume(int);

void audiohw_set_volume(int volume)
{
#if CONFIG_CODEC == SWCODEC
#if (CONFIG_PLATFORM & PLATFORM_MAEMO5)
    pcm_set_mixer_volume(volume);
#else
    pcm_set_mixer_volume(
        SDL_MIX_MAXVOLUME * ((volume - VOLUME_MIN) / 10) / (VOLUME_RANGE / 10));
#endif /* (CONFIG_PLATFORM & PLATFORM_MAEMO) */
#else
    (void)volume;
#endif /* CONFIG_CODEC == SWCODEC */
}

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, VOLUME_MIN / 10, VOLUME_MAX / 10, -25},
/* Bass and treble tone controls */
#ifdef AUDIOHW_HAVE_BASS
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
#endif
#ifdef AUDIOHW_HAVE_TREBLE
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
#endif
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#if defined(HAVE_RECORDING)
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,-128,  96,   0},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,-128,  96,   0},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,-128, 108,  16},
#endif
#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
    [SOUND_BASS_CUTOFF]   = {"",   0,  1,   1,   4,   1},
#endif
#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
    [SOUND_TREBLE_CUTOFF] = {"",   0,  1,   1,   4,   1},
#endif
#if defined(AUDIOHW_HAVE_DEPTH_3D)
    [SOUND_DEPTH_3D]      = {"%",  0,   1,   0,  15,   0},
#endif
/* Hardware EQ tone controls */
#if defined(AUDIOHW_HAVE_EQ_BAND1)
    [SOUND_EQ_BAND1_GAIN] = {"dB", 0,  1, -12,  12,   0},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2)
    [SOUND_EQ_BAND2_GAIN] = {"dB", 0,  1, -12,  12,   0},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3)
    [SOUND_EQ_BAND3_GAIN] = {"dB", 0,  1, -12,  12,   0},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND4)
    [SOUND_EQ_BAND4_GAIN] = {"dB", 0,  1, -12,  12,   0},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND5)
    [SOUND_EQ_BAND5_GAIN] = {"dB", 0,  1, -12,  12,   0},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND1_FREQUENCY)
    [SOUND_EQ_BAND1_FREQUENCY] = {"",   0,  1,   1,   4,   1},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2_FREQUENCY)
    [SOUND_EQ_BAND2_FREQUENCY] = {"",   0,  1,   1,   4,   1},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3_FREQUENCY)
    [SOUND_EQ_BAND3_FREQUENCY] = {"",   0,  1,   1,   4,   1},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND4_FREQUENCY)
    [SOUND_EQ_BAND4_FREQUENCY] = {"",   0,  1,   1,   4,   1},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND5_FREQUENCY)
    [SOUND_EQ_BAND5_FREQUENCY] = {"",   0,  1,   1,   4,   1},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2_WIDTH)
    [SOUND_EQ_BAND2_WIDTH]     = {"",   0,  1,   0,   1,   0},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3_WIDTH)
    [SOUND_EQ_BAND3_WIDTH]     = {"",   0,  1,   0,   1,   0},
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND4_WIDTH)
    [SOUND_EQ_BAND4_WIDTH]     = {"",   0,  1,   0,   1,   0},
#endif

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_LOUDNESS]      = {"dB", 0,  1,   0,  17,   0},
    [SOUND_AVC]           = {"",   0,  1,  -1,   4,   0},
    [SOUND_MDB_STRENGTH]  = {"dB", 0,  1,   0, 127,  48},
    [SOUND_MDB_HARMONICS] = {"%",  0,  1,   0, 100,  50},
    [SOUND_MDB_CENTER]    = {"Hz", 0, 10,  20, 300,  60},
    [SOUND_MDB_SHAPE]     = {"Hz", 0, 10,  50, 300,  90},
    [SOUND_MDB_ENABLE]    = {"",   0,  1,   0,   1,   0},
    [SOUND_SUPERBASS]     = {"",   0,  1,   0,   1,   0},
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
};

/**
 * stubs here, for the simulator
 **/

#if defined(AUDIOHW_HAVE_PRESCALER)
void audiohw_set_prescaler(int value)   { (void)value; }
#endif
#if defined(AUDIOHW_HAVE_BALANCE)
void audiohw_set_balance(int value)     { (void)value; }
#endif
#if defined(AUDIOHW_HAVE_BASS)
void audiohw_set_bass(int value)        { (void)value; }
#endif
#if defined(AUDIOHW_HAVE_TREBLE)
void audiohw_set_treble(int value)      { (void)value; }
#endif
#if CONFIG_CODEC != SWCODEC
void audiohw_set_channel(int value)     { (void)value; }
void audiohw_set_stereo_width(int value){ (void)value; }
#endif
#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
void audiohw_set_bass_cutoff(int value) { (void)value; }
#endif
#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
void audiohw_set_treble_cutoff(int value){ (void)value; }
#endif
/* EQ-based tone controls */
#if defined(AUDIOHW_HAVE_EQ)
void audiohw_set_eq_band_gain(unsigned int band, int value)
    { (void)band; (void)value; }
#endif
#if defined(AUDIOHW_HAVE_EQ_FREQUENCY)
void audiohw_set_eq_band_frequency(unsigned int band, int value)
    { (void)band; (void)value; }
#endif
#if defined(AUDIOHW_HAVE_EQ_WIDTH)
void audiohw_set_eq_band_width(unsigned int band, int value)
    { (void)band; (void)value; }
#endif
#if defined(AUDIOHW_HAVE_DEPTH_3D)
void audiohw_set_depth_3d(int value)
    { (void)value; }
#endif
#ifdef HAVE_RECORDING
#if SAMPR_TYPE_REC != 0
unsigned int pcm_sampr_type_rec_to_play(unsigned int samplerate)
    { return samplerate; }
#endif
#endif /* HAVE_RECORDING */
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
int mas_codec_readreg(int reg)
{
    (void)reg;
    return 0;
}

int mas_codec_writereg(int reg, unsigned int val)
{
    (void)reg;
    (void)val;
    return 0;
}
int mas_writemem(int bank, int addr, const unsigned long* src, int len)
{
    (void)bank;
    (void)addr;
    (void)src;
    (void)len;
    return 0;
}
#endif
