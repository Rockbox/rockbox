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


#if CONFIG_CODEC == SWCODEC
enum {
    DSP_CALLBACK_SET_PRESCALE = 0,
    DSP_CALLBACK_SET_BASS,
    DSP_CALLBACK_SET_TREBLE,
    DSP_CALLBACK_SET_CHANNEL_CONFIG,
    DSP_CALLBACK_SET_STEREO_WIDTH,
#ifdef HAVE_SW_VOLUME_CONTROL
    DSP_CALLBACK_SET_SW_VOLUME,
#endif
};
#endif

typedef void sound_set_type(int value);

const char *sound_unit(int setting);
int sound_numdecimals(int setting);
int sound_steps(int setting);
int sound_min(int setting);
int sound_max(int setting);
int sound_default(int setting);
sound_set_type* sound_get_fn(int setting);

void sound_set_dsp_callback(int (*func)(int, intptr_t));
void sound_set_volume(int value);
void sound_set_balance(int value);
void sound_set_bass(int value);
void sound_set_treble(int value);
void sound_set_channels(int value);
void sound_set_stereo_width(int value);
#if defined(HAVE_WM8758) || defined(HAVE_WM8985)
void sound_set_bass_cutoff(int value);
void sound_set_treble_cutoff(int value);
#endif
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

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void sound_set_pitch(int32_t pitch);
int32_t sound_get_pitch(void);
#endif

#ifdef HAVE_PITCHSCREEN
/* precision of the pitch and speed variables */
/* One zero per decimal (100 means two decimal places */
#define PITCH_SPEED_PRECISION 100L
#define PITCH_SPEED_100 (100L * PITCH_SPEED_PRECISION)  /* 100% speed */
#endif /* HAVE_PITCHSCREEN */

#endif
