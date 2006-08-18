/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id $
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


*/


const struct button_mapping button_context_standard[]  = {
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
    
    LAST_ITEM_IN_LIST
}; /* button_context_standard */


const struct button_mapping button_context_wps[]  = {
    { ACTION_NONE,              BUTTON_ON,                      BUTTON_NONE },
    { ACTION_WPS_PLAY,          BUTTON_ON|BUTTON_REL,           BUTTON_ON },
    { ACTION_WPS_SKIPNEXT,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,      BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,       BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_ABSETB_NEXTDIR,       BUTTON_ON|BUTTON_RIGHT,         BUTTON_ON },
    { ACTION_WPS_ABSETA_PREVDIR,       BUTTON_ON|BUTTON_LEFT,          BUTTON_ON },
    { ACTION_WPS_STOP,          BUTTON_OFF|BUTTON_REL,          BUTTON_OFF },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN|BUTTON_REL,         BUTTON_DOWN },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP|BUTTON_REL,           BUTTON_UP },
    { ACTION_WPS_VOLUP,         BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_ON|BUTTON_UP,            BUTTON_ON },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_ON|BUTTON_DOWN,          BUTTON_ON },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_MODE|BUTTON_REPEAT,      BUTTON_MODE },
    { ACTION_WPS_MENU,          BUTTON_MODE|BUTTON_REL,         BUTTON_MODE },
    { ACTION_WPS_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT },
    { ACTION_WPS_BROWSE,        BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },
    { ACTION_WPSAB_RESET,       BUTTON_ON|BUTTON_SELECT,        BUTTON_ON },
    { ACTION_WPS_ID3SCREEN,     BUTTON_ON|BUTTON_MODE,          BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */
   
const struct button_mapping button_context_listtree[]  = {
    { ACTION_LISTTREE_PGUP,         BUTTON_ON|BUTTON_UP,                    BUTTON_ON },
    { ACTION_LISTTREE_PGUP,         BUTTON_UP|BUTTON_REL,                   BUTTON_ON|BUTTON_UP },
    { ACTION_LISTTREE_PGUP,         BUTTON_ON|BUTTON_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_ON|BUTTON_DOWN,                  BUTTON_ON|BUTTON_UP },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_DOWN|BUTTON_REL,                 BUTTON_ON|BUTTON_DOWN },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_ON|BUTTON_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_listtree */

const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_ON|BUTTON_REL,         BUTTON_ON },   
    { ACTION_TREE_STOP,   BUTTON_OFF|BUTTON_REL,        BUTTON_OFF },
    { ACTION_TREE_STOP,   BUTTON_OFF|BUTTON_REPEAT,     BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_listtree */

const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    { ACTION_NONE,              BUTTON_ON,                              BUTTON_NONE }, 
    { ACTION_TREE_PGLEFT,       BUTTON_ON|BUTTON_LEFT,                  BUTTON_ON },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,                 BUTTON_ON|BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_ON|BUTTON_LEFT,                  BUTTON_LEFT|BUTTON_REL },
    { ACTION_TREE_PGLEFT,       BUTTON_ON|BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_ON|BUTTON_RIGHT,                 BUTTON_ON },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,                BUTTON_ON|BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_ON|BUTTON_RIGHT,                 BUTTON_RIGHT|BUTTON_REL },
    { ACTION_TREE_PGRIGHT,      BUTTON_ON|BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|1),
};

const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|1),
};

const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_RIGHT,                   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_settings */

const struct button_mapping button_context_settingsgraphical[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_settingsgraphical */

const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,                  BUTTON_NONE },
    { ACTION_YESNO_ACCEPT,          BUTTON_RC_ON,                   BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_settings_yesno */

const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMARK_DELETE,      BUTTON_REC,    BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_SELECT,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGSGRAPHICAL),
}; /* button_context_settings_bmark */

