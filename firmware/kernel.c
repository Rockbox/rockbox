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
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "kernel.h"
#ifdef SIMULATOR
#include "system-sdl.h"
#include "debug.h"
#endif
#include "thread.h"
#include "cpu.h"
#include "system.h"
#include "panic.h"

/* Make this nonzero to enable more elaborate checks on objects */
#if defined(DEBUG) || defined(SIMULATOR)
#define KERNEL_OBJECT_CHECKS 1 /* Always 1 for DEBUG and sim*/
#else
#define KERNEL_OBJECT_CHECKS 0
#endif

#if KERNEL_OBJECT_CHECKS
#ifdef SIMULATOR
#define KERNEL_ASSERT(exp, msg...) \
    ({ if (!({ exp; })) { DEBUGF(msg); exit(-1); } })
#else
#define KERNEL_ASSERT(exp, msg...) \
    ({ if (!({ exp; })) panicf(msg); })
#endif
#else
#define KERNEL_ASSERT(exp, msg...) ({})
#endif

#if !defined(CPU_PP) || !defined(BOOTLOADER) 
volatile long current_tick NOCACHEDATA_ATTR = 0;
#endif

void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

extern struct core_entry cores[NUM_CORES];

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct
{
    int count;
    struct event_queue *queues[MAX_NUM_QUEUES];
    IF_COP( struct corelock cl; )
} all_queues NOCACHEBSS_ATTR;

/****************************************************************************
 * Standard kernel stuff
 ****************************************************************************/
void kernel_init(void)
{
    /* Init the threading API */
    init_threads();

    /* Other processors will not reach this point in a multicore build.
     * In a single-core build with multiple cores they fall-through and
     * sleep in cop_main without returning. */
    if (CURRENT_CORE == CPU)
    {
        memset(tick_funcs, 0, sizeof(tick_funcs));
        memset(&all_queues, 0, sizeof(all_queues));
        corelock_init(&all_queues.cl);
        tick_start(1000/HZ);
    }
}

/****************************************************************************
 * Timer tick
 ****************************************************************************/
#if CONFIG_CPU == SH7034
void tick_start(unsigned int interval_in_ms)
{
    unsigned long count;

    count = CPU_FREQ * interval_in_ms / 1000 / 8;

    if(count > 0x10000)
    {
        panicf("Error! The tick interval is too long (%d ms)\n",
               interval_in_ms);
        return;
    }
    
    /* We are using timer 0 */
    
    TSTR &= ~0x01; /* Stop the timer */
    TSNC &= ~0x01; /* No synchronization */
    TMDR &= ~0x01; /* Operate normally */

    TCNT0 = 0;   /* Start counting at 0 */
    GRA0 = (unsigned short)(count - 1);
    TCR0 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 1 */
    IPRC = (IPRC & ~0x00f0) | 0x0010;
    
    TSR0 &= ~0x01;
    TIER0 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x01; /* Start timer 1 */
}

void IMIA0(void) __attribute__ ((interrupt_handler));
void IMIA0(void)
{
    int i;

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;

    TSR0 &= ~0x01;
}
#elif defined(CPU_COLDFIRE)
void tick_start(unsigned int interval_in_ms)
{
    unsigned long count;
    int prescale;

    count = CPU_FREQ/2 * interval_in_ms / 1000 / 16;

    if(count > 0x10000)
    {
        panicf("Error! The tick interval is too long (%d ms)\n",
               interval_in_ms);
        return;
    }

    prescale = cpu_frequency / CPU_FREQ;
    /* Note: The prescaler is later adjusted on-the-fly on CPU frequency
       changes within timer.c */
    
    /* We are using timer 0 */

    TRR0 = (unsigned short)(count - 1); /* The reference count */
    TCN0 = 0; /* reset the timer */
    TMR0 = 0x001d | ((unsigned short)(prescale - 1) << 8); 
           /* restart, CLK/16, enabled, prescaler */

    TER0 = 0xff; /* Clear all events */

    ICR1 = 0x8c; /* Interrupt on level 3.0 */
    IMR &= ~0x200;
}

