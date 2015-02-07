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

#include "debug-ibasso.h"
#include "sysfs-ibasso.h"


extern void set_software_volume(void);


void audiohw_set_volume(int volume)
{
    set_software_volume();

    /*
        See codec-dx50.h.
        -128db        -> -1 (adjusted to 0, mute)
        -127dB to 0dB ->  1 to 255 in steps of 2
        volume is in centibels (tenth-decibels).
    */
    int volume_adjusted = (volume / 10) * 2 + 255;

    DEBUGF("DEBUG %s: volume: %d, volume_adjusted: %d.", __func__, volume, volume_adjusted);

    if(volume_adjusted > 255)
    {
        DEBUGF("DEBUG %s: Adjusting volume from %d to 255.", __func__, volume);
        volume_adjusted = 255;
    }
    if(volume_adjusted < 0)
    {
        DEBUGF("DEBUG %s: Adjusting volume from %d to 0.", __func__, volume);
        volume_adjusted = 0;
    }

    /*
        /dev/codec_volume
        0 ... 255
    */
    if(! sysfs_set_int(SYSFS_DX50_CODEC_VOLUME, volume_adjusted))
    {
        DEBUGF("ERROR %s: Can not set volume.", __func__);
    }
}


void audiohw_set_filter_roll_off(int val)
{
    DEBUGF("DEBUG %s: val: %d", __func__, val);

    if(! sysfs_set_char(SYSFS_ES9018_FILTER, (char) val))
    {
        DEBUGF("ERROR %s: Can not set roll off filter.", __func__);
    }
}
