/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Marcin Bukat
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

/* Button Code Definitions for rk27xx reference design target */

#include "config.h"
#include "action.h"
#include "button.h"

/* 
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code } 
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping 
 */
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,       BUTTON_REW,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_FF,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_CONTEXT,    BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_STD_CANCEL,     BUTTON_VOL,                 BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_STD_MENU,       BUTTON_M,                   BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,       BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_WPS_SKIPNEXT,   BUTTON_FF|BUTTON_REL,       BUTTON_FF   },
    { ACTION_WPS_SKIPPREV,   BUTTON_REW|BUTTON_REL,      BUTTON_REW  },
    { ACTION_WPS_SEEKBACK,   BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,    BUTTON_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,   BUTTON_REW|BUTTON_REL,      BUTTON_REW|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,   BUTTON_FF|BUTTON_REL,       BUTTON_FF|BUTTON_REPEAT },
    { ACTION_WPS_STOP,       BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_WPS_VOLDOWN,    BUTTON_REW|BUTTON_VOL,      BUTTON_NONE  },
    { ACTION_WPS_VOLDOWN,    BUTTON_REW|BUTTON_VOL|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,      BUTTON_FF|BUTTON_VOL,       BUTTON_NONE  },
    { ACTION_WPS_VOLUP,      BUTTON_FF|BUTTON_VOL|BUTTON_REPEAT,    BUTTON_NONE },

    { ACTION_WPS_BROWSE,     BUTTON_VOL|BUTTON_REL,      BUTTON_VOL },
    { ACTION_WPS_MENU,       BUTTON_M|BUTTON_REL,        BUTTON_M   },
    { ACTION_WPS_CONTEXT,    BUTTON_M|BUTTON_REPEAT,     BUTTON_M   },
    { ACTION_STD_KEYLOCK,    BUTTON_M|BUTTON_VOL,        BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

#ifdef CONFIG_TUNER
static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,                  BUTTON_M|BUTTON_REPEAT,             BUTTON_NONE },
    { ACTION_FM_PLAY,                  BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY },
    { ACTION_FM_STOP,                  BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_SETTINGS_INC,             BUTTON_VOL|BUTTON_FF,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,       BUTTON_VOL|BUTTON_FF|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,             BUTTON_VOL|BUTTON_REW,              BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,       BUTTON_VOL|BUTTON_REW|BUTTON_REPEAT,BUTTON_NONE },

    { ACTION_FM_EXIT,                  BUTTON_M|BUTTON_REL,                BUTTON_M },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */
#endif

#ifdef USB_ENABLE_HID
static const struct button_mapping button_context_usb_hid[] = {
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_M|BUTTON_FF|BUTTON_REL,    BUTTON_M|BUTTON_FF },
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_M|BUTTON_FF|BUTTON_REPEAT, BUTTON_M|BUTTON_FF },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_M|BUTTON_REW|BUTTON_REL,     BUTTON_M|BUTTON_REW },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_M|BUTTON_REW|BUTTON_REPEAT,  BUTTON_M|BUTTON_REW },

    LAST_ITEM_IN_LIST
}; /* button_context_usb_hid */

static const struct button_mapping button_context_usb_hid_mode_multimedia[] = {
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL|BUTTON_REW|BUTTON_REL, BUTTON_VOL|BUTTON_REW },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL|BUTTON_FF|BUTTON_REL,  BUTTON_VOL|BUTTON_FF },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,         BUTTON_VOL|BUTTON_REPEAT,         BUTTON_VOL },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE, BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,       BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV, BUTTON_REW|BUTTON_REL,            BUTTON_REW },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT, BUTTON_FF|BUTTON_REL,             BUTTON_FF },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_multimedia */

static const struct button_mapping button_context_usb_hid_mode_presentation[] = {
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_START,   BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE,   BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },
    { ACTION_USB_HID_PRESENTATION_SLIDE_PREV,        BUTTON_REW|BUTTON_REL,            BUTTON_REW },
    { ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,        BUTTON_FF|BUTTON_REL,             BUTTON_FF },
    { ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,       BUTTON_REW|BUTTON_REPEAT,         BUTTON_REW },
    { ACTION_USB_HID_PRESENTATION_SLIDE_LAST,        BUTTON_FF|BUTTON_REPEAT,          BUTTON_FF },
    { ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,      BUTTON_M|BUTTON_REL,              BUTTON_M },
    { ACTION_USB_HID_PRESENTATION_SCREEN_WHITE,      BUTTON_M|BUTTON_REPEAT,           BUTTON_M },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,         BUTTON_VOL|BUTTON_REW|BUTTON_REL, BUTTON_VOL|BUTTON_REW },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,         BUTTON_VOL|BUTTON_FF|BUTTON_REL,  BUTTON_VOL|BUTTON_FF },
    { ACTION_USB_HID_PRESENTATION_MOUSE_CLICK,       BUTTON_VOL|BUTTON_REL,            BUTTON_VOL },
    { ACTION_USB_HID_PRESENTATION_MOUSE_OVER,        BUTTON_VOL|BUTTON_REPEAT,      BUTTON_VOL },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_presentation */

