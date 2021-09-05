/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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

/* Button Code Definitions for Sandisk Sansa Connect target */

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
  {ACTION_STD_PREV,       BUTTON_SCROLL_BACK,               BUTTON_NONE},
  {ACTION_STD_PREVREPEAT, BUTTON_SCROLL_BACK|BUTTON_REPEAT, BUTTON_NONE},
  {ACTION_STD_NEXT,       BUTTON_SCROLL_FWD,                BUTTON_NONE},
  {ACTION_STD_NEXTREPEAT, BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE},
  {ACTION_STD_OK,         BUTTON_SELECT|BUTTON_REL,         BUTTON_SELECT},
  {ACTION_STD_OK,         BUTTON_RIGHT,                     BUTTON_NONE},
  {ACTION_STD_CANCEL,     BUTTON_LEFT,                      BUTTON_NONE},
  {ACTION_STD_CONTEXT,    BUTTON_SELECT|BUTTON_REPEAT,      BUTTON_NONE},
  {ACTION_STD_MENU,       BUTTON_DOWN|BUTTON_REL,           BUTTON_DOWN},
  {ACTION_STD_QUICKSCREEN,BUTTON_DOWN|BUTTON_REL,           BUTTON_DOWN},
  {ACTION_STD_HOTKEY,     BUTTON_UP|BUTTON_REL,             BUTTON_UP},
  LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
  {ACTION_WPS_PLAY,       BUTTON_DOWN|BUTTON_REL,           BUTTON_DOWN},
  {ACTION_WPS_SEEKBACK,   BUTTON_LEFT|BUTTON_REPEAT,        BUTTON_NONE},
  {ACTION_WPS_SEEKFWD,    BUTTON_RIGHT|BUTTON_REPEAT,       BUTTON_NONE},
  {ACTION_WPS_STOPSEEK,   BUTTON_LEFT|BUTTON_REL,           BUTTON_LEFT|BUTTON_REPEAT},
  {ACTION_WPS_SKIPNEXT,   BUTTON_NEXT,                      BUTTON_NONE},
  {ACTION_WPS_SKIPPREV,   BUTTON_PREV,                      BUTTON_NONE},
  {ACTION_WPS_STOP,       BUTTON_POWER,          BUTTON_NONE},
  {ACTION_WPS_VOLDOWN,    BUTTON_VOL_DOWN,                  BUTTON_NONE},
  {ACTION_WPS_VOLDOWN,    BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE},
  {ACTION_WPS_VOLUP,      BUTTON_VOL_UP,                    BUTTON_NONE},
  {ACTION_WPS_VOLUP,      BUTTON_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE},
  {ACTION_WPS_BROWSE,     BUTTON_SELECT|BUTTON_REL,         BUTTON_SELECT},
  {ACTION_WPS_CONTEXT,    BUTTON_SELECT|BUTTON_REPEAT,      BUTTON_SELECT},
  {ACTION_WPS_MENU,       BUTTON_DOWN|BUTTON_REL,           BUTTON_DOWN},
  {ACTION_WPS_ABSETA_PREVDIR, BUTTON_POWER|BUTTON_RIGHT,    BUTTON_POWER},
  {ACTION_WPS_ABSETB_NEXTDIR, BUTTON_POWER|BUTTON_LEFT,     BUTTON_POWER},
  {ACTION_WPS_ABRESET,    BUTTON_POWER|BUTTON_UP,           BUTTON_POWER},
  {ACTION_WPS_HOTKEY,     BUTTON_UP|BUTTON_REL,             BUTTON_UP},
  LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_yesno[] = {
  {ACTION_YESNO_ACCEPT,   BUTTON_SELECT,                    BUTTON_NONE},
  {ACTION_STD_CANCEL,     BUTTON_PREV,                      BUTTON_NONE},
  {ACTION_STD_CANCEL,     BUTTON_NEXT,                      BUTTON_NONE},
  {ACTION_STD_CANCEL,     BUTTON_VOL_UP,                      BUTTON_NONE},
  {ACTION_STD_CANCEL,     BUTTON_VOL_DOWN,                      BUTTON_NONE},
  {ACTION_STD_CANCEL,     BUTTON_POWER,                      BUTTON_NONE},
  LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_yesno */

static const struct button_mapping button_context_settings_time[] = {
  {ACTION_STD_PREV,       BUTTON_PREV|BUTTON_REL,     BUTTON_PREV},
  {ACTION_STD_PREVREPEAT, BUTTON_PREV|BUTTON_REPEAT,  BUTTON_PREV},
  {ACTION_STD_NEXT,       BUTTON_NEXT|BUTTON_REL,     BUTTON_NEXT},
  {ACTION_STD_NEXTREPEAT, BUTTON_NEXT|BUTTON_REPEAT,  BUTTON_NEXT},
  {ACTION_STD_CANCEL,     BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT},
  {ACTION_STD_OK,         BUTTON_SELECT|BUTTON_REL,   BUTTON_SELECT},
  {ACTION_SETTINGS_INC,   BUTTON_SCROLL_FWD,          BUTTON_NONE},
  {ACTION_SETTINGS_DEC,   BUTTON_SCROLL_BACK,         BUTTON_NONE},
  LAST_ITEM_IN_LIST
}; /* button_context_settings_time */

static const struct button_mapping button_context_keyboard[] = {
  {ACTION_KBD_LEFT,       BUTTON_LEFT,                      BUTTON_NONE},
  {ACTION_KBD_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,        BUTTON_NONE},
  {ACTION_KBD_RIGHT,      BUTTON_RIGHT,                     BUTTON_NONE},
  {ACTION_KBD_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,       BUTTON_NONE},

  {ACTION_KBD_CURSOR_LEFT,BUTTON_VOL_DOWN,                  BUTTON_NONE},
  {ACTION_KBD_CURSOR_LEFT,BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE},
  {ACTION_KBD_CURSOR_RIGHT,BUTTON_VOL_UP,                   BUTTON_NONE},
  {ACTION_KBD_CURSOR_RIGHT,BUTTON_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE},

  {ACTION_KBD_UP,         BUTTON_SCROLL_BACK,               BUTTON_NONE},
  {ACTION_KBD_UP,         BUTTON_SCROLL_BACK|BUTTON_REPEAT, BUTTON_NONE},
  {ACTION_KBD_DOWN,       BUTTON_SCROLL_FWD,                BUTTON_NONE},
  {ACTION_KBD_DOWN,       BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE},
  {ACTION_KBD_PAGE_FLIP,  BUTTON_NEXT,                      BUTTON_NONE},
  {ACTION_KBD_BACKSPACE,  BUTTON_PREV,                      BUTTON_NONE},
  {ACTION_KBD_BACKSPACE,  BUTTON_PREV|BUTTON_REPEAT,        BUTTON_NONE},
  {ACTION_KBD_SELECT,     BUTTON_SELECT,                    BUTTON_NONE},
  {ACTION_KBD_DONE,       BUTTON_UP,                        BUTTON_NONE},
  {ACTION_KBD_ABORT,      BUTTON_POWER,                     BUTTON_NONE},
  {ACTION_KBD_MORSE_INPUT,BUTTON_DOWN|BUTTON_REL,           BUTTON_NONE},
  {ACTION_KBD_MORSE_SELECT,BUTTON_SELECT|BUTTON_REL,        BUTTON_NONE},
  LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_KEYBOARD:
        case CONTEXT_MORSE_INPUT:
            return button_context_keyboard;
        case CONTEXT_SETTINGS_TIME:
            return button_context_settings_time;

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
