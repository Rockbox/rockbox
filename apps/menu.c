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
#include "backlight.h"
#include "menu.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"
#include "usb.h"
#include "panic.h"
#include "settings.h"
#include "status.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

#ifdef LOADABLE_FONTS
#include "ajf.h"
#endif

struct menu {
    int top;
    int cursor;
    struct menu_items* items;
    int itemcount;
};

#define MAX_MENUS 4

#ifdef HAVE_LCD_BITMAP

#define MARGIN_X    (global_settings.scrollbar ? SCROLLBAR_WIDTH : 0) + CURSOR_WIDTH /* X pixel margin */
#define MARGIN_Y    (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)  /* Y pixel margin */

#define LINE_X      0 /* X position the entry-list starts at */
#define LINE_Y      (global_settings.statusbar ? 1 : 0) /* Y position the entry-list starts at */
#define LINE_HEIGTH 8 /* pixels for each text line */

#define MENU_LINES  (LCD_HEIGHT / LINE_HEIGTH - LINE_Y)

#define CURSOR_X    (global_settings.scrollbar ? 1 : 0)
#define CURSOR_Y    0 /* the cursor is not positioned in regard to
                         the margins, so this is the amount of lines
                         we add to the cursor Y position to position
                         it on a line */
#define CURSOR_WIDTH  4

#define SCROLLBAR_X      0
#define SCROLLBAR_Y      lcd_getymargin()
#define SCROLLBAR_WIDTH  6

#else /* HAVE_LCD_BITMAP */

#define LINE_X      1 /* X position the entry-list starts at */

#define MENU_LINES 2

#define CURSOR_X    0
#define CURSOR_Y    0 /* not really used for players */

#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_NEW_CHARCELL_LCD
#define CURSOR_CHAR 0x7e
#else
#define CURSOR_CHAR 0x89
#endif

static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };

/* count in letter posistions, NOT pixels */
void put_cursorxy(int x, int y, bool on)
{
#ifdef HAVE_LCD_BITMAP
#ifdef LOADABLE_FONTS
    int fh;
    unsigned char* font = lcd_getcurrentldfont();
    fh = ajf_get_fontheight(font);
#else 
    int fh = 8;
#endif
#endif

    /* place the cursor */
    if(on) {
#ifdef HAVE_LCD_BITMAP
        lcd_bitmap ( bitmap_icons_6x8[Cursor], 
                     x*6, y*fh + lcd_getymargin(), 4, 8, true);
#elif defined(SIMULATOR)
        /* player simulator */
        unsigned char cursor[] = { 0x7f, 0x3e, 0x1c, 0x08 };
        lcd_bitmap ( cursor, x*6, 12+y*16, 4, 8, true);
#else
        lcd_putc(x, y, CURSOR_CHAR);
#endif
    }
    else {
#if defined(HAVE_LCD_BITMAP)
        /* I use xy here since it needs to disregard the margins */
        lcd_clearrect (x*6, y*fh + lcd_getymargin(), 4, 8);
#elif defined(SIMULATOR)
        /* player simulator in action */
        lcd_clearrect (x*6, 12+y*16, 4, 8);
#else
        lcd_putc(x, y, ' ');
#endif
    }
}

static void menu_draw(int m)
{
    int i = 0;
#ifdef LOADABLE_FONTS
    int menu_lines;
    int fh;
    unsigned char* font = lcd_getcurrentldfont();
    fh = ajf_get_fontheight(font);
    if (global_settings.statusbar)
        menu_lines = (LCD_HEIGHT - STATUSBAR_HEIGHT) / fh;
    else
        menu_lines = LCD_HEIGHT/fh;
#else
    int menu_lines = MENU_LINES;
#endif

    lcd_scroll_pause();  /* halt scroll first... */
    lcd_clear_display(); /* ...then clean the screen */
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(MARGIN_X,MARGIN_Y); /* leave room for cursor and icon */
    lcd_setfont(0);
#endif
    /* correct cursor pos if out of screen */
    if (menus[m].cursor - menus[m].top >= menu_lines)
        menus[m].top++;

    for (i = menus[m].top; 
         (i < menus[m].itemcount) && (i<menus[m].top+menu_lines);
         i++) {
        if((menus[m].cursor - menus[m].top)==(i-menus[m].top))
            lcd_puts_scroll(LINE_X, i-menus[m].top, menus[m].items[i].desc);
        else
            lcd_puts(LINE_X, i-menus[m].top, menus[m].items[i].desc);
    }

    /* place the cursor */
    put_cursorxy(CURSOR_X, menus[m].cursor - menus[m].top, true);
#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar) 
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  LCD_HEIGHT - SCROLLBAR_Y, menus[m].itemcount, menus[m].top,
                  menus[m].top + menu_lines, VERTICAL);
