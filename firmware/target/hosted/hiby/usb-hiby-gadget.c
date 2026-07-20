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
 * Copyright (C) 2026 by Michael McAllister
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
#include <errno.h>
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

#include "audio.h"
#include "pcm.h"
#include "pcm_sampr.h"
#include "usb_ch9.h"
#include "usb-dac-hiby.h"

#ifdef HAVE_GENERAL_PURPOSE_LED
#include "led-general-purpose.h"
#endif

#define LOGF_ENABLE
#include "logf.h"
#include <stdio.h>

/* Configfs root of the gadget created by usb_init_device() */
#define USB_GADGET_PATH   "/sys/kernel/config/usb_gadget/adb_demo"

static int _usb_mode = -1;
static bool _usb_init = false;
static bool is_adb_running = false;
static int _usb_audio = 0;   /* usb_audio setting: 0 never .. 3 while mass-storage */

void enable_adb(void);
void enable_mass_storage(void);
void enable_charging(void);
bool enable_usb_audio(void);

void disable_adb(void);
void disable_mass_storage(void);
void disable_usb_audio(void);

#ifdef HAVE_MULTIDRIVE
void cleanup_rbhome(void);
void startup_rbhome(void);
#endif

/* Bring up the gadget for a base USB mode (mass storage / charge / adb). */
static void configure_usb_mode(int mode)
{
    switch(mode) {
    case USB_MODE_MASS_STORAGE:
        logf("Enabling Mass Storage\n");
        enable_mass_storage();
        break;
    case USB_MODE_CHARGE:
        logf("Enabling Charge\n");
        enable_charging();
        break;
    case USB_MODE_ADB:
        logf("Enabling ADB\n");
        enable_adb();
        break;
    default:
        break;
    }
}

/* Whether the USB-DAC function should be running, given the usb_audio
 * setting (never/always/while_charge_only/while_mass_storage) and the
 * current base USB mode. */
static bool dac_should_run(void)
{
    switch (_usb_audio) {
    case 1: return true;                                /* always */
    case 2: return _usb_mode == USB_MODE_CHARGE;        /* while charge-only */
    case 3: return _usb_mode == USB_MODE_MASS_STORAGE;  /* while mass-storage */
    default: return false;                              /* never */
    }
}

/* Reconcile the DAC gadget with the setting + current mode.
 *
 * In adb mode the DAC rides alongside adb as a composite function, so we
 * rebuild the adb gadget (enable_adb() composes uac_sa when dac_should_run()).
 * In charge/mass-storage mode the DAC is a standalone self-bound gadget. */
static void apply_usb_audio(void)
{
    if (_usb_mode == USB_MODE_ADB) {
        enable_adb();
        return;
    }

    bool want = dac_should_run();
    bool active = usb_audio_get_active();

    if (want && !active) {
        /* enable_usb_audio() returns false if applied before audio_init()
         * (PCM mixer not up yet -- e.g. at boot) or if uac_sa never comes
         * up. Either way restore the base mode so we never leave a dead
         * gadget behind. */
        if (!enable_usb_audio())
            configure_usb_mode(_usb_mode);
    } else if (!want && active) {
        disable_usb_audio();
        configure_usb_mode(_usb_mode);
    }
}

/* usb_set_audio() is the target's implementation of the usb_audio setting
 * callback declared in usb.h (HAVE_HOST_USB_AUDIO); it drives the DAC gadget
 * up or down. */
void usb_set_audio(int value)
{
    _usb_audio = value;
    apply_usb_audio();
}

/* Compose uac_sa into the adb config, linked FIRST so the audio interfaces
 * are 0-1 (the vendor uac_sa function hard-codes them), with the DAC VID/PID
 * + IAD descriptor. Returns true if it composed the DAC. Skipped before
 * audio_init() (no PCM mixer yet); re-applied once it is up. */
