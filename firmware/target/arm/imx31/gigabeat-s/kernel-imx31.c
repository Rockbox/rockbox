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
#include "system.h"
#include "avic-imx31.h"
#include "kernel.h"
#include "thread.h"

extern void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

static __attribute__((interrupt("IRQ"))) void EPIT1_HANDLER(void)
{
    int i;

    EPITSR1 = EPITSR_OCIF; /* Clear the pending status */

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
            tick_funcs[i]();
    }

    current_tick++;
}

void tick_start(unsigned int interval_in_ms)
{
    CLKCTL_CGR0 |= CGR0_EPIT1(CG_ON_ALL);   /* EPIT1 module clock ON -
                                               before writing regs! */
    EPITCR1 &= ~(EPITCR_OCIEN | EPITCR_EN); /* Disable the counter */
    CLKCTL_WIMR0 &= ~(1 << 23);             /* Clear wakeup mask */

    /* mcu_main_clk = 528MHz = 27MHz * 2 * ((9 + 7/9) / 1)
     * CLKSRC = ipg_clk = 528MHz / 4 / 2 = 66MHz,
     * EPIT Output Disconnected,
     * Enabled in wait mode
     * Prescale 1/2640 for 25KHz
     * Reload from modulus register,
     * Compare interrupt enabled,
     * Count from load value */
    EPITCR1 = EPITCR_CLKSRC_IPG_CLK | EPITCR_WAITEN | EPITCR_IOVW |
              EPITCR_PRESCALER(2640-1) | EPITCR_RLD | EPITCR_OCIEN |
              EPITCR_ENMOD;
 
    EPITLR1 = interval_in_ms*25; /* Count down from interval */
    EPITCMPR1 = 0;               /* Event when counter reaches 0 */
    EPITSR1 = EPITSR_OCIF;       /* Clear any pending interrupt */
    avic_enable_int(EPIT1, IRQ, 7, EPIT1_HANDLER);
    EPITCR1 |= EPITCR_EN;        /* Enable the counter */
}

#ifdef BOOTLOADER
void tick_stop(void)
{
    avic_disable_int(EPIT1);                /* Disable insterrupt */
    EPITCR1 &= ~(EPITCR_OCIEN | EPITCR_EN); /* Disable counter */
    CLKCTL_CGR0 &= ~CGR0_EPIT1(CG_MASK);    /* EPIT1 module clock OFF */
}
#endif