#endif
    status_draw();
    lcd_update();
}

/* 
 * Move the cursor to a particular id, 
 *   target: where you want it to be 
 */
static void put_cursor(int m, int target)
{
    bool do_update = true;
#ifdef LOADABLE_FONTS
    int menu_lines;
    int fh;
    unsigned char* font = lcd_getcurrentldfont();
    fh = ajf_get_fontheight(font);
    if (global_settings.statusbar)
        menu_lines = (LCD_HEIGHT-STATUSBAR_HEIGHT)/fh;
    else
        menu_lines = LCD_HEIGHT/fh;
#else
    int menu_lines = MENU_LINES;
#endif
    put_cursorxy(CURSOR_X, menus[m].cursor - menus[m].top, false);
    menus[m].cursor = target;
    menu_draw(m);

    if ( target < menus[m].top ) {
        menus[m].top--;
        menu_draw(m);
        do_update = false;
    }
    else if ( target-menus[m].top > menu_lines-1 ) {
        menus[m].top++;
        menu_draw(m);
        do_update = false;
    }

    if (do_update) {
        put_cursorxy(CURSOR_X, menus[m].cursor - menus[m].top, true); 
        lcd_update();
    }

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
    menus[i].top = 0;
    menus[i].cursor = 0;

    return i;
}

void menu_exit(int m)
{
    inuse[m] = false;
}

Menu menu_run(int m)
{
    Menu result = MENU_OK;

    menu_draw(m);

    while(1) {
        switch( button_get_w_tmo(HZ/2) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#else
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
#endif
                if (menus[m].cursor) {
                    /* move up */
                    put_cursor(m, menus[m].cursor-1);
                }
                else {
                    /* move to bottom */
#ifdef HAVE_RECORDER_KEYPAD
                    menus[m].top = menus[m].itemcount-9;
#else
                    menus[m].top = menus[m].itemcount-3;
#endif
                    if (menus[m].top < 0)
                        menus[m].top = 0;
                    menus[m].cursor = menus[m].itemcount-1;
                    put_cursor(m, menus[m].itemcount-1);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#else
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                if (menus[m].cursor < menus[m].itemcount-1) {
                    /* move down */
                    put_cursor(m, menus[m].cursor+1);
                }
                else {
                    /* move to top */
                    menus[m].top = 0;
                    menus[m].cursor = 0;
                    put_cursor(m, 0);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_RIGHT:
#endif
            case BUTTON_PLAY:
                /* Erase current display state */
                lcd_scroll_pause(); /* pause is better than stop when
                                       are gonna clear the screen anyway */
                lcd_clear_display();
            
                /* if a child returns that the contents is changed, we
                   must remember this, even if we perhaps invoke other
                   children too before returning back */
                if(MENU_DISK_CHANGED ==
                   menus[m].items[menus[m].cursor].function())
                  result = MENU_DISK_CHANGED;
            
                /* Return to previous display state */
                menu_draw(m);
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
            case BUTTON_F1:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                lcd_stop_scroll();
                while (button_get(false)); /* clear button queue */
                return result;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F3: {
#ifdef HAVE_LCD_BITMAP
                  unsigned char state;
                  state = global_settings.statusbar << 1 | global_settings.scrollbar;
                  state = (state + 1) % 4;
                  global_settings.statusbar = state >> 1;
                  global_settings.scrollbar = state & 0x1;
                  settings_save();

                  menu_draw(m);
#endif
                }
                break;
#endif

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                backlight_time(4);
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&button_queue);
                backlight_time(global_settings.backlight);
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_PARAM, true);
#endif
                menu_draw(m);
                result = MENU_DISK_CHANGED;
                break;
#endif

            default:
                break;
        }
        
        status_draw();
        lcd_update();
    }

    return result;
}
