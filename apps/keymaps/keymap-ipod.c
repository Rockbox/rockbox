/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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

/* Button Code Definitions for ipod target */

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


*/

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,          BUTTON_SCROLL_BACK,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_SCROLL_BACK|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_SCROLL_FWD,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_SCROLL_FWD|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT,                        BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT,                       BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    { ACTION_STD_MENU,          BUTTON_MENU|BUTTON_REL,             BUTTON_MENU },
    { ACTION_STD_QUICKSCREEN,   BUTTON_MENU|BUTTON_REPEAT,          BUTTON_MENU },
    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT },
    { ACTION_STD_CANCEL,        BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,          BUTTON_PLAY|BUTTON_REL,      BUTTON_PLAY },
    { ACTION_TREE_STOP,         BUTTON_PLAY|BUTTON_REPEAT,   BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_tree */

static const struct button_mapping button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_tree_scroll_lr */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,      BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },
    { ACTION_WPS_STOP,      BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY },
    { ACTION_WPS_SKIPPREV,  BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,  BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,  BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,  BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SEEKFWD,   BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,  BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_VOLDOWN,   BUTTON_SCROLL_BACK,                 BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_SCROLL_BACK|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_SCROLL_FWD,                  BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_SCROLL_FWD|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_BROWSE,    BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    { ACTION_WPS_CONTEXT,   BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT },
    { ACTION_WPS_VIEW_PLAYLIST, BUTTON_SELECT|BUTTON_PLAY,      BUTTON_NONE },
    { ACTION_WPS_MENU,          BUTTON_MENU|BUTTON_REL,         BUTTON_MENU },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_MENU|BUTTON_REPEAT,      BUTTON_MENU },

    LAST_ITEM_IN_LIST,
}; /* button_context_wps */

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_SCROLL_FWD,         BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_SCROLL_BACK,          BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_SCROLL_BACK|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_LEFT,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_OK,                BUTTON_SELECT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_STD_CANCEL,            BUTTON_MENU|BUTTON_REL,      BUTTON_MENU },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,                  BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_yesno */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,          BUTTON_MENU|BUTTON_REPEAT,       BUTTON_MENU },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_TOP,        BUTTON_MENU,                    BUTTON_NONE },
    { ACTION_QS_TOP,        BUTTON_MENU|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_PLAY,                    BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_SELECT,                  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_SCROLL_FWD,                BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_SCROLL_BACK,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_SCROLL_BACK|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_PLAY,                BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_MENU,                BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_SELECT,              BUTTON_NONE },
    { ACTION_PS_SLOWER,         BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchscreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                           BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,             BUTTON_NONE },   
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                          BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_SELECT,                         BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_PLAY,                           BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_MENU|BUTTON_REL,                BUTTON_MENU },
    { ACTION_KBD_UP,           BUTTON_SCROLL_BACK,                    BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_SCROLL_BACK|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_SCROLL_FWD,                     BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_SCROLL_FWD|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_MENU|BUTTON_REPEAT,             BUTTON_MENU },
    { ACTION_KBD_MORSE_SELECT, BUTTON_SELECT|BUTTON_REL,              BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

#ifdef HAVE_RECORDING
const struct button_mapping button_context_recscreen[]  = {

    { ACTION_REC_NEWFILE,        BUTTON_SELECT|BUTTON_REL,        BUTTON_SELECT },
    { ACTION_STD_MENU,           BUTTON_SELECT|BUTTON_REPEAT,     BUTTON_SELECT },
    { ACTION_REC_PAUSE,          BUTTON_PLAY|BUTTON_REL,            BUTTON_PLAY },
    { ACTION_STD_CANCEL,         BUTTON_PLAY|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_MENU,                       BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_SCROLL_FWD,                 BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_SCROLL_FWD|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_SCROLL_BACK,                BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_SCROLL_BACK|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_INC,       BUTTON_RIGHT,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RIGHT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_LEFT,                       BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_LEFT|BUTTON_REPEAT,         BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_recscreen */
#endif

/** FM Radio Screen **/
 #if CONFIG_TUNER
 static const struct button_mapping button_context_radio[]  = {
     { ACTION_FM_MENU,           BUTTON_SELECT | BUTTON_REPEAT,   BUTTON_NONE },
     { ACTION_FM_STOP,           BUTTON_PLAY | BUTTON_REPEAT,     BUTTON_PLAY },
     { ACTION_FM_MODE,           BUTTON_SELECT,                   BUTTON_NONE },
     { ACTION_FM_EXIT,           BUTTON_MENU | BUTTON_REL,        BUTTON_NONE },
     { ACTION_FM_PLAY,           BUTTON_PLAY | BUTTON_REL,        BUTTON_PLAY },   
     { ACTION_SETTINGS_INC,      BUTTON_SCROLL_FWD,               BUTTON_NONE },
     { ACTION_SETTINGS_INCREPEAT,BUTTON_SCROLL_FWD|BUTTON_REPEAT, BUTTON_NONE },
     { ACTION_SETTINGS_DEC,      BUTTON_SCROLL_BACK,              BUTTON_NONE },
     { ACTION_SETTINGS_DECREPEAT,BUTTON_SCROLL_BACK|BUTTON_REPEAT,BUTTON_NONE },
     { ACTION_STD_NEXT,          BUTTON_RIGHT,                    BUTTON_NONE },
     { ACTION_STD_NEXTREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },
     { ACTION_STD_PREV,          BUTTON_LEFT,                     BUTTON_NONE },
     { ACTION_STD_PREVREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,       BUTTON_NONE },
     LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
 }; /* button_context_radio */
 #endif

#ifdef USB_ENABLE_HID
static const struct button_mapping button_context_usb_hid[] = {
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_SELECT|BUTTON_RIGHT|BUTTON_REL,    BUTTON_SELECT|BUTTON_RIGHT },
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_SELECT|BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_SELECT|BUTTON_RIGHT },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_SELECT|BUTTON_LEFT|BUTTON_REL,     BUTTON_SELECT|BUTTON_LEFT },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_SELECT|BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_SELECT|BUTTON_LEFT },

    LAST_ITEM_IN_LIST
}; /* button_context_usb_hid */

