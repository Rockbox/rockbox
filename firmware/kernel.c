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
#include "kernel.h"
#include "thread.h"
#include "sh7034.h"
#include "system.h"
#include "panic.h"

long current_tick = 0;

static void tick_start(unsigned int interval_in_ms);

/****************************************************************************
 * Standard kernel stuff
 ****************************************************************************/
void kernel_init(void)
{
    tick_start(1000/HZ);
}

void sleep(int ticks)
{
    /* Always sleep at least 1 tick */
    int timeout = current_tick + ticks + 1;

    while (TIME_BEFORE( current_tick, timeout )) {
        yield();
    }
}

void yield(void)
{
    switch_thread();
}

/****************************************************************************
 * Interrupt level setting
 ****************************************************************************/
static int current_irq_level = 15;

int set_irq_level(int level)
{
    int old;

    /* First raise to highest level and update the shadow */
    asm volatile ("ldc %0, sr" : : "r" (15<<4));
    old = current_irq_level;
    current_irq_level = level;

    /* Then set the wanted level */
    asm volatile ("ldc %0, sr" : : "r" ((unsigned int)level<<4));

    return ((unsigned int)old >> 4) & 0x0f;
}

/****************************************************************************
 * Queue handling stuff
 ****************************************************************************/
void queue_init(struct event_queue *q)
{
    q->read = 0;
    q->write = 0;
}

struct event *queue_wait(struct event_queue *q)
{
    while(q->read == q->write)
    {
        switch_thread();
    }

    return &q->events[(q->read++) & QUEUE_LENGTH_MASK];
}

void queue_post(struct event_queue *q, int id, void *data)
{
    int wr;
    int oldlevel;

    oldlevel = set_irq_level(15);
    wr = (q->write++) & QUEUE_LENGTH_MASK;

    q->events[wr].id = id;
    q->events[wr].data = data;
    set_irq_level(oldlevel);
}


/****************************************************************************
 * Timer tick
 ****************************************************************************/
#define NUM_TICK_TASKS 4
void (*tick_funcs[NUM_TICK_TASKS])(void) = {NULL, NULL, NULL, NULL};

static void tick_start(unsigned int interval_in_ms)
{
    unsigned int count;

    count = FREQ / 1000 / 8 * interval_in_ms;

    if(count > 0xffff)
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
    GRA0 = count;
    TCR0 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 1 */
    IPRC = (IPRC & ~0x00f0) | 0x0010;
    
    TSR0 &= ~0x01;
    TIER0 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x01; /* Start timer 1 */
}

#pragma interrupt
void IMIA0(void)
{
    int i;

    /* Run through the list of tick tasks */
    for(i = 0;i < NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;

    TSR0 &= ~0x01;
}

int tick_add_task(void (*f)(void))
{
    int i;
    int oldlevel = set_irq_level(15);

    /* Add a task if there is room */
    for(i = 0;i < NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == NULL)
        {
            tick_funcs[i] = f;
            set_irq_level(oldlevel);
            return 0;
        }
    }
    set_irq_level(oldlevel);
    return -1;
}

int tick_remove_task(void (*f)(void))
{
    int i;
    int oldlevel = set_irq_level(15);

    /* Remove a task if it is there */
    for(i = 0;i < NUM_TICK_TASKS;i++)
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
