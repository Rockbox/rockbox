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
#ifdef HAVE_SW_VOLUME_CONTROL
#include "system.h"
#include "pcm_sw_volume.h"
#include "sound.h"

static int sound_value_to_cb(int setting, int value)
{
    int shift = 1 - sound_numdecimals(setting);
    if (shift < 0) do { value /= 10; } while (++shift);
    if (shift > 0) do { value *= 10; } while (--shift);
    return value;
}

void audiohw_set_volume(int balance, int volume)
{
    const int minvol = sound_value_to_cb(SOUND_VOLUME, sound_min(SOUND_VOLUME));
    const int maxvol = sound_value_to_cb(SOUND_VOLUME, sound_max(SOUND_VOLUME));
    extern void pcm_set_mixer_volume(int);
    int l = maxvol, r = maxvol;
    pcm_set_mixer_volume(volume);
    if (balance > 0)
    {
        l -= (l - minvol) * balance / 100;
    }
    else if (balance < 0)
    {
        r += (r - minvol) * balance / 100;
    }
    pcm_set_master_volume(l,r);
}
#else

void audiohw_set_volume(int volume)
{
    extern void pcm_set_mixer_volume(int);
    pcm_set_mixer_volume(volume);
}
#endif /* HAVE_SW_VOLUME_CONTROL */

void audiohw_set_balance(int balance)
{
    (void)balance;
}

void audiohw_close(void)
{
    extern void pcm_shutdown(void);
    pcm_shutdown();
}