void TIMER0(void) __attribute__ ((interrupt_handler));
void TIMER0(void)
{
    int i;

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;

    TER0 = 0xff; /* Clear all events */
}

#elif defined(CPU_PP)

#ifndef BOOTLOADER
void TIMER1(void)
{
    int i;

    /* Run through the list of tick tasks (using main core) */
    TIMER1_VAL; /* Read value to ack IRQ */

    /* Run through the list of tick tasks using main CPU core - 
       wake up the COP through its control interface to provide pulse */
    for (i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if (tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

#if NUM_CORES > 1
    /* Pulse the COP */
    core_wake(COP);
#endif /* NUM_CORES */

    current_tick++;
}
#endif

/* Must be last function called init kernel/thread initialization */
void tick_start(unsigned int interval_in_ms)
{
#ifndef BOOTLOADER
    TIMER1_CFG = 0x0;
    TIMER1_VAL;
    /* enable timer */
    TIMER1_CFG = 0xc0000000 | (interval_in_ms*1000 - 1);
    /* unmask interrupt source */
    CPU_INT_EN = TIMER1_MASK;
#else
    /* We don't enable interrupts in the bootloader */
    (void)interval_in_ms;
#endif
}

#elif CONFIG_CPU == PNX0101

void timer_handler(void)
{
    int i;

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
            tick_funcs[i]();
    }

    current_tick++;

    TIMER0.clr = 0;
}

void tick_start(unsigned int interval_in_ms)
{
    TIMER0.ctrl &= ~0x80; /* Disable the counter */
    TIMER0.ctrl |= 0x40;  /* Reload after counting down to zero */
    TIMER0.load = 3000000 * interval_in_ms / 1000;
    TIMER0.ctrl &= ~0xc;  /* No prescaler */
    TIMER0.clr = 1;       /* Clear the interrupt request */

    irq_set_int_handler(IRQ_TIMER0, timer_handler);
    irq_enable_int(IRQ_TIMER0);

    TIMER0.ctrl |= 0x80;  /* Enable the counter */
}
#endif

int tick_add_task(void (*f)(void))
{
    int i;
    int oldlevel = disable_irq_save();

    /* Add a task if there is room */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == NULL)
        {
            tick_funcs[i] = f;
            restore_irq(oldlevel);
            return 0;
        }
    }
    restore_irq(oldlevel);
    panicf("Error! tick_add_task(): out of tasks");
    return -1;
}

int tick_remove_task(void (*f)(void))
{
    int i;
    int oldlevel = disable_irq_save();

    /* Remove a task if it is there */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == f)
        {
            tick_funcs[i] = NULL;
            restore_irq(oldlevel);
            return 0;
        }
    }
    
    restore_irq(oldlevel);
    return -1;
}

/****************************************************************************
 * Tick-based interval timers/one-shots - be mindful this is not really
 * intended for continuous timers but for events that need to run for a short
 * time and be cancelled without further software intervention.
 ****************************************************************************/
#ifdef INCLUDE_TIMEOUT_API
static struct timeout *tmo_list = NULL; /* list of active timeout events */

/* timeout tick task - calls event handlers when they expire
 * Event handlers may alter ticks, callback and data during operation.
 */
static void timeout_tick(void)
{
    unsigned long tick = current_tick;
    struct timeout *curr, *next;

    for (curr = tmo_list; curr != NULL; curr = next)
    {
        next = (struct timeout *)curr->next;

        if (TIME_BEFORE(tick, curr->expires))
            continue;

        /* this event has expired - call callback */
        if (curr->callback(curr))
            *(long *)&curr->expires = tick + curr->ticks; /* reload */
        else
            timeout_cancel(curr); /* cancel */
    }
}

/* Cancels a timeout callback - can be called from the ISR */
void timeout_cancel(struct timeout *tmo)
{
    int oldlevel = disable_irq_save();

    if (tmo_list != NULL)
    {
        struct timeout *curr = tmo_list;
        struct timeout *prev = NULL;

        while (curr != tmo && curr != NULL)
        {
            prev = curr;
            curr = (struct timeout *)curr->next;
        }

        if (curr != NULL)
        {
            /* in list */
            if (prev == NULL)
                tmo_list = (struct timeout *)curr->next;
            else
                *(const struct timeout **)&prev->next = curr->next;

            if (tmo_list == NULL)
                tick_remove_task(timeout_tick); /* last one - remove task */
        }
        /* not in list or tmo == NULL */
    }

    restore_irq(oldlevel);
}

