/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2018 by Marcin Bukat
 * Copyright (C) 2025 by Melissa Autumn
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

#include <dirent.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "dir.h"
#include "disk.h"
#include "usb.h"
#include "sysfs.h"
#include "power.h"
#include "usb-hiby-gadget.h"

#ifdef HAVE_GENERAL_PURPOSE_LED
#include "led-general-purpose.h"
#endif

#define LOGF_ENABLE
#include "logf.h"
#include <stdio.h>

static int _usb_mode = -1;
static bool _usb_init = false;
static bool is_adb_running = false;

void enable_adb(void);
void enable_mass_storage(void);
void enable_charging(void);
void enable_usb_audio(void);

void disable_adb(void);
void disable_mass_storage(void);
void disable_usb_audio(void);

#ifdef HAVE_MULTIDRIVE
void cleanup_rbhome(void);
void startup_rbhome(void);
#endif

void hiby_set_usb_mode(int mode) {
    logf(">>>>>>>>>>>>>>>>> hiby_set_usb_mode(%d)\n", mode);
    if (!_usb_init) {
        logf("Need to init usb!\n");
        usb_init_device();
    }

    switch(mode) {
    case USB_MODE_MASS_STORAGE:
        logf("Enabling Mass Storage\n");
        enable_mass_storage();
        break;
    case USB_MODE_CHARGE:
        logf("Enabling Charge\n");
        enable_charging();
        break;
    default:
        break;
    }

    _usb_mode = mode;
}

/* TODO: implement usb detection properly */
int usb_detect(void)
{
#ifdef HAVE_GENERAL_PURPOSE_LED
    led_hw_on();
#endif
    return power_input_status() == POWER_INPUT_USB_CHARGER ? USB_INSERTED : USB_EXTRACTED;
}

void usb_enable(bool on)
{
    logf(">>>>>>>>>>>>>>>>> usb_enable(%d)\n", on);
    logf("usb enable %d %d\n", on, _usb_mode);

    /* Ignore usb enable/disable when ADB is enabled so we can fireup adb shell
     * without entering ums mode
     */
    //if (_usb_mode != USB_MODE_ADB) {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/UDC", on ? "13500000.otg_new" : "");
    //}
}

/* This is called by usb thread after usb extract in order to return
 * regular FS access
 *
 * returns the # of successful mounts
*/
int disk_mount_all(void)
{
    logf(">>>>>>>>>>>>>>>>> disk_mount_all()\n");

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
    // We're always mounted as rockbox lives on the sdcard
    logf(">>>>>>>>>>>>>>>>> disk_unmount_all()\n");

#ifdef HAVE_MULTIDRIVE
    cleanup_rbhome();
#endif

#ifdef HAVE_MULTIDRIVE
    startup_rbhome();
#endif

    return 1;
}

void enable_charging(void) {
    logf(">>>>>>>>>>>>>>>>> enable_charging()\n");

    disable_mass_storage();
    disable_adb();
}

#if 1
void enable_adb(void) {
    logf(">>>>>>>>>>>>>>>>> set_adb()\n");

    // Disable mass storage if it was running
    disable_mass_storage();

    // Remove any lingering adb daemon
    system("killall -9 adbd");

    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409");
    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/functions/ffs.adb");

    // Now we'll override configuration and MaxPower
    sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", "adb");
    sysfs_set_int("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/MaxPower", 120);

    // And link up the adb function to the usb gadget config
    system("ln -s /sys/kernel/config/usb_gadget/adb_demo/functions/ffs.adb /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/");

    int is_mounted = !system("mountpoint -q /dev/usb-ffs/adb");

    if (!is_mounted) {
        system("mkdir -p /dev/usb-ffs/adb");
        /* This seems to fail, but adb will still work and then it will be mounted. Not sure what's up here... */
        system("mount -t functionfs adb /dev/usb-ffs/adb");
    }

    // Boot adb daemon
    system("/usr/bin/adbd &");
}


