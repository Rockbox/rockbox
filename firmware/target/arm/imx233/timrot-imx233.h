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
#ifndef TIMROT_IMX233_H
#define TIMROT_IMX233_H

#include "system.h"
#include "cpu.h"
#include "icoll-imx233.h"

#include "regs/regs-timrot.h"

/* list of timers */
enum
{
    TIMER_TICK, /* for tick task */
    TIMER_USER, /* for user timer */
    TIMER_WATCHDOG, /* for watchdog */
};

struct imx233_timrot_info_t
{
    unsigned fixed_count, run_count;
    unsigned src;
    unsigned prescale;
    bool reload;
    bool polarity;
    bool irqen;
};

typedef void (*imx233_timer_fn_t)(void);

void imx233_timrot_init(void);
void imx233_timrot_setup(unsigned timer_nr, bool reload, unsigned count,
    unsigned src, unsigned prescale, bool polarity, imx233_timer_fn_t fn);
void imx233_timrot_set_priority(unsigned timer_nr, unsigned prio);
struct imx233_timrot_info_t imx233_timrot_get_info(unsigned timer_nr);

#endif /* TIMROT_IMX233_H */
