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
static int g_atexit = HWSTUB_ATEXIT_OFF;

/**
 *
 * Power
 *
 */

#define HW_POWER_BASE       0x80044000

void power_off(void)
{
    switch(g_stmp_family)
    {
        case STMP3600:
            *(volatile uint32_t *)(HW_POWER_BASE + 0xc0) = 0x3e770014;
            break;
        case STMP3700:
        case STMP3770:
            *(volatile uint32_t *)(HW_POWER_BASE + 0xe0) = 0x3e770003;
            break;
        case STMP3780:
            *(volatile uint32_t *)(HW_POWER_BASE + 0x100) = 0x3e770003;
            break;
        default:
            break;
    }
}

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

void clkctrl_reset(void)
{
    switch(g_stmp_family)
    {
        case STMP3600:
            *(volatile uint32_t *)(HW_POWER_BASE + 0xc0) = 0x3e770002;
            break;
        case STMP3700:
        case STMP3770:
            *(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xf0) = 0x1;
            break;
        case STMP3780:
            *(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x100) = 0x1;
            break;
        default:
            break;
    }
}

/**
 *
 * Digctl
 *
 */

/* Digital control */
#define HW_DIGCTL_BASE          0x8001C000
#define HW_DIGCTL_CTRL          (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0))
#define HW_DIGCTL_CTRL__USB_CLKGATE (1 << 2)

#define HW_DIGCTL_MICROSECONDS  (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0xC0))

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

void target_init(void)
{
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

    if(g_stmp_family == STMP3600)
    {
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
        HW_CLKCTRL_HBUSCLKCTRL = 7;
        HW_CLKCTRL_XBUSCLKCTRL = 1;
        __REG_CLR(HW_CLKCTRL_UTMICLKCTRL) = HW_CLKCTRL_UTMICLKCTRL__UTMI_CLK120M_GATE;
        __REG_CLR(HW_CLKCTRL_UTMICLKCTRL) = HW_CLKCTRL_UTMICLKCTRL__UTMI_CLK30M_GATE;
    }
    else
        __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__POWER;
    /* enable USB PHY PLL */
    __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS;
    /* power up USB PHY */
    __REG_CLR(HW_USBPHY_CTRL) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
    HW_USBPHY_PWD = 0;
    /* enable USB controller */
    __REG_CLR(HW_DIGCTL_CTRL) = HW_DIGCTL_CTRL__USB_CLKGATE;
}

static struct usb_resp_info_stmp_t g_stmp;
static struct usb_resp_info_target_t g_target =
{
    .id = HWSTUB_TARGET_STMP,
    .name = "STMP3600 / STMP3700 / STMP3780 (i.MX233)"
};

int target_get_info(int info, void **buffer)
{
    if(info == HWSTUB_INFO_STMP)
    {
        g_stmp.chipid = __XTRACT(HW_DIGCTL_CHIPID, PRODUCT_CODE);
        g_stmp.rev = __XTRACT(HW_DIGCTL_CHIPID, REVISION);
        g_stmp.is_supported = g_stmp_family != 0;
        *buffer = &g_stmp;
        return sizeof(g_stmp);
    }
    else if(info == HWSTUB_INFO_TARGET)
    {
        *buffer = &g_target;
        return sizeof(g_target);
    }
    else
        return -1;
}

int target_atexit(int method)
{
    g_atexit = method;
    return 0;
}

void target_exit(void)
{
    switch(g_atexit)
    {
        case HWSTUB_ATEXIT_OFF:
            power_off();
            // fallthrough in case of return
        case HWSTUB_ATEXIT_REBOOT:
            clkctrl_reset();
            // fallthrough in case of return
        case HWSTUB_ATEXIT_NOP:
        default:
            return;
    }
}
