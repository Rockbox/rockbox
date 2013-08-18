/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
#include "kernel.h"
#include "button.h"
#include "scroll_engine.h" /* for scroll_do_step() */

static struct event_queue button_queue SHAREDBSS_ATTR;
static intptr_t button_data; /* data value from last message dequeued */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static void button_boost(bool state)
{
    static bool boosted = false;
    
    if (state && !boosted)
    {
        cpu_boost(true);
        boosted = true;
    }
    else if (!state && boosted)
    {
        cpu_boost(false);
        boosted = false;
    }
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */

void button_queue_post(long id, intptr_t data)
{
    queue_post(&button_queue, id, data);
}

void button_queue_post_remove_head(long id, intptr_t data)
{
    queue_remove_from_head(&button_queue, id);
    queue_post(&button_queue, id, data);

}

bool button_queue_try_post(long button, int data)
{
#ifdef HAVE_TOUCHSCREEN
    /* one can swipe over the scren very quickly,
     * for this to work we want to forget about old presses and
     * only respect the very latest ones */
    const bool force_post = true;
#else
    /* Only post events if the queue is empty,
     * to avoid afterscroll effects.
     * i.e. don't post new buttons if previous ones haven't been
     * processed yet - but always post releases */
    const bool force_post = button & BUTTON_REL;
#endif

    bool ret = queue_empty(&button_queue);
    if (!ret && force_post)
    {
        queue_remove_from_head(&button_queue, button);
        ret = true;
    }

    if (ret)
        queue_post(&button_queue, button, data);

    /* on touchscreen we posted unconditionally */
    return ret;
}

int button_queue_count(void)
{
    return queue_count(&button_queue);
}

bool button_queue_empty(void)
{
    return queue_empty(&button_queue);
}

void button_clear_queue(void)
{
    queue_clear(&button_queue);
}

long button_get_w_tmo(int ticks)
{
    struct queue_event ev;

    long tick = current_tick;
    long exit_tick = tick + ticks;

    for (;;)
    {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        /* Control the CPU boost trying to keep queue empty. */
        int pending_count = queue_count(&button_queue);
        if (pending_count == 0)
            button_boost(false);
        else if (pending_count > 2)
            button_boost(true);
#endif
        long wake_tick = scroll_do_step();
        if (ticks >= 0 && TIME_BEFORE(exit_tick, wake_tick))
            wake_tick = exit_tick;

        int delay = wake_tick - tick;
        queue_wait_w_tmo(&button_queue, &ev, MAX(delay, 0));

        if (ev.id != SYS_TIMEOUT)
        {
            button_data = ev.data;
            return ev.id;
        }
        else if (ticks >= 0 && !TIME_BEFORE(tick, exit_tick))
        {
            return BUTTON_NONE;
        }

        tick = current_tick;
    }
}

long button_get(bool block)
{
    return button_get_w_tmo(block ? TIMEOUT_BLOCK : TIMEOUT_NOBLOCK);
}

intptr_t button_get_data(void)
{
    return button_data;
}

void button_queue_init(void)
{
    queue_init(&button_queue, true);
}
