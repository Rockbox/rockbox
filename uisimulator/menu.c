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

#include "types.h"
#include "lcd.h"
#include "menu.h"

#include "tree.h"

#ifdef HAVE_LCD_BITMAP

#include "screensaver.h"
extern void tetris(void);


#define MENU_ITEM_FONT    0
#define MENU_ITEM_Y_LOC   6
#define MENU_LINE_HEIGHT  8

enum Main_Menu_Ids {
    Tetris, Screen_Saver, Browse, Last_Id
};

struct Main_Menu_Items items[] = {
    { Tetris,       "Tetris",       tetris      },
    { Screen_Saver, "Screen Saver", screensaver },
    { Browse,       "Browse",       browse_root },
};

/* Global values for menuing */
int menu_top;
int menu_bottom;
int menu_line_height;
int cursor;

int get_line_height(void)
{
    return menu_line_height;
}

int is_cursor_menu_top(void)
{
    return ((cursor == menu_top) ? 1 : 0);
}

int is_cursor_menu_bottom(void)
{
    return ((cursor == menu_bottom) ? 1 : 0);
}

void put_cursor_menu_top(void)
{
    put_cursor(menu_top);
}

void put_cursor_menu_bottom(void)
{
    put_cursor(menu_bottom);
}

void move_cursor_up(void)
{
    put_cursor(cursor-1);
}

void move_cursor_down(void)
{
    put_cursor(cursor+1);
}

void redraw_cursor(void)
{
    lcd_puts(0, cursor*menu_line_height, "-", 0);
}

/* 
 * Move the cursor to a particular id, 
 *   current: where it is now 
 *   target: where you want it to be 
 */
void put_cursor(int target)
{
    lcd_puts(0, cursor*menu_line_height, " ", 0);
    cursor = target;
    lcd_puts(0, cursor*menu_line_height, "-", 0);
}

/* We call the function pointer related to the current cursor position */
void execute_menu_item(void)
{
    /* call the proper function for this line */
    items[cursor].function();
}

void add_menu_item(int location, char *string)
{
    lcd_puts(MENU_ITEM_Y_LOC, MENU_LINE_HEIGHT*location,
             string, MENU_ITEM_FONT);
    if (location < menu_top)
        menu_top = location;
    if (location > menu_bottom)
        menu_bottom = location;
}

void menu_init(void)
{
    int i = 0;

    menu_top = Tetris;
    menu_bottom = Last_Id-1;
    menu_line_height = MENU_LINE_HEIGHT;
    cursor = menu_top;

    for (i = i; i < Last_Id; i++)
        add_menu_item(items[i].menu_id, (char *) items[i].menu_desc);

    lcd_puts(8, 38, "Rockbox!", 2);
    redraw_cursor();
}

#endif /* end HAVE_LCD_BITMAP */