void disable_adb(void) {
    // Remove any lingering adb daemon
    system("killall -9 adbd");

    // Remove the adb link to config
    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/ffs.adb", F_OK) == 0) {
        system("rm /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/ffs.adb");
    }

    // Remove the adb function
    if (access("/sys/kernel/config/usb_gadget/adb_demo/functions/ffs.adb", F_OK) == 0) {
        system("rm -rf /sys/kernel/config/usb_gadget/adb_demo/functions/ffs.adb");
    }

    // Reset the MaxPower to its default value
    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/MaxPower", F_OK) == 0) {
        sysfs_set_int("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/MaxPower", 120);
    }

    // Disable storage or adb configs
    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", F_OK) == 0) {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", "");
    }

    // Unmount adb
    if (!system("mountpoint -q /dev/usb-ffs/adb")) {
        system("unmount /dev/usb-ffs/adb");
    }
}
#else
void enable_adb(void) {}
void disable_adb(void) {}
#endif

void enable_mass_storage(void) {
    logf(">>>>>>>>>>>>>>>>> set_mass_storage()\n");

    // Disable adb if it's running
    disable_adb();

    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0/lun.0");
    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409");

    if (is_adb_running) {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", "adb,storage");
    } else {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", "storage");
    }
    sysfs_set_int("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/MaxPower", 120);

    system("ln -s /sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0 /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/");

    char mount_device[32] = "/dev/mmcblk0p1";

    // If partition 1 doesn't exist we'll try the main device
    if (access(mount_device, F_OK) != 0) {
        memset(mount_device, 0, sizeof(mount_device));
        strcpy(mount_device, "/dev/mmcblk0");
    }

    sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0/lun.0/file", mount_device);
}

void disable_mass_storage(void) {
    // Remove the mass_storage.0 link to config
    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/mass_storage.0", F_OK) == 0) {
        system("rm /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/mass_storage.0");
    }

    // Remove the mass_storage.0 function
    if (access("/sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0", F_OK) == 0) {
        system("rm -rf /sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0");
    }

    // Reset the MaxPower to its default value
    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/MaxPower", F_OK) == 0) {
        sysfs_set_int("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/MaxPower", 120);
    }

    // Disable storage or adb configs
    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", F_OK) == 0) {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", "");
    }
}

#if defined(HAVE_USB_AUDIO)
void enable_usb_audio(void) {

}

void disable_usb_audio(void) {

}
#else
void enable_usb_audio(void) {}
void disable_usb_audio(void) {}
#endif

void usb_init_device(void)
{
    logf(">>>>>>>>>>>>>>>>> usb_init_device()\n");
    if (_usb_init) {
        logf("usb is already init, skipping!\n");
        return;
    }

    char functions[128] = {0};

    /* Before we can do anything here we need to mount configfs */
    int is_mounted = !system("mountpoint -q /sys/kernel/config");

    if (!is_mounted && system("mount -t configfs none /sys/kernel/config")) {
        logf("mount configfs failed, can't do usb functionality! ErrNo: %d\n", errno);
        return;
    }

    _usb_init = true;

    system("ls -la /sys/kernel/config/usb_gadget");

    /* os_mkdir doesn't seem to work here for whatever reason */
    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo");
    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/strings/0x409");

    system("ls -la /sys/kernel/config/usb_gadget/adb_demo");
    system("ls -la /sys/kernel/config/usb_gadget/adb_demo/configs/");

    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409");

    /* Check if ADB was activated in bootloader */
    logf("checking if adb is already on\n");

    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", F_OK) == 0) {
        logf("found usb config string!\n");

        sysfs_get_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", functions, sizeof(functions));
        is_adb_running = (strstr(functions, "adb") == NULL) ? false : true;
    }

    sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/strings/0x409/manufacturer", "Rockbox.org");
    sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/strings/0x409/product", "Rockbox media player");
    sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/strings/0x409/serialnumber", "0123456789ABCDEF");
}
