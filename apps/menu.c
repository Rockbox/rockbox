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

#include "lcd.h"
#include "menu.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"

/* Global values for menuing */
static int menu_top;
static int menu_bottom;
static int cursor;
static struct menu_items* items;
static int itemcount;

/* 
 * Move the cursor to a particular id, 
 *   target: where you want it to be 
 */
static void put_cursor(int target)
{
    lcd_puts(0, cursor, " ");
    cursor = target;
    lcd_puts(0, cursor, "-");
}

/* We call the function pointer related to the current cursor position */
static void execute_menu_item(void)
{
    /* call the proper function for this line */
    items[cursor].function();
}

void menu_init(struct menu_items* mitems, int count)
{
    items = mitems;
    itemcount = count;
    menu_top = items[0].id;
    menu_bottom = count-1;
    cursor = menu_top;
}

static void menu_draw(void)
{
    int i = 0;

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,0);
    lcd_setfont(0);
#endif
    for (i = 0; i < itemcount; i++) {
        lcd_puts(1, i, items[i].desc);
        if (i < menu_top)
            menu_top = i;
        if (i > menu_bottom)
            menu_bottom = i;
    }

    lcd_puts(0, cursor, "-");
    lcd_update();
}

void menu_run(void)
{
    int key;

    menu_draw();
    
    while(1) {
        key = button_get();
        if(!key) {
            sleep(1);
            continue;
        }
        
        switch(key) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_LEFT:
#endif
                if (cursor == menu_top) {
                    /* wrap around to menu bottom */
                    put_cursor(menu_bottom);
                } else {
                    /* move up */
                    put_cursor(cursor-1);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_RIGHT:
#endif
                if (cursor == menu_bottom) {
                    /* wrap around to menu top */
                    put_cursor(menu_top);
                } else {
                    /* move down */
                    put_cursor(cursor+1);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_RIGHT:
#endif
            case BUTTON_PLAY:
                /* Erase current display state */
                lcd_clear_display();
            
                execute_menu_item();
            
                /* Return to previous display state */
                menu_draw();
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
#endif
                return;

            default:
                break;
        }
        
        lcd_update();
    }
}
