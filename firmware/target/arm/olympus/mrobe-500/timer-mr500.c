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
}

static void stop_timer(void)
{
}

bool __timer_set(long cycles, bool start)
{
    /* taken from linux/arch/arm/mach-itdm320-20/time.c and timer-meg-fx.c */

  	/* Turn off all timers */
/*	outw(CONFIG_TIMER0_TMMD_STOP, IO_TIMER0_TMMD);
	outw(CONFIG_TIMER1_TMMD_STOP, IO_TIMER1_TMMD);
	outw(CONFIG_TIMER2_TMMD_STOP, IO_TIMER2_TMMD);
	outw(CONFIG_TIMER3_TMMD_STOP, IO_TIMER3_TMMD);
 */
	/* Turn Timer0 to Free Run mode */
//	outw(CONFIG_TIMER0_TMMD_FREE_RUN, IO_TIMER0_TMMD);

    bool retval = false;

    /* Find the minimum factor that puts the counter in range 1-65535 */
    unsigned int prescaler = (cycles + 65534) / 65535;

    /* Test this by writing 1's to registers to see how many bits we have */
    /* Maximum divider setting is x / 1024 / 65536 = x / 67108864 */
    {
        int oldlevel;
        unsigned int divider;

        if (start && pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }

        oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

        /* Max prescale is 1023+1 */
        for (divider = 0; prescaler > 1024; prescaler >>= 1, divider++);

        /* Setup the Prescalar */
        outw(prescaler, IO_TIMER0_TMPRSCL);

        /* Setup the Divisor */
        outw(divider, IO_TIMER0_TMDIV);

        set_irq_level(oldlevel);

        retval = true;
    }

    return retval;
}

bool __timer_register(void)
{
    bool retval = true;

    int oldstatus = set_interrupt_status(IRQ_FIQ_DISABLED, IRQ_FIQ_STATUS);

    stop_timer();

    /* Turn Timer0 to Free Run mode */
	outw(0x0002, IO_TIMER0_TMMD);

    set_interrupt_status(oldstatus, IRQ_FIQ_STATUS);

    return retval;
}

void __timer_unregister(void)
{
    int oldstatus = set_interrupt_status(IRQ_FIQ_DISABLED, IRQ_FIQ_STATUS);
    stop_timer();
    set_interrupt_status(oldstatus, IRQ_FIQ_STATUS);
}
