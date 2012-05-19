/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef ICOLL_IMX233_H
#define ICOLL_IMX233_H

#include "config.h"
#include "system.h"

/* Interrupt collector */
#define HW_ICOLL_BASE           0x80000000

#define HW_ICOLL_VECTOR         (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x0))

#define HW_ICOLL_LEVELACK       (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x10))
#define HW_ICOLL_LEVELACK__LEVEL0   0x1

#define HW_ICOLL_CTRL           (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x20))
#define HW_ICOLL_CTRL__IRQ_FINAL_ENABLE (1 << 16)
#define HW_ICOLL_CTRL__ARM_RSE_MODE     (1 << 18)

#define HW_ICOLL_VBASE          (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x40))
#define HW_ICOLL_INTERRUPT(i)   (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x120 + (i) * 0x10))
#define HW_ICOLL_INTERRUPT__PRIORITY_BM 0x3
#define HW_ICOLL_INTERRUPT__ENABLE      0x4
#define HW_ICOLL_INTERRUPT__SOFTIRQ     0x8
#define HW_ICOLL_INTERRUPT__ENFIQ       0x10

#define INT_SRC_SSP2_ERROR  2
#define INT_SRC_VDD5V       3
#define INT_SRC_DAC_DMA     5
#define INT_SRC_DAC_ERROR   6
#define INT_SRC_ADC_DMA     7
#define INT_SRC_ADC_ERROR   8
#define INT_SRC_USB_CTRL    11
#define INT_SRC_SSP1_DMA    14
#define INT_SRC_SSP1_ERROR  15
#define INT_SRC_GPIO0       16
#define INT_SRC_GPIO1       17
#define INT_SRC_GPIO2       18
#define INT_SRC_GPIO(i)     (INT_SRC_GPIO0 + (i))
#define INT_SRC_SSP2_DMA    20
#define INT_SRC_I2C_DMA     26
#define INT_SRC_I2C_ERROR   27
#define INT_SRC_TIMER(nr)   (28 + (nr))
#define INT_SRC_TOUCH_DETECT    36
#define INT_SRC_LRADC_CHx(x)    (37 + (x))
#define INT_SRC_LCDIF_DMA   45
#define INT_SRC_LCDIF_ERROR 46
#define INT_SRC_RTC_1MSEC   48
#define INT_SRC_DCP         54
#define INT_SRC_NR_SOURCES  66

struct imx233_icoll_irq_info_t
{
    bool enabled;
    unsigned freq;
};

void imx233_icoll_init(void);
void imx233_icoll_enable_interrupt(int src, bool enable);
struct imx233_icoll_irq_info_t imx233_icoll_get_irq_info(int src);

#endif /* ICOLL_IMX233_H */
