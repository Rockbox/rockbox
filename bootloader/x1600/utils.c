/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 * Copyright (C) 2022 Aidan MacDonald
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/

/* Buttons, confirmation prompts and card presence.
 * Mirrors bootloader/x1000/utils.c. */

#include "x1600bootloader.h"
#include "ingenic/gui-ingenic.h"
#include "core_alloc.h"
#include "kernel.h"
#include "storage.h"
#include "disk.h"
#include "usb.h"
#include "rb-loader.h"
#include "lcd.h"

int get_recovery_button(int timeout)
{
    int btn = get_button(timeout);
    if(btn != SYS_USB_CONNECTED)
        return btn;

    splashf(0, "USB mode\nDisconnect to continue");

    /* Like X1000 usb_mode(): no local file or NAND access after the ACK.
     * A reconnect during input draining needs its own acknowledgement. */
    do {
        if(btn == SYS_USB_CONNECTED)
            usb_acknowledge(SYS_USB_CONNECTED_ACK, usb_connection_seqnum);

        if(is_usb_connected)
            btn = get_button(TIMEOUT_BLOCK);
        else if(button_status() != BUTTON_NONE)
            btn = get_button(HZ/20);
        else
            btn = get_button(TIMEOUT_NOBLOCK);
    } while(is_usb_connected || btn != BUTTON_NONE ||
            button_status() != BUTTON_NONE);

    /* USB remounts before broadcasting disconnect. Rescan here as well in
     * case disconnect beat our ACK and hotplug events were suppressed. */
    disk_mount_all();
    return SYS_USB_DISCONNECTED;
}

/* Discard held keys and queued presses before showing a new prompt. */
void wait_btn_release(void)
{
    while(button_status() != BUTTON_NONE)
        get_recovery_button(HZ/20);
    while(get_recovery_button(TIMEOUT_NOBLOCK) != BUTTON_NONE);
}

int wait_btn_press(void)
{
    while(1) {
        int btn = get_recovery_button(TIMEOUT_BLOCK);
        if(btn == BL_UP || btn == BL_DOWN ||
           btn == BL_SELECT || btn == BL_QUIT || btn == SYS_USB_DISCONNECTED)
            return btn;
    }
}

/* Returns true if the user confirms.  Used for the two destructive actions. */
bool confirm(const char* what, const char* consequence)
{
    init_lcd();
    while(1) {
        wait_btn_release();
        lcd_clear_display();
        int line = 0;
        lcd_putsf(0, line++, "%s?", what);
        line++;
        lcd_putsf(0, line++, "%s", consequence);
        line++;
        lcd_putsf(0, line++, "%s = yes", BL_SELECT_NAME);
        lcd_putsf(0, line++, "%s = no",  BL_QUIT_NAME);
        lcd_update();

        int btn = wait_btn_press();
        if(btn == BL_SELECT) return true;
        if(btn == BL_QUIT || btn == SYS_USB_DISCONNECTED) return false;
    }
}

/* Check for a card before starting a file operation. */
bool card_available(void)
{
    if(storage_present(IF_MD(0)))
        return true;

    init_lcd();
    do {
        wait_btn_release();
        lcd_clear_display();
        lcd_putsf(0, 0, "No SD card detected.");
        lcd_putsf(0, 2, "Recovery reads and writes");
        lcd_putsf(0, 3, "files on the card.");
        lcd_putsf(0, 5, "%s to continue", BL_QUIT_NAME);
        lcd_update();
    } while(wait_btn_press() != BL_QUIT);
    return false;
}

/* Load into a buflib allocation. Leave error reporting to the caller,
 * since the display may not be initialized yet. */
int load_rockbox(const char* filename, size_t* sizep)
{
    int handle = core_alloc_maximum(sizep, &buflib_ops_locked);
    if(handle < 0)
        return -2;

    unsigned char* loadbuffer = core_get_data(handle);
    int rc = load_firmware(loadbuffer, filename, *sizep);
    if(rc <= 0) {
        core_free(handle);
        return -3;
    }

    core_shrink(handle, loadbuffer, rc);
    *sizep = rc;
    return handle;
}
