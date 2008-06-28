/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
 *
 * Gigabeat S GPIO interrupt event descriptions
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
#include "config.h"
#include "system.h"
#include "gpio-imx31.h"

/* Gigabeat S definitions for static GPIO event registration */

/* Describes single events for each GPIO1 pin */
static const struct gpio_event gpio1_events[] =
{
    [MC13783_EVENT_ID-GPIO1_EVENT_FIRST] =
    {
        .mask     = 1 << MC13783_GPIO_LINE,
        .sense    = GPIO_SENSE_RISING,
        .callback = mc13783_event,
    }
};

/* Describes the events attached to GPIO1 port */
const struct gpio_event_list gpio1_event_list =
{
    .ints_priority = 7,
    .count         = ARRAYLEN(gpio1_events),
    .events        = gpio1_events,
};
