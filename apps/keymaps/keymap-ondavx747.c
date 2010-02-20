/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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

/* Button Code Definitions for the Onda VX747 target */
/* NB: Up/Down/Left/Right are not physical buttons - touchscreen emulation */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },

    { ACTION_STD_OK,          BUTTON_MENU|BUTTON_REL,        BUTTON_MENU },
    { ACTION_STD_CANCEL,      BUTTON_POWER,                  BUTTON_NONE },
    
    { ACTION_STD_QUICKSCREEN, BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_CONTEXT,     BUTTON_MENU|BUTTON_REPEAT,     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_standard */


static const struct button_mapping button_context_wps[]  = {

    { ACTION_WPS_VOLDOWN,       BUTTON_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_VOL_UP,                   BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_MENU,          BUTTON_MENU|BUTTON_REL,          BUTTON_MENU },
    { ACTION_WPS_CONTEXT,       BUTTON_MENU|BUTTON_REPEAT,       BUTTON_MENU },
    
    { ACTION_WPS_MENU,          BUTTON_POWER,                    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_OK,                BUTTON_MENU,                   BUTTON_NONE },
    { ACTION_STD_CANCEL,            BUTTON_POWER,                  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_right_is_inc[]  = {

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_MENU,              BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_colorchooser */

static const struct button_mapping button_context_eq[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_eq */

/** Bookmark Screen **/
static const struct button_mapping button_context_bmark[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {
    { ACTION_SETTINGS_INC,       BUTTON_VOL_UP,                   BUTTON_NONE  },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE  },
    { ACTION_SETTINGS_DEC,       BUTTON_VOL_DOWN,                 BUTTON_NONE  },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE  },
    { ACTION_STD_OK,             BUTTON_MENU|BUTTON_REL,          BUTTON_MENU  },
    { ACTION_STD_CANCEL,         BUTTON_POWER|BUTTON_REL,         BUTTON_POWER },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_time */

static const struct button_mapping button_context_quickscreen[]  = {

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {

    { ACTION_PS_INC_SMALL, BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_PS_INC_SMALL, BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_PS_DEC_SMALL, BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_PS_DEC_SMALL, BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_EXIT,      BUTTON_POWER,                  BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

/** FM Radio Screen **/
static const struct button_mapping button_context_radio[]  = {
    { ACTION_STD_PREV,       BUTTON_VOL_DOWN,                   BUTTON_NONE   },
    { ACTION_STD_PREVREPEAT, BUTTON_VOL_DOWN|BUTTON_REPEAT,     BUTTON_NONE   },
    { ACTION_STD_NEXT,       BUTTON_VOL_UP,                     BUTTON_NONE   },
    { ACTION_STD_NEXTREPEAT, BUTTON_VOL_UP|BUTTON_REPEAT,       BUTTON_NONE   },
    { ACTION_FM_MENU,        BUTTON_MENU|BUTTON_REPEAT,         BUTTON_NONE   },
    { ACTION_FM_PRESET,      BUTTON_MENU|BUTTON_REL,            BUTTON_NONE   },
    { ACTION_FM_MODE,        BUTTON_POWER|BUTTON_REL,           BUTTON_POWER  },
    { ACTION_FM_EXIT,        BUTTON_POWER|BUTTON_REPEAT,        BUTTON_NONE   },
    //{ ACTION_FM_QUICKSCREEN, BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_PAGE_FLIP,    BUTTON_MENU,                     BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_VOL_UP,                   BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_MENU|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_POWER,                    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_keyboard */

#ifdef USB_ENABLE_HID
static const struct button_mapping button_context_usb_hid[] = {
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_POWER|BUTTON_REL,    BUTTON_POWER },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_POWER|BUTTON_REPEAT, BUTTON_POWER },

    LAST_ITEM_IN_LIST
}; /* button_context_usb_hid */

static const struct button_mapping button_context_usb_hid_mode_multimedia[] = {
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN,                             BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP,                               BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP|BUTTON_REPEAT,                 BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,         BUTTON_VOL_DOWN|BUTTON_VOL_UP|BUTTON_REPEAT, BUTTON_VOL_DOWN|BUTTON_VOL_UP },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE, BUTTON_MENU|BUTTON_REL,                      BUTTON_MENU },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV, BUTTON_MENU|BUTTON_LEFT|BUTTON_REL,          BUTTON_MENU|BUTTON_LEFT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT, BUTTON_MENU|BUTTON_RIGHT|BUTTON_REL,         BUTTON_MENU|BUTTON_RIGHT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_multimedia */

static const struct button_mapping button_context_usb_hid_mode_presentation[] = {
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_START, BUTTON_MENU|BUTTON_REL,                      BUTTON_MENU },
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE, BUTTON_MENU|BUTTON_REPEAT,                   BUTTON_MENU },
    { ACTION_USB_HID_PRESENTATION_SLIDE_PREV,      BUTTON_VOL_DOWN|BUTTON_REL,                  BUTTON_VOL_DOWN },
    { ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,      BUTTON_VOL_UP|BUTTON_REL,                    BUTTON_VOL_UP },
    { ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,     BUTTON_VOL_DOWN|BUTTON_REPEAT,               BUTTON_VOL_DOWN },
    { ACTION_USB_HID_PRESENTATION_SLIDE_LAST,      BUTTON_VOL_UP|BUTTON_REPEAT,                 BUTTON_VOL_UP },
    { ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,    BUTTON_VOL_DOWN|BUTTON_VOL_UP|BUTTON_REPEAT, BUTTON_VOL_DOWN|BUTTON_VOL_UP },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_presentation */

static const struct button_mapping button_context_usb_hid_mode_browser[] = {
    { ACTION_USB_HID_BROWSER_SCROLL_UP,           BUTTON_VOL_DOWN,                        BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_UP,           BUTTON_VOL_DOWN|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,         BUTTON_VOL_UP,                          BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,         BUTTON_VOL_UP|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_TAB_PREV,            BUTTON_MENU|BUTTON_VOL_UP|BUTTON_REL,   BUTTON_MENU|BUTTON_VOL_UP },
    { ACTION_USB_HID_BROWSER_TAB_NEXT,            BUTTON_MENU|BUTTON_VOL_DOWN|BUTTON_REL, BUTTON_MENU|BUTTON_VOL_DOWN },
    { ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN,    BUTTON_MENU|BUTTON_REL,                 BUTTON_MENU },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_browser */
#endif

const struct button_mapping* target_get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_MAINMENU:
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_listtree_scroll_without_combo;
            else
                return button_context_listtree_scroll_with_combo;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_SETTINGS:
            return button_context_settings;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings_right_is_inc;

        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_colorchooser;
        case CONTEXT_SETTINGS_EQ:
            return button_context_eq;

        case CONTEXT_SETTINGS_TIME:
            return button_context_time;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_FM:
            return button_context_radio;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
#ifdef USB_ENABLE_HID
        case CONTEXT_USB_HID:
            return button_context_usb_hid;
        case CONTEXT_USB_HID_MODE_MULTIMEDIA:
            return button_context_usb_hid_mode_multimedia;
        case CONTEXT_USB_HID_MODE_PRESENTATION:
            return button_context_usb_hid_mode_presentation;
        case CONTEXT_USB_HID_MODE_BROWSER:
            return button_context_usb_hid_mode_browser;
#endif
    }
    return button_context_standard;
}
