/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © Rafaël Carré 2008
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
/* Button Code Definitions for Sansa Clip target */

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
    { ACTION_STD_PREV,        BUTTON_UP,                        BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_UP|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_DOWN,                      BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_DOWN|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_STD_MENU,        BUTTON_HOME|BUTTON_REL,           BUTTON_HOME },
    { ACTION_STD_QUICKSCREEN, BUTTON_HOME|BUTTON_REPEAT,        BUTTON_HOME },

    { ACTION_STD_OK,          BUTTON_SELECT|BUTTON_REL,         BUTTON_SELECT },
    { ACTION_STD_OK,          BUTTON_RIGHT,                     BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_LEFT,                      BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_POWER,                     BUTTON_NONE },

    { ACTION_STD_CONTEXT,     BUTTON_SELECT|BUTTON_REPEAT,      BUTTON_SELECT },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */


static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,     BUTTON_UP|BUTTON_REL,        BUTTON_UP },
    { ACTION_WPS_STOP,     BUTTON_POWER|BUTTON_REL,     BUTTON_POWER },

    { ACTION_WPS_SKIPPREV, BUTTON_LEFT|BUTTON_REL,    BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK, BUTTON_LEFT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_LEFT|BUTTON_REL,    BUTTON_LEFT|BUTTON_REPEAT },

    { ACTION_WPS_SKIPNEXT, BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_WPS_SEEKFWD,  BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },

    { ACTION_WPS_ABSETB_NEXTDIR,    BUTTON_POWER|BUTTON_RIGHT,   BUTTON_POWER },
    { ACTION_WPS_ABSETA_PREVDIR,    BUTTON_POWER|BUTTON_LEFT,    BUTTON_POWER },
    { ACTION_WPS_ABRESET,           BUTTON_POWER|BUTTON_UP,      BUTTON_POWER },

    { ACTION_WPS_VOLUP,     BUTTON_VOL_UP,                   BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },

    { ACTION_WPS_BROWSE,        BUTTON_SELECT|BUTTON_REL,     BUTTON_SELECT },
    { ACTION_WPS_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,  BUTTON_SELECT },

    { ACTION_WPS_QUICKSCREEN,   BUTTON_HOME|BUTTON_REPEAT,    BUTTON_HOME },
    { ACTION_WPS_MENU,          BUTTON_HOME|BUTTON_REL,      BUTTON_HOME },

    { ACTION_WPS_PITCHSCREEN,   BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_ID3SCREEN,     BUTTON_SELECT|BUTTON_DOWN,      BUTTON_SELECT },

#ifdef HAVE_HOTKEY /* down|repeat doesn't work in the file browser */
    { ACTION_WPS_HOTKEY,        BUTTON_DOWN,                    BUTTON_NONE },
