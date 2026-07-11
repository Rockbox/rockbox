/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "timer.h"
#include "nvic-arm.h"
#include "clock-stm32h7.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/tim.h"

static uint32_t tim_reg;
static int tim_irq;
static const struct stm32_clock *tim_clk;

void stm32h7_timer_init(uint32_t instance,
                        int irq,
                        const struct stm32_clock *clock)
{
    tim_reg = instance;
    tim_irq = irq;
    tim_clk = clock;
}

void stm32h7_timer_irq(void)
{
    reg_assignlf(tim_reg, TIM_SR, UIF(0));

    if (pfn_timer)
        pfn_timer();
}

bool timer_set(long cycles, bool start)
{
    if (cycles <= 0)
        return false;

    /* Calculate timer interval */
    uint32_t counter = cycles;
    uint32_t prescaler = 1;
    while (counter > 0xFFFF && prescaler < 0x10000)
    {
        counter >>= 1;
        prescaler <<= 1;
    }

    /* Duration too long */
    if (counter >= 0xFFFF)
        return false;

    /* Unregister old function */
    if (start && pfn_unregister)
    {
        pfn_unregister();
        pfn_unregister = NULL;
    }

    /* Set up timer */
    stm32_clock_enable(tim_clk);
    reg_writef(RCC_APB1LENR, TIM7EN(1));
    reg_assignlf(tim_reg, TIM_CR1, URS(1));
    reg_assignlf(tim_reg, TIM_EGR, UG(1));
    reg_assignlf(tim_reg, TIM_DIER, UIE(1));
    reg_assignlf(tim_reg, TIM_SR, UIF(1));
    reg_varl(tim_reg, TIM_CNT) = 0;
    reg_varl(tim_reg, TIM_ARR) = counter;
    reg_varl(tim_reg, TIM_PSC) = prescaler - 1;

    if (start)
        return timer_start();

    return true;
}

bool timer_start(void)
{
    stm32_clock_enable(tim_clk);
    nvic_enable_irq(tim_irq);

    reg_writef(RCC_APB1LENR, TIM7EN(1));
    reg_writelf(tim_reg, TIM_CR1, URS(0), CEN(1));
    return true;
}

void timer_stop(void)
{
    nvic_disable_irq(tim_irq);

    reg_writelf(tim_reg, TIM_CR1, CEN(0));
    stm32_clock_disable(tim_clk);
}
