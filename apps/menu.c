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
/*
2005 Kevin Ferrare :
 - Multi screen support
 - Rewrote/removed a lot of code now useless with the new gui API
*/
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
#endif

/* gui api */
#include "list.h"
#include "statusbar.h"
#include "buttonbar.h"

struct menu {
    struct menu_item* items;
    int (*callback)(int, int);
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
#endif
    struct gui_synclist synclist;
};

#define MAX_MENUS 5

static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };

char * menu_get_itemname(int selected_item, void * data, char *buffer)
{
    struct menu *local_menus=(struct menu *)data;
    (void)buffer;
    return(P2STR(local_menus->items[selected_item].desc));
}

int menu_find_free(void)
{
    int i;
    /* Tries to find an unused slot to put the new menu */
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
    return(i);
}

int menu_init(const struct menu_item* mitems, int count, int (*callback)(int, int),
              const char *button1, const char *button2, const char *button3)
{
    int menu=menu_find_free();
    if(menu==-1)/* Out of menus */
        return -1;
    menus[menu].items = (struct menu_item*)mitems; /* de-const */
    gui_synclist_init(&(menus[menu].synclist),
                      &menu_get_itemname, &menus[menu]);
    gui_synclist_set_icon_callback(&(menus[menu].synclist), NULL);
    gui_synclist_set_nb_items(&(menus[menu].synclist), count);
    menus[menu].callback = callback;
#ifdef HAS_BUTTONBAR
    gui_buttonbar_init(&(menus[menu].buttonbar));
    gui_buttonbar_set_display(&(menus[menu].buttonbar), &(screens[SCREEN_MAIN]) );
    gui_buttonbar_set(&(menus[menu].buttonbar), button1, button2, button3);
#else
    (void)button1;
    (void)button2;
    (void)button3;
#endif
    return menu;
}

void menu_exit(int m)
{
    inuse[m] = false;
}

int menu_show(int m)
{
#ifdef HAS_BUTTONBAR
    gui_buttonbar_draw(&(menus[m].buttonbar));
#endif
    bool exit = false;
    int key;

    gui_synclist_draw(&(menus[m].synclist));
    gui_syncstatusbar_draw(&statusbars, true);
    menu_talk_selected(m);
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
        /* If moved, "say" the entry under the cursor */
        if(gui_synclist_do_button(&(menus[m].synclist), key))
            menu_talk_selected(m);
        switch( key ) {
            case MENU_ENTER:
#ifdef MENU_ENTER2
            case MENU_ENTER2:
#endif
#ifdef MENU_RC_ENTER
            case MENU_RC_ENTER:
#endif
#ifdef MENU_RC_ENTER2
            case MENU_RC_ENTER2:
#endif
                return gui_synclist_get_sel_pos(&(menus[m].synclist));


            case MENU_EXIT:
#ifdef MENU_EXIT2
            case MENU_EXIT2:
#endif
#ifdef MENU_EXIT_MENU
            case MENU_EXIT_MENU:
#endif
#ifdef MENU_RC_EXIT
            case MENU_RC_EXIT:
#endif
#ifdef MENU_RC_EXIT_MENU
            case MENU_RC_EXIT_MENU:
#endif
                exit = true;
                break;

            default:
                if(default_event_handler(key) == SYS_USB_CONNECTED)
                    return MENU_ATTACHED_USB;
                break;
        }
        gui_syncstatusbar_draw(&statusbars, false);
    }
    return MENU_SELECTED_EXIT;
}


bool menu_run(int m)
{
    int selected;
    while (1) {
        switch (selected=menu_show(m))
        {
            case MENU_SELECTED_EXIT:
                return false;

            case MENU_ATTACHED_USB:
                return true;

            default:
            {
                if (menus[m].items[selected].function &&
                    menus[m].items[selected].function())
                    return  true;
                gui_syncstatusbar_draw(&statusbars, true);
            }
        }
    }
    return false;
}

/*
 *  Property function - return the current cursor for "menu"
 */

