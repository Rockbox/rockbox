/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Marcin Bukat
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
#include "stddef.h"
#include "target.h"
#include "system.h"
#include "logf.h"
#include "rk27xx.h"

#define HZ  1000000

enum rk27xx_family_t
{
    UNKNOWN,
    REV_A,
    REV_B,
};

static enum rk27xx_family_t g_rk27xx_family = UNKNOWN;

static void _enable_irq(void)
{
    asm volatile ("mrs r0, cpsr\n"
                  "bic r0, r0, #0x80\n"
                  "msr cpsr_c, r0\n"
                 );
}

/* us may be at most 2^31/200 (~10 seconds) for 200MHz max cpu freq */
void target_udelay(int us)
{
    unsigned cycles_per_us;
    unsigned delay;

    cycles_per_us = (200000000 + 999999) / 1000000;

    delay = (us * cycles_per_us) / 5;

    asm volatile(
        "1: subs %0, %0, #1  \n"    /* 1 cycle  */
        "   nop              \n"    /* 1 cycle  */
        "   bne  1b          \n"    /* 3 cycles */
        : : "r"(delay)
    );
}

void target_mdelay(int ms)
{
    return target_udelay(ms * 1000);
}

void target_init(void)
{
    /* ungate all clocks */
    SCU_CLKCFG = 0;

    /* keep act line */
    GPIO_PCDR |= (1<<0);
    GPIO_PCCON |= (1<<0);

    /* disable watchdog */
    WDTCON &= ~(1<<3);

    /* enable UDC interrupt */
    INTC_IMR = (1<<16);
    INTC_IECR = (1<<16);

    EN_INT = EN_SUSP_INTR   |  /* Enable Suspend Interrupt */
             EN_RESUME_INTR |  /* Enable Resume Interrupt */
             EN_USBRST_INTR |  /* Enable USB Reset Interrupt */
             EN_OUT0_INTR   |  /* Enable OUT Token receive Interrupt EP0 */
             EN_IN0_INTR    |  /* Enable IN Token transmits Interrupt EP0 */
             EN_SETUP_INTR;    /* Enable SETUP Packet Receive Interrupt */

    /* 6. configure INTCON */
    INTCON = UDC_INTHIGH_ACT |  /* interrupt high active */
             UDC_INTEN;         /* enable EP0 interrupts */

    /* enable irq */
    _enable_irq();

    /* detect revision */
    uint32_t rk27xx_id = SCU_ID;

    if(rk27xx_id == 0xa1000604)
    {
        logf("identified rk27xx REV_A \n");
        g_rk27xx_family = REV_A;
    }
    else if(rk27xx_id == 0xa100027b)
    {
        logf("identified rk27xx REV_B \n");
        g_rk27xx_family = REV_B;
    }
    else
    {
        logf("unknown rk27xx revision \n");
    }
}

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_RK27,
    "Rockchip RK27XX"
};

void target_get_desc(int desc, void **buffer)
{
    (void) desc;
    *buffer = NULL;
}

void target_get_config_desc(void *buffer, int *size)
{
    (void) buffer;
    (void) size;
}
