/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
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

#ifndef __MENU_H__
#define __MENU_H__

#include <stdbool.h>
#include "icon.h"
#include "icons.h"
#include "root_menu.h" /* needed for MENU_* return codes */


enum menu_item_type {
    MT_MENU = 0,
    MT_SETTING,
    MT_SETTING_W_TEXT, /* same as setting, but uses different
                          text for the setting title,
                          ID2P() or "literal" for the str param */
    MT_FUNCTION_CALL, /* call a function from the menus */
    MT_RETURN_ID, /* returns the position of the selected item (starting at 0)*/
    MT_RETURN_VALUE, /* returns a value associated with an item */
};
#define MENU_TYPE_MASK 0xF /* MT_* type */

typedef int (*menu_function)(void);
struct menu_func {
    union {
        int (*function_w_param)(void* param); /* intptr_t instead of void*
                                                    for 64bit systems */
        int (*function)(void);
    };
    void *param; /* passed to function_w_param */
};

/* these next two are mutually exclusive */
#define MENU_HAS_DESC   0x10
#define MENU_DYNAMIC_DESC 0x20 /* the name of this menu item is set by the  \
                                  list_get_name callback */

#define MENU_EXITAFTERTHISMENU 0x40 /* do_menu() will exiting out of any    \
                                       menu item with this flag set */

/* Flags for MT_FUNCTION_CALL */
#define MENU_FUNC_USEPARAM 0x80
#define MENU_FUNC_CHECK_RETVAL 0x100

#define MENU_COUNT_MASK 0xFFF
#define MENU_COUNT_SHIFT 12
#define MENU_ITEM_COUNT(c) ((c&MENU_COUNT_MASK)<<MENU_COUNT_SHIFT)
#define MENU_GET_COUNT(flags) ((flags>>MENU_COUNT_SHIFT)&MENU_COUNT_MASK)

struct menu_item_ex {
    unsigned int flags; /* above defines */
    union {
        const struct menu_item_ex **submenus; /* used with MT_MENU */
        void *variable; /* used with MT_SETTING,
                           must be in the settings_list.c list */
        const struct menu_func *function; /* MT_FUNCTION_* */
        const char **strings; /* used with MT_RETURN_ID */
        int value; /* MT_RETURN_VALUE */
    };
    union {
        /* For settings */
        int (*menu_callback)(int action, const struct menu_item_ex *this_item);
        /* For everything else, except if the text is dynamic */
        const struct menu_callback_with_desc {
            int (*menu_callback)(int action, 
                                 const struct menu_item_ex *this_item);
            unsigned char *desc; /* string or ID */
            int icon_id; /* from icons_6x8 in icons.h */
        } *callback_and_desc;
        /* For when the item text is dynamic */
        const struct menu_get_name_and_icon {
            int (*menu_callback)(int action, 
                                 const struct menu_item_ex *this_item);
            char *(*list_get_name)(int selected_item, void * data, char *buffer);
            int (*list_speak_item)(int selected_item, void * data);
            void *list_get_name_data;
            int icon_id;
        } *menu_get_name_and_icon;
    };
};

typedef int (*menu_callback_type)(int action,
                                  const struct menu_item_ex *this_item);
bool do_setting_from_menu(const struct menu_item_ex *temp,
                          struct viewport parent[NB_SCREENS]);

/* 
   int do_menu(const struct menu_item_ex *menu, int *start_selected)
    
   Return value - usually one of the GO_TO_* values from root_menu.h,
   however, some of the following defines can cause this to 
   return a different value.
   
   *menu - The menu to run, can be a pointer to a MAKE_MENU() variable,
            MENUITEM_STRINGLIST() or MENUITEM_RETURNVALUE() variable.
            
   *start_selected - the item to select when the menu is first run.
                     When do_menu() returns, this will be set to the 
                     index of the selected item at the time of the exit.
                     This is always set, even if the menu was cancelled.
                     If NULL it is ignored and the firs item starts selected
*/
int do_menu(const struct menu_item_ex *menu, int *start_selected,
            struct viewport parent[NB_SCREENS], bool hide_bars);

/* In all the following macros the argument names are as follows:
    - name: The name for the variable (so it can be used in a MAKE_MENU()
    - str:  the string to display for this menu item. use ID2P() for LANG_* id's
    - callback: The callback function to call for this menu item.
*/

/*  Use this to put a setting into a menu.
    The setting must appear in settings_list.c.
    If the setting is not configured properly, the menu will display "Not Done yet!"
    When the user selects this item the setting select screen will load,
    when that screen exits the user wll be back in the menu */
