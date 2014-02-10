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
#include "string.h"

static imx233_timer_fn_t timer_fns[4];

#define define_timer_irq(nr) \
void INT_TIMER##nr(void) \
{ \
    if(timer_fns[nr]) \
        timer_fns[nr](); \
    BF_CLRn(TIMROT_TIMCTRLn, nr, IRQ); \
}

define_timer_irq(0)
define_timer_irq(1)
define_timer_irq(2)
define_timer_irq(3)

void imx233_timrot_setup(unsigned timer_nr, bool reload, unsigned count,
    unsigned src, unsigned prescale, bool polarity, imx233_timer_fn_t fn)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    /* only enable interrupt if function is set */
    bool irq = fn != NULL;
    timer_fns[timer_nr] = fn;

    /* make sure we start from stop state */
    HW_TIMROT_TIMCTRLn(timer_nr) = BF_OR2(TIMROT_TIMCTRLn,
        SELECT(BV_TIMROT_TIMCTRLn_SELECT__NEVER_TICK), UPDATE(1));
    /* write count and take effect immediately with UPDATE
     * manual says count-1 for reload timers */
    HW_TIMROT_TIMCOUNTn(timer_nr) = reload ? count - 1 : count;
    /* start timer */
    HW_TIMROT_TIMCTRLn(timer_nr) = BF_OR6(TIMROT_TIMCTRLn, SELECT(src),
        PRESCALE(prescale), POLARITY(polarity), RELOAD(reload), IRQ(irq),
        IRQ_EN(irq));
    imx233_icoll_enable_interrupt(INT_SRC_TIMER(timer_nr), irq);

    restore_interrupt(oldstatus);
}

void imx233_timrot_set_priority(unsigned timer_nr, unsigned prio)
{
    imx233_icoll_set_priority(INT_SRC_TIMER(timer_nr), prio);
}

struct imx233_timrot_info_t imx233_timrot_get_info(unsigned timer_nr)
{
    struct imx233_timrot_info_t info;
    memset(&info, 0, sizeof(info));
    info.src = BF_RDn(TIMROT_TIMCTRLn, timer_nr, SELECT);
    info.prescale = BF_RDn(TIMROT_TIMCTRLn, timer_nr, PRESCALE);
    info.reload = BF_RDn(TIMROT_TIMCTRLn, timer_nr, RELOAD);
    info.polarity = BF_RDn(TIMROT_TIMCTRLn, timer_nr, POLARITY);
    info.fixed_count = BF_RDn(TIMROT_TIMCOUNTn, timer_nr, FIXED_COUNT);
    info.run_count = BF_RDn(TIMROT_TIMCOUNTn, timer_nr, RUNNING_COUNT);
    return info;
}

void imx233_timrot_init(void)
{
    imx233_reset_block(&HW_TIMROT_ROTCTRL);
    /* enable xtal path to timrot */
    imx233_clkctrl_enable(CLK_TIMROT, true);
}
