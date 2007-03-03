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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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


struct menu_item {
    unsigned char *desc; /* string or ID */
    bool (*function) (void); /* return true if USB was connected */
};

int menu_init(const struct menu_item* mitems, int count,
                int (*callback)(int, int),
                const char *button1, const char *button2, const char *button3);
void menu_exit(int menu);

void put_cursorxy(int x, int y, bool on);

 /* Returns below define, or number of selected menu item*/
int menu_show(int m);
#define MENU_ATTACHED_USB -1
#define MENU_SELECTED_EXIT -2
#define MENU_EXIT_ALL -3
#define MENU_RETURN_TO_WPS -4

bool menu_run(int menu);
int menu_cursor(int menu);
char* menu_description(int menu, int position);
void menu_delete(int menu, int position);
int menu_count(int menu);
bool menu_moveup(int menu);
bool menu_movedown(int menu);
void menu_draw(int menu);
void menu_insert(int menu, int position, char *desc, bool (*function) (void));
void menu_set_cursor(int menu, int position);
void menu_talk_selected(int m);


enum menu_item_type {
    MT_MENU = 0,
    MT_SETTING,
    MT_FUNCTION_CALL, /* used when the standard code wont work */
    MT_FUNCTION_WITH_PARAM,
    MT_RETURN_ID, /* returns the position of the selected item (starting at 0)*/
    MT_RETURN_VALUE, /* returns a value associated with an item */
};

typedef int (*menu_function)(void);
struct menu_func_with_param {
    int (*function)(void* param);
    void *param;
};

#define MENU_TYPE_MASK 0xF /* MT_* type */
/* these next two are mutually exclusive */
#define MENU_HAS_DESC   0x10
#define MENU_DYNAMIC_DESC 0x20
/* unless we need more flags*/
#define MENU_COUNT_MASK (~(MENU_TYPE_MASK|MENU_HAS_DESC|MENU_DYNAMIC_DESC))
#define MENU_COUNT_SHIFT 6

struct menu_item_ex {
    int flags; /* above defines */
    union {
        const struct menu_item_ex **submenus; /* used with MT_MENU */
        void *variable; /* used with MT_SETTING,
                           must be in the settings_list.c list */
        int (*function)(void); /* used with MT_FUNCTION_CALL */
        const struct menu_func_with_param 
                                *func_with_param; /* MT_FUNCTION_WITH_PARAM */
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
            void *list_get_name_data;
            int icon_id;
        } *menu_get_name_and_icon;
    };
};

typedef int (*menu_callback_type)(int action,
                                  const struct menu_item_ex *this_item);
int do_menu(const struct menu_item_ex *menu, int *start_selected);
bool do_setting_from_menu(const struct menu_item_ex *temp);

#define MENU_ITEM_COUNT(c) (c<<MENU_COUNT_SHIFT)
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

/*  Use this To create a list of NON-XLATABLE (for the time being) Strings
    When the user enters this list and selects one, the menu will exits
    and its return value will be the index of the chosen item */
#define MENUITEM_STRINGLIST(name, str, callback, ... )                  \
    static const char *name##_[] = {__VA_ARGS__};                       \
    static const struct menu_callback_with_desc name##__ = {callback,str, Icon_NOICON};\
    static const struct menu_item_ex name =                             \
        {MT_RETURN_ID|MENU_HAS_DESC|                                    \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { .submenus = name##_},{.callback_and_desc = & name##__}};

            
/* returns a value associated with the item */
#define MENUITEM_RETURNVALUE(name, str, val, cb, icon)                      \
     static const struct menu_callback_with_desc name##_ = {cb,str,icon};     \
     static const struct menu_item_ex name   =                                  \
         { MT_RETURN_VALUE|MENU_HAS_DESC, { .value = val},             \
         {.callback_and_desc = & name##_}};
         
/* same as above, except the item name is dynamic */
#define MENUITEM_RETURNVALUE_DYNTEXT(name, val, cb, text_callback, text_cb_data, icon)                      \
     static const struct menu_get_name_and_icon name##_                       \
                                = {cb,text_callback,text_cb_data,icon}; \
     static const struct menu_item_ex name   =                              \
        { MT_RETURN_VALUE|MENU_DYNAMIC_DESC, { .value = val},               \
        {.menu_get_name_and_icon = & name##_}};
        
/*  Use this to put a function call into the menu.
    When the user selects this item the function will be run,
    when it exits the user will be back in the menu. return value is ignored */
#define MENUITEM_FUNCTION(name, str, func, callback, icon)                     \
    static const struct menu_callback_with_desc name##_ = {callback,str,icon}; \
    static const struct menu_item_ex name   =                                  \
        { MT_FUNCTION_CALL|MENU_HAS_DESC, { .function = func},          \
        {.callback_and_desc = & name##_}};

/* This one should be static'ed also, 
   but cannot be done untill recording menu is done */
/* Same as above, except the function will be called with a (void*)param. */
#define MENUITEM_FUNCTION_WPARAM(name, str, func, param, callback, icon)    \
    static const struct menu_callback_with_desc name##_ = {callback,str,icon}; \
    static const struct menu_func_with_param name##__ = {func, param};      \
    const struct menu_item_ex name   =                               \
        { MT_FUNCTION_WITH_PARAM|MENU_HAS_DESC,                             \
            { .func_with_param = &name##__},                                \
            {.callback_and_desc = & name##_}};
            
/* As above, except the text is dynamic */
#define MENUITEM_FUNCTION_WPARAM_DYNTEXT(name, func, param, callback,  \
                                         text_callback, text_cb_data, icon) \
    static const struct menu_get_name_and_icon name##_                         \
                                = {callback,text_callback,text_cb_data,icon};\
    static const struct menu_func_with_param name##__ = {func, param};      \
    static const struct menu_item_ex name   =                               \
        { MT_FUNCTION_WITH_PARAM|MENU_DYNAMIC_DESC,                             \
            { .func_with_param = &name##__},                                \
            {.menu_get_name_and_icon = & name##_}};

/*  Use this to actually create a menu. the ... argument is a list of pointers 
    to any of the above macro'd variables. (It can also have other menus in the list. */
#define MAKE_MENU( name, str, callback, icon, ... )                           \
    static const struct menu_item_ex *name##_[]  = {__VA_ARGS__};       \
    static const struct menu_callback_with_desc name##__ = {callback,str,icon};\
    const struct menu_item_ex name =                                    \
        {MT_MENU|MENU_HAS_DESC|                                         \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { (void*)name##_},{.callback_and_desc = & name##__}};
            


#endif /* End __MENU_H__ */
