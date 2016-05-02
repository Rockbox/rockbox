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

/* WARNING timrot code assumes APBX is running at 24MHz */
/* list of timers */
enum
{
    TIMER_TICK, /* for tick task */
    TIMER_USER, /* for user timer */
    TIMER_WATCHDOG, /* for watchdog */
};

/* timer sources */
enum imx233_timrot_src_t
{
    TIMER_SRC_24MHZ,
    TIMER_SRC_12MHZ,
    TIMER_SRC_6MHZ,
    TIMER_SRC_3MHZ,
    TIMER_SRC_32KHZ,
    TIMER_SRC_8KHZ,
    TIMER_SRC_4KHZ,
    TIMER_SRC_1KHZ,
    TIMER_SRC_STOP
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

/* maximum count for non-periodic timers, add one for periodic timers */
#define IMX233_TIMROT_MAX_COUNT 0xffff

void imx233_timrot_init(void);
/* low-level function all-in-one function */
void imx233_timrot_setup(unsigned timer_nr, bool reload, unsigned count,
    unsigned src, unsigned prescale, bool polarity, imx233_timer_fn_t fn);
/* change interrupt priority */
void imx233_timrot_set_priority(unsigned timer_nr, unsigned prio);
/* simple setup function */
void imx233_timrot_setup_simple(unsigned timer_nr, bool periodic, unsigned count,
    enum imx233_timrot_src_t src, imx233_timer_fn_t fn);
/* get timer count */
unsigned imx233_timrot_get_count(unsigned timer_nr);
/* update timer running count */
struct imx233_timrot_info_t imx233_timrot_get_info(unsigned timer_nr);

#endif /* TIMROT_IMX233_H */
