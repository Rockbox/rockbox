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
#if CONFIG_CPU == IMX31L
#include "avic-imx31.h"
#endif

/* Make this nonzero to enable more elaborate checks on objects */
#ifdef DEBUG
#define KERNEL_OBJECT_CHECKS 1 /* Always 1 for DEBUG */
#else
#define KERNEL_OBJECT_CHECKS 0
#endif

#if KERNEL_OBJECT_CHECKS
#define KERNEL_ASSERT(exp, msg...) \
    ({ if (!({ exp; })) panicf(msg); })
#else
#define KERNEL_ASSERT(exp, msg...) ({})
#endif

#if (!defined(CPU_PP) && (CONFIG_CPU != IMX31L)) || !defined(BOOTLOADER) 
volatile long current_tick NOCACHEDATA_ATTR = 0;
#endif

void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

extern struct core_entry cores[NUM_CORES];

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct
{
    int count;
    struct event_queue *queues[MAX_NUM_QUEUES];
#if NUM_CORES > 1
    struct corelock cl;
#endif
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
        switch_thread(NULL);
#else
    sleep_thread(ticks);
#endif
}

void yield(void)
{
#if ((CONFIG_CPU == S3C2440 || defined(ELIO_TPJ1022) || CONFIG_CPU == IMX31L) && defined(BOOTLOADER))
    /* Some targets don't like yielding in the bootloader */
#else
    switch_thread(NULL);
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
    struct thread_entry **spp = &send->senders[i];

    if(*spp)
    {
        send->curr_sender = *spp;
        *spp = NULL;
    }
}

/* Puts the specified return value in the waiting thread's return value
 * and wakes the thread.
 * 1) A sender should be confirmed to exist before calling which makes it
 *    more efficent to reject the majority of cases that don't need this
      called.
 * 2) Requires interrupts disabled since queue overflows can cause posts
 *    from interrupt handlers to wake threads. Not doing so could cause
 *    an attempt at multiple wakes or other problems.
 */
static void queue_release_sender(struct thread_entry **sender,
                                 intptr_t retval)
{
    (*sender)->retval = retval;
    wakeup_thread_no_listlock(sender);
    /* This should _never_ happen - there must never be multiple
       threads in this list and it is a corrupt state */
    KERNEL_ASSERT(*sender == NULL, "queue->send slot ovf: %08X", (int)*sender);
}

/* Releases any waiting threads that are queued with queue_send -
 * reply with 0.
 * Disable IRQs and lock before calling since it uses
 * queue_release_sender.
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

/* Enables queue_send on the specified queue - caller allocates the extra
   data structure. Only queues which are taken to be owned by a thread should
   enable this. Public waiting is not permitted. */
void queue_enable_queue_send(struct event_queue *q,
                             struct queue_sender_list *send)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);

    q->send = NULL;
    if(send != NULL)
    {
        memset(send, 0, sizeof(*send));
        q->send = send;
    }

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
}
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

/* Queue must not be available for use during this call */
void queue_init(struct event_queue *q, bool register_queue)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    if(register_queue)
    {
        corelock_lock(&all_queues.cl);
    }

    corelock_init(&q->cl);
    thread_queue_init(&q->queue);
    q->read   = 0;
    q->write  = 0;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    q->send   = NULL; /* No message sending by default */
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

    set_irq_level(oldlevel);
}

/* Queue must not be available for use during this call */
void queue_delete(struct event_queue *q)
{
    int oldlevel;
    int i;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
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

    /* Release threads waiting on queue head */
    thread_queue_wake(&q->queue);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    /* Release waiting threads for reply and reply to any dequeued
       message waiting for one. */
    queue_release_all_senders(q);
    queue_reply(q, 0);
#endif

    q->read = 0;
    q->write = 0;

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
}

/* NOTE: multiple threads waiting on a queue head cannot have a well-
   defined release order if timeouts are used. If multiple threads must
   access the queue head, use a dispatcher or queue_wait only. */
