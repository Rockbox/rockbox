/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include "debug.h"
#include "pcm_sw_volume.h"
#include "settings.h"

#include "debug-ibasso.h"
#include "pcm-ibasso.h"


void audiohw_close(void)
{
    TRACE;

    pcm_close_device();
}


void set_software_volume(void)
{
    /* -73dB (?) minimum software volume in decibels. See pcm-internal.h. */
    static const int SW_VOLUME_MIN = 730;

    int sw_volume_l = 0;
    int sw_volume_r = 0;

    if(global_settings.balance > 0)
    {
        if(global_settings.balance == 100)
        {
            sw_volume_l = PCM_MUTE_LEVEL;
        }
        else
        {
            sw_volume_l -= SW_VOLUME_MIN * global_settings.balance / 100;
        }
    }
    else if(global_settings.balance < 0)
    {
        if(global_settings.balance == -100)
        {
            sw_volume_r = PCM_MUTE_LEVEL;
        }
        else
        {
            sw_volume_r = SW_VOLUME_MIN * global_settings.balance / 100;
        }
    }

    DEBUGF("DEBUG %s: global_settings.balance: %d, sw_volume_l: %d, sw_volume_r: %d.",
           __func__,
           global_settings.balance,
           sw_volume_l,
           sw_volume_r);

    /* Emulate balance with software volume. */
    pcm_set_master_volume(sw_volume_l, sw_volume_r);
}
