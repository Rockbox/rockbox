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
#include "rtc-imx233.h"

#include "regs/power.h"

#define BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__10mA    (1 << 0)
#define BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__20mA    (1 << 1)
#define BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__50mA    (1 << 2)
#define BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__100mA   (1 << 3)
#define BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__200mA   (1 << 4)
#define BV_POWER_5VCTRL_CHARGE_4P2_ILIMIT__400mA   (1 << 5)


#define BV_POWER_CHARGE_BATTCHRG_I__10mA   (1 << 0)
#define BV_POWER_CHARGE_BATTCHRG_I__20mA   (1 << 1)
#define BV_POWER_CHARGE_BATTCHRG_I__50mA   (1 << 2)
#define BV_POWER_CHARGE_BATTCHRG_I__100mA  (1 << 3)
#define BV_POWER_CHARGE_BATTCHRG_I__200mA  (1 << 4)
#define BV_POWER_CHARGE_BATTCHRG_I__400mA  (1 << 5)

#define BV_POWER_CHARGE_STOP_ILIMIT__10mA  (1 << 0)
#define BV_POWER_CHARGE_STOP_ILIMIT__20mA  (1 << 1)
#define BV_POWER_CHARGE_STOP_ILIMIT__50mA  (1 << 2)
#define BV_POWER_CHARGE_STOP_ILIMIT__100mA (1 << 3)

#if IMX233_SUBTARGET >= 3700
#define HW_POWER_VDDDCTRL__TRG_STEP 25 /* mV */
#define HW_POWER_VDDDCTRL__TRG_MIN  800 /* mV */

#define HW_POWER_VDDACTRL__TRG_STEP 25 /* mV */
#define HW_POWER_VDDACTRL__TRG_MIN  1500 /* mV */

#define HW_POWER_VDDIOCTRL__TRG_STEP    25 /* mV */
#define HW_POWER_VDDIOCTRL__TRG_MIN 2800 /* mV */

#define HW_POWER_VDDMEMCTRL__TRG_STEP    50 /* mV */
#define HW_POWER_VDDMEMCTRL__TRG_MIN 1700 /* mV */
#else
/* don't use the full available range because of the weird encodings for
 * extreme values which are useless anyway */
#define HW_POWER_VDDDCTRL__TRG_STEP 32 /* mV */
#define HW_POWER_VDDDCTRL__TRG_MIN  1280 /* mV */
#define HW_POWER_VDDDCTRL__TRG_OFF  8 /* below 8, the register value doesn't encode linearly */
#endif

#define BV_POWER_MISC_FREQSEL__RES         0
#define BV_POWER_MISC_FREQSEL__20MHz       1
#define BV_POWER_MISC_FREQSEL__24MHz       2
#define BV_POWER_MISC_FREQSEL__19p2MHz     3
#define BV_POWER_MISC_FREQSEL__14p4MHz     4
#define BV_POWER_MISC_FREQSEL__18MHz       5
#define BV_POWER_MISC_FREQSEL__21p6MHz     6
#define BV_POWER_MISC_FREQSEL__17p28MHz    7

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

#if IMX233_SUBTARGET >= 3780
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
#endif

/* FIXME
 * POWER_STS.VBUSVALID does not reflect the actual vbusvalid value, only
 * VBUSVALID_STATUS does. Indeed the VBUSVALID field can be locked using
 * VBUSVALIDPIOLOCK. Some Freescale code suggests locking is required for
 * proper operation of the USB ARC core. This is problematic though
 * because it prevents proper usage of the VDD5V irq.
 * Since we didn't encounter this problem, we never lock VBUSVALID
 *
 * WARNING
 * Using VBUSVALID irq on STMP3700 seems broken, once the irq is fired,
 * it cannot be acked. Currently fallback to the VDD5V>VDDIO method.
 */
#if IMX233_SUBTARGET >= 3780
#define USE_VBUSVALID
#endif

bool imx233_power_usb_detect(void)
{
#ifdef USE_VBUSVALID
    return BF_RD(POWER_STS, VBUSVALID);
#else
    return BF_RD(POWER_STS, VDD5V_GT_VDDIO);
#endif
}

