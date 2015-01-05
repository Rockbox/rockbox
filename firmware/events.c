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
    bool has_user_data;
    union {
        void (*callback)(unsigned short id, void *event_data);
        struct {
            void (*callback2)(unsigned short id, void *event_data, void *user_data);
            void *user_data;
        };
    } handler;
};

static struct sysevent events[MAX_SYS_EVENTS];

static bool do_add_event(unsigned short id, bool oneshot, bool user_data_valid,
                         void *handler, void *user_data)
{
    int i;
    
    /* Check if the event already exists. */
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].handler.callback == handler && events[i].id == id
                && (!user_data_valid || (user_data == events[i].handler.user_data)))
            return false;
    }
    
    /* Try to find a free slot. */
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].handler.callback == NULL)
        {
            events[i].id = id;
            events[i].oneshot = oneshot;
            if ((events[i].has_user_data = user_data_valid))
                events[i].handler.user_data = user_data;
            events[i].handler.callback = handler;
            return true;
        }
    }
    
    panicf("event line full");
    return false;
}

bool add_event(unsigned short id, void (*handler)(unsigned short id, void *data))
{
    return do_add_event(id, false, false, handler, NULL);
}

bool add_event_ex(unsigned short id, bool oneshot, void (*handler)(unsigned short id, void *event_data, void *user_data), void *user_data)
{
    return do_add_event(id, oneshot, true, handler, user_data);
}

static void do_remove_event(unsigned short id, bool user_data_valid,
                  void *handler, void *user_data)
{
    int i;
    
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].id == id && events[i].handler.callback == handler
                && (!user_data_valid || (user_data == events[i].handler.user_data)))
        {
            events[i].handler.callback = NULL;
            return;
        }
    }
}

void remove_event(unsigned short id, void (*handler)(unsigned short id, void *data))
{
    do_remove_event(id, false, handler, NULL);
}

void remove_event_ex(unsigned short id,
                     void (*handler)(unsigned short id, void *event_data, void *user_data),
                     void *user_data)
{
    do_remove_event(id, true, handler, user_data);
}

void send_event(unsigned short id, void *data)
{
    int i;
    
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].id == id && events[i].handler.callback != NULL)
        {
            if (events[i].has_user_data)
                events[i].handler.callback2(id, data, events[i].handler.user_data);
            else
                events[i].handler.callback(id, data);
            
            if (events[i].oneshot)
                events[i].handler.callback = NULL;
        }
    }
}
