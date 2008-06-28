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


/* This API is for existing plugins and shouldn't be used by new ones.
   This provides a simpler menu system for plugins, but does not allow for
   translatable or talkable strings in the menus. */
#ifndef __OLDMENUAPI_H__
#define __OLDMENUAPI_H__

#include <stdbool.h>

struct menu_item {
    unsigned char *desc; /* string or ID */
    bool (*function) (void); /* return true if USB was connected */
};

int menu_init(const struct plugin_api *api, const struct menu_item* mitems, 
              int count, int (*callback)(int, int),
              const char *button1, const char *button2, const char *button3);
void menu_exit(int menu);

void put_cursorxy(int x, int y, bool on);

 /* Returns below define, or number of selected menu item*/
int menu_show(int m);

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

#endif /* End __OLDMENUAPI_H__ */
