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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <stdbool.h>
#include <inttypes.h>
#include "config.h"

#include "thread.h"

/* wrap-safe macros for tick comparison */
#define TIME_AFTER(a,b)         ((long)(b) - (long)(a) < 0)
#define TIME_BEFORE(a,b)        TIME_AFTER(b,a)

#define HZ      100 /* number of ticks per second */

#define MAX_NUM_TICK_TASKS 8

#define MAX_NUM_QUEUES 32
#define QUEUE_LENGTH 16 /* MUST be a power of 2 */
#define QUEUE_LENGTH_MASK (QUEUE_LENGTH - 1)

/* System defined message ID's - |sign bit = 1|class|id| */
/* Event class list */
#define SYS_EVENT_CLS_QUEUE   0
#define SYS_EVENT_CLS_USB     1
#define SYS_EVENT_CLS_POWER   2
#define SYS_EVENT_CLS_FILESYS 3
#define SYS_EVENT_CLS_PLUG    4
#define SYS_EVENT_CLS_MISC    5
/* make sure SYS_EVENT_CLS_BITS has enough range */

/* Bit 31->|S|c...c|i...i| */
#define SYS_EVENT                 ((long)(int)(1 << 31))
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
#define SYS_USB_DISCONNECTED_ACK  MAKE_SYS_EVENT(SYS_EVENT_CLS_USB, 3)
#define SYS_POWEROFF              MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 0)
#define SYS_CHARGER_CONNECTED     MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 1)
#define SYS_CHARGER_DISCONNECTED  MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 2)
#define SYS_BATTERY_UPDATE        MAKE_SYS_EVENT(SYS_EVENT_CLS_POWER, 3)
#define SYS_FS_CHANGED            MAKE_SYS_EVENT(SYS_EVENT_CLS_FILESYS, 0)
#define SYS_HOTSWAP_INSERTED      MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 0)
#define SYS_HOTSWAP_EXTRACTED     MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 1)
#define SYS_PHONE_PLUGGED         MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 2)
#define SYS_PHONE_UNPLUGGED       MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 3)
#define SYS_REMOTE_PLUGGED        MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 4)
#define SYS_REMOTE_UNPLUGGED      MAKE_SYS_EVENT(SYS_EVENT_CLS_PLUG, 5)
#define SYS_SCREENDUMP            MAKE_SYS_EVENT(SYS_EVENT_CLS_MISC, 0)
#define SYS_CAR_ADAPTER_RESUME    MAKE_SYS_EVENT(SYS_EVENT_CLS_MISC, 1)

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
    struct thread_entry *senders[QUEUE_LENGTH];
    /* Send info for last message dequeued or NULL if replied or not sent */
    struct thread_entry *curr_sender;
};
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

struct event_queue
{
    struct thread_queue queue;         /* Waiter list */
    struct queue_event events[QUEUE_LENGTH]; /* list of events */
    unsigned int read;                 /* head of queue */
    unsigned int write;                /* tail of queue */
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    struct queue_sender_list *send;    /* list of threads waiting for
                                          reply to an event */
#endif
#if NUM_CORES > 1
    struct corelock cl;                /* inter-core sync */
#endif
};

struct mutex
{
    struct thread_entry *queue;  /* Waiter list */
#if CONFIG_CORELOCK == SW_CORELOCK
    struct corelock cl;          /* inter-core sync */
#endif
    struct thread_entry *thread; /* thread that owns lock */
    int count;                   /* lock owner recursion count */
    unsigned char locked;        /* locked semaphore */
};

#if NUM_CORES > 1
struct spinlock
{
    struct corelock cl;          /* inter-core sync */
    struct thread_entry *thread; /* lock owner */
    int count;                   /* lock owner recursion count */
};
#endif

#ifdef HAVE_SEMAPHORE_OBJECTS
struct semaphore
{
    struct thread_entry *queue;  /* Waiter list */
#if CONFIG_CORELOCK == SW_CORELOCK
    struct corelock cl;          /* inter-core sync */
#endif
    int count;                   /* # of waits remaining before unsignaled */
    int max;                     /* maximum # of waits to remain signaled */
};
#endif

