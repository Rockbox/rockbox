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


#include "config.h"
#include "audiohw.h"

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
};


void audiohw_set_volume(int volume)
{
    extern void pcm_set_mixer_volume(int);
    pcm_set_mixer_volume(volume);
}

void audiohw_set_balance(int balance)
{
    (void)balance;
}