static bool adb_compose_dac(void)
{
    if (!dac_should_run() || !pcm_is_initialized())
        return false;

    sysfs_set_string(USB_GADGET_PATH "/idVendor", "0x" USB_VID_STR);
    sysfs_set_string(USB_GADGET_PATH "/idProduct", "0x0004");
    sysfs_set_int(USB_GADGET_PATH "/bDeviceClass", USB_CLASS_MISC);
    sysfs_set_int(USB_GADGET_PATH "/bDeviceSubClass", 2);
    sysfs_set_int(USB_GADGET_PATH "/bDeviceProtocol", 1);

    system("mkdir -p " USB_GADGET_PATH "/functions/uac_sa.a");
    sysfs_set_int(USB_GADGET_PATH "/functions/uac_sa.a/c_chmask", 3);
    sysfs_set_int(USB_GADGET_PATH "/functions/uac_sa.a/c_ssize", 2);
    sysfs_set_int(USB_GADGET_PATH "/functions/uac_sa.a/c_srate", SAMPR_48);
    system("ln -sf " USB_GADGET_PATH "/functions/uac_sa.a " USB_GADGET_PATH "/configs/c.1/");
    sysfs_set_string(USB_GADGET_PATH "/configs/c.1/strings/0x409/configuration", "uac,adb");
    return true;
}

void hiby_set_usb_mode(int mode) {
    logf(">>>>>>>>>>>>>>>>> hiby_set_usb_mode(%d)\n", mode);
    if (!_usb_init) {
        logf("Need to init usb!\n");
        usb_init_device();
    }

    if (_usb_mode == mode)
        return;

    /* Drop any active DAC before reconfiguring the base gadget. */
    if (usb_audio_get_active())
        disable_usb_audio();

    /* Set the mode first so enable_adb()/dac_should_run() see it. */
    _usb_mode = mode;
    configure_usb_mode(mode);

    /* adb composes the DAC itself in enable_adb(); charge/mass-storage get
     * the standalone self-bound DAC here. */
    if (mode != USB_MODE_ADB)
        apply_usb_audio();
}

/* TODO: implement usb detection properly */
int usb_detect(void)
{
#ifdef HAVE_GENERAL_PURPOSE_LED
    led_hw_on();
#endif
    return power_input_status() == POWER_INPUT_USB_CHARGER ? USB_INSERTED : USB_EXTRACTED;
}

static void set_mass_storage_lun(void)
{
    const char *device = "/dev/mmcblk0p1";

    // If partition 1 doesn't exist we'll try the main device
    if (access(device, F_OK) != 0)
        device = "/dev/mmcblk0";

    sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0/lun.0/file", device);
}

void usb_enable(bool on)
{
    logf(">>>>>>>>>>>>>>>>> usb_enable(%d)\n", on);
    logf("usb enable %d %d\n", on, _usb_mode);

    /* ADB binds the UDC itself, and so does the USB-DAC while it is active,
     * so leave the UDC alone in those cases. */
    if (_usb_mode == USB_MODE_ADB || usb_audio_get_active())
        return;

    // Re-arm the LUN on each connect, otherwise the disk is only exported the first time.
    if (on && _usb_mode == USB_MODE_MASS_STORAGE)
        set_mass_storage_lun();

    sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/UDC",
                     on ? "13500000.otg_new" : "\n");
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
    disable_usb_audio();
}