/* Adds a timeout callback - calling with an active timeout resets the
   interval - can be called from the ISR */
void timeout_register(struct timeout *tmo, timeout_cb_type callback,
                      int ticks, intptr_t data)
{
    int oldlevel;
    struct timeout *curr;

    if (tmo == NULL)
        return;

    oldlevel = disable_irq_save();

    /* see if this one is already registered */
    curr = tmo_list;
    while (curr != tmo && curr != NULL)
        curr = (struct timeout *)curr->next;

    if (curr == NULL)
    {
        /* not found - add it */
        if (tmo_list == NULL)
            tick_add_task(timeout_tick); /* first one - add task */

        *(struct timeout **)&tmo->next = tmo_list;
        tmo_list = tmo;
    }

    tmo->callback = callback;
    tmo->ticks = ticks;
    tmo->data = data;
    *(long *)&tmo->expires = current_tick + ticks;

    restore_irq(oldlevel);
}

#endif /* INCLUDE_TIMEOUT_API */

/****************************************************************************
 * Thread stuff
 ****************************************************************************/
void sleep(int ticks)
{
#if CONFIG_CPU == S3C2440 && defined(BOOTLOADER)
    volatile int counter;
    TCON &= ~(1 << 20); // stop timer 4
    // TODO: this constant depends on dividers settings inherited from
    // firmware. Set them explicitly somwhere.
    TCNTB4 = 12193 * ticks / HZ;
    TCON |= 1 << 21; // set manual bit
    TCON &= ~(1 << 21); // reset manual bit
    TCON &= ~(1 << 22); //autoreload Off
    TCON |= (1 << 20); // start timer 4
    do {
       counter = TCNTO4;
    } while(counter > 0);

#elif defined(CPU_PP) && defined(BOOTLOADER)
    unsigned stop = USEC_TIMER + ticks * (1000000/HZ);
    while (TIME_BEFORE(USEC_TIMER, stop))
        switch_thread();
#else
    disable_irq();
    sleep_thread(ticks);
    switch_thread();
#endif
}

void yield(void)
{
#if ((CONFIG_CPU == S3C2440 || defined(ELIO_TPJ1022)) && defined(BOOTLOADER))
    /* Some targets don't like yielding in the bootloader */
#else
    switch_thread();
#endif
}

/****************************************************************************
 * Queue handling stuff
 ****************************************************************************/

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/****************************************************************************
 * Sender thread queue structure that aids implementation of priority
 * inheritance on queues because the send list structure is the same as
 * for all other kernel objects:
 *
 * Example state:
 * E0 added with queue_send and removed by thread via queue_wait(_w_tmo)
 * E3 was posted with queue_post
 * 4 events remain enqueued (E1-E4)
 *
 *                                 rd                          wr
 * q->events[]:          |  XX  |  E1  |  E2  |  E3  |  E4  |  XX  |
 * q->send->senders[]:   | NULL |  T1  |  T2  | NULL |  T3  | NULL |
 *                                 \/     \/            \/
 * q->send->list:       >->|T0|<->|T1|<->|T2|<-------->|T3|<-<
 * q->send->curr_sender:    /\
 *
 * Thread has E0 in its own struct queue_event.
 *
 ****************************************************************************/

/* Puts the specified return value in the waiting thread's return value
 * and wakes the thread.
 *
 * A sender should be confirmed to exist before calling which makes it
 * more efficent to reject the majority of cases that don't need this
 * called.
 */
static void queue_release_sender(struct thread_entry **sender,
                                 intptr_t retval)
{
    struct thread_entry *thread = *sender;

    *sender = NULL;               /* Clear slot. */
    thread->wakeup_ext_cb = NULL; /* Clear callback. */
    thread->retval = retval;      /* Assign thread-local return value. */
    *thread->bqp = thread;        /* Move blocking queue head to thread since
                                     wakeup_thread wakes the first thread in
                                     the list. */
    wakeup_thread(thread->bqp);
}

