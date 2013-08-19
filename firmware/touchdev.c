/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Jean-Louis Biasini
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

#include <stdbool.h>
#include "touchdev.h"

static bool disable_on_hold = true;
static bool last_hold_state = false;

void touchdev_disable_dev_on_hold(bool en)
{
    disable_on_hold = en;
    if(!en)
        touchdev_enable(true); /* make sure we enable back ! */
}

void touchdev_do_hold(bool hold_state)
{
    if(last_hold_state != hold_state)
    {
        last_hold_state = hold_state;
        if(disable_on_hold)
            touchdev_enable(!hold_state);
    }
}
