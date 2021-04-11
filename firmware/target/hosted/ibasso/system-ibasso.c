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
#include <stdlib.h>
#include <sys/reboot.h>

#include "config.h"
#include "debug.h"
#include "mv.h"

#include "button-ibasso.h"
#include "debug-ibasso.h"
#include "sysfs-ibasso.h"
#include "vold-ibasso.h"


/* Fake stack. */
uintptr_t* stackbegin;
uintptr_t* stackend;

/* forward-declare */
bool os_file_exists(const char *ospath);

void system_init(void)
{
    TRACE;

    /* Fake stack. */
    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;

	vold_monitor_start();

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
    if(! sysfs_set_char(SYSFS_WM8740_MUTE, 0))
    {
        DEBUGF("ERROR %s: Can not set WM8740 lock.", __func__);
    }
}


void system_reboot(void)
{
    TRACE;

    button_close_device();

    if(vold_monitor_forced_close_imminent())
    {
        /*
            We are here, because Android Vold is going to kill Rockbox. Instead of powering off,
            we exit into the loader.
        */
        exit(42);
    }

    reboot(RB_AUTOBOOT);
}


void system_exception_wait(void)
{
    TRACE;

    while(1) {};
}

bool hostfs_removable(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    if (drive > 0)
        return true;
    else
#endif
        return false; /* internal: always present */
}

#ifdef HAVE_MULTIDRIVE
int volume_drive(int drive)
{
    return drive;
}
#endif /* HAVE_MULTIDRIVE */

#ifdef CONFIG_STORAGE_MULTI
int hostfs_driver_type(int drive)
{
    return drive > 0 ? STORAGE_SD_NUM : STORAGE_HOSTFS_NUM;
}
#endif /* CONFIG_STORAGE_MULTI */

bool hostfs_present(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    if (drive > 0)
#if defined(MULTIDRIVE_DEV)
        return os_file_exists(MULTIDRIVE_DEV);
#else
        return extsd_present;
#endif
    else
#endif
        return true; /* internal: always present */
}

#ifdef HAVE_HOTSWAP
bool volume_removable(int volume)
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_removable(volume);
}

bool volume_present(int volume)
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_present(volume);
}
#endif
