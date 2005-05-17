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
#include <stdlib.h>

#include "hwcompat.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "menu.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"
#include "usb.h"
#include "panic.h"
#include "settings.h"
#include "status.h"
#include "screens.h"
#include "talk.h"
#include "lang.h"
#include "misc.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

struct menu {
    int top;
    int cursor;
    struct menu_item* items;
    int itemcount;
    int (*callback)(int, int);
#if defined(HAVE_LCD_BITMAP) && (CONFIG_KEYPAD == RECORDER_PAD)
    bool use_buttonbar; /* true if a buttonbar is defined */
    const char *buttonbar[3];
#endif
};

#define MAX_MENUS 5

#ifdef HAVE_LCD_BITMAP

/* pixel margins */
#define MARGIN_X (global_settings.scrollbar && \
                  menu_lines < menus[m].itemcount ? SCROLLBAR_WIDTH : 0) +\
                  CURSOR_WIDTH
#define MARGIN_Y (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)

/* position the entry-list starts at */
#define LINE_X   0
#define LINE_Y   (global_settings.statusbar ? 1 : 0)

#define CURSOR_X (global_settings.scrollbar && \
                  menu_lines < menus[m].itemcount ? 1 : 0)
#define CURSOR_Y 0 /* the cursor is not positioned in regard to
                      the margins, so this is the amount of lines
                      we add to the cursor Y position to position
                      it on a line */
#define CURSOR_WIDTH (global_settings.invert_cursor ? 0 : 4)

#define SCROLLBAR_X      0
#define SCROLLBAR_Y      lcd_getymargin()
#define SCROLLBAR_WIDTH  6

#else /* HAVE_LCD_BITMAP */

#define LINE_X      1 /* X position the entry-list starts at */

#define MENU_LINES 2

#define CURSOR_X    0
#define CURSOR_Y    0 /* not really used for players */

#endif /* HAVE_LCD_BITMAP */

#define CURSOR_CHAR 0x92

static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };

/* count in letter positions, NOT pixels */
void put_cursorxy(int x, int y, bool on)
{
#ifdef HAVE_LCD_BITMAP
    int fh, fw;
    int xpos, ypos;

    /* check here instead of at every call (ugly, but cheap) */
    if (global_settings.invert_cursor)
        return;

    lcd_getstringsize("A", &fw, &fh);
    xpos = x*6;
    ypos = y*fh + lcd_getymargin();
    if ( fh > 8 )
        ypos += (fh - 8) / 2;
#endif

    /* place the cursor */
    if(on) {
#ifdef HAVE_LCD_BITMAP
        lcd_bitmap ( bitmap_icons_6x8[Cursor], 
                     xpos, ypos, 4, 8, true);
#else
        lcd_putc(x, y, CURSOR_CHAR);
#endif
    }
    else {
#if defined(HAVE_LCD_BITMAP)
        /* I use xy here since it needs to disregard the margins */
        lcd_clearrect (xpos, ypos, 4, 8);
#else
        lcd_putc(x, y, ' ');
#endif
    }
}

void menu_draw(int m)
{
    int i = 0;
#ifdef HAVE_LCD_BITMAP
    int fw, fh;
    int menu_lines;
    int height = LCD_HEIGHT;
    
    lcd_setfont(FONT_UI);
    lcd_getstringsize("A", &fw, &fh);
    if (global_settings.statusbar)
        height -= STATUSBAR_HEIGHT;

#if CONFIG_KEYPAD == RECORDER_PAD
    if(global_settings.buttonbar && menus[m].use_buttonbar) {
        buttonbar_set(menus[m].buttonbar[0],
                      menus[m].buttonbar[1],
                      menus[m].buttonbar[2]);
        height -= BUTTONBAR_HEIGHT;
    }
#endif

    menu_lines = height / fh;
    
#else
    int menu_lines = MENU_LINES;
#endif

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(MARGIN_X,MARGIN_Y); /* leave room for cursor and icon */
#endif
    /* Adjust cursor pos if it's below the screen */
    if (menus[m].cursor - menus[m].top >= menu_lines)
        menus[m].top = menus[m].cursor - (menu_lines - 1);

    /* Adjust cursor pos if it's above the screen */
    if(menus[m].cursor < menus[m].top)
        menus[m].top = menus[m].cursor;

    for (i = menus[m].top; 
         (i < menus[m].itemcount) && (i<menus[m].top+menu_lines);
         i++) {

        /* We want to scroll the line where the cursor is */
        if((menus[m].cursor - menus[m].top)==(i-menus[m].top))
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor)
                lcd_puts_scroll_style(LINE_X, i-menus[m].top,
                                       P2STR(menus[m].items[i].desc), STYLE_INVERT);
            else
#endif
                lcd_puts_scroll(LINE_X, i-menus[m].top, P2STR(menus[m].items[i].desc));
        else
            lcd_puts(LINE_X, i-menus[m].top, P2STR(menus[m].items[i].desc));
    }

    /* place the cursor */
    put_cursorxy(CURSOR_X, menus[m].cursor - menus[m].top, true);
    
