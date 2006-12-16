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

static void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

int set_irq_level (int level)
{
    static int _lv = 0;
    return (_lv = level);
}

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/* Moves waiting thread's descriptor to the current sender when a
   message is dequeued */
static void queue_fetch_sender(struct queue_sender_list *send,
                               unsigned int i)
{
    int old_level = set_irq_level(15<<4);
    struct queue_sender **spp = &send->senders[i];

    if(*spp)
    {
        send->curr_sender = *spp;
        *spp = NULL;
    }

    set_irq_level(old_level);
}

/* Puts the specified return value in the waiting thread's return value
   and wakes the thread  - a sender should be confirmed to exist first */
static void queue_release_sender(struct queue_sender **sender, void *retval)
{
    (*sender)->retval = retval;
    *sender = NULL;
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
            struct queue_sender **spp =
                &q->send->senders[i & QUEUE_LENGTH_MASK];
            if(*spp)
            {
                queue_release_sender(spp, NULL);
            }
        }
    }
}

/* Enables queue_send on the specified queue - caller allocates the extra
   data structure */
void queue_enable_queue_send(struct event_queue *q,
                             struct queue_sender_list *send)
{
    q->send = send;
    memset(send, 0, sizeof(*send));
}
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

void queue_init(struct event_queue *q, bool register_queue)
{
    (void)register_queue;
    
    q->read   = 0;
    q->write  = 0;
    q->thread = NULL;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    q->send   = NULL; /* No message sending by default */
#endif
}

void queue_delete(struct event_queue *q)
{
    (void)q;
}

void queue_wait(struct event_queue *q, struct event *ev)
{
    unsigned int rd;

    while(q->read == q->write)
    {
        switch_thread(true, NULL);
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
    unsigned int timeout = current_tick + ticks;

    while(q->read == q->write && TIME_BEFORE( current_tick, timeout ))
    {
        sim_sleep(1);
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

void queue_post(struct event_queue *q, long id, void *data)
{
    int oldlevel = set_irq_level(15<<4);
    unsigned int wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send)
    {
        struct queue_sender **spp = &q->send->senders[wr];

        if(*spp)
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, NULL);
        }
    }
#endif

    set_irq_level(oldlevel);
}

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
void * queue_send(struct event_queue *q, long id, void *data)
{
    int oldlevel = set_irq_level(15<<4);
    unsigned int wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;

    if(q->send)
    {
        struct queue_sender **spp = &q->send->senders[wr];
        struct queue_sender sender;

        if(*spp)
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, NULL);
        }

        *spp = &sender;

        set_irq_level(oldlevel);
        while (*spp != NULL)
        {
            switch_thread(true, NULL);
        }

        return sender.retval;
    }

    /* Function as queue_post if sending is not enabled */
    set_irq_level(oldlevel);
    return NULL;
}

#if 0 /* not used now but probably will be later */
/* Query if the last message dequeued was added by queue_send or not */
bool queue_in_queue_send(struct event_queue *q)
{
    return q->send && q->send->curr_sender;
}
#endif

/* Replies with retval to any dequeued message sent with queue_send */
void queue_reply(struct event_queue *q, void *retval)
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
    int oldlevel = set_irq_level(15<<4);
    
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
            struct queue_sender **spp = &q->send->senders[rd];

            if(*spp)
            {
                /* Release any thread waiting on this message */
                queue_release_sender(spp, NULL);
            }
        }
#endif
        q->read++;
    }
    
    set_irq_level(oldlevel);
}

void switch_thread(bool save_context, struct thread_entry **blocked_list)
{
    (void)save_context;
    (void)blocked_list;
    
    yield ();
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
    DEBUGF("Error! tick_add_task(): out of tasks");
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
    m->locked = false;
}

void mutex_lock(struct mutex *m)
{
    while(m->locked)
        switch_thread(true, NULL);
    m->locked = true;
}

void mutex_unlock(struct mutex *m)
{
    m->locked = false;
}
