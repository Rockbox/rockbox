/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include <stdbool.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "timer.h"

#ifndef SIMULATOR

static int timer_prio = -1;
static void (*pfn_timer)(void) = NULL;      /* timer callback */
static void (*pfn_unregister)(void) = NULL; /* unregister callback */
#ifdef CPU_COLDFIRE
static int base_prescale;
#endif

/* interrupt handler */
#if CONFIG_CPU == SH7034
void IMIA4(void) __attribute__((interrupt_handler));
void IMIA4(void)
{
    if (pfn_timer != NULL)
        pfn_timer();
    and_b(~0x01, &TSR4); /* clear the interrupt */
}
#elif defined CPU_COLDFIRE
void TIMER1(void) __attribute__ ((interrupt_handler));
void TIMER1(void)
{
    if (pfn_timer != NULL)
        pfn_timer();
    TER1 = 0xff; /* clear all events */
}
#endif /* CONFIG_CPU */

static bool timer_set(long cycles, bool start)
{
    int phi = 0; /* bits for the prescaler */
    int prescale = 1;
    
#ifdef CPU_COLDFIRE
    cycles >>= 1; /* the coldfire timer works on busclk == cpuclk/2 */
#endif

    while (cycles > 0x10000)
    {   /* work out the smallest prescaler that makes it fit */
#if CONFIG_CPU == SH7034
        phi++;
#endif
        prescale *= 2;
        cycles >>= 1;
    }

#if CONFIG_CPU == SH7034
    if (prescale > 8)
        return false;

    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }

        and_b(~0x10, &TSTR); /* Stop the timer 4 */
        and_b(~0x10, &TSNC); /* No synchronization */
        and_b(~0x10, &TMDR); /* Operate normally */

        TIER4 = 0xF9;        /* Enable GRA match interrupt */
    }

    TCR4 = 0x20 | phi;   /* clear at GRA match, set prescaler */
    GRA4 = (unsigned short)(cycles - 1);
    if (start || (TCNT4 >= GRA4))
        TCNT4 = 0;
    and_b(~0x01, &TSR4); /* clear an eventual interrupt */

#elif defined CPU_COLDFIRE
    if (prescale > 4096/CPUFREQ_MAX_MULT)
        return false;

    if (prescale > 256/CPUFREQ_MAX_MULT)
    {
        phi = 0x05;      /* prescale sysclk/16, timer enabled */
        prescale >>= 4;
    }
    else
        phi = 0x03;      /* prescale sysclk, timer enabled */
        
    base_prescale = prescale;
    prescale *= (cpu_frequency / CPU_FREQ);

    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }
        phi &= ~1;       /* timer disabled at start */
    }

    /* We are using timer 1 */
    TMR1 = 0x0018 | (unsigned short)phi | ((unsigned short)(prescale - 1) << 8);
    TRR1 = (unsigned short)(cycles - 1);
    if (start || (TCN1 >= TRR1))
        TCN1 = 0; /* reset the timer */
    TER1 = 0xff;  /* clear all events */

#endif /* CONFIG_CPU */
    return true;
}

#ifdef CPU_COLDFIRE
void timers_adjust_prescale(int multiplier, bool enable_irq)
{
    /* tick timer */
    TMR0 = (TMR0 & 0x00ef)
         | ((unsigned short)(multiplier - 1) << 8)
         | (enable_irq ? 0x10 : 0);

    if (pfn_timer)
    {
        /* user timer */
        int prescale = base_prescale * multiplier;
        TMR1 = (TMR1 & 0x00ef)
             | ((unsigned short)(prescale - 1) << 8)
             | (enable_irq ? 0x10 : 0);
    }
}
#endif

/* Register a user timer, called every <cycles> CPU_FREQ cycles */
bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, int int_prio, void (*timer_callback)(void))
{
    if (reg_prio <= timer_prio || cycles == 0)
        return false;

#if CONFIG_CPU == SH7034
    if (int_prio < 1 || int_prio > 15)
        return false;
#elif defined CPU_COLDFIRE
    (void)int_prio;
#endif

    if (!timer_set(cycles, true))
        return false;
        
    pfn_timer = timer_callback;
    pfn_unregister = unregister_callback;
    timer_prio = reg_prio;

#if CONFIG_CPU == SH7034
    IPRD = (IPRD & 0xFF0F) | int_prio << 4;  /* interrupt priority */
    or_b(0x10, &TSTR); /* start timer 4 */
#elif defined CPU_COLDFIRE
    ICR2 = 0x90;       /* interrupt on level 4.0 */
    and_l(~(1<<10), &IMR);
    TMR1 |= 1;         /* start timer */
#endif

    return true;
}

bool timer_set_period(long cycles)
{
    return timer_set(cycles, false);
}

void timer_unregister(void)
{
#if CONFIG_CPU == SH7034
    and_b(~0x10, &TSTR);    /* stop the timer 4 */
    IPRD = (IPRD & 0xFF0F); /* disable interrupt */
#elif defined CPU_COLDFIRE
    TMR1 = 0;               /* disable timer 1 */
    or_l((1<<10), &IMR);    /* disable interrupt */
#endif
    pfn_timer = NULL;
    pfn_unregister = NULL;
    timer_prio = -1;
}

#endif /* !SIMULATOR */