void queue_wait(struct event_queue *q, struct queue_event *ev)
{
    int oldlevel;
    unsigned int rd;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send && q->send->curr_sender)
    {
        /* auto-reply */
        queue_release_sender(&q->send->curr_sender, 0);
    }
#endif
    
    if (q->read == q->write)
    {
        do
        {
#if CONFIG_CORELOCK == CORELOCK_NONE
            cores[CURRENT_CORE].irq_level = oldlevel;
#elif CONFIG_CORELOCK == SW_CORELOCK
            const unsigned int core = CURRENT_CORE;
            cores[core].blk_ops.irq_level = oldlevel;
            cores[core].blk_ops.flags = TBOP_UNLOCK_CORELOCK | TBOP_IRQ_LEVEL;
            cores[core].blk_ops.cl_p = &q->cl;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
            const unsigned int core = CURRENT_CORE;
            cores[core].blk_ops.irq_level = oldlevel;
            cores[core].blk_ops.flags = TBOP_SET_VARu8 | TBOP_IRQ_LEVEL;
            cores[core].blk_ops.var_u8p = &q->cl.locked;
            cores[core].blk_ops.var_u8v = 0;
#endif /* CONFIG_CORELOCK */
            block_thread(&q->queue);
            oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
            corelock_lock(&q->cl);
        }
        /* A message that woke us could now be gone */
        while (q->read == q->write);
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

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
}

void queue_wait_w_tmo(struct event_queue *q, struct queue_event *ev, int ticks)
{
    int oldlevel;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if (q->send && q->send->curr_sender)
    {
        /* auto-reply */
        queue_release_sender(&q->send->curr_sender, 0);
    }
#endif

    if (q->read == q->write && ticks > 0)
    {
#if CONFIG_CORELOCK == CORELOCK_NONE
        cores[CURRENT_CORE].irq_level = oldlevel;
#elif CONFIG_CORELOCK == SW_CORELOCK
        const unsigned int core = CURRENT_CORE;
        cores[core].blk_ops.irq_level = oldlevel;
        cores[core].blk_ops.flags = TBOP_UNLOCK_CORELOCK | TBOP_IRQ_LEVEL;
        cores[core].blk_ops.cl_p  = &q->cl;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
        const unsigned int core = CURRENT_CORE;
        cores[core].blk_ops.irq_level = oldlevel;
        cores[core].blk_ops.flags = TBOP_SET_VARu8 | TBOP_IRQ_LEVEL;
        cores[core].blk_ops.var_u8p = &q->cl.locked;
        cores[core].blk_ops.var_u8v = 0;
#endif
        block_thread_w_tmo(&q->queue, ticks);
        oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
        corelock_lock(&q->cl);
    }

    /* no worry about a removed message here - status is checked inside
       locks - perhaps verify if timeout or false alarm */
    if (q->read != q->write)
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

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
}

void queue_post(struct event_queue *q, long id, intptr_t data)
{
    int oldlevel;
    unsigned int wr;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);

    wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send)
    {
        struct thread_entry **spp = &q->send->senders[wr];

        if (*spp)
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, 0);
        }
    }
#endif

    /* Wakeup a waiting thread if any */
    wakeup_thread(&q->queue);

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
}

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/* IRQ handlers are not allowed use of this function - we only aim to
   protect the queue integrity by turning them off. */
intptr_t queue_send(struct event_queue *q, long id, intptr_t data)
{
    int oldlevel;
    unsigned int wr;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);

    wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;
    
    if(q->send)
    {
        const unsigned int core = CURRENT_CORE;
        struct thread_entry **spp = &q->send->senders[wr];

        if(*spp)
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, 0);
        }

        /* Wakeup a waiting thread if any */
        wakeup_thread(&q->queue);

#if CONFIG_CORELOCK == CORELOCK_NONE
        cores[core].irq_level = oldlevel;
