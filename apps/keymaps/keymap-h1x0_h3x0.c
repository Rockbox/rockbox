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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Button Code Definitions for iriver h100/h300 target */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "action.h"
#include "button.h"
#include "lcd-remote.h" /* for remote_type() */
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
    { ACTION_STD_PREV,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    
    { ACTION_STD_CANCEL,        BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_OFF,                 BUTTON_NONE },
    
    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_SELECT },

    { ACTION_STD_QUICKSCREEN,   BUTTON_MODE|BUTTON_REPEAT,  BUTTON_MODE }, 
    { ACTION_STD_MENU,          BUTTON_MODE|BUTTON_REL,     BUTTON_MODE },
    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,   BUTTON_SELECT },
    { ACTION_STD_OK,            BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_ON|BUTTON_REL,       BUTTON_NONE },
    { ACTION_STD_REC,           BUTTON_REC|BUTTON_REPEAT,        BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,          BUTTON_ON|BUTTON_REL,           BUTTON_ON },
    { ACTION_WPS_SKIPNEXT,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,      BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,       BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_ABSETB_NEXTDIR,       BUTTON_ON|BUTTON_RIGHT,         BUTTON_NONE },
    { ACTION_WPS_ABSETA_PREVDIR,       BUTTON_ON|BUTTON_LEFT,          BUTTON_NONE },
    { ACTION_WPS_STOP,          BUTTON_OFF|BUTTON_REL,          BUTTON_OFF },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP,                      BUTTON_NONE },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_ON|BUTTON_UP,            BUTTON_ON },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_ON|BUTTON_DOWN,          BUTTON_ON },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_MODE|BUTTON_REPEAT,      BUTTON_MODE },
    { ACTION_WPS_MENU,          BUTTON_MODE|BUTTON_REL,         BUTTON_MODE },
    { ACTION_WPS_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT },
    { ACTION_WPS_BROWSE,        BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },
    { ACTION_WPS_ABRESET,       BUTTON_ON|BUTTON_SELECT,        BUTTON_ON },
    { ACTION_WPS_ID3SCREEN,     BUTTON_ON|BUTTON_MODE,          BUTTON_NONE },
    { ACTION_WPS_REC,           BUTTON_REC|BUTTON_REPEAT,        BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */
   
static const struct button_mapping button_context_list[]  = {
    { ACTION_LISTTREE_PGUP,         BUTTON_ON|BUTTON_UP,                    BUTTON_ON },
    { ACTION_LISTTREE_PGUP,         BUTTON_UP|BUTTON_REL,                   BUTTON_ON|BUTTON_UP },
    { ACTION_LISTTREE_PGUP,         BUTTON_ON|BUTTON_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_ON|BUTTON_DOWN,                  BUTTON_ON },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_DOWN|BUTTON_REL,                 BUTTON_ON|BUTTON_DOWN },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_ON|BUTTON_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_NONE,                BUTTON_ON|BUTTON_REL,                   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_ON|BUTTON_REL,         BUTTON_ON },
    { ACTION_TREE_STOP,   BUTTON_OFF,                   BUTTON_NONE },
    { ACTION_TREE_STOP,   BUTTON_OFF|BUTTON_REPEAT,     BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    { ACTION_NONE,              BUTTON_ON,                              BUTTON_NONE }, 
    { ACTION_TREE_PGLEFT,       BUTTON_ON|BUTTON_LEFT,                  BUTTON_ON },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,                 BUTTON_ON|BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_ON|BUTTON_LEFT,                  BUTTON_LEFT|BUTTON_REL },
    { ACTION_TREE_ROOT_INIT,    BUTTON_ON|BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_ON|BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_ON|BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_ON|BUTTON_RIGHT,                 BUTTON_ON },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,                BUTTON_ON|BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_ON|BUTTON_RIGHT,                 BUTTON_RIGHT|BUTTON_REL },
    { ACTION_TREE_PGRIGHT,      BUTTON_ON|BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
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
};

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_RESET,        BUTTON_ON,                      BUTTON_NONE },
    
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
    { ACTION_SETTINGS_RESET,        BUTTON_ON,                  BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,                  BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = {
    { ACTION_STD_OK,            BUTTON_ON|BUTTON_REL,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_settings_colorchooser */

static const struct button_mapping button_context_eq[]  = {
    { ACTION_STD_OK,                BUTTON_SELECT|BUTTON_REL,               BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_ON|BUTTON_REL,                   BUTTON_NONE },
    { ACTION_SETTINGS_INCBIGSTEP,   BUTTON_ON|BUTTON_RIGHT,                 BUTTON_ON },
    { ACTION_SETTINGS_INCBIGSTEP,   BUTTON_RIGHT|BUTTON_REL,                BUTTON_ON|BUTTON_RIGHT },
    { ACTION_SETTINGS_INCBIGSTEP,   BUTTON_ON|BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DECBIGSTEP,   BUTTON_ON|BUTTON_LEFT,                  BUTTON_ON },
    { ACTION_SETTINGS_DECBIGSTEP,   BUTTON_LEFT|BUTTON_REL,                 BUTTON_ON|BUTTON_LEFT },
    { ACTION_SETTINGS_DECBIGSTEP,   BUTTON_ON|BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_settings_context_eq */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,       BUTTON_REC,      BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_settings_bmark */

static const struct button_mapping button_context_time[]  = {
    { ACTION_STD_CANCEL,       BUTTON_OFF,  BUTTON_NONE },
    { ACTION_STD_OK,           BUTTON_ON,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_settings_bmark */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV,    BUTTON_UP,                      BUTTON_NONE },
    { ACTION_QS_DOWNINV,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_MODE,                    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_MODE,                BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_SELECT,              BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_ON,                  BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_OFF,                 BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,             BUTTON_ON,                  BUTTON_NONE },
    { ACTION_REC_NEWFILE,           BUTTON_REC,                 BUTTON_NONE },
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                           BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,             BUTTON_NONE },   
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                          BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_ON|BUTTON_LEFT,                 BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_ON|BUTTON_LEFT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_ON|BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_ON|BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_SELECT,                         BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_MODE,                           BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_ON|BUTTON_REL,                  BUTTON_ON },
    { ACTION_KBD_ABORT,        BUTTON_OFF,                            BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_REC,                            BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_REC|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                             BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                           BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,             BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_ON|BUTTON_MODE,                 BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_SELECT|BUTTON_REL,              BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,        BUTTON_SELECT | BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_FM_PRESET,      BUTTON_SELECT | BUTTON_REL,            BUTTON_SELECT },
    { ACTION_FM_STOP,        BUTTON_OFF,                            BUTTON_NONE },
    { ACTION_FM_MODE,        BUTTON_ON | BUTTON_REPEAT,             BUTTON_ON },
    { ACTION_FM_EXIT,        BUTTON_MODE | BUTTON_REL,              BUTTON_MODE },
    { ACTION_FM_PLAY,        BUTTON_ON | BUTTON_REL,                BUTTON_ON },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
    
};

/*****************************************************************************
 *    Remote control mappings 
 *****************************************************************************/
 
 
static const struct button_mapping button_context_standard_h100remote[]  = {
    { ACTION_STD_PREV,          BUTTON_RC_REW,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_RC_FF,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT,     BUTTON_NONE },
    
    { ACTION_STD_CANCEL,        BUTTON_RC_STOP,                 BUTTON_NONE },
    { ACTION_STD_CONTEXT,       BUTTON_RC_MENU|BUTTON_REPEAT,   BUTTON_RC_MENU },

    { ACTION_STD_QUICKSCREEN,   BUTTON_RC_MODE|BUTTON_REPEAT,   BUTTON_RC_MODE },
    { ACTION_STD_MENU,          BUTTON_RC_MODE|BUTTON_REL,      BUTTON_RC_MODE },
    { ACTION_STD_OK,            BUTTON_RC_ON,                   BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RC_MENU|BUTTON_REL,      BUTTON_RC_MENU },
    
    LAST_ITEM_IN_LIST
}; /* button_context_standard_h100lcdremote */

static const struct button_mapping button_context_standard_h300lcdremote[]  = {
    { ACTION_STD_PREV,          BUTTON_RC_VOL_UP,                   BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_RC_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_RC_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },

    { ACTION_STD_CANCEL,        BUTTON_RC_REW,                      BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_RC_STOP,                     BUTTON_NONE },
    { ACTION_STD_CONTEXT,       BUTTON_RC_MENU|BUTTON_REPEAT,       BUTTON_RC_MENU },
    { ACTION_STD_QUICKSCREEN,   BUTTON_RC_MODE|BUTTON_REPEAT,       BUTTON_RC_MODE },
    { ACTION_STD_MENU,          BUTTON_RC_MODE|BUTTON_REL,          BUTTON_RC_MODE },
    { ACTION_STD_OK,            BUTTON_RC_MENU|BUTTON_REL,          BUTTON_RC_MENU },
    { ACTION_STD_OK,            BUTTON_RC_FF,                       BUTTON_NONE },
    
    LAST_ITEM_IN_LIST

};

static const struct button_mapping button_context_wps_remotescommon[]  = {
    { ACTION_WPS_PLAY,              BUTTON_RC_ON|BUTTON_REL,            BUTTON_RC_ON },
    { ACTION_WPS_SKIPNEXT,          BUTTON_RC_FF|BUTTON_REL,            BUTTON_RC_FF },
    { ACTION_WPS_SKIPPREV,          BUTTON_RC_REW|BUTTON_REL,           BUTTON_RC_REW },
    { ACTION_WPS_SEEKBACK,          BUTTON_RC_REW|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,           BUTTON_RC_FF|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,          BUTTON_RC_REW|BUTTON_REL,           BUTTON_RC_REW|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,          BUTTON_RC_FF|BUTTON_REL,            BUTTON_RC_FF|BUTTON_REPEAT },
    { ACTION_WPS_ABSETB_NEXTDIR,    BUTTON_RC_BITRATE,                  BUTTON_NONE },
    { ACTION_WPS_ABSETA_PREVDIR,    BUTTON_RC_SOURCE,                   BUTTON_NONE },
    { ACTION_WPS_STOP,              BUTTON_RC_STOP|BUTTON_REL,          BUTTON_RC_STOP },
    { ACTION_WPS_VOLDOWN,           BUTTON_RC_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,           BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,             BUTTON_RC_VOL_UP,                   BUTTON_NONE },
    { ACTION_WPS_VOLUP,             BUTTON_RC_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_PITCHSCREEN,       BUTTON_RC_ON|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_WPS_QUICKSCREEN,       BUTTON_RC_MODE|BUTTON_REPEAT,       BUTTON_RC_MODE },
    { ACTION_WPS_MENU,              BUTTON_RC_MODE|BUTTON_REL,          BUTTON_RC_MODE },
    { ACTION_WPS_CONTEXT,           BUTTON_RC_MENU|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_WPS_BROWSE,            BUTTON_RC_MENU|BUTTON_REL,          BUTTON_RC_MENU },
/* Now the specific combos, because H100 & H300 LCD remotes have different 
 * keys, capable of acting as "modifier" - H100 : RC_ON; H300: RC_MENU 
 */    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_REMOTE|CONTEXT_WPS)
}; 


static const struct button_mapping button_context_wps_h100remote[] = {
    { ACTION_WPS_ABRESET,           BUTTON_RC_ON|BUTTON_RC_MENU,        BUTTON_RC_ON },
    { ACTION_WPS_ID3SCREEN,         BUTTON_RC_ON|BUTTON_RC_MODE,        BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};
static const struct button_mapping button_context_wps_h300lcdremote[] = {
    { ACTION_WPS_ABRESET,           BUTTON_RC_MENU|BUTTON_RC_ON,        BUTTON_RC_MENU },
    { ACTION_WPS_ID3SCREEN,         BUTTON_RC_MENU|BUTTON_RC_MODE,      BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};

static const struct button_mapping button_context_list_h100remote[]  = {
    { ACTION_LISTTREE_PGUP,         BUTTON_RC_SOURCE,                       BUTTON_NONE },
    { ACTION_LISTTREE_PGUP,         BUTTON_RC_SOURCE|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_RC_BITRATE,                      BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_RC_BITRATE|BUTTON_REPEAT,        BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};

static const struct button_mapping *button_context_list_h300lcdremote =
                button_context_list_h100remote;


static const struct button_mapping button_context_tree_h100remote[]  = {
    { ACTION_TREE_WPS,    BUTTON_RC_ON,                     BUTTON_NONE },   

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST|CONTEXT_REMOTE)
}; /* button_context_tree_h100remote */

static const struct button_mapping button_context_tree_h300lcdremote[] = {
    { ACTION_TREE_STOP,     BUTTON_RC_STOP,                 BUTTON_NONE },
    { ACTION_TREE_WPS,      BUTTON_RC_ON,                   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST|CONTEXT_REMOTE)
};  /* button_context_tree_h300lcdremote */
               

static const struct button_mapping button_context_listtree_scroll_w_cmb_h100remote[]  = {
    { ACTION_TREE_PGLEFT,       BUTTON_RC_ON|BUTTON_RC_REW,               BUTTON_RC_ON },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_REL,                 BUTTON_RC_ON|BUTTON_RC_REW },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_ON|BUTTON_RC_REW,               BUTTON_RC_REW|BUTTON_REL },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_ON|BUTTON_RC_REW|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_VOL_DOWN,                       BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_ON|BUTTON_RC_FF,                BUTTON_RC_ON },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_REL,                  BUTTON_RC_ON|BUTTON_RC_FF },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_ON|BUTTON_RC_FF,                BUTTON_RC_FF|BUTTON_REL },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_ON|BUTTON_RC_FF|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_VOL_UP,                         BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_VOL_UP|BUTTON_REPEAT,           BUTTON_NONE }, 
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE|CONTEXT_REMOTE),
};

