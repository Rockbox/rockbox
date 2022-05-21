/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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
#include "logf.h"

#undef DEBUGF
#define DEBUGF(...)
//#define DEBUGF printf

#define EV_EXIT MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFF)

unsigned char stack[DEFAULT_STACK_SIZE];
struct event_queue queue;
int thread_id;
const char* state = "none";
const char* prev_state = "none";

static void main_loop(void)
{
    bool exiting = false;
    struct queue_event ev;

    while(true) {
        rb->queue_wait(&queue, &ev);

        /* events that are handled whether exiting or not */
        switch(ev.id) {
        case EV_EXIT:
            return;
        }

        if(exiting)
            continue;

        /* events handled only when not exiting */
        switch(ev.id) {
        case SYS_USB_CONNECTED:
            prev_state = state;
            state = "connected";
            logf("test_usb: connect ack %ld", *rb->current_tick);
            DEBUGF("test_usb: connect ack %ld\n", *rb->current_tick);
            rb->usb_acknowledge(SYS_USB_CONNECTED_ACK);
            break;

        case SYS_USB_DISCONNECTED:
            prev_state = state;
            state = "disconnected";
            logf("test_usb: disconnect %ld", *rb->current_tick);
            DEBUGF("test_usb: disconnect %ld\n", *rb->current_tick);
            break;

        case SYS_POWEROFF:
        case SYS_REBOOT:
            prev_state = state;
            state = "exiting";
            exiting = true;
            break;
        }
    }
}

static void kill_tsr(void)
{
    rb->queue_post(&queue, EV_EXIT, 0);
    rb->thread_wait(thread_id);
    rb->queue_delete(&queue);
}

static bool exit_tsr(bool reenter)
{
    MENUITEM_STRINGLIST(menu, "USB test menu", NULL,
                        "Status", "Stop plugin", "Back");

    while(true) {
        int result = reenter ? rb->do_menu(&menu, NULL, NULL, false) : 1;
        switch(result) {
        case 0:
            rb->splashf(HZ, "State: %s", state);
            rb->splashf(HZ, "Prev: %s", prev_state);
            break;
        case 1:
            rb->splashf(HZ, "Stopping USB test thread");
            kill_tsr();
            return true;
        case 2:
            return false;
        }
    }
}

static void run_tsr(void)
{
    rb->queue_init(&queue, true);
    thread_id = rb->create_thread(main_loop, stack, sizeof(stack),
                                  0, "test_usb TSR"
                                  IF_PRIO(, PRIORITY_BACKGROUND)
                                  IF_COP(, CPU));
    rb->plugin_tsr(exit_tsr);
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    MENUITEM_STRINGLIST(menu, "USB test menu", NULL,
                        "Start", "Quit");

    switch(rb->do_menu(&menu, NULL, NULL, false)) {
    case 0:
        run_tsr();
        rb->splashf(HZ, "Thread started");
        return PLUGIN_OK;
    case 1:
        return PLUGIN_OK;
    default:
        return PLUGIN_ERROR;
    }
}
