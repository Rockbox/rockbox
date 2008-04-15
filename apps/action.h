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

#define TIMEOUT_BLOCK   -1
#define TIMEOUT_NOBLOCK  0

#define CONTEXT_STOPSEARCHING 0xFFFFFFFF
#define CONTEXT_REMOTE  0x80000000 /* | this against another context to get remote buttons for that context */
#define CONTEXT_CUSTOM  0x40000000 /* | this against anything to get your context number */
#define CONTEXT_CUSTOM2 0x20000000 /* as above */

#define LAST_ITEM_IN_LIST { CONTEXT_STOPSEARCHING, BUTTON_NONE, BUTTON_NONE }
#define LAST_ITEM_IN_LIST__NEXTLIST(a) { a, BUTTON_NONE, BUTTON_NONE }

#ifndef HAS_BUTTON_HOLD
#define ALLOW_SOFTLOCK 0x10000000 /* will be stripped.. never needed except in calls to get_action() */
#else
#define ALLOW_SOFTLOCK 0
#endif

enum {
    CONTEXT_STD = 0,
    /* These CONTEXT_ values were here before me,
    there values may have significance, so dont touch! */
    CONTEXT_WPS = 1,
    CONTEXT_TREE = 2, 
    CONTEXT_RECORD = 3,
    CONTEXT_MAINMENU = 4, /* uses CONTEXT_TREE and ACTION_TREE_* */
    CONTEXT_ID3DB = 5,
    /* Add new contexts here, no need to explicitly define a value for them */    
    CONTEXT_LIST,
    CONTEXT_SETTINGS, /* regular setting screens (and debug screens) */
    /* bellow are setting screens which may need to redefine the standard 
       setting screen keys, targets should return the CONTEXT_SETTINGS
       keymap unless they are not adequate for the screen
    NOTE: uses ACTION_STD_[NEXT|PREV] so make sure they are there also   
          and (possibly) ACTION_SETTINGS_[INC|DEC] */
    CONTEXT_SETTINGS_EQ,            
    CONTEXT_SETTINGS_COLOURCHOOSER, 
    CONTEXT_SETTINGS_TIME,          
    
    /* The following contexts should use ACTION_STD_[NEXT|PREV]
        and (possibly) ACTION_SETTINGS_[INC|DEC] 
       Also add any extra actions they need                        */
    CONTEXT_BOOKMARKSCREEN, /* uses ACTION_BMS_ defines */
    CONTEXT_ALARMSCREEN, /* uses ACTION_AS_ defines */   
    CONTEXT_QUICKSCREEN, /* uses ACTION_QS_ defines below */
    CONTEXT_PITCHSCREEN, /* uses ACTION_PS_ defines below */
    
    CONTEXT_YESNOSCREEN, /*NOTE: make sure your target has this and ACTION_YESNO_ACCEPT */
    CONTEXT_RECSCREEN,
    CONTEXT_KEYBOARD,
    CONTEXT_FM,
};


enum {
    
    ACTION_NONE = BUTTON_NONE,
    ACTION_UNKNOWN,
    ACTION_REDRAW, /* returned if keys are locked and we splash()'ed */
    ACTION_TOUCHPAD,
    ACTION_TOUCHPAD_MODE, /* toggle the touchpad mode */
    
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
    ACTION_STD_KEYLOCK,
    ACTION_STD_REC,
    
    ACTION_F3, /* just so everything works again, possibly change me */
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
    ACTION_WPS_PITCHSCREEN,/* optional */
    ACTION_WPS_ID3SCREEN,/* optional */
    ACTION_WPS_CONTEXT,
    ACTION_WPS_QUICKSCREEN,/* optional */
    ACTION_WPS_MENU, /*this should be the same as ACTION_STD_MENU */
    ACTION_WPS_REC,
#if 0
    ACTION_WPSAB_SINGLE, /* This needs to be #defined in 
                            the config-<target>.h to one of the ACTION_WPS_ actions
                            so it can be used */
#endif
    ACTION_WPS_ABSETA_PREVDIR, /* these should be safe to put together seen as */
    ACTION_WPS_ABSETB_NEXTDIR, /* you shouldnt want to change dir in ab-mode */
    ACTION_WPS_ABRESET,
    
    /* list and tree page up/down */    
    ACTION_LISTTREE_PGUP,/* optional */
    ACTION_LISTTREE_PGDOWN,/* optional */
#ifdef HAVE_VOLUME_IN_LIST
    ACTION_LIST_VOLUP,
    ACTION_LIST_VOLDOWN,
#endif
    
