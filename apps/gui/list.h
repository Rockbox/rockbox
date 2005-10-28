/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kévin FERRARE
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
#define LIST_RC_NEXT   BUTTON_RC_FF
#define LIST_RC_PREV   BUTTON_RC_REW
#define LIST_RC_PGUP   BUTTON_RC_SOURCE
#define LIST_RC_PGDN   BUTTON_RC_BITRATE

#elif CONFIG_KEYPAD == RECORDER_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP
#define LIST_PGUP      (BUTTON_ON | BUTTON_UP)
#define LIST_PGDN      (BUTTON_ON | BUTTON_DOWN)
#define LIST_RC_NEXT   BUTTON_RC_RIGHT
#define LIST_RC_PREV   BUTTON_RC_LEFT

#elif CONFIG_KEYPAD == PLAYER_PAD
#define LIST_NEXT      BUTTON_RIGHT
#define LIST_PREV      BUTTON_LEFT
#define LIST_RC_NEXT   BUTTON_RC_RIGHT
#define LIST_RC_PREV   BUTTON_RC_LEFT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP

#elif CONFIG_KEYPAD == GMINI100_PAD
#define LIST_NEXT      BUTTON_DOWN
#define LIST_PREV      BUTTON_UP
#define LIST_PGUP      (BUTTON_ON | BUTTON_UP)
#define LIST_PGDN      (BUTTON_ON | BUTTON_DOWN)
#endif


struct gui_list
{
    int nb_items;
    int selected_item;
    int start_item; /* the item that is displayed at the top of the screen */

    void (*callback_get_item_icon)(int selected_item, ICON * icon);
    char * (*callback_get_item_name)(int selected_item, char *buffer);

    struct screen * display;
    int line_scroll_limit;
};

/*
 * Initializes a scrolling list
 *  - gui_list : the list structure to initialize
 *  - callback_get_item_icon : pointer to a function that associates an icon
 *    to a given item number
 *  - callback_get_item_name : pointer to a function that associates a label
 *    to a given item number
 */
extern void gui_list_init(struct gui_list * gui_list,
            void (*callback_get_item_icon)(int selected_item, ICON * icon),
            char * (*callback_get_item_name)(int selected_item, char *buffer)
            );

/*
 * Sets the numburs of items the list can currently display
 * note that the list's context like the currently pointed item is resetted
 *  - gui_list : the list structure to initialize
 *  - nb_items : the numbers of items you want
 */
extern void gui_list_set_nb_items(struct gui_list * gui_list, int nb_items);

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
 * Attach the scrolling list to a screen
 * (The previous screen attachement is lost)
 *  - gui_list : the list structure
 *  - display : the screen to attach
 */
extern void gui_list_set_display(struct gui_list * gui_list,
                                 struct screen * display);

/*
 * Gives the name of the selected object
 *  - gui_list : the list structure
 *  - buffer : a buffer which is filled with the name
 */
extern void gui_list_get_selected_item_name(struct gui_list * gui_list,
                                            char *buffer);

/*
 * Gives the position of the selected item
 *  - gui_list : the list structure
 * Returns the position
 */
extern int gui_list_get_selected_item_position(struct gui_list * gui_list);

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

extern void gui_synclist_init(struct gui_synclist * lists,
             void (*callback_get_item_icon)(int selected_item, ICON * icon),
             char * (*callback_get_item_name)(int selected_item, char *buffer)
             );
extern void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items);
extern void gui_synclist_get_selected_item_name(struct gui_synclist * lists,
                                                char *buffer);
extern int  gui_synclist_get_selected_item_position(struct gui_synclist * lists);
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
/*
 * Do the action implied by the given button,
 * returns true if something has been done, false otherwise
 *  - lists : the synchronized lists
 *  - button : the keycode of a pressed button
 */
extern bool gui_synclist_do_button(struct gui_synclist * lists, unsigned button);

#endif /* _GUI_LIST_H_ */
