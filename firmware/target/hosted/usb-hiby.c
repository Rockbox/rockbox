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

//#define LOGF_ENABLE
#include "logf.h"

static bool adb_mode = false;

#ifdef HAVE_MULTIDRIVE
void cleanup_rbhome(void);
void startup_rbhome(void);
#endif

/* TODO: implement usb detection properly */
int usb_detect(void)
{
    return power_input_status() == POWER_INPUT_USB_CHARGER ? USB_INSERTED : USB_EXTRACTED;
}

void usb_enable(bool on)
{
    logf("usb enable %d %d\n", on, adb_mode);

    /* Ignore usb enable/disable when ADB is enabled so we can fireup adb shell
     * without entering ums mode
     */
    if (!adb_mode)
    {
        sysfs_set_int("/sys/class/android_usb/android0/enable", on ? 1 : 0);
    }
}

/* This is called by usb thread after usb extract in order to return
 * regular FS access
 *
 * returns the # of successful mounts
*/
int disk_mount_all(void)
{
    const char *dev[] = {"/dev/mmcblk0p1", "/dev/mmcblk0"};
    const char *fs[] = {"vfat", "exfat"};

    sysfs_set_string("/sys/class/android_usb/android0/f_mass_storage/lun/file", "");

    for (int i=0; i<2; i++)
    {
        for (int j=0; j<2; j++)
        {
            int rval = mount(dev[i], PIVOT_ROOT, fs[j], 0, NULL);
            if (rval == 0 || errno == -EBUSY)
            {
                logf("mount good! %d/%d %d %d", i, j, rval, errno);
#ifdef HAVE_MULTIDRIVE
                startup_rbhome();
#endif
                return 1;
            }
         }
    }

    logf("mount failed! %d", errno);
    return 0;
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

    if (umount(PIVOT_ROOT) == 0)
    {
        sysfs_set_string("/sys/class/android_usb/android0/f_mass_storage/lun/file", "/dev/mmcblk0");
        logf("umount_all good");
        return 1;
    }

    logf("umount_all failed! %d", errno);
#ifdef HAVE_MULTIDRIVE
    startup_rbhome();
#endif

    return 0;
}

void usb_init_device(void)
{
    char functions[32] = {0};

    /* Check if ADB was activated in bootloader */
    sysfs_get_string("/sys/class/android_usb/android0/functions", functions, sizeof(functions));
    adb_mode = (strstr(functions, "adb") == NULL) ? false : true;

    usb_enable(false);

    if (adb_mode)
    {
        sysfs_set_string("/sys/class/android_usb/android0/functions", "mass_storage,adb");
        sysfs_set_string("/sys/class/android_usb/android0/idVendor", "18D1");
        sysfs_set_string("/sys/class/android_usb/android0/idProduct", "D002");
    }
    else
    {
        sysfs_set_string("/sys/class/android_usb/android0/functions", "mass_storage");
        sysfs_set_string("/sys/class/android_usb/android0/idVendor", "C502");
        sysfs_set_string("/sys/class/android_usb/android0/idProduct", "0029");
    }

    sysfs_set_string("/sys/class/android_usb/android0/iManufacturer", "Rockbox.org");
    sysfs_set_string("/sys/class/android_usb/android0/iProduct", "Rockbox media player");
    sysfs_set_string("/sys/class/android_usb/android0/iSerial", "0123456789ABCDEF");
    sysfs_set_string("/sys/class/android_usb/android0/f_mass_storage/inquiry_string", "Rockbox 0100");
}
