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
#include "menu.h"

/* Apps to include */
#include "tree.h"

#ifdef HAVE_LCD_BITMAP

/*#include "screensaver.h"*/

/*extern void tetris(void);*/

void app_main(void)
{
    int key;
    
    menu_init();
    put_cursor_menu_top();
    
    while(1) {
        key = button_get();
        
        if(!key) {
            sleep(1);
            continue;
        }
        
        switch(key) {
        case BUTTON_UP:
            if(is_cursor_menu_top()){
                /* wrap around to menu bottom */
                put_cursor_menu_bottom();
            } else {
                /* move up */
                move_cursor_up();
            }
            break;
        case BUTTON_DOWN:
            if(is_cursor_menu_bottom() ){
                /* wrap around to menu top */
                put_cursor_menu_top();
            } else {
                /* move down */
                move_cursor_down();
            }
            break;
        case BUTTON_RIGHT:      
        case BUTTON_PLAY:      
            /* Erase current display state */
            lcd_clear_display();
            
            execute_menu_item();
            
            /* Return to previous display state */
            lcd_clear_display();
            menu_init();
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
