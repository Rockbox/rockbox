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
 * Copyright (c) 2018 Marcin Bukat
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
#include "panic.h"
#include "alsa-controls.h"

static int hw_init = 0;

static long int vol_l_hw = 255;
static long int vol_r_hw = 255;

static int muted = -1;

void audiohw_mute(int mute)
{
    if (!hw_init || muted == mute)
       return;

    muted = mute;

    if(mute)
    {
        long int ps0 = 0;
        alsa_controls_set_ints("Output Port Switch", 1, &ps0);
    }
    else
    {
        long int ps2 = 2;
        alsa_controls_set_ints("Output Port Switch", 1, &ps2);
    }
}

void audiohw_preinit(void)
{
    alsa_controls_init("default");
    hw_init = 1;
#if defined(AUDIOHW_MUTE_ON_STOP) || defined(AUDIOHW_NEEDS_INITIAL_UNMUTE)
    audiohw_mute(true);  /* Start muted to avoid the POP */
#else
    audiohw_mute(false);
#endif
}

void audiohw_postinit(void)
{
}

void audiohw_close(void)
{
    hw_init = 0;
    muted = -1;
    alsa_controls_close();
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    vol_l_hw = -vol_l/5;
    vol_r_hw = -vol_r/5;

    if (!hw_init)
       return;

    alsa_controls_set_ints("Left Playback Volume", 1, &vol_l_hw);
    alsa_controls_set_ints("Right Playback Volume", 1, &vol_r_hw);
}
