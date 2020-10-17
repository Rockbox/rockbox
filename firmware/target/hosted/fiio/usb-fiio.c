/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2018 by Marcin Bukat
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

#include <stdlib.h>
#include <sys/mount.h>
#include <string.h>
#include "config.h"
#include "disk.h"
#include "usb.h"
#include "sysfs.h"
#include "power.h"
#include "power-fiio.h"

#ifdef HAVE_MULTIDRIVE
void cleanup_rbhome(void);
void startup_rbhome(void);
#endif

const char * const sysfs_usb_online =
    "/sys/class/power_supply/usb/online";

int usb_detect(void)
{
    int present = 0;
    sysfs_get_int(sysfs_usb_online, &present);

    return present ? USB_INSERTED : USB_EXTRACTED;
}

static bool usb_enabled = 0;

void usb_enable(bool on)
{
    if (usb_enabled == on)
        return;

    usb_enabled = on;
    if (on)
    {
	system ("insmod /lib/modules/3.10.14/kernel/driver/usb/gadget/libcomposite.ko");
	system ("insmod /lib/modules/3.10.14/kernel/driver/usb/gadget/usb_f_mass_storage.ko");
	system ("insmod /lib/modules/3.10.14/kernel/driver/usb/gadget/g_mass_storage.ko file=/dev/mmcblk0 removable=1");
    }
    else
    {
	system ("rmmod g_mass_storage");
	system ("rmmod usb_f_mass_storage");
	system ("rmmod libcomposite");
    }
}

/* This is called by usb thread after usb extract in order to return
 * regular FS access
 *
 * returns the # of successful mounts
*/
int disk_mount_all(void)
{
#ifdef HAVE_MULTIDRIVE
    startup_rbhome();
#endif
    return 1;
}

/* This is called by usb thread after all threads ACKs usb inserted message
 *
 * returns the # of successful unmounts
 */
int disk_unmount_all(void)
{
#ifdef HAVE_MULTIDRIVE
    cleanup_rbhome();
#endif

    return 1;
}

void usb_init_device(void)
{
    system ("insmod /lib/modules/3.10.14/kernel/driver/staging/dwc2/dwc2.ko");
    usb_enable(true);
}
