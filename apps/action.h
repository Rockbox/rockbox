/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2004 Brent Coutts
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __ACTION_H__
#define __ACTION_H__

#include "stdbool.h"
#include "button.h"

#define LAST_ITEM_IN_LIST { ACTION_NONE, BUTTON_NONE, BUTTON_NONE }
#define LAST_ITEM_IN_LIST__NEXTLIST(a) { a, BUTTON_NONE, BUTTON_NONE }

#define TIMEOUT_BLOCK   -1
#define TIMEOUT_NOBLOCK  0
#define CONTEXT_REMOTE 0x80000000 /* | this against another context to get remote buttons for that context */
#define CONTEXT_CUSTOM 0x40000000 /* | this against anything to get your context number */

enum {
    CONTEXT_STD = 0,
    /* These CONTEXT_ values were here before me,
    there values may have significance, so dont touch! */
    CONTEXT_WPS = 1,
    CONTEXT_TREE = 2, 
    CONTEXT_RECORD = 3,
    CONTEXT_MAINMENU = 4,
    CONTEXT_ID3DB = 5,
    /* Add new contexts here, no need to explicitly define a value for them */    
    CONTEXT_LIST,
    CONTEXT_SETTINGS, /* options style settings, like from menus */
    CONTEXT_SETTINGSGRAPHICAL, /* screens like eq config and colour chooser */
    
    CONTEXT_YESNOSCREEN, /*NOTE: make sure your target has this and ACTION_YESNO_ACCEPT */
    CONTEXT_BOOKMARKSCREEN, /*NOTE: requires the action_setting_* mappings also */
    CONTEXT_QUICKSCREEN, /* uses ACTION_QS_ defines below */
};


enum {
    
    ACTION_NONE = BUTTON_NONE,
    ACTION_UNKNOWN,
    
    /* standard actions, use these first */
    ACTION_STD_PREV, 
    ACTION_STD_PREVREPEAT,
    ACTION_STD_NEXT,
    ACTION_STD_NEXTREPEAT,
    
    ACTION_STD_OK,
    ACTION_STD_CANCEL,
    ACTION_STD_CONTEXT,
    ACTION_STD_MENU,
    ACTION_STD_QUICKSCREEN,
    ACTION_STD_KEYLOCK, /* software keylock in wps screen, very optional
                           use with action_setsoftwarekeylock */
    
    
    /* code context actions */
    
    /* WPS codes */
    ACTION_WPS_BROWSE,
    ACTION_WPS_PLAY,
    ACTION_WPS_SEEKBACK,
    ACTION_WPS_SEEKFWD,
    ACTION_WPS_STOPSEEK,
    ACTION_WPS_SKIPNEXT,
    ACTION_WPS_SKIPPREV,
    ACTION_WPS_STOP,
    ACTION_WPS_VOLDOWN,
    ACTION_WPS_VOLUP,
    ACTION_WPS_NEXTDIR,/* optional */
    ACTION_WPS_PREVDIR,/* optional */
    ACTION_WPS_PITCHSCREEN,/* optional */
    ACTION_WPS_ID3SCREEN,/* optional */
    ACTION_WPS_CONTEXT,
    ACTION_WPS_QUICKSCREEN,/* optional */
    ACTION_WPS_MENU, /*this should be the same as ACTION_STD_MENU */
    /* following code are for AB mode in wps, 
       only needed if defined(AB_REPEAT_ENABLE) */
    ACTION_WPSAB_SINGLE, /* No targets use this, but leave n just-in-case! */
    ACTION_WPSAB_SETA,   /* either #define WPS_AB_SHARE_DIR_BUTTONS        */
    ACTION_WPSAB_SETB,   /* OR implement ACTION_WPSAB_SET[AB]              */
    ACTION_WPSAB_RESET,
    
    /* list and tree page up/down */    
    ACTION_LISTTREE_PGUP,/* optional */
    ACTION_LISTTREE_PGDOWN,/* optional */
    ACTION_LISTTREE_RC_PGUP,/* optional */
    ACTION_LISTTREE_RC_PGDOWN,/* optional */
    
    /* tree */ 
    ACTION_TREE_PGLEFT,/* optional */
    ACTION_TREE_PGRIGHT,/* optional */
    ACTION_TREE_STOP,
    ACTION_TREE_WPS,
    
    /* recording screen */
    
    /* main menu */
    
    /* id3db */
    
    /* list */
    
    /* settings */
    ACTION_SETTINGS_INC,
    ACTION_SETTINGS_INCREPEAT,
    ACTION_SETTINGS_DEC,
    ACTION_SETTINGS_DECREPEAT,
    
    /* yesno screen */
    ACTION_YESNO_ACCEPT,
    
    /* bookmark screen */
    ACTION_BMARK_DELETE,
    
    /* quickscreen */
    ACTION_QS_LEFT,
    ACTION_QS_RIGHT,
    ACTION_QS_DOWN,
    ACTION_QS_DOWNINV, /* why is this not called up?? :p */
    
};

struct button_mapping {
    int action_code;
    int button_code;
    int pre_button_code;
};
/* use if you want to supply your own button mappings, PLUGINS ONLY */
/* get_context_map is a function which returns a button_mapping* depedning on the given context */
/* custom button_mappings may "chain" to inbuilt CONTEXT's */
int get_custom_action(int context,int timeout,
                      const struct button_mapping* (*get_context_map)(int));
/* use if one of the standard CONTEXT_ mappings will work (ALL the core should be using this! */
int get_action(int context, int timeout);
/* call this whenever you leave your button loop */
void action_signalscreenchange(void);

/* call this if you need to check for ACTION_STD_CANCEL only (i.e user abort! */
bool action_userabort(int timeout);

/* on targets without hardware keylock, use this to to emulate keylock.
   unlock_action is the action which will disaable the keylock
   allow_remote should be true if you want the remote buttons to still be usable while locked */
void action_setsoftwarekeylock(int unlock_action, bool allow_remote);

/* no other code should need this apart from action.c */
const struct button_mapping* get_context_mapping(int context);

#endif
