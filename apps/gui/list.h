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

/* Key assignement */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP
#define LIST_PGUP      (BUTTON_ON | BUTTON_UP)
#define LIST_PGDN      (BUTTON_ON | BUTTON_DOWN)
#define LIST_PGRIGHT   (BUTTON_ON | BUTTON_RIGHT)
#define LIST_PGLEFT    (BUTTON_ON | BUTTON_LEFT)

#ifdef CONFIG_REMOTE_KEYPAD
#define LIST_RC_NEXT   BUTTON_RC_FF
#define LIST_RC_PREV   BUTTON_RC_REW
#define LIST_RC_PGUP   BUTTON_RC_SOURCE
#define LIST_RC_PGDN   BUTTON_RC_BITRATE
#define LIST_RC_PGRIGHT (BUTTON_RC_VOL_UP)
#define LIST_RC_PGLEFT (BUTTON_RC_VOL_DOWN)
#endif /* CONFIG_REMOTE_KEYPAD */

#elif CONFIG_KEYPAD == RECORDER_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP
#define LIST_PGUP      (BUTTON_ON | BUTTON_UP)
#define LIST_PGDN      (BUTTON_ON | BUTTON_DOWN)
#define LIST_PGRIGHT   (BUTTON_ON | BUTTON_RIGHT)
#define LIST_PGLEFT    (BUTTON_ON | BUTTON_LEFT)

#ifdef CONFIG_REMOTE_KEYPAD
#define LIST_RC_NEXT   BUTTON_RC_RIGHT
#define LIST_RC_PREV   BUTTON_RC_LEFT
#endif /* CONFIG_REMOTE_KEYPAD */

#elif CONFIG_KEYPAD == PLAYER_PAD
#define LIST_NEXT      BUTTON_RIGHT
#define LIST_PREV      BUTTON_LEFT

#ifdef CONFIG_REMOTE_KEYPAD
#define LIST_RC_NEXT   BUTTON_RC_RIGHT
#define LIST_RC_PREV   BUTTON_RC_LEFT
#endif /* CONFIG_REMOTE_KEYPAD */

#elif CONFIG_KEYPAD == ONDIO_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP
#define LIST_PGRIGHT   (BUTTON_MENU | BUTTON_RIGHT)
#define LIST_PGLEFT    (BUTTON_MENU | BUTTON_LEFT)

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define LIST_NEXT      BUTTON_SCROLL_FWD
#define LIST_PREV      BUTTON_SCROLL_BACK

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP
  
#elif CONFIG_KEYPAD == GMINI100_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP
#define LIST_PGUP      (BUTTON_ON | BUTTON_UP)
#define LIST_PGDN      (BUTTON_ON | BUTTON_DOWN)
#define LIST_PGRIGHT   (BUTTON_ON | BUTTON_RIGHT)
#define LIST_PGLEFT    (BUTTON_ON | BUTTON_LEFT)

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP

#endif

/*
 * The gui_list is based on callback functions, if you want the list
 * to display something you have to provide it a function that
 * tells it what to display.
 * There are two callback function :
 * one to get the text and one to get the icon
 */

/*
 * Icon callback
 *  - selected_item : an integer that tells the number of the item to display
 *  - data : a void pointer to the data you gave to the list when
 *           you initialized it
 *  - icon : a pointer to the icon, the value inside it is used to display
 *           the icon after the function returns.
 * Note : we use the ICON type because the real type depends of the plateform
 */
typedef void list_get_icon(int selected_item,
                           void * data,
                           ICON * icon);
/*
 * Text callback
 *  - selected_item : an integer that tells the number of the item to display
 *  - data : a void pointer to the data you gave to the list when
 *           you initialized it
 *  - buffer : a buffer to put the resulting text on it
 *             (The content of the buffer may not be used by the list, we use
 *             the return value of the function in all cases to avoid filling
 *             a buffer when it's not necessary)
 * Returns a pointer to a string that contains the text to display
 */
typedef char * list_get_name(int selected_item,
                             void * data,
                             char *buffer);

struct gui_list
{
    int nb_items;
    int selected_item;
    bool cursor_flash_state;
    int start_item; /* the item that is displayed at the top of the screen */

#ifdef HAVE_LCD_BITMAP
    int offset_position; /* the list's screen scroll placement in pixels */
#endif
    list_get_icon *callback_get_item_icon;
    list_get_name *callback_get_item_name;

    struct screen * display;
    /* defines wether the list should stop when reaching the top/bottom
     * or should continue (by going to bottom/top) */
    bool limit_scroll;
    /* The data that will be passed to the callback function YOU implement */
    void * data;
};

/*
 * Initializes a scrolling list
 *  - gui_list : the list structure to initialize
 *  - callback_get_item_icon : pointer to a function that associates an icon
 *    to a given item number
 *  - callback_get_item_name : pointer to a function that associates a label
 *    to a given item number
 *  - data : extra data passed to the list callback
 */
extern void gui_list_init(struct gui_list * gui_list,
    list_get_name callback_get_item_name,
    void * data
    );

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
 * Puts the selection in the screen
 *  - gui_list : the list structure
 *  - put_from_end : if true, selection will be put as close from
 *                   the end of the list as possible, else, it's
 *                   from the beginning
 */
