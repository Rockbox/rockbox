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

#ifndef _GUI_OPTION_SELECT_H_
#define _GUI_OPTION_SELECT_H_
#include "settings.h"

typedef void option_formatter(char* dest, int dest_length,
                              int variable, const char* unit);

struct option_select
{
    const char * title;
    int min_value;
    int max_value;
    int step;
    int option;
    const char * extra_string;
    /* In the case the option is a number */
    option_formatter *formatter;
    const struct opt_items * items;
    bool limit_loop;
};

/*
 * Initializes an option containing a numeric values
 *  - title : the title of the option
 *  - init_value : the initial value the number will be
 *  - min_value, max_value : bounds to the value
 *  - step : the ammount you want to add / withdraw to the initial number
 *           each time a key is pressed
 *  - unit : the unit in which the value is  (ex "s", "bytes", ...)
 *  - formatter : a callback function that generates a string
 *                from the number it gets
 */
extern void option_select_init_numeric(struct option_select * opt,
                                       const char * title,
                                       int init_value,
                                       int min_value,
                                       int max_value,
                                       int step,
                                       const char * unit,
                                       option_formatter *formatter);

/*
 * Initializes an option containing a list of choices
 *  - title : the title of the option
 *  - selected : the initially selected item
 *  - items : the list of items, defined in settings.h
 *  - nb_items : the number of items in the 'items' list
 */
extern void option_select_init_items(struct option_select * opt,
                                     const char * title,
                                     int selected,
                                     const struct opt_items * items,
                                     int nb_items);

/*
 * Gets the selected option
 *  - opt : the option struct
 *  - buffer : a buffer to eventually format the option
 * Returns the selected option
 */
extern const char * option_select_get_text(struct option_select * opt, char * buffer);

/*
 * Selects the next value
 *  - opt : the option struct
 */
extern void option_select_next(struct option_select * opt);

/*
 * Selects the previous value
 *  - opt : the option struct
 */
extern void option_select_prev(struct option_select * opt);

/*
 * Returns the selected number
 *  - opt : the option struct
 */
#define option_select_get_selected(_opt) \
    (_opt)->option

/*
 * Returns the title
 *  - opt : the option struct
 */
#define option_select_get_title(_opt) \
    (_opt)->title

/*
 * Tells the option selector wether it should stop when reaching the min/max value
 * or should continue (by going to max/min)
 *  - opt : the option struct
 *  - scroll :
 *    - true : stops when reaching min/max
 *    - false : continues to go to max/min when reaching min/max
 */
#define option_select_limit_loop(_opt, loop) \
    (_opt)->limit_loop=loop

#endif /* _GUI_OPTION_SELECT_H_ */