static const struct button_mapping button_context_listtree_scroll_w_cmb_h300lcdremote[]  = {
    { ACTION_TREE_PGLEFT,       BUTTON_RC_MENU|BUTTON_RC_SOURCE,                BUTTON_RC_MENU },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_SOURCE|BUTTON_REL,                    BUTTON_RC_MENU|BUTTON_RC_SOURCE },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_MENU|BUTTON_RC_SOURCE,                BUTTON_RC_SOURCE|BUTTON_REL },
    { ACTION_TREE_ROOT_INIT,    BUTTON_RC_MENU|BUTTON_RC_SOURCE|BUTTON_REPEAT,  BUTTON_RC_MENU|BUTTON_RC_SOURCE },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_MENU|BUTTON_RC_SOURCE|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_MENU|BUTTON_RC_BITRATE,               BUTTON_RC_MENU },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_BITRATE|BUTTON_REL,                   BUTTON_RC_MENU|BUTTON_RC_BITRATE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_MENU|BUTTON_RC_BITRATE,               BUTTON_RC_BITRATE|BUTTON_REL },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_MENU|BUTTON_RC_BITRATE|BUTTON_REPEAT, BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE|CONTEXT_REMOTE),
};

static const struct button_mapping button_context_listtree_scroll_wo_cmb_h100remote[]  = {
    { ACTION_TREE_PGLEFT,       BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_TREE_ROOT_INIT,    BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_RC_VOL_DOWN },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE|CONTEXT_REMOTE),
};

