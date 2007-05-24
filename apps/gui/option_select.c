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

#include "option_select.h"
#include "sprintf.h"
#include "kernel.h"
#include "lang.h"

void option_select_init_items(struct option_select * opt,
                              const char * title,
                              int selected,
                              const struct opt_items * items,
                              int nb_items)
{
    opt->title=title;
    opt->min_value=0;
    opt->max_value=nb_items;
    opt->option=selected;
    opt->items=items;
}

void option_select_next(struct option_select * opt)
{
    if(opt->option + 1 >= opt->max_value)
    {
            if(opt->option==opt->max_value-1)
                opt->option=opt->min_value;
            else
                opt->option=opt->max_value-1;
    }
    else
        opt->option+=1;
}

void option_select_prev(struct option_select * opt)
{
    if(opt->option - 1 < opt->min_value)
    {
        /* the dissimilarity to option_select_next() arises from the 
         * sleep timer problem (bug #5000 and #5001): 
         * there we have min=0, step = 5 but the value itself might
         * not be a multiple of 5 -- as time elapsed;
         * We need to be able to set timer to 0 (= Off) nevertheless. */
        if(opt->option!=opt->min_value)
            opt->option=opt->min_value;
        else
            opt->option=opt->max_value-1;
    }
    else
        opt->option-=1;
}

const char * option_select_get_text(struct option_select * opt/*, char * buffer,
                                    int buffersize*/)
{
        return(P2STR(opt->items[opt->option].string));
}
