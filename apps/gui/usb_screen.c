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
#include "settings.h"
#include "led.h"
#include "appevents.h"
#include "usb_screen.h"
#include "skin_engine/skin_engine.h"
#include "playlist.h"
#include "misc.h"

#include "bitmaps/usblogo.h"

#ifdef HAVE_REMOTE_LCD
#include "bitmaps/remote_usblogo.h"
#endif

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
                reset_runtime();
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
    struct viewport logo;
#ifdef USB_ENABLE_HID
    struct viewport title;
#endif
};

static void usb_screen_fix_viewports(struct screen *screen,
        struct usb_screen_vps_t *usb_screen_vps)
{
    int logo_width, logo_height;
    struct viewport *parent = &usb_screen_vps->parent;
    struct viewport *logo = &usb_screen_vps->logo;

#ifdef HAVE_REMOTE_LCD
    if (screen->screen_type == SCREEN_REMOTE)
    {
        logo_width = BMPWIDTH_remote_usblogo;
        logo_height = BMPHEIGHT_remote_usblogo;
    }
    else
#endif
    {
        logo_width = BMPWIDTH_usblogo;
        logo_height = BMPHEIGHT_usblogo;
    }

    viewportmanager_theme_enable(screen->screen_type, true, parent);

    if (logo_width  > parent->width)
        logo_width  = parent->width;
    if (logo_height > parent->height)
        logo_height = parent->height;

    *logo = *parent;
    logo->x = parent->x + parent->width - logo_width;
    logo->y = parent->y + (parent->height - logo_height) / 2;
    logo->width = logo_width;
    logo->height = logo_height;

#ifdef USB_ENABLE_HID
    if (usb_hid)
    {
        struct viewport *title = &usb_screen_vps->title;
        int char_height = font_get(parent->font)->height;
        *title = *parent;
        title->y = logo->y + logo->height + char_height;
        title->height = char_height;
        /* try to fit logo and title to parent */
        if (parent->y + parent->height < title->y + title->height)
        {
            logo->y = parent->y;
            title->y = parent->y + logo->height;
        }
    }
#endif
}

static void usb_screens_draw(struct usb_screen_vps_t *usb_screen_vps_ar)
{
    struct viewport *last_vp;
    static const struct bitmap* logos[NB_SCREENS] = {
        &bm_usblogo,
#ifdef HAVE_REMOTE_LCD
        &bm_remote_usblogo,
#endif
    };

    FOR_NB_SCREENS(i)
    {
        struct screen *screen = &screens[i];

        struct usb_screen_vps_t *usb_screen_vps = &usb_screen_vps_ar[i];
        struct viewport *parent = &usb_screen_vps->parent;
        struct viewport *logo = &usb_screen_vps->logo;

        last_vp = screen->set_viewport(parent);
        screen->clear_viewport();
        screen->backlight_on();

        screen->set_viewport(logo);
        screen->bmp(logos[i], 0, 0);
        if (i == SCREEN_MAIN)
        {
#ifdef USB_ENABLE_HID
            if (usb_hid)
            {
                char modestring[100];
                screen->set_viewport(&usb_screen_vps->title);
                usb_screen_vps->title.flags |= VP_FLAG_ALIGN_CENTER;
                snprintf(modestring, sizeof(modestring), "%s: %s",
                        str(LANG_USB_KEYPAD_MODE),
                        str(keypad_mode_name_get()));
                screen->puts_scroll(0, 0, modestring);
            }
#endif /* USB_ENABLE_HID */
        }
        screen->set_viewport(parent);

        screen->set_viewport(last_vp);
        screen->update_viewport();
    }
}

void gui_usb_screen_run(bool early_usb)
{
    (void) early_usb;
    struct usb_screen_vps_t usb_screen_vps_ar[NB_SCREENS];
#if defined HAVE_TOUCHSCREEN
    enum touchscreen_mode old_mode = touchscreen_get_mode();

    /* TODO: Paint buttons on screens OR switch to point mode and use
     * touchscreen as a touchpad to move the host's mouse cursor */
    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif

    push_current_activity(ACTIVITY_USBSCREEN);

#ifdef USB_ENABLE_HID
    usb_hid = global_settings.usb_hid;
    usb_keypad_mode = global_settings.usb_keypad_mode;
#endif

    FOR_NB_SCREENS(i)
    {
        struct screen *screen = &screens[i];
        /* we might be coming from anywhere, and the originating screen
         * can't be practically expected to cleanup the UI because
         * we're invoked via default_event_handler(), therefore we make a
         * generic cleanup here */
        screen->set_viewport(NULL);
        screen->scroll_stop();
        usb_screen_fix_viewports(screen, &usb_screen_vps_ar[i]);
    }

    /* update the UI before disabling fonts, this maximizes the propability
     * that font cache lookups succeed during USB */
    send_event(GUI_EVENT_ACTIONUPDATE, NULL);
    if(!early_usb)
    {
        /* The font system leaves the .fnt fd's open, so we need for force close them all */
        font_disable_all();
    }

    usb_acknowledge(SYS_USB_CONNECTED_ACK);

    while (1)
    {
        usb_screens_draw(usb_screen_vps_ar);
#ifdef SIMULATOR
        if (button_get_w_tmo(HZ/2))
            break;
        send_event(GUI_EVENT_ACTIONUPDATE, NULL);
#else
        if (handle_usb_events())
            break;
#endif /* SIMULATOR */
    }

    FOR_NB_SCREENS(i)
    {
        const struct viewport* vp = NULL;

#if defined(USB_ENABLE_HID)
        vp = usb_hid ? &usb_screen_vps_ar[i].title : NULL;
#endif
        if (vp)
            screens[i].scroll_stop_viewport(vp);
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

    if(!early_usb)
    {
        font_enable_all();
        /* Not pretty, reload all settings so fonts are loaded again correctly */
        settings_apply(true);
        /* Reload playlist */
        playlist_resume();
    }

    FOR_NB_SCREENS(i)
    {
        screens[i].backlight_on();
        viewportmanager_theme_undo(i, false);
    }

    pop_current_activity();
}