#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar && menus[m].itemcount > menu_lines) 
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  height, menus[m].itemcount, menus[m].top,
                  menus[m].top + menu_lines, VERTICAL);

#if CONFIG_KEYPAD == RECORDER_PAD
    if(global_settings.buttonbar && menus[m].use_buttonbar)
        buttonbar_draw();
#endif /* CONFIG_KEYPAD == RECORDER_PAD */
#endif /* HAVE_LCD_BITMAP */
    status_draw(true);

    lcd_update();
}

/* 
 * Move the cursor to a particular id, 
 *   target: where you want it to be 
 */
static void put_cursor(int m, int target)
{
    int voice_id;
    
    menus[m].cursor = target;
    menu_draw(m);

    /* "say" the entry under the cursor */
    if(global_settings.talk_menu)
    {
        voice_id = P2ID(menus[m].items[menus[m].cursor].desc);
        if (voice_id >= 0) /* valid ID given? */
            talk_id(voice_id, false); /* say it */
    }
}

int menu_init(const struct menu_item* mitems, int count, int (*callback)(int, int),
              const char *button1, const char *button2, const char *button3)
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
    menus[i].items = (struct menu_item*)mitems; /* de-const */
    menus[i].itemcount = count;
    menus[i].top = 0;
    menus[i].cursor = 0;
    menus[i].callback = callback;
#if defined(HAVE_LCD_BITMAP) && (CONFIG_KEYPAD == RECORDER_PAD)
    menus[i].buttonbar[0] = button1;
    menus[i].buttonbar[1] = button2;
    menus[i].buttonbar[2] = button3;

    if(button1 || button2 || button3)
        menus[i].use_buttonbar = true;
    else
        menus[i].use_buttonbar = false;
#else
    (void)button1;
    (void)button2;
    (void)button3;
#endif
    return i;
}

void menu_exit(int m)
{
    inuse[m] = false;
}

int menu_show(int m)
{
    bool exit = false;
    int key;
#ifdef HAVE_LCD_BITMAP
    int fw, fh;
    int menu_lines;
    int height = LCD_HEIGHT;
    
    lcd_setfont(FONT_UI);
    lcd_getstringsize("A", &fw, &fh);
    if (global_settings.statusbar)
        height -= STATUSBAR_HEIGHT;

#if CONFIG_KEYPAD == RECORDER_PAD
    if(global_settings.buttonbar && menus[m].use_buttonbar) {
        buttonbar_set(menus[m].buttonbar[0],
                      menus[m].buttonbar[1],
                      menus[m].buttonbar[2]);
        height -= BUTTONBAR_HEIGHT;
    }
#endif

    menu_lines = height / fh;
#else
    int menu_lines = MENU_LINES;
#endif

    /* Put the cursor on the first line and draw the menu */
    put_cursor(m, menus[m].cursor);

    while (!exit) {
        key = button_get_w_tmo(HZ/2);

        /*  
         * "short-circuit" the default keypresses by running the
         * callback function
         * The callback may return a new key value, often this will be
         * BUTTON_NONE or the same key value, but it's perfectly legal
         * to "simulate" key presses by returning another value.
         */

        if( menus[m].callback != NULL )
            key = menus[m].callback(key, m);
        
        switch( key ) {
            case MENU_PREV:
            case MENU_PREV | BUTTON_REPEAT:
                if (menus[m].cursor) {
                    /* move up */
                    put_cursor(m, menus[m].cursor-1);
                }
                else {
                    /* move to bottom */
                    menus[m].top = menus[m].itemcount-(menu_lines+1);
                    if (menus[m].top < 0)
                        menus[m].top = 0;
                    menus[m].cursor = menus[m].itemcount-1;
                    put_cursor(m, menus[m].itemcount-1);
                }
                break;

            case MENU_NEXT:
            case MENU_NEXT | BUTTON_REPEAT:
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

            case MENU_ENTER:
#ifdef MENU_ENTER2
            case MENU_ENTER2:
#endif
                /* Erase current display state */
                lcd_clear_display();
                return menus[m].cursor;

            case MENU_EXIT:
#ifdef MENU_EXIT2
            case MENU_EXIT2:
#endif
#ifdef MENU_EXIT3
            case MENU_EXIT3:
#endif
                lcd_stop_scroll();
                exit = true;
                break;

            default:
                if(default_event_handler(key) == SYS_USB_CONNECTED)
                    return MENU_ATTACHED_USB;
                break;
        }
        
        status_draw(false);
    }
    return MENU_SELECTED_EXIT;
}


