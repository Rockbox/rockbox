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

bool option_screen(struct settings_list *setting,
                   bool use_temp_var, unsigned char* option_title);

struct option_select
{
    const char * title;
    int min_value;
    int max_value;
    int option;
    const struct opt_items * items;
};

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
 * Returns the selected option
 */
extern const char * option_select_get_text(struct option_select * opt);

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

#endif /* _GUI_OPTION_SELECT_H_ */
