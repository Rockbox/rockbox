/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Miika Pekkarinen
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

#include <stdio.h>
#include "events.h"
#include "panic.h"

#define MAX_SYS_EVENTS 28

struct sysevent {
    unsigned short id;
    bool oneshot;
    union {
        void (*cb)(unsigned short id, void *event_data);
        void (*cb_ex)(unsigned short id, void *event_data, void *user_data);
    } handler;
    void *user_data;
};

static struct sysevent events[MAX_SYS_EVENTS];
static int invalid_userdata;

static bool do_add_event(unsigned short id, bool oneshot,
                         void *handler, void *user_data)
{
    size_t free = MAX_SYS_EVENTS;
    struct sysevent *ev;
    /* Check if the event already exists. & lowest free slot available */
    for (size_t i = MAX_SYS_EVENTS - 1; i < MAX_SYS_EVENTS; i--)
    {
        ev = &events[i];
        if (ev->handler.cb == NULL)
            free = i;

        if (ev->id == id && ev->handler.cb == handler && user_data == ev->user_data)
        {
            return false;
        }
    }

    /* is there a free slot? */
    if (free < MAX_SYS_EVENTS)
    {
        ev = &events[free];
        ev->id = id;
        ev->handler.cb = handler;
        ev->user_data = user_data;
        ev->oneshot = oneshot;

        return true;
    }

    panicf("event line full");
    return false;
}

bool add_event(unsigned short id, void (*handler)(unsigned short id, void *data))
{
    return do_add_event(id, false, handler, &invalid_userdata);
}

bool add_event_ex(unsigned short id, bool oneshot,
                  void (*handler)(unsigned short id, void *event_data, void *user_data), void *user_data)
{
    return do_add_event(id, oneshot, handler, user_data);
}

static void do_remove_event(unsigned short id, void *handler, void *user_data)
{
    for (size_t i = 0; i < MAX_SYS_EVENTS; i++)
    {
        struct sysevent *ev = &events[i];
        if (ev->id == id && ev->handler.cb == handler && user_data == ev->user_data)
        {
            ev->handler.cb = NULL;
            return;
        }
    }
}

void remove_event(unsigned short id, void (*handler)(unsigned short id, void *data))
{
    do_remove_event(id, handler, &invalid_userdata);
}

void remove_event_ex(unsigned short id,
                     void (*handler)(unsigned short id, void *event_data, void *user_data),
                     void *user_data)
{
    do_remove_event(id, handler, user_data);
}

void send_event(unsigned short id, void *data)
{
    for (size_t i = 0; i < MAX_SYS_EVENTS; i++)
    {
        struct sysevent *ev = &events[i];
        if (ev->id == id && ev->handler.cb != NULL)
        {
            if (ev->user_data != &invalid_userdata)
            {
                ev->handler.cb_ex(id, data, ev->user_data);
                if (ev->oneshot) /* only _ex events have option of oneshot */
                    ev->handler.cb = NULL;
            }
            else
                ev->handler.cb(id, data);
        }
    }
}
