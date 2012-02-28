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
#define HW_POWER_5VCTRL__ENABLE_DCDC        (1 << 0)
#define HW_POWER_5VCTRL__PWRUP_VBUS_CMPS    (1 << 1)
#define HW_POWER_5VCTRL__VBUSVALID_5VDETECT (1 << 4)
#define HW_POWER_5VCTRL__DCDC_XFER          (1 << 5)
#define HW_POWER_5VCTRL__VBUSVALID_TRSH_BP  8
#define HW_POWER_5VCTRL__VBUSVALID_TRSH_BM  (0x7 << 8)
#define HW_POWER_5VCTRL__VBUSVALID_TRSH_2p9 (0 << 8)
#define HW_POWER_5VCTRL__VBUSVALID_TRSH_4V  (1 << 8)
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT_BP   12
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT_BM   (0x3f << 12)
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT__10mA    (1 << 12)
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT__20mA    (1 << 13)
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT__50mA    (1 << 14)
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT__100mA   (1 << 15)
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT__200mA   (1 << 16)
#define HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT__400mA   (1 << 17)
#define HW_POWER_5VCTRL__PWD_CHARGE_4P2     (1 << 20)

#define HW_POWER_MINPWR         (*(volatile uint32_t *)(HW_POWER_BASE + 0x20))
#define HW_POWER_MINPWR__HALF_FETS  (1 << 5)
#define HW_POWER_MINPWR__DOUBLE_FETS    (1 << 6)

#define HW_POWER_CHARGE         (*(volatile uint32_t *)(HW_POWER_BASE + 0x30))
#define HW_POWER_CHARGE__BATTCHRG_I_BP  0
#define HW_POWER_CHARGE__BATTCHRG_I_BM  0x3f
#define HW_POWER_CHARGE__BATTCHRG_I__10mA   (1 << 0)
#define HW_POWER_CHARGE__BATTCHRG_I__20mA   (1 << 1)
#define HW_POWER_CHARGE__BATTCHRG_I__50mA   (1 << 2)
#define HW_POWER_CHARGE__BATTCHRG_I__100mA  (1 << 3)
#define HW_POWER_CHARGE__BATTCHRG_I__200mA  (1 << 4)
#define HW_POWER_CHARGE__BATTCHRG_I__400mA  (1 << 5)
#define HW_POWER_CHARGE__STOP_ILIMIT_BP 8
#define HW_POWER_CHARGE__STOP_ILIMIT_BM 0xf00
#define HW_POWER_CHARGE__STOP_ILIMIT__10mA  (1 << 8)
#define HW_POWER_CHARGE__STOP_ILIMIT__20mA  (1 << 9)
#define HW_POWER_CHARGE__STOP_ILIMIT__50mA  (1 << 10)
#define HW_POWER_CHARGE__STOP_ILIMIT__100mA (1 << 11)
#define HW_POWER_CHARGE__PWD_BATTCHRG   (1 << 16)
#define HW_POWER_CHARGE__CHRG_STS_OFF   (1 << 19)
#define HW_POWER_CHARGE__ENABLE_LOAD    (1 << 22)

#define HW_POWER_VDDDCTRL       (*(volatile uint32_t *)(HW_POWER_BASE + 0x40))
#define HW_POWER_VDDDCTRL__TRG_BP   0
#define HW_POWER_VDDDCTRL__TRG_BM   0x1f
#define HW_POWER_VDDDCTRL__TRG_STEP 25 /* mV */
#define HW_POWER_VDDDCTRL__TRG_MIN  800 /* mV */
#define HW_POWER_VDDDCTRL__LINREG_OFFSET_BP 16
#define HW_POWER_VDDDCTRL__LINREG_OFFSET_BM (0x3 << 16)
#define HW_POWER_VDDDCTRL__ENABLE_LINREG    (1 << 21)

