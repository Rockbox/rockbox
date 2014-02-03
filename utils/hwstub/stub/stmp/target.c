/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "stddef.h"
#include "target.h"
#include "system.h"
#include "logf.h"
#include "memory.h"

#define __REG_SET(reg)  (*((volatile uint32_t *)(&reg + 1)))
#define __REG_CLR(reg)  (*((volatile uint32_t *)(&reg + 2)))
#define __REG_TOG(reg)  (*((volatile uint32_t *)(&reg + 3)))

#define __BLOCK_SFTRST  (1 << 31)
#define __BLOCK_CLKGATE (1 << 30)

#define __XTRACT(reg, field)    ((reg & reg##__##field##_BM) >> reg##__##field##_BP)
#define __XTRACT_EX(val, field)    (((val) & field##_BM) >> field##_BP)
#define __FIELD_SET(reg, field, val) reg = (reg & ~reg##__##field##_BM) | (val << reg##__##field##_BP)

/**
 *
 * Global
 *
 */

enum stmp_family_t
{
    UNKNOWN,
    STMP3600,
    STMP3700,
    STMP3770,
    STMP3780
};

static enum stmp_family_t g_stmp_family = UNKNOWN;

/**
 *
 * Clkctrl
 *
 */

#define HW_CLKCTRL_BASE     0x80040000

#define HW_CLKCTRL_PLLCTRL0 (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x0))
#define HW_CLKCTRL_PLLCTRL0__BYPASS         (1 << 17) /* STMP3600 only */
#define HW_CLKCTRL_PLLCTRL0__POWER          (1 << 16)
#define HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS    (1 << 18)

#define HW_CLKCTRL_PLLCTRL1 (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x10))
#define HW_CLKCTRL_PLLCTRL1__LOCK           (1 << 31)

/* STMP3600 only */
#define HW_CLKCTRL_CPUCLKCTRL   (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x20))
#define HW_CLKCTRL_CPUCLKCTRL__DIV_BP   0
#define HW_CLKCTRL_CPUCLKCTRL__DIV_BM   0x3ff
#define HW_CLKCTRL_CPUCLKCTRL__WAIT_PLL_LOCK    (1 << 30)

/* STMP3600 */
#define HW_CLKCTRL_HBUSCLKCTRL  (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x30))

/* STMP3600 only */
#define HW_CLKCTRL_XBUSCLKCTRL  (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x40))
#define HW_CLKCTRL_XBUSCLKCTRL__DIV_BP   0
#define HW_CLKCTRL_XBUSCLKCTRL__DIV_BM   0x3ff

/* STMP3600 only */
#define HW_CLKCTRL_UTMICLKCTRL  (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x70))
#define HW_CLKCTRL_UTMICLKCTRL__UTMI_CLK30M_GATE    (1 << 30)
#define HW_CLKCTRL_UTMICLKCTRL__UTMI_CLK120M_GATE   (1 << 31)

/**
 *
 * Digctl
 *
 */

/* Digital control */
#define HW_DIGCTL_BASE          0x8001C000
#define HW_DIGCTL_CTRL          (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0))
#define HW_DIGCTL_CTRL__USB_CLKGATE (1 << 2)
#define HW_DIGCTL_CTRL__PACKAGE_SENSE_ENABLE_STMP3600 (1 << 0)

#define HW_DIGCTL_STATUS          (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0x10))
#define HW_DIGCTL_STATUS__PACKAGE_TYPE_BP   1
#define HW_DIGCTL_STATUS__PACKAGE_TYPE_BM   (7 << 1)
#define HW_DIGCTL_STATUS__PACKAGE_TYPE_STMP3600_BP  1
#define HW_DIGCTL_STATUS__PACKAGE_TYPE_STMP3600_BM  (1 << 1)

/* STMP3700+ */
#define HW_DIGCTL_MICROSECONDS  (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0xC0))
/* STMP3600 */
#define HW_DIGCTL_MICROSECONDS_STMP3600 (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0xB0))

#define HW_DIGCTL_CHIPID        (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0x310))
#define HW_DIGCTL_CHIPID__PRODUCT_CODE_BP   16
#define HW_DIGCTL_CHIPID__PRODUCT_CODE_BM   0xffff0000
#define HW_DIGCTL_CHIPID__REVISION_BP       0
#define HW_DIGCTL_CHIPID__REVISION_BM       0xff

#define HZ  1000000

/**
 *
 * USB PHY
 *
 */
/* USB Phy */
#define HW_USBPHY_BASE          0x8007C000
#define HW_USBPHY_PWD           (*(volatile uint32_t *)(HW_USBPHY_BASE + 0))

#define HW_USBPHY_CTRL          (*(volatile uint32_t *)(HW_USBPHY_BASE + 0x30))

/**
 *
 * RTC
 *
 */
#define HW_RTC_BASE             0x8005C000
#define HW_RTC_CTRL             (*(volatile uint32_t *)(HW_RTC_BASE + 0))
#define HW_RTC_CTRL__WATCHDOGEN (1 << 4)

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_STMP,
    "STMP3600 / STMP3700 / STMP3780 (i.MX233)"
};