#elif CONFIG_CORELOCK == SW_CORELOCK
        cores[core].blk_ops.irq_level = oldlevel;
        cores[core].blk_ops.flags = TBOP_UNLOCK_CORELOCK | TBOP_IRQ_LEVEL;
        cores[core].blk_ops.cl_p  = &q->cl; 
#elif CONFIG_CORELOCK == CORELOCK_SWAP
        cores[core].blk_ops.irq_level = oldlevel;
        cores[core].blk_ops.flags = TBOP_SET_VARu8 | TBOP_IRQ_LEVEL;
        cores[core].blk_ops.var_u8p = &q->cl.locked;
        cores[core].blk_ops.var_u8v = 0;
#endif
        block_thread_no_listlock(spp);
        return cores[core].running->retval;
    }

    /* Function as queue_post if sending is not enabled */
    wakeup_thread(&q->queue);

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
    
    return 0;
}

#if 0 /* not used now but probably will be later */
/* Query if the last message dequeued was added by queue_send or not */
bool queue_in_queue_send(struct event_queue *q)
{
    bool in_send;

#if NUM_CORES > 1
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);
#endif

    in_send = q->send && q->send->curr_sender;

#if NUM_CORES > 1
    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
#endif

    return in_send;
}
#endif

/* Replies with retval to the last dequeued message sent with queue_send */
void queue_reply(struct event_queue *q, intptr_t retval)
{
    if(q->send && q->send->curr_sender)
    {
#if NUM_CORES > 1
        int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
        corelock_lock(&q->cl);
        /* Double-check locking */
        if(q->send && q->send->curr_sender)
        {
#endif

            queue_release_sender(&q->send->curr_sender, retval);

#if NUM_CORES > 1
        }
        corelock_unlock(&q->cl);
        set_irq_level(oldlevel);
#endif
    }
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

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    /* Release all threads waiting in the queue for a reply -
       dequeued sent message will be handled by owning thread */
    queue_release_all_senders(q);
#endif

    q->read = 0;
    q->write = 0;

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
}

void queue_remove_from_head(struct event_queue *q, long id)
{
    int oldlevel;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&q->cl);

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

            if (*spp)
            {
                /* Release any thread waiting on this message */
                queue_release_sender(spp, 0);
            }
        }
#endif
        q->read++;
    }

    corelock_unlock(&q->cl);
    set_irq_level(oldlevel);
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
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    corelock_lock(&all_queues.cl);
#endif
    
    for(i = 0;i < all_queues.count;i++)
    {
        queue_post(all_queues.queues[i], id, data);
    }

#if NUM_CORES > 1
    corelock_unlock(&all_queues.cl);
    set_irq_level(oldlevel);
#endif
   
    return i;
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
#elif CONFIG_CPU == IMX31L
void tick_start(unsigned int interval_in_ms)
{
    EPITCR1 &= ~0x1;        /* Disable the counter */

    EPITCR1 &= ~0xE;        /* Disable interrupt, count down from 0xFFFFFFFF */
    EPITCR1 &= ~0xFFF0;     /* Clear prescaler */
#ifdef BOOTLOADER
    EPITCR1 |= (2700 << 2); /* Prescaler = 2700 */
#endif
    EPITCR1 &= ~(0x3 << 24);
    EPITCR1 |= (0x2 << 24); /* Set clock source to external clock (27mhz) */
    EPITSR1 = 1;            /* Clear the interrupt request */
#ifndef BOOTLOADER
    EPITLR1 = 27000000 * interval_in_ms / 1000;
    EPITCMPR1 = 27000000 * interval_in_ms / 1000;
#else
    (void)interval_in_ms;
#endif

    //avic_enable_int(EPIT1, IRQ, EPIT_HANDLER);

    EPITCR1 |= 0x1;           /* Enable the counter */
}

