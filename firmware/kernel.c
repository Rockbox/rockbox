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
#include "thread.h"
#include "cpu.h"
#include "system.h"
#include "panic.h"

#if !defined(CPU_PP) || !defined(BOOTLOADER) 
long current_tick = 0;
#endif

static void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct event_queue *all_queues[32];
static int num_queues;

void queue_wait(struct event_queue *q, struct event *ev) ICODE_ATTR;

/****************************************************************************
 * Standard kernel stuff
 ****************************************************************************/
void kernel_init(void)
{
    /* Init the threading API */
    init_threads();
    
    memset(tick_funcs, 0, sizeof(tick_funcs));

    num_queues = 0;
    memset(all_queues, 0, sizeof(all_queues));

    tick_start(1000/HZ);
}

void sleep(int ticks)
{
#if CONFIG_CPU == S3C2440 && defined(BOOTLOADER)
    int counter;
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

#else
    sleep_thread(ticks);
#endif
}

void yield(void)
{
#if (CONFIG_CPU == S3C2440 || defined(ELIO_TPJ1022) && defined(BOOTLOADER))
    /* Some targets don't like yielding in the bootloader */
#else
    switch_thread(true, NULL);
#endif
}

/****************************************************************************
 * Queue handling stuff
 ****************************************************************************/

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/* Moves waiting thread's descriptor to the current sender when a
   message is dequeued */
static void queue_fetch_sender(struct queue_sender_list *send,
                               unsigned int i)
{
    int old_level = set_irq_level(HIGHEST_IRQ_LEVEL);
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
    wakeup_thread(&(*sender)->thread);
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
    q->read   = 0;
    q->write  = 0;
    q->thread = NULL;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    q->send   = NULL; /* No message sending by default */
#endif

    if(register_queue)
    {
        /* Add it to the all_queues array */
        all_queues[num_queues++] = q;
    }
}

void queue_delete(struct event_queue *q)
{
    int i;
    bool found = false;

    wakeup_thread(&q->thread);
    
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
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
        /* Release waiting threads and reply to any dequeued message
           waiting for one. */
        queue_release_all_senders(q);
        queue_reply(q, NULL);
#endif
        /* Move the following queues up in the list */
        for(;i < num_queues-1;i++)
        {
            all_queues[i] = all_queues[i+1];
        }
        
        num_queues--;
    }
}

void queue_wait(struct event_queue *q, struct event *ev)
{
    unsigned int rd;

    if(q->read == q->write)
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
    if(q->read == q->write && ticks > 0)
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

void queue_post(struct event_queue *q, long id, void *data)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
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

    wakeup_thread(&q->thread);
    set_irq_level(oldlevel);
}

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
void * queue_send(struct event_queue *q, long id, void *data)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
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
        sender.thread = NULL;

        wakeup_thread(&q->thread);
        set_irq_level_and_block_thread(&sender.thread, oldlevel);
        return sender.retval;
    }

    /* Function as queue_post if sending is not enabled */
    wakeup_thread(&q->thread);
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
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    /* Release all thread waiting in the queue for a reply -
       dequeued sent message will be handled by owning thread */
    queue_release_all_senders(q);
#endif

    q->read = 0;
    q->write = 0;
    set_irq_level(oldlevel);
}

void queue_remove_from_head(struct event_queue *q, long id)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    
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

int queue_broadcast(long id, void *data)
{
   int i;

   for(i = 0;i < num_queues;i++)
   {
      queue_post(all_queues[i], id, data);
   }
   
   return num_queues;
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

    TIMER1_VAL; /* Read value to ack IRQ */
    /* Run through the list of tick tasks */
    for (i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if (tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;
}
#endif

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

    TIMERR0C = 1;
}

void tick_start(unsigned int interval_in_ms)
{
  TIMERR08 &= ~0x80;
  TIMERR0C = 1;
  TIMERR08 &= ~0x80;
  TIMERR08 |= 0x40;
  TIMERR00 = 3000000 * interval_in_ms / 1000;
  TIMERR08 &= ~0xc;
  TIMERR0C = 1;

  irq_set_int_handler(4, timer_handler);
  irq_enable_int(4);

  TIMERR08 |= 0x80;
}
#elif CONFIG_CPU == S3C2440
void tick_start(unsigned int interval_in_ms)
{
    unsigned long count;

    /* period = (n + 1) / 128 , n = tick time count (1~127)*/
    count = interval_in_ms / 1000 * 128 - 1;

    if(count > 127)
    {
        panicf("Error! The tick interval is too long (%d ms)\n",
               interval_in_ms);
        return;
    }

    /* Disable the tick */
    TICNT &= ~(1<<7);
    /* Set the count value */
    TICNT |= count;
    /* Start up the ticker */
    TICNT |= (1<<7);

    /* need interrupt handler ??? */
    
}
#endif

int tick_add_task(void (*f)(void))
{
    int i;
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Add a task if there is room */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == NULL)
        {
            tick_funcs[i] = f;
            set_irq_level(oldlevel);
            return 0;
        }
    }
    set_irq_level(oldlevel);
    panicf("Error! tick_add_task(): out of tasks");
    return -1;
}

int tick_remove_task(void (*f)(void))
{
    int i;
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Remove a task if it is there */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == f)
        {
            tick_funcs[i] = NULL;
            set_irq_level(oldlevel);
            return 0;
        }
    }
    
    set_irq_level(oldlevel);
    return -1;
}

#ifndef SIMULATOR
/*
 * Simulator versions in uisimulator/SIMVER/
 */

/****************************************************************************
 * Simple mutex functions
 ****************************************************************************/
void mutex_init(struct mutex *m)
{
    m->locked = false;
    m->thread = NULL;
}

void mutex_lock(struct mutex *m)
{
    if (m->locked)
    {
        /* Wait until the lock is open... */
        block_thread(&m->thread);
    }
    
    /* ...and lock it */
    m->locked = true;
}

void mutex_unlock(struct mutex *m)
{
    if (m->thread == NULL)
        m->locked = false;
    else
        wakeup_thread(&m->thread);
}

#endif
