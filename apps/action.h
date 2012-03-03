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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __ACTION_H__
#define __ACTION_H__

#include "stdbool.h"
#include "button.h"
#include "viewport.h"

#define TIMEOUT_BLOCK   -1
#define TIMEOUT_NOBLOCK  0

#define CONTEXT_STOPSEARCHING 0xFFFFFFFF
#define CONTEXT_REMOTE  0x80000000 /* | this against another context to get remote buttons for that context */
#define CONTEXT_CUSTOM  0x40000000 /* | this against anything to get your context number */
#define CONTEXT_CUSTOM2 0x20000000 /* as above */
#define CONTEXT_PLUGIN  0x10000000 /* for plugins using get_custom_action */

#define LAST_ITEM_IN_LIST { CONTEXT_STOPSEARCHING, BUTTON_NONE, BUTTON_NONE }
#define LAST_ITEM_IN_LIST__NEXTLIST(a) { a, BUTTON_NONE, BUTTON_NONE }

#ifndef HAS_BUTTON_HOLD
#define ALLOW_SOFTLOCK 0x08000000 /* will be stripped.. never needed except in calls to get_action() */
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
    CONTEXT_SETTINGS_RECTRIGGER,
    
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
    CONTEXT_MORSE_INPUT,
    CONTEXT_FM,
    CONTEXT_USB_HID,
    CONTEXT_USB_HID_MODE_MULTIMEDIA,
    CONTEXT_USB_HID_MODE_PRESENTATION,
    CONTEXT_USB_HID_MODE_BROWSER,
    CONTEXT_USB_HID_MODE_MOUSE,
};


enum {
    
    ACTION_NONE = BUTTON_NONE,
    ACTION_UNKNOWN,
    ACTION_REDRAW, /* returned if keys are locked and we splash()'ed */
    ACTION_TOUCHSCREEN,
    ACTION_TOUCHSCREEN_MODE, /* toggle the touchscreen mode */
    ACTION_TOUCHSCREEN_IGNORE, /* used for the 'none' action in skins */

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
    ACTION_STD_HOTKEY,
    
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
    ACTION_WPS_VIEW_PLAYLIST,
    ACTION_WPS_LIST_BOOKMARKS,/* optional */
    ACTION_WPS_CREATE_BOOKMARK,/* optional */
    ACTION_WPS_REC,
#if 0
    ACTION_WPSAB_SINGLE, /* This needs to be #defined in 
                            the config-<target>.h to one of the ACTION_WPS_ actions
                            so it can be used */
#endif
    ACTION_WPS_ABSETA_PREVDIR, /* these should be safe to put together seen as */
    ACTION_WPS_ABSETB_NEXTDIR, /* you shouldnt want to change dir in ab-mode */
    ACTION_WPS_ABRESET,
    ACTION_WPS_HOTKEY,
    
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
    ACTION_TREE_HOTKEY,
    
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
    ACTION_SETTINGS_SET, /* Used by touchscreen targets */
    
    /* bookmark screen */
    ACTION_BMS_DELETE,
    
    /* quickscreen */
    ACTION_QS_LEFT,
    ACTION_QS_RIGHT,
    ACTION_QS_DOWN,
    ACTION_QS_TOP,
    
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
    ACTION_PS_SLOWER,
    ACTION_PS_FASTER,
    
    /* yesno screen */
    ACTION_YESNO_ACCEPT,
    
    /* keyboard screen */
    ACTION_KBD_LEFT,
    ACTION_KBD_RIGHT,
    ACTION_KBD_CURSOR_LEFT,
    ACTION_KBD_CURSOR_RIGHT,
    ACTION_KBD_SELECT,
    ACTION_KBD_PAGE_FLIP,
    ACTION_KBD_DONE,
    ACTION_KBD_ABORT,
    ACTION_KBD_BACKSPACE,
    ACTION_KBD_UP,
    ACTION_KBD_DOWN,
    ACTION_KBD_MORSE_INPUT,
    ACTION_KBD_MORSE_SELECT,
    
#ifdef HAVE_TOUCHSCREEN
    /* the following are helper actions for touchscreen targets,
     * These are for actions which are not doable or required if buttons are
     * being used, but are nice additions if the touchscreen is used */
    ACTION_TOUCH_SHUFFLE,
    ACTION_TOUCH_REPMODE,
    ACTION_TOUCH_MUTE,
    ACTION_TOUCH_SCROLLBAR,
    ACTION_TOUCH_VOLUME,
    ACTION_TOUCH_SOFTLOCK,
#endif    

