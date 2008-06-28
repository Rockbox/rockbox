/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "action.h"
#include "menu.h"
#include "menu_common.h"
#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
#endif

#if CONFIG_CODEC == SWCODEC
/* Use this callback if your menu adjusts DSP settings. */
int lowlatency_callback(int action, const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM: /* on entering an item */
            pcmbuf_set_low_latency(true);
            break;
        case ACTION_EXIT_MENUITEM: /* on exit */
            pcmbuf_set_low_latency(false);
            break;
    }
    return action;
}
#endif

