/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 by Michael Sevakis
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
#include "config.h"
#include "system.h"
#include "timer.h"
#include "ccm-imx31.h"
#include "avic-imx31.h"

static void __attribute__((interrupt("IRQ"))) EPIT2_HANDLER(void)
{
    EPITSR2 = EPITSR_OCIF; /* Clear the pending status */

    if (pfn_timer != NULL)
        pfn_timer();
}

static void stop_timer(bool clock_off)
{
    /* Ensure clock gating on (before touching any module registers) */
    ccm_module_clock_gating(CG_EPIT2, CGM_ON_RUN_WAIT);
    /* Disable insterrupt */
    avic_disable_int(INT_EPIT2);
    /* Clear wakeup mask */
    CCM_WIMR0 &= ~CCM_WIMR0_IPI_INT_EPIT2;
    /* Disable counter */
    EPITCR2 &= ~(EPITCR_OCIEN | EPITCR_EN);
    /* Clear pending */
    EPITSR2 = EPITSR_OCIF;

    if (clock_off)
    {
        /* Final stop, not reset; don't clock module any longer */
        ccm_module_clock_gating(CG_EPIT2, CGM_OFF);
    }
}

bool timer_set(long cycles, bool start)
{
    /* Maximum cycle count expressible in the cycles parameter is 2^31-1
     * and the modulus counter is capable of 2^32-1 and as a result there is
     * no requirement to use a prescaler > 1. This gives a frequency range of
     * ~0.015366822Hz - 66000000Hz. The highest input frequency gives the
     * greatest possible accuracy anyway. */ 
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    /* Halt timer if running - leave module clock enabled */
    stop_timer(false);

    if (start && pfn_unregister != NULL)
    {
        pfn_unregister();
        pfn_unregister = NULL;
    }

    /* CLKSRC = ipg_clk,
     * EPIT output disconnected,
     * Enabled in wait mode
     * Prescale 1 for 66MHz
     * Reload from modulus register,
     * Count from load value */
    EPITCR2 = EPITCR_CLKSRC_IPG_CLK | EPITCR_WAITEN | EPITCR_IOVW |
              ((1-1) << EPITCR_PRESCALER_POS) | EPITCR_RLD | EPITCR_ENMOD;
    EPITLR2 = cycles;
    /* Event when counter reaches 0 */
    EPITCMPR2 = 0;

    restore_interrupt(oldstatus);
    return true;
}

bool timer_start(void)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    /* Halt timer if running - leave module clock enabled */
    stop_timer(false);

    /* Enable interrupt */
    EPITCR2 |= EPITCR_OCIEN;
    avic_enable_int(INT_EPIT2, INT_TYPE_IRQ, INT_PRIO_DEFAULT,
                    EPIT2_HANDLER);
    /* Start timer */
    EPITCR2 |= EPITCR_EN;

    restore_interrupt(oldstatus);
    return true;
}

void timer_stop(void)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    /* Halt timer if running - stop module clock */
    stop_timer(true);
    restore_interrupt(oldstatus);
}
