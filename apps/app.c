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

#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "menu.h"

/* Apps to include */
#include "tree.h"

/* Wait on a key press.  Return the key pressed */
int busy_wait(void)
{
    int key;

    while(1) {
        key = button_get();
        
        if(!key)
            sleep(1);
        else
            break;
    }

    return key;
}

#ifdef HAVE_LCD_BITMAP
void app_main(void)
{
    int key;

    show_splash();

    menu_init();
    menu_draw();
    put_cursor_menu_top();
    
    while(1) {
        key = busy_wait();
        
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
            menu_draw();
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

    show_splash();

    browse_root();
    
}

#endif




