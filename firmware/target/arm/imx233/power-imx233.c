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
#include "usb.h"
#include "system-target.h"
#include "power-imx233.h"
#include "pinctrl-imx233.h"
#include "fmradio_i2c.h"

struct current_step_bit_t
{
    unsigned current;
    uint32_t bit;
};

/* in decreasing order */
static struct current_step_bit_t g_charger_current_bits[] =
{
    { 400, BV_POWER_CHARGE_BATTCHRG_I__400mA },
    { 200, BV_POWER_CHARGE_BATTCHRG_I__200mA },
    { 100, BV_POWER_CHARGE_BATTCHRG_I__100mA },
    { 50, BV_POWER_CHARGE_BATTCHRG_I__50mA },
    { 20, BV_POWER_CHARGE_BATTCHRG_I__20mA },
    { 10, BV_POWER_CHARGE_BATTCHRG_I__10mA }
};

/* in decreasing order */
static struct current_step_bit_t g_charger_stop_current_bits[] =
{
    { 100, BV_POWER_CHARGE_STOP_ILIMIT__100mA },
    { 50, BV_POWER_CHARGE_STOP_ILIMIT__50mA },
    { 20, BV_POWER_CHARGE_STOP_ILIMIT__20mA },
    { 10, BV_POWER_CHARGE_STOP_ILIMIT__10mA }
};

/* in decreasing order */
static struct current_step_bit_t g_4p2_charge_limit_bits[] =
{
    { 400, BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__400mA },
    { 200, BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__200mA },
    { 100, BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__100mA },
    { 50, BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__50mA },
    { 20, BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__20mA },
    { 10, BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__10mA }
};

void INT_VDD5V(void)
{
    if(BF_RD(POWER_CTRL, VBUSVALID_IRQ))
    {
        if(BF_RD(POWER_STS, VBUSVALID))
            usb_insert_int();
        else
            usb_remove_int();
        /* reverse polarity */
        BF_TOG(POWER_CTRL, POLARITY_VBUSVALID);
        /* enable int */
        BF_CLR(POWER_CTRL, VBUSVALID_IRQ);
    }
}

void imx233_power_init(void)
{
    /* setup vbusvalid parameters: set threshold to 4v and power up comparators */
    BF_CLR(POWER_5VCTRL, VBUSVALID_TRSH);
    BF_SETV(POWER_5VCTRL, VBUSVALID_TRSH, 1);
    BF_SET(POWER_5VCTRL, PWRUP_VBUS_CMPS);
    /* enable vbusvalid detection method for the dcdc (improves efficiency) */
    BF_SET(POWER_5VCTRL, VBUSVALID_5VDETECT);
    BF_CLR(POWER_CTRL, VBUSVALID_IRQ);
    if(BF_RD(POWER_STS, VBUSVALID))
        BF_CLR(POWER_CTRL, POLARITY_VBUSVALID);
    else
        BF_SET(POWER_CTRL, POLARITY_VBUSVALID);
    BF_SET(POWER_CTRL, ENIRQ_VBUS_VALID);
    imx233_icoll_enable_interrupt(INT_SRC_VDD5V, true);
    /* setup linear regulator offsets to 25 mV below to prevent contention between
     * linear regulators and DCDC */
    BF_WR(POWER_VDDDCTRL, LINREG_OFFSET, 2);
    BF_WR(POWER_VDDACTRL, LINREG_OFFSET, 2);
    BF_WR(POWER_VDDIOCTRL, LINREG_OFFSET, 2);
    /* enable DCDC (more efficient) */
    BF_SET(POWER_5VCTRL, ENABLE_DCDC);
    /* enable a few bits controlling the DC-DC as recommended by Freescale */
    BF_SET(POWER_LOOPCTRL, TOGGLE_DIF);
    BF_SET(POWER_LOOPCTRL, EN_CM_HYST);
    BF_CLR(POWER_LOOPCTRL, EN_RCSCALE);
    BF_SETV(POWER_LOOPCTRL, EN_RCSCALE, 1);
}

void power_init(void)
{
}

