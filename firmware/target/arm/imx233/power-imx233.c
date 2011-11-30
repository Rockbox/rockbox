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

#include "config.h"
#include "system.h"
#include "power.h"
#include "string.h"
#include "system-target.h"
#include "usb-target.h"

void INT_VDD5V(void)
{
    if(HW_POWER_CTRL & HW_POWER_CTRL__VBUSVALID_IRQ)
    {
        if(HW_POWER_STS & HW_POWER_STS__VBUSVALID)
            usb_insert_int();
        else
            usb_remove_int();
        /* reverse polarity */
        __REG_TOG(HW_POWER_CTRL) = HW_POWER_CTRL__POLARITY_VBUSVALID;
        /* enable int */
        __REG_CLR(HW_POWER_CTRL) = HW_POWER_CTRL__VBUSVALID_IRQ;
    }
}

void power_init(void)
{
    /* clear vbusvalid irq and set correct polarity */
    __REG_CLR(HW_POWER_CTRL) = HW_POWER_CTRL__VBUSVALID_IRQ;
    if(HW_POWER_STS & HW_POWER_STS__VBUSVALID)
        __REG_CLR(HW_POWER_CTRL) = HW_POWER_CTRL__POLARITY_VBUSVALID;
    else
        __REG_SET(HW_POWER_CTRL) = HW_POWER_CTRL__POLARITY_VBUSVALID;
    __REG_SET(HW_POWER_CTRL) = HW_POWER_CTRL__ENIRQ_VBUS_VALID;
    imx233_enable_interrupt(INT_SRC_VDD5V, true);
}

void power_off(void)
{
    /* wait a bit, useful for the user to stop touching anything */
    sleep(HZ / 2);
    /* power down */
    HW_POWER_RESET = HW_POWER_RESET__UNLOCK | HW_POWER_RESET__PWD;
    while(1);
}

unsigned int power_input_status(void)
{
    return POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return false;
}

struct imx233_power_info_t imx233_power_get_info(unsigned flags)
{
    static int dcdc_freqsel[8] = {
        [HW_POWER_MISC__FREQSEL__RES] = 0,
        [HW_POWER_MISC__FREQSEL__20MHz] = 20000,
        [HW_POWER_MISC__FREQSEL__24MHz] = 24000,
        [HW_POWER_MISC__FREQSEL__19p2MHz] = 19200,
        [HW_POWER_MISC__FREQSEL__14p4MHz] = 14200,
        [HW_POWER_MISC__FREQSEL__18MHz] = 18000,
        [HW_POWER_MISC__FREQSEL__21p6MHz] = 21600,
        [HW_POWER_MISC__FREQSEL__17p28MHz] = 17280,
    };
    
    struct imx233_power_info_t s;
    memset(&s, 0, sizeof(s));
    if(flags & POWER_INFO_VDDD)
    {
        s.vddd = HW_POWER_VDDDCTRL__TRG_MIN + HW_POWER_VDDDCTRL__TRG_STEP * __XTRACT(HW_POWER_VDDDCTRL, TRG);
        s.vddd_linreg = HW_POWER_VDDDCTRL & HW_POWER_VDDDCTRL__ENABLE_LINREG;
    }
    if(flags & POWER_INFO_VDDA)
    {
        s.vdda = HW_POWER_VDDACTRL__TRG_MIN + HW_POWER_VDDACTRL__TRG_STEP * __XTRACT(HW_POWER_VDDACTRL, TRG);
        s.vdda_linreg = HW_POWER_VDDACTRL & HW_POWER_VDDACTRL__ENABLE_LINREG;
    }
    if(flags & POWER_INFO_VDDIO)
        s.vddio = HW_POWER_VDDIOCTRL__TRG_MIN + HW_POWER_VDDIOCTRL__TRG_STEP * __XTRACT(HW_POWER_VDDIOCTRL, TRG);
    if(flags & POWER_INFO_VDDMEM)
    {
        s.vddmem = HW_POWER_VDDMEMCTRL__TRG_MIN + HW_POWER_VDDMEMCTRL__TRG_STEP * __XTRACT(HW_POWER_VDDMEMCTRL, TRG);
        s.vddmem_linreg = HW_POWER_VDDMEMCTRL & HW_POWER_VDDMEMCTRL__ENABLE_LINREG;
    }
    if(flags & POWER_INFO_DCDC)
    {
        s.dcdc_sel_pllclk = HW_POWER_MISC & HW_POWER_MISC__SEL_PLLCLK;
        s.dcdc_freqsel = dcdc_freqsel[__XTRACT(HW_POWER_MISC, FREQSEL)];
    }
    return s;
}