void INT_VDD5V(void)
{
#ifdef USE_VBUSVALID
    if(BF_RD(POWER_CTRL, VBUSVALID_IRQ))
    {
        if(BF_RD(POWER_STS, VBUSVALID))
            usb_insert_int();
        else
            usb_remove_int();
        /* reverse polarity */
        BF_WR(POWER_CTRL_TOG, POLARITY_VBUSVALID(1));
        /* clear int */
        BF_CLR(POWER_CTRL, VBUSVALID_IRQ);
    }
#else
    if(BF_RD(POWER_CTRL, VDD5V_GT_VDDIO_IRQ))
    {
        if(BF_RD(POWER_STS, VDD5V_GT_VDDIO))
            usb_insert_int();
        else
            usb_remove_int();
        /* reverse polarity */
        BF_WR(POWER_CTRL_TOG, POLARITY_VDD5V_GT_VDDIO(1));
        /* clear int */
        BF_CLR(POWER_CTRL, VDD5V_GT_VDDIO_IRQ);
    }
#endif
}

void imx233_power_init(void)
{
#if IMX233_SUBTARGET >= 3700
    BF_CLR(POWER_MINPWR, HALF_FETS);
#endif
    /* setup vbusvalid parameters: set threshold to 4v and power up comparators */
    BF_CS(POWER_5VCTRL, VBUSVALID_TRSH(1));
#if IMX233_SUBTARGET >= 3780
    BF_SET(POWER_5VCTRL, PWRUP_VBUS_CMPS);
#else
    BF_SET(POWER_5VCTRL, OTG_PWRUP_CMPS);
#endif
    /* enable vbusvalid detection method for the dcdc (improves efficiency) */
    BF_SET(POWER_5VCTRL, VBUSVALID_5VDETECT);
    /* disable shutdown on 5V fail */
    BF_CLR(POWER_5VCTRL, PWDN_5VBRNOUT);
#ifdef USE_VBUSVALID
    /* make sure VBUSVALID is unlocked */
    BF_CLR(POWER_DEBUG, VBUSVALIDPIOLOCK);
    /* clear vbusvalid irq and set correct polarity */
    BF_CLR(POWER_CTRL, VBUSVALID_IRQ);
    if(BF_RD(POWER_STS, VBUSVALID))
        BF_CLR(POWER_CTRL, POLARITY_VBUSVALID);
    else
        BF_SET(POWER_CTRL, POLARITY_VBUSVALID);
    BF_SET(POWER_CTRL, ENIRQ_VBUS_VALID);
    /* make sure old detection way is not enabled */
    BF_CLR(POWER_CTRL, ENIRQ_VDD5V_GT_VDDIO);
#else
    BF_CLR(POWER_CTRL,  VDD5V_GT_VDDIO_IRQ);
    if(BF_RD(POWER_STS, VDD5V_GT_VDDIO))
        BF_CLR(POWER_CTRL, POLARITY_VDD5V_GT_VDDIO);
    else
        BF_SET(POWER_CTRL, POLARITY_VDD5V_GT_VDDIO);
    BF_SET(POWER_CTRL, ENIRQ_VDD5V_GT_VDDIO);
    /* make the vbusvalid detection way is not enabled */
#if IMX233_SUBTARGET >= 3700
    BF_CLR(POWER_CTRL, ENIRQ_VBUS_VALID);
#endif
#endif
    /* the VDD5V IRQ is shared by several sources, disable them */
#if IMX233_SUBTARGET >= 3700
    BF_CLR(POWER_CTRL, ENIRQ_PSWITCH);
    BF_CLR(POWER_CTRL, ENIRQ_DC_OK);
#if IMX233_SUBTARGET < 3780
    BF_CLR(POWER_CTRL, ENIRQ_LINREG_OK);
#endif /* IMX233_SUBTARGET < 3780 */
#endif /* IMX233_SUBTARGET >= 3700 */
    imx233_icoll_enable_interrupt(INT_SRC_VDD5V, true);
}

void power_init(void)
{
}