extern void gui_list_put_selection_in_screen(struct gui_list * gui_list,
                                             bool put_from_end);

/*
 * Sets the icon callback function
 *  - gui_list : the list structure
 *  - _callback : the callback function
 */
#define gui_list_set_icon_callback(gui_list, _callback) \
    (gui_list)->callback_get_item_icon=_callback

/*
 * Attach the scrolling list to a screen
 * (The previous screen attachement is lost)
 *  - gui_list : the list structure
 *  - display : the screen to attach
 */
extern void gui_list_set_display(struct gui_list * gui_list,
                                 struct screen * display);

/*
 * Gives the position of the selected item
 *  - gui_list : the list structure
 * Returns the position
 */
#define gui_list_get_sel_pos(gui_list) \
    (gui_list)->selected_item


/*
 * Selects an item in the list
 *  - gui_list : the list structure
 *  - item_number : the number of the item which will be selected
 */
extern void gui_list_select_item(struct gui_list * gui_list, int item_number);

/*
 * Draws the list on the attached screen
 * - gui_list : the list structure
 */
extern void gui_list_draw(struct gui_list * gui_list);

/*
 * Selects the next item in the list
 * (Item 0 gets selected if the end of the list is reached)
 * - gui_list : the list structure
 */
extern void gui_list_select_next(struct gui_list * gui_list);

/*
 * Selects the previous item in the list
 * (Last item in the list gets selected if the list beginning is reached)
 * - gui_list : the list structure
 */
extern void gui_list_select_previous(struct gui_list * gui_list);

#ifdef HAVE_LCD_BITMAP
/*
 * Makes all the item in the list scroll by one step to the right.
 * Should stop increasing the value when reaching the widest item value 
 * in the list.
 */
void gui_list_scroll_right(struct gui_list * gui_list);

/*
 * Makes all the item in the list scroll by one step to the left.
 * stops at starting position.
 */
void gui_list_scroll_left(struct gui_list * gui_list);

/* parse global setting to static int */
extern void gui_list_screen_scroll_step(int ofs);

/* parse global setting to static bool */
extern void gui_list_screen_scroll_out_of_view(bool enable);
#endif /* HAVE_LCD_BITMAP */

/*
 * Go to next page if any, else selects the last item in the list
 * - gui_list : the list structure
 * - nb_lines : the number of lines to try to move the cursor
 */
extern void gui_list_select_next_page(struct gui_list * gui_list,
                                      int nb_lines);

/*
 * Go to previous page if any, else selects the first item in the list
 * - gui_list : the list structure
 * - nb_lines : the number of lines to try to move the cursor
 */
extern void gui_list_select_previous_page(struct gui_list * gui_list,
                                          int nb_lines);

/*
 * Adds an item to the list (the callback will be asked for one more item)
 * - gui_list : the list structure
 */
extern void gui_list_add_item(struct gui_list * gui_list);

/*
 * Removes an item to the list (the callback will be asked for one less item)
 * - gui_list : the list structure
 */
extern void gui_list_del_item(struct gui_list * gui_list);

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
 * One call on 2, the selected lune will either blink the cursor or
 * invert/display normal the selected line
 * - gui_list : the list structure
 */
extern void gui_list_flash(struct gui_list * gui_list);


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
    void * data
    );
extern void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items);
extern void gui_synclist_set_icon_callback(struct gui_synclist * lists, list_get_icon icon_callback);
#define gui_synclist_get_nb_items(lists) \
    gui_list_get_nb_items(&((lists)->gui_list[0]))

extern int  gui_synclist_get_sel_pos(struct gui_synclist * lists);

#define gui_synclist_get_sel_pos(lists) \
    gui_list_get_sel_pos(&((lists)->gui_list[0]))

extern void gui_synclist_draw(struct gui_synclist * lists);
extern void gui_synclist_select_item(struct gui_synclist * lists,
                                     int item_number);
extern void gui_synclist_select_next(struct gui_synclist * lists);
extern void gui_synclist_select_previous(struct gui_synclist * lists);
extern void gui_synclist_select_next_page(struct gui_synclist * lists,
                                          enum screen_type screen);
extern void gui_synclist_select_previous_page(struct gui_synclist * lists,
                                              enum screen_type screen);
extern void gui_synclist_add_item(struct gui_synclist * lists);
extern void gui_synclist_del_item(struct gui_synclist * lists);
extern void gui_synclist_limit_scroll(struct gui_synclist * lists, bool scroll);
extern void gui_synclist_flash(struct gui_synclist * lists);
void gui_synclist_scroll_right(struct gui_synclist * lists);
void gui_synclist_scroll_left(struct gui_synclist * lists);

/*
 * Do the action implied by the given button,
 * returns the action taken if any, 0 else
 *  - lists : the synchronized lists
 *  - button : the keycode of a pressed button
 * returned value :
 *  - LIST_NEXT when moving forward (next item or pgup)
 *  - LIST_PREV when moving backward (previous item or pgdown)
 */
extern unsigned gui_synclist_do_button(struct gui_synclist * lists, unsigned button);

#endif /* _GUI_LIST_H_ */
