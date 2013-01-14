/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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

#include <stdio.h>
#include <stdbool.h>
#include "action.h"
#include "font.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "lang.h"
#include "usb.h"
#if defined(HAVE_USBSTACK)
#include "usb_core.h"
#ifdef USB_ENABLE_HID
#include "usb_keymaps.h"
#endif
#endif
#include "misc.h"
#include "settings.h"
#include "led.h"
#include "appevents.h"
#include "usb_screen.h"
#include "skin_engine/skin_engine.h"
#include "playlist.h"


#if (CONFIG_STORAGE & STORAGE_MMC)
#include "ata_mmc.h"
#endif

#ifdef USB_ENABLE_HID
int usb_keypad_mode;
static bool usb_hid;
#endif

#ifndef SIMULATOR

static int handle_usb_events(void)
{
#if (CONFIG_STORAGE & STORAGE_MMC)
    int next_update=0;
#endif /* STORAGE_MMC */

    /* Don't return until we get SYS_USB_DISCONNECTED or SYS_TIMEOUT */
    while(1)
    {
        int button;
#ifdef USB_ENABLE_HID
        if (usb_hid)
        {
            button = get_hid_usb_action();

            /* On mode change, we need to refresh the screen */
            if (button == ACTION_USB_HID_MODE_SWITCH_NEXT ||
                    button == ACTION_USB_HID_MODE_SWITCH_PREV)
            {
                break;
            }
        }
        else
#endif
        {
            button = button_get_w_tmo(HZ/2);
            /* hid emits the event in get_action */
            send_event(GUI_EVENT_ACTIONUPDATE, NULL);
        }

        switch(button)
        {
            case SYS_USB_DISCONNECTED:
                return 1;
            case SYS_CHARGER_DISCONNECTED:
                /*reset rockbox battery runtime*/
                global_status.runtime = 0;
                break;
            case SYS_TIMEOUT:
                break;
        }

#if (CONFIG_STORAGE & STORAGE_MMC) /* USB-MMC bridge can report activity */
        if(TIME_AFTER(current_tick,next_update))
        {
            if(usb_inserted()) {
                led(mmc_usb_active(HZ));
            }
            next_update=current_tick+HZ/2;
        }
#endif /* STORAGE_MMC */
    }

    return 0;
}
#endif /* SIMULATOR */

#define MODE_NAME_LEN 32

struct usb_screen_vps_t
{
    struct viewport parent;
#ifdef HAVE_LCD_BITMAP
    struct viewport logo;
#ifdef USB_ENABLE_HID
    struct viewport title;
#endif
#endif
};

static void usb_screens_draw(void)
{
#ifdef HAVE_LCD_BITMAP
    int i;
    FOR_NB_SCREENS(i)
    {
        skin_update(CUSTOM_STATUSBAR, i, SKIN_REFRESH_DYNAMIC);
    }
#else /* !HAVE_LCD_BITMAP */
    screen->double_height(false);
    screen->puts_scroll(0, 0, "[USB Mode]");
    status_set_param(false);
    status_set_audio(false);
    status_set_usb(true);
#endif /* HAVE_LCD_BITMAP */

}

void gui_usb_screen_run(bool early_usb)
{
#if defined HAVE_TOUCHSCREEN
    enum touchscreen_mode old_mode = touchscreen_get_mode();

    /* TODO: Paint buttons on screens OR switch to point mode and use
     * touchscreen as a touchpad to move the host's mouse cursor */
    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif

#ifdef USB_ENABLE_HID
    usb_hid = global_settings.usb_hid;
    usb_keypad_mode = global_settings.usb_keypad_mode;
#endif

    push_current_activity(ACTIVITY_USBSCREEN);

    if(!early_usb)
    {
        /* The font system leaves the .fnt fd's open, so we need to force close them all */
#ifdef HAVE_LCD_BITMAP
        font_unload_all();
        FOR_NB_SCREENS(i)
        {
            screens[i].setuifont(FONT_SYSFIXED);
        }
#endif
    }

    usb_acknowledge(SYS_USB_CONNECTED_ACK);
#ifdef HAVE_LCD_BITMAP
    int i;
    FOR_NB_SCREENS(i)
    {
        skin_do_full_update(CUSTOM_STATUSBAR, i);
    }
#endif

    while (1)
    {
        usb_screens_draw();
#ifdef SIMULATOR
        if (button_get_w_tmo(HZ/2))
            break;
        send_event(GUI_EVENT_ACTIONUPDATE, NULL);
#else
        if (handle_usb_events())
            break;
#endif /* SIMULATOR */
    }

#ifdef USB_ENABLE_HID
    if (global_settings.usb_keypad_mode != usb_keypad_mode)
    {
        global_settings.usb_keypad_mode = usb_keypad_mode;
        settings_save();
    }
#endif

#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(old_mode);
#endif

#ifdef HAVE_LCD_CHARCELLS
    status_set_usb(false);
#endif /* HAVE_LCD_CHARCELLS */
#ifdef HAVE_LCD_BITMAP
    if(!early_usb)
    {
        /* Not pretty, reload all settings so fonts are loaded again correctly */
        settings_apply(true);
        settings_apply_skins();
        /* Reload playlist */
        playlist_resume();
    }
#endif

    FOR_NB_SCREENS(i)
    {
        screens[i].backlight_on();
    }

    pop_current_activity();
}