void enable_adb(void) {
    logf(">>>>>>>>>>>>>>>>> set_adb()\n");

    // Stop the respawner FIRST so it can't rebind adbd under us.
    system("killall adbserver.sh adbd 2>/dev/null");

    /* Disable mass storage and USB DAC if they were running (unbinds the UDC
     * and clears any uac_sa link/descriptor). */
    disable_mass_storage();
    disable_usb_audio();

    /* Also drop the adb function link so we control the link order below
     * (uac_sa must be linked before adb -- see adb_compose_dac()). */
    if (access(USB_GADGET_PATH "/configs/c.1/ffs.adb", F_OK) == 0)
        system("rm " USB_GADGET_PATH "/configs/c.1/ffs.adb");

    system("mkdir -p " USB_GADGET_PATH "/configs/c.1/strings/0x409");
    system("mkdir -p " USB_GADGET_PATH "/functions/ffs.adb");

    /* Compose the USB-DAC (uac_sa, linked first) alongside adb when the
     * usb_audio setting wants it; otherwise present plain adb. */
    bool with_dac = adb_compose_dac();
    if (!with_dac) {
        // Use the adb VID/PID so the host recognises the device
        sysfs_set_string(USB_GADGET_PATH "/idVendor", "0x18d1");
        sysfs_set_string(USB_GADGET_PATH "/idProduct", "0xd002");
        sysfs_set_string(USB_GADGET_PATH "/configs/c.1/strings/0x409/configuration", "adb");
    }

    sysfs_set_int(USB_GADGET_PATH "/configs/c.1/MaxPower", 120);

    // Link adb into the config (after uac_sa, so adb is the later interface)
    system("ln -sf " USB_GADGET_PATH "/functions/ffs.adb " USB_GADGET_PATH "/configs/c.1/");

    system("mkdir -p /dev/usb-ffs/adb");

    // Let the vendor respawner mount functionfs and run adbd, which binds the UDC.
    system("/sbin/adbserver.sh 440 &");

    /* uac_sa is bound by the same auto-bind; start pumping /dev/uac_sa. */
    if (with_dac)
        usb_dac_start();
}


void disable_adb(void) {
    // Remove any lingering adb daemon and its respawner
    system("killall adbserver.sh adbd 2>/dev/null");

    // Unbind the UDC so the gadget can be reconfigured
    if (access("/sys/kernel/config/usb_gadget/adb_demo/UDC", F_OK) == 0) {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/UDC", "\n");
    }

    // Unlink the adb function from config (the function is left in place and
    // reused, so re-creating it in enable_adb can't block)
    if (access("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/ffs.adb", F_OK) == 0) {
        system("rm /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/ffs.adb");
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
        system("umount -l /dev/usb-ffs/adb");
    }
}

