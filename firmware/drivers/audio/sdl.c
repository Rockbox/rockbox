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
#include "sound.h"
#include "pcm_sampr.h"
#ifdef HAVE_SW_VOLUME_CONTROL
#include "pcm_sw_volume.h"
#include "fixedpoint.h"
#endif

/**
 * Audio Hardware api. Make some of them do nothing as we cannot properly
 * simulate with SDL. if we used DSP we would run code that doesn't actually
 * run on the target
 **/
#ifdef HAVE_SW_VOLUME_CONTROL
static int sdl_volume_level(int volume)
{
    int shift = (1 - sound_numdecimals(SOUND_VOLUME)) << 16;
    int minvol = fp_mul(sound_min(SOUND_VOLUME), fp_exp10(shift, 16), 16);
    return volume <= minvol ? INT_MIN : volume;
}
#endif /* HAVE_SW_VOLUME_CONTROL */

#if defined(AUDIOHW_HAVE_MONO_VOLUME)
void audiohw_set_volume(int volume)
{
#ifdef HAVE_SW_VOLUME_CONTROL
    volume = sdl_volume_level(volume);
    pcm_set_master_volume(volume, volume);
#elif CONFIG_CODEC == SWCODEC
    extern void pcm_set_mixer_volume(int volume);
    pcm_set_mixer_volume(volume);
#endif
    (void)volume;
}
#else /* !AUDIOHW_HAVE_MONO_VOLUME */
void audiohw_set_volume(int vol_l, int vol_r)
{
#ifdef HAVE_SW_VOLUME_CONTROL
    vol_l = sdl_volume_level(vol_l);
    vol_r = sdl_volume_level(vol_r);
    pcm_set_master_volume(vol_l, vol_r);
#endif
    (void)vol_l; (void)vol_r;
}
#endif /* AUDIOHW_HAVE_MONO_VOLUME */

#if defined(AUDIOHW_HAVE_PRESCALER)
void audiohw_set_prescaler(int value)
{
#ifdef HAVE_SW_VOLUME_CONTROL
    pcm_set_prescaler(value);
#endif
    (void)value;
}
#endif /* AUDIOHW_HAVE_PRESCALER */

#if defined(AUDIOHW_HAVE_BALANCE)
void audiohw_set_balance(int value)     { (void)value; }
#endif
#ifndef HAVE_SW_TONE_CONTROLS
#if defined(AUDIOHW_HAVE_BASS)
void audiohw_set_bass(int value)        { (void)value; }
#endif
#if defined(AUDIOHW_HAVE_TREBLE)
void audiohw_set_treble(int value)      { (void)value; }
#endif
#endif /* HAVE_SW_TONE_CONTROLS */
#if CONFIG_CODEC != SWCODEC
void audiohw_set_channel(int value)     { (void)value; }
void audiohw_set_stereo_width(int value){ (void)value; }
#ifdef HAVE_PITCHCONTROL
void audiohw_set_pitch(int32_t value)   { (void)value; }
int32_t audiohw_get_pitch(void)         { return PITCH_SPEED_100; }
#endif
#endif /* CONFIG_CODEC != SWCODEC */
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
#if defined(AUDIOHW_HAVE_LINEOUT)
void audiohw_set_lineout_volume(int vol_l, int vol_r)
    { (void)vol_l; (void)vol_r; }
#endif
#if defined(AUDIOHW_HAVE_FILTER_ROLL_OFF)
void audiohw_set_filter_roll_off(int value)
    { (void)value; }
#endif
#if defined(AUDIOHW_HAVE_FUNCTIONAL_MODE)
void audiohw_set_functional_mode(int value)
    { (void)value; }
#endif

void audiohw_close(void) {}

#ifdef CONFIG_SAMPR_TYPES
unsigned int pcm_sampr_to_hw_sampr(unsigned int samplerate,
                                   unsigned int type)
    { return samplerate; (void)type; }
#endif /* CONFIG_SAMPR_TYPES */
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
