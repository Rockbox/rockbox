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

#if (CONFIG_CPU != PP5020) || !defined(BOOTLOADER)
long current_tick = 0;
#endif

static void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct event_queue *all_queues[32];
static int num_queues;

void sleep(int ticks) ICODE_ATTR;
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
    /* Always sleep at least 1 tick */
    int timeout = current_tick + ticks + 1;

    while (TIME_BEFORE( current_tick, timeout )) {
        sleep_thread();
    }
    wake_up_thread();
}

void yield(void)
{
    switch_thread();
    wake_up_thread();
}

/****************************************************************************
 * Queue handling stuff
 ****************************************************************************/
void queue_init(struct event_queue *q)
{
    q->read = 0;
    q->write = 0;

    /* Add it to the all_queues array */
    all_queues[num_queues++] = q;
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
}

void queue_wait(struct event_queue *q, struct event *ev)
{
    while(q->read == q->write)
    {
        sleep_thread();
    }
    wake_up_thread();

    *ev = q->events[(q->read++) & QUEUE_LENGTH_MASK];
}

void queue_wait_w_tmo(struct event_queue *q, struct event *ev, int ticks)
{
    unsigned int timeout = current_tick + ticks;

    while(q->read == q->write && TIME_BEFORE( current_tick, timeout ))
    {
        sleep_thread();
    }
    wake_up_thread();

    if(q->read != q->write)
    {
        *ev = q->events[(q->read++) & QUEUE_LENGTH_MASK];
    }
    else
    {
        ev->id = SYS_TIMEOUT;
    }
}

void queue_post(struct event_queue *q, long id, void *data)
{
    int wr;
    int oldlevel;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    wr = (q->write++) & QUEUE_LENGTH_MASK;

    q->events[wr].id = id;
    q->events[wr].data = data;
    set_irq_level(oldlevel);
}

bool queue_empty(const struct event_queue* q)
{
    return ( q->read == q->write );
}

void queue_clear(struct event_queue* q)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    q->read = 0;
    q->write = 0;
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
    wake_up_thread();

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
    wake_up_thread();

    TER0 = 0xff; /* Clear all events */
}

#elif CONFIG_CPU == TCC730

void TIMER0(void)
{
    int i;
        
    /* Keep alive (?)
     * If this is not done, power goes down when DC is unplugged.
     */
    if (current_tick % 2 == 0)
        P8 |= 1;
    else
        P8 &= ~1;
    
    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;
    wake_up_thread();

    /* re-enable timer by clearing the counter */
    TACON |= 0x80;
}

void tick_start(unsigned int interval_in_ms)
{
    long count;
    count = (long)FREQ * (long)interval_in_ms / 1000 / 16;

    if(count > 0xffffL)
    {
        panicf("Error! The tick interval is too long (%dms->%lx)\n",
               interval_in_ms, count);
        return;
    }

    /* Use timer A */
    TAPRE = 0x0;
    TADATA = count;

    TACON = 0x89;
    /* counter clear; */
    /* interval mode; */
    /* TICS = F(osc) / 16 */
    /* TCS = internal clock */
    /* enable */
    
    /* enable the interrupt */
    interrupt_vector[2] = TIMER0;
    IMR0 |= (1<<2);
}

#elif CONFIG_CPU == PP5020

#define USECS_PER_INT 0x2710

#ifndef BOOTLOADER
void TIMER1(void)
{
    int i;

    PP5020_TIMER1_ACK;
    /* Run through the list of tick tasks */
    for (i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if (tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;
    wake_up_thread();
}
#endif

void tick_start(unsigned int interval_in_ms)
{
#ifndef BOOTLOADER
    /* TODO: use interval_in_ms to set timer periode */
    (void)interval_in_ms;
    PP5020_TIMER1 = 0x0;
    PP5020_TIMER1_ACK;
    /* enable timer, period, trigger value 0x2710 -> 100Hz */
    PP5020_TIMER1 = 0xc0000000 | USECS_PER_INT;
    /* unmask interrupt source */
    PP5020_CPU_INT_EN = PP5020_TIMER1_MASK;
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
    wake_up_thread();

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
}

void mutex_lock(struct mutex *m)
{
    /* Wait until the lock is open... */
    while(m->locked)
        sleep_thread();
    wake_up_thread();

    /* ...and lock it */
    m->locked = true;
}

void mutex_unlock(struct mutex *m)
{
    m->locked = false;
}

#endif
