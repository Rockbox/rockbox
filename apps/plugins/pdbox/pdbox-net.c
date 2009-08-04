/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Wincent Balin
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

#include "plugin.h"
#include "pdbox.h"

/* Datagram pool will contains 16 datagrams. */
#define MAX_DATAGRAMS 16

/* Datagram pool. */
struct datagram datagrams[MAX_DATAGRAMS];

/* UDP message queues. */
struct event_queue gui_to_core;
struct event_queue core_to_gui;

/* Initialize net infrastructure. */
void net_init(void)
{
    unsigned int i;

    /* Initialize message pool. */
    for(i = 0; i < MAX_DATAGRAMS; i++)
        datagrams[i].used = false;

    /* Initialize and register message queues. */
    rb->queue_init(&gui_to_core, true);
    rb->queue_init(&core_to_gui, true);
}

/* Send datagram. */
bool send_datagram(struct event_queue* route,
                   int port,
                   char* data,
                   size_t size)
{
    unsigned int i;

    /* If datagram too long, abort. */
    if(size > MAX_DATAGRAM_SIZE)
        return false;

    /* Find free datagram buffer. */
    for(i = 0; i < MAX_DATAGRAMS; i++)
        if(!datagrams[i].used)
            break;

    /* If no free buffer found, abort. */
    if(i == MAX_DATAGRAMS)
        return false;

    /* Copy datagram to the buffer. */
    memcpy(datagrams[i].data, data, size);
    datagrams[i].size = size;

    /* Mark datagram buffer as used. */
    datagrams[i].used = true;

    /* Send event via route. */
    rb->queue_post(route, port, (intptr_t) &datagrams[i]);

    /* Everything went ok. */
    return true;
}

/* Receive datagram. */
bool receive_datagram(struct event_queue* route,
                      int port,
                      struct datagram* buffer)
{
    struct queue_event event;

    /* If route queue empty, abort. */
    if(rb->queue_empty(route))
        return false;

    /* Receive event. */
    rb->queue_wait(route, &event);

    /* If wrong port, abort.
       NOTE: Event is removed from the queue in any case! */
    if(event.id != port)
        return false;

    /* Copy datagram. */
    memcpy(buffer, (struct datagram*) event.data, sizeof(struct datagram));

    /* Clear datagram event. */
    memset(((struct datagram*) event.data)->data,
           0,
           ((struct datagram*) event.data)->size);

    /* Free datagram event. */
    ((struct datagram*) event.data)->used = false;

    /* Everything went ok. */
    return true;
}

/* Destroy net infrastructure. */
void net_destroy(void)
{
    /* Remove message queues. */
    rb->queue_delete(&gui_to_core);
    rb->queue_delete(&core_to_gui);
}