void power_off(void)
{
    /* wait a bit, useful for the user to stop touching anything */
    sleep(HZ / 2);
#ifdef SANSA_FUZEPLUS
    /* This pin seems to be important to shutdown the hardware properly */
    imx233_pinctrl_acquire(0, 9, "power off");
    imx233_pinctrl_set_function(0, 9, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 9, true);
    imx233_pinctrl_set_gpio(0, 9, true);
#endif
    /* power down */
    HW_POWER_RESET = BM_OR2(POWER_RESET, UNLOCK, PWD);
    while(1);
}

unsigned int power_input_status(void)
{
    return (usb_detect() == USB_INSERTED)
        ? POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return BF_RD(POWER_STS, CHRGSTS);
}

void imx233_power_set_charge_current(unsigned current)
{
    BF_CLR(POWER_CHARGE, BATTCHRG_I);
    /* find closest current LOWER THAN OR EQUAL TO the expected current */
    for(unsigned i = 0; i < ARRAYLEN(g_charger_current_bits); i++)
        if(current >= g_charger_current_bits[i].current)
        {
            current -= g_charger_current_bits[i].current;
            BF_SETV(POWER_CHARGE, BATTCHRG_I, g_charger_current_bits[i].bit);
        }
}

void imx233_power_set_stop_current(unsigned current)
{
    BF_CLR(POWER_CHARGE, STOP_ILIMIT);
    /* find closest current GREATHER THAN OR EQUAL TO the expected current */
    unsigned sum = 0;
    for(unsigned i = 0; i < ARRAYLEN(g_charger_stop_current_bits); i++)
        sum += g_charger_stop_current_bits[i].current;
    for(unsigned i = 0; i < ARRAYLEN(g_charger_stop_current_bits); i++)
    {
        sum -= g_charger_stop_current_bits[i].current;
        if(current > sum)
        {
            current -= g_charger_stop_current_bits[i].current;
            BF_SETV(POWER_CHARGE, STOP_ILIMIT, g_charger_stop_current_bits[i].bit);
        }
    }
}

/* regulator info */
#define HAS_BO              (1 << 0)
#define HAS_LINREG          (1 << 1)
#define HAS_LINREG_OFFSET   (1 << 2)

static struct
{
    unsigned min, step;
    volatile uint32_t *reg;
    uint32_t trg_bm, trg_bp; // bitmask and bitpos
    unsigned flags;
    uint32_t bo_bm, bo_bp; // bitmask and bitpos
    uint32_t linreg_bm;
    uint32_t linreg_offset_bm, linreg_offset_bp; // bitmask and bitpos
} regulator_info[] =
{
#define ADD_REGULATOR(name, mask) \
        .min = HW_POWER_##name##CTRL__TRG_MIN, \
        .step = HW_POWER_##name##CTRL__TRG_STEP, \
        .reg = &HW_POWER_##name##CTRL, \
        .trg_bm = BM_POWER_##name##CTRL_TRG, \
        .trg_bp = BP_POWER_##name##CTRL_TRG, \
        .flags = mask
#define ADD_REGULATOR_BO(name) \
        .bo_bm = BM_POWER_##name##CTRL_BO_OFFSET, \
        .bo_bp = BP_POWER_##name##CTRL_BO_OFFSET
#define ADD_REGULATOR_LINREG(name) \
        .linreg_bm = BM_POWER_##name##CTRL_ENABLE_LINREG
#define ADD_REGULATOR_LINREG_OFFSET(name) \
        .linreg_offset_bm = BM_POWER_##name##CTRL_LINREG_OFFSET, \
        .linreg_offset_bp = BP_POWER_##name##CTRL_LINREG_OFFSET
    [REGULATOR_VDDD] =
    {
        ADD_REGULATOR(VDDD, HAS_BO|HAS_LINREG|HAS_LINREG_OFFSET),
        ADD_REGULATOR_BO(VDDD),
        ADD_REGULATOR_LINREG(VDDD),
        ADD_REGULATOR_LINREG_OFFSET(VDDD)
    },
    [REGULATOR_VDDA] =
    {
        ADD_REGULATOR(VDDA, HAS_BO|HAS_LINREG|HAS_LINREG_OFFSET),
        ADD_REGULATOR_BO(VDDA),
        ADD_REGULATOR_LINREG(VDDA),
        ADD_REGULATOR_LINREG_OFFSET(VDDA)
    },
    [REGULATOR_VDDIO] =
    {
        ADD_REGULATOR(VDDIO, HAS_BO|HAS_LINREG_OFFSET),
        ADD_REGULATOR_BO(VDDIO),
        ADD_REGULATOR_LINREG_OFFSET(VDDIO)
    },
    [REGULATOR_VDDMEM] =
    {
        ADD_REGULATOR(VDDMEM, HAS_LINREG),
        ADD_REGULATOR_LINREG(VDDMEM),
    },
};

