/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Lorenzo Miori
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

/* Button Code Definitions for the Creative Zen Vision target */
/* Copied from ZVM target for now... */
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

/* CONTEXT_CUSTOM's used in this file...

CONTEXT_CUSTOM|CONTEXT_TREE = the standard list/tree defines (without directions)
CONTEXT_CUSTOM|CONTEXT_SETTINGS = the direction keys for the eq/col picker screens
                                  i.e where up/down is inc/dec
               CONTEXT_SETTINGS = up/down is prev/next, l/r is inc/dec

*/

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,          BUTTON_UP|BUTTON_REL,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_UP|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_DOWN|BUTTON_REL,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,             BUTTON_NONE },
    
    { ACTION_STD_PREV,          BUTTON_FF|BUTTON_REL,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_FF|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_REW|BUTTON_REL,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_REW|BUTTON_REPEAT,             BUTTON_NONE },

    { ACTION_STD_CANCEL,        BUTTON_BACK|BUTTON_REL,                BUTTON_NONE },

    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,           BUTTON_SELECT },

    //{ ACTION_STD_QUICKSCREEN,   BUTTON_BACK|BUTTON_REPEAT,             BUTTON_BACK },
    { ACTION_STD_MENU,          BUTTON_BACK|BUTTON_REPEAT,                BUTTON_NONE },

    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,              BUTTON_SELECT },
//    { ACTION_STD_OK,            BUTTON_RIGHT,                          BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */


static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,          BUTTON_POWER|BUTTON_REL,         BUTTON_POWER },
    //{ ACTION_WPS_STOP,          BUTTON_POWER|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_WPS_SKIPNEXT,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },

    { ACTION_WPS_SEEKBACK,      BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,       BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },

    { ACTION_WPS_VOLDOWN,       BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_VOL_DOWN,                    BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_VOL_UP,                      BUTTON_NONE },

    //{ ACTION_WPS_PITCHSCREEN,   BUTTON_BACK|BUTTON_REPEAT,      BUTTON_BACK },

    { ACTION_WPS_QUICKSCREEN,   BUTTON_BACK|BUTTON_REPEAT,      BUTTON_BACK },
    { ACTION_WPS_MENU,          BUTTON_BACK|BUTTON_REL,         BUTTON_BACK },
    { ACTION_WPS_VIEW_PLAYLIST, BUTTON_POWER,                   BUTTON_NONE },
    { ACTION_WPS_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT },

    { ACTION_WPS_HOTKEY,        BUTTON_BACK|BUTTON_REL,         BUTTON_NONE },
    { ACTION_WPS_BROWSE,        BUTTON_SELECT|BUTTON_REL,       BUTTON_BACK },
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[]  = {
#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLUP,   BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_LIST_VOLUP,   BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    //{ ACTION_TREE_WPS,    BUTTON_POWER|BUTTON_REL,         BUTTON_POWER },
    //{ ACTION_TREE_STOP,   BUTTON_POWER,                   BUTTON_NONE },
    //{ ACTION_TREE_STOP,   BUTTON_POWER|BUTTON_REL,        BUTTON_POWER },
    //{ ACTION_TREE_STOP,   BUTTON_POWER|BUTTON_REPEAT,     BUTTON_NONE },
    //{ ACTION_TREE_HOTKEY, BUTTON_BACK|BUTTON_REL,         BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_BACK|BUTTON_REL,     BUTTON_BACK },
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_LEFT },
    { ACTION_STD_NEXT,              BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_RIGHT },
    { ACTION_SETTINGS_RESET,        BUTTON_POWER,                    BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_RESET,        BUTTON_BACK,                BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,                  BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = {
    { ACTION_STD_OK,            BUTTON_BACK|BUTTON_REL,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_colorchooser */

static const struct button_mapping button_context_eq[]  = {
    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_eq */

/** Bookmark Screen **/
static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,       BUTTON_BACK, BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_time */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_TOP,        BUTTON_UP,                      BUTTON_NONE },
    { ACTION_QS_TOP,        BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_BACK,                    BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                             BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                            BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_POWER,                             BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_POWER|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_SELECT,                           BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_BACK|BUTTON_BACK,                 BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_POWER|BUTTON_REL,                  BUTTON_POWER },
    { ACTION_KBD_ABORT,        BUTTON_BACK|BUTTON_REL,                  BUTTON_BACK },
    { ACTION_KBD_BACKSPACE,    BUTTON_BACK,                             BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_BACK|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                               BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,                 BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                             BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_SELECT|BUTTON_REL,                BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

/** FM Radio Screen **/
#if CONFIG_TUNER
static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,        BUTTON_SELECT | BUTTON_REPEAT,     BUTTON_SELECT },
    { ACTION_FM_MODE,        BUTTON_SELECT,                     BUTTON_SELECT },
//    { ACTION_FM_PRESET,      BUTTON_STOP,                     BUTTON_NONE },
    { ACTION_FM_PLAY,        BUTTON_POWER | BUTTON_REL,         BUTTON_NONE },
    { ACTION_FM_STOP,        BUTTON_POWER | BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_FM_EXIT,        BUTTON_BACK,                       BUTTON_NONE },
    
    //{ ACTION_SETTINGS_INC,      BUTTON_VOL_UP,                  BUTTON_NONE },
    //{ ACTION_SETTINGS_INCREPEAT,BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    //{ ACTION_SETTINGS_DEC,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    //{ ACTION_SETTINGS_DECREPEAT,BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */
#endif

#ifdef HAVE_RECORDING
static const struct button_mapping button_context_recscreen[]  = {
 
    { ACTION_REC_PAUSE,                BUTTON_SELECT,                      BUTTON_NONE },
    { ACTION_SETTINGS_INC,             BUTTON_RIGHT,                       BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,       BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_SETTINGS_DEC,             BUTTON_LEFT,                        BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,       BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */
#endif

#ifdef USB_ENABLE_HID
static const struct button_mapping button_context_usb_hid[] = {
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_BACK|BUTTON_REL,            BUTTON_BACK },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_BACK|BUTTON_REPEAT,         BUTTON_BACK },

    LAST_ITEM_IN_LIST
}; /* button_context_usb_hid */

static const struct button_mapping button_context_usb_hid_mode_multimedia[] = {

    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,         BUTTON_SELECT|BUTTON_REL,      BUTTON_SELECT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE, BUTTON_POWER|BUTTON_REL,       BUTTON_POWER },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,       BUTTON_POWER|BUTTON_REPEAT,    BUTTON_POWER },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV, BUTTON_REW|BUTTON_REL,         BUTTON_REW },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT, BUTTON_FF|BUTTON_REL,          BUTTON_FF },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_multimedia */