#ifndef BOOTLOADER
void EPIT_HANDLER(void) __attribute__((interrupt("IRQ")));
void EPIT_HANDLER(void) {
    int i;

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
            tick_funcs[i]();
    }

    current_tick++;

    EPITSR1 = 1;            /* Clear the interrupt request */
}
#endif
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
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

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

    set_irq_level(oldlevel);
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

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

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

    set_irq_level(oldlevel);
}

#endif /* INCLUDE_TIMEOUT_API */

/****************************************************************************
 * Simple mutex functions ;)
 ****************************************************************************/
void mutex_init(struct mutex *m)
{
    m->queue = NULL;
    m->thread = NULL;
    m->count = 0;
    m->locked = 0;
#if CONFIG_CORELOCK == SW_CORELOCK
    corelock_init(&m->cl);
#endif
}

void mutex_lock(struct mutex *m)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *const thread = cores[core].running;

    if(thread == m->thread)
    {
        m->count++;
        return;
    }

    /* Repeat some stuff here or else all the variation is too difficult to
       read */
#if CONFIG_CORELOCK == CORELOCK_SWAP
    /* peek at lock until it's no longer busy */
    unsigned int locked;
    while ((locked = xchg8(&m->locked, STATE_BUSYu8)) == STATE_BUSYu8);
    if(locked == 0)
    {
        m->thread = thread;
        m->locked = 1;
        return;
    }

    /* Block until the lock is open... */
    cores[core].blk_ops.flags = TBOP_SET_VARu8;
    cores[core].blk_ops.var_u8p = &m->locked;
    cores[core].blk_ops.var_u8v = 1;
#else
    corelock_lock(&m->cl);
    if (m->locked == 0)
    {
        m->locked = 1;
        m->thread = thread;
        corelock_unlock(&m->cl);
        return;
    }

    /* Block until the lock is open... */
#if CONFIG_CORELOCK == SW_CORELOCK
    cores[core].blk_ops.flags = TBOP_UNLOCK_CORELOCK;
    cores[core].blk_ops.cl_p = &m->cl;
#endif
#endif /* CONFIG_CORELOCK */

    block_thread_no_listlock(&m->queue);
}

void mutex_unlock(struct mutex *m)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(m->thread == cores[CURRENT_CORE].running,
                  "mutex_unlock->wrong thread (recurse)");

    if(m->count > 0)
    {
        /* this thread still owns lock */
        m->count--;
        return;
    }

#if CONFIG_CORELOCK == SW_CORELOCK
    /* lock out other cores */
    corelock_lock(&m->cl);
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    /* wait for peeker to move on */
    while (xchg8(&m->locked, STATE_BUSYu8) == STATE_BUSYu8);
#endif

    /* transfer to next queued thread if any */

    /* This can become busy using SWP but is safe since only one thread
       will be changing things at a time. Allowing timeout waits will
       change that however but not now. There is also a hazard the thread
       could be killed before performing the wakeup but that's just
       irresponsible. :-) */
    m->thread = m->queue;

    if(m->thread == NULL)
    {
        m->locked = 0; /* release lock */
#if CONFIG_CORELOCK == SW_CORELOCK
        corelock_unlock(&m->cl);
#endif
    }
    else /* another thread is waiting - remain locked */
    {
        wakeup_thread_no_listlock(&m->queue);
#if CONFIG_CORELOCK == SW_CORELOCK
        corelock_unlock(&m->cl);
#elif CONFIG_CORELOCK == CORELOCK_SWAP
        m->locked = 1;
#endif
    }
}

/****************************************************************************
 * Simpl-er mutex functions ;)
 ****************************************************************************/
void spinlock_init(struct spinlock *l IF_COP(, unsigned int flags))
{
    l->locked = 0;
    l->thread = NULL;
    l->count = 0;
#if NUM_CORES > 1
    l->task_switch = flags & SPINLOCK_TASK_SWITCH;
    corelock_init(&l->cl);
#endif
}