void power_off(void)
{
    imx233_system_prepare_shutdown();
    /* power down */
    HW_POWER_RESET = BF_OR(POWER_RESET, UNLOCK_V(KEY), PWD(1));
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
#if IMX233_SUBTARGET >= 3700
    BF_CLR(POWER_CHARGE, BATTCHRG_I);
#else
    BF_CLR(POWER_BATTCHRG, BATTCHRG_I);
#endif
    /* find closest current LOWER THAN OR EQUAL TO the expected current */
    for(unsigned i = 0; i < ARRAYLEN(g_charger_current_bits); i++)
        if(current >= g_charger_current_bits[i].current)
        {
            current -= g_charger_current_bits[i].current;
#if IMX233_SUBTARGET >= 3700
            BF_WR(POWER_CHARGE_SET, BATTCHRG_I(g_charger_current_bits[i].bit));
#else
            BF_WR(POWER_BATTCHRG_SET, BATTCHRG_I(g_charger_current_bits[i].bit));
#endif
        }
}

void imx233_power_set_stop_current(unsigned current)
{
#if IMX233_SUBTARGET >= 3700
    BF_CLR(POWER_CHARGE, STOP_ILIMIT);
#else
    BF_CLR(POWER_BATTCHRG, STOP_ILIMIT);
#endif
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
#if IMX233_SUBTARGET >= 3700
            BF_WR(POWER_CHARGE_SET, STOP_ILIMIT(g_charger_stop_current_bits[i].bit));
#else
            BF_WR(POWER_BATTCHRG_SET, STOP_ILIMIT(g_charger_stop_current_bits[i].bit));
#endif
        }
    }
}

/* regulator info */
#define HAS_BO              (1 << 0)
#define HAS_LINREG          (1 << 1)
#define HAS_LINREG_OFFSET   (1 << 2)
#define HAS_ABS_BO          (1 << 3)

static struct
{
    unsigned min, step;
    int off; // offset in the register value
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
        .flags = mask, \
        .off = 0
#define ADD_REGULATOR_BO(name) \
        .bo_bm = BM_POWER_##name##CTRL_BO_OFFSET, \
        .bo_bp = BP_POWER_##name##CTRL_BO_OFFSET
#define ADD_REGULATOR_LINREG(name) \
        .linreg_bm = BM_POWER_##name##CTRL_ENABLE_LINREG
#define ADD_REGULATOR_LINREG_OFFSET(name) \
        .linreg_offset_bm = BP_POWER_##name##CTRL_LINREG_OFFSET, \
        .linreg_offset_bp = BM_POWER_##name##CTRL_LINREG_OFFSET

#if IMX233_SUBTARGET >= 3700
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
#if IMX233_SUBTARGET >= 3780
    [REGULATOR_VDDMEM] =
    {
        ADD_REGULATOR(VDDMEM, HAS_LINREG),
        ADD_REGULATOR_LINREG(VDDMEM),
    },
#endif
#else
    [REGULATOR_VDDD] =
    {
        .min = HW_POWER_VDDDCTRL__TRG_MIN,
        .step = HW_POWER_VDDDCTRL__TRG_STEP,
        .off = HW_POWER_VDDDCTRL__TRG_OFF,
        .reg = &HW_POWER_VDDCTRL,
        .flags = HAS_BO | HAS_ABS_BO,
        .trg_bm = BM_POWER_VDDCTRL_VDDD_TRG,
        .trg_bp = BP_POWER_VDDCTRL_VDDD_TRG,
        .bo_bm = BM_POWER_VDDCTRL_VDDD_BO,
        .bo_bp = BP_POWER_VDDCTRL_VDDD_BO,
    },
#endif
};

