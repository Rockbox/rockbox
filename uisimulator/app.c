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

/* Apps to include */
#include "tree.h"
#include "screensaver.h"

#ifdef HAVE_LCD_BITMAP

extern void tetris(void);

#define MENU_ITEM_FONT    0
#define MENU_ITEM_Y_LOC   6
#define MENU_LINE_HEIGHT  8

enum Menu_Ids {
    Tetris, Screen_Saver, Browse, Last_Id
};

struct Menu_Items {
    int menu_id;
    const char *menu_desc;
    void (*function) (void);
};

struct Menu_Items items[] = {
    { Tetris,       "Tetris",       tetris      },
    { Screen_Saver, "Screen Saver", screensaver },
    { Browse,       "Browse",       browse_root },
};


int menu_top = Tetris;
int menu_bottom = Last_Id-1;

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
    for (i = i; i < Last_Id; i++)
        add_menu_item(items[i].menu_id, (char *) items[i].menu_desc);

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
                cursor = put_cursor(cursor, menu_top);
            } else {
                /* move down */
                cursor = put_cursor(cursor, cursor+1);
            }
            break;
        case BUTTON_RIGHT:      
        case BUTTON_PLAY:      
            /* Erase current display state */
            lcd_clear_display();

            /* call the proper function for this line */
            items[cursor].function();
            
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

#else

void app_main(void)
{
    int key;
    int cursor = 0;

    lcd_puts(0,0, "Mooo!");
    lcd_puts(1,1, " Rockbox!");

    while(1) {
      key = button_get();

      if(!key) {
        sleep(1);
        continue;
      }

    }
    
}

#endif