void spinlock_lock(struct spinlock *l)
{
    struct thread_entry *const thread = cores[CURRENT_CORE].running;

    if (l->thread == thread)
    {
        l->count++;
        return;
    }

#if NUM_CORES > 1
    if (l->task_switch != 0)
#endif
    {
        /* Let other threads run until the lock is free */
        while(test_and_set(&l->locked, 1, &l->cl) != 0)
        {
            /* spin and switch until the lock is open... */
            switch_thread(NULL);
        }
    }
#if NUM_CORES > 1
    else
    {
        /* Use the corelock purely */
        corelock_lock(&l->cl);
    }
#endif

    l->thread = thread;
}

void spinlock_unlock(struct spinlock *l)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(l->thread == cores[CURRENT_CORE].running,
                  "spinlock_unlock->wrong thread");

    if (l->count > 0)
    {
        /* this thread still owns lock */
        l->count--;
        return;
    }

    /* clear owner */
    l->thread = NULL;

#if NUM_CORES > 1
    if (l->task_switch != 0)
#endif
    {
        /* release lock */
#if CONFIG_CORELOCK == SW_CORELOCK
        /* This must be done since our unlock could be missed by the
           test_and_set and leave the object locked permanently */
        corelock_lock(&l->cl);
#endif
        l->locked = 0;
    }

#if NUM_CORES > 1
    corelock_unlock(&l->cl);
#endif
}

/****************************************************************************
 * Simple semaphore functions ;)
 ****************************************************************************/
#ifdef HAVE_SEMAPHORE_OBJECTS
void semaphore_init(struct semaphore *s, int max, int start)
{
    KERNEL_ASSERT(max > 0 && start >= 0 && start <= max,
                  "semaphore_init->inv arg");
    s->queue = NULL;
    s->max = max;
    s->count = start;
#if CONFIG_CORELOCK == SW_CORELOCK
    corelock_init(&s->cl);
#endif
}

void semaphore_wait(struct semaphore *s)
{
#if CONFIG_CORELOCK == CORELOCK_NONE || CONFIG_CORELOCK == SW_CORELOCK
    corelock_lock(&s->cl);
    if(--s->count >= 0)
    {
        corelock_unlock(&s->cl);
        return;
    }
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    int count;
    while ((count = xchg32(&s->count, STATE_BUSYi)) == STATE_BUSYi);
    if(--count >= 0)
    {
        s->count = count;
        return;
    }
#endif

    /* too many waits - block until dequeued */
#if CONFIG_CORELOCK == SW_CORELOCK
    const unsigned int core = CURRENT_CORE;
    cores[core].blk_ops.flags = TBOP_UNLOCK_CORELOCK;
    cores[core].blk_ops.cl_p = &s->cl;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    const unsigned int core = CURRENT_CORE;
    cores[core].blk_ops.flags = TBOP_SET_VARi;
    cores[core].blk_ops.var_ip = &s->count;
    cores[core].blk_ops.var_iv = count;
#endif
    block_thread_no_listlock(&s->queue);
}