static const struct button_mapping button_context_listtree_scroll_wo_cmb_h300lcdremote[]  = {
    { ACTION_NONE,              BUTTON_RC_REW,               BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_RC_REW|BUTTON_REL,    BUTTON_RC_REW },
    { ACTION_TREE_ROOT_INIT,    BUTTON_RC_REW|BUTTON_REPEAT, BUTTON_RC_REW },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_REL,    BUTTON_RC_REW|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_RC_FF,                BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RC_FF|BUTTON_REL,     BUTTON_RC_FF },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_REL,     BUTTON_RC_FF|BUTTON_REPEAT },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE|CONTEXT_REMOTE),
};

static const struct button_mapping button_context_settings_h100remote[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RC_REW,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_RC_FF,                   BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT,     BUTTON_NONE },
/*    { ACTION_NONE,                  BUTTON_RC_ON,                   BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_RC_STOP,                 BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_RC_MENU|BUTTON_REL,      BUTTON_NONE },
*/    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_h300lcdremote[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RC_VOL_UP,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RC_VOL_UP|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_RC_VOL_DOWN,             BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_RC_REW,                  BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_RC_FF,                   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

 

static const struct button_mapping button_context_settingsgraphical_h100remote[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RC_FF,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_RC_REW,                 BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_RC_SOURCE,              BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_RC_SOURCE|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RC_BITRATE,             BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RC_BITRATE|BUTTON_REPEAT, BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical_h100remote */

static const struct button_mapping button_context_settingsgraphical_h300lcdremote[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RC_FF,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_RC_REW,              BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_RC_VOL_UP,           BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_RC_VOL_UP|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RC_VOL_DOWN,         BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical_h300remote */

static const struct button_mapping button_context_yesno_h100remote[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_RC_MENU,                   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping *button_context_yesno_h300lcdremote =
                button_context_yesno_h100remote;

static const struct button_mapping button_context_bmark_h100remote[] = {
    { ACTION_BMS_DELETE,      BUTTON_RC_REC,    BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_settings_bmark */

static const struct button_mapping *button_context_bmark_h300lcdremote =
                button_context_bmark_h100remote;

static const struct button_mapping button_context_quickscreen_nonlcdremote[]  = {
    { ACTION_QS_DOWNINV,    BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_QS_DOWNINV,    BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_FF,                     BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_FF|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_RC_ON,                   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_quickscreen_h100lcdremote[]  = {
    { ACTION_QS_DOWNINV,    BUTTON_RC_VOL_UP,                   BUTTON_NONE },
    { ACTION_QS_DOWNINV,    BUTTON_RC_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_RC_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_REW,                      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_REW|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_FF,                      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_FF|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_SOURCE,                  BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_SOURCE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_BITRATE,                  BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_BITRATE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_RC_MODE,                     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_quickscreen_h300lcdremote[]  = {
    { ACTION_QS_DOWNINV,    BUTTON_RC_VOL_UP,               BUTTON_NONE },
    { ACTION_QS_DOWNINV,    BUTTON_RC_VOL_UP|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_RC_VOL_DOWN,             BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_REW,                  BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_RC_REW|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_FF,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RC_FF|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_RC_MODE,                 BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen_nonlcdremote[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_RC_VOL_UP,           BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_RC_VOL_UP|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_RC_VOL_DOWN,         BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_RC_REW,              BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_RC_REW|BUTTON_REL,   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RC_FF,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RC_FF|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_RC_MENU,             BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_RC_ON,               BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_RC_STOP,             BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_pitchscreen_h100lcdremote[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_RC_FF,                   BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_RC_FF|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_RC_REW,                  BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_RC_REW|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_RC_SOURCE,               BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_RC_SOURCE|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RC_BITRATE,              BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RC_BITRATE|BUTTON_REL,   BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_RC_MENU,                 BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_RC_ON,                   BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_RC_STOP,                 BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};

static const struct button_mapping button_context_pitchscreen_h300lcdremote[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_RC_VOL_UP,           BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_RC_VOL_UP|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_RC_VOL_DOWN,         BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_RC_REW,              BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_RC_REW|BUTTON_REL,   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RC_FF,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RC_FF|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_RC_MENU,             BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_RC_ON,               BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_RC_STOP,             BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};

static const struct button_mapping button_context_recscreen_h100remote[]  = {
    { ACTION_REC_LCD,             BUTTON_RC_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_REC_PAUSE,           BUTTON_RC_ON,                       BUTTON_NONE },
    { ACTION_REC_NEWFILE,         BUTTON_RC_REC,                      BUTTON_NONE },
    { ACTION_SETTINGS_INC,        BUTTON_RC_BITRATE,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,  BUTTON_RC_BITRATE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,        BUTTON_RC_SOURCE,                   BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,  BUTTON_RC_SOURCE|BUTTON_REPEAT,     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen_h100remote */

static const struct button_mapping button_context_recscreen_h300lcdremote[]  = {
    { ACTION_REC_LCD,             BUTTON_RC_SOURCE,              BUTTON_NONE },
    { ACTION_REC_PAUSE,           BUTTON_RC_ON,                  BUTTON_NONE },
    { ACTION_REC_NEWFILE,         BUTTON_RC_REC,                 BUTTON_NONE },
    { ACTION_SETTINGS_INC,        BUTTON_RC_FF,                  BUTTON_NONE },
    { ACTION_SETTINGS_INC,        BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,        BUTTON_RC_REW,                 BUTTON_NONE },
    { ACTION_SETTINGS_DEC,        BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen_h300lcdremote */

static const struct button_mapping button_context_keyboard_h100remote[]  = {
    { ACTION_KBD_LEFT,         BUTTON_RC_REW,                            BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_RC_REW|BUTTON_REPEAT,              BUTTON_NONE },   
    { ACTION_KBD_RIGHT,        BUTTON_RC_FF,                             BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RC_FF|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RC_ON|BUTTON_RC_REW,               BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RC_ON|BUTTON_RC_REW|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RC_ON|BUTTON_RC_FF,                BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RC_ON|BUTTON_RC_FF|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RC_VOL_DOWN,                       BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RC_VOL_UP,                         BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RC_VOL_UP|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_KBD_SELECT_REM,   BUTTON_RC_MENU,                           BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_RC_MODE,                           BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_RC_ON|BUTTON_REL,                  BUTTON_RC_ON },
    { ACTION_KBD_ABORT,        BUTTON_RC_STOP,                           BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_RC_REC,                            BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_RC_REC|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_RC_SOURCE,                         BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_RC_SOURCE|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_RC_BITRATE,                        BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_RC_BITRATE|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_RC_ON|BUTTON_RC_MODE,              BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_RC_MENU|BUTTON_REL,                BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard_h100remote */

static const struct button_mapping button_context_keyboard_h300lcdremote[]  = {
    { ACTION_KBD_LEFT,         BUTTON_RC_REW,                            BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_RC_REW|BUTTON_REPEAT,              BUTTON_NONE },   
    { ACTION_KBD_RIGHT,        BUTTON_RC_FF,                             BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RC_FF|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RC_SOURCE,                         BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RC_SOURCE|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RC_BITRATE,                        BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RC_BITRATE|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_KBD_SELECT_REM,   BUTTON_RC_MENU,                           BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_RC_MODE,                           BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_RC_ON,                             BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_RC_STOP,                           BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_RC_REC,                            BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_RC_REC|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_RC_VOL_UP,                         BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_RC_VOL_UP|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_RC_VOL_DOWN,                       BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_RC_MENU|BUTTON_RC_MODE,            BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_RC_MENU|BUTTON_REL,                BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard_h300lcdremote */

static const struct button_mapping button_context_radio_h100remote[]  = {
    { ACTION_FM_MENU,        BUTTON_RC_MENU | BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_FM_PRESET,      BUTTON_RC_MENU | BUTTON_REL,            BUTTON_RC_MENU },
    { ACTION_FM_STOP,        BUTTON_RC_STOP,                         BUTTON_NONE },
    { ACTION_FM_MODE,        BUTTON_RC_ON | BUTTON_REPEAT,           BUTTON_RC_ON },
    { ACTION_FM_EXIT,        BUTTON_RC_MODE | BUTTON_REL,            BUTTON_RC_MODE },
    { ACTION_FM_PLAY,        BUTTON_RC_ON | BUTTON_REL,              BUTTON_RC_ON },
    { ACTION_FM_NEXT_PRESET, BUTTON_RC_BITRATE,                      BUTTON_NONE },
    { ACTION_FM_PREV_PRESET, BUTTON_RC_SOURCE,                       BUTTON_NONE },
    { ACTION_SETTINGS_INC,       BUTTON_RC_VOL_UP,                   BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RC_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_RC_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_PREV,       BUTTON_RC_REW,                          BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_RC_REW|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_RC_FF,                           BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_RC_FF|BUTTON_REPEAT,             BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

static const struct button_mapping button_context_radio_h300lcdremote[] = {
    { ACTION_FM_MENU,        BUTTON_RC_MENU | BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_FM_PRESET,      BUTTON_RC_MENU | BUTTON_REL,            BUTTON_RC_MENU },
    { ACTION_FM_STOP,        BUTTON_RC_STOP,                         BUTTON_NONE },
    { ACTION_FM_MODE,        BUTTON_RC_ON | BUTTON_REPEAT,           BUTTON_RC_ON },
    { ACTION_FM_EXIT,        BUTTON_RC_MODE | BUTTON_REL,            BUTTON_RC_MODE },
    { ACTION_FM_PLAY,        BUTTON_RC_ON | BUTTON_REL,              BUTTON_RC_ON },
    { ACTION_FM_NEXT_PRESET, BUTTON_RC_BITRATE,                      BUTTON_NONE },
    { ACTION_FM_PREV_PRESET, BUTTON_RC_SOURCE,                       BUTTON_NONE },
    { ACTION_STD_PREV,       BUTTON_RC_REW,                          BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_RC_REW|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_RC_FF,                           BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_RC_FF|BUTTON_REPEAT,             BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
};


static const struct button_mapping button_context_time_remote[]  = {
    { ACTION_STD_CANCEL,       BUTTON_OFF,  BUTTON_NONE },
    { ACTION_STD_OK,           BUTTON_ON,   BUTTON_NONE },
    { ACTION_SETTINGS_INC,     BUTTON_RC_BITRATE,  BUTTON_NONE },
    { ACTION_SETTINGS_DEC,     BUTTON_RC_SOURCE,  BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_settings_bmark */

/* the actual used tables */
static const struct button_mapping 
        *remote_btn_ctxt_std = 0, 
        *remote_btn_ctxt_wps = 0,
        *remote_btn_ctxt_list = 0,
        *remote_btn_ctxt_tree = 0,
        *remote_btn_ctxt_listtree_scroll_w_cmb = 0,
        *remote_btn_ctxt_listtree_scroll_wo_cmb = 0,
        *remote_btn_ctxt_settings = 0,
        *remote_btn_ctxt_settingsgrph = 0,
        *remote_btn_ctxt_yesno = 0,
        *remote_btn_ctxt_bmark = 0,
        *remote_btn_ctxt_quickscreen = 0,
        *remote_btn_ctxt_pitchscreen = 0,
        *remote_btn_ctxt_recscreen = 0,
        *remote_btn_ctxt_keyboard  = 0,
        *remote_btn_ctxt_radio  = 0;

static int _remote_type = -1; /*safe value, forces the first press to init the mappings */

static void remap_remote(void)
{
    _remote_type = remote_type();
    switch(_remote_type)
    {
        case REMOTETYPE_UNPLUGGED:
            remote_btn_ctxt_std = NULL; 
            remote_btn_ctxt_wps = NULL;
            remote_btn_ctxt_list = NULL;
            remote_btn_ctxt_tree = NULL;
            remote_btn_ctxt_listtree_scroll_w_cmb = NULL;
            remote_btn_ctxt_listtree_scroll_wo_cmb = NULL;
            remote_btn_ctxt_settings = NULL;
            remote_btn_ctxt_settingsgrph = NULL;
            remote_btn_ctxt_yesno = NULL;
            remote_btn_ctxt_bmark = NULL;
            remote_btn_ctxt_quickscreen = NULL;
            remote_btn_ctxt_pitchscreen = NULL;
            remote_btn_ctxt_recscreen = NULL;
            remote_btn_ctxt_keyboard = NULL;
            remote_btn_ctxt_radio  = NULL;
            break;

        case REMOTETYPE_H100_LCD:
            remote_btn_ctxt_std = button_context_standard_h100remote, 
            remote_btn_ctxt_wps = button_context_wps_h100remote,
            remote_btn_ctxt_list = button_context_list_h100remote,
            remote_btn_ctxt_tree = button_context_tree_h100remote,
            remote_btn_ctxt_listtree_scroll_w_cmb 
                = button_context_listtree_scroll_w_cmb_h100remote,
            remote_btn_ctxt_listtree_scroll_wo_cmb 
                = button_context_listtree_scroll_wo_cmb_h100remote,
            remote_btn_ctxt_settings = button_context_settings_h100remote,
            remote_btn_ctxt_settingsgrph 
                = button_context_settingsgraphical_h100remote,
            remote_btn_ctxt_yesno = button_context_yesno_h100remote,
            remote_btn_ctxt_bmark = button_context_bmark_h100remote,
            remote_btn_ctxt_quickscreen
                    = button_context_quickscreen_h100lcdremote,
            remote_btn_ctxt_pitchscreen
                    = button_context_pitchscreen_h100lcdremote,
            remote_btn_ctxt_recscreen 
                = button_context_recscreen_h100remote,
            remote_btn_ctxt_keyboard 
                = button_context_keyboard_h100remote,
            remote_btn_ctxt_radio
                = button_context_radio_h100remote;
            break;

        case REMOTETYPE_H300_LCD:
            remote_btn_ctxt_std = button_context_standard_h300lcdremote, 
            remote_btn_ctxt_wps = button_context_wps_h300lcdremote,
            remote_btn_ctxt_list = button_context_list_h300lcdremote,
            remote_btn_ctxt_tree = button_context_tree_h300lcdremote,
            remote_btn_ctxt_listtree_scroll_w_cmb 
                = button_context_listtree_scroll_w_cmb_h300lcdremote,
            remote_btn_ctxt_listtree_scroll_wo_cmb 
                = button_context_listtree_scroll_wo_cmb_h300lcdremote,
            remote_btn_ctxt_settings = button_context_settings_h300lcdremote,
            remote_btn_ctxt_settingsgrph 
                = button_context_settingsgraphical_h300lcdremote,
            remote_btn_ctxt_yesno = button_context_yesno_h300lcdremote,
            remote_btn_ctxt_bmark = button_context_bmark_h300lcdremote,
            remote_btn_ctxt_quickscreen
                = button_context_quickscreen_h300lcdremote,
            remote_btn_ctxt_pitchscreen
                = button_context_pitchscreen_h300lcdremote,
            remote_btn_ctxt_recscreen
                = button_context_recscreen_h300lcdremote,
            remote_btn_ctxt_keyboard 
                = button_context_keyboard_h300lcdremote,
            remote_btn_ctxt_radio
                = button_context_radio_h300lcdremote;
            break;

        case REMOTETYPE_H300_NONLCD: /* FIXME: add its tables */        
            remote_btn_ctxt_std = button_context_standard_h300lcdremote, 
            remote_btn_ctxt_wps = button_context_wps_h300lcdremote,
            remote_btn_ctxt_list = button_context_list_h300lcdremote,
            remote_btn_ctxt_tree = button_context_tree_h300lcdremote,
            remote_btn_ctxt_listtree_scroll_w_cmb 
                = button_context_listtree_scroll_w_cmb_h300lcdremote,
            remote_btn_ctxt_listtree_scroll_wo_cmb 
                = button_context_listtree_scroll_wo_cmb_h300lcdremote,
            remote_btn_ctxt_settings = button_context_settings_h300lcdremote,
            remote_btn_ctxt_settingsgrph 
                = button_context_settingsgraphical_h300lcdremote,
            remote_btn_ctxt_yesno = button_context_yesno_h300lcdremote,
            remote_btn_ctxt_bmark = button_context_bmark_h300lcdremote,
            remote_btn_ctxt_quickscreen
                = button_context_quickscreen_nonlcdremote,
            remote_btn_ctxt_pitchscreen
                = button_context_pitchscreen_nonlcdremote,
            remote_btn_ctxt_recscreen
                = button_context_recscreen_h300lcdremote,
            remote_btn_ctxt_keyboard 
                = button_context_keyboard_h300lcdremote,
            remote_btn_ctxt_radio
                = button_context_radio_h300lcdremote;
#if 0 
            remote_btn_ctxt_std = 
            remote_btn_ctxt_wps = 
            remote_btn_ctxt_list = 
            remote_btn_ctxt_tree = 
            remote_btn_ctxt_listtree_scroll_w_cmb = 
            remote_btn_ctxt_listtree_scroll_wo_cmb = 
            remote_btn_ctxt_settings = 
            remote_btn_ctxt_settingsgrph = 
            remote_btn_ctxt_yesno = 
            remote_btn_ctxt_bmark = 
            remote_btn_ctxt_quickscreen = 
            remote_btn_ctxt_pitchscreen = 
            remote_btn_ctxt_recscreen = 
            remote_btn_ctxt_keyboard = 
            remote_btn_ctxt_radio = 
#endif
            break;

    }
}


static const struct button_mapping* get_context_mapping_remote(int context)
{
    if(remote_type() != _remote_type)
        remap_remote();
    context ^= CONTEXT_REMOTE;
    
    switch (context)
    {
        case CONTEXT_STD:
            return remote_btn_ctxt_std;
        case CONTEXT_WPS: /* common for all remotes */
            return button_context_wps_remotescommon;

        case CONTEXT_CUSTOM|CONTEXT_WPS:
            return remote_btn_ctxt_wps; 

        case CONTEXT_LIST:
            return remote_btn_ctxt_list;
        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            if (global_settings.hold_lr_for_scroll_in_list)
                return remote_btn_ctxt_listtree_scroll_wo_cmb;
            else 
                return remote_btn_ctxt_listtree_scroll_w_cmb; 
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return remote_btn_ctxt_tree;
        case CONTEXT_SETTINGS_TIME:
            return remote_btn_ctxt_settingsgrph;
            
        case CONTEXT_SETTINGS:
            return remote_btn_ctxt_settings;
            
        case CONTEXT_YESNOSCREEN:
            return remote_btn_ctxt_yesno;
            
        case CONTEXT_BOOKMARKSCREEN:
            return remote_btn_ctxt_bmark; 
        case CONTEXT_QUICKSCREEN:
            return remote_btn_ctxt_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return remote_btn_ctxt_pitchscreen;
        case CONTEXT_RECSCREEN:
            return remote_btn_ctxt_recscreen;
        case CONTEXT_KEYBOARD:
            return remote_btn_ctxt_keyboard;
        case CONTEXT_FM:
            return remote_btn_ctxt_radio;
    } 
    return remote_btn_ctxt_std; 
}

const struct button_mapping* get_context_mapping(int context)
{
    if (context&CONTEXT_REMOTE)
        return get_context_mapping_remote(context);
    
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
            
        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_listtree_scroll_without_combo;
            else 
                return button_context_listtree_scroll_with_combo;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;
        
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
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
        case CONTEXT_FM:
            return button_context_radio;
    } 
    return button_context_standard;
}
