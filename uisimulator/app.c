/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
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
#include "button.h"
#include "kernel.h"

#include "tree.h"

extern void tetris(void);

#define MENU_ITEM_FONT    0
#define MENU_ITEM_Y_LOC   6
#define MENU_LINE_HEIGHT  8

/* menu ids */
#define ITEM_TETRIS       0
#define ITEM_SCREENSAVER  1
#define ITEM_BROWSE       2

/* the last index with info, starting on 0 */
#define MAX_LINE          ITEM_BROWSE

int menu_top = 0;
int menu_bottom = MAX_LINE;

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
    add_menu_item(ITEM_SCREENSAVER, "Screen Saver");
    add_menu_item(ITEM_BROWSE, "Browse");
    add_menu_item(ITEM_TETRIS, "Tetris");

    lcd_puts(8, 38, "Rockbox!", 2);
}

/* 
 * Move the cursor to a particular id, 
 *   current: where it is now 
 *   target: where you want it to be 
 * Returns: the new current location
 */
int put_cursor(int current, int target)
{
    lcd_puts(0, current*MENU_LINE_HEIGHT, " ", 0);
    lcd_puts(0, target*MENU_LINE_HEIGHT, "-", 0);
    return target;
}

void app_main(void)
{
    int key;
    int cursor = 0;

    menu_init();
    cursor = put_cursor(menu_top, menu_top);

    while(1) {
        key = button_get();

        if(!key) {
            sleep(1);
            continue;
        }

        switch(key) {
        case BUTTON_UP:
            if(cursor == menu_top ){
                /* wrap around to menu bottom */
                cursor = put_cursor(cursor, menu_bottom);
            } else {
                /* move up */
                cursor = put_cursor(cursor, cursor-1);
            }
            break;
        case BUTTON_DOWN:
            if(cursor == menu_bottom ){
                /* wrap around to menu top */
                printf("from (%d) to (%d)\n", cursor, menu_top);
                cursor = put_cursor(cursor, menu_top);
            } else {
                /* move down */
                printf("from (%d) to (%d)\n", cursor, cursor+1);
                cursor = put_cursor(cursor, cursor+1);
            }
            break;
        case BUTTON_RIGHT:      
        case BUTTON_PLAY:      
            /* Erase current display state */
            lcd_clear_display();
            
            switch(cursor) {
            case ITEM_TETRIS:
                tetris();
                break;
            case ITEM_BROWSE:
                dirbrowse("/");
                break;
            case ITEM_SCREENSAVER:
                screensaver();
                break;
            default:
                continue;
            }

            /* Return to previous display state */
            lcd_clear_display();
            menu_init();
            cursor = put_cursor(cursor, cursor);
            break;
        case BUTTON_OFF:
            return;
        default:
            break;
        }

        lcd_update();
    }
}