/* Releases any waiting threads that are queued with queue_send -
 * reply with 0.
 */
static void queue_release_all_senders(struct event_queue *q)
{
    if(q->send)
    {
        unsigned int i;
        for(i = q->read; i != q->write; i++)
        {
            struct thread_entry **spp =
                &q->send->senders[i & QUEUE_LENGTH_MASK];

            if(*spp)
            {
                queue_release_sender(spp, 0);
            }
        }
    }
}

/* Callback to do extra forced removal steps from sender list in addition
 * to the normal blocking queue removal and priority dis-inherit */
static void queue_remove_sender_thread_cb(struct thread_entry *thread)
{
    *((struct thread_entry **)thread->retval) = NULL;
    thread->wakeup_ext_cb = NULL;
    thread->retval = 0;
}

/* Enables queue_send on the specified queue - caller allocates the extra
 * data structure. Only queues which are taken to be owned by a thread should
 * enable this however an official owner is not compulsory but must be
 * specified for priority inheritance to operate.
 *
 * Use of queue_wait(_w_tmo) by multiple threads on a queue using synchronous
 * messages results in an undefined order of message replies.
 */
void queue_enable_queue_send(struct event_queue *q,
                             struct queue_sender_list *send,
                             struct thread_entry *owner)
{
    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    if(send != NULL && q->send == NULL)
    {
        memset(send, 0, sizeof(*send));
#ifdef HAVE_PRIORITY_SCHEDULING
        send->blocker.wakeup_protocol = wakeup_priority_protocol_release;
        send->blocker.priority = PRIORITY_IDLE;
        send->blocker.thread = owner;
        if(owner != NULL)
            q->blocker_p = &send->blocker;
#endif
        q->send = send;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);

    (void)owner;
}

/* Unblock a blocked thread at a given event index */
static inline void queue_do_unblock_sender(struct queue_sender_list *send,
                                           unsigned int i)
{
    if(send)
    {
        struct thread_entry **spp = &send->senders[i];

        if(*spp)
        {
            queue_release_sender(spp, 0);
        }
    }
}

/* Perform the auto-reply sequence */
static inline void queue_do_auto_reply(struct queue_sender_list *send)
{
    if(send && send->curr_sender)
    {
        /* auto-reply */
        queue_release_sender(&send->curr_sender, 0);
    }
}

/* Moves waiting thread's refrence from the senders array to the
 * current_sender which represents the thread waiting for a reponse to the
 * last message removed from the queue. This also protects the thread from
 * being bumped due to overflow which would not be a valid action since its
 * message _is_ being processed at this point. */
static inline void queue_do_fetch_sender(struct queue_sender_list *send,
                                         unsigned int rd)
{
    if(send)
    {
        struct thread_entry **spp = &send->senders[rd];

        if(*spp)
        {
            /* Move thread reference from array to the next thread
               that queue_reply will release */
            send->curr_sender = *spp;
            (*spp)->retval = (intptr_t)spp;
            *spp = NULL;
        }
        /* else message was posted asynchronously with queue_post */
    }
}
#else
/* Empty macros for when synchoronous sending is not made */
#define queue_release_all_senders(q)
#define queue_do_unblock_sender(send, i)
#define queue_do_auto_reply(send)
#define queue_do_fetch_sender(send, rd)
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

/* Queue must not be available for use during this call */
void queue_init(struct event_queue *q, bool register_queue)
{
    int oldlevel = disable_irq_save();

    if(register_queue)
    {
        corelock_lock(&all_queues.cl);
    }

    corelock_init(&q->cl);
    q->queue = NULL;
    q->read = 0;
    q->write = 0;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    q->send = NULL; /* No message sending by default */
    IF_PRIO( q->blocker_p = NULL; )
#endif

    if(register_queue)
    {
        if(all_queues.count >= MAX_NUM_QUEUES)
        {
            panicf("queue_init->out of queues");
        }
        /* Add it to the all_queues array */
        all_queues.queues[all_queues.count++] = q;
        corelock_unlock(&all_queues.cl);
    }

    restore_irq(oldlevel);
}

