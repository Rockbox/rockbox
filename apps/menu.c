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
#include <stdbool.h>
#include "lcd.h"
#include "menu.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"

struct menu {
    int menu_top;
    int menu_bottom;
    int cursor;
    struct menu_items* items;
    int itemcount;
};

#define MAX_MENUS 4

static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };

/* 
 * Move the cursor to a particular id, 
 *   target: where you want it to be 
 */
static void put_cursor(int m, int target)
{
    lcd_puts(0, menus[m].cursor, " ");
    menus[m].cursor = target;
    lcd_puts(0, menus[m].cursor, "-");
}

int menu_init(struct menu_items* mitems, int count)
{
    int i;

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
    menus[i].items = mitems;
    menus[i].itemcount = count;
    menus[i].menu_top = 0;
    menus[i].menu_bottom = count-1;
    menus[i].cursor = 0;

    return i;
}

void menu_exit(int m)
{
    inuse[m] = false;
}

static void menu_draw(int m)
{
    int i = 0;

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,0);
    lcd_setfont(0);
#endif
    for (i = 0; i < menus[m].itemcount; i++) {
        lcd_puts(1, i, menus[m].items[i].desc);
        if (i < menus[m].menu_top)
            menus[m].menu_top = i;
        if (i > menus[m].menu_bottom)
            menus[m].menu_bottom = i;
    }

    lcd_puts(0, menus[m].cursor, "-");
    lcd_update();
}

void menu_run(int m)
{
    int key;

    menu_draw(m);
    
    while(1) {
        switch( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_LEFT:
#endif
                if (menus[m].cursor == menus[m].menu_top) {
                    /* wrap around to menu bottom */
                    put_cursor(m, menus[m].menu_bottom);
                } else {
                    /* move up */
                    put_cursor(m, menus[m].cursor-1);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_RIGHT:
#endif
                if (menus[m].cursor == menus[m].menu_bottom) {
                    /* wrap around to menu top */
                    put_cursor(m, menus[m].menu_top);
                } else {
                    /* move down */
                    put_cursor(m, menus[m].cursor+1);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_RIGHT:
#endif
            case BUTTON_PLAY:
                /* Erase current display state */
                lcd_clear_display();
            
                menus[m].items[menus[m].cursor].function();
            
                /* Return to previous display state */
                menu_draw(m);
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