void imx233_power_get_regulator(enum imx233_regulator_t reg, unsigned *value_mv,
    unsigned *brownout_mv)
{
    uint32_t reg_val = *regulator_info[reg].reg;
    /* read target value */
    unsigned raw_val = (reg_val & regulator_info[reg].trg_bm) >> regulator_info[reg].trg_bp;
    /* convert it to mv */
    if(value_mv)
        *value_mv = regulator_info[reg].min + regulator_info[reg].step * raw_val;
    if(regulator_info[reg].flags & HAS_BO)
    {
        /* read brownout offset */
        unsigned raw_bo = (reg_val & regulator_info[reg].bo_bm) >> regulator_info[reg].bo_bp;
        /* convert it to mv */
        if(brownout_mv)
            *brownout_mv = regulator_info[reg].min + regulator_info[reg].step * (raw_val - raw_bo);
    }
    else if(brownout_mv)
        *brownout_mv = 0;
}

void imx233_power_set_regulator(enum imx233_regulator_t reg, unsigned value_mv,
    unsigned brownout_mv)
{
    // compute raw values
    unsigned raw_val = (value_mv - regulator_info[reg].min) / regulator_info[reg].step;
    unsigned raw_bo_offset = (value_mv - brownout_mv) / regulator_info[reg].step;
    // clear dc-dc ok flag
    BF_SET(POWER_CTRL, DC_OK_IRQ);
    // update
    uint32_t reg_val = (*regulator_info[reg].reg) & ~regulator_info[reg].trg_bm;
    reg_val |= raw_val << regulator_info[reg].trg_bp;
    if(regulator_info[reg].flags & HAS_BO)
    {
        reg_val &= ~regulator_info[reg].bo_bm;
        reg_val |= raw_bo_offset << regulator_info[reg].bo_bp;
    }
    *regulator_info[reg].reg = reg_val;
    /* Wait until regulator is stable (ie brownout condition is gone)
     * If DC-DC is used, we can use the DCDC_OK irq
     * Otherwise it is unreliable (doesn't work when lowering voltage on linregs)
     * It usually takes between 0.5ms and 2.5ms */
    if(!BF_RD(POWER_5VCTRL, ENABLE_DCDC))
        panicf("regulator %d: wait for voltage stabilize in linreg mode !", reg);
    unsigned timeout = current_tick + (HZ * 20) / 1000;
    while(!BF_RD(POWER_CTRL, DC_OK_IRQ) || !TIME_AFTER(current_tick, timeout))
        yield();
    if(!BF_RD(POWER_CTRL, DC_OK_IRQ))
        panicf("regulator %d: failed to stabilize", reg);
}

// offset is -1,0 or 1
void imx233_power_get_regulator_linreg(enum imx233_regulator_t reg,
    bool *enabled, int *linreg_offset)
{
    if(enabled && regulator_info[reg].flags & HAS_LINREG)
        *enabled = !!(*regulator_info[reg].reg & regulator_info[reg].linreg_bm);
    else if(enabled)
        *enabled = true;
    if(regulator_info[reg].flags & HAS_LINREG_OFFSET)
    {
        unsigned v = (*regulator_info[reg].reg & regulator_info[reg].linreg_offset_bm);
        v >>= regulator_info[reg].linreg_offset_bp;
        if(linreg_offset)
            *linreg_offset = (v == 0) ? 0 : (v == 1) ? 1 : -1;
    }
    else if(linreg_offset)
        *linreg_offset = 0;
}