bool menu_run(int m)
{
    while (1) {
        switch (menu_show(m))
        {
            case MENU_SELECTED_EXIT:
                return false;

            case MENU_ATTACHED_USB:
                return true;

            default:
                if ((menus[m].items[menus[m].cursor].function) &&
                    (menus[m].items[menus[m].cursor].function()))
                    return true;
        }
    }
    return false;
}

/*  
 *  Property function - return the current cursor for "menu"
 */

int menu_cursor(int menu)
{
    return menus[menu].cursor;
}

/*  
 *  Property function - return the "menu" description at "position"
 */

char* menu_description(int menu, int position)
{
    return P2STR(menus[menu].items[position].desc);
}

/*  
 *  Delete the element "position" from the menu items in "menu"
 */

void menu_delete(int menu, int position)
{
    int i;
    
    /* copy the menu item from the one below */
    for( i = position; i < (menus[menu].itemcount - 1); i++)
        menus[menu].items[i] = menus[menu].items[i + 1];
 
    /* reduce the count */       
    menus[menu].itemcount--;
    
    /* adjust if this was the last menu item and the cursor was on it */
    if(menus[menu].itemcount && menus[menu].itemcount <= menus[menu].cursor)
        menus[menu].cursor = menus[menu].itemcount - 1;
}

void menu_insert(int menu, int position, char *desc, bool (*function) (void))
{
    int i;

    if(position < 0)
       position = menus[menu].itemcount;

    /* Move the items below one position forward */
    for( i = menus[menu].itemcount; i > position; i--)
       menus[menu].items[i] = menus[menu].items[i - 1];

    /* Increase the count */
    menus[menu].itemcount++;
    
    /* Update the current item */
    menus[menu].items[position].desc = desc;
    menus[menu].items[position].function = function;
}

/*  
 *  Property function - return the "count" of menu items in "menu" 
 */

int menu_count(int menu)
{
    return menus[menu].itemcount;
}

/*  
 *  Allows a menu item at the current cursor position in "menu" to be moved up the list
 */

bool menu_moveup(int menu)
{
    struct menu_item swap;
    
    /* can't be the first item ! */
    if( menus[menu].cursor == 0)
        return false;
    
    /* use a temporary variable to do the swap */    
    swap = menus[menu].items[menus[menu].cursor - 1];
    menus[menu].items[menus[menu].cursor - 1] = menus[menu].items[menus[menu].cursor];
    menus[menu].items[menus[menu].cursor] = swap;
    menus[menu].cursor--;
    
    return true;
}

/*  
 *  Allows a menu item at the current cursor position in "menu" to be moved down the list
 */

bool menu_movedown(int menu)
{
    struct menu_item swap;
    
    /* can't be the last item ! */
    if( menus[menu].cursor == menus[menu].itemcount - 1)
        return false;
    
    /* use a temporary variable to do the swap */    
    swap = menus[menu].items[menus[menu].cursor + 1];
    menus[menu].items[menus[menu].cursor + 1] = menus[menu].items[menus[menu].cursor];
    menus[menu].items[menus[menu].cursor] = swap;
    menus[menu].cursor++;
    
    return true;
}

/*
 * Allows to set the cursor position. Doesn't redraw by itself.
 */

void menu_set_cursor(int menu, int position)
{
    menus[menu].cursor = position;
}