void semaphore_release(struct semaphore *s)
{
#if CONFIG_CORELOCK == CORELOCK_NONE || CONFIG_CORELOCK == SW_CORELOCK
    corelock_lock(&s->cl);
    if (s->count < s->max)
    {
        if (++s->count <= 0)
        {
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    int count;
    while ((count = xchg32(&s->count, STATE_BUSYi)) == STATE_BUSYi);
    if(count < s->max)
    {
        if(++count <= 0)
        {
#endif /* CONFIG_CORELOCK */

            /* there should be threads in this queue */
            KERNEL_ASSERT(s->queue.queue != NULL, "semaphore->wakeup");
            /* a thread was queued - wake it up */
            wakeup_thread_no_listlock(&s->queue);
        }
    }

#if CONFIG_CORELOCK == SW_CORELOCK
    corelock_unlock(&s->cl);
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    s->count = count;
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
#if CONFIG_CORELOCK == SW_CORELOCK
    corelock_init(&e->cl);
#endif
}

void event_wait(struct event *e, unsigned int for_state)
{
   unsigned int last_state;
#if CONFIG_CORELOCK == CORELOCK_NONE || CONFIG_CORELOCK == SW_CORELOCK
    corelock_lock(&e->cl);
    last_state = e->state;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    while ((last_state = xchg8(&e->state, STATE_BUSYu8)) == STATE_BUSYu8);
#endif

    if(e->automatic != 0)
    {
        /* wait for false always satisfied by definition
           or if it just changed to false */
        if(last_state == STATE_SIGNALED || for_state == STATE_NONSIGNALED)
        {
            /* automatic - unsignal */
            e->state = STATE_NONSIGNALED;
#if CONFIG_CORELOCK == SW_CORELOCK
            corelock_unlock(&e->cl);
#endif
            return;
        }
        /* block until state matches */
    }
    else if(for_state == last_state)
    {
        /* the state being waited for is the current state */
#if CONFIG_CORELOCK == SW_CORELOCK
        corelock_unlock(&e->cl);
#elif CONFIG_CORELOCK == CORELOCK_SWAP
        e->state = last_state;
#endif
        return;
    }

    {
        /* current state does not match wait-for state */
#if CONFIG_CORELOCK == SW_CORELOCK
        const unsigned int core = CURRENT_CORE;
        cores[core].blk_ops.flags = TBOP_UNLOCK_CORELOCK;
        cores[core].blk_ops.cl_p = &e->cl;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
        const unsigned int core = CURRENT_CORE;
        cores[core].blk_ops.flags = TBOP_SET_VARu8;
        cores[core].blk_ops.var_u8p = &e->state;
        cores[core].blk_ops.var_u8v = last_state;
#endif
        block_thread_no_listlock(&e->queues[for_state]);
    }
}

void event_set_state(struct event *e, unsigned int state)
{
    unsigned int last_state;
#if CONFIG_CORELOCK == CORELOCK_NONE || CONFIG_CORELOCK == SW_CORELOCK
    corelock_lock(&e->cl);
    last_state = e->state;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    while ((last_state = xchg8(&e->state, STATE_BUSYu8)) == STATE_BUSYu8);
#endif

    if(last_state == state)
    {
        /* no change */
#if CONFIG_CORELOCK == SW_CORELOCK
        corelock_unlock(&e->cl);
#elif CONFIG_CORELOCK == CORELOCK_SWAP
        e->state = last_state;
#endif
        return;
    }

    if(state == STATE_SIGNALED)
    {
        if(e->automatic != 0)
        {
            struct thread_entry *thread;
            /* no thread should have ever blocked for unsignaled */
            KERNEL_ASSERT(e->queues[STATE_NONSIGNALED].queue == NULL,
                          "set_event_state->queue[NS]:S");
            /* pass to next thread and keep unsignaled - "pulse" */
            thread = wakeup_thread_no_listlock(&e->queues[STATE_SIGNALED]);
            e->state = thread != NULL ? STATE_NONSIGNALED : STATE_SIGNALED;
        }
        else
        {
            /* release all threads waiting for signaled */
            thread_queue_wake_no_listlock(&e->queues[STATE_SIGNALED]);
            e->state = STATE_SIGNALED;
        }
    }
    else
    {
        /* release all threads waiting for unsignaled */

        /* no thread should have ever blocked if automatic */
        KERNEL_ASSERT(e->queues[STATE_NONSIGNALED].queue == NULL ||
                      e->automatic == 0, "set_event_state->queue[NS]:NS");

        thread_queue_wake_no_listlock(&e->queues[STATE_NONSIGNALED]);
        e->state = STATE_NONSIGNALED;
    }

#if CONFIG_CORELOCK == SW_CORELOCK
    corelock_unlock(&e->cl);
#endif
}
#endif /* HAVE_EVENT_OBJECTS */
