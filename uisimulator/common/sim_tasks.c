/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Jens Arnold
 *
 * Rockbox simulator specific tasks
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
#include "kernel.h"
#include "screendump.h"
#include "thread.h"

static void sim_thread(void);
static long sim_thread_stack[DEFAULT_STACK_SIZE/sizeof(long)];
            /* stack isn't actually used in the sim */
static const char sim_thread_name[] = "sim";
static struct event_queue sim_queue;

/* possible events for the sim thread */
enum {
    SIM_SCREENDUMP,
};

void sim_thread(void)
{
    struct queue_event ev;
    
    while (1)
    {
        queue_wait(&sim_queue, &ev);
        switch(ev.id)
        {
            case SIM_SCREENDUMP:
                screen_dump();
#ifdef HAVE_REMOTE_LCD
                remote_screen_dump();
#endif
                break;
        }
    }
}

void sim_tasks_init(void)
{
    queue_init(&sim_queue, false);
    
    create_thread(sim_thread, sim_thread_stack, sizeof(sim_thread_stack), 0,
                  sim_thread_name IF_PRIO(,PRIORITY_BACKGROUND) IF_COP(,CPU));
}

void sim_trigger_screendump(void)
{
    queue_post(&sim_queue, SIM_SCREENDUMP, 0);
}
