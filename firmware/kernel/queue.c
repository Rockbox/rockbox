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
#include <string.h>
#include "kernel-internal.h"
#include "queue.h"
#include "general.h"

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct
{
    struct event_queue *queues[MAX_NUM_QUEUES+1];
#ifdef HAVE_CORELOCK_OBJECT
    struct corelock cl;
#endif
} all_queues SHAREDBSS_ATTR;

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
 * q->send->list:       0<-|T0|<->|T1|<->|T2|<-------->|T3|->0
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
static void queue_release_sender_inner(
    struct thread_entry * volatile * sender, intptr_t retval)
{
    struct thread_entry *thread = *sender;
    *sender = NULL;               /* Clear slot. */
    thread->retval = retval;      /* Assign thread-local return value. */
    wakeup_thread(thread, WAKEUP_RELEASE);
}

static inline void queue_release_sender(
    struct thread_entry * volatile * sender, intptr_t retval)
{
    if(UNLIKELY(*sender))
        queue_release_sender_inner(sender, retval);
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
            queue_release_sender(spp, 0);
        }
    }
}

/* Enables queue_send on the specified queue - caller allocates the extra
 * data structure. Only queues which are taken to be owned by a thread should
 * enable this however an official owner is not compulsory but must be
 * specified for priority inheritance to operate.
 *
 * Use of queue_wait(_w_tmo) by multiple threads on a queue using synchronous
 * messages results in an undefined order of message replies or possible default
 * replies if two or more waits happen before a reply is done.
 */
void queue_enable_queue_send(struct event_queue *q,
                             struct queue_sender_list *send,
                             unsigned int owner_id)
{
    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    if(send != NULL && q->send == NULL)
    {
        memset(send, 0, sizeof(*send));
        wait_queue_init(&send->list);
#ifdef HAVE_PRIORITY_SCHEDULING
        blocker_init(&send->blocker);
        if(owner_id != 0)
        {
            send->blocker.thread = __thread_id_entry(owner_id);
            q->blocker_p = &send->blocker;
        }
#endif
        q->send = send;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);

    (void)owner_id;
}

/* Unblock a blocked thread at a given event index */
static inline void queue_do_unblock_sender(struct queue_sender_list *send,
                                           unsigned int i)
{
    if(send)
        queue_release_sender(&send->senders[i], 0);
}

/* Perform the auto-reply sequence */
static inline void queue_do_auto_reply(struct queue_sender_list *send)
{
    if(send)
        queue_release_sender(&send->curr_sender, 0);
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

static void queue_wake_waiter_inner(struct thread_entry *thread)
{
    wakeup_thread(thread, WAKEUP_DEFAULT);
}

static inline void queue_wake_waiter(struct event_queue *q)
{
    struct thread_entry *thread = WQ_THREAD_FIRST(&q->queue);
    if(thread != NULL)
        queue_wake_waiter_inner(thread);
}

/* Queue must not be available for use during this call */
void queue_init(struct event_queue *q, bool register_queue)
{
    int oldlevel = disable_irq_save();

    if(register_queue)
        corelock_lock(&all_queues.cl);

    corelock_init(&q->cl);
    wait_queue_init(&q->queue);
    /* What garbage is in write is irrelevant because of the masking design-
     * any other functions the empty the queue do this as well so that
     * queue_count and queue_empty return sane values in the case of a
     * concurrent change without locking inside them. */
    q->read = q->write;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    q->send = NULL; /* No message sending by default */
    IF_PRIO( q->blocker_p = NULL; )
#endif

    if(register_queue)
    {
        void **queues = (void **)all_queues.queues;
        void **p = find_array_ptr(queues, q);

        if(p - queues >= MAX_NUM_QUEUES)
        {
            panicf("queue_init->out of queues");
        }

        if(*p == NULL)
        {
            /* Add it to the all_queues array */
            *p = q;
            corelock_unlock(&all_queues.cl);
        }
    }

    restore_irq(oldlevel);
}

/* Queue must not be available for use during this call */
void queue_delete(struct event_queue *q)
{
    int oldlevel = disable_irq_save();
    corelock_lock(&all_queues.cl);
    corelock_lock(&q->cl);

    /* Remove the queue if registered */
    remove_array_ptr((void **)all_queues.queues, q);

    corelock_unlock(&all_queues.cl);

    /* Release thread(s) waiting on queue head */
    wait_queue_wake(&q->queue);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send)
    {
        /* Release threads waiting for replies */
        queue_release_all_senders(q);

        /* Reply to any dequeued message waiting for one */
        queue_do_auto_reply(q->send);

        q->send = NULL;
        IF_PRIO( q->blocker_p = NULL; )
    }
#endif

    q->read = q->write;

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
                  QUEUE_GET_THREAD(q) == __running_self_entry(),
                  "queue_wait->wrong thread\n");
