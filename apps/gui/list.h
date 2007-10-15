/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _GUI_LIST_H_
#define _GUI_LIST_H_

#include "config.h"
#include "icon.h"
#include "screen_access.h"

#define SCROLLBAR_WIDTH  6

enum list_wrap {
    LIST_WRAP_ON = 0,
    LIST_WRAP_OFF,
    LIST_WRAP_UNLESS_HELD,
};

/*
 * The gui_list is based on callback functions, if you want the list
 * to display something you have to provide it a function that
 * tells it what to display.
 * There are three callback function :
 * one to get the text, one to get the icon and one to get the color
 */

/*
 * Icon callback
 *  - selected_item : an integer that tells the number of the item to display
 *  - data : a void pointer to the data you gave to the list when you
 *           initialized it
 *  Returns a pointer to the icon, the value inside it is used to display the
 *          icon after the function returns.
 * Note : we use the ICON type because the real type depends of the plateform
 */
typedef enum themable_icons list_get_icon(int selected_item, void * data);
/*
 * Text callback
 *  - selected_item : an integer that tells the number of the item to display
 *  - data : a void pointer to the data you gave to the list when you
 *           initialized it
 *  - buffer : a buffer to put the resulting text on it
 *             (The content of the buffer may not be used by the list, we use
 *             the return value of the function in all cases to avoid filling
 *             a buffer when it's not necessary)
 * Returns a pointer to a string that contains the text to display
 */
typedef char * list_get_name(int selected_item, void * data, char * buffer);
#ifdef HAVE_LCD_COLOR
/*
 * Color callback
 *  - selected_item : an integer that tells the number of the item to display
 *  - data : a void pointer to the data you gave to the list when you
 *           initialized it
 *  Returns an int with the lower 16 bits representing the color to display the
 *          selected item, negative value for default coloring.
 */
typedef int list_get_color(int selected_item, void * data);
#endif

struct gui_list
{
    /* defines wether the list should stop when reaching the top/bottom
     * or should continue (by going to bottom/top) */
    bool limit_scroll;
    /* wether the text of the whole items of the list have to be
     * scrolled or only for the selected item */
    bool scroll_all;

    int nb_items;
    int selected_item;
    int start_item; /* the item that is displayed at the top of the screen */
    /* the number of lines that are selected at the same time */
    int selected_size;
    /* These are used to calculate how much of the screen content we need
       to redraw. */
    int last_displayed_selected_item;
    int last_displayed_start_item;
#ifdef HAVE_LCD_BITMAP
    int offset_position; /* the list's screen scroll placement in pixels */
#endif
    /* Cache the width of the title string in pixels/characters */
    int title_width;

    list_get_icon *callback_get_item_icon;
    list_get_name *callback_get_item_name;

    struct screen * display;
    /* The data that will be passed to the callback function YOU implement */
    void * data;
    /* The optional title, set to NULL for none */
    char * title;
    /* Optional title icon */
    enum themable_icons title_icon;
    bool show_selection_marker; /* set to true by default */

#ifdef HAVE_LCD_COLOR
    int title_color;
    list_get_color *callback_get_item_color;
#endif
};

/*
 * Sets the numbers of items the list can currently display
 * note that the list's context like the currently pointed item is resetted
 *  - gui_list : the list structure
 *  - nb_items : the numbers of items you want
 */
#define gui_list_set_nb_items(gui_list, nb) \
    (gui_list)->nb_items = nb

/*
 * Returns the numbers of items currently in the list
 *  - gui_list : the list structure
 */
#define gui_list_get_nb_items(gui_list) \
    (gui_list)->nb_items

/*
 * Sets the icon callback function
 *  - gui_list : the list structure
 *  - _callback : the callback function
 */
#define gui_list_set_icon_callback(gui_list, _callback) \
    (gui_list)->callback_get_item_icon=_callback

#ifdef HAVE_LCD_COLOR
/*
 * Sets the color callback function
 *  - gui_list : the list structure
 *  - _callback : the callback function
 */
#define gui_list_set_color_callback(gui_list, _callback) \
    (gui_list)->callback_get_item_color=_callback
#endif

/*
 * Gives the position of the selected item
 *  - gui_list : the list structure
 * Returns the position
 */
#define gui_list_get_sel_pos(gui_list) \
    (gui_list)->selected_item


#ifdef HAVE_LCD_BITMAP
/* parse global setting to static int */
extern void gui_list_screen_scroll_step(int ofs);

/* parse global setting to static bool */
extern void gui_list_screen_scroll_out_of_view(bool enable);
#endif /* HAVE_LCD_BITMAP */
/*
 * Tells the list wether it should stop when reaching the top/bottom
 * or should continue (by going to bottom/top)
 * - gui_list : the list structure
 * - scroll :
 *    - true : stops when reaching top/bottom
 *    - false : continues to go to bottom/top when reaching top/bottom
 */
#define gui_list_limit_scroll(gui_list, scroll) \
    (gui_list)->limit_scroll=scroll

/*
 * This part handles as many lists as there are connected screens
 * (the api is similar to the ones above)
 * The lists on the screens are synchronized ;
 * theirs items and selected items are the same, but of course,
 * they can be displayed on screens with different sizes
 * The final aim is to let the programmer handle many lists in one
 * function call and make its code independant from the number of screens
 */
struct gui_synclist
{
    struct gui_list gui_list[NB_SCREENS];
};

extern void gui_synclist_init(
    struct gui_synclist * lists,
    list_get_name callback_get_item_name,
    void * data,
    bool scroll_all,
    int selected_size
    );
extern void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items);
extern void gui_synclist_set_icon_callback(struct gui_synclist * lists, list_get_icon icon_callback);
extern int gui_synclist_get_nb_items(struct gui_synclist * lists);

extern int  gui_synclist_get_sel_pos(struct gui_synclist * lists);

extern void gui_synclist_draw(struct gui_synclist * lists);
extern void gui_synclist_select_item(struct gui_synclist * lists,
                                     int item_number);
extern void gui_synclist_add_item(struct gui_synclist * lists);
extern void gui_synclist_del_item(struct gui_synclist * lists);
extern void gui_synclist_limit_scroll(struct gui_synclist * lists, bool scroll);
extern void gui_synclist_flash(struct gui_synclist * lists);
extern void gui_synclist_set_title(struct gui_synclist * lists, char * title,
                                   int icon);
extern void gui_synclist_hide_selection_marker(struct gui_synclist *lists,
                                                bool hide);
/*
 * Do the action implied by the given button,
 * returns true if the action was handled.
 * NOTE: *action may be changed regardless of return value
 */
extern bool gui_synclist_do_button(struct gui_synclist * lists,
                                       unsigned *action,
                                       enum list_wrap);

#endif /* _GUI_LIST_H_ */
