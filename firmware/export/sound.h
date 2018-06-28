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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SOUND_H
#define SOUND_H

#include <inttypes.h>
#include <audiohw.h>

typedef void sound_set_type(int value);

const char *sound_unit(int setting);
int sound_numdecimals(int setting);
int sound_steps(int setting);
int sound_min(int setting);
int sound_max(int setting);
int sound_default(int setting);
sound_set_type* sound_get_fn(int setting);

void sound_set_volume(int value);
void sound_set_balance(int value);
void sound_set_bass(int value);
void sound_set_treble(int value);
void sound_set_channels(int value);
void sound_set_stereo_width(int value);
#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
void sound_set_bass_cutoff(int value);
#endif
#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
void sound_set_treble_cutoff(int value);
#endif

#if defined(AUDIOHW_HAVE_DEPTH_3D)
void sound_set_depth_3d(int value);
#endif

#if defined(AUDIOHW_HAVE_FILTER_ROLL_OFF)
void sound_set_filter_roll_off(int value);
#endif

#if defined(AUDIOHW_HAVE_FUNCTIONAL_MODE)
void sound_set_functional_mode(int value);
#endif

#ifdef AUDIOHW_HAVE_EQ
/*
 * band = SOUND_EQ_BANDb
 * band_setting = AUDIOHW_EQ_s
 *
 * Returns SOUND_EQ_BANDb_s or -1 if it doesn't exist.
 *
 * b: band number
 * s: one of GAIN, FREQUENCY, WIDTH
 */
int sound_enum_hw_eq_band_setting(unsigned int band,
                                  unsigned int band_setting);
/* Band1 implied */
void sound_set_hw_eq_band1_gain(int value);
#ifdef AUDIOHW_HAVE_EQ_BAND1_FREQUENCY
void sound_set_hw_eq_band1_frequency(int value);
#endif
#ifdef AUDIOHW_HAVE_EQ_BAND2
void sound_set_hw_eq_band2_gain(int value);
#ifdef AUDIOHW_HAVE_EQ_BAND2_FREQUENCY
void sound_set_hw_eq_band2_frequency(int value);
#endif
#ifdef AUDIOHW_HAVE_EQ_BAND2_WIDTH
void sound_set_hw_eq_band2_width(int value);
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND2 */
#ifdef AUDIOHW_HAVE_EQ_BAND3
/* Band 3 */
void sound_set_hw_eq_band3_gain(int value);
#ifdef AUDIOHW_HAVE_EQ_BAND3_FREQUENCY
void sound_set_hw_eq_band3_frequency(int value);
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3_WIDTH)
void sound_set_hw_eq_band3_width(int value);
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND3 */
#ifdef AUDIOHW_HAVE_EQ_BAND4
void sound_set_hw_eq_band4_gain(int value);
#ifdef AUDIOHW_HAVE_EQ_BAND4_FREQUENCY
void sound_set_hw_eq_band4_frequency(int value);
#endif
#ifdef AUDIOHW_HAVE_EQ_BAND4_WIDTH
void sound_set_hw_eq_band4_width(int value);
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND4 */
#ifdef AUDIOHW_HAVE_EQ_BAND5
void sound_set_hw_eq_band5_gain(int value);
#ifdef AUDIOHW_HAVE_EQ_BAND5_FREQUENCY
void sound_set_hw_eq_band5_frequency(int value);
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND5 */
#endif /* AUDIOHW_HAVE_EQ */

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void sound_set_loudness(int value);
void sound_set_avc(int value);
void sound_set_mdb_strength(int value);
void sound_set_mdb_harmonics(int value);
void sound_set_mdb_center(int value);
void sound_set_mdb_shape(int value);
void sound_set_mdb_enable(int value);
void sound_set_superbass(int value);
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */

void sound_set(int setting, int value);
int sound_val2phys(int setting, int value);

void sound_set_pitch(int32_t pitch);
int32_t sound_get_pitch(void);

#ifdef HAVE_PITCHCONTROL
/* precision of the pitch and speed variables */
/* One zero per decimal (100 means two decimal places */
#define PITCH_SPEED_PRECISION 100L
#define PITCH_SPEED_100 (100L * PITCH_SPEED_PRECISION)  /* 100% speed */
#endif /* HAVE_PITCHCONTROL */

#endif
