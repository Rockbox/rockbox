/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2011 Andrew Ryabinin
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
#include "audio.h"
#include "audiohw.h"
#include "system.h"
#include "pcm_sw_volume.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1,   VOLUME_MIN/10,   VOLUME_MAX/10,   0},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
};

void audiohw_preinit(void) { }

void audiohw_postinit(void) { }

void audiohw_close(void) { }

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

#ifdef HAVE_SW_VOLUME_CONTROL
void audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* SW volume for <= 1.0 gain, HW at unity, < VOLUME_MIN == MUTE */
    int sw_volume_l = vol_l < VOLUME_MIN ? PCM_MUTE_LEVEL : MIN(vol_l, 0);
    int sw_volume_r = vol_r < VOLUME_MIN ? PCM_MUTE_LEVEL : MIN(vol_r, 0);
    pcm_set_master_volume(sw_volume_l, sw_volume_r);
}
#endif /* HAVE_SW_VOLUME_CONTROL */