static struct hwstub_stmp_desc_t __attribute__((aligned(2))) stmp_descriptor =
{
    sizeof(struct hwstub_stmp_desc_t),
    HWSTUB_DT_STMP,
    0, 0, 0
};

void target_init(void)
{
    stmp_descriptor.wChipID = __XTRACT(HW_DIGCTL_CHIPID, PRODUCT_CODE);
    stmp_descriptor.bRevision = __XTRACT(HW_DIGCTL_CHIPID, REVISION);
    /* detect family */
    uint16_t product_code = __XTRACT(HW_DIGCTL_CHIPID, PRODUCT_CODE);
    if(product_code >= 0x3600 && product_code < 0x3700)
    {
        logf("identified STMP3600 family\n");
        g_stmp_family = STMP3600;
    }
    else if(product_code == 0x3700)
    {
        logf("identified STMP3700 family\n");
        g_stmp_family = STMP3700;
    }
    else if(product_code == 0x37b0)
    {
        logf("identified STMP3770 family\n");
        g_stmp_family = STMP3770;
    }
    else if(product_code == 0x3780)
    {
        logf("identified STMP3780 family\n");
        g_stmp_family = STMP3780;
    }
    else
        logf("cannot identify family: 0x%x\n", product_code);
    /* disable watchdog */
    __REG_CLR(HW_RTC_CTRL) = HW_RTC_CTRL__WATCHDOGEN;

    if(g_stmp_family == STMP3600)
    {
        stmp_descriptor.bPackage = __XTRACT(HW_DIGCTL_STATUS, PACKAGE_TYPE);
        /* CPU clock is always derived from PLL, if we switch to PLL, cpu will
         * run at 480 MHz unprepared ! That's bad so prepare to run at slow sleed
         * (1.2MHz) for a safe transition */
        HW_CLKCTRL_CPUCLKCTRL = HW_CLKCTRL_CPUCLKCTRL__WAIT_PLL_LOCK | 400;
        /* We need to ensure that XBUS < HBUS but HBUS will be 1.2 MHz after the
         * switch so lower XBUS too */
        HW_CLKCTRL_XBUSCLKCTRL = 20;
        /* Power PLL */
        __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__POWER;
        HW_CLKCTRL_PLLCTRL0 = (HW_CLKCTRL_PLLCTRL0 & ~0x3ff) | 480;
        /* Wait lock */
        while(!(HW_CLKCTRL_PLLCTRL1 & HW_CLKCTRL_PLLCTRL1__LOCK));
        /* Switch to PLL source */
        __REG_CLR(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__BYPASS;
        /* Get back XBUS = 24 MHz and CPU = HBUS = 64MHz */
        HW_CLKCTRL_CPUCLKCTRL = 7;
        HW_CLKCTRL_HBUSCLKCTRL = 2;
        HW_CLKCTRL_XBUSCLKCTRL = 1;
        __REG_CLR(HW_CLKCTRL_UTMICLKCTRL) = HW_CLKCTRL_UTMICLKCTRL__UTMI_CLK120M_GATE;
        __REG_CLR(HW_CLKCTRL_UTMICLKCTRL) = HW_CLKCTRL_UTMICLKCTRL__UTMI_CLK30M_GATE;
    }
    else
    {
        __REG_SET(HW_DIGCTL_CTRL) = HW_DIGCTL_CTRL__PACKAGE_SENSE_ENABLE_STMP3600;
        stmp_descriptor.bPackage = __XTRACT(HW_DIGCTL_STATUS, PACKAGE_TYPE_STMP3600);
        __REG_CLR(HW_DIGCTL_CTRL) = HW_DIGCTL_CTRL__PACKAGE_SENSE_ENABLE_STMP3600;

        __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__POWER;
    }
    /* enable USB PHY PLL */
    __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS;
    /* power up USB PHY */
    __REG_CLR(HW_USBPHY_CTRL) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
    HW_USBPHY_PWD = 0;
    /* enable USB controller */
    __REG_CLR(HW_DIGCTL_CTRL) = HW_DIGCTL_CTRL__USB_CLKGATE;
}

void target_get_desc(int desc, void **buffer)
{
    if(desc == HWSTUB_DT_STMP)
        *buffer = &stmp_descriptor;
    else
        *buffer = NULL;
}

void target_get_config_desc(void *buffer, int *size)
{
    memcpy(buffer, &stmp_descriptor, sizeof(stmp_descriptor));
    *size += sizeof(stmp_descriptor);
}

void target_udelay(int us)
{
    volatile uint32_t *reg = g_stmp_family == STMP3600 ? 
        &HW_DIGCTL_MICROSECONDS_STMP3600 : &HW_DIGCTL_MICROSECONDS;
    uint32_t cur = *reg;
    uint32_t end = cur + us;
    if(cur < end)
        while(*reg < end) {}
    else
        while(*reg >= cur) {}
}

void target_mdelay(int ms)
{
    return target_udelay(ms * 1000);
}
