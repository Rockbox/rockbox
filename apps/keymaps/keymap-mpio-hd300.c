/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */

/*****************************************************************************
 *    Main control mappings 
 *****************************************************************************/

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,           BUTTON_UP,                    BUTTON_NONE  },
    { ACTION_STD_PREVREPEAT,     BUTTON_UP|BUTTON_REPEAT,      BUTTON_NONE  },
    { ACTION_STD_NEXT,           BUTTON_DOWN,                     BUTTON_NONE  },
    { ACTION_STD_NEXTREPEAT,     BUTTON_DOWN|BUTTON_REPEAT,       BUTTON_NONE  },
    { ACTION_STD_OK,             BUTTON_ENTER|BUTTON_REL,       BUTTON_ENTER },
    { ACTION_STD_CANCEL,         BUTTON_MENU|BUTTON_REL,        BUTTON_MENU  },
    { ACTION_STD_CONTEXT,        BUTTON_ENTER|BUTTON_REPEAT,    BUTTON_ENTER },
    { ACTION_STD_MENU,           BUTTON_MENU|BUTTON_REPEAT,     BUTTON_MENU  },
/*  { ACTION_STD_QUICKSCREEN,    BUTTON_,                       BUTTON_ }, */
/*  { ACTION_STD_KEYLOCK,        BUTTON_,                       BUTTON_ }, */
/*  { ACTION_STD_REC,            BUTTON_,                       BUTTON_ }, */
/*  { ACTION_STD_HOTKEY,         BUTTON_,                       BUTTON_ }, */
/*  { ACTION_F3,                 BUTTON_,                       BUTTON_ }, */
  
    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_tree[]  = {
/*  { ACTION_TREE_ROOT_INIT,     BUTTON_,                       BUTTON_ }, */
/*  { ACTION_TREE_PGLEFT,        BUTTON_,                       BUTTON_ }, */
/*  { ACTION_TREE_PGRIGHT,       BUTTON_,                       BUTTON_ }, */
    { ACTION_TREE_STOP,          BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_TREE_WPS,           BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
/*  { ACTION_TREE_HOTKEY,        BUTTON_,                       BUTTON_ }, */

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_tree */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_BROWSE,         BUTTON_ENTER|BUTTON_REL,       BUTTON_ENTER },
    { ACTION_WPS_PLAY,           BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_WPS_SEEKBACK,       BUTTON_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,        BUTTON_FF|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,       BUTTON_REW|BUTTON_REL,         BUTTON_REW|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,       BUTTON_FF|BUTTON_REL,          BUTTON_FF|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,       BUTTON_FF|BUTTON_REL,          BUTTON_FF },
    { ACTION_WPS_SKIPPREV,       BUTTON_REW|BUTTON_REL,         BUTTON_REW },
    { ACTION_WPS_STOP,           BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_WPS_VOLDOWN,        BUTTON_DOWN,               BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,        BUTTON_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLUP,          BUTTON_UP,                 BUTTON_NONE }, 
    { ACTION_WPS_VOLUP,          BUTTON_UP|BUTTON_REPEAT,   BUTTON_NONE },
/*  { ACTION_WPS_PITCHSCREEN,    BUTTON_,                       BUTTON_ }, */
/*  { ACTION_WPS_ID3SCREEN,      BUTTON_,                       BUTTON_ }, */ 
    { ACTION_WPS_CONTEXT,        BUTTON_ENTER|BUTTON_REPEAT,    BUTTON_ENTER },
    { ACTION_WPS_QUICKSCREEN,    BUTTON_REC|BUTTON_REPEAT,      BUTTON_REC },
    { ACTION_WPS_MENU,           BUTTON_MENU|BUTTON_REL,        BUTTON_MENU },
/*  { ACTION_WPS_VIEW_PLAYLIST,  BUTTON_,                       BUTTON_ }, */
/*  { ACTION_WPS_REC,            BUTTON_,                       BUTTON_ }, */
/*  { ACTION_WPS_ABSETA_PREVDIR, BUTTON_,                       BUTTON_ }, */
/*  { ACTION_WPS_ABSETB_NEXTDIR, BUTTON_,                       BUTTON_ }, */
/*  { ACTION_WPS_ABRESET,        BUTTON_,                       BUTTON_ }, */
/*  { ACTION_WPS_HOTKEY,         BUTTON_,                       BUTTON_ }, */

    LAST_ITEM_IN_LIST,
}; /* button_context_wps */

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,       BUTTON_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_UP|BUTTON_REPEAT,   BUTTON_NONE },
/*    { ACTION_SETTINGS_INCBIGSTEP,BUTTON_,                       BUTTON_ }, */
    { ACTION_SETTINGS_DEC,       BUTTON_DOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_DOWN|BUTTON_REPEAT, BUTTON_NONE },