static const struct button_mapping button_context_usb_hid_mode_multimedia[] = {
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_SCROLL_BACK,               BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_SCROLL_BACK|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_SCROLL_FWD,                BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,         BUTTON_SELECT|BUTTON_REL,         BUTTON_SELECT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE, BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,       BUTTON_MENU|BUTTON_REL,           BUTTON_MENU },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,       BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV, BUTTON_LEFT|BUTTON_REL,           BUTTON_LEFT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT, BUTTON_RIGHT|BUTTON_REL,          BUTTON_RIGHT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_multimedia */

static const struct button_mapping button_context_usb_hid_mode_presentation[] = {
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_START,   BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE,   BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },
    { ACTION_USB_HID_PRESENTATION_SLIDE_PREV,        BUTTON_LEFT|BUTTON_REL,           BUTTON_LEFT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,        BUTTON_RIGHT|BUTTON_REL,          BUTTON_RIGHT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,       BUTTON_LEFT|BUTTON_REPEAT,        BUTTON_LEFT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_LAST,        BUTTON_RIGHT|BUTTON_REPEAT,       BUTTON_RIGHT },
    { ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,      BUTTON_MENU|BUTTON_REL,           BUTTON_MENU },
    { ACTION_USB_HID_PRESENTATION_SCREEN_WHITE,      BUTTON_MENU|BUTTON_REPEAT,        BUTTON_MENU },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,         BUTTON_SCROLL_BACK,               BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,         BUTTON_SCROLL_BACK|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,         BUTTON_SCROLL_FWD,                BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,         BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_USB_HID_PRESENTATION_MOUSE_CLICK,       BUTTON_SELECT|BUTTON_REL,         BUTTON_SELECT },
    { ACTION_USB_HID_PRESENTATION_MOUSE_OVER,        BUTTON_SELECT|BUTTON_REPEAT,      BUTTON_SELECT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_presentation */

static const struct button_mapping button_context_usb_hid_mode_browser[] = {
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_SCROLL_BACK,                      BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_SCROLL_BACK|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_SCROLL_FWD,                       BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_SCROLL_FWD|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_PAGE_UP,   BUTTON_PLAY|BUTTON_REL,                  BUTTON_PLAY },
    { ACTION_USB_HID_BROWSER_SCROLL_PAGE_DOWN, BUTTON_MENU|BUTTON_REL,                  BUTTON_MENU },
    { ACTION_USB_HID_BROWSER_ZOOM_IN,          BUTTON_PLAY|BUTTON_REPEAT,               BUTTON_PLAY },
    { ACTION_USB_HID_BROWSER_ZOOM_OUT,         BUTTON_MENU|BUTTON_REPEAT,               BUTTON_MENU },
    { ACTION_USB_HID_BROWSER_ZOOM_RESET,       BUTTON_PLAY|BUTTON_MENU|BUTTON_REPEAT,   BUTTON_PLAY|BUTTON_MENU },
    { ACTION_USB_HID_BROWSER_TAB_PREV,         BUTTON_LEFT|BUTTON_REL,                  BUTTON_LEFT },
    { ACTION_USB_HID_BROWSER_TAB_NEXT,         BUTTON_RIGHT|BUTTON_REL,                 BUTTON_RIGHT },
    { ACTION_USB_HID_BROWSER_TAB_CLOSE,        BUTTON_SELECT|BUTTON_MENU|BUTTON_REPEAT, BUTTON_SELECT|BUTTON_MENU },
    { ACTION_USB_HID_BROWSER_HISTORY_BACK,     BUTTON_LEFT|BUTTON_REPEAT,               BUTTON_LEFT },
    { ACTION_USB_HID_BROWSER_HISTORY_FORWARD,  BUTTON_RIGHT|BUTTON_REPEAT,              BUTTON_RIGHT },
    { ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN, BUTTON_SELECT|BUTTON_PLAY|BUTTON_REPEAT, BUTTON_SELECT|BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_browser */

#ifdef HAVE_USB_HID_MOUSE
static const struct button_mapping button_context_usb_hid_mode_mouse[] = {
    { ACTION_USB_HID_MOUSE_UP,                BUTTON_MENU,                              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_UP_REP,            BUTTON_MENU|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN,              BUTTON_PLAY,                              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN_REP,          BUTTON_PLAY|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT,              BUTTON_LEFT,                              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT_REP,          BUTTON_LEFT|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT,             BUTTON_RIGHT,                             BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT_REP,         BUTTON_RIGHT|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT,       BUTTON_SELECT,                            BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT_REL,   BUTTON_SELECT|BUTTON_REL,                 BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_SCROLL_BACK,                       BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_SCROLL_BACK|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_SCROLL_FWD,                        BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_SCROLL_FWD|BUTTON_REPEAT,          BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_mouse */
#endif
#endif

#if BUTTON_REMOTE != 0
/*****************************************************************************
 *    Remote control mappings
 *****************************************************************************/

static const struct button_mapping remote_button_context_standard[]  = {
    { ACTION_STD_PREV,      BUTTON_RC_LEFT,     BUTTON_NONE },
    { ACTION_STD_NEXT,      BUTTON_RC_RIGHT,    BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_RC_STOP,     BUTTON_NONE },
    { ACTION_STD_OK,        BUTTON_RC_PLAY,     BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* remote_button_context_standard */

static const struct button_mapping remote_button_context_wps[]  = {
    { ACTION_WPS_VOLDOWN,   BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },

    { ACTION_WPS_PLAY,      BUTTON_RC_PLAY|BUTTON_REL,    BUTTON_RC_PLAY },
    { ACTION_WPS_STOP,      BUTTON_RC_PLAY|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_SKIPNEXT,  BUTTON_RC_RIGHT|BUTTON_REL,   BUTTON_RC_RIGHT },
    { ACTION_WPS_SEEKFWD,   BUTTON_RC_RIGHT|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,  BUTTON_RC_RIGHT|BUTTON_REL,   BUTTON_RC_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_SKIPPREV,  BUTTON_RC_LEFT|BUTTON_REL,    BUTTON_RC_LEFT },
    { ACTION_WPS_SEEKBACK,  BUTTON_RC_LEFT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,  BUTTON_RC_LEFT|BUTTON_REL,    BUTTON_RC_LEFT|BUTTON_REPEAT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_wps */

static const struct button_mapping remote_button_context_tree[]  = {
    { ACTION_TREE_WPS,          BUTTON_RC_PLAY|BUTTON_REL,    BUTTON_RC_PLAY },
    { ACTION_TREE_STOP,         BUTTON_RC_PLAY|BUTTON_REPEAT, BUTTON_RC_PLAY },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_tree */

#if CONFIG_TUNER
 static const struct button_mapping remote_button_context_radio[]  = {
     { ACTION_FM_STOP,           BUTTON_RC_PLAY | BUTTON_REPEAT,     BUTTON_NONE },
     { ACTION_FM_PLAY,           BUTTON_RC_PLAY | BUTTON_REL,        BUTTON_RC_PLAY },   
     { ACTION_STD_NEXT,          BUTTON_RC_RIGHT|BUTTON_REL,         BUTTON_RC_RIGHT },
     { ACTION_STD_NEXTREPEAT,    BUTTON_RC_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },
     { ACTION_STD_PREV,          BUTTON_RC_LEFT|BUTTON_REL,          BUTTON_RC_LEFT },
     { ACTION_STD_PREVREPEAT,    BUTTON_RC_LEFT|BUTTON_REPEAT,       BUTTON_NONE },
     
     LAST_ITEM_IN_LIST
 }; /* remote_button_context_radio */
#endif

static const struct button_mapping* get_context_mapping_remote( int context )
{
    context ^= CONTEXT_REMOTE;

    switch (context)
    {
        case CONTEXT_WPS:
            return remote_button_context_wps;
        case CONTEXT_TREE:
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return remote_button_context_tree;

#ifdef CONFIG_TUNER
        case CONTEXT_FM:
            return remote_button_context_radio;
#endif
        default:
            return remote_button_context_standard;
    }
}
#endif /* BUTTON_REMOTE != 0 */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
#if BUTTON_REMOTE != 0
    if (context&CONTEXT_REMOTE)
        return get_context_mapping_remote(context);
#endif

    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_tree_scroll_lr;
            /* else fall through to CUSTOM|CONTEXT_TREE */
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_LIST:
            return button_context_standard;

        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_SETTINGS_TIME:
        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings;
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
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
#endif
#if CONFIG_TUNER
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
        default:
            return button_context_standard;
    }
    return button_context_standard;
}
