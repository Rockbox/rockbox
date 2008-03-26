/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 by Michael Sevakis
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

    SRCPND = TIMER0_MASK;
    INTPND = TIMER0_MASK;
}

static void stop_timer(void)
{
    /* mask interrupt */
    INTMSK |= TIMER0_MASK;

    /* stop any running TIMER0 */
    TCON &= ~(1 << 0);

    /* clear pending */
    SRCPND = TIMER0_MASK;
    INTPND = TIMER0_MASK;
}

bool __timer_set(long cycles, bool start)
{
    bool retval = false;

    /* Find the minimum factor that puts the counter in range 1-65535 */
    unsigned int prescaler = (cycles + 65534) / 65535;

    /* Maximum divider setting is x / 256 / 16 = x / 4096 - min divider
       is x / 2 however */
    if (prescaler <= 2048)
    {
        int oldlevel;
        unsigned int divider;

        if (start && pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }

        oldlevel = disable_irq_save();

        TCMPB0 = 0;
        TCNTB0 = (unsigned int)cycles / prescaler;

        /* Max prescale is 255+1 */
        for (divider = 0; prescaler > 256; prescaler >>= 1, divider++);

        TCFG0 = (TCFG0 & ~0xff) | (prescaler - 1);
        TCFG1 = (TCFG1 & ~0xf) | divider;

        restore_irq(oldlevel);

        retval = true;
    }

    return retval;
}

bool __timer_register(void)
{
    bool retval = true;

    int oldstatus = set_interrupt_status(IRQ_FIQ_DISABLED, IRQ_FIQ_STATUS);

    stop_timer();

    /* neurosis - make sure something didn't set GPB0 to TOUT0 */
    if ((GPBCON & 0x3) != 0x2)
    {
        /* manual update: on (to reset count) */
        TCON |= (1 << 1);
        /* dead zone: off, inverter: off, manual off */
        TCON &= ~((1 << 4) | (1 << 2) | (1 << 1));
        /* interval mode (auto reload): on */
        TCON |= (1 << 3);
        /* start timer */
        TCON |= (1 << 0);
        /* unmask interrupt */
        INTMSK &= ~TIMER0_MASK;
    }

    if (!(TCON & (1 << 0)))
    {
        /* timer could not be started due to config error */
        logf("Timer error: GPB0 set to TOUT0");
        retval = false;
    }

    set_interrupt_status(oldstatus, IRQ_FIQ_STATUS);

    return retval;
}

void __timer_unregister(void)
{
    int oldstatus = set_interrupt_status(IRQ_FIQ_DISABLED, IRQ_FIQ_STATUS);
    stop_timer();
    set_interrupt_status(oldstatus, IRQ_FIQ_STATUS);
}
