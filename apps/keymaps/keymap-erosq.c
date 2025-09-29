/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 Solomon Peachy
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

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* {Action Code,    Button code,    Prereq button code } */

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,       BUTTON_SCROLL_BACK,            BUTTON_NONE },
    { ACTION_STD_PREV,       BUTTON_PREV,                   BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_PREV|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_SCROLL_FWD,             BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_NEXT,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_NEXT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_STD_HOTKEY,     BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_STD_CANCEL,     BUTTON_BACK|BUTTON_REL,        BUTTON_BACK },
    { ACTION_STD_CONTEXT,    BUTTON_MENU|BUTTON_REPEAT,     BUTTON_MENU },
    { ACTION_STD_MENU,       BUTTON_MENU|BUTTON_REL,        BUTTON_MENU },
    { ACTION_STD_KEYLOCK,    BUTTON_POWER|BUTTON_REL,       BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_mainmenu[]  = {

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_TREE),
}; /* button_context_mainmenu as with sansa clip mapping - "back" button returns you to WPS */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_BROWSE,      BUTTON_BACK|BUTTON_REL,        BUTTON_BACK },
    { ACTION_WPS_PLAY,        BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_WPS_SEEKBACK,    BUTTON_PREV|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,     BUTTON_NEXT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_NEXT|BUTTON_REL,        BUTTON_NEXT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,    BUTTON_PREV|BUTTON_REL,        BUTTON_PREV|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,    BUTTON_NEXT|BUTTON_REL,        BUTTON_NEXT },
    { ACTION_WPS_SKIPPREV,    BUTTON_PREV|BUTTON_REL,        BUTTON_PREV },
    { ACTION_WPS_QUICKSCREEN, BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_WPS_HOTKEY,      BUTTON_SCROLL_BACK,            BUTTON_NONE },
    { ACTION_WPS_HOTKEY,      BUTTON_SCROLL_FWD,             BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
/*    ACTION_WPS_ID3SCREEN    optional */
    { ACTION_WPS_CONTEXT,     BUTTON_MENU|BUTTON_REPEAT,     BUTTON_MENU },
    { ACTION_WPS_MENU,        BUTTON_MENU|BUTTON_REL,        BUTTON_MENU }, /* this should be the same as ACTION_STD_MENU */
/*    ACTION_WPS_VIEW_PLAYLIST
 *    ACTION_WPS_LIST_BOOKMARKS,  optional
 *    ACTION_WPS_CREATE_BOOKMARK, optional
 */
    { ACTION_STD_KEYLOCK,     BUTTON_POWER|BUTTON_REL,                 BUTTON_POWER },
    { ACTION_WPS_STOP,        BUTTON_POWER|BUTTON_REPEAT,               BUTTON_POWER },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,      BUTTON_SCROLL_FWD,               BUTTON_NONE },
    { ACTION_SETTINGS_INCBIGSTEP, BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_SCROLL_BACK,             BUTTON_NONE },
    { ACTION_SETTINGS_DECBIGSTEP, BUTTON_VOL_DOWN,               BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings */

static const struct button_mapping button_context_list[]  = {
/*    ACTION_LISTTREE_PGUP, optional
 *    ACTION_LISTTREE_PGDOWN, optional
 */
    { ACTION_TREE_WPS,       BUTTON_BACK|BUTTON_REPEAT,      BUTTON_BACK }, // back returns to WPS from many contexts

#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLUP,   BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_LIST_VOLUP,   BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
/*
    { ACTION_TREE_WPS,        BUTTON_OPTION|BUTTON_REL,         BUTTON_OPTION },
    { ACTION_TREE_STOP,       BUTTON_POWER|BUTTON_REL,          BUTTON_POWER },
*/
    { ACTION_TREE_HOTKEY,     BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },
    { ACTION_STD_MENU,        BUTTON_MENU|BUTTON_REL,                      BUTTON_MENU },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_tree */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT, BUTTON_PLAY, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_TOP,   BUTTON_PREV|BUTTON_REL,        BUTTON_NONE },
    { ACTION_QS_TOP,   BUTTON_PREV|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_QS_RIGHT,  BUTTON_SCROLL_FWD,  BUTTON_NONE },
    { ACTION_QS_LEFT,  BUTTON_SCROLL_BACK, BUTTON_NONE },
    { ACTION_QS_DOWN, BUTTON_NEXT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_QS_DOWN, BUTTON_NEXT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL, BUTTON_BACK, BUTTON_NONE },
    { ACTION_STD_CONTEXT,BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU },

    LAST_ITEM_IN_LIST
}; /* button_context_quickscreen */

