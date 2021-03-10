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


#include <stdbool.h>
#include <stdlib.h>
#include <sys/reboot.h>

#include "config.h"
#include "debug.h"
#include "power.h"

#include "button-ibasso.h"
#include "debug-ibasso.h"
#include "pcm-ibasso.h"
#include "sysfs-ibasso.h"
#include "vold-ibasso.h"

void power_off(void)
{
    TRACE;

    button_close_device();

    if(vold_monitor_forced_close_imminent())
    {
        /*
            We are here, because Android Vold is going to kill Rockbox. Instead of powering off,
            we exit into the loader.
        */
        DEBUGF("DEBUG %s: Exit Rockbox.", __func__);
        exit(42);
    }

    reboot(RB_POWER_OFF);
}
