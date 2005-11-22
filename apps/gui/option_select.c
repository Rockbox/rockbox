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

void option_select_init_numeric(struct option_select * opt,
                                const char * title,
                                int init_value,
                                int min_value,
                                int max_value,
                                int step,
                                const char * unit,
                                option_formatter *formatter)
{
    opt->title=title;
    opt->min_value=min_value;
    opt->max_value=max_value+1;
    opt->option=init_value;
    opt->step=step;
    opt->extra_string=unit;
    opt->formatter=formatter;
    opt->items=NULL;
    opt->limit_loop=false;
}

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
    opt->step=1;
    opt->formatter=NULL;
    opt->items=items;
    opt->limit_loop=false;
}

void option_select_next(struct option_select * opt)
{
    if(opt->option + opt->step >= opt->max_value)
    {
        if(!opt->limit_loop)
        {
            if(opt->option==opt->max_value-1)
                opt->option=opt->min_value;
            else
                opt->option=opt->max_value-1;
        }
    }
    else
        opt->option+=opt->step;
}

void option_select_prev(struct option_select * opt)
{
    if(opt->option - opt->step < opt->min_value)
    {
        if(!opt->limit_loop)
        {
            if(opt->option==opt->min_value)
                opt->option=opt->max_value-1;
            else
                opt->option=opt->min_value;
        }
    }
    else
        opt->option-=opt->step;
}

const char * option_select_get_text(struct option_select * opt, char * buffer)
{
    if(opt->items)
        return(P2STR(opt->items[opt->option].string));
    if(!opt->formatter)
        snprintf(buffer, sizeof buffer,"%d %s", opt->option, opt->extra_string);
    else
        opt->formatter(buffer, sizeof buffer, opt->option, opt->extra_string);
    return(buffer);
}
