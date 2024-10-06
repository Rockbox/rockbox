/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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

#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include "config.h"
#include "thread.h"

/* System defined message ID's - |sign bit = 1|class|id| */
/* Event class list */
#define SYS_EVENT_CLS_QUEUE   0
#define SYS_EVENT_CLS_USB     1
#define SYS_EVENT_CLS_POWER   2
#define SYS_EVENT_CLS_FILESYS 3
#define SYS_EVENT_CLS_PLUG    4
#define SYS_EVENT_CLS_MISC    5
#define SYS_EVENT_CLS_PRIVATE 7 /* For use inside plugins */
/* make sure SYS_EVENT_CLS_BITS has enough range */

/* Bit 31->|S|c...c|i...i| */
#define SYS_EVENT                 ((long)(int)(1u << 31))
#define SYS_EVENT_CLS_BITS        (3)
#define SYS_EVENT_CLS_SHIFT       (31-SYS_EVENT_CLS_BITS)
#define SYS_EVENT_CLS_MASK        (((1l << SYS_EVENT_CLS_BITS)-1) << SYS_EVENT_SHIFT)
#define MAKE_SYS_EVENT(cls, id)   (SYS_EVENT | ((long)(cls) << SYS_EVENT_CLS_SHIFT) | (long)(id))
/* Macros for extracting codes */
#define SYS_EVENT_CLS(e)          (((e) & SYS_EVENT_CLS_MASK) >> SYS_EVENT_SHIFT)
#define SYS_EVENT_ID(e)           ((e) & ~(SYS_EVENT|SYS_EVENT_CLS_MASK))

#define SYS_TIMEOUT               MAKE_SYS_EVENT(SYS_EVENT_CLS_QUEUE, 0)
#define SYS_USB_CONNECTED         MAKE_SYS_EVENT(SYS_EVENT_CLS_USB, 0)
#define SYS_USB_CONNECTED_ACK     MAKE_SYS_EVENT(SYS_EVENT_CLS_USB, 1)
#define SYS_USB_DISCONNECTED      MAKE_SYS_EVENT(SYS_EVENT_CLS_USB, 2)
#define SYS_USB_LUN_LOCKED        MAKE_SYS_EVENT(SYS_EVENT_CLS_USB, 4)
#define SYS_USB_READ_DATA         MAKE_SYS_EVENT(SYS_EVENT_CLS_USB, 5)
#define SYS_USB_WRITE_DATA        MAKE_SYS_EVENT(SYS_EVENT_CLS_USB, 6)
#define SYS_POWEROFF              MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 0)
#define SYS_CHARGER_CONNECTED     MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 1)
#define SYS_CHARGER_DISCONNECTED  MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 2)
#define SYS_BATTERY_UPDATE        MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 3)
#define SYS_REBOOT                MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 4)
#define SYS_FS_CHANGED            MAKE_SYS_EVENT(SYS_EVENT_CLS_FILESYS, 0)
#define SYS_HOTSWAP_INSERTED      MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 0)
#define SYS_HOTSWAP_EXTRACTED     MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 1)
#define SYS_PHONE_PLUGGED         MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 2)
#define SYS_PHONE_UNPLUGGED       MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 3)
#define SYS_REMOTE_PLUGGED        MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 4)
#define SYS_REMOTE_UNPLUGGED      MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 5)
#define SYS_LINEOUT_PLUGGED       MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 6)
#define SYS_LINEOUT_UNPLUGGED     MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 7)
#define SYS_CAR_ADAPTER_RESUME    MAKE_SYS_EVENT(SYS_EVENT_CLS_MISC, 0)
#define SYS_CALL_INCOMING         MAKE_SYS_EVENT(SYS_EVENT_CLS_MISC, 3)
#define SYS_CALL_HUNG_UP          MAKE_SYS_EVENT(SYS_EVENT_CLS_MISC, 4)
#define SYS_VOLUME_CHANGED        MAKE_SYS_EVENT(SYS_EVENT_CLS_MISC, 5)

#define IS_SYSEVENT(ev)           ((ev & SYS_EVENT) == SYS_EVENT)

#define MAX_NUM_QUEUES 32
#define QUEUE_LENGTH 16 /* MUST be a power of 2 */
#define QUEUE_LENGTH_MASK (QUEUE_LENGTH - 1)

struct queue_event
{
    long     id;
    intptr_t data;
};

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
struct queue_sender_list
{
    /* If non-NULL, there is a thread waiting for the corresponding event */
    /* Must be statically allocated to put in non-cached ram. */
    struct thread_entry *senders[QUEUE_LENGTH]; /* message->thread map */
    struct __wait_queue list;                   /* list of senders in map */
    /* Send info for last message dequeued or NULL if replied or not sent */
    struct thread_entry * volatile curr_sender;
#ifdef HAVE_PRIORITY_SCHEDULING
    struct blocker blocker;
#endif
};
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

#ifdef HAVE_PRIORITY_SCHEDULING
#define QUEUE_GET_THREAD(q) \
    (((q)->send == NULL) ? NULL : (q)->send->blocker.thread)
#else
/* Queue without priority enabled have no owner provision _at this time_ */
#define QUEUE_GET_THREAD(q) \
    (NULL)
#endif

struct event_queue
{
    struct __wait_queue queue;          /* waiter list */
    struct queue_event events[QUEUE_LENGTH]; /* list of events */
    unsigned int volatile read;         /* head of queue */
    unsigned int volatile write;        /* tail of queue */
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    struct queue_sender_list * volatile send; /* list of threads waiting for
                                           reply to an event */
#ifdef HAVE_PRIORITY_SCHEDULING
    struct blocker *blocker_p;          /* priority inheritance info
                                           for sync message senders */
#endif
#endif
    IF_COP( struct corelock cl; )       /* multiprocessor sync */
};

extern void queue_init(struct event_queue *q, bool register_queue);
extern void queue_delete(struct event_queue *q);
extern void queue_wait(struct event_queue *q, struct queue_event *ev);
extern void queue_wait_w_tmo(struct event_queue *q, struct queue_event *ev,
                             int ticks);
extern void queue_post(struct event_queue *q, long id, intptr_t data);
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
extern void queue_enable_queue_send(struct event_queue *q,
                                    struct queue_sender_list *send,
                                    unsigned int owner_id);
extern intptr_t queue_send(struct event_queue *q, long id, intptr_t data);
extern void queue_reply(struct event_queue *q, intptr_t retval);
extern bool queue_in_queue_send(struct event_queue *q);
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */
extern bool queue_empty(const struct event_queue* q);
extern bool queue_full(const struct event_queue* q);
extern bool queue_peek(struct event_queue *q, struct queue_event *ev);

#define QPEEK_FILTER_COUNT_MASK (0xffu) /* 0x00=1 filter, 0xff=256 filters */
#define QPEEK_FILTER_HEAD_ONLY  (1u << 8) /* Ignored if no filters */
#define QPEEK_REMOVE_EVENTS     (1u << 9) /* Remove or discard events */
#define QPEEK_FILTER1(a)    QPEEK_FILTER2((a), (a))
#define QPEEK_FILTER2(a, b) (&(const long [2]){ (a), (b) })
extern bool queue_peek_ex(struct event_queue *q,
                          struct queue_event *ev,
                          unsigned int flags,
                          const long (*filters)[2]);

extern void queue_clear(struct event_queue* q);
extern void queue_remove_from_head(struct event_queue *q, long id);
extern int queue_count(const struct event_queue *q);
extern int queue_broadcast(long id, intptr_t data);
extern void init_queues(void);

#endif /* QUEUE_H */
