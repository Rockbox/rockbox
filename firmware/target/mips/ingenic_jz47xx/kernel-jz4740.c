/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "kernel.h"
#include "jz4740.h"

#define USE_RTC_CLOCK 0
void tick_start(unsigned int interval_in_ms)
{
    unsigned int tps = interval_in_ms;
    unsigned int latch;
    __cpm_start_tcu();
    
    __tcu_stop_counter(0);
    __tcu_disable_pwm_output(0);
    __tcu_stop_counter(1);
    __tcu_disable_pwm_output(1);
    __tcu_stop_counter(2);
    __tcu_disable_pwm_output(2);
    __tcu_clear_full_match_flag(2);
    
    __tcu_mask_half_match_irq(0); 
    __tcu_unmask_full_match_irq(0);

#if USE_RTC_CLOCK
    __tcu_select_rtcclk(0);
    __tcu_select_clk_div1(0);
    latch = (__cpm_get_rtcclk() + (tps>>1)) / tps;
#else    
    __tcu_select_extalclk(0);
    __tcu_select_clk_div4(0);
    
    latch = (JZ_EXTAL / 4 + (tps>>1)) / tps;
#endif
    REG_TCU_TCNT(0) = 0;
    REG_TCU_TDFR(0) = latch;
    REG_TCU_TDHR(0) = latch;
    //REG_TCU_TDHR(0) = 0;

    __tcu_clear_full_match_flag(0);
    __tcu_start_counter(0);
    
    system_enable_irq(IRQ_TCU0);
}

/* Interrupt handler */
void TCU0(void)
{
    __tcu_clear_full_match_flag(0);
    
    /* Run through the list of tick tasks */
    call_tick_tasks();
}