void imx233_power_get_regulator(enum imx233_regulator_t reg, unsigned *value_mv,
    unsigned *brownout_mv)
{
    uint32_t reg_val = *regulator_info[reg].reg;
    /* read target value */
    unsigned raw_val = (reg_val & regulator_info[reg].trg_bm) >> regulator_info[reg].trg_bp;
    raw_val -= regulator_info[reg].off;
    /* convert it to mv */
    if(value_mv)
        *value_mv = regulator_info[reg].min + regulator_info[reg].step * raw_val;
    if(regulator_info[reg].flags & HAS_BO)
    {
        /* read brownout offset */
        unsigned raw_bo = (reg_val & regulator_info[reg].bo_bm) >> regulator_info[reg].bo_bp;
        raw_bo -= regulator_info[reg].off;
        if(!(regulator_info[reg].flags & HAS_ABS_BO))
            raw_bo = raw_val - raw_bo;
        /* convert it to mv */
        if(brownout_mv)
            *brownout_mv = regulator_info[reg].min + regulator_info[reg].step * raw_bo;
    }
    else if(brownout_mv)
        *brownout_mv = 0;
}

#if IMX233_SUBTARGET >= 3700 && IMX233_SUBTARGET < 3780
static void update_dcfuncv(void)
{
    int vddd, vdda, vddio;
    imx233_power_get_regulator(REGULATOR_VDDD, &vddd, NULL);
    imx233_power_get_regulator(REGULATOR_VDDA, &vdda, NULL);
    imx233_power_get_regulator(REGULATOR_VDDIO, &vddio, NULL);
    // assume Li-Ion, to divide by 6.25, do *100 and /625
    BF_WR_ALL(POWER_DCFUNCV, VDDIO(((vddio - vdda) * 100) / 625),
        VDDD(((vdda - vddd) * 100) / 625));
}
#endif

