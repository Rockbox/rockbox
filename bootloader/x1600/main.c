/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 * Copyright (C) 2021 Aidan MacDonald
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

/* HiBy R1 bootloader. Booting the original firmware is not implemented. */

#include "x1600bootloader.h"
#include "ingenic/gui-ingenic.h"
#include "system.h"
#include "core_alloc.h"
#include "kernel/kernel-internal.h"
#include "power.h"
#include "adc.h"
#include "storage.h"
#include "disk.h"
#include "file_internal.h"
#include "usb.h"
#include "boot-x1600.h"
#include "lcd.h"
#include "i2c-ingenic.h"

void main(void)
{
    system_init();
    core_allocator_init();
    kernel_init();
    i2c_init();
    power_init();

    /* The keys are an ADC ladder, so button_init() needs the SADC up. After
     * power_init(): the ladder is referenced to a PMU rail */
    adc_init();
    button_init();
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif
    enable_irq();

    if(storage_init() < 0) {
        init_lcd();
        lcd_clear_display();
        lcd_putsf(0, 0, "Storage init failed.");
        lcd_putsf(0, 2, "The SD controller did not");
        lcd_putsf(0, 3, "come up. Recovery needs it");
        lcd_putsf(0, 4, "too, so there is nothing");
        lcd_putsf(0, 5, "useful to offer here.");
        lcd_update();

        while(1);
    }
    filesystem_init();
    usb_init();
    usb_start_monitoring();

    /* An unmounted card is handled by the recovery menu. */

    disk_mount_all();

    int btn = read_btn();
    btn &= ~BUTTON_POWER;   /* ignore power, it is held during a cold start */

    /* Enter recovery on a held button or on a USB boot, as the X1000 does. It
     * matters more here: a USB boot has no power-on event, so the button would
     * otherwise have to be down at the instant the host issues PROGRAM_START2.
     *
     * The X1000's SPL sets the flag after reading boot_sel; on the X1600 the
     * usbstage1 payload sets it, since that only runs on the USB path.
     *
     * BL_RECOVERY is BUTTON_NEXT, which is also the BootROM's USB boot strap.
     * They do not collide: holding NEXT with USB attached never reaches this
     * code, because the BootROM claims the boot first. */
    if(btn == BL_RECOVERY || get_boot_flag(BOOT_FLAG_USB_BOOT)) {
        clr_boot_flag(BOOT_FLAG_USB_BOOT);   /* one-shot; do not stick */
        recovery_menu();    /* returns when the user picks Exit */
    }

    /* boot_rockbox() does not return on success, having overwritten this code
     * with the Rockbox image. On failure offer recovery rather than dying
     * silently: no card, no /.rockbox/rockbox.r1, an unmountable card or a bad
     * checksum are all user-actionable, and recovery is the only route to
     * Restore. */
    while(1) {
        boot_rockbox();

        init_lcd();
        do {
            wait_btn_release();
            lcd_clear_display();
            lcd_putsf(0, 0, "Could not start Rockbox.");
            lcd_putsf(0, 2, "Looked for %s", BOOTDIR "/" BOOTFILE);
            lcd_putsf(0, 3, "on a FAT32 card. exFAT will");
            lcd_putsf(0, 4, "not mount.");
            lcd_putsf(0, 6, "%s = recovery options", BL_SELECT_NAME);
            lcd_update();
        } while(wait_btn_press() != BL_SELECT);

        recovery_menu();
    }
}
