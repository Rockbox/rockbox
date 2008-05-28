/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* *
 * Button Code Definitions for iaudio M3 targets
 */

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* CONTEXT_CUSTOM's used in this file...

CONTEXT_CUSTOM|CONTEXT_TREE = the standard list/tree defines (without directions)


*/

/** Keep things alphabetized for easy reference but standard contexts can
 ** be first and sub-alphebetized.
 **/

/** Standard Button Contexts **/
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },

    { ACTION_STD_CONTEXT,     BUTTON_MODE|BUTTON_REPEAT,     BUTTON_MODE },
    { ACTION_STD_CANCEL,      BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_PLAY,                   BUTTON_NONE },
    { ACTION_STD_MENU,        BUTTON_REC|BUTTON_REL,         BUTTON_REC },
    { ACTION_STD_QUICKSCREEN, BUTTON_REC|BUTTON_REPEAT,      BUTTON_REC },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping remote_button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },

    { ACTION_STD_CONTEXT,     BUTTON_RC_MODE|BUTTON_REPEAT,     BUTTON_RC_MODE },
    { ACTION_STD_CANCEL,      BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RC_FF,                     BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RC_PLAY,                   BUTTON_NONE },
    { ACTION_STD_MENU,        BUTTON_RC_MENU|BUTTON_REL,        BUTTON_RC_MENU },
    { ACTION_STD_QUICKSCREEN, BUTTON_RC_MENU|BUTTON_REPEAT,     BUTTON_RC_MENU },

    LAST_ITEM_IN_LIST
}; /* remote_button_context_standard */

/** Bookmark Screen **/
static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,      BUTTON_REC|BUTTON_REPEAT,      BUTTON_REC },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_settings_bmark */

