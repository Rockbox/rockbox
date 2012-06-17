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
 * Copyright (C) 2011 by Thomas Martitz
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
#include "debug.h"
#include "usb.h"

static void sim_thread(void);
static long sim_thread_stack[DEFAULT_STACK_SIZE/sizeof(long)];
            /* stack isn't actually used in the sim */
static const char sim_thread_name[] = "sim";
static struct event_queue sim_queue;

/* possible events for the sim thread */
enum {
    SIM_SCREENDUMP,
    SIM_USB_INSERTED,
    SIM_USB_EXTRACTED,
};

void sim_thread(void)
{
    struct queue_event ev;
    long last_broadcast_tick = current_tick;
    int num_acks_to_expect = 0;
    
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
            case SIM_USB_INSERTED:
            /* from firmware/usb.c: */
                /* Tell all threads that they have to back off the storage.
                   We subtract one for our own thread. Expect an ACK for every
                   listener for each broadcast they received. If it has been too
                   long, the user might have entered a screen that didn't ACK
                   when inserting the cable, such as a debugging screen. In that
                   case, reset the count or else USB would be locked out until
                   rebooting because it most likely won't ever come. Simply
                   resetting to the most recent broadcast count is racy. */
                if(TIME_AFTER(current_tick, last_broadcast_tick + HZ*5))
                {
                    num_acks_to_expect = 0;
                    last_broadcast_tick = current_tick;
                }

                num_acks_to_expect += queue_broadcast(SYS_USB_CONNECTED, 0) - 1;
                DEBUGF("USB inserted. Waiting for %d acks...\n",
                       num_acks_to_expect);
                break;
            case SYS_USB_CONNECTED_ACK:
                if(num_acks_to_expect > 0 && --num_acks_to_expect == 0)
                {
                    DEBUGF("All threads have acknowledged the connect.\n");
                }
                else
                {
                    DEBUGF("usb: got ack, %d to go...\n",
                           num_acks_to_expect);
                }
                break;
            case SIM_USB_EXTRACTED:
                /* in usb.c, this is only done for exclusive storage
                 * do it here anyway but don't depend on the acks */
                queue_broadcast(SYS_USB_DISCONNECTED, 0);
                break;
            default:
                DEBUGF("sim_tasks: unhandled event: %ld\n", ev.id);
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

static bool is_usb_inserted;
void sim_trigger_usb(bool inserted)
{
    if (inserted)
        queue_post(&sim_queue, SIM_USB_INSERTED, 0);
    else
        queue_post(&sim_queue, SIM_USB_EXTRACTED, 0);
    is_usb_inserted = inserted;
}

int usb_detect(void)
{
    return is_usb_inserted ? USB_INSERTED : USB_EXTRACTED;
}

void usb_init(void)
{
}

void usb_start_monitoring(void)
{
}

void usb_acknowledge(long id)
{
    queue_post(&sim_queue, id, 0);
}

void usb_wait_for_disconnect(struct event_queue *q)
{
    struct queue_event ev;

    /* Don't return until we get SYS_USB_DISCONNECTED */
    while(1)
    {
        queue_wait(q, &ev);
        if(ev.id == SYS_USB_DISCONNECTED)
            return;
    }
}
