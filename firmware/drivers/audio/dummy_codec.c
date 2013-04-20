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

void audiohw_preinit(void) { }

void audiohw_postinit(void) { }

void audiohw_close(void) { }

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

#ifdef HAVE_SW_VOLUME_CONTROL
void audiohw_set_volume(int vol_l, int vol_r)
{
    /* SW volume for <= 1.0 gain, HW at unity, <= DUMMY_VOLUME_MIN == MUTE */
    int sw_volume_l = vol_l <= DUMMY_VOLUME_MIN ? PCM_MUTE_LEVEL : vol_l;
    int sw_volume_r = vol_r <= DUMMY_VOLUME_MIN ? PCM_MUTE_LEVEL : vol_r;
    pcm_set_master_volume(sw_volume_l, sw_volume_r);
}
#endif /* HAVE_SW_VOLUME_CONTROL */