static const struct button_mapping button_context_usb_hid_mode_browser[] = {
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_REW|BUTTON_REPEAT,                BUTTON_REW },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_FF|BUTTON_REPEAT,                 BUTTON_FF },
    { ACTION_USB_HID_BROWSER_SCROLL_PAGE_UP,   BUTTON_PLAY|BUTTON_REL,                  BUTTON_PLAY },
    { ACTION_USB_HID_BROWSER_SCROLL_PAGE_DOWN, BUTTON_M|BUTTON_REL,                     BUTTON_M },
    { ACTION_USB_HID_BROWSER_ZOOM_IN,          BUTTON_PLAY|BUTTON_REPEAT,               BUTTON_PLAY },
    { ACTION_USB_HID_BROWSER_ZOOM_OUT,         BUTTON_M|BUTTON_REPEAT,                  BUTTON_M },
    { ACTION_USB_HID_BROWSER_ZOOM_RESET,       BUTTON_PLAY|BUTTON_M|BUTTON_REPEAT,      BUTTON_PLAY|BUTTON_M },
    { ACTION_USB_HID_BROWSER_TAB_PREV,         BUTTON_REW|BUTTON_REL,                   BUTTON_REW },
    { ACTION_USB_HID_BROWSER_TAB_NEXT,         BUTTON_FF|BUTTON_REL,                    BUTTON_FF },
    { ACTION_USB_HID_BROWSER_TAB_CLOSE,        BUTTON_VOL|BUTTON_REPEAT,                BUTTON_VOL },
    { ACTION_USB_HID_BROWSER_HISTORY_BACK,     BUTTON_VOL|BUTTON_REW|BUTTON_REL,        BUTTON_VOL|BUTTON_REW },
    { ACTION_USB_HID_BROWSER_HISTORY_FORWARD,  BUTTON_VOL|BUTTON_FF|BUTTON_REL,         BUTTON_VOL|BUTTON_FF },
    { ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN, BUTTON_VOL|BUTTON_REL,                   BUTTON_VOL },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_browser */

#ifdef HAVE_USB_HID_MOUSE
static const struct button_mapping button_context_usb_hid_mode_mouse[] = {
    { ACTION_USB_HID_MOUSE_UP,                BUTTON_PLAY,                              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_UP_REP,            BUTTON_PLAY|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN,              BUTTON_M,                                 BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN_REP,          BUTTON_M|BUTTON_REPEAT,                   BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT,              BUTTON_REW,                               BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT_REP,          BUTTON_REW|BUTTON_REPEAT,                 BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT,             BUTTON_FF,                                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT_REP,         BUTTON_FF|BUTTON_REPEAT,                  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT,       BUTTON_VOL,                               BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT_REL,   BUTTON_VOL|BUTTON_REL,                    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_SCROLL_BACK,                       BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_SCROLL_BACK|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_SCROLL_FWD,                        BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_SCROLL_FWD|BUTTON_REPEAT,          BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_mouse */
#endif
#endif

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
#ifdef CONFIG_TUNER
        case CONTEXT_FM:
             return button_context_radio;
#endif
#ifdef USB_ENABLE_HID
        case CONTEXT_USB_HID:
            return button_context_usb_hid;
        case CONTEXT_USB_HID_MODE_MULTIMEDIA:
            return button_context_usb_hid_mode_multimedia;
        case CONTEXT_USB_HID_MODE_PRESENTATION:
            return button_context_usb_hid_mode_presentation;
        case CONTEXT_USB_HID_MODE_BROWSER:
            return button_context_usb_hid_mode_browser;
#ifdef HAVE_USB_HID_MOUSE
        case CONTEXT_USB_HID_MODE_MOUSE:
            return button_context_usb_hid_mode_mouse;
#endif
#endif
        case CONTEXT_TREE:
        case CONTEXT_LIST:
        case CONTEXT_MAINMENU:
            
        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS|CONTEXT_REMOTE:
        default:
            return button_context_standard;
    } 
    return button_context_standard;
}
