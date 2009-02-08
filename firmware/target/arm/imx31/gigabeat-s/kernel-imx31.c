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
#include "avic-imx31.h"
#include "spi-imx31.h"
#include "mc13783.h"
#include "clkctl-imx31.h"
#include "sdma-imx31.h"
#include "kernel.h"
#include "thread.h"

static __attribute__((interrupt("IRQ"))) void EPIT1_HANDLER(void)
{
    EPITSR1 = EPITSR_OCIF; /* Clear the pending status */

    /* Run through the list of tick tasks */
    call_tick_tasks();
}

void tick_start(unsigned int interval_in_ms)
{
    imx31_clkctl_module_clock_gating(CG_EPIT1, CGM_ON_ALL); /* EPIT1 module
                                               clock ON - before writing
                                               regs! */
    EPITCR1 &= ~(EPITCR_OCIEN | EPITCR_EN); /* Disable the counter */
    CLKCTL_WIMR0 &= ~WIM_IPI_INT_EPIT1;     /* Clear wakeup mask */

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

void kernel_device_init(void)
{
#ifndef BOOTLOADER
    sdma_init();
#endif
    spi_init();
    mc13783_init();
}

#ifdef BOOTLOADER
void tick_stop(void)
{
    avic_disable_int(EPIT1);                /* Disable insterrupt */
    EPITCR1 &= ~(EPITCR_OCIEN | EPITCR_EN); /* Disable counter */
    EPITSR1 = EPITSR_OCIF;                  /* Clear pending */
    imx31_clkctl_module_clock_gating(CG_EPIT1, CGM_OFF); /* Turn off module clock */
}
#endif