#ifdef HAVE_EVENT_OBJECTS
struct event
{
    struct thread_entry *queues[2]; /* waiters for each state */
#if CONFIG_CORELOCK == SW_CORELOCK
    struct corelock cl;             /* inter-core sync */
#endif
    unsigned char automatic;        /* event performs auto-reset */
    unsigned char state;            /* state: 1 = signaled */
};
#endif

/* global tick variable */
#if defined(CPU_PP) && defined(BOOTLOADER)
/* We don't enable interrupts in the iPod bootloader, so we need to fake
   the current_tick variable */
#define current_tick (signed)(USEC_TIMER/10000)
#else
extern volatile long current_tick;
#endif

#ifdef SIMULATOR
#define sleep(x) sim_sleep(x)
#endif

/* kernel functions */
extern void kernel_init(void);
extern void yield(void);
extern void sleep(int ticks);
int tick_add_task(void (*f)(void));
int tick_remove_task(void (*f)(void));
extern void tick_start(unsigned int interval_in_ms);

struct timeout;

/* timeout callback type
 * tmo - pointer to struct timeout associated with event
 */
typedef bool (* timeout_cb_type)(struct timeout *tmo);

struct timeout
{
    /* for use by callback/internal - read/write */
    timeout_cb_type callback;/* callback - returning false cancels */
    int             ticks;   /* timeout period in ticks */
    intptr_t        data;    /* data passed to callback */
    /* internal use - read-only */
    const struct timeout * const next; /* next timeout in list */
    const long expires; /* expiration tick */
};

void timeout_register(struct timeout *tmo, timeout_cb_type callback,
                      int ticks, intptr_t data);
void timeout_cancel(struct timeout *tmo);

#define STATE_NONSIGNALED 0
#define STATE_SIGNALED    1

#define WAIT_TIMEDOUT     (-1)
#define WAIT_SUCCEEDED    1

extern void queue_init(struct event_queue *q, bool register_queue);
extern void queue_delete(struct event_queue *q);
extern void queue_wait(struct event_queue *q, struct queue_event *ev);
extern void queue_wait_w_tmo(struct event_queue *q, struct queue_event *ev,
                             int ticks);
extern void queue_post(struct event_queue *q, long id, intptr_t data);
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
extern void queue_enable_queue_send(struct event_queue *q, struct queue_sender_list *send);
extern intptr_t queue_send(struct event_queue *q, long id, intptr_t data);
extern void queue_reply(struct event_queue *q, intptr_t retval);
extern bool queue_in_queue_send(struct event_queue *q);
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */
extern bool queue_empty(const struct event_queue* q);
extern bool queue_peek(struct event_queue *q, struct queue_event *ev);
extern void queue_clear(struct event_queue* q);
extern void queue_remove_from_head(struct event_queue *q, long id);
extern int queue_count(const struct event_queue *q);
extern int queue_broadcast(long id, intptr_t data);

extern void mutex_init(struct mutex *m);
extern void mutex_lock(struct mutex *m);
extern void mutex_unlock(struct mutex *m);
#if NUM_CORES > 1
extern void spinlock_init(struct spinlock *l);
extern void spinlock_lock(struct spinlock *l);
extern void spinlock_unlock(struct spinlock *l);
#endif
#ifdef HAVE_SEMAPHORE_OBJECTS
extern void semaphore_init(struct semaphore *s, int max, int start);
extern void semaphore_wait(struct semaphore *s);
extern void semaphore_release(struct semaphore *s);
#endif /* HAVE_SEMAPHORE_OBJECTS */
#ifdef HAVE_EVENT_OBJECTS
#define EVENT_AUTOMATIC 0x10
#define EVENT_MANUAL    0x00
extern void event_init(struct event *e, unsigned int flags);
extern void event_wait(struct event *e, unsigned int for_state);
extern void event_set_state(struct event *e, unsigned int state);
#endif /* HAVE_EVENT_OBJECTS */
#define IS_SYSEVENT(ev) ((ev & SYS_EVENT) == SYS_EVENT)

#endif /* _KERNEL_H_ */
