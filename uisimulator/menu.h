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

struct main_menu_items {
    int menu_id;
    const char *menu_desc;
    void (*function) (void);
};

int get_line_height(void);

/* Cursor calls */
void put_cursor(int target);
void put_cursor_menu_top(void);
void put_cursor_menu_bottom(void);
void move_cursor_up(void);
void move_cursor_down(void);
int is_cursor_menu_top(void);
int is_cursor_menu_bottom(void);

/* Menu calls */
void add_menu_item(int location, char *string);
void menu_init(void);
void execute_menu_item(void);

#endif /* End __MENU_H__ */