#endif

    oldlevel = disable_irq_save();

    ASSERT_CPU_MODE(CPU_MODE_THREAD_CONTEXT, oldlevel);

    corelock_lock(&q->cl);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    /* Auto-reply (even if ev is NULL to avoid stalling a waiting thread) */
    queue_do_auto_reply(q->send);
#endif

    while(1)
    {
        rd = q->read;
        if (rd != q->write) /* A waking message could disappear */
            break;

        struct thread_entry *current = __running_self_entry();
        block_thread(current, TIMEOUT_BLOCK, &q->queue, NULL);

        corelock_unlock(&q->cl);
        switch_thread();

        disable_irq();
        corelock_lock(&q->cl);
    } 

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(ev)
#endif
    {
        q->read = rd + 1;
        rd &= QUEUE_LENGTH_MASK;
        *ev = q->events[rd];

        /* Get data for a waiting thread if one */
        queue_do_fetch_sender(q->send, rd);
    }
    /* else just waiting on non-empty */

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

void queue_wait_w_tmo(struct event_queue *q, struct queue_event *ev, int ticks)
{
    int oldlevel;
    unsigned int rd, wr;

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    KERNEL_ASSERT(QUEUE_GET_THREAD(q) == NULL ||
                  QUEUE_GET_THREAD(q) == __running_self_entry(),
                  "queue_wait_w_tmo->wrong thread\n");
#endif

    oldlevel = disable_irq_save();

    corelock_lock(&q->cl);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    /* Auto-reply (even if ev is NULL to avoid stalling a waiting thread) */
    queue_do_auto_reply(q->send);
#endif

    rd = q->read;
    wr = q->write;

    if(rd != wr || ticks == 0)
        ; /* no block */
    else while(1)
    {
        ASSERT_CPU_MODE(CPU_MODE_THREAD_CONTEXT, oldlevel);

        struct thread_entry *current = __running_self_entry();
        block_thread(current, ticks, &q->queue, NULL);

        corelock_unlock(&q->cl);
        switch_thread();

        disable_irq();
        corelock_lock(&q->cl);

        rd = q->read;
        wr = q->write;

        if(rd != wr)
            break;

        if(ticks < 0)
            continue; /* empty again, infinite block */

        /* timeout is legit if thread is still queued and awake */
        if(LIKELY(wait_queue_try_remove(current)))
            break;

        /* we mustn't return earlier than expected wait time */
        ticks = get_tmo_tick(current) - current_tick;
        if(ticks <= 0)
            break;
    }

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(UNLIKELY(!ev))
        ; /* just waiting for something */
    else
#endif
    if(rd != wr)
    {
        q->read = rd + 1;
        rd &= QUEUE_LENGTH_MASK;
        *ev = q->events[rd];

        /* Get data for a waiting thread if one */
        queue_do_fetch_sender(q->send, rd);
    }
    else
    {
        ev->id   = SYS_TIMEOUT;
        ev->data = 0;
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

    KERNEL_ASSERT((q->write - q->read) <= QUEUE_LENGTH,
                  "queue_post ovf q=%p", q);

    q->events[wr].id   = id;
    q->events[wr].data = data;

    /* overflow protect - unblock any thread waiting at this index */
    queue_do_unblock_sender(q->send, wr);

    /* Wakeup a waiting thread if any */
    queue_wake_waiter(q);

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

    ASSERT_CPU_MODE(CPU_MODE_THREAD_CONTEXT, oldlevel);

    corelock_lock(&q->cl);

    wr = q->write++ & QUEUE_LENGTH_MASK;

    KERNEL_ASSERT((q->write - q->read) <= QUEUE_LENGTH,
                  "queue_send ovf q=%p", q);

    q->events[wr].id   = id;
    q->events[wr].data = data;
    
    if(LIKELY(q->send))
    {
        struct queue_sender_list *send = q->send;
        struct thread_entry **spp = &send->senders[wr];
        struct thread_entry *current = __running_self_entry();

        /* overflow protect - unblock any thread waiting at this index */
        queue_release_sender(spp, 0);

        /* Wakeup a waiting thread if any */
        queue_wake_waiter(q);

        /* Save thread in slot, add to list and wait for reply */
        *spp = current;
        block_thread(current, TIMEOUT_BLOCK, &send->list, q->blocker_p);

        corelock_unlock(&q->cl);
        switch_thread();

        return current->retval;
    }

    /* Function as queue_post if sending is not enabled */
    queue_wake_waiter(q);

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

        struct queue_sender_list *send = q->send;
        if(send)
            queue_release_sender(&send->curr_sender, retval);

        corelock_unlock(&q->cl);
        restore_irq(oldlevel);
    }
}
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/* Scan the even queue from head to tail, returning any event from the
   filter list that was found, optionally removing the event. If an
   event is returned, synchronous events are handled in the same manner as
   with queue_wait(_w_tmo); if discarded, then as queue_clear.
   If filters are NULL, any event matches. If filters exist, the default
   is to search the full queue depth.
   Earlier filters take precedence.

   Return true if an event was found, false otherwise. */
