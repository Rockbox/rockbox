/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 by Amaury Pouly
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
#ifndef RTC_IMX233_H
#define RTC_IMX233_H

#include "config.h"
#include "system.h"
#include "cpu.h"

#define HW_RTC_BASE     0x8005c000 

#define HW_RTC_CTRL         (*(volatile uint32_t *)(HW_RTC_BASE + 0x0))
#define HW_RTC_CTRL__ALARM_IRQ_EN   (1 << 0)
#define HW_RTC_CTRL__ONEMSEC_IRQ_EN (1 << 1)
#define HW_RTC_CTRL__ALARM_IRQ      (1 << 2)
#define HW_RTC_CTRL__ONEMSEC_IRQ    (1 << 3)
#define HW_RTC_CTRL__WATCHDOGEN     (1 << 4)
#define HW_RTC_CTRL__FORCE_UPDATE   (1 << 5)
#define HW_RTC_CTRL__SUPPRESS_COPY2ANALOG   (1 << 6)

#define HW_RTC_STAT         (*(volatile uint32_t *)(HW_RTC_BASE + 0x10))
#define HW_RTC_STAT__NEW_REGS_BP    8
#define HW_RTC_STAT__NEW_REGS_BM    0xff00
#define HW_RTC_STAT__STALE_REGS_BP  16
#define HW_RTC_STAT__STALE_REGS_BM  0xff0000
#define HW_RTC_STAT__XTAL32768_PRESENT  (1 << 27)
#define HW_RTC_STAT__XTAL32000_PRESENT  (1 << 28)
#define HW_RTC_STAT__WATCHDOG_PRESENT   (1 << 29)
#define HW_RTC_STAT__ALARM_PRESENT      (1 << 30)
#define HW_RTC_STAT__RTC_PRESENT        (1 << 31)

#define HW_RTC_MILLISECONDS (*(volatile uint32_t *)(HW_RTC_BASE + 0x20))

#define HW_RTC_SECONDS      (*(volatile uint32_t *)(HW_RTC_BASE + 0x30))

#define HW_RTC_ALARM        (*(volatile uint32_t *)(HW_RTC_BASE + 0x40))

#define HW_RTC_WATCHDOG     (*(volatile uint32_t *)(HW_RTC_BASE + 0x50))

#define HW_RTC_PERSISTENTx(x)   (*(volatile uint32_t *)(HW_RTC_BASE + 0x60 + (x) * 0x10))

#define HW_RTC_PERSISTENT0  (*(volatile uint32_t *)(HW_RTC_BASE + 0x60))
#define HW_RTC_PERSISTENT0__CLOCKSOURCE (1 << 0)
#define HW_RTC_PERSISTENT0__ALARM_WAKE_EN   (1 << 1)
#define HW_RTC_PERSISTENT0__ALARM_EN    (1 << 2)
#define HW_RTC_PERSISTENT0__XTAL24MHZ_PWRUP (1 << 4)
#define HW_RTC_PERSISTENT0__XTAL32KHZ_PWRUP (1 << 5)
#define HW_RTC_PERSISTENT0__XTAL32_FREQ (1 << 6)
#define HW_RTC_PERSISTENT0__ALARM_WAKE  (1 << 7)
#define HW_RTC_PERSISTENT0__AUTO_RESTART    (1 << 17)
#define HW_RTC_PERSISTENT0__SPARE_BP    18
#define HW_RTC_PERSISTENT0__SPARE_BM    (0x3fff << 18)
#define HW_RTC_PERSISTENT0__SPARE__RELEASE_GND  (1 << 19)

#define HW_RTC_PERSISTENT1  (*(volatile uint32_t *)(HW_RTC_BASE + 0x70))

#define HW_RTC_PERSISTENT2  (*(volatile uint32_t *)(HW_RTC_BASE + 0x80))

#define HW_RTC_PERSISTENT3  (*(volatile uint32_t *)(HW_RTC_BASE + 0x90))

#define HW_RTC_PERSISTENT4  (*(volatile uint32_t *)(HW_RTC_BASE + 0xa0))

#define HW_RTC_PERSISTENT5  (*(volatile uint32_t *)(HW_RTC_BASE + 0xb0))

struct imx233_rtc_info_t
{
    uint32_t seconds;
    uint32_t persistent[6];
};

static inline void imx233_rtc_init(void)
{
    __REG_CLR(HW_RTC_CTRL) = __BLOCK_CLKGATE;
}

static inline uint32_t imx233_rtc_read_seconds(void)
{
    return HW_RTC_SECONDS;
}

static inline uint32_t imx233_rtc_read_persistent(int idx)
{
    return HW_RTC_PERSISTENTx(idx);
}

static inline void imx233_rtc_clear_msec_irq(void)
{
    __REG_CLR(HW_RTC_CTRL) = HW_RTC_CTRL__ONEMSEC_IRQ;
}

static inline void imx233_rtc_enable_msec_irq(bool enable)
{
    imx233_rtc_clear_msec_irq();
    if(enable)
        __REG_SET(HW_RTC_CTRL) = HW_RTC_CTRL__ONEMSEC_IRQ_EN;
    else
        __REG_CLR(HW_RTC_CTRL) = HW_RTC_CTRL__ONEMSEC_IRQ_EN;
}

void imx233_rtc_write_seconds(uint32_t seconds);
void imx233_rtc_write_persistent(int idx, uint32_t val);

struct imx233_rtc_info_t imx233_rtc_get_info(void);

#endif /* RTC_IMX233_H */