static const struct button_mapping remote_button_context_bmark[]  = {
    { ACTION_BMS_DELETE,      BUTTON_RC_REC|BUTTON_REPEAT,   BUTTON_RC_REC },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_settings_bmark */

/** FM Radio Screen **/
static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,         BUTTON_MODE|BUTTON_REPEAT,     BUTTON_MODE },
    { ACTION_FM_PRESET,       BUTTON_MODE|BUTTON_REL,        BUTTON_MODE },
    { ACTION_FM_STOP,         BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_FM_PLAY,         BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_FM_MODE,         BUTTON_REC|BUTTON_REPEAT,      BUTTON_REC },
    { ACTION_FM_EXIT,         BUTTON_REC|BUTTON_REL,         BUTTON_REC },
    { ACTION_STD_PREVREPEAT,  BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */

static const struct button_mapping remote_button_context_radio[]  = {
    { ACTION_FM_MENU,         BUTTON_RC_MODE|BUTTON_REPEAT,  BUTTON_RC_MODE },
    { ACTION_FM_PRESET,       BUTTON_RC_MODE|BUTTON_REL,     BUTTON_RC_MODE },
    { ACTION_FM_STOP,         BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_RC_PLAY },
    { ACTION_FM_PLAY,         BUTTON_RC_PLAY|BUTTON_REL,     BUTTON_RC_PLAY },
    { ACTION_FM_MODE,         BUTTON_RC_REC|BUTTON_REPEAT,   BUTTON_RC_REC },
    { ACTION_FM_EXIT,         BUTTON_RC_MENU,                BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_RC_FF,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* remote_button_context_radio */

/** Keyboard **/
static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,        BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_KBD_LEFT,        BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_RIGHT,       BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_KBD_RIGHT,       BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_KBD_UP,          BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_KBD_UP,          BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_KBD_DOWN,        BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_KBD_DOWN,        BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,   BUTTON_MODE,                   BUTTON_NONE },
    { ACTION_KBD_SELECT,      BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_KBD_DONE,        BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_KBD_ABORT,       BUTTON_REC,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping remote_button_context_keyboard[] = {
    { ACTION_KBD_LEFT,        BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_KBD_LEFT,        BUTTON_RC_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_RIGHT,       BUTTON_RC_FF,                     BUTTON_NONE },
    { ACTION_KBD_RIGHT,       BUTTON_RC_FF|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_UP,          BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_KBD_UP,          BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_DOWN,        BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_KBD_DOWN,        BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,   BUTTON_RC_MODE,                   BUTTON_NONE },
    { ACTION_KBD_SELECT,      BUTTON_RC_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_KBD_DONE,        BUTTON_RC_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_KBD_ABORT,       BUTTON_RC_REC,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* remote_button_context_keyboard */

/** Pitchscreen **/
static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_VOL_UP,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_MODE,                    BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_REC,                     BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_PLAY,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchscreen */

static const struct button_mapping remote_button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_RC_REW|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RC_FF,                     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RC_FF|BUTTON_REL,          BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_RC_MODE,                   BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_RC_REC,                    BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_RC_PLAY,                   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_pitchscreen */

/** Quickscreen **/
static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV,      BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_QS_DOWNINV,      BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_DOWN,         BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_QS_DOWN,         BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_LEFT,         BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_QS_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_QS_RIGHT,        BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_QS_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_REC,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping remote_button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV,      BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_QS_DOWNINV,      BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_DOWN,         BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_QS_DOWN,         BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_LEFT,         BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_QS_LEFT,         BUTTON_RC_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,        BUTTON_RC_FF,                     BUTTON_NONE },
    { ACTION_QS_RIGHT,        BUTTON_RC_FF|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_RC_REC,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_quickscreen */

/** Recording Screen **/
static const struct button_mapping button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,          BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_STD_CANCEL,         BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_REC_NEWFILE,        BUTTON_REC|BUTTON_REL,      BUTTON_REC },
    { ACTION_STD_MENU,           BUTTON_REC|BUTTON_REPEAT,   BUTTON_REC },
    { ACTION_SETTINGS_INC,       BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */

static const struct button_mapping remote_button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,          BUTTON_RC_PLAY|BUTTON_REL,     BUTTON_RC_PLAY },
    { ACTION_STD_CANCEL,         BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_RC_PLAY },
    { ACTION_REC_NEWFILE,        BUTTON_RC_REC,                 BUTTON_NONE },
    { ACTION_STD_MENU,           BUTTON_RC_MENU,                BUTTON_NONE },
    { ACTION_SETTINGS_INC,       BUTTON_RC_FF,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_RC_REW,                 BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_recscreen */

/** Settings - General Mappings **/
static const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,       BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_REC,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping remote_button_context_settings[] = {
    { ACTION_SETTINGS_INC,       BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_RC_REC,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_settings */

/** Settings - Using Sliders **/
static const struct button_mapping button_context_settings_r_is_inc[]  = {
    { ACTION_SETTINGS_INC,       BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_REC,                 BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_r_is_inc */

static const struct button_mapping remote_button_context_settings_r_is_inc[]  = {
    { ACTION_SETTINGS_INC,       BUTTON_RC_FF,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RC_FF|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_RC_REW,              BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_RC_REW|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_RC_REC,              BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_settings_r_is_inc */

/** Tree **/
static const struct button_mapping button_context_tree[]  = {
    { ACTION_NONE,            BUTTON_PLAY,                   BUTTON_NONE },
    { ACTION_TREE_WPS,        BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_TREE_STOP,       BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_tree */

static const struct button_mapping remote_button_context_tree[]  = {
    { ACTION_NONE,            BUTTON_RC_PLAY,                BUTTON_NONE },
    { ACTION_TREE_WPS,        BUTTON_RC_PLAY|BUTTON_REL,     BUTTON_RC_PLAY },
    { ACTION_TREE_STOP,       BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_RC_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* remote_button_context_tree */

static const struct button_mapping button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,            BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT },
    { ACTION_TREE_ROOT_INIT,  BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,     BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_TREE_PGLEFT,     BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_NONE,            BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,    BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,    BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT|BUTTON_REPEAT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_tree_scroll_lr */

static const struct button_mapping remote_button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,            BUTTON_RC_REW,                 BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_RC_REW|BUTTON_REL,      BUTTON_RC_REW },
    { ACTION_TREE_ROOT_INIT,  BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_RC_REW },
    { ACTION_TREE_PGLEFT,     BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_TREE_PGLEFT,     BUTTON_RC_REW|BUTTON_REL,      BUTTON_RC_REW|BUTTON_REPEAT },
    { ACTION_NONE,            BUTTON_RC_FF,                  BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RC_FF|BUTTON_REL,       BUTTON_RC_FF },
    { ACTION_TREE_PGRIGHT,    BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,    BUTTON_RC_FF|BUTTON_REL,       BUTTON_RC_FF|BUTTON_REPEAT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* remote_button_context_tree_scroll_lr */

/** While-Playing Screen (WPS) **/
static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,        BUTTON_PLAY|BUTTON_REL,        BUTTON_PLAY },
    { ACTION_WPS_STOP,        BUTTON_PLAY|BUTTON_REPEAT,     BUTTON_PLAY },
    { ACTION_WPS_SKIPPREV,    BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,    BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,    BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT },
    { ACTION_WPS_SEEKFWD,     BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT|BUTTON_REPEAT },

    { ACTION_WPS_VOLDOWN,     BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },

    { ACTION_WPS_BROWSE,      BUTTON_MODE|BUTTON_REL,        BUTTON_MODE },
    { ACTION_WPS_CONTEXT,     BUTTON_MODE|BUTTON_REPEAT,     BUTTON_MODE },
    { ACTION_WPS_MENU,        BUTTON_REC|BUTTON_REL,         BUTTON_REC },
    { ACTION_WPS_QUICKSCREEN, BUTTON_REC|BUTTON_REPEAT,      BUTTON_REC },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping remote_button_context_wps[]  = {
    { ACTION_WPS_PLAY,        BUTTON_RC_PLAY|BUTTON_REL,     BUTTON_RC_PLAY },
    { ACTION_WPS_STOP,        BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_RC_PLAY },
    { ACTION_WPS_SKIPPREV,    BUTTON_RC_REW|BUTTON_REL,      BUTTON_RC_REW },
    { ACTION_WPS_SEEKBACK,    BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_RC_REW|BUTTON_REL,      BUTTON_RC_REW|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,    BUTTON_RC_FF|BUTTON_REL,       BUTTON_RC_FF },
    { ACTION_WPS_SEEKFWD,     BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_RC_FF|BUTTON_REL,       BUTTON_RC_FF|BUTTON_REPEAT },

    { ACTION_WPS_VOLDOWN,     BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },

    { ACTION_WPS_BROWSE,      BUTTON_RC_MODE|BUTTON_REL,     BUTTON_RC_MODE },
    { ACTION_WPS_CONTEXT,     BUTTON_RC_MODE|BUTTON_REPEAT,  BUTTON_RC_MODE },
    { ACTION_WPS_MENU,        BUTTON_RC_MENU|BUTTON_REL,     BUTTON_RC_MENU },
    { ACTION_WPS_QUICKSCREEN, BUTTON_RC_MENU|BUTTON_REPEAT,  BUTTON_RC_MENU },

    LAST_ITEM_IN_LIST
}; /* remote_button_context_wps */

/** Yes/No Screen **/
static const struct button_mapping button_context_yesnoscreen[]  = {
    { ACTION_YESNO_ACCEPT,    BUTTON_PLAY,                   BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_yesnoscreen */

static const struct button_mapping remote_button_context_yesnoscreen[]  = {
    { ACTION_YESNO_ACCEPT,    BUTTON_RC_PLAY,                BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* remote_button_context_yesnoscreen */


static const struct button_mapping* get_context_mapping_remote( int context )
{
    context ^= CONTEXT_REMOTE;

    switch (context)
    {
        /* anything that uses remote_button_context_standard */
        default:
            return remote_button_context_standard;

        /* remote contexts with special mapping */
        case CONTEXT_BOOKMARKSCREEN:
            return remote_button_context_bmark;

        case CONTEXT_FM:
            return remote_button_context_radio;

        case CONTEXT_KEYBOARD:
            return remote_button_context_keyboard;

        case CONTEXT_PITCHSCREEN:
            return remote_button_context_pitchscreen;

        case CONTEXT_QUICKSCREEN:
            return remote_button_context_quickscreen;

        case CONTEXT_RECSCREEN:
            return remote_button_context_recscreen;

        case CONTEXT_SETTINGS:
            return remote_button_context_settings;

        case CONTEXT_SETTINGS_EQ:
            return remote_button_context_settings_r_is_inc;

        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            if (global_settings.hold_lr_for_scroll_in_list)
                return remote_button_context_tree_scroll_lr;
            /* else fall through to CONTEXT_TREE|CONTEXT_CUSTOM */
        case CONTEXT_TREE|CONTEXT_CUSTOM:
            return remote_button_context_tree;

        case CONTEXT_WPS:
            return remote_button_context_wps;

        case CONTEXT_YESNOSCREEN:
            return remote_button_context_yesnoscreen;
    }
}

const struct button_mapping* get_context_mapping(int context)
{
    if (context & CONTEXT_REMOTE)
        return get_context_mapping_remote(context);

    switch (context)
    {
        /* anything that uses button_context_standard */
        default:
            return button_context_standard;

        /* contexts with special mapping */
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;

        case CONTEXT_FM:
            return button_context_radio;

        case CONTEXT_KEYBOARD:
            return button_context_keyboard;

        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;

        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;

        case CONTEXT_RECSCREEN:
            return button_context_recscreen;

        case CONTEXT_SETTINGS:
            return button_context_settings;

        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings_r_is_inc;

        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_tree_scroll_lr;
            /* else fall through to CONTEXT_TREE|CONTEXT_CUSTOM */
        case CONTEXT_TREE|CONTEXT_CUSTOM:
            return button_context_tree;

        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesnoscreen;
    }
}