const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV,    BUTTON_UP,                      BUTTON_NONE },
    { ACTION_QS_DOWNINV,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_MODE,                    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_quickscreen */

const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_ON,                  BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_OFF,                 BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_pitchcreen */
/*****************************************************************************
 *    Remote control mappings 
 *****************************************************************************/
 
 
/*********   H100 LCD remote ******/
const struct button_mapping button_context_standard_h100lcdremote[]  = {
    { ACTION_STD_PREV,          BUTTON_RC_REW,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_RC_FF,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT,     BUTTON_NONE },
    
    { ACTION_STD_OK,            BUTTON_RC_ON|BUTTON_REL,        BUTTON_RC_ON },
    { ACTION_STD_CONTEXT,       BUTTON_RC_MENU|BUTTON_REPEAT,   BUTTON_RC_MENU },
    { ACTION_STD_CANCEL,        BUTTON_RC_STOP,                 BUTTON_NONE },
    { ACTION_STD_QUICKSCREEN,   BUTTON_RC_MODE|BUTTON_REPEAT,   BUTTON_RC_MODE },
    { ACTION_STD_MENU,          BUTTON_RC_MODE|BUTTON_REL,      BUTTON_RC_MODE },
    { ACTION_STD_OK,            BUTTON_RC_MENU|BUTTON_REL,      BUTTON_RC_MENU },
    
    LAST_ITEM_IN_LIST
}; /* button_context_standard_h100lcdremote */

const struct button_mapping button_context_wps_h100lcdremote[]  = {
    { ACTION_WPS_PLAY,              BUTTON_RC_ON|BUTTON_REL,            BUTTON_RC_ON },
    { ACTION_WPS_SKIPNEXT,          BUTTON_RC_FF|BUTTON_REL,            BUTTON_RC_FF },
    { ACTION_WPS_SEEKFWD,           BUTTON_RC_FF|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_WPS_SKIPPREV,          BUTTON_RC_REW|BUTTON_REL,           BUTTON_RC_REW },
    { ACTION_WPS_SEEKBACK,          BUTTON_RC_REW|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_STOP,              BUTTON_RC_STOP,                     BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,           BUTTON_RC_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,           BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,             BUTTON_RC_VOL_UP,                   BUTTON_NONE },
    { ACTION_WPS_VOLUP,             BUTTON_RC_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_ABSETB_NEXTDIR,           BUTTON_RC_BITRATE,                  BUTTON_NONE },
    { ACTION_WPS_ABSETA_PREVDIR,           BUTTON_RC_SOURCE,                   BUTTON_NONE },
    { ACTION_WPS_PITCHSCREEN,       BUTTON_RC_ON|BUTTON_REPEAT,         BUTTON_RC_ON },
    { ACTION_WPS_QUICKSCREEN,       BUTTON_RC_MODE|BUTTON_REPEAT,       BUTTON_RC_MODE },
    { ACTION_WPS_MENU,              BUTTON_RC_MODE|BUTTON_REL,          BUTTON_RC_MODE },
    { ACTION_WPS_CONTEXT,           BUTTON_RC_MENU|BUTTON_REPEAT,       BUTTON_RC_MENU },
    { ACTION_WPS_BROWSE,            BUTTON_RC_MENU|BUTTON_REL,          BUTTON_RC_MENU },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps_h100lcdremote */

const struct button_mapping button_context_listtree_h100lcdremote[]  = {
    { ACTION_LISTTREE_PGUP,     BUTTON_RC_SOURCE,                    BUTTON_NONE },
    { ACTION_LISTTREE_PGUP,     BUTTON_RC_SOURCE|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_RC_BITRATE,                   BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_RC_BITRATE|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_TREE_WPS,      BUTTON_RC_ON|BUTTON_REL,             BUTTON_RC_ON },
    
    LAST_ITEM_IN_LIST
}; /* button_context_listtree_h100lcdremote */
/* Not needed? _std_ actions seem to be fine */
const struct button_mapping button_context_settings_h100lcdremote[]  = { 
    LAST_ITEM_IN_LIST
};/*  button_context_settings_h100lcdremote */

const struct button_mapping button_context_settingsgraphical_h100lcdremote[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RC_FF,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_RC_REW,                     BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_RC_VOL_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_RC_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RC_VOL_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_settingsgraphical */

const struct button_mapping button_context_bmark_h100lcdremote[]  = {
    { ACTION_BMARK_DELETE,      BUTTON_RC_REC,    BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RC_MENU,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGSGRAPHICAL),
}; /* button_context_settings_bmark */



/*********   H300 LCD remote ******/
const struct button_mapping button_context_standard_h300lcdremote[]  = {
    { ACTION_STD_PREV,          BUTTON_RC_VOL_UP,                   BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_RC_VOL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_RC_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RC_ON|BUTTON_REL,            BUTTON_RC_ON },
    { ACTION_STD_CANCEL,        BUTTON_RC_REW,                      BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_RC_REW|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_CONTEXT,       BUTTON_RC_MENU|BUTTON_REPEAT,       BUTTON_RC_MENU },
    { ACTION_STD_CANCEL,        BUTTON_RC_STOP,                     BUTTON_NONE },
    { ACTION_STD_QUICKSCREEN,   BUTTON_RC_MODE|BUTTON_REPEAT,       BUTTON_RC_MODE },
    { ACTION_STD_MENU,          BUTTON_RC_MODE|BUTTON_REL,          BUTTON_RC_MODE },
    { ACTION_STD_OK,            BUTTON_RC_MENU|BUTTON_REL,          BUTTON_RC_MENU },
    { ACTION_STD_OK,            BUTTON_RC_FF,                       BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RC_FF|BUTTON_REPEAT,         BUTTON_NONE},
    
    LAST_ITEM_IN_LIST

}; /* button_context_standard */

/* the mapping of the 2 LCD remotes in the WPS screen should be the same */
const struct button_mapping *button_context_wps_h300lcdremote =
                                button_context_wps_h100lcdremote;

const struct button_mapping button_context_listtree_h300lcdremote[] = {
    { ACTION_LISTTREE_PGUP,     BUTTON_RC_SOURCE,                    BUTTON_NONE },
    { ACTION_LISTTREE_PGUP,     BUTTON_RC_SOURCE|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_RC_BITRATE,                   BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_RC_BITRATE|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_TREE_WPS,      BUTTON_RC_ON|BUTTON_REL,             BUTTON_RC_ON },
    { ACTION_TREE_STOP,     BUTTON_RC_STOP,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST

}; /* button_context_listtree_h300lcdremote */

const struct button_mapping button_context_settingsgraphical_h300lcdremote[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RC_FF,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_RC_REW,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_RC_VOL_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_RC_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RC_VOL_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_settingsgraphical */

const struct button_mapping button_context_bmark_h300lcdremote[]  = {
    { ACTION_BMARK_DELETE,      BUTTON_RC_REC,    BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RC_MENU,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGSGRAPHICAL),
}; /* button_context_settings_bmark */

const struct button_mapping *button_context_settings_h300lcdremote  = 
                                button_context_settings_h100lcdremote; 
/* FIXME: non lcd remotes need mappings.. ?? */






/* the actual used tables */
static const struct button_mapping 
        *remote_button_context_std = button_context_standard_h100lcdremote, 
        *remote_button_context_wps = button_context_wps_h100lcdremote,
        *remote_button_context_listtree = button_context_listtree_h100lcdremote,
        *remote_button_context_settings = button_context_settings_h100lcdremote,
        *remote_button_context_settingsgraphical = button_context_settingsgraphical_h100lcdremote,
        *remote_button_context_bmark = button_context_bmark_h100lcdremote;
        
static int _remote_type = 0;

static void remap_remote(void)
{
    _remote_type = remote_type();
    switch(_remote_type)
    {
        case REMOTETYPE_UNPLUGGED:
            remote_button_context_std = NULL;
            remote_button_context_wps = NULL;
            remote_button_context_listtree = NULL;
            remote_button_context_settings = NULL;
            remote_button_context_settingsgraphical = NULL;
            remote_button_context_bmark = NULL;
            break;
        case REMOTETYPE_H100_LCD:
            remote_button_context_std = button_context_standard_h100lcdremote;
            remote_button_context_wps = button_context_wps_h100lcdremote;
            remote_button_context_listtree = button_context_listtree_h100lcdremote;
            remote_button_context_settings = button_context_settings_h100lcdremote;
            remote_button_context_settingsgraphical = button_context_settingsgraphical_h100lcdremote;
            remote_button_context_bmark = button_context_bmark_h100lcdremote;
            break;
        case REMOTETYPE_H300_LCD:
            remote_button_context_std = button_context_standard_h300lcdremote;
            remote_button_context_wps = button_context_wps_h300lcdremote;
            remote_button_context_listtree = button_context_listtree_h300lcdremote;
            remote_button_context_settings = button_context_settings_h300lcdremote;
            remote_button_context_settingsgraphical = button_context_settingsgraphical_h300lcdremote;
            remote_button_context_bmark = button_context_bmark_h300lcdremote;
            break;
        case REMOTETYPE_H300_NONLCD: /* FIXME: add its tables */        
            remote_button_context_std = button_context_standard_h300lcdremote;
            remote_button_context_wps = button_context_wps_h300lcdremote;
            remote_button_context_listtree = button_context_listtree_h300lcdremote;
            remote_button_context_settings = button_context_settings_h300lcdremote;
            remote_button_context_settingsgraphical = button_context_settingsgraphical_h300lcdremote;
            remote_button_context_bmark = button_context_bmark_h300lcdremote;
            break;
    }
}








const struct button_mapping* get_context_mapping_remote(int context)
{
    if(remote_type() != _remote_type)
        remap_remote();
    context ^= CONTEXT_REMOTE;
    
    switch (context)
    {
        case CONTEXT_STD:
        case CONTEXT_MAINMENU:
        case CONTEXT_SETTINGS:
            return remote_button_context_std;
        case CONTEXT_WPS:
            return remote_button_context_wps;
                    
        case CONTEXT_TREE:
        case CONTEXT_LIST:
            return remote_button_context_listtree;
        case CONTEXT_SETTINGSGRAPHICAL:
            return remote_button_context_settingsgraphical;
        case CONTEXT_BOOKMARKSCREEN:
            return remote_button_context_bmark;
            
        case CONTEXT_YESNOSCREEN:            
            ; /* fall out of the switch */            
    } 
    return remote_button_context_std;
}

const struct button_mapping* get_context_mapping(int context)
{
    if (context&CONTEXT_REMOTE)
        return get_context_mapping_remote(context);
    
    switch (context)
    {
        case CONTEXT_STD:
        case CONTEXT_MAINMENU:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
            
        case CONTEXT_LIST:
            return button_context_listtree;
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_listtree_scroll_without_combo;
            else return button_context_listtree_scroll_with_combo;
        case CONTEXT_CUSTOM|1:
            return button_context_tree;
        case CONTEXT_SETTINGSGRAPHICAL:
            return button_context_settingsgraphical;
        
        case CONTEXT_SETTINGS:
            return button_context_settings;
            
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
            
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
    } 
    return button_context_standard;
}
