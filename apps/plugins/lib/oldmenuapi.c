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
/*
2005 Kevin Ferrare :
 - Multi screen support
 - Rewrote/removed a lot of code now useless with the new gui API
*/
#include <stdbool.h>
#include <stdlib.h>

#include "plugin.h"
#include "oldmenuapi.h"

struct menu {
    struct menu_item* items;
    int (*callback)(int, int);
    struct gui_synclist synclist;
};

#define MAX_MENUS 6

static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };

static char * menu_get_itemname(int selected_item, void * data,
                                char *buffer, size_t buffer_len)
{
    (void)buffer; (void)buffer_len;
    struct menu *local_menus=(struct menu *)data;
    return(local_menus->items[selected_item].desc);
}

static int menu_find_free(void)
{
    int i;
    /* Tries to find an unused slot to put the new menu */
    for ( i=0; i<MAX_MENUS; i++ ) {
        if ( !inuse[i] ) {
            inuse[i] = true;
            break;
        }
    }
    if ( i == MAX_MENUS ) {
        DEBUGF("Out of menus!\n");
        return -1;
    }
    return(i);
}

int menu_init(const struct menu_item* mitems,
              int count, int (*callback)(int, int),
              const char *button1, const char *button2, const char *button3)
{
    int menu=menu_find_free();
    if(menu==-1)/* Out of menus */
        return -1;
    menus[menu].items = (struct menu_item*)mitems; /* de-const */
    rb->gui_synclist_init(&(menus[menu].synclist),
                          &menu_get_itemname, &menus[menu], false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&(menus[menu].synclist), NULL);
    rb->gui_synclist_set_nb_items(&(menus[menu].synclist), count);
    menus[menu].callback = callback;
    (void)button1;
    (void)button2;
    (void)button3;
    return menu;
}

void menu_exit(int m)
{
    inuse[m] = false;
}

int menu_show(int m)
{
    bool exit = false;
    int key;

    rb->gui_synclist_draw(&(menus[m].synclist));
    while (!exit) {
        key = rb->get_action(CONTEXT_MAINMENU,HZ/2);
        /*
         * "short-circuit" the default keypresses by running the
         * callback function
         * The callback may return a new key value, often this will be
         * BUTTON_NONE or the same key value, but it's perfectly legal
         * to "simulate" key presses by returning another value.
         */
        if( menus[m].callback != NULL )
            key = menus[m].callback(key, m);
        rb->gui_synclist_do_button(&(menus[m].synclist), &key,LIST_WRAP_UNLESS_HELD);
        switch( key ) {
            case ACTION_STD_OK:
                return rb->gui_synclist_get_sel_pos(&(menus[m].synclist));

            case ACTION_STD_CANCEL:
            case ACTION_STD_MENU:
            case SYS_POWEROFF:
                exit = true;
                break;

            default:
                if(rb->default_event_handler(key) == SYS_USB_CONNECTED)
                    return MENU_ATTACHED_USB;
                break;
        }
    }
    return MENU_SELECTED_EXIT;
}


bool menu_run(int m)
{
    int selected;
    while (1) {
        switch (selected=menu_show(m))
        {
            case MENU_SELECTED_EXIT:
                return false;

            case MENU_ATTACHED_USB:
                return true;

            default:
            {
                if (menus[m].items[selected].function &&
                    menus[m].items[selected].function())
                    return  true;
            }
        }
    }
    return false;
}

/*
 *  Property function - return the current cursor for "menu"
 */

int menu_cursor(int menu)
{
    return rb->gui_synclist_get_sel_pos(&(menus[menu].synclist));
}

/*
 *  Property function - return the "menu" description at "position"
 */

char* menu_description(int menu, int position)
{
    return menus[menu].items[position].desc;
}

/*
 *  Delete the element "position" from the menu items in "menu"
 */

void menu_delete(int menu, int position)
{
    int i;
    int nb_items=rb->gui_synclist_get_nb_items(&(menus[menu].synclist));
    /* copy the menu item from the one below */
    for( i = position; i < nb_items - 1; i++)
        menus[menu].items[i] = menus[menu].items[i + 1];

    rb->gui_synclist_del_item(&(menus[menu].synclist));
}

void menu_insert(int menu, int position, char *desc, bool (*function) (void))
{
    int i;
    int nb_items=rb->gui_synclist_get_nb_items(&(menus[menu].synclist));
    if(position < 0)
       position = nb_items;

    /* Move the items below one position forward */
    for( i = nb_items; i > position; i--)
       menus[menu].items[i] = menus[menu].items[i - 1];

    /* Update the current item */
    menus[menu].items[position].desc = (unsigned char *)desc;
    menus[menu].items[position].function = function;
    rb->gui_synclist_add_item(&(menus[menu].synclist));
}

/*
 *  Property function - return the "count" of menu items in "menu"
 */

int menu_count(int menu)
{
    return rb->gui_synclist_get_nb_items(&(menus[menu].synclist));
}

/*
 * Allows to set the cursor position. Doesn't redraw by itself.
 */

void menu_set_cursor(int menu, int position)
{
    rb->gui_synclist_select_item(&(menus[menu].synclist), position);
}
#if 0
void menu_talk_selected(int m)
{
    if(rb->global_settings->talk_menu)
    {
        int selected=rb->gui_synclist_get_sel_pos(&(menus[m].synclist));
        int voice_id = P2ID(menus[m].items[selected].desc);
        if (voice_id >= 0) /* valid ID given? */
            talk_id(voice_id, false); /* say it */
    }
}
#endif
void menu_draw(int m)
{
    rb->gui_synclist_draw(&(menus[m].synclist));
}
