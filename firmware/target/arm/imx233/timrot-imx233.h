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

#define HW_TIMROT_BASE          0x80068000

#define HW_TIMROT_ROTCTRL       (*(volatile uint32_t *)(HW_TIMROT_BASE + 0x0))

#define HW_TIMROT_ROTCOUNT      (*(volatile uint32_t *)(HW_TIMROT_BASE + 0x10))

#define HW_TIMROT_TIMCTRL(i)    (*(volatile uint32_t *)(HW_TIMROT_BASE + 0x20 + (i) * 0x20))
#define HW_TIMROT_TIMCTRL__IRQ      (1 << 15)
#define HW_TIMROT_TIMCTRL__IRQ_EN   (1 << 14)
#define HW_TIMROT_TIMCTRL__POLARITY (1 << 8)
#define HW_TIMROT_TIMCTRL__UPDATE   (1 << 7)
#define HW_TIMROT_TIMCTRL__RELOAD   (1 << 6)
#define HW_TIMROT_TIMCTRL__PRESCALE_1   (0 << 4)
#define HW_TIMROT_TIMCTRL__PRESCALE_2   (1 << 4)
#define HW_TIMROT_TIMCTRL__PRESCALE_4   (2 << 4)
#define HW_TIMROT_TIMCTRL__PRESCALE_8   (3 << 4)
#define HW_TIMROT_TIMCTRL__SELECT_32KHZ_XTAL    8
#define HW_TIMROT_TIMCTRL__SELECT_8KHZ_XTAL     9
#define HW_TIMROT_TIMCTRL__SELECT_4KHZ_XTAL     10
#define HW_TIMROT_TIMCTRL__SELECT_1KHZ_XTAL     11
#define HW_TIMROT_TIMCTRL__SELECT_TICK_ALWAYS   12

#define HW_TIMROT_TIMCOUNT(i)   (*(volatile uint32_t *)(HW_TIMROT_BASE + 0x30 + (i) * 0x20))

typedef void (*imx233_timer_fn_t)(void);

void imx233_timrot_init(void);
void imx233_setup_timer(unsigned timer_nr, bool reload, unsigned count,
    unsigned src, unsigned prescale, bool polarity, imx233_timer_fn_t fn);

#endif /* TIMROT_IMX233_H */