    /* USB HID codes */
    ACTION_USB_HID_FIRST, /* Place holder */
    ACTION_USB_HID_NONE,
    ACTION_USB_HID_MODE_SWITCH_NEXT,
    ACTION_USB_HID_MODE_SWITCH_PREV,
    ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,
    ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,
    ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,
    ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE,
    ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,
    ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV,
    ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT,
    ACTION_USB_HID_PRESENTATION_SLIDESHOW_START,
    ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE,
    ACTION_USB_HID_PRESENTATION_SLIDE_PREV,
    ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,
    ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,
    ACTION_USB_HID_PRESENTATION_SLIDE_LAST,
    ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,
    ACTION_USB_HID_PRESENTATION_SCREEN_WHITE,
    ACTION_USB_HID_PRESENTATION_LINK_PREV,
    ACTION_USB_HID_PRESENTATION_LINK_NEXT,
    ACTION_USB_HID_PRESENTATION_MOUSE_CLICK,
    ACTION_USB_HID_PRESENTATION_MOUSE_OVER,
    ACTION_USB_HID_BROWSER_SCROLL_UP,
    ACTION_USB_HID_BROWSER_SCROLL_DOWN,
    ACTION_USB_HID_BROWSER_SCROLL_PAGE_DOWN,
    ACTION_USB_HID_BROWSER_SCROLL_PAGE_UP,
    ACTION_USB_HID_BROWSER_ZOOM_IN,
    ACTION_USB_HID_BROWSER_ZOOM_OUT,
    ACTION_USB_HID_BROWSER_ZOOM_RESET,
    ACTION_USB_HID_BROWSER_TAB_PREV,
    ACTION_USB_HID_BROWSER_TAB_NEXT,
    ACTION_USB_HID_BROWSER_TAB_CLOSE,
    ACTION_USB_HID_BROWSER_HISTORY_BACK,
    ACTION_USB_HID_BROWSER_HISTORY_FORWARD,
    ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN,
    ACTION_USB_HID_MOUSE_UP,
    ACTION_USB_HID_MOUSE_UP_REP,
    ACTION_USB_HID_MOUSE_DOWN,
    ACTION_USB_HID_MOUSE_DOWN_REP,
    ACTION_USB_HID_MOUSE_LEFT,
    ACTION_USB_HID_MOUSE_LEFT_REP,
    ACTION_USB_HID_MOUSE_RIGHT,
    ACTION_USB_HID_MOUSE_RIGHT_REP,
    ACTION_USB_HID_MOUSE_LDRAG_UP,
    ACTION_USB_HID_MOUSE_LDRAG_UP_REP,
    ACTION_USB_HID_MOUSE_LDRAG_DOWN,
    ACTION_USB_HID_MOUSE_LDRAG_DOWN_REP,
    ACTION_USB_HID_MOUSE_LDRAG_LEFT,
    ACTION_USB_HID_MOUSE_LDRAG_LEFT_REP,
    ACTION_USB_HID_MOUSE_LDRAG_RIGHT,
    ACTION_USB_HID_MOUSE_LDRAG_RIGHT_REP,
    ACTION_USB_HID_MOUSE_RDRAG_UP,
    ACTION_USB_HID_MOUSE_RDRAG_UP_REP,
    ACTION_USB_HID_MOUSE_RDRAG_DOWN,
    ACTION_USB_HID_MOUSE_RDRAG_DOWN_REP,
    ACTION_USB_HID_MOUSE_RDRAG_LEFT,
    ACTION_USB_HID_MOUSE_RDRAG_LEFT_REP,
    ACTION_USB_HID_MOUSE_RDRAG_RIGHT,
    ACTION_USB_HID_MOUSE_RDRAG_RIGHT_REP,
    ACTION_USB_HID_MOUSE_BUTTON_LEFT,
    ACTION_USB_HID_MOUSE_BUTTON_LEFT_REL,
    ACTION_USB_HID_MOUSE_BUTTON_RIGHT,
    ACTION_USB_HID_MOUSE_BUTTON_RIGHT_REL,
    ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,
    ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN,
    ACTION_USB_HID_LAST, /* Place holder */

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

#ifdef HAVE_TOUCHSCREEN
/* return BUTTON_NONE               on error
 *        BUTTON_REPEAT             if repeated press
 *        BUTTON_REPEAT|BUTTON_REL  if release after repeated press
 *        BUTTON_REL                if it's a short press = release after press
 *        BUTTON_TOUCHSCREEN        if press
 */
int action_get_touchscreen_press(short *x, short *y);

/*
 * wrapper action_get_touchscreen_press()
 * to filter the touchscreen coordinates through a viewport
 *
 * returns the action and x1, y1 relative to the viewport if
 * the press was within the viewport,
 * ACTION_UNKNOWN (and x1, y1 untouched) if the press was outside
 * BUTTON_NONE else
 * 
 **/
int action_get_touchscreen_press_in_vp(short *x1, short *y1, struct viewport *vp);
#endif

/* Don't let get_action*() return any ACTION_* values until the current buttons
 * have been released. SYS_* and BUTTON_NONE will go through.
 * Any actions relying on _RELEASE won't get seen.
 */
void action_wait_for_release(void);

#endif /* __ACTION_H__ */