/* Queue must not be available for use during this call */
void queue_delete(struct event_queue *q)
{
    int oldlevel;
    int i;

    oldlevel = disable_irq_save();
    corelock_lock(&all_queues.cl);
    corelock_lock(&q->cl);

    /* Find the queue to be deleted */
    for(i = 0;i < all_queues.count;i++)
    {
        if(all_queues.queues[i] == q)
        {
            /* Move the following queues up in the list */
            all_queues.count--;

            for(;i < all_queues.count;i++)
            {
                all_queues.queues[i] = all_queues.queues[i+1];
            }

            break;
        }
    }

    corelock_unlock(&all_queues.cl);

    /* Release thread(s) waiting on queue head */
    thread_queue_wake(&q->queue);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send)
    {
        /* Release threads waiting for replies */
        queue_release_all_senders(q);

        /* Reply to any dequeued message waiting for one */
        queue_do_auto_reply(q->send);

        q->send = NULL;
    }
#endif

    q->read = 0;
    q->write = 0;

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

/* NOTE: multiple threads waiting on a queue head cannot have a well-
   defined release order if timeouts are used. If multiple threads must
   access the queue head, use a dispatcher or queue_wait only. */
void queue_wait(struct event_queue *q, struct queue_event *ev)
{
    int oldlevel;
    unsigned int rd;

#ifdef HAVE_PRIORITY_SCHEDULING
    KERNEL_ASSERT(QUEUE_GET_THREAD(q) == NULL ||
                  QUEUE_GET_THREAD(q) == thread_get_current(),
                  "queue_wait->wrong thread\n");
#endif

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* auto-reply */
    queue_do_auto_reply(q->send);
    
    if (q->read == q->write)
    {
        struct thread_entry *current = cores[CURRENT_CORE].running;

        do
        {
            IF_COP( current->obj_cl = &q->cl; )
            current->bqp = &q->queue;

            block_thread(current);

            corelock_unlock(&q->cl);
            switch_thread();

            oldlevel = disable_irq_save();
            corelock_lock(&q->cl);
        }
        /* A message that woke us could now be gone */
        while (q->read == q->write);
    } 

    rd = q->read++ & QUEUE_LENGTH_MASK;
    *ev = q->events[rd];

    /* Get data for a waiting thread if one */
    queue_do_fetch_sender(q->send, rd);

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

void queue_wait_w_tmo(struct event_queue *q, struct queue_event *ev, int ticks)
{
    int oldlevel;

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    KERNEL_ASSERT(QUEUE_GET_THREAD(q) == NULL ||
                  QUEUE_GET_THREAD(q) == thread_get_current(),
                  "queue_wait_w_tmo->wrong thread\n");
#endif

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* Auto-reply */
    queue_do_auto_reply(q->send);

    if (q->read == q->write && ticks > 0)
    {
        struct thread_entry *current = cores[CURRENT_CORE].running;

        IF_COP( current->obj_cl = &q->cl; )
        current->bqp = &q->queue;

        block_thread_w_tmo(current, ticks);
        corelock_unlock(&q->cl);    

        switch_thread();

        oldlevel = disable_irq_save();
        corelock_lock(&q->cl);
    }

    /* no worry about a removed message here - status is checked inside
       locks - perhaps verify if timeout or false alarm */
    if (q->read != q->write)
    {
        unsigned int rd = q->read++ & QUEUE_LENGTH_MASK;
        *ev = q->events[rd];
        /* Get data for a waiting thread if one */
        queue_do_fetch_sender(q->send, rd);
    }
    else
    {
        ev->id = SYS_TIMEOUT;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

void queue_post(struct event_queue *q, long id, intptr_t data)
{
    int oldlevel;
    unsigned int wr;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;

    /* overflow protect - unblock any thread waiting at this index */
    queue_do_unblock_sender(q->send, wr);

    /* Wakeup a waiting thread if any */
    wakeup_thread(&q->queue);

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/* IRQ handlers are not allowed use of this function - we only aim to
   protect the queue integrity by turning them off. */
intptr_t queue_send(struct event_queue *q, long id, intptr_t data)
{
    int oldlevel;
    unsigned int wr;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;
    
    if(q->send)
    {
        struct queue_sender_list *send = q->send;
        struct thread_entry **spp = &send->senders[wr];
        struct thread_entry *current = cores[CURRENT_CORE].running;

        if(*spp)
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, 0);
        }

        /* Wakeup a waiting thread if any */
        wakeup_thread(&q->queue);

        /* Save thread in slot, add to list and wait for reply */
        *spp = current;
        IF_COP( current->obj_cl = &q->cl; )
        IF_PRIO( current->blocker = q->blocker_p; )
        current->wakeup_ext_cb = queue_remove_sender_thread_cb;
        current->retval = (intptr_t)spp;
        current->bqp = &send->list;

        block_thread(current);

        corelock_unlock(&q->cl);
        switch_thread();

        return current->retval;
    }

    /* Function as queue_post if sending is not enabled */
    wakeup_thread(&q->queue);

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
    
    return 0;
}

#if 0 /* not used now but probably will be later */
/* Query if the last message dequeued was added by queue_send or not */
bool queue_in_queue_send(struct event_queue *q)
{
    bool in_send;

#if NUM_CORES > 1
    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);
#endif

    in_send = q->send && q->send->curr_sender;

#if NUM_CORES > 1
    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
#endif

    return in_send;
}
#endif

/* Replies with retval to the last dequeued message sent with queue_send */
void queue_reply(struct event_queue *q, intptr_t retval)
{
    if(q->send && q->send->curr_sender)
    {
        int oldlevel = disable_irq_save();
        corelock_lock(&q->cl);
        /* Double-check locking */
        IF_COP( if(q->send && q->send->curr_sender) )
        {
            queue_release_sender(&q->send->curr_sender, retval);
        }

        corelock_unlock(&q->cl);
        restore_irq(oldlevel);
    }
}

bool queue_peek(struct event_queue *q, struct queue_event *ev)
{
    if(q->read == q->write)
         return false;

    bool have_msg = false;

    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    if(q->read != q->write)
    {
        *ev = q->events[q->read & QUEUE_LENGTH_MASK];
        have_msg = true;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);

    return have_msg;
}
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

/* Poll queue to see if a message exists - careful in using the result if
 * queue_remove_from_head is called when messages are posted - possibly use
 * queue_wait_w_tmo(&q, 0) in that case or else a removed message that
 * unsignals the queue may cause an unwanted block */
bool queue_empty(const struct event_queue* q)
{
    return ( q->read == q->write );
}

void queue_clear(struct event_queue* q)
{
    int oldlevel;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* Release all threads waiting in the queue for a reply -
       dequeued sent message will be handled by owning thread */
    queue_release_all_senders(q);

    q->read = 0;
    q->write = 0;

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

void queue_remove_from_head(struct event_queue *q, long id)
{
    int oldlevel;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    while(q->read != q->write)
    {
        unsigned int rd = q->read & QUEUE_LENGTH_MASK;

        if(q->events[rd].id != id)
        {
            break;
        }

        /* Release any thread waiting on this message */
        queue_do_unblock_sender(q->send, rd);

        q->read++;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

/**
 * The number of events waiting in the queue.
 * 
 * @param struct of event_queue
 * @return number of events in the queue
 */
int queue_count(const struct event_queue *q)
{
    return q->write - q->read;
}

int queue_broadcast(long id, intptr_t data)
{
    int i;

#if NUM_CORES > 1
    int oldlevel = disable_irq_save();
    corelock_lock(&all_queues.cl);
#endif
    
    for(i = 0;i < all_queues.count;i++)
    {
        queue_post(all_queues.queues[i], id, data);
    }

#if NUM_CORES > 1
    corelock_unlock(&all_queues.cl);
    restore_irq(oldlevel);
#endif
   
    return i;
}

/****************************************************************************
 * Simple mutex functions ;)
 ****************************************************************************/

/* Initialize a mutex object - call before any use and do not call again once
 * the object is available to other threads */
void mutex_init(struct mutex *m)
{
    corelock_init(&m->cl);
    m->queue = NULL;
    m->count = 0;
    m->locked = 0;
    MUTEX_SET_THREAD(m, NULL);
#ifdef HAVE_PRIORITY_SCHEDULING
    m->blocker.priority = PRIORITY_IDLE;
    m->blocker.wakeup_protocol = wakeup_priority_protocol_transfer;
    m->no_preempt = false;
#endif
}

/* Gain ownership of a mutex object or block until it becomes free */
void mutex_lock(struct mutex *m)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *current = cores[core].running;

    if(current == MUTEX_GET_THREAD(m))
    {
        /* current thread already owns this mutex */
        m->count++;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    if(m->locked == 0)
    {
        /* lock is open */
        MUTEX_SET_THREAD(m, current);
        m->locked = 1;
        corelock_unlock(&m->cl);
        return;
    }

    /* block until the lock is open... */
    IF_COP( current->obj_cl = &m->cl; )
    IF_PRIO( current->blocker = &m->blocker; )
    current->bqp = &m->queue;

    disable_irq();
    block_thread(current);

    corelock_unlock(&m->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release ownership of a mutex object - only owning thread must call this */
void mutex_unlock(struct mutex *m)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(MUTEX_GET_THREAD(m) == thread_get_current(),
                  "mutex_unlock->wrong thread (%s != %s)\n",
                  MUTEX_GET_THREAD(m)->name,
                  thread_get_current()->name);

    if(m->count > 0)
    {
        /* this thread still owns lock */
        m->count--;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    /* transfer to next queued thread if any */
    if(m->queue == NULL)
    {
        /* no threads waiting - open the lock */
        MUTEX_SET_THREAD(m, NULL);
        m->locked = 0;
        corelock_unlock(&m->cl);
        return;
    }
    else
    {
        const int oldlevel = disable_irq_save();
        /* Tranfer of owning thread is handled in the wakeup protocol
         * if priorities are enabled otherwise just set it from the
         * queue head. */
        IFN_PRIO( MUTEX_SET_THREAD(m, m->queue); )
        IF_PRIO( unsigned int result = ) wakeup_thread(&m->queue);
        restore_irq(oldlevel);

        corelock_unlock(&m->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
        if((result & THREAD_SWITCH) && !m->no_preempt)
            switch_thread();
#endif
    }
}

/****************************************************************************
 * Simpl-er mutex functions ;)
 ****************************************************************************/
#if NUM_CORES > 1
void spinlock_init(struct spinlock *l)
{
    corelock_init(&l->cl);
    l->thread = NULL;
    l->count = 0;
}

void spinlock_lock(struct spinlock *l)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *current = cores[core].running;

    if(l->thread == current)
    {
        /* current core already owns it */
        l->count++;
        return;
    }

    /* lock against other processor cores */
    corelock_lock(&l->cl);

    /* take ownership */
    l->thread = current;
}

void spinlock_unlock(struct spinlock *l)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(l->thread == thread_get_current(),
                  "spinlock_unlock->wrong thread\n");

    if(l->count > 0)
    {
        /* this core still owns lock */
        l->count--;
        return;
    }

    /* clear owner */
    l->thread = NULL;

    /* release lock */
    corelock_unlock(&l->cl);
}
#endif /* NUM_CORES > 1 */

/****************************************************************************
 * Simple semaphore functions ;)
 ****************************************************************************/
#ifdef HAVE_SEMAPHORE_OBJECTS
void semaphore_init(struct semaphore *s, int max, int start)
{
    KERNEL_ASSERT(max > 0 && start >= 0 && start <= max,
                  "semaphore_init->inv arg\n");
    s->queue = NULL;
    s->max = max;
    s->count = start;
    corelock_init(&s->cl);
}

void semaphore_wait(struct semaphore *s)
{
    struct thread_entry *current;

    corelock_lock(&s->cl);

    if(--s->count >= 0)
    {
        /* wait satisfied */
        corelock_unlock(&s->cl);
        return;
    }

    /* too many waits - block until dequeued... */
    current = cores[CURRENT_CORE].running;

    IF_COP( current->obj_cl = &s->cl; )
    current->bqp = &s->queue;

    disable_irq();
    block_thread(current);

    corelock_unlock(&s->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

void semaphore_release(struct semaphore *s)
{
    IF_PRIO( unsigned int result = THREAD_NONE; )

    corelock_lock(&s->cl);

    if(s->count < s->max && ++s->count <= 0)
    {
        /* there should be threads in this queue */
        KERNEL_ASSERT(s->queue != NULL, "semaphore->wakeup\n");
        /* a thread was queued - wake it up */
        int oldlevel = disable_irq_save();
        IF_PRIO( result = ) wakeup_thread(&s->queue);
        restore_irq(oldlevel);
    }

    corelock_unlock(&s->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if(result & THREAD_SWITCH)
        switch_thread();
#endif
}
#endif /* HAVE_SEMAPHORE_OBJECTS */

/****************************************************************************
 * Simple event functions ;)
 ****************************************************************************/
#ifdef HAVE_EVENT_OBJECTS
void event_init(struct event *e, unsigned int flags)
{
    e->queues[STATE_NONSIGNALED] = NULL;
    e->queues[STATE_SIGNALED] = NULL;
    e->state = flags & STATE_SIGNALED;
    e->automatic = (flags & EVENT_AUTOMATIC) ? 1 : 0;
    corelock_init(&e->cl);
}

void event_wait(struct event *e, unsigned int for_state)
{
    struct thread_entry *current;

    corelock_lock(&e->cl);

    if(e->automatic != 0)
    {
        /* wait for false always satisfied by definition
           or if it just changed to false */
        if(e->state == STATE_SIGNALED || for_state == STATE_NONSIGNALED)
        {
            /* automatic - unsignal */
            e->state = STATE_NONSIGNALED;
            corelock_unlock(&e->cl);
            return;
        }
        /* block until state matches */
    }
    else if(for_state == e->state)
    {
        /* the state being waited for is the current state */
        corelock_unlock(&e->cl);
        return;
    }

    /* block until state matches what callers requests */
    current = cores[CURRENT_CORE].running;

    IF_COP( current->obj_cl = &e->cl; )
    current->bqp = &e->queues[for_state];

    disable_irq();
    block_thread(current);

    corelock_unlock(&e->cl);

    /* turn control over to next thread */
    switch_thread();
}

void event_set_state(struct event *e, unsigned int state)
{
    unsigned int result;
    int oldlevel;

    corelock_lock(&e->cl);

    if(e->state == state)
    {
        /* no change */
        corelock_unlock(&e->cl);
        return;
    }

    IF_PRIO( result = THREAD_OK; )

    oldlevel = disable_irq_save();

    if(state == STATE_SIGNALED)
    {
        if(e->automatic != 0)
        {
            /* no thread should have ever blocked for nonsignaled */
            KERNEL_ASSERT(e->queues[STATE_NONSIGNALED] == NULL,
                          "set_event_state->queue[NS]:S\n");
            /* pass to next thread and keep unsignaled - "pulse" */
            result = wakeup_thread(&e->queues[STATE_SIGNALED]);
            e->state = (result & THREAD_OK) ? STATE_NONSIGNALED : STATE_SIGNALED;
        }
        else
        {
            /* release all threads waiting for signaled */
            e->state = STATE_SIGNALED;
            IF_PRIO( result = )
                thread_queue_wake(&e->queues[STATE_SIGNALED]);
        }
    }
    else
    {
        /* release all threads waiting for nonsignaled */

        /* no thread should have ever blocked if automatic */
        KERNEL_ASSERT(e->queues[STATE_NONSIGNALED] == NULL ||
                      e->automatic == 0, "set_event_state->queue[NS]:NS\n");

        e->state = STATE_NONSIGNALED;
        IF_PRIO( result = )
            thread_queue_wake(&e->queues[STATE_NONSIGNALED]);
    }

    restore_irq(oldlevel);

    corelock_unlock(&e->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if(result & THREAD_SWITCH)
        switch_thread();
#endif
}
#endif /* HAVE_EVENT_OBJECTS */