static const struct button_mapping button_context_settings_time[] = {
    { ACTION_STD_PREV,           BUTTON_PREV|BUTTON_REL,     BUTTON_PREV },
    { ACTION_STD_PREVREPEAT,     BUTTON_PREV|BUTTON_REPEAT,  BUTTON_PREV },
    { ACTION_STD_NEXT,           BUTTON_NEXT|BUTTON_REL,     BUTTON_NEXT },
    { ACTION_STD_NEXTREPEAT,     BUTTON_NEXT|BUTTON_REPEAT,  BUTTON_NEXT },
    { ACTION_STD_CANCEL,         BUTTON_BACK|BUTTON_REL,     BUTTON_BACK },
    { ACTION_STD_OK,             BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_SETTINGS_INC,       BUTTON_SCROLL_FWD,          BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_SCROLL_BACK,         BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_settings_time */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_SCROLL_FWD,            BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_VOL_UP,                BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_SCROLL_BACK,           BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_VOL_DOWN,              BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_PREV,                  BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_NEXT,                  BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_PLAY|BUTTON_REL,       BUTTON_PLAY },
    { ACTION_PS_RESET,          BUTTON_PLAY|BUTTON_REPEAT,    BUTTON_PLAY },
    { ACTION_PS_EXIT,           BUTTON_BACK,                  BUTTON_NONE },
    { ACTION_PS_SLOWER,         BUTTON_PREV|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_NEXT|BUTTON_REPEAT,    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_pitchscreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_SCROLL_BACK,                      BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_SCROLL_FWD,                       BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_VOL_UP,                           BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_VOL_DOWN,                         BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_PREV,                             BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_PREV|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_NEXT,                             BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_NEXT|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_MENU|BUTTON_REL,                  BUTTON_MENU },
    { ACTION_KBD_BACKSPACE,    BUTTON_BACK,                             BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_BACK|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_PLAY|BUTTON_REL,                  BUTTON_PLAY },
    { ACTION_KBD_DONE,         BUTTON_PLAY|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_POWER,                            BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_MENU|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_PLAY|BUTTON_REL,                  BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,      BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_usb_hid[] = {
    {ACTION_USB_HID_MODE_SWITCH_NEXT,               BUTTON_POWER,        BUTTON_NONE},
    LAST_ITEM_IN_LIST,
}; /* button_context_usb_hid */

static const struct button_mapping button_context_usb_hid_mode_multimedia[] = {
    {ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP,                  BUTTON_NONE},
    {ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE},
    {ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_SCROLL_FWD,              BUTTON_NONE}, // might be annoying
    {ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN,                BUTTON_NONE},
    {ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
    {ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_SCROLL_BACK,             BUTTON_NONE}, // might be annoying
    {ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,         BUTTON_MENU|BUTTON_REL,         BUTTON_MENU},
    {ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE, BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY},
    {ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,       BUTTON_POWER|BUTTON_REPEAT,     BUTTON_POWER},
    {ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV, BUTTON_PREV,                    BUTTON_NONE},
    {ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT, BUTTON_NEXT,                    BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_multimedia */

static const struct button_mapping button_context_usb_hid_mode_presentation[] = {
    // TODO
    // {ACTION_USB_HID_PRESENTATION_SLIDESHOW_START,   BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY},
    // {ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE,   BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY},
    // {ACTION_USB_HID_PRESENTATION_SLIDE_PREV,        BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT},
    // {ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,        BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT},
    // {ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_LEFT},
    // {ACTION_USB_HID_PRESENTATION_SLIDE_LAST,        BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_RIGHT},
    // {ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,      BUTTON_VOL_UP,                  BUTTON_NONE},
    // {ACTION_USB_HID_PRESENTATION_SCREEN_WHITE,      BUTTON_VOL_DOWN,                BUTTON_NONE},
    // {ACTION_USB_HID_PRESENTATION_LINK_PREV,         BUTTON_MENU,                    BUTTON_NONE},
    // {ACTION_USB_HID_PRESENTATION_LINK_NEXT,         BUTTON_BACK,                    BUTTON_NONE},
    // {ACTION_USB_HID_PRESENTATION_MOUSE_CLICK,       BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT},
    // {ACTION_USB_HID_PRESENTATION_MOUSE_OVER,        BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_presentation */

static const struct button_mapping button_context_usb_hid_mode_browser[] = {
    // TODO
    // {ACTION_USB_HID_BROWSER_SCROLL_UP,              BUTTON_SCROLL_BACK,                 BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_SCROLL_UP,              BUTTON_SCROLL_BACK|BUTTON_REPEAT,   BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_SCROLL_DOWN,            BUTTON_SCROLL_FWD,                  BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_SCROLL_DOWN,            BUTTON_SCROLL_FWD|BUTTON_REPEAT,    BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_SCROLL_PAGE_DOWN,       BUTTON_DOWN,                        BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_SCROLL_PAGE_UP,         BUTTON_UP,                          BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_SCROLL_PAGE_DOWN,       BUTTON_DOWN|BUTTON_REPEAT,          BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_SCROLL_PAGE_UP,         BUTTON_UP|BUTTON_REPEAT,            BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_ZOOM_IN,                BUTTON_VOL_UP,                      BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_ZOOM_OUT,               BUTTON_VOL_DOWN,                    BUTTON_NONE},
    // {ACTION_USB_HID_BROWSER_ZOOM_RESET,             BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    // {ACTION_USB_HID_BROWSER_TAB_PREV,               BUTTON_LEFT|BUTTON_REL,             BUTTON_LEFT},
    // {ACTION_USB_HID_BROWSER_TAB_NEXT,               BUTTON_RIGHT|BUTTON_REL,            BUTTON_RIGHT},
    // {ACTION_USB_HID_BROWSER_TAB_CLOSE,              BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT},
    // {ACTION_USB_HID_BROWSER_HISTORY_BACK,           BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_LEFT},
    // {ACTION_USB_HID_BROWSER_HISTORY_FORWARD,        BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_RIGHT},
    // {ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN,       BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_PLAY},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_browser */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context & ~CONTEXT_LOCKED)
    {
        case CONTEXT_STD:
            return button_context_standard;

        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_MAINMENU:
            return button_context_mainmenu;

        case CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_LIST:
            return button_context_list;

        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_settings;

        case CONTEXT_SETTINGS_TIME:
            return button_context_settings_time;

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

        case CONTEXT_USB_HID:
            return button_context_usb_hid;

        case CONTEXT_USB_HID_MODE_MULTIMEDIA:
            return button_context_usb_hid_mode_multimedia;

        case CONTEXT_USB_HID_MODE_PRESENTATION:
            return button_context_usb_hid_mode_presentation;

        case CONTEXT_USB_HID_MODE_BROWSER:
            return button_context_usb_hid_mode_browser;

        default:
            return button_context_standard;
    }
    return button_context_standard;
}
