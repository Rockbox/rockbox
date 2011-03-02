/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Thomas Jarosch
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
#include "powermgmt.h"
#include "thread.h"
#include "kernel.h"

static char power_stack[DEFAULT_STACK_SIZE];
static const char power_thread_name[] = "power";

void powermgmt_init_target(void);

#if !(CONFIG_PLATFORM & PLATFORM_ANDROID)
void powermgmt_init_target(void)
{
    /* Nothing to do */
}
#endif

static void power_thread(void)
{
    powermgmt_init_target();

    while (1)
    {
        /* Sleep two seconds */
        sleep(HZ*2);

        handle_sleep_timer();
    }
} /* power_thread */

void powermgmt_init(void)
{
    create_thread(power_thread, power_stack, sizeof(power_stack), 0,
                  power_thread_name IF_PRIO(, PRIORITY_SYSTEM)
                  IF_COP(, CPU));
}