#define HW_POWER_VDDACTRL       (*(volatile uint32_t *)(HW_POWER_BASE + 0x50))
#define HW_POWER_VDDACTRL__TRG_BP   0
#define HW_POWER_VDDACTRL__TRG_BM   0x1f
#define HW_POWER_VDDACTRL__TRG_STEP 25 /* mV */
#define HW_POWER_VDDACTRL__TRG_MIN  1500 /* mV */
#define HW_POWER_VDDACTRL__LINREG_OFFSET_BP 12
#define HW_POWER_VDDACTRL__LINREG_OFFSET_BM (0x3 << 12)
#define HW_POWER_VDDACTRL__ENABLE_LINREG    (1 << 17)

#define HW_POWER_VDDIOCTRL      (*(volatile uint32_t *)(HW_POWER_BASE + 0x60))
#define HW_POWER_VDDIOCTRL__TRG_BP  0
#define HW_POWER_VDDIOCTRL__TRG_BM  0x1f
#define HW_POWER_VDDIOCTRL__TRG_STEP    25 /* mV */
#define HW_POWER_VDDIOCTRL__TRG_MIN 2800 /* mV */
#define HW_POWER_VDDIOCTRL__LINREG_OFFSET_BP    12
#define HW_POWER_VDDIOCTRL__LINREG_OFFSET_BM    (0x3 << 12)

#define HW_POWER_VDDMEMCTRL     (*(volatile uint32_t *)(HW_POWER_BASE + 0x70))
#define HW_POWER_VDDMEMCTRL__TRG_BP  0
#define HW_POWER_VDDMEMCTRL__TRG_BM  0x1f
#define HW_POWER_VDDMEMCTRL__TRG_STEP    50 /* mV */
#define HW_POWER_VDDMEMCTRL__TRG_MIN 1700 /* mV */
#define HW_POWER_VDDMEMCTRL__ENABLE_LINREG  (1 << 8)

#define HW_POWER_DCDC4P2        (*(volatile uint32_t *)(HW_POWER_BASE + 0x80))
#define HW_POWER_DCDC4P2__CMPTRIP_BP    0
#define HW_POWER_DCDC4P2__CMPTRIP_BM    0x1f
#define HW_POWER_DCDC4P2__CMPTRIP__0p85 0
#define HW_POWER_DCDC4P2__ENABLE_DCDC   (1 << 22)
#define HW_POWER_DCDC4P2__ENABLE_4P2    (1 << 23)
#define HW_POWER_DCDC4P2__DROPOUT_CTRL_BP   28
#define HW_POWER_DCDC4P2__DROPOUT_CTRL_BM   (0xf << 28)
#define HW_POWER_DCDC4P2__DROPOUT_CTRL__200mV   (3 << 30)
#define HW_POWER_DCDC4P2__DROPOUT_CTRL__HIGHER  (2 << 28)

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

#define HW_POWER_LOOPCTRL       (*(volatile uint32_t *)(HW_POWER_BASE + 0xb0))
#define HW_POWER_LOOPCTRL__DC_C_BP  0
#define HW_POWER_LOOPCTRL__DC_C_BM  0x3
#define HW_POWER_LOOPCTRL__DC_R_BP  4
#define HW_POWER_LOOPCTRL__DC_R_BM  0xf0
#define HW_POWER_LOOPCTRL__DC_FF_BP 8
#define HW_POWER_LOOPCTRL__DC_FF_BM (0x7 << 8)
#define HW_POWER_LOOPCTRL__EN_RCSCALE_BP    12
#define HW_POWER_LOOPCTRL__EN_RCSCALE_BM    (0x3 << 12)
#define HW_POWER_LOOPCTRL__EN_RCSCALE__DISABLED 0
#define HW_POWER_LOOPCTRL__EN_RCSCALE__2X       1
#define HW_POWER_LOOPCTRL__EN_RCSCALE__4X       2
#define HW_POWER_LOOPCTRL__EN_RCSCALE__8X       3
#define HW_POWER_LOOPCTRL__RCSCALE_THRESH   (1 << 14)
#define HW_POWER_LOOPCTRL__DF_HYST_THRESH   (1 << 15)
#define HW_POWER_LOOPCTRL__CM_HYST_THRESH   (1 << 16)
#define HW_POWER_LOOPCTRL__EN_DF_HYST       (1 << 17)
#define HW_POWER_LOOPCTRL__EN_CM_HYST       (1 << 18)
#define HW_POWER_LOOPCTRL__HYST_SIGN        (1 << 19)
#define HW_POWER_LOOPCTRL__TOGGLE_DIF       (1 << 20)

