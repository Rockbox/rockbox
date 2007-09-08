/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdlib.h>
#include "memory.h"
#include "uisdl.h"
#include "kernel.h"
#include "thread-sdl.h"
#include "thread.h"
#include "debug.h"

volatile long current_tick = 0;
static void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct event_queue *all_queues[32];
static int num_queues = 0;

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/* Moves waiting thread's descriptor to the current sender when a
   message is dequeued */
static void queue_fetch_sender(struct queue_sender_list *send,
                               unsigned int i)
{
    struct thread_entry **spp = &send->senders[i];

    if(*spp)
    {
        send->curr_sender = *spp;
        *spp = NULL;
    }
}

/* Puts the specified return value in the waiting thread's return value
   and wakes the thread  - a sender should be confirmed to exist first */
static void queue_release_sender(struct thread_entry **sender,
                                 intptr_t retval)
{
    (*sender)->retval = retval;
    wakeup_thread(sender);
    if(*sender != NULL)
    {
        fprintf(stderr, "queue->send slot ovf: %p\n", *sender);
        exit(-1);
    }
}

/* Releases any waiting threads that are queued with queue_send -
   reply with NULL */
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

/* Enables queue_send on the specified queue - caller allocates the extra
   data structure */
void queue_enable_queue_send(struct event_queue *q,
                             struct queue_sender_list *send)
{
    q->send = NULL;
    if(send)
    {
        q->send = send;
        memset(send, 0, sizeof(*send));
    }
}
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

void queue_init(struct event_queue *q, bool register_queue)
{
    q->read   = 0;
    q->write  = 0;
    q->thread = NULL;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    q->send   = NULL; /* No message sending by default */
#endif

    if(register_queue)
    {
        if(num_queues >= 32)
        {
            fprintf(stderr, "queue_init->out of queues");
            exit(-1);
        }
        /* Add it to the all_queues array */
        all_queues[num_queues++] = q;
    }
}

void queue_delete(struct event_queue *q)
{
    int i;
    bool found = false;

    /* Find the queue to be deleted */
    for(i = 0;i < num_queues;i++)
    {
        if(all_queues[i] == q)
        {
            found = true;
            break;
        }
    }

    if(found)
    {
        /* Move the following queues up in the list */
        for(;i < num_queues-1;i++)
        {
            all_queues[i] = all_queues[i+1];
        }
        
        num_queues--;
    }

    /* Release threads waiting on queue head */
    wakeup_thread(&q->thread);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    /* Release waiting threads and reply to any dequeued message
       waiting for one. */
    queue_release_all_senders(q);
    queue_reply(q, 0);
#endif

    q->read = 0;
    q->write = 0;
}

void queue_wait(struct event_queue *q, struct event *ev)
{
    unsigned int rd;

    if (q->read == q->write)
    {
        block_thread(&q->thread);
    }

    rd = q->read++ & QUEUE_LENGTH_MASK;
    *ev = q->events[rd];

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send && q->send->senders[rd])
    {
        /* Get data for a waiting thread if one */
        queue_fetch_sender(q->send, rd);
    }
#endif
}

void queue_wait_w_tmo(struct event_queue *q, struct event *ev, int ticks)
{
    if (q->read == q->write && ticks > 0)
    {
        block_thread_w_tmo(&q->thread, ticks);
    }

    if(q->read != q->write)
    {
        unsigned int rd = q->read++ & QUEUE_LENGTH_MASK;
        *ev = q->events[rd];

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
        if(q->send && q->send->senders[rd])
        {
            /* Get data for a waiting thread if one */
            queue_fetch_sender(q->send, rd);
        }
#endif
    }
    else
    {
        ev->id = SYS_TIMEOUT;
    }
}

void queue_post(struct event_queue *q, long id, intptr_t data)
{
    unsigned int wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send)
    {
        struct thread_entry **spp = &q->send->senders[wr];

        if(*spp)
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, 0);
        }
    }
#endif

    wakeup_thread(&q->thread);
}

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
intptr_t queue_send(struct event_queue *q, long id, intptr_t data)
{
    unsigned int wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;

    if(q->send)
    {
        struct thread_entry **spp = &q->send->senders[wr];

        if(*spp)
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, 0);
        }

        wakeup_thread(&q->thread);

        block_thread(spp);
        return thread_get_current()->retval;
    }

    /* Function as queue_post if sending is not enabled */
    return 0;
}

#if 0 /* not used now but probably will be later */
/* Query if the last message dequeued was added by queue_send or not */
bool queue_in_queue_send(struct event_queue *q)
{
    return q->send && q->send->curr_sender;
}
#endif

/* Replies with retval to any dequeued message sent with queue_send */
void queue_reply(struct event_queue *q, intptr_t retval)
{
    if(q->send && q->send->curr_sender)
    {
        queue_release_sender(&q->send->curr_sender, retval);
    }
}
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

bool queue_empty(const struct event_queue* q)
{
    return ( q->read == q->write );
}

void queue_clear(struct event_queue* q)
{
    /* fixme: This is potentially unsafe in case we do interrupt-like processing */
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    /* Release all thread waiting in the queue for a reply -
       dequeued sent message will be handled by owning thread */
    queue_release_all_senders(q);
#endif
    q->read = 0;
    q->write = 0;
}

void queue_remove_from_head(struct event_queue *q, long id)
{
    while(q->read != q->write)
    {
        unsigned int rd = q->read & QUEUE_LENGTH_MASK;

        if(q->events[rd].id != id)
        {
            break;
        }

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
        if(q->send)
        {
            struct thread_entry **spp = &q->send->senders[rd];

            if(*spp)
            {
                /* Release any thread waiting on this message */
                queue_release_sender(spp, 0);
            }
        }
#endif
        q->read++;
    }
}

int queue_count(const struct event_queue *q)
{
    return q->write - q->read;
}

int queue_broadcast(long id, intptr_t data)
{
    int i;
    
    for(i = 0;i < num_queues;i++)
    {
        queue_post(all_queues[i], id, data);
    }
   
    return num_queues;
}

void yield(void)
{
    switch_thread(true, NULL);
}

void sleep(int ticks)
{
    sleep_thread(ticks);
}

void sim_tick_tasks(void)
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
}

int tick_add_task(void (*f)(void))
{
    int i;

    /* Add a task if there is room */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == NULL)
        {
            tick_funcs[i] = f;
            return 0;
        }
    }
    fprintf(stderr, "Error! tick_add_task(): out of tasks");
    exit(-1);
    return -1;
}

int tick_remove_task(void (*f)(void))
{
    int i;

    /* Remove a task if it is there */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == f)
        {
            tick_funcs[i] = NULL;
            return 0;
        }
    }
    
    return -1;
}

/* Very simple mutex simulation - won't work with pre-emptive
   multitasking, but is better than nothing at all */
void mutex_init(struct mutex *m)
{
    m->thread = NULL;
    m->locked = 0;
}

void mutex_lock(struct mutex *m)
{
    if (test_and_set(&m->locked, 1))
    {
        block_thread(&m->thread);
    }
}

void mutex_unlock(struct mutex *m)
{
    if (m->thread != NULL)
    {
        wakeup_thread(&m->thread);
    }
    else
    {
        m->locked = 0;
    }
}

void spinlock_lock(struct mutex *l)
{
    while(test_and_set(&l->locked, 1))
    {
        switch_thread(true, NULL);
    }
}

void spinlock_unlock(struct mutex *l)
{
    l->locked = 0;
}
