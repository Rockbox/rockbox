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
};

typedef int (*menu_function)(void);
struct menu_func_with_param {
    int (*function)(void* param);
    void *param;
};

#define MENU_TYPE_MASK 0xF /* MT_* type */
#define MENU_HAS_DESC   0x10
#define MENU_COUNT_MASK (~(MENU_TYPE_MASK|MENU_HAS_DESC)) /* unless we need more flags*/
#define MENU_COUNT_SHIFT 5

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
    };
    union {
        int (*menu_callback)(int action, const struct menu_item_ex *this_item);
        const struct menu_callback_with_desc {
            int (*menu_callback)(int action, 
                                 const struct menu_item_ex *this_item);
            unsigned char *desc; /* string or ID */
        } *callback_and_desc;
    };
};

typedef int (*menu_callback_type)(int action,
                                  const struct menu_item_ex *this_item);
int do_menu(const struct menu_item_ex *menu);

#define MENU_ITEM_COUNT(c) (c<<MENU_COUNT_SHIFT)

#define MENUITEM_SETTING(name,var,callback)                  \
    static const struct menu_item_ex name =                  \
        {MT_SETTING, {.variable = (void*)var},{callback}};

#define MAKE_MENU( name, str, cb, ... )                                 \
    static const struct menu_item_ex *name##_[]  = {__VA_ARGS__};       \
    static const struct menu_callback_with_desc name##__ = {cb,str};    \
    const struct menu_item_ex name =                                    \
        {MT_MENU|MENU_HAS_DESC|                                         \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { (void*)name##_},{.callback_and_desc = & name##__}};

#define MENUITEM_STRINGLIST(name, str, cb, ... )                        \
    static const char *name##_[] = {__VA_ARGS__};                       \
    static const struct menu_callback_with_desc name##__ = {cb,str};    \
    static const struct menu_item_ex name =                             \
        {MT_RETURN_ID|MENU_HAS_DESC|                                    \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { .submenus = name##_},{.callback_and_desc = & name##__}};
/* This one should be static'ed also, 
   but cannot be done untill sound and playlist menus are done */
#define MENUITEM_FUNCTION(name, str, func, cb)                          \
    static const struct menu_callback_with_desc name##_ = {cb,str};     \
    const struct menu_item_ex name   =                                  \
        { MT_FUNCTION_CALL|MENU_HAS_DESC, { .function = func},          \
        {.callback_and_desc = & name##_}};

#define MENUITEM_FUNCTION_WPARAM(name, str, func, param, callback)          \
    static const struct menu_callback_with_desc name##_ = {callback,str};   \
    static const struct menu_func_with_param name##__ = {func, param};      \
    static const struct menu_item_ex name   =                               \
        { MT_FUNCTION_WITH_PARAM|MENU_HAS_DESC,                             \
            { .func_with_param = &name##__},                                \
            {.callback_and_desc = & name##_}};


#endif /* End __MENU_H__ */
