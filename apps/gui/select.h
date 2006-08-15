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

#ifndef _GUI_SELECT_H_
#define _GUI_SELECT_H_
#include "screen_access.h"
#include "settings.h"
#include "option_select.h"

struct gui_select
{
    bool canceled;
    bool validated;
    struct option_select options;
};

/*
 * Initializes a select that let's you choose between several numeric values
 *  - title : the title of the select
 *  - init_value : the initial value the number will be
 *  - min_value, max_value : bounds to the value
 *  - step : the ammount you want to add / withdraw to the initial number
 *           each time a key is pressed
 *  - unit : the unit in which the value is  (ex "s", "bytes", ...)
 *  - formatter : a callback function that generates a string
 *                from the number it gets
 */
extern void gui_select_init_numeric(struct gui_select * select,
        const char * title,
        int init_value,
        int min_value,
        int max_value,
        int step,
        const char * unit,
        option_formatter *formatter);


/*
 * Initializes a select that let's you choose between options in a list
 *  - title : the title of the select
 *  - selected : the initially selected item
 *  - items : the list of items, defined in settings.h
 *  - nb_items : the number of items in the 'items' list
 */
extern void gui_select_init_items(struct gui_select * select,
        const char * title,
        int selected,
        const struct opt_items * items,
        int nb_items
        );

/*
 * Draws the select on the given screen
 *  - select : the select struct
 *  - display : the display on which you want to output
 */
extern void gui_select_draw(struct gui_select * select, struct screen * display);

/*
 * Draws the select on all the screens
 *  - select : the select struct
 */
extern void gui_syncselect_draw(struct gui_select * select);

/*
 * Handles key events for a synced (drawn on all screens) select
 *  - select : the select struct
 */
extern bool gui_syncselect_do_button(struct gui_select * select, int button);

#endif /* _GUI_SELECT_H_ */