#endif
    { ACTION_WPS_VIEW_PLAYLIST, BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

#ifndef HAS_BUTTON_HOLD /* Clip+ */
    { ACTION_STD_KEYLOCK,       BUTTON_HOME|BUTTON_SELECT,      BUTTON_NONE },
#endif /* HAS_BUTTON_HOLD */

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_settings[] = {
    { ACTION_STD_OK,            BUTTON_HOME|BUTTON_REL,     BUTTON_HOME },
    { ACTION_STD_CANCEL,        BUTTON_POWER,               BUTTON_NONE },
    { ACTION_SETTINGS_RESET,    BUTTON_SELECT,              BUTTON_NONE },
    
    { ACTION_SETTINGS_INC,      BUTTON_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,      BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_STD_PREV,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings */

static const struct button_mapping button_context_list[]  = {
    { ACTION_LISTTREE_PGUP,   BUTTON_SELECT|BUTTON_UP,    BUTTON_SELECT },
    { ACTION_LISTTREE_PGDOWN, BUTTON_SELECT|BUTTON_DOWN,  BUTTON_SELECT },

#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLUP,        BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_LIST_VOLUP,        BUTTON_VOL_UP,                  BUTTON_NONE },

    { ACTION_LIST_VOLDOWN,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,      BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
#else
    { ACTION_LISTTREE_PGUP,     BUTTON_VOL_UP,                  BUTTON_NONE },
    { ACTION_LISTTREE_PGUP,     BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_HOME|BUTTON_SELECT,    BUTTON_HOME },
    { ACTION_TREE_STOP,   BUTTON_POWER|BUTTON_REL,      BUTTON_POWER },
#ifdef HAVE_HOTKEY
    { ACTION_TREE_HOTKEY, BUTTON_HOME|BUTTON_DOWN,      BUTTON_HOME },
#endif

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_without_combo */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    { ACTION_TREE_PGLEFT,   BUTTON_HOME|BUTTON_LEFT,               BUTTON_NONE },
    { ACTION_TREE_ROOT_INIT,BUTTON_HOME|BUTTON_LEFT|BUTTON_REPEAT, BUTTON_HOME|BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,   BUTTON_HOME|BUTTON_LEFT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,  BUTTON_HOME|BUTTON_RIGHT,              BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,  BUTTON_HOME|BUTTON_RIGHT|BUTTON_REPEAT,BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_with_combo */

static const struct button_mapping button_context_mainmenu[]  = {
    { ACTION_TREE_WPS, BUTTON_HOME|BUTTON_REL, BUTTON_HOME },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_TREE),
}; /* button_context_mainmenu */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,              BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_NONE,       BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_STD_CANCEL, BUTTON_POWER|BUTTON_REL,       BUTTON_NONE },
    { ACTION_STD_CANCEL, BUTTON_HOME,                   BUTTON_NONE },
    { ACTION_STD_CANCEL, BUTTON_SELECT,                 BUTTON_NONE },
    { ACTION_QS_TOP,     BUTTON_UP|BUTTON_REL,          BUTTON_NONE },
    { ACTION_QS_TOP,     BUTTON_UP|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_DOWN|BUTTON_REL,        BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_DOWN|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_LEFT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_RIGHT|BUTTON_REL,       BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_quickscreen */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,      BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,      BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_STD_CANCEL,        BUTTON_POWER,               BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_right_is_inc */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                      BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },   
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_HOME|BUTTON_REL,         BUTTON_HOME },
    { ACTION_PS_RESET,          BUTTON_SELECT,                  BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_POWER,                   BUTTON_NONE },
    { ACTION_PS_SLOWER,         BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_pitchscreen */

#ifdef HAVE_RECORDING
/** Recording Screen **/
static const struct button_mapping button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,          BUTTON_SELECT,                 BUTTON_NONE },
    { ACTION_REC_NEWFILE,        BUTTON_HOME|BUTTON_REL,        BUTTON_HOME },
    { ACTION_STD_MENU,           BUTTON_DOWN|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_INC,       BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_INC,       BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */
#endif

/** FM Radio Screen **/
static const struct button_mapping button_context_radio[]  = {
    { ACTION_NONE,              BUTTON_SELECT,                  BUTTON_NONE },
    { ACTION_NONE,              BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_NONE,              BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_FM_MODE,           BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_FM_PRESET,         BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },
    { ACTION_FM_STOP,           BUTTON_POWER|BUTTON_REL,        BUTTON_POWER },
    { ACTION_FM_MENU,           BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT},
    { ACTION_FM_EXIT,           BUTTON_HOME|BUTTON_REL,         BUTTON_HOME },
    { ACTION_FM_PLAY,           BUTTON_UP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INC,      BUTTON_VOL_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

#ifndef HAS_BUTTON_HOLD /* Clip+ */
    { ACTION_STD_KEYLOCK,       BUTTON_HOME|BUTTON_SELECT,      BUTTON_NONE },
#endif /* HAS_BUTTON_HOLD */

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                     BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                    BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                       BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                     BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,       BUTTON_NONE },

    { ACTION_KBD_CURSOR_LEFT,  BUTTON_VOL_UP,                   BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },

    /*{ ACTION_KBD_PAGE_FLIP,    ,       BUTTON_NONE },*/
    { ACTION_KBD_BACKSPACE,    BUTTON_HOME,                     BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_HOME|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_SELECT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_SELECT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_POWER,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,       BUTTON_HOME,        BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

#ifdef USB_ENABLE_HID
static const struct button_mapping button_context_usb_hid[] = {
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_POWER|BUTTON_REL,    BUTTON_POWER },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_POWER|BUTTON_REPEAT, BUTTON_POWER },

    LAST_ITEM_IN_LIST
}; /* button_context_usb_hid */

static const struct button_mapping button_context_usb_hid_mode_multimedia[] = {
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,         BUTTON_SELECT|BUTTON_REL,      BUTTON_SELECT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE, BUTTON_UP|BUTTON_REL,          BUTTON_UP },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,       BUTTON_HOME|BUTTON_REL,        BUTTON_HOME },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV, BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT, BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_multimedia */

static const struct button_mapping button_context_usb_hid_mode_presentation[] = {
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_START, BUTTON_UP|BUTTON_REL,          BUTTON_UP },
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE, BUTTON_HOME|BUTTON_REL,        BUTTON_HOME },
    { ACTION_USB_HID_PRESENTATION_SLIDE_PREV,      BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,      BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,     BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_LEFT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_LAST,      BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_RIGHT },
    { ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,    BUTTON_DOWN|BUTTON_REL,        BUTTON_DOWN },
    { ACTION_USB_HID_PRESENTATION_SCREEN_WHITE,    BUTTON_DOWN|BUTTON_REPEAT,     BUTTON_DOWN },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,       BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,       BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,       BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,       BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_MOUSE_CLICK,     BUTTON_SELECT|BUTTON_REL,      BUTTON_SELECT },
    { ACTION_USB_HID_PRESENTATION_MOUSE_OVER,      BUTTON_SELECT|BUTTON_REPEAT,   BUTTON_SELECT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_presentation */

static const struct button_mapping button_context_usb_hid_mode_browser[] = {
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_VOL_UP,                       BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_VOL_UP|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_VOL_DOWN,                     BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_VOL_DOWN|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_PAGE_UP,   BUTTON_UP|BUTTON_REL,                BUTTON_UP },
    { ACTION_USB_HID_BROWSER_SCROLL_PAGE_DOWN, BUTTON_DOWN|BUTTON_REL,              BUTTON_DOWN },
    { ACTION_USB_HID_BROWSER_ZOOM_IN,          BUTTON_UP|BUTTON_REPEAT,             BUTTON_UP },
    { ACTION_USB_HID_BROWSER_ZOOM_OUT,         BUTTON_DOWN|BUTTON_REPEAT,           BUTTON_DOWN },
    { ACTION_USB_HID_BROWSER_ZOOM_RESET,       BUTTON_SELECT|BUTTON_REPEAT,         BUTTON_SELECT },
    { ACTION_USB_HID_BROWSER_ZOOM_RESET,       BUTTON_UP|BUTTON_DOWN|BUTTON_REPEAT, BUTTON_UP|BUTTON_DOWN },
    { ACTION_USB_HID_BROWSER_TAB_PREV,         BUTTON_LEFT|BUTTON_REL,              BUTTON_LEFT },
    { ACTION_USB_HID_BROWSER_TAB_NEXT,         BUTTON_RIGHT|BUTTON_REL,             BUTTON_RIGHT },
    { ACTION_USB_HID_BROWSER_TAB_CLOSE,        BUTTON_HOME|BUTTON_REPEAT,           BUTTON_HOME },
    { ACTION_USB_HID_BROWSER_HISTORY_BACK,     BUTTON_LEFT|BUTTON_REPEAT,           BUTTON_LEFT },
    { ACTION_USB_HID_BROWSER_HISTORY_FORWARD,  BUTTON_RIGHT|BUTTON_REPEAT,          BUTTON_RIGHT },
    { ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN, BUTTON_SELECT|BUTTON_REL,            BUTTON_SELECT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_browser */

#ifdef HAVE_USB_HID_MOUSE
static const struct button_mapping button_context_usb_hid_mode_mouse[] = {
    { ACTION_USB_HID_MOUSE_UP,                BUTTON_UP,                                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_UP_REP,            BUTTON_UP|BUTTON_REPEAT,                  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN,              BUTTON_DOWN,                              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN_REP,          BUTTON_DOWN|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT,              BUTTON_LEFT,                              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT_REP,          BUTTON_LEFT|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT,             BUTTON_RIGHT,                             BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT_REP,         BUTTON_RIGHT|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT,       BUTTON_SELECT,                            BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT_REL,   BUTTON_SELECT|BUTTON_REL,                 BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_UP,          BUTTON_SELECT|BUTTON_UP,                  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_UP_REP,      BUTTON_SELECT|BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_DOWN,        BUTTON_SELECT|BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_DOWN_REP,    BUTTON_SELECT|BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_LEFT,        BUTTON_SELECT|BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_LEFT_REP,    BUTTON_SELECT|BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_RIGHT,       BUTTON_SELECT|BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LDRAG_RIGHT_REP,   BUTTON_SELECT|BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_RIGHT,      BUTTON_HOME,                              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_RIGHT_REL,  BUTTON_HOME|BUTTON_REL,                   BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_UP,          BUTTON_HOME|BUTTON_UP,                    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_UP_REP,      BUTTON_HOME|BUTTON_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_DOWN,        BUTTON_HOME|BUTTON_DOWN,                  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_DOWN_REP,    BUTTON_HOME|BUTTON_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_LEFT,        BUTTON_HOME|BUTTON_LEFT,                  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_LEFT_REP,    BUTTON_HOME|BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_RIGHT,       BUTTON_HOME|BUTTON_RIGHT,                 BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RDRAG_RIGHT_REP,   BUTTON_HOME|BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_VOL_UP,                            BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_VOL_UP|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_VOL_DOWN,                          BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_VOL_DOWN|BUTTON_REPEAT,            BUTTON_NONE },

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

        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_listtree_scroll_without_combo;
            else
                return button_context_listtree_scroll_with_combo;
        case CONTEXT_MAINMENU:
            return button_context_mainmenu;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_TIME:
            return button_context_settings;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings_right_is_inc;

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
#ifdef HAVE_RECORDING
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
#endif
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
#ifdef HAVE_USB_HID_MOUSE
        case CONTEXT_USB_HID_MODE_MOUSE:
            return button_context_usb_hid_mode_mouse;
#endif
#endif
        default:
            return button_context_standard;
    }
    return button_context_standard;
}