// offset is -1,0 or 1
/*
void imx233_power_set_regulator_linreg(enum imx233_regulator_t reg,
    bool enabled, int linreg_offset)
{
}
*/


struct imx233_power_info_t imx233_power_get_info(unsigned flags)
{
    static int dcdc_freqsel[8] = {
        [BV_POWER_MISC_FREQSEL__RES] = 0,
        [BV_POWER_MISC_FREQSEL__20MHz] = 20000,
        [BV_POWER_MISC_FREQSEL__24MHz] = 24000,
        [BV_POWER_MISC_FREQSEL__19p2MHz] = 19200,
        [BV_POWER_MISC_FREQSEL__14p4MHz] = 14200,
        [BV_POWER_MISC_FREQSEL__18MHz] = 18000,
        [BV_POWER_MISC_FREQSEL__21p6MHz] = 21600,
        [BV_POWER_MISC_FREQSEL__17p28MHz] = 17280,
    };
    
    struct imx233_power_info_t s;
    memset(&s, 0, sizeof(s));
    if(flags & POWER_INFO_DCDC)
    {
        s.dcdc_sel_pllclk = BF_RD(POWER_MISC, SEL_PLLCLK);
        s.dcdc_freqsel = dcdc_freqsel[BF_RD(POWER_MISC, FREQSEL)];
    }
    if(flags & POWER_INFO_CHARGE)
    {
        uint32_t current = BF_RD(POWER_CHARGE, BATTCHRG_I);
        uint32_t stop_current = BF_RD(POWER_CHARGE, STOP_ILIMIT);
        for(unsigned i = 0; i < ARRAYLEN(g_charger_current_bits); i++)
            if(current & g_charger_current_bits[i].bit)
                s.charge_current += g_charger_current_bits[i].current;
        for(unsigned i = 0; i < ARRAYLEN(g_charger_stop_current_bits); i++)
            if(stop_current & g_charger_stop_current_bits[i].bit)
                s.stop_current += g_charger_stop_current_bits[i].current;
        s.charging = BF_RD(POWER_STS, CHRGSTS);
        s.batt_adj = BF_RD(POWER_BATTMONITOR, EN_BATADJ);
    }
    if(flags & POWER_INFO_4P2)
    {
        s._4p2_enable = BF_RD(POWER_DCDC4P2, ENABLE_4P2);
        s._4p2_dcdc = BF_RD(POWER_DCDC4P2, ENABLE_DCDC);
        s._4p2_cmptrip = BF_RD(POWER_DCDC4P2, CMPTRIP);
        s._4p2_dropout = BF_RD(POWER_DCDC4P2, DROPOUT_CTRL);
    }
    if(flags & POWER_INFO_5V)
    {
        s._5v_pwd_charge_4p2 = BF_RD(POWER_5VCTRL, PWD_CHARGE_4P2);
        s._5v_enable_dcdc = BF_RD(POWER_5VCTRL, ENABLE_DCDC);
        uint32_t charge_4p2_ilimit = BF_RD(POWER_5VCTRL, CHARGE_4P2_ILIMIT);
        for(unsigned i = 0; i < ARRAYLEN(g_4p2_charge_limit_bits); i++)
            if(charge_4p2_ilimit & g_4p2_charge_limit_bits[i].bit)
                s._5v_charge_4p2_limit += g_4p2_charge_limit_bits[i].current;
        s._5v_vbusvalid_detect = BF_RD(POWER_5VCTRL, VBUSVALID_5VDETECT);
        s._5v_vbus_cmps = BF_RD(POWER_5VCTRL, PWRUP_VBUS_CMPS);
        s._5v_vbusvalid_thr =
            BF_RD(POWER_5VCTRL, VBUSVALID_TRSH) == 0 ?
                2900
                : 3900 + BF_RD(POWER_5VCTRL, VBUSVALID_TRSH) * 100;
    }
    return s;
}
