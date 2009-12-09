/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: screens.c 23269 2009-10-19 20:06:51Z tomers $
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

#ifdef HAVE_LCD_BITMAP
#include "bitmaps/usblogo.h"
#endif

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
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                return 1;
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
#endif

#ifdef USB_NONE
void gui_usb_screen_run(void)
{
}
#else
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

#ifdef HAVE_LCD_BITMAP
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

    viewport_set_defaults(parent, screen->screen_type);
    if (parent->width < logo_width || parent->height < logo_height)
        viewportmanager_theme_enable(screen->screen_type, false, parent);

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
#endif

static void usb_screens_draw(struct usb_screen_vps_t *usb_screen_vps_ar)
{
    int i;
    lcd_clear_display();
#ifdef HAVE_LCD_REMOTE
    lcd_remote_clear_display();
#endif

    FOR_NB_SCREENS(i)
    {
        struct screen *screen = &screens[i];

        struct usb_screen_vps_t *usb_screen_vps = &usb_screen_vps_ar[i];
        struct viewport *parent = &usb_screen_vps->parent;
#ifdef HAVE_LCD_BITMAP
        struct viewport *logo = &usb_screen_vps->logo;
#endif

        screen->set_viewport(parent);
        screen->backdrop_show(BACKDROP_MAIN);
        screen->backlight_on();
        screen->clear_viewport();

#ifdef HAVE_LCD_BITMAP
        screen->set_viewport(logo);
#ifdef HAVE_REMOTE_LCD
        if (i == SCREEN_REMOTE)
        {
            screen->bitmap(remote_usblogo, 0, 0, logo->width,
                logo->height);
        }
        else
#endif
        {
            screen->transparent_bitmap(usblogo, 0, 0, logo->width,
                logo->height);
#ifdef USB_ENABLE_HID
            if (usb_hid)
            {
                screen->set_viewport(&usb_screen_vps->title);
                usb_screen_vps->title.flags |= VP_FLAG_ALIGN_CENTER;
                screen->puts_scroll(0, 0, str(keypad_mode_name_get()));
            }
#endif /* USB_ENABLE_HID */
        }
        screen->set_viewport(parent);

#else /* HAVE_LCD_BITMAP */
        screen->double_height(false);
        screen->puts_scroll(0, 0, "[USB Mode]");
        status_set_param(false);
        status_set_audio(false);
        status_set_usb(true);
#endif /* HAVE_LCD_BITMAP */

        screen->update_viewport();
        screen->set_viewport(NULL);
    }
}

void gui_usb_screen_run(void)
{
    int i;
    struct usb_screen_vps_t usb_screen_vps_ar[NB_SCREENS];
#if defined HAVE_TOUCHSCREEN
    enum touchscreen_mode old_mode = touchscreen_get_mode();

    /* TODO: Paint buttons on screens OR switch to point mode and use
     * touchscreen as a touchpad to move the host's mouse cursor */
    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif

#ifndef SIMULATOR
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
#endif

#ifdef USB_ENABLE_HID
    usb_hid = global_settings.usb_hid;
    usb_keypad_mode = global_settings.usb_keypad_mode;
#endif

    FOR_NB_SCREENS(i)
    {
        struct screen *screen = &screens[i];

        screen->set_viewport(NULL);
#ifdef HAVE_LCD_BITMAP
        usb_screen_fix_viewports(screen, &usb_screen_vps_ar[i]);
#endif
    }

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

#if defined(HAVE_LCD_BITMAP) && defined(USB_ENABLE_HID)
        vp = usb_hid ? &usb_screen_vps_ar[i].title : NULL;
#elif !defined(HAVE_LCD_BITMAP)
        vp = &usb_screen_vps_ar[i].parent;
#endif
        if (vp)
            screens[i].scroll_stop(vp);
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
    FOR_NB_SCREENS(i)
    {
        screens[i].backlight_on();
        viewportmanager_theme_undo(i);
    }

}
#endif /* !defined(USB_NONE) */