#define MENUITEM_SETTING(name,var,callback)                  \
    static const struct menu_item_ex name =                  \
        {MT_SETTING, {.variable = (void*)var},{callback}};

/*  Use this for settings which have a differnt title in their
    setting screen than in the menu (e.g scroll options */
#define MENUITEM_SETTING_W_TEXT(name, var, str, callback )              \
    static const struct menu_callback_with_desc name##__ =              \
                {callback,str, Icon_NOICON};                            \
    static const struct menu_item_ex name =                             \
        {MT_SETTING_W_TEXT|MENU_HAS_DESC, {.variable = (void*)var },    \
            {.callback_and_desc = & name##__}};

/*  Use this To create a list of Strings (or ID2P()'s )
    When the user enters this list and selects one, the menu will exit
    and do_menu() will return value the index of the chosen item.
    if the user cancels, GO_TO_PREVIOUS will be returned */
#define MENUITEM_STRINGLIST(name, str, callback, ... )                  \
    static const char *name##_[] = {__VA_ARGS__};                       \
    static const struct menu_callback_with_desc name##__ =              \
            {callback,str, Icon_NOICON};                                \
    static const struct menu_item_ex name =                             \
        {MT_RETURN_ID|MENU_HAS_DESC|                                    \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { .strings = name##_},{.callback_and_desc = & name##__}};

            
/* causes do_menu() to return a value associated with the item */
#define MENUITEM_RETURNVALUE(name, str, val, cb, icon)                      \
     static const struct menu_callback_with_desc name##_ = {cb,str,icon};   \
     static const struct menu_item_ex name   =                              \
         { MT_RETURN_VALUE|MENU_HAS_DESC, { .value = val},                  \
         {.callback_and_desc = & name##_}};
         
/* same as above, except the item name is dynamic */
#define MENUITEM_RETURNVALUE_DYNTEXT(name, val, cb, text_callback,          \
                                     voice_callback, text_cb_data, icon)    \
     static const struct menu_get_name_and_icon name##_                     \
         = {cb,text_callback,voice_callback,text_cb_data,icon};             \
     static const struct menu_item_ex name   =                              \
        { MT_RETURN_VALUE|MENU_DYNAMIC_DESC, { .value = val},               \
        {.menu_get_name_and_icon = & name##_}};
        
/*  Use this to put a function call into the menu.
    When the user selects this item the function will be run,
    if MENU_FUNC_CHECK_RETVAL is set, the return value
    will be checked, returning 1 will exit do_menu();
    if MENU_FUNC_USEPARAM is set, param will be passed to the function */
#define MENUITEM_FUNCTION(name, flags, str, func, param,                       \
                              callback, icon)                                  \
    static const struct menu_callback_with_desc name##_ = {callback,str,icon}; \
    static const struct menu_func name##__ = {{(void*)func}, param};           \
    /* should be const, but recording_settings wont let us do that */          \
    const struct menu_item_ex name   =                                         \
        { MT_FUNCTION_CALL|MENU_HAS_DESC|flags,                                \
         { .function = & name##__}, {.callback_and_desc = & name##_}};
            
/* As above, except the text is dynamic */
#define MENUITEM_FUNCTION_DYNTEXT(name, flags, func, param,                 \
                                  text_callback, voice_callback,            \
                                  text_cb_data, callback, icon)             \
    static const struct menu_get_name_and_icon name##_                      \
        = {callback,text_callback,voice_callback,text_cb_data,icon};        \
    static const struct menu_func name##__ = {{(void*)func}, param};           \
    static const struct menu_item_ex name   =                                  \
        { MT_FUNCTION_CALL|MENU_DYNAMIC_DESC|flags,                            \
         { .function = & name##__}, {.menu_get_name_and_icon = & name##_}};

/*  Use this to actually create a menu. the ... argument is a list of pointers 
    to any of the above macro'd variables. 
    (It can also have other menus in the list.) */
#define MAKE_MENU( name, str, callback, icon, ... )                            \
    static const struct menu_item_ex *name##_[]  = {__VA_ARGS__};              \
    static const struct menu_callback_with_desc name##__ = {callback,str,icon};\
    const struct menu_item_ex name =                                           \
        {MT_MENU|MENU_HAS_DESC|                                                \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),                   \
            { (void*)name##_},{.callback_and_desc = & name##__}};
            

#endif /* End __MENU_H__ */