/*  { ACTION_SETTINGS_DECBIGSTEP,BUTTON_,                       BUTTON_ }, */
    { ACTION_STD_PREV,           BUTTON_REW,                    BUTTON_NONE  },
    { ACTION_STD_PREVREPEAT,     BUTTON_REW|BUTTON_REPEAT,      BUTTON_NONE  },
    { ACTION_STD_NEXT,           BUTTON_FF,                     BUTTON_NONE  },
    { ACTION_STD_NEXTREPEAT,     BUTTON_FF|BUTTON_REPEAT,       BUTTON_NONE  },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,       BUTTON_ENTER,                  BUTTON_NONE },
    { ACTION_YESNO_ACCEPT,       BUTTON_PLAY,                   BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_yesno */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,         BUTTON_REC|BUTTON_REPEAT,      BUTTON_REC },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {
    { ACTION_STD_CANCEL,       BUTTON_MENU,  BUTTON_NONE },
    { ACTION_STD_OK,           BUTTON_ENTER,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_time */

/*
static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_LEFT,            BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_QS_LEFT,            BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_RIGHT,           BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_QS_RIGHT,           BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_DOWN,            BUTTON_FF,                     BUTTON_NONE },
    { ACTION_QS_DOWN,            BUTTON_FF|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_QS_TOP,             BUTTON_REW,                    BUTTON_NONE },
    { ACTION_QS_TOP,             BUTTON_REW|BUTTON_REPEAT,      BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};*/  /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
//    { ACTION_PS_INC_SMALL,       BUTTON_VOL_UP,                 BUTTON_NONE },
//    { ACTION_PS_INC_BIG,         BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
//    { ACTION_PS_DEC_SMALL,       BUTTON_VOL_DOWN,               BUTTON_NONE },
//    { ACTION_PS_DEC_BIG,         BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,      BUTTON_REW,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,     BUTTON_FF,                     BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,   BUTTON_REW|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF,  BUTTON_FF|BUTTON_REL,          BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,     BUTTON_PLAY,                   BUTTON_NONE },
    { ACTION_PS_RESET,           BUTTON_ENTER,                   BUTTON_NONE },
    { ACTION_PS_EXIT,            BUTTON_MENU,                    BUTTON_NONE },
    { ACTION_PS_SLOWER,          BUTTON_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_PS_FASTER,          BUTTON_FF|BUTTON_REPEAT,       BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchscreen */

static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,            BUTTON_ENTER|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_FM_PRESET,          BUTTON_ENTER|BUTTON_REL,        BUTTON_ENTER },
/*  { ACTION_FM_RECORD,          BUTTON_,      BUTTON_ }, */
/*  { ACTION_FM_FREEZE,          BUTTON_,                       BUTTON_ }, */
    { ACTION_FM_STOP,            BUTTON_MENU|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_FM_MODE,            BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_FM_EXIT,            BUTTON_MENU|BUTTON_REL,         BUTTON_MENU },
    { ACTION_FM_PLAY,            BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
/*  { ACTION_FM_RECORD_DBLPRE,   BUTTON_,                       BUTTON_ }, */
    { ACTION_FM_NEXT_PRESET,     BUTTON_FF,                       BUTTON_NONE },
    { ACTION_FM_PREV_PRESET,     BUTTON_REW,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */

static const struct button_mapping button_context_recscreen[]  = {
/*  { ACTION_REC_LCD,            BUTTON_,                       BUTTON_ }, */
    { ACTION_REC_PAUSE,          BUTTON_PLAY,                   BUTTON_NONE },
    { ACTION_REC_NEWFILE,        BUTTON_ENTER|BUTTON_REPEAT,    BUTTON_ENTER },
/*  { ACTION_REC_F2,             BUTTON_,                       BUTTON_ }, */
/*  { ACTION_REC_F3,             BUTTON_,                       BUTTON_ }, */

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_recscreen */

#if 0
static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,           BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_KBD_LEFT,           BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_RIGHT,          BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_KBD_RIGHT,          BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
/*  { ACTION_KBD_CURSOR_LEFT,    BUTTON_,                       BUTTON_ }, */
/*  { ACTION_KBD_CURSOR_RIGHT,   BUTTON_,                       BUTTON_ }, */
    { ACTION_KBD_SELECT,         BUTTON_FUNC,                   BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,      BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_KBD_DONE,           BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_KBD_ABORT,          BUTTON_REC|BUTTON_REL,         BUTTON_REC },
/*  { ACTION_KBD_BACKSPACE,      BUTTON_,                       BUTTON_ }, */
    { ACTION_KBD_UP,             BUTTON_REW,                    BUTTON_NONE },
    { ACTION_KBD_UP,             BUTTON_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_DOWN,           BUTTON_FF,                     BUTTON_NONE },
    { ACTION_KBD_DOWN,           BUTTON_FF|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,    BUTTON_REC|BUTTON_REPEAT,      BUTTON_REC },
    { ACTION_KBD_MORSE_SELECT,   BUTTON_FUNC|BUTTON_REL,        BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */
#endif

const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;

        case CONTEXT_WPS:
            return button_context_wps;

#if CONFIG_TUNER
         case CONTEXT_FM:
             return button_context_radio;
#endif

#ifdef HAVE_RECORDING
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
#endif

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;

        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;

//        case CONTEXT_QUICKSCREEN:
//            return button_context_quickscreen;

        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;

//        case CONTEXT_KEYBOARD:
//        case CONTEXT_MORSE_INPUT:
//            return button_context_keyboard;

        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_EQ:
            return button_context_settings;

        case CONTEXT_SETTINGS_TIME:
            return button_context_time;

        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_LIST:
        default:
            return button_context_standard;
    }
    return button_context_standard;
}