void imx233_power_set_regulator(enum imx233_regulator_t reg, unsigned value_mv,
    unsigned brownout_mv)
{
    // compute raw values
    unsigned raw_val = (value_mv - regulator_info[reg].min) / regulator_info[reg].step;
    raw_val += regulator_info[reg].off;
    if(!(regulator_info[reg].flags & HAS_ABS_BO))
        brownout_mv = value_mv - brownout_mv;
    unsigned raw_bo_offset = brownout_mv/ regulator_info[reg].step;
    raw_bo_offset += regulator_info[reg].off;
    // clear dc-dc ok flag
#if IMX233_SUBTARGET >= 3700
    BF_SET(POWER_CTRL, DC_OK_IRQ);
#endif
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
#if IMX233_SUBTARGET >= 3700
    sleep(1);
#else
    if(!BF_RD(POWER_5VCTRL, EN_DCDC1) || !BF_RD(POWER_5VCTRL, EN_DCDC2))
        panicf("regulator %d: wait for voltage stabilize in linreg mode !", reg);
    unsigned timeout = current_tick + (HZ * 20) / 1000;
    while(!BF_RD(POWER_STS, DC1_OK) || !BF_RD(POWER_STS, DC2_OK) || !TIME_AFTER(current_tick, timeout))
        yield();
    if(!BF_RD(POWER_STS, DC1_OK) || !BF_RD(POWER_STS, DC2_OK))
        panicf("regulator %d: failed to stabilize", reg);
#endif
    /* On STMP37xx, we need to update the weird HW_POWER_DCFUNCV register */
#if IMX233_SUBTARGET >= 3700 && IMX233_SUBTARGET < 3780
    update_dcfuncv();
#endif
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

#if IMX233_SUBTARGET < 3700
int imx233_power_sense_die_temperature(int *min, int *max)
{
    static int die_temp[] =
    {
        -50, -40, -30, -20, -10, 0, 15, 25, 35, 45, 55, 70, 85, 95, 105, 115, 130
    };
    /* power up temperature sensor */
    BF_WR(POWER_SPEEDTEMP_CLR, TEMP_CTRL(1 << 3));
    /* read temp */
    int sense = BF_RD(POWER_SPEEDTEMP, TEMP_STS);
    *min = die_temp[sense];
    *max = die_temp[sense + 1];
    /* power down temperature sensor */
    BF_WR(POWER_SPEEDTEMP_SET, TEMP_CTRL(1 << 3));
    return 0;
}
#endif

struct imx233_power_info_t imx233_power_get_info(unsigned flags)
{
#if IMX233_SUBTARGET >= 3700
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
#endif
    struct imx233_power_info_t s;
    memset(&s, 0, sizeof(s));
#if IMX233_SUBTARGET >= 3700
    if(flags & POWER_INFO_DCDC)
    {
        s.dcdc_sel_pllclk = BF_RD(POWER_MISC, SEL_PLLCLK);
        s.dcdc_freqsel = dcdc_freqsel[BF_RD(POWER_MISC, FREQSEL)];
    }
#endif
    if(flags & POWER_INFO_CHARGE)
    {
#if IMX233_SUBTARGET >= 3700
        uint32_t current = BF_RD(POWER_CHARGE, BATTCHRG_I);
        uint32_t stop_current = BF_RD(POWER_CHARGE, STOP_ILIMIT);
#else
        uint32_t current = BF_RD(POWER_BATTCHRG, BATTCHRG_I);
        uint32_t stop_current = BF_RD(POWER_BATTCHRG, STOP_ILIMIT);
#endif
        for(unsigned i = 0; i < ARRAYLEN(g_charger_current_bits); i++)
            if(current & g_charger_current_bits[i].bit)
                s.charge_current += g_charger_current_bits[i].current;
        for(unsigned i = 0; i < ARRAYLEN(g_charger_stop_current_bits); i++)
            if(stop_current & g_charger_stop_current_bits[i].bit)
                s.stop_current += g_charger_stop_current_bits[i].current;
        s.charging = BF_RD(POWER_STS, CHRGSTS);
#if IMX233_SUBTARGET >= 3700
        s.batt_adj = BF_RD(POWER_BATTMONITOR, EN_BATADJ);
#else
        s.batt_adj = BF_RD(POWER_DC1MULTOUT, EN_BATADJ);
#endif
    }
#if IMX233_SUBTARGET >= 3780
    if(flags & POWER_INFO_4P2)
    {
        s._4p2_enable = BF_RD(POWER_DCDC4P2, ENABLE_4P2);
        s._4p2_dcdc = BF_RD(POWER_DCDC4P2, ENABLE_DCDC);
        s._4p2_cmptrip = BF_RD(POWER_DCDC4P2, CMPTRIP);
        s._4p2_dropout = BF_RD(POWER_DCDC4P2, DROPOUT_CTRL);
    }
#endif
    if(flags & POWER_INFO_5V)
    {
#if IMX233_SUBTARGET >= 3780
        s._5v_pwd_charge_4p2 = BF_RD(POWER_5VCTRL, PWD_CHARGE_4P2);
#endif
        s._5v_dcdc_xfer = BF_RD(POWER_5VCTRL, DCDC_XFER);
#if IMX233_SUBTARGET >= 3700
        s._5v_enable_dcdc = BF_RD(POWER_5VCTRL, ENABLE_DCDC);
#else
        s._5v_enable_dcdc = BF_RD(POWER_5VCTRL, EN_DCDC1) && BF_RD(POWER_5VCTRL, EN_DCDC2);
#endif
#if IMX233_SUBTARGET >= 3780
        uint32_t charge_4p2_ilimit = BF_RD(POWER_5VCTRL, CHARGE_4P2_ILIMIT);
        for(unsigned i = 0; i < ARRAYLEN(g_4p2_charge_limit_bits); i++)
            if(charge_4p2_ilimit & g_4p2_charge_limit_bits[i].bit)
                s._5v_charge_4p2_limit += g_4p2_charge_limit_bits[i].current;
#endif
        s._5v_vbusvalid_detect = BF_RD(POWER_5VCTRL, VBUSVALID_5VDETECT);
#if IMX233_SUBTARGET >= 3780
        s._5v_vbus_cmps = BF_RD(POWER_5VCTRL, PWRUP_VBUS_CMPS);
#else
        s._5v_vbus_cmps = BF_RD(POWER_5VCTRL, OTG_PWRUP_CMPS);
#endif
        s._5v_vbusvalid_thr =
            BF_RD(POWER_5VCTRL, VBUSVALID_TRSH) == 0 ?
                2900
                : 3900 + BF_RD(POWER_5VCTRL, VBUSVALID_TRSH) * 100;
    }
    return s;
}
