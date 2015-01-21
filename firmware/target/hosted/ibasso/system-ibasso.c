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


#include <stdint.h>
#include <sys/reboot.h>

#include "config.h"
#include "cpufreq-linux.h"
#include "debug.h"

#include "button-ibasso.h"
#include "debug-ibasso.h"
#include "sysfs-ibasso.h"


/* Fake stack. */
uintptr_t* stackbegin;
uintptr_t* stackend;


void system_init(void)
{
    TRACE;

    /* Fake stack. */
    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;

    /*
        Prevent device from deep sleeping, which will interrupt playback.
        /sys/power/wake_lock
    */
    if(! sysfs_set_string(SYSFS_POWER_WAKE_LOCK, "rockbox"))
    {
        DEBUGF("ERROR %s: Can not set suspend blocker.", __func__);
    }

    /*
        Prevent device to mute, which will cause tinyalsa pcm_writes to fail.
        /sys/class/codec/wm8740_mute
    */
    if(! sysfs_set_char(SYSFS_WM8740_MUTE, '0'))
    {
        DEBUGF("ERROR %s: Can not set WM8740 lock.", __func__);
    }

    cpufreq_set_governor("powersave", CPUFREQ_ALL_CPUS);
}


void system_reboot(void)
{
    TRACE;

    button_close_device();

    reboot(RB_AUTOBOOT);
}


void system_exception_wait(void)
{
    TRACE;

    while(1) {};
}