int menu_cursor(int menu)
{
    return gui_synclist_get_sel_pos(&(menus[menu].synclist));
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
    int nb_items=gui_synclist_get_nb_items(&(menus[menu].synclist));
    /* copy the menu item from the one below */
    for( i = position; i < nb_items - 1; i++)
        menus[menu].items[i] = menus[menu].items[i + 1];

    gui_synclist_del_item(&(menus[menu].synclist));
}

void menu_insert(int menu, int position, char *desc, bool (*function) (void))
{
    int i;
    int nb_items=gui_synclist_get_nb_items(&(menus[menu].synclist));
    if(position < 0)
       position = nb_items;

    /* Move the items below one position forward */
    for( i = nb_items; i > position; i--)
       menus[menu].items[i] = menus[menu].items[i - 1];

    /* Update the current item */
    menus[menu].items[position].desc = (unsigned char *)desc;
    menus[menu].items[position].function = function;
    gui_synclist_add_item(&(menus[menu].synclist));
}

/*
 *  Property function - return the "count" of menu items in "menu"
 */

int menu_count(int menu)
{
    return gui_synclist_get_nb_items(&(menus[menu].synclist));
}

/*
 *  Allows a menu item at the current cursor position in "menu"
 * to be moved up the list
 */

bool menu_moveup(int menu)
{
    struct menu_item swap;
    int selected=menu_cursor(menu);
    /* can't be the first item ! */
    if( selected == 0)
        return false;

    /* use a temporary variable to do the swap */
    swap = menus[menu].items[selected - 1];
    menus[menu].items[selected - 1] = menus[menu].items[selected];
    menus[menu].items[selected] = swap;

    gui_synclist_select_previous(&(menus[menu].synclist));
    return true;
}

/*
 *  Allows a menu item at the current cursor position in "menu" to be moved down the list
 */

bool menu_movedown(int menu)
{
    struct menu_item swap;
    int selected=menu_cursor(menu);
    int nb_items=gui_synclist_get_nb_items(&(menus[menu].synclist));

    /* can't be the last item ! */
    if( selected == nb_items - 1)
        return false;

    /* use a temporary variable to do the swap */
    swap = menus[menu].items[selected + 1];
    menus[menu].items[selected + 1] = menus[menu].items[selected];
    menus[menu].items[selected] = swap;

    gui_synclist_select_next(&(menus[menu].synclist));
    return true;
}

/*
 * Allows to set the cursor position. Doesn't redraw by itself.
 */

void menu_set_cursor(int menu, int position)
{
    gui_synclist_select_item(&(menus[menu].synclist), position);
}

void menu_talk_selected(int m)
{
    if(global_settings.talk_menu)
    {
        int selected=gui_synclist_get_sel_pos(&(menus[m].synclist));
        int voice_id = P2ID(menus[m].items[selected].desc);
        if (voice_id >= 0) /* valid ID given? */
            talk_id(voice_id, false); /* say it */
    }
}

void menu_draw(int m)
{
    gui_synclist_draw(&(menus[m].synclist));
}

/* count in letter positions, NOT pixels */
void put_cursorxy(int x, int y, bool on)
{
#ifdef HAVE_LCD_BITMAP
    int fh, fw;
    int xpos, ypos;

    /* check here instead of at every call (ugly, but cheap) */
    if (global_settings.invert_cursor)
        return;

    lcd_getstringsize((unsigned char *)"A", &fw, &fh);
    xpos = x*6;
    ypos = y*fh + lcd_getymargin();
    if ( fh > 8 )
        ypos += (fh - 8) / 2;
#endif

    /* place the cursor */
    if(on) {
#ifdef HAVE_LCD_BITMAP
        lcd_mono_bitmap(bitmap_icons_6x8[Icon_Cursor], xpos, ypos, 4, 8);
#else
        lcd_putc(x, y, CURSOR_CHAR);
#endif
    }
    else {
#if defined(HAVE_LCD_BITMAP)
        /* I use xy here since it needs to disregard the margins */
        lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        lcd_fillrect (xpos, ypos, 4, 8);
        lcd_set_drawmode(DRMODE_SOLID);
#else
        lcd_putc(x, y, ' ');
#endif
    }
}
