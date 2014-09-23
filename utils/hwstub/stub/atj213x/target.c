/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Marcin Bukat
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
#include "atj213x.h"

#define CORE_FREQ 7500000
#define HZ  1000000

static void backlight_init(void)
{
    /* backlight clock enable, select backlight clock as 32kHz */
    CMU_FMCLK = (CMU_FMCLK & ~(CMU_FMCLK_BCLK_MASK)) | CMU_FMCLK_BCKE | CMU_FMCLK_BCLK_32K;

    /* baclight enable */
    PMU_CTL |= PMU_CTL_BL_EN;

    /* pwm output, phase high, some initial duty cycle set as 24/32 */
    PMU_CHG = ((PMU_CHG & ~PMU_CHG_PDOUT_MASK)| PMU_CHG_PBLS_PWM | PMU_CHG_PPHS_HIGH | PMU_CHG_PDUT(24));

}

void backlight_set(int level)
{
    /* set duty cycle in 1/32 units */
    PMU_CHG = ((PMU_CHG & ~PMU_CHG_PDOUT_MASK) | PMU_CHG_PDUT(level));
}

void target_udelay(int us)
{
    unsigned int i = us * (CORE_FREQ / 2000000);
    asm volatile (
                  ".set noreorder    \n"
                  "1:                \n"
                  "bnez  %0, 1b   \n"
                  "addiu %0, %0, -1   \n"
                  ".set reorder      \n"
                  : "=r" (i)
                  : "0" (i)
                  );
}

void target_mdelay(int ms)
{
    return target_udelay(ms * 1000);
}

void blink(int cnt)
{
    int i;

    for (i=0; i<cnt; i++)
    {
        backlight_set(0);
        target_mdelay(300);
        backlight_set(24);
        target_mdelay(300);
    }
}

void target_init(void)
{
    RTC_WDCTL = (RTC_WDCTL & ~(1<<4))|(1<<6)|1; /* disable WDT */

    /* Configure USB interrupt as IP6. IP6 is unmasked in crt0.S */
    INTC_CFG0 = 0;
    INTC_CFG1 = 0;
    INTC_CFG2 = (1<<4);

    /* mask all interrupts to avoid pending irq servicing */
    INTC_MSK = 0;

    backlight_init();
}

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_ATJ,
    "ATJ213X"
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
