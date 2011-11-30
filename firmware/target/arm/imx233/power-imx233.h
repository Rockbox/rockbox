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
#ifndef __POWER_IMX233__
#define __POWER_IMX233__

#include "system.h"
#include "system-target.h"
#include "cpu.h"

#define HW_POWER_BASE           0x80044000

#define HW_POWER_CTRL           (*(volatile uint32_t *)(HW_POWER_BASE + 0x0))
#define HW_POWER_CTRL__ENIRQ_VBUS_VALID     (1 << 3)
#define HW_POWER_CTRL__VBUSVALID_IRQ        (1 << 4)
#define HW_POWER_CTRL__POLARITY_VBUSVALID   (1 << 5)

#define HW_POWER_5VCTRL         (*(volatile uint32_t *)(HW_POWER_BASE + 0x10))
#define HW_POWER_5VCTRL__VBUSVALID_5VDETECT (1 << 4)
#define HW_POWER_5VCTRL__VBUSVALID_TRSH_BP  8
#define HW_POWER_5VCTRL__VBUSVALID_TRSH_BM  (0x7 << 8)

#define HW_POWER_MINPWR         (*(volatile uint32_t *)(HW_POWER_BASE + 0x20))

#define HW_POWER_CHARGE         (*(volatile uint32_t *)(HW_POWER_BASE + 0x30))
#define HW_POWER_CHARGE__BATTCHRG_I_BP  0
#define HW_POWER_CHARGE__BATTCHRG_I_BM  0x3f
#define HW_POWER_CHARGE__STOP_ILIMIT_BP 8
#define HW_POWER_CHARGE__STOP_ILIMIT_BM 0xf00
#define HW_POWER_CHARGE__PWD_BATTCHRG   (1 << 16)
#define HW_POWER_CHARGE__CHRG_STS_OFF   (1 << 19)

#define HW_POWER_VDDDCTRL       (*(volatile uint32_t *)(HW_POWER_BASE + 0x40))
#define HW_POWER_VDDDCTRL__TRG_BP   0
#define HW_POWER_VDDDCTRL__TRG_BM   0x1f
#define HW_POWER_VDDDCTRL__TRG_STEP 25 /* mV */
#define HW_POWER_VDDDCTRL__TRG_MIN  800 /* mV */
#define HW_POWER_VDDDCTRL__ENABLE_LINREG    (1 << 21)

#define HW_POWER_VDDACTRL       (*(volatile uint32_t *)(HW_POWER_BASE + 0x50))
#define HW_POWER_VDDACTRL__TRG_BP   0
#define HW_POWER_VDDACTRL__TRG_BM   0x1f
#define HW_POWER_VDDACTRL__TRG_STEP 25 /* mV */
#define HW_POWER_VDDACTRL__TRG_MIN  1500 /* mV */
#define HW_POWER_VDDACTRL__ENABLE_LINREG    (1 << 17)

#define HW_POWER_VDDIOCTRL      (*(volatile uint32_t *)(HW_POWER_BASE + 0x60))
#define HW_POWER_VDDIOCTRL__TRG_BP  0
#define HW_POWER_VDDIOCTRL__TRG_BM  0x1f
#define HW_POWER_VDDIOCTRL__TRG_STEP    25 /* mV */
#define HW_POWER_VDDIOCTRL__TRG_MIN 2800 /* mV */

#define HW_POWER_VDDMEMCTRL     (*(volatile uint32_t *)(HW_POWER_BASE + 0x70))
#define HW_POWER_VDDMEMCTRL__TRG_BP  0
#define HW_POWER_VDDMEMCTRL__TRG_BM  0x1f
#define HW_POWER_VDDMEMCTRL__TRG_STEP    50 /* mV */
#define HW_POWER_VDDMEMCTRL__TRG_MIN 1700 /* mV */
#define HW_POWER_VDDMEMCTRL__ENABLE_LINREG  (1 << 8)

#define HW_POWER_MISC           (*(volatile uint32_t *)(HW_POWER_BASE + 0x90))
#define HW_POWER_MISC__SEL_PLLCLK   1
#define HW_POWER_MISC__FREQSEL_BP   4
#define HW_POWER_MISC__FREQSEL_BM   (0x7 << 4)
#define HW_POWER_MISC__FREQSEL__RES         0
#define HW_POWER_MISC__FREQSEL__20MHz       1
#define HW_POWER_MISC__FREQSEL__24MHz       2
#define HW_POWER_MISC__FREQSEL__19p2MHz     3
#define HW_POWER_MISC__FREQSEL__14p4MHz     4
#define HW_POWER_MISC__FREQSEL__18MHz       5
#define HW_POWER_MISC__FREQSEL__21p6MHz     6
#define HW_POWER_MISC__FREQSEL__17p28MHz    7

#define HW_POWER_STS            (*(volatile uint32_t *)(HW_POWER_BASE + 0xc0))
#define HW_POWER_STS__VBUSVALID     (1 << 1)
#define HW_POWER_STS__PSWITCH_BP    20
#define HW_POWER_STS__PSWITCH_BM    (3 << 20)

#define HW_POWER_BATTMONITOR    (*(volatile uint32_t *)(HW_POWER_BASE + 0xe0))
#define HW_POWER_BATTMONITOR__BATT_VAL_BP   16
#define HW_POWER_BATTMONITOR__BATT_VAL_BM   (0x3ff << 16)

#define HW_POWER_RESET          (*(volatile uint32_t *)(HW_POWER_BASE + 0x100))
#define HW_POWER_RESET__UNLOCK  0x3E770000
#define HW_POWER_RESET__PWD     0x1

struct imx233_power_info_t
{
    int vddd; /* in mV */
    bool vddd_linreg; /* VDDD source: linreg from VDDA or DC-DC */
    int vdda; /* in mV */
    bool vdda_linreg; /* VDDA source: linreg from VDDIO or DC-DC */
    int vddio; /* in mV */
    int vddmem; /* in mV */
    bool vddmem_linreg; /* VDDMEM source: linreg from VDDIO or off */
    bool dcdc_sel_pllclk; /* clock source of DC-DC: pll or 24MHz xtal */
    int dcdc_freqsel;
};

#define POWER_INFO_VDDD     (1 << 0)
#define POWER_INFO_VDDA     (1 << 1)
#define POWER_INFO_VDDIO    (1 << 2)
#define POWER_INFO_VDDMEM   (1 << 3)
#define POWER_INFO_DCDC     (1 << 4)
#define POWER_INFO_ALL      0x1f

struct imx233_power_info_t imx233_power_get_info(unsigned flags);

#endif /* __POWER_IMX233__ */