    /* tree */ 
    ACTION_TREE_ROOT_INIT,
    ACTION_TREE_PGLEFT,/* optional */
    ACTION_TREE_PGRIGHT,/* optional */
    ACTION_TREE_STOP,
    ACTION_TREE_WPS,
    
    /* radio */
    ACTION_FM_MENU,
    ACTION_FM_PRESET,
    ACTION_FM_RECORD,
    ACTION_FM_FREEZE,
    ACTION_FM_STOP,
    ACTION_FM_MODE,
    ACTION_FM_EXIT,
    ACTION_FM_PLAY,
    ACTION_FM_RECORD_DBLPRE,
    ACTION_FM_NEXT_PRESET,
    ACTION_FM_PREV_PRESET,

    /* recording screen */
    ACTION_REC_LCD,
    ACTION_REC_PAUSE,
    ACTION_REC_NEWFILE,
    ACTION_REC_F2,
    ACTION_REC_F3,
    
    /* main menu */
    /* These are not strictly actions, but must be here
       so they dont conflict with real actions in the menu code */
    ACTION_REQUEST_MENUITEM,
    ACTION_EXIT_MENUITEM,
    ACTION_EXIT_AFTER_THIS_MENUITEM, /* if a menu returns this the menu will exit
                                        once the subitem returns */
    ACTION_ENTER_MENUITEM,
    
    /* id3db */
    
    /* list */
    
    /* settings */
    ACTION_SETTINGS_INC,
    ACTION_SETTINGS_INCREPEAT,
    ACTION_SETTINGS_INCBIGSTEP,
    ACTION_SETTINGS_DEC,
    ACTION_SETTINGS_DECREPEAT,
    ACTION_SETTINGS_DECBIGSTEP,
    ACTION_SETTINGS_RESET,
    
    /* bookmark screen */
    ACTION_BMS_DELETE,
    
    /* alarm menu screen */    
        
    /* quickscreen */
    ACTION_QS_LEFT,
    ACTION_QS_RIGHT,
    ACTION_QS_DOWN,
    ACTION_QS_DOWNINV, /* why is this not called up?? :p */
    
    /* pitchscreen */
    /* obviously ignore if you dont have thise screen */
    ACTION_PS_INC_SMALL,
    ACTION_PS_INC_BIG,
    ACTION_PS_DEC_SMALL,
    ACTION_PS_DEC_BIG,
    ACTION_PS_NUDGE_LEFT,
    ACTION_PS_NUDGE_RIGHT,
    ACTION_PS_NUDGE_LEFTOFF,
    ACTION_PS_NUDGE_RIGHTOFF,
    ACTION_PS_TOGGLE_MODE,
    ACTION_PS_RESET,
    ACTION_PS_EXIT, /* _STD_* isnt going to work here */
    
    /* yesno screen */
    ACTION_YESNO_ACCEPT,
    
    /* keyboard screen */
    ACTION_KBD_LEFT,
    ACTION_KBD_RIGHT,
    ACTION_KBD_CURSOR_LEFT,
    ACTION_KBD_CURSOR_RIGHT,
    ACTION_KBD_SELECT,
    ACTION_KBD_SELECT_REM,
    ACTION_KBD_PAGE_FLIP,
    ACTION_KBD_DONE,
    ACTION_KBD_ABORT,
    ACTION_KBD_BACKSPACE,
    ACTION_KBD_UP,
    ACTION_KBD_DOWN,
    ACTION_KBD_MORSE_INPUT,
    ACTION_KBD_MORSE_SELECT,


    LAST_ACTION_PLACEHOLDER, /* custom actions should be this + something */
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

/* call this if you need to check for ACTION_STD_CANCEL only (i.e user abort! */
bool action_userabort(int timeout);

/* no other code should need this apart from action.c */
const struct button_mapping* get_context_mapping(int context);
#ifndef HAS_BUTTON_HOLD
bool is_keys_locked(void);
#endif

/* returns the status code variable from action.c for the button just pressed 
   If button != NULL it will be set to the actual button code */
#define ACTION_REMOTE   0x1 /* remote was pressed */
#define ACTION_REPEAT   0x2 /* action was repeated (NOT button) */
int get_action_statuscode(int *button);

/* returns the data value associated with the last action that is not
   BUTTON_NONE or flagged with SYS_EVENT */
intptr_t get_action_data(void);

#ifdef HAVE_TOUCHPAD
/* return BUTTON_NONE on error
          BUTTON_REPEAT if repeated press
          BUTTON_REL    if its a short press
          BUTTON_TOUCHPAD   otherwise
*/
int action_get_touchpad_press(short *x, short *y);
#endif

#endif /* __ACTION_H__ */