bool queue_peek_ex(struct event_queue *q, struct queue_event *ev,
                   unsigned int flags, const long (*filters)[2])
{
    bool have_msg;
    unsigned int rd, wr;
    int oldlevel;

    if(LIKELY(q->read == q->write))
        return false; /* Empty: do nothing further */

    have_msg = false;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* Starting at the head, find first match  */
    for(rd = q->read, wr = q->write; rd != wr; rd++)
    {
        struct queue_event *e = &q->events[rd & QUEUE_LENGTH_MASK];

        if(filters)
        {
            /* Have filters - find the first thing that passes */
            const long (* f)[2] = filters;
            const long (* const f_last)[2] =
                &filters[flags & QPEEK_FILTER_COUNT_MASK];
            long id = e->id;

            do
            {
                if(UNLIKELY(id >= (*f)[0] && id <= (*f)[1]))
                    goto passed_filter;
            }
            while(++f <= f_last);

            if(LIKELY(!(flags & QPEEK_FILTER_HEAD_ONLY)))
                continue;   /* No match; test next event */
            else
                break;      /* Only check the head */
        }
        /* else - anything passes */

    passed_filter:

        /* Found a matching event */
        have_msg = true;

        if(ev)
            *ev = *e;       /* Caller wants the event */

        if(flags & QPEEK_REMOVE_EVENTS)
        {
            /* Do event removal */
            unsigned int r = q->read;
            q->read = r + 1; /* Advance head */

            if(ev)
            {
                /* Auto-reply */
                queue_do_auto_reply(q->send);
                /* Get the thread waiting for reply, if any */
                queue_do_fetch_sender(q->send, rd & QUEUE_LENGTH_MASK);
            }
            else
            {
                /* Release any thread waiting on this message */
                queue_do_unblock_sender(q->send, rd & QUEUE_LENGTH_MASK);
            }

            /* Slide messages forward into the gap if not at the head */
            while(rd != r)
            {
                unsigned int dst = rd & QUEUE_LENGTH_MASK;
                unsigned int src = --rd & QUEUE_LENGTH_MASK;

                q->events[dst] = q->events[src];
                /* Keep sender wait list in sync */
                if(q->send)
                    q->send->senders[dst] = q->send->senders[src];
            }
        }

        break;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);

    return have_msg;
}

bool queue_peek(struct event_queue *q, struct queue_event *ev)
{
    return queue_peek_ex(q, ev, 0, NULL);
}

void queue_remove_from_head(struct event_queue *q, long id)
{
    const long f[2] = { id, id };
    while (queue_peek_ex(q, NULL,
            QPEEK_FILTER_HEAD_ONLY | QPEEK_REMOVE_EVENTS, &f));
}
#else /* !HAVE_EXTENDED_MESSAGING_AND_NAME */
/* The more powerful routines aren't required */
bool queue_peek(struct event_queue *q, struct queue_event *ev)
{
    unsigned int rd;

    if(q->read == q->write)
         return false;

    bool have_msg = false;

    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    rd = q->read;
    if(rd != q->write)
    {
        *ev = q->events[rd & QUEUE_LENGTH_MASK];
        have_msg = true;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);

    return have_msg;
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
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

/* Poll queue to see if a message exists - careful in using the result if
 * queue_remove_from_head is called when messages are posted - possibly use
 * queue_wait_w_tmo(&q, 0) in that case or else a removed message that
 * unsignals the queue may cause an unwanted block */
bool queue_empty(const struct event_queue* q)
{
    return ( q->read == q->write );
}

/* Poll queue to see if it is full */
bool queue_full(const struct event_queue* q)
{
    return ((q->write - q->read) >= QUEUE_LENGTH);
}

void queue_clear(struct event_queue* q)
{
    int oldlevel;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* Release all threads waiting in the queue for a reply -
       dequeued sent message will be handled by owning thread */
    queue_release_all_senders(q);

    q->read = q->write;

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
    struct event_queue **p = all_queues.queues;
    struct event_queue *q;

#if NUM_CORES > 1
    int oldlevel = disable_irq_save();
    corelock_lock(&all_queues.cl);
#endif

    for(q = *p; q != NULL; q = *(++p))
    {
        queue_post(q, id, data);
    }

#if NUM_CORES > 1
    corelock_unlock(&all_queues.cl);
    restore_irq(oldlevel);
#endif
   
    return p - all_queues.queues;
}

void init_queues(void)
{
    corelock_init(&all_queues.cl);
}
