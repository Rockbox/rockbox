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
#ifdef HAVE_SDL
#include "SDL.h"
#include "window-sdl.h"
#endif

static struct event_queue button_queue SHAREDBSS_ATTR;
static intptr_t button_data; /* data value from last message dequeued */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static bool button_boosted = false;
static long button_unboost_tick;
#define BUTTON_UNBOOST_TMO HZ

static void button_boost(bool state)
{
    if (state)
    {
        /* update the unboost time each button_boost(true) call */
        button_unboost_tick = current_tick + BUTTON_UNBOOST_TMO;

        if (!button_boosted)
        {
            button_boosted = true;
            cpu_boost(true);
        }
    }
    else if (button_boosted && TIME_AFTER(current_tick, button_unboost_tick))
    {
        button_boosted = false;
        cpu_boost(false);
    }
}

static void button_queue_wait(struct queue_event *evp, int timeout)
{
    /* Loop once after wait time if boosted in order to unboost and wait the
       full remaining time */
    do
    {
        int ticks = timeout;

        if (ticks == TIMEOUT_NOBLOCK)
            ;
        else if (ticks > 0)
        {
            if (button_boosted && ticks > BUTTON_UNBOOST_TMO)
                ticks = BUTTON_UNBOOST_TMO;

            timeout -= ticks;
        }
        else if (button_boosted) /* TIMEOUT_BLOCK (ticks < 0) */
        {
            ticks = BUTTON_UNBOOST_TMO;
        }

        queue_wait_w_tmo(&button_queue, evp, ticks);
        if (evp->id != SYS_TIMEOUT)
        {
            /* GUI boost build gets immediate kick, otherwise at least 3
               messages had to be there */
        #ifndef HAVE_GUI_BOOST
            if (queue_count(&button_queue) >= 2)
        #endif
                button_boost(true);

            break;
        }
        button_boost(false);
    }
    while (timeout);
}
#else /* ndef HAVE_ADJUSTABLE_CPU_FREQ */
static inline void button_queue_wait(struct queue_event *evp, int timeout)
{
#if defined(__APPLE__) && (CONFIG_PLATFORM & PLATFORM_SDL)
    unsigned long initial_tick = current_tick;
    unsigned long curr_tick, remaining;
    while(true)
    {
        SDL_PumpEvents();
        queue_wait_w_tmo(&button_queue, evp, TIMEOUT_NOBLOCK);
        if (evp->id != SYS_TIMEOUT || timeout == TIMEOUT_NOBLOCK)
            return;
        else if (timeout == TIMEOUT_BLOCK)
            sleep(HZ/60);
        else
        {
            curr_tick = current_tick;
            if (!TIME_AFTER(initial_tick + timeout, curr_tick))
                return;
            remaining = ((initial_tick + timeout) - curr_tick);
            sleep(remaining < HZ/60 ? remaining : HZ/60);
        }
    }
#else
    queue_wait_w_tmo(&button_queue, evp, timeout);
#ifdef HAVE_SDL
    sdl_window_adjust(); /* Window may have been resized */
#endif
#endif
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

    if (!queue_empty(&button_queue))
    {
        if (force_post)
            queue_remove_from_head(&button_queue, button);
        else
            return false;
    }

    queue_post(&button_queue, button, data);

    /* on touchscreen we posted unconditionally */
    return true;
}

int button_queue_count(void)
{
    return queue_count(&button_queue);
}

bool button_queue_empty(void)
{
    return queue_empty(&button_queue);
}

bool button_queue_full(void)
{
    return queue_full(&button_queue);
}

void button_clear_queue(void)
{
    queue_clear(&button_queue);
}

/* clears anything but release and sysevents */
void button_clear_pressed(void)
{
    long button;
    for (int count = queue_count(&button_queue); count > 0; count--)
    {
        button = button_get(false);
        if (button & (BUTTON_REL | SYS_EVENT))
        {
            button_queue_post(button, button_data);
        }
    }
}

long button_get_w_tmo(int ticks)
{
    struct queue_event ev;
    button_queue_wait(&ev, ticks);

    if (ev.id == SYS_TIMEOUT)
        ev.id = BUTTON_NONE;
    else
        button_data = ev.data;

    return ev.id;
}

long button_get(bool block)
{
    return button_get_w_tmo(block ? TIMEOUT_BLOCK : TIMEOUT_NOBLOCK);
}

intptr_t button_get_data(void)
{
    return button_data;
}

void INIT_ATTR button_queue_init(void)
{
    queue_init(&button_queue, true);
}
