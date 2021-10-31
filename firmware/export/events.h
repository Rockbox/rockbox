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

#ifndef _EVENTS_H
#define _EVENTS_H

#include <stdbool.h>
/**
 * Synchronouos event system.
 *
 * Callbacks are subscribed with add_event() or add_event_ex(). events
 * are fired using send_event().
 *
 * Events are always dispatched synchronously: the callbacks are called
 * in the thread context of the event sender, without context switch. This
 * also means that callbacks should be as simple as possible to avoid
 * blocking the sender and other callbacks
 *
 * Use the kernel-level event_queue for cross-thread event dispatching.
 * */

/*
 * Only CLASS defines and firmware/ level events should be defined here.
 * apps/ level events are defined in apps/appevents.h
 * 
 * High byte = Event class definition
 * Low byte  = Event ID
 */

#define EVENT_CLASS_DISK       0x0100
#define EVENT_CLASS_PLAYBACK   0x0200
#define EVENT_CLASS_BUFFERING  0x0400
#define EVENT_CLASS_GUI        0x0800
#define EVENT_CLASS_RECORDING  0x1000
#define EVENT_CLASS_LCD        0x2000
#define EVENT_CLASS_VOICE      0x4000
#define EVENT_CLASS_SYSTEM     0x8000 /*LAST ONE */
/**
 * Subscribe to an event with a simple callback. The callback will be called
 * synchronously everytime the event fires, passing the event id and data to
 * the callback.
 *
 * Must be removed with remove_event(). 
 */
bool add_event(unsigned short id, void (*handler)(unsigned short id, void *event_data));

/**
 * Subscribe to an event with a detailed callback. The callback will be called
 * synchronously everytime the event fires, passing the event id and data, as
 * well as the user_data pointer passed here, to the callback.
 *
 * With oneshot == true, the callback is unsubscribed automatically after
 * the event fired for the first time. In this case the event need not to be
 * removed with remove_event_ex().
 *
 * Must be removed with remove_event_ex(). remove_event() will never remove
 * events added with this function. 
 */
bool add_event_ex(unsigned short id, bool oneshot, void (*handler)(unsigned short id, void *event_data, void *user_data), void *user_data);

/**
 * Unsubscribe a callback from an event. The handler pointer is matched.
 *
 * This will only work for subscriptions made with add_event().
 */
void remove_event(unsigned short id, void (*handler)(unsigned short id, void *data));

/**
 * Unsubscribe a callback from an event. The handler and user_data pointers
 * are matched. That means the same user_data that was passed to add_event_ex()
 * must be passed to this too.
 *
 * This will only work for subscriptions made with add_event_ex().
 */
void remove_event_ex(unsigned short id, void (*handler)(unsigned short id, void *event_data, void *user_data), void *user_data);

/**
 * Fire an event, which synchronously calls all subscribed callbacks. The
 * event id and data pointer are passed to the callbacks as well, and
 * optionally the user_data pointer from add_event_ex().
 */
void send_event(unsigned short id, void *data);

/** System events **/
enum {
    /* USB_INSERTED
       data = &usbmode */
    SYS_EVENT_USB_INSERTED = (EVENT_CLASS_SYSTEM|1),
    /* USB_EXTRACTED
       data = NULL */
    SYS_EVENT_USB_EXTRACTED,
};

#endif