#define HW_POWER_STS            (*(volatile uint32_t *)(HW_POWER_BASE + 0xc0))
#define HW_POWER_STS__VBUSVALID     (1 << 1)
#define HW_POWER_STS__CHRGSTS       (1 << 11)
#define HW_POWER_STS__PSWITCH_BP    20
#define HW_POWER_STS__PSWITCH_BM    (3 << 20)
#define HW_POWER_STS__PWRUP_SOURCE_BP   24
#define HW_POWER_STS__PWRUP_SOURCE_BM   (0x3f << 24)

#define HW_POWER_BATTMONITOR    (*(volatile uint32_t *)(HW_POWER_BASE + 0xe0))
#define HW_POWER_BATTMONITOR__ENBATADJ      (1 << 10)
#define HW_POWER_BATTMONITOR__BATT_VAL_BP   16
#define HW_POWER_BATTMONITOR__BATT_VAL_BM   (0x3ff << 16)

#define HW_POWER_RESET          (*(volatile uint32_t *)(HW_POWER_BASE + 0x100))
#define HW_POWER_RESET__UNLOCK  0x3E770000
#define HW_POWER_RESET__PWD     0x1

void imx233_power_set_charge_current(unsigned current); /* in mA */
void imx233_power_set_stop_current(unsigned current); /* in mA */
void imx233_power_enable_batadj(bool enable);

static inline void imx233_power_set_dcdc_freq(bool pll, unsigned freq)
{
    HW_POWER_MISC &= ~(HW_POWER_MISC__SEL_PLLCLK | HW_POWER_MISC__FREQSEL_BM);
    /* WARNING: HW_POWER_MISC does have a SET/CLR variant ! */
    if(pll)
    {
        HW_POWER_MISC |= freq << HW_POWER_MISC__FREQSEL_BP;
        HW_POWER_MISC |= HW_POWER_MISC__SEL_PLLCLK;
    }
}

struct imx233_power_info_t
{
    int vddd; /* in mV */
    bool vddd_linreg; /* VDDD source: linreg from VDDA or DC-DC */
    int vddd_linreg_offset;
    int vdda; /* in mV */
    bool vdda_linreg; /* VDDA source: linreg from VDDIO or DC-DC */
    int vdda_linreg_offset;
    int vddio; /* in mV */
    int vddio_linreg_offset;
    int vddmem; /* in mV */
    bool vddmem_linreg; /* VDDMEM source: linreg from VDDIO or off */
    bool dcdc_sel_pllclk; /* clock source of DC-DC: pll or 24MHz xtal */
    int dcdc_freqsel;
    int charge_current;
    int stop_current;
    bool charging;
    bool batt_adj;
    bool _4p2_enable;
    bool _4p2_dcdc;
    int _4p2_cmptrip;
    int _4p2_dropout;
    bool _5v_pwd_charge_4p2;
    int _5v_charge_4p2_limit;
    bool _5v_dcdc_xfer;
    bool _5v_enable_dcdc;
    int _5v_vbusvalid_thr;
    bool _5v_vbusvalid_detect;
    bool _5v_vbus_cmps;
};

#define POWER_INFO_VDDD     (1 << 0)
#define POWER_INFO_VDDA     (1 << 1)
#define POWER_INFO_VDDIO    (1 << 2)
#define POWER_INFO_VDDMEM   (1 << 3)
#define POWER_INFO_DCDC     (1 << 4)
#define POWER_INFO_CHARGE   (1 << 5)
#define POWER_INFO_4P2      (1 << 6)
#define POWER_INFO_5V       (1 << 7)
#define POWER_INFO_ALL      0xff

struct imx233_power_info_t imx233_power_get_info(unsigned flags);

#endif /* __POWER_IMX233__ */
