/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 Tomer Shalev
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

#include "plugin.h"

#include "lib/pluginlib_actions.h"

PLUGIN_HEADER

#define PLUGIN_CONTINUE 10

static inline void remote_control_setcolors(void);

/*****************************************************************************
* remote_control_setcolors() set the foreground and background colors.
******************************************************************************/
static inline void remote_control_setcolors(void)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(181, 181, 222));
    rb->lcd_set_foreground(LCD_BLACK);
#endif
}

static int menu_desktop(void)
{
    int selection = 0;

    MENUITEM_STRINGLIST(menu, "Desktop", NULL, "Escape", "Windows", "F10",
            "Page Up", "Page Down");
    while(1)
    {
        int id = HID_GENERIC_DESKTOP_UNDEFINED;

        switch (rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0: /* Escape */
                id = HID_KEYBOARD_ESCAPE;
                break;
            case 1: /* Windows */
                /* Not sure whether this is the right key */
                id = HID_KEYBOARD_LEFT_GUI;
                break;
            case 2: /* F10 */
                id = HID_KEYBOARD_F10;
                break;
            case 3: /* Page Up */
                id = HID_KEYBOARD_PAGE_UP;
                break;
            case 4: /* Page Down */
                id = HID_KEYBOARD_PAGE_DOWN;
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            case GO_TO_PREVIOUS:
                return PLUGIN_CONTINUE;
            default:
                break;
        }

        if (id != HID_GENERIC_DESKTOP_UNDEFINED)
            rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, id);
    }
}

static int menu_presentation(void)
{
    int selection = 0;

    MENUITEM_STRINGLIST(menu, "Presentation", NULL, "Next Slide", "Prev Slide",
            "Start Slideshow", "Leave Slideshow", "Black Screen",
            "White Screen");
    while(1)
    {
        int id = HID_GENERIC_DESKTOP_UNDEFINED;

        switch (rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0: /* Next Slide */
                id = HID_KEYBOARD_N;
                break;
            case 1: /* Prev Slide */
                id = HID_KEYBOARD_P;
                break;
            case 2: /* Start Slideshow */
                id = HID_KEYBOARD_F5;
                break;
            case 3: /* Leave Slideshow */
                id = HID_KEYBOARD_ESCAPE;
                break;
            case 4: /* Black Screen */
                id = HID_KEYBOARD_DOT;
                break;
            case 5: /* White Screen */
                id = HID_KEYBOARD_COMMA;
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            case GO_TO_PREVIOUS:
                return PLUGIN_CONTINUE;
            default:
                break;
        }

        if (id != HID_GENERIC_DESKTOP_UNDEFINED)
            rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, id);
    }
}

static int menu_media_player(void)
{
    int selection = 0;

    MENUITEM_STRINGLIST(menu, "Media Player", NULL, "Play", "Stop", "Next",
            "Previous", "Volume Up", "Volume Down", "Mute");
    while(1)
    {
        int id = HID_CONSUMER_USAGE_UNASSIGNED;

        switch (rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0: /* Play */
                id = HID_CONSUMER_USAGE_PLAY_PAUSE;
                break;
            case 1: /* Stop */
                id = HID_CONSUMER_USAGE_STOP;
                break;
            case 2: /* Next */
                id = HID_CONSUMER_USAGE_SCAN_NEXT_TRACK;
                break;
            case 3: /* Previous */
                id = HID_CONSUMER_USAGE_SCAN_PREVIOUS_TRACK;
                break;
            case 4: /* Volume Up */
                id = HID_CONSUMER_USAGE_VOLUME_INCREMENT;
                break;
            case 5: /* Volume Down */
                id = HID_CONSUMER_USAGE_VOLUME_DECREMENT;
                break;
            case 6: /* Mute */
                id = HID_CONSUMER_USAGE_MUTE;
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            case GO_TO_PREVIOUS:
                return PLUGIN_CONTINUE;
            default:
                break;
        }

        if (id != HID_CONSUMER_USAGE_UNASSIGNED)
            rb->usb_hid_send(HID_USAGE_PAGE_CONSUMER, id);
    }
}

/*****************************************************************************
* plugin entry point.
******************************************************************************/
enum plugin_status plugin_start(const void* parameter)
{
    enum plugin_status rc = PLUGIN_CONTINUE;
    int selection = 0;

    (void)parameter;

    rb->lcd_clear_display();

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    rb->lcd_setfont(FONT_SYSFIXED);

    remote_control_setcolors();

    MENUITEM_STRINGLIST(menu, "Remote Control", NULL, "Desktop", "Presentation",
            "Media Player", "Quit");
    while(rc == PLUGIN_CONTINUE)
    {
        switch (rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0: /* Desktop */
                rc = menu_desktop();
                break;
            case 1: /* Presentation */
                rc = menu_presentation();
                break;
            case 2: /* Media Player */
                rc = menu_media_player();
                break;
            case 3: /* Quit */
            case GO_TO_PREVIOUS:
                rc = PLUGIN_OK;
                break;
            case MENU_ATTACHED_USB:
                rc = PLUGIN_USB_CONNECTED;
                break;
            default:
                break;
        }
    }
    rb->lcd_setfont(FONT_UI);

    return rc;
}