static const struct button_mapping button_context_usb_hid_mode_presentation[] = {
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_START, BUTTON_POWER|BUTTON_REL,     BUTTON_POWER },
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE, BUTTON_POWER|BUTTON_REPEAT,  BUTTON_POWER },
    { ACTION_USB_HID_PRESENTATION_SLIDE_PREV,      BUTTON_LEFT|BUTTON_REL,          BUTTON_LEFT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,      BUTTON_RIGHT|BUTTON_REL,         BUTTON_RIGHT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,     BUTTON_LEFT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_SLIDE_LAST,      BUTTON_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },
    //{ ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,    BUTTON_BOTTOMRIGHT|BUTTON_REPEAT, BUTTON_BOTTOMRIGHT },
    //{ ACTION_USB_HID_PRESENTATION_SCREEN_WHITE,    BUTTON_BOTTOMLEFT|BUTTON_REPEAT, BUTTON_BOTTOMLEFT },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,       BUTTON_UP,                       BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,       BUTTON_UP|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,       BUTTON_DOWN,                     BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,       BUTTON_DOWN|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_MOUSE_CLICK,     BUTTON_SELECT,                   BUTTON_SELECT },
    { ACTION_USB_HID_PRESENTATION_MOUSE_OVER,      BUTTON_SELECT|BUTTON_REPEAT,     BUTTON_SELECT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_presentation */

static const struct button_mapping button_context_usb_hid_mode_browser[] = {
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_UP,                           BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_UP|BUTTON_REPEAT,             BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_DOWN,                         BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_DOWN|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_ZOOM_IN,          BUTTON_VOL_UP,                       BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_ZOOM_IN,          BUTTON_VOL_UP|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_ZOOM_OUT,         BUTTON_VOL_DOWN,                     BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_ZOOM_OUT,         BUTTON_VOL_DOWN|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_ZOOM_RESET,       BUTTON_POWER|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_TAB_PREV,         BUTTON_LEFT|BUTTON_REL,              BUTTON_LEFT },
    { ACTION_USB_HID_BROWSER_TAB_NEXT,         BUTTON_RIGHT|BUTTON_REL,             BUTTON_RIGHT },
    { ACTION_USB_HID_BROWSER_TAB_CLOSE,        BUTTON_BACK|BUTTON_REPEAT,           BUTTON_BACK },
    { ACTION_USB_HID_BROWSER_HISTORY_BACK,     BUTTON_LEFT|BUTTON_REPEAT,           BUTTON_LEFT },
    { ACTION_USB_HID_BROWSER_HISTORY_FORWARD,  BUTTON_RIGHT|BUTTON_REPEAT,          BUTTON_RIGHT },
    { ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN, BUTTON_SELECT|BUTTON_REPEAT,         BUTTON_SELECT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_browser */

#ifdef HAVE_USB_HID_MOUSE
static const struct button_mapping button_context_usb_hid_mode_mouse[] = {
    { ACTION_USB_HID_MOUSE_UP,                BUTTON_UP,                            BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_UP_REP,            BUTTON_UP|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN,              BUTTON_DOWN,                          BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN_REP,          BUTTON_DOWN|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT,              BUTTON_LEFT,                          BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT_REP,          BUTTON_LEFT|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT,             BUTTON_RIGHT,                         BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT_REP,         BUTTON_RIGHT|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT,       BUTTON_SELECT,                        BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT_REL,   BUTTON_SELECT|BUTTON_REL,             BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_RIGHT,      BUTTON_POWER,                         BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_RIGHT_REL,  BUTTON_POWER|BUTTON_REL,              BUTTON_POWER },

    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_FF,                            BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_FF|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_REW,                           BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_REW|BUTTON_REPEAT,             BUTTON_NONE },
 
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_mouse */
#endif
#endif

const struct button_mapping* get_context_mapping(int context)
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
            return button_context_listtree_scroll_without_combo;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;
#if CONFIG_TUNER
        case CONTEXT_FM:
            return button_context_radio;
#endif
        case CONTEXT_SETTINGS:
            return button_context_settings;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
            return button_context_settings_right_is_inc;
        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_colorchooser;
        case CONTEXT_SETTINGS_EQ:
            return button_context_eq;
        case CONTEXT_SETTINGS_TIME:
            return button_context_time;
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_KEYBOARD:
        case CONTEXT_MORSE_INPUT:
            return button_context_keyboard;
#ifdef HAVE_RECORDING
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings_right_is_inc;
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
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
    }
    return button_context_standard;
}
