/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
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
#include "cpu.h"
#include "system.h"
#include "timer.h"

#define TIMER_ID 5

/* Interrupt handler */
void TCU1(void)
{
    __tcu_clear_full_match_flag(TIMER_ID);

    if (pfn_timer != NULL)
        pfn_timer();
}

bool timer_set(long cycles, bool start)
{
    unsigned int divider = cycles, prescaler_bit = 0, prescaler = 1, old_irq;

    if(cycles < 1)
        return false;

    if(start && pfn_unregister != NULL)
    {
        pfn_unregister();
        pfn_unregister = NULL;
    }

    /* Increase prescale values starting from 0 to make the cycle count fit */
    while(divider > 65535 && prescaler <= 1024)
    {
        prescaler <<= 2; /* 1, 4, 16, 64, 256, 1024 */
        prescaler_bit++;
        divider = cycles / prescaler;
    }

    old_irq = disable_irq_save();

    __tcu_stop_counter(TIMER_ID);
    if(start)
    {
        __tcu_disable_pwm_output(TIMER_ID);

        __tcu_mask_half_match_irq(TIMER_ID);
        __tcu_unmask_full_match_irq(TIMER_ID);

        /* EXTAL clock = CFG_EXTAL (12Mhz in most targets) */
        __tcu_select_extalclk(TIMER_ID);
    }

    REG_TCU_TCSR(TIMER_ID) = (REG_TCU_TCSR(TIMER_ID) & ~TCSR_PRESCALE_MASK) | (prescaler_bit << TCSR_PRESCALE_LSB);
    REG_TCU_TCNT(TIMER_ID) = 0;
    REG_TCU_TDHR(TIMER_ID) = 0;
    REG_TCU_TDFR(TIMER_ID) = divider;

    __tcu_clear_full_match_flag(TIMER_ID);

    if(start)
    {
        system_enable_irq(IRQ_TCU1);
    }

    restore_irq(old_irq);
    __tcu_start_counter(TIMER_ID);

    return true;
}

bool timer_start(void)
{
    __tcu_start_counter(TIMER_ID);

    return true;
}

void timer_stop(void)
{
    unsigned int old_irq = disable_irq_save();
    __tcu_stop_counter(TIMER_ID);
    restore_irq(old_irq);
}
