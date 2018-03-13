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
#include "config.h"
#include "disk.h"
#include "usb.h"
#include "sysfs.h"

/* TODO: implement usb detection properly */
int usb_detect(void)
{
    return USB_EXTRACTED;
}

void usb_enable(bool on)
{
    sysfs_set_int("/sys/class/android_usb/android0/enable", on ? 1 : 0);
}

/* This is called by usb thread after usb extract in order to return
 * regular FS access
 *
 * returns the # of successful mounts
*/
int disk_mount_all(void)
{
    sysfs_set_string("/sys/class/android_usb/android0/f_mass_storage/lun/file", "empty");

    system("mount /dev/mmcblk0 /mnt/sd_0");
    system("mount /dev/mmcblk0p1 /mnt/sd_0");

    return 1;
}

/* This is called by usb thread after all threads ACKs usb inserted message
 *
 * returns the # of successful unmounts
 */
int disk_unmount_all(void)
{
    /* TODO: figure out actual block device */
    system("umount /dev/mmcblk0p1 /mnt/sd_0");
    system("umount /dev/mmcblk0 /mnt/sd_0");

    sysfs_set_string("/sys/class/android_usb/android0/f_mass_storage/lun/file", "/dev/mmcblk0p1");
    return 1;
}

void usb_init_device(void)
{
    usb_enable(false);

    sysfs_set_string("/sys/class/android_usb/android0/idVendor", "C502");
    sysfs_set_string("/sys/class/android_usb/android0/idProduct", "0029");
    sysfs_set_string("/sys/class/android_usb/android0/iManufacturer", "Rockbox.org");
    sysfs_set_string("/sys/class/android_usb/android0/iProduct", "Rockbox media player");
    sysfs_set_string("/sys/class/android_usb/android0/iSerial", "0123456789ABCDEF");
    sysfs_set_string("/sys/class/android_usb/android0/functions", "mass_storage");
    sysfs_set_string("/sys/class/android_usb/android0/f_mass_storage/inquiry_string", "Agptek Rocker 0100");
}
