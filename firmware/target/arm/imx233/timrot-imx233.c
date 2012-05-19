/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "timrot-imx233.h"
#include "clkctrl-imx233.h"

imx233_timer_fn_t timer_fns[4];

#define define_timer_irq(nr) \
void INT_TIMER##nr(void) \
{ \
    if(timer_fns[nr]) \
        timer_fns[nr](); \
    __REG_CLR(HW_TIMROT_TIMCTRL(nr)) = HW_TIMROT_TIMCTRL__IRQ; \
}

define_timer_irq(0)
define_timer_irq(1)
define_timer_irq(2)
define_timer_irq(3)

void imx233_setup_timer(unsigned timer_nr, bool reload, unsigned count,
    unsigned src, unsigned prescale, bool polarity, imx233_timer_fn_t fn)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    
    timer_fns[timer_nr] = fn;
    
    HW_TIMROT_TIMCTRL(timer_nr) = src | prescale;
    if(polarity)
        __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__POLARITY;
    if(reload)
    {
        __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__RELOAD;
        /* manual says count - 1 for reload timers */
        HW_TIMROT_TIMCOUNT(timer_nr) = count - 1;
    }
    else
        HW_TIMROT_TIMCOUNT(timer_nr) = count;
    /* only enable interrupt if function is set */
    if(fn != NULL)
    {
        /* enable interrupt */
        imx233_icoll_enable_interrupt(INT_SRC_TIMER(timer_nr), true);
        /* clear irq bit and enable */
        __REG_CLR(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__IRQ;
        __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__IRQ_EN;
    }
    else
        imx233_icoll_enable_interrupt(INT_SRC_TIMER(timer_nr), false);
    /* finally update */
    __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__UPDATE;

    restore_interrupt(oldstatus);
}

void imx233_timrot_init(void)
{
    imx233_reset_block(&HW_TIMROT_ROTCTRL);
    /* enable xtal path to timrot */
    imx233_clkctrl_enable_xtal(XTAL_TIMROT, true);
}
