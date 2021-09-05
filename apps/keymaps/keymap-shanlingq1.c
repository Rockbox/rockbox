/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

/* Button Code Definitions for Shanling Q1 target */

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* {Action Code,    Button code,    Prereq button code } */

static const struct button_mapping button_context_standard[] = {
    {ACTION_STD_PREV,           BUTTON_PREV,                        BUTTON_NONE},
    {ACTION_STD_PREVREPEAT,     BUTTON_PREV|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_STD_NEXT,           BUTTON_NEXT,                        BUTTON_NONE},
    {ACTION_STD_NEXTREPEAT,     BUTTON_NEXT|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_STD_OK,             BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {ACTION_STD_CONTEXT,        BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_PLAY},
    {ACTION_STD_CANCEL,         BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[] = {
    {ACTION_WPS_PLAY,           BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {ACTION_WPS_STOP,           BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_WPS_VOLUP,          BUTTON_VOL_UP|BUTTON_REL,           BUTTON_NONE},
    {ACTION_WPS_VOLDOWN,        BUTTON_VOL_DOWN|BUTTON_REL,         BUTTON_NONE},
    {ACTION_WPS_SKIPNEXT,       BUTTON_NEXT|BUTTON_REL,             BUTTON_NEXT},
    {ACTION_WPS_SKIPPREV,       BUTTON_PREV|BUTTON_REL,             BUTTON_PREV},
    {ACTION_WPS_SEEKFWD,        BUTTON_NEXT|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_WPS_STOPSEEK,       BUTTON_NEXT|BUTTON_REL,             BUTTON_NEXT|BUTTON_REPEAT},
    {ACTION_WPS_SEEKBACK,       BUTTON_PREV|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_WPS_STOPSEEK,       BUTTON_PREV|BUTTON_REL,             BUTTON_PREV|BUTTON_REPEAT},
    {ACTION_STD_KEYLOCK,        BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[] = {
    {ACTION_LIST_VOLUP,         BUTTON_VOL_UP|BUTTON_REL,           BUTTON_NONE},
    {ACTION_LIST_VOLDOWN,       BUTTON_VOL_DOWN|BUTTON_REL,         BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_yesno[] = {
    /* note: touchscreen buttons are usable in addition to physical keys */
    {ACTION_YESNO_ACCEPT,       BUTTON_PLAY,                        BUTTON_NONE},
    {ACTION_STD_CANCEL,         BUTTON_POWER,                       BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_yesno */

const struct button_mapping* target_get_context_mapping(int context)
{
    switch (context)
    {
        default:
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
        case CONTEXT_TREE:
        case CONTEXT_CUSTOM|CONTEXT_TREE:
        case CONTEXT_MAINMENU:
        case CONTEXT_BOOKMARKSCREEN:
        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
    }
}
