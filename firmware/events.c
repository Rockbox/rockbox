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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include "events.h"
#include "panic.h"

#define MAX_SYS_EVENTS 10

struct sysevent {
    unsigned short id;
    bool oneshot;
    void (*callback)(void *data);
};

static struct sysevent events[MAX_SYS_EVENTS];

bool add_event(unsigned short id, bool oneshot, void (*handler))
{
    int i;
    
    /* Check if the event already exists. */
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].callback == handler && events[i].id == id)
            return false;
    }
    
    /* Try to find a free slot. */
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].callback == NULL)
        {
            events[i].id = id;
            events[i].oneshot = oneshot;
            events[i].callback = handler;
            return true;
        }
    }
    
    panicf("event line full");
    return false;
}

void remove_event(unsigned short id, void (*handler))
{
    int i;
    
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].id == id && events[i].callback == handler)
        {
            events[i].callback = NULL;
            return;
        }
    }
    
    panicf("event %d not found", (int)id);
}

void send_event(unsigned short id, void *data)
{
    int i;
    
    for (i = 0; i < MAX_SYS_EVENTS; i++)
    {
        if (events[i].id == id && events[i].callback != NULL)
        {
            events[i].callback(data);
            
            if (events[i].oneshot)
                events[i].callback = NULL;
        }
    }
}

