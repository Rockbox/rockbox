/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 by Karl Kurbjun
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "timer.h"
#include "logf.h"

/* GPB0/TOUT0 should already have been configured as output so that pin
   should not be a functional pin and TIMER0 output unseen there */
void TIMER0(void)
{
    if (pfn_timer != NULL)
        pfn_timer();
    IO_INTC_IRQ0 |= 1<<IRQ_TIMER0;
}

bool __timer_set(long cycles, bool start)
{
    int oldlevel;
    unsigned int divider=cycles, prescaler=0;

    if(cycles<1)
        return false;

    IO_TIMER0_TMMD = CONFIG_TIMER0_TMMD_STOP;

    if (start && pfn_unregister != NULL)
    {
        pfn_unregister();
        pfn_unregister = NULL;
    }

    oldlevel = disable_irq_save();

    /* Increase prescale values starting from 0 to make the cycle count fit */
    while(divider>65535 && prescaler<1024)
    {
        prescaler++;
        divider=cycles/(prescaler+1);
    }

    IO_TIMER0_TMPRSCL = prescaler;
    IO_TIMER0_TMDIV = divider;

    restore_irq(oldlevel);

    return true;
}

static void stop_timer(void)
{
    IO_INTC_EINT0 &= ~(1<<IRQ_TIMER0);

    IO_INTC_IRQ0 |= 1<<IRQ_TIMER0;
    
    IO_TIMER0_TMMD = CONFIG_TIMER0_TMMD_STOP;
}

bool __timer_register(void)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    stop_timer();

    /* Turn Timer0 to Free Run mode */
    IO_TIMER0_TMMD = CONFIG_TIMER0_TMMD_FREE_RUN;

    IO_INTC_EINT0 |= 1<<IRQ_TIMER0;

    restore_interrupt(oldstatus);

    return true;
}

void __timer_unregister(void)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    stop_timer();
    restore_interrupt(oldstatus);
}