void enable_mass_storage(void) {
    logf(">>>>>>>>>>>>>>>>> set_mass_storage()\n");

    /* Disable adb and USB DAC if they're running */
    disable_adb();
    disable_usb_audio();

    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0/lun.0");
    system("mkdir -p /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409");

    if (is_adb_running) {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", "adb,storage");
    } else {
        sysfs_set_string("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/strings/0x409/configuration", "storage");
    }
    sysfs_set_int("/sys/kernel/config/usb_gadget/adb_demo/configs/c.1/MaxPower", 120);

    system("ln -s /sys/kernel/config/usb_gadget/adb_demo/functions/mass_storage.0 /sys/kernel/config/usb_gadget/adb_demo/configs/c.1/");

    set_mass_storage_lun();
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

/* USB DAC (USB Audio Class gadget).
 *
 * The vendor kernel provides a UAC function "uac_sa"; building it into the
 * gadget and binding the UDC creates /dev/uac_sa, which usb-dac-hiby.c pumps
 * into the PCM mixer. enable_usb_audio() is the standalone path used in
 * charge/mass-storage mode; adb mode composes uac_sa alongside adb in
 * enable_adb() instead. */

bool enable_usb_audio(void)
{
    logf(">>>>>>>>>>>>>>>>> enable_usb_audio()\n");

    /* The DAC pump feeds the PCM mixer, which isn't up until audio_init(), so
     * a DAC setting applied at boot lands too early. Bail before touching
     * anything so the caller can restore the base mode. */
    if (!pcm_is_initialized())
        return false;

    /* The DAC hands the output path to the host's PCM; stop local playback
     * first (it can't restart while the DAC is active). */
    audio_stop();

    /* Standalone DAC: tear down the other gadget functions first. */
    disable_mass_storage();
    disable_adb();

    /* Build the "uac_sa" function: stereo, 16-bit, capture (host->device).
     * Set all three explicitly: the kernel's defaults are 3/2/64000, so at
     * least the 64 kHz capture rate must always be overridden. */
    system("mkdir -p " USB_GADGET_PATH "/functions/uac_sa.a");
    sysfs_set_int(USB_GADGET_PATH "/functions/uac_sa.a/c_chmask", 3);
    sysfs_set_int(USB_GADGET_PATH "/functions/uac_sa.a/c_ssize", 2);
    sysfs_set_int(USB_GADGET_PATH "/functions/uac_sa.a/c_srate", SAMPR_48);

    /* Present an IAD audio device with the HiBy DAC VID/PID so the host
     * recognises it as a USB DAC. */
    sysfs_set_int(USB_GADGET_PATH "/bDeviceClass", USB_CLASS_MISC);
    sysfs_set_int(USB_GADGET_PATH "/bDeviceSubClass", 2);
    sysfs_set_int(USB_GADGET_PATH "/bDeviceProtocol", 1);
    sysfs_set_string(USB_GADGET_PATH "/idVendor", "0x" USB_VID_STR);
    sysfs_set_string(USB_GADGET_PATH "/idProduct", "0x0004");

    system("mkdir -p " USB_GADGET_PATH "/configs/c.1/strings/0x409");
    sysfs_set_string(USB_GADGET_PATH
                     "/configs/c.1/strings/0x409/configuration", "uac");
    sysfs_set_int(USB_GADGET_PATH "/configs/c.1/bmAttributes",
                  USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER);
    sysfs_set_int(USB_GADGET_PATH "/configs/c.1/MaxPower", 120);

    system("ln -sf " USB_GADGET_PATH "/functions/uac_sa.a "
           USB_GADGET_PATH "/configs/c.1/");

    /* Bind the UDC ourselves -- in charge mode the usb thread is power-only,
     * and usb_enable() bails while the DAC is active, so nothing else binds
     * it. This creates /dev/uac_sa. */
    sysfs_set_string(USB_GADGET_PATH "/UDC", "13500000.otg_new");

    return usb_dac_start();
}

void disable_usb_audio(void)
{
    logf(">>>>>>>>>>>>>>>>> disable_usb_audio()\n");

    usb_dac_stop();

    /* Unbind the UDC and tear the function down so the next mode starts
     * from a clean gadget. */
    if (access(USB_GADGET_PATH "/UDC", F_OK) == 0)
        sysfs_set_string(USB_GADGET_PATH "/UDC", "\n");

    if (access(USB_GADGET_PATH "/configs/c.1/uac_sa.a", F_OK) == 0)
        system("rm " USB_GADGET_PATH "/configs/c.1/uac_sa.a");

    if (access(USB_GADGET_PATH "/functions/uac_sa.a", F_OK) == 0)
        system("rmdir " USB_GADGET_PATH "/functions/uac_sa.a");

    /* Restore a plain (non-IAD) device descriptor and the target's own
     * VID/PID for the other modes. */
    sysfs_set_int(USB_GADGET_PATH "/bDeviceClass", USB_CLASS_PER_INTERFACE);
    sysfs_set_int(USB_GADGET_PATH "/bDeviceSubClass", 0);
    sysfs_set_int(USB_GADGET_PATH "/bDeviceProtocol", 0);
    sysfs_set_string(USB_GADGET_PATH "/idVendor", "0x" USB_VID_STR);
    sysfs_set_string(USB_GADGET_PATH "/idProduct", "0x" USB_PID_STR);

    if (access(USB_GADGET_PATH
               "/configs/c.1/strings/0x409/configuration", F_OK) == 0)
        sysfs_set_string(USB_GADGET_PATH
                         "/configs/c.1/strings/0x409/configuration", "");
}

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
