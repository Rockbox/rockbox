/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "axp192.h"
#include "system.h"
#include "power.h"
#include "i2c-async.h"
#include "logf.h"

/*
 * Direct register access
 */

int axp_read(uint8_t reg)
{
    int ret = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, reg);
    if(ret < 0)
        logf("axp: read reg %02x err=%d", reg, ret);

    return ret;
}

int axp_write(uint8_t reg, uint8_t value)
{
    int ret = i2c_reg_write1(AXP_PMU_BUS, AXP_PMU_ADDR, reg, value);
    if(ret < 0)
        logf("axp: write reg %02x err=%d", reg, ret);

    return ret;
}

int axp_modify(uint8_t reg, uint8_t clr, uint8_t set)
{
    int ret = i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR, reg, clr, set, NULL);
    if(ret < 0)
        logf("axp: modify reg %02x err=%d", reg, ret);

    return ret;
}

/*
 * Power supplies: enable/disable, set voltage
 */

struct axp_supplydata {
    uint8_t en_reg;
    uint8_t en_bit;
    uint8_t volt_reg;
    uint8_t volt_msb: 4;
    uint8_t volt_lsb: 4;
    short min_mV;
    short step_mV;
};

static const struct axp_supplydata supplydata[] = {
    [AXP_SUPPLY_EXTEN] = {
        .en_reg = AXP_REG_PWRCTL1,
        .en_bit = 1 << 2,
        .volt_reg = 0xff, /* undefined */
        .volt_msb = 0xf,
        .volt_lsb = 0xf,
        .min_mV = 0,
        .step_mV = 0,
    },
    [AXP_SUPPLY_DCDC1] = {
        .en_reg = AXP_REG_PWRCTL2,
        .en_bit = 1 << 0,
        .volt_reg = AXP_REG_DCDC1VOLT,
        .volt_msb = 6,
        .volt_lsb = 0,
        .min_mV = 700,
        .step_mV = 25,
    },
    [AXP_SUPPLY_DCDC2] = {
        .en_reg = AXP_REG_PWRCTL1,
        .en_bit = 1 << 0,
        .volt_reg = AXP_REG_DCDC2VOLT,
        .volt_msb = 5,
        .volt_lsb = 0,
        .min_mV = 700,
        .step_mV = 25,
    },
    [AXP_SUPPLY_DCDC3] = {
        .en_reg = AXP_REG_PWRCTL2,
        .en_bit = 1 << 1,
        .volt_reg = AXP_REG_DCDC3VOLT,
        .volt_msb = 6,
        .volt_lsb = 0,
        .min_mV = 700,
        .step_mV = 25,
    },
    [AXP_SUPPLY_LDO2] = {
        .en_reg = AXP_REG_PWRCTL2,
        .en_bit = 1 << 2,
        .volt_reg = AXP_REG_LDO2LDO3VOLT,
        .volt_msb = 7,
        .volt_lsb = 4,
        .min_mV = 1800,
        .step_mV = 100,
    },
    [AXP_SUPPLY_LDO3] = {
        .en_reg = AXP_REG_PWRCTL2,
        .en_bit = 1 << 3,
        .volt_reg = AXP_REG_LDO2LDO3VOLT,
        .volt_msb = 3,
        .volt_lsb = 0,
        .min_mV = 1800,
        .step_mV = 100,
    },
    [AXP_SUPPLY_LDOIO0] = {
        .en_reg = 0xff, /* undefined */
        .en_bit = 0,
        .volt_reg = AXP_REG_GPIO0LDO,
        .volt_msb = 7,
        .volt_lsb = 4,
        .min_mV = 1800,
        .step_mV = 100,
    },
};

void axp_enable_supply(int supply, bool enable)
{
    const struct axp_supplydata* data = &supplydata[supply];
    axp_modify(data->en_reg, data->en_bit, enable ? data->en_bit : 0);
}

void axp_set_enabled_supplies(unsigned int supply_mask)
{
    uint8_t xfer[3];
    xfer[0] = 0;
    xfer[1] = AXP_REG_PWRCTL2;
    xfer[2] = 0;

    for(int i = 0; i < AXP_NUM_SUPPLIES; ++i) {
        if(!(supply_mask & (1 << i)))
            continue;

        const struct axp_supplydata* data = &supplydata[i];
        if(data->en_reg == AXP_REG_PWRCTL1) {
            xfer[0] |= data->en_bit;
            xfer[2] |= data->en_bit << 4; /* HACK: work around AXP quirk */
        } else {
            xfer[2] |= data->en_bit;
        }
    }

    i2c_reg_write(AXP_PMU_BUS, AXP_PMU_ADDR, AXP_REG_PWRCTL1, 3, xfer);
}

void axp_set_supply_voltage(int supply, int output_mV)
{
    const struct axp_supplydata* data = &supplydata[supply];
    uint8_t mask = (1 << (data->volt_msb - data->volt_lsb + 1)) - 1;
    uint8_t value = (output_mV - data->min_mV) / data->step_mV;
    axp_modify(data->volt_reg, mask << data->volt_lsb, value << data->volt_lsb);
}

/*
 * ADC control: enable/disable, read
 */

struct axp_adcdata {
    uint8_t data_reg;
    uint8_t en_reg;
    uint8_t en_bit;
    int8_t num;
    int8_t den;
};

static const struct axp_adcdata adcdata[] = {
    [AXP_ADC_ACIN_VOLTAGE]      = {0x56, AXP_REG_ADCEN1, 1 << 5, 17, 10},
    [AXP_ADC_ACIN_CURRENT]      = {0x58, AXP_REG_ADCEN1, 1 << 4,  5,  8},
    [AXP_ADC_VBUS_VOLTAGE]      = {0x5a, AXP_REG_ADCEN1, 1 << 3, 17, 10},
    [AXP_ADC_VBUS_CURRENT]      = {0x5c, AXP_REG_ADCEN1, 1 << 2,  3,  8},
    [AXP_ADC_INTERNAL_TEMP]     = {0x5e, AXP_REG_ADCEN2, 1 << 7,  0,  0},
    [AXP_ADC_TS_INPUT]          = {0x62, AXP_REG_ADCEN1, 1 << 0,  4,  5},
    [AXP_ADC_GPIO0]             = {0x64, AXP_REG_ADCEN2, 1 << 3,  1,  2},
    [AXP_ADC_GPIO1]             = {0x66, AXP_REG_ADCEN2, 1 << 2,  1,  2},
    [AXP_ADC_GPIO2]             = {0x68, AXP_REG_ADCEN2, 1 << 1,  1,  2},
    [AXP_ADC_GPIO3]             = {0x6a, AXP_REG_ADCEN2, 1 << 0,  1,  2},
    [AXP_ADC_BATTERY_VOLTAGE]   = {0x78, AXP_REG_ADCEN1, 1 << 7, 11, 10},
    [AXP_ADC_CHARGE_CURRENT]    = {0x7a, AXP_REG_ADCEN1, 1 << 6,  1,  2},
    [AXP_ADC_DISCHARGE_CURRENT] = {0x7c, AXP_REG_ADCEN1, 1 << 6,  1,  2},
    [AXP_ADC_APS_VOLTAGE]       = {0x7e, AXP_REG_ADCEN1, 1 << 1,  7,  5},
};

void axp_enable_adc(int adc, bool enable)
{
    const struct axp_adcdata* data = &adcdata[adc];
    axp_modify(data->en_reg, data->en_bit, enable ? data->en_bit : 0);
}

void axp_set_enabled_adcs(unsigned int adc_mask)
{
    uint8_t xfer[3];
    xfer[0] = 0;
    xfer[1] = AXP_REG_ADCEN2;
    xfer[2] = 0;

    for(int i = 0; i < AXP_NUM_ADCS; ++i) {
        if(!(adc_mask & (1 << i)))
            continue;

        const struct axp_adcdata* data = &adcdata[i];
        if(data->en_reg == AXP_REG_ADCEN1)
            xfer[0] |= data->en_bit;
        else
            xfer[2] |= data->en_bit;
    }

    i2c_reg_write(AXP_PMU_BUS, AXP_PMU_ADDR, AXP_REG_ADCEN1, 3, xfer);
}

int axp_read_adc_raw(int adc)
{
    uint8_t data[2];
    int ret = i2c_reg_read(AXP_PMU_BUS, AXP_PMU_ADDR,
                           adcdata[adc].data_reg, 2, data);
    if(ret < 0) {
        logf("axp: ADC read failed, err=%d", ret);
        return INT_MIN;
    }

    if(adc == AXP_ADC_CHARGE_CURRENT || adc == AXP_ADC_DISCHARGE_CURRENT)
        return (data[0] << 5) | data[1];
    else
        return (data[0] << 4) | data[1];
}

int axp_conv_adc(int adc, int value)
{
    const struct axp_adcdata* data = &adcdata[adc];
    if(adc == AXP_ADC_INTERNAL_TEMP)
        return value - 1447;
    else
        return data->num * value / data->den;
}

int axp_read_adc(int adc)
{
    int ret = axp_read_adc_raw(adc);
    if(ret == INT_MIN)
        return ret;

    return axp_conv_adc(adc, ret);
}

/*
 * GPIOs: set function, pull down control, get/set pin level
 */

struct axp_gpiodata {
    uint8_t func_reg;
    uint8_t func_msb: 4;
    uint8_t func_lsb: 4;
    uint8_t level_reg;
    uint8_t level_out: 4;
    uint8_t level_in: 4;
};

static const struct axp_gpiodata gpiodata[] = {
    {AXP_REG_GPIO0FUNC,      2, 0, AXP_REG_GPIOLEVEL1, 0, 4},
    {AXP_REG_GPIO1FUNC,      2, 0, AXP_REG_GPIOLEVEL1, 1, 5},
    {AXP_REG_GPIO2FUNC,      2, 0, AXP_REG_GPIOLEVEL1, 2, 6},
    {AXP_REG_GPIO3GPIO4FUNC, 1, 0, AXP_REG_GPIOLEVEL2, 0, 4},
    {AXP_REG_GPIO3GPIO4FUNC, 3, 2, AXP_REG_GPIOLEVEL2, 1, 5},
    {AXP_REG_NRSTO,          7, 6, AXP_REG_NRSTO,      4, 5},
};

static const uint8_t gpio34funcmap[8] = {
    [AXP_GPIO_SPECIAL]           = 0x0,
    [AXP_GPIO_OPEN_DRAIN_OUTPUT] = 0x1,
    [AXP_GPIO_INPUT]             = 0x2,
    [AXP_GPIO_ADC_IN]            = 0x3,
};

static const uint8_t nrstofuncmap[8] = {
    [AXP_GPIO_SPECIAL]           = 0x0,
    [AXP_GPIO_OPEN_DRAIN_OUTPUT] = 0x2,
    [AXP_GPIO_INPUT]             = 0x3,
};

void axp_set_gpio_function(int gpio, int function)
{
    const struct axp_gpiodata* data = &gpiodata[gpio];
    int mask = (1 << (data->func_msb - data->func_lsb + 1)) - 1;

    if(gpio == 5)
        function = nrstofuncmap[function];
    else if(gpio >= 3)
        function = gpio34funcmap[function];

    axp_modify(data->func_reg, mask << data->func_lsb, function << data->func_lsb);
}

void axp_set_gpio_pulldown(int gpio, bool enable)
{
    int bit = 1 << gpio;
    axp_modify(AXP_REG_GPIOPULL, bit, enable ? bit : 0);
}

int axp_get_gpio(int gpio)
{
    const struct axp_gpiodata* data = &gpiodata[gpio];
    return axp_read(data->level_reg) & (1 << data->level_in);
}

void axp_set_gpio(int gpio, bool enable)
{
    const struct axp_gpiodata* data = &gpiodata[gpio];
    uint8_t bit = 1 << data->level_out;
    axp_modify(data->level_reg, bit, enable ? bit : 0);
}

/*
 * Charging: set charging current, query charging/input status
 */

static const short chargecurrent_tbl[] = {
    100,  190,  280,  360,
    450,  550,  630,  700,
    780,  880,  960,  1000,
    1080, 1160, 1240, 1320,
};

void axp_set_charge_current(int current_mA)
{
    /* find greatest charging current not exceeding requested current */
    unsigned int index = 0;
    while(index < ARRAYLEN(chargecurrent_tbl)-1 &&
          chargecurrent_tbl[index+1] <= current_mA)
        ++index;

    axp_modify(AXP_REG_CHGCTL1, BM_AXP_CHGCTL1_CHARGE_CURRENT,
               index << BP_AXP_CHGCTL1_CHARGE_CURRENT);
}

int axp_get_charge_current(void)
{
    int value = axp_read(AXP_REG_CHGCTL1);
    if(value < 0)
        value = 0;

    value &= BM_AXP_CHGCTL1_CHARGE_CURRENT;
    value >>= BP_AXP_CHGCTL1_CHARGE_CURRENT;
    return chargecurrent_tbl[value];
}

void axp_set_vbus_limit(int mode)
{
    const int mask = BM_AXP_VBUSIPSOUT_VHOLD_LIM |
                     BM_AXP_VBUSIPSOUT_VBUS_LIM |
                     BM_AXP_VBUSIPSOUT_LIM_100mA;

    axp_modify(AXP_REG_VBUSIPSOUT, mask, mode);
}

void axp_set_vhold_level(int vhold_mV)
{
    if(vhold_mV < 4000)
        vhold_mV = 4000;
    else if(vhold_mV > 4700)
        vhold_mV = 4700;

    int level = (vhold_mV - 4000) / 100;
    axp_modify(AXP_REG_VBUSIPSOUT, BM_AXP_VBUSIPSOUT_VHOLD_LEV,
               level << BP_AXP_VBUSIPSOUT_VHOLD_LEV);
}

bool axp_is_charging(void)
{
    int value = axp_read(AXP_REG_CHGSTS);
    return (value >= 0) && (value & BM_AXP_CHGSTS_CHARGING);
}

unsigned int axp_power_input_status(void)
{
    unsigned int state = 0;
    int value = axp_read(AXP_REG_PWRSTS);
    if(value >= 0) {
        /* ACIN is the main charger. Includes USB */
        if(value & BM_AXP_PWRSTS_ACIN_VALID)
            state |= POWER_INPUT_MAIN_CHARGER;

        /* Report USB separately if discernable from ACIN */
        if((value & BM_AXP_PWRSTS_VBUS_VALID) &&
           !(value & BM_AXP_PWRSTS_PCB_SHORTED))
            state |= POWER_INPUT_USB_CHARGER;
    }

#ifdef HAVE_BATTERY_SWITCH
    /* If target allows switching batteries then report if the
     * battery is present or not */
    value = axp_read(AXP_REG_CHGSTS);
    if(value >= 0 && (value & BM_AXP_CHGSTS_BATT_PRESENT))
        state |= POWER_INPUT_BATTERY;
#endif

    return state;
}

/*
 * Misc. functions
 */

void axp_power_off(void)
{
    axp_modify(AXP_REG_PWROFF, BM_AXP_PWROFF_SHUTDOWN, BM_AXP_PWROFF_SHUTDOWN);
}

/*
 * Debug menu
 */

#ifndef BOOTLOADER
#include "action.h"
#include "list.h"
#include "splash.h"
#include <stdio.h>

/* enable extra debug menus which are only useful for development,
 * allow potentially dangerous operations and increase code size
 * significantly */
/*#define AXP_EXTRA_DEBUG*/

enum {
    MODE_ADC,
#ifdef AXP_EXTRA_DEBUG
    MODE_SUPPLY,
    MODE_REGISTER,
#endif
    NUM_MODES,
};

static const char* const axp_modenames[NUM_MODES] = {
    [MODE_ADC]      = "ADCs",
#ifdef AXP_EXTRA_DEBUG
    [MODE_SUPPLY]   = "Power supplies",
    [MODE_REGISTER] = "Register viewer",
#endif
};

struct axp_adcdebuginfo {
    const char* name;
    const char* unit;
};

static const struct axp_adcdebuginfo adc_debuginfo[AXP_NUM_ADCS] = {
    [AXP_ADC_ACIN_VOLTAGE]      = {"V_acin",  "mV"},
    [AXP_ADC_ACIN_CURRENT]      = {"I_acin",  "mA"},
    [AXP_ADC_VBUS_VOLTAGE]      = {"V_vbus",  "mV"},
    [AXP_ADC_VBUS_CURRENT]      = {"I_vbus",  "mA"},
    [AXP_ADC_INTERNAL_TEMP]     = {"T_int",   "C"},
    [AXP_ADC_TS_INPUT]          = {"V_ts",    "mV"},
    [AXP_ADC_GPIO0]             = {"V_gpio0", "mV"},
    [AXP_ADC_GPIO1]             = {"V_gpio1", "mV"},
    [AXP_ADC_GPIO2]             = {"V_gpio2", "mV"},
    [AXP_ADC_GPIO3]             = {"V_gpio3", "mV"},
    [AXP_ADC_BATTERY_VOLTAGE]   = {"V_batt",  "mV"},
    [AXP_ADC_CHARGE_CURRENT]    = {"I_chrg",  "mA"},
    [AXP_ADC_DISCHARGE_CURRENT] = {"I_dchg",  "mA"},
    [AXP_ADC_APS_VOLTAGE]       = {"V_aps",   "mV"},
};

#ifdef AXP_EXTRA_DEBUG
static const char* supply_names[AXP_NUM_SUPPLIES] = {
    [AXP_SUPPLY_EXTEN]  = "EXTEN",
    [AXP_SUPPLY_DCDC1]  = "DCDC1",
    [AXP_SUPPLY_DCDC2]  = "DCDC2",
    [AXP_SUPPLY_DCDC3]  = "DCDC3",
    [AXP_SUPPLY_LDO2]   = "LDO2",
    [AXP_SUPPLY_LDO3]   = "LDO3",
    [AXP_SUPPLY_LDOIO0] = "LDOIO0",
};

struct axp_fieldinfo {
    uint8_t rnum;
    uint8_t msb: 4;
    uint8_t lsb: 4;
};

enum {
#define DEFREG(name, ...) AXP_RNUM_##name,
#include "axp192-defs.h"
    AXP_NUM_REGS,
};

enum {
#define DEFFLD(regname, fldname, ...) AXP_FNUM_##regname##_##fldname,
#include "axp192-defs.h"
    AXP_NUM_FIELDS,
};

static const uint8_t axp_regaddr[AXP_NUM_REGS] = {
#define DEFREG(name, addr) addr,
#include "axp192-defs.h"
};

static const struct axp_fieldinfo axp_fieldinfo[AXP_NUM_FIELDS] = {
#define DEFFLD(regname, fldname, _msb, _lsb, ...) \
    {.rnum = AXP_RNUM_##regname, .msb = _msb, .lsb = _lsb},
#include "axp192-defs.h"
};

static const char* const axp_regnames[AXP_NUM_REGS] = {
#define DEFREG(name, ...) #name,
#include "axp192-defs.h"
};

static const char* const axp_fldnames[AXP_NUM_FIELDS] = {
#define DEFFLD(regname, fldname, ...) #fldname,
#include "axp192-defs.h"
};
#endif /* AXP_EXTRA_DEBUG */

struct axp_debug_menu_state {
    int mode;
#ifdef AXP_EXTRA_DEBUG
    int reg_num;
    int field_num;
    int field_cnt;
    uint8_t cache[AXP_NUM_REGS];
    uint8_t is_cached[AXP_NUM_REGS];
#endif
};

#ifdef AXP_EXTRA_DEBUG
static void axp_debug_clear_cache(struct axp_debug_menu_state* state)
{
    memset(state->is_cached, 0, sizeof(state->is_cached));
}

static int axp_debug_get_rnum(uint8_t addr)
{
    for(int i = 0; i < AXP_NUM_REGS; ++i)
        if(axp_regaddr[i] == addr)
            return i;

    return -1;
}

static uint8_t axp_debug_read(struct axp_debug_menu_state* state, int rnum)
{
    if(state->is_cached[rnum])
        return state->cache[rnum];

    int value = axp_read(axp_regaddr[rnum]);
    if(value < 0)
        return 0;

    state->is_cached[rnum] = 1;
    state->cache[rnum] = value;
    return value;
}

static void axp_debug_get_sel(const struct axp_debug_menu_state* state,
                              int item, int* rnum, int* fnum)
{
    if(state->reg_num >= 0 && state->field_num >= 0) {
        int i = item - state->reg_num;
        if(i <= 0) {
            /* preceding register is selected */
        } else if(i <= state->field_cnt) {
            /* field is selected */
            *rnum = state->reg_num;
            *fnum = i + state->field_num - 1;
            return;
        } else {
            /* subsequent regiser is selected */
            item -= state->field_cnt;
        }
    }

    /* register is selected */
    *rnum = item;
    *fnum = -1;
}

static int axp_debug_set_sel(struct axp_debug_menu_state* state, int rnum)
{
    state->reg_num = rnum;
    state->field_num = -1;
    state->field_cnt = 0;

    for(int i = 0; i < AXP_NUM_FIELDS; ++i) {
        if(axp_fieldinfo[i].rnum != rnum)
            continue;

        state->field_num = i;
        do {
            state->field_cnt++;
            i++;
        } while(axp_fieldinfo[i].rnum == rnum);
        break;
    }

    return rnum;
}
#endif /* AXP_EXTRA_DEBUG */

static const char* axp_debug_menu_get_name(int item, void* data,
                                           char* buf, size_t buflen)
{
    struct axp_debug_menu_state* state = data;
    int value;

    /* for safety */
    buf[0] = '\0';

    if(state->mode == MODE_ADC && item < AXP_NUM_ADCS)
    {
        const struct axp_adcdebuginfo* info = &adc_debuginfo[item];
        value = axp_read_adc(item);
        if(item == AXP_ADC_INTERNAL_TEMP) {
            snprintf(buf, buflen, "%s: %d.%d %s",
                     info->name, value/10, value%10, info->unit);
        } else {
            snprintf(buf, buflen, "%s: %d %s", info->name, value, info->unit);
        }
    }
#ifdef AXP_EXTRA_DEBUG
    else if(state->mode == MODE_SUPPLY && item < AXP_NUM_SUPPLIES)
    {
        const struct axp_supplydata* data = &supplydata[item];
        int en_rnum = axp_debug_get_rnum(data->en_reg);
        int volt_rnum = axp_debug_get_rnum(data->volt_reg);
        bool enabled = false;
        int voltage = -1;

        if(en_rnum >= 0) {
            value = axp_debug_read(state, en_rnum);
            if(value & data->en_bit)
                enabled = true;
            else
                enabled = false;
        } else if(item == AXP_SUPPLY_LDOIO0) {
            value = axp_debug_read(state, AXP_RNUM_GPIO0FUNC);
            if((value & 0x7) == AXP_GPIO_SPECIAL)
                enabled = true;
            else
                enabled = false;
        }

        if(volt_rnum >= 0) {
            voltage = axp_debug_read(state, volt_rnum);
            voltage >>= data->volt_lsb;
            voltage &= (1 << (data->volt_msb - data->volt_lsb + 1)) - 1;

            /* convert to mV */
            voltage = data->min_mV + voltage * data->step_mV;
        }

        if(enabled && voltage >= 0) {
            snprintf(buf, buflen, "%s: %d mV",
                     supply_names[item], voltage);
        } else {
            snprintf(buf, buflen, "%s: %sabled",
                     supply_names[item], enabled ? "en" : "dis");
        }
    }
    else if(state->mode == MODE_REGISTER)
    {
        int rnum, fnum;
        axp_debug_get_sel(state, item, &rnum, &fnum);

        if(fnum >= 0) {
            const struct axp_fieldinfo* info = &axp_fieldinfo[fnum];
            value = axp_debug_read(state, info->rnum);
            value >>= info->lsb;
            value &= (1 << (info->msb - info->lsb + 1)) - 1;
            snprintf(buf, buflen, "\t%s: %d (0x%x)",
                     axp_fldnames[fnum], value, value);
        } else if(rnum < AXP_NUM_REGS) {
            value = axp_debug_read(state, rnum);
            snprintf(buf, buflen, "%s: 0x%02x", axp_regnames[rnum], value);
        }
    }
#endif /* AXP_EXTRA_DEBUG */

    return buf;
}

static int axp_debug_menu_cb(int action, struct gui_synclist* lists)
{
    struct axp_debug_menu_state* state = lists->data;

    if(state->mode == MODE_ADC)
    {
        /* update continuously */
        if(action == ACTION_NONE)
            action = ACTION_REDRAW;
    }
#ifdef AXP_EXTRA_DEBUG
    else if(state->mode == MODE_REGISTER)
    {
        if(action == ACTION_STD_OK) {
            /* expand a register to show its fields */
            int rnum, fnum;
            int sel_pos = gui_synclist_get_sel_pos(lists);
            axp_debug_get_sel(state, sel_pos, &rnum, &fnum);
            if(fnum < 0 && rnum < AXP_NUM_REGS) {
                int delta_items = -state->field_cnt;
                if(rnum != state->reg_num) {
                    if(rnum > state->reg_num)
                        sel_pos += delta_items;

                    axp_debug_set_sel(state, rnum);
                    delta_items += state->field_cnt;
                } else {
                    state->reg_num = -1;
                    state->field_num = -1;
                    state->field_cnt = 0;
                }

                gui_synclist_set_nb_items(lists, lists->nb_items + delta_items);
                gui_synclist_select_item(lists, sel_pos);
                action = ACTION_REDRAW;
            }
        }
    }
    else if(state->mode == MODE_SUPPLY)
    {
        /* disable a supply... use with caution */
        if(action == ACTION_STD_CONTEXT) {
            int sel_pos = gui_synclist_get_sel_pos(lists);
            axp_enable_supply(sel_pos, false);
        }
    }
#endif

#ifdef AXP_EXTRA_DEBUG
    /* clear register cache to refresh values */
    if(state->mode != MODE_ADC && action == ACTION_STD_CONTEXT) {
        splashf(HZ/2, "Refreshed");
        axp_debug_clear_cache(state);
        action = ACTION_REDRAW;
    }
#endif

    /* mode switching */
    if(action == ACTION_STD_MENU) {
        state->mode = (state->mode + 1) % NUM_MODES;
        gui_synclist_set_title(lists, (char*)axp_modenames[state->mode], Icon_NOICON);
        action = ACTION_REDRAW;

        switch(state->mode) {
        case MODE_ADC:
            gui_synclist_set_nb_items(lists, AXP_NUM_ADCS);
            gui_synclist_select_item(lists, 0);
            break;

#ifdef AXP_EXTRA_DEBUG
        case MODE_SUPPLY:
            axp_debug_clear_cache(state);
            gui_synclist_set_nb_items(lists, AXP_NUM_SUPPLIES);
            gui_synclist_select_item(lists, 0);
            break;

        case MODE_REGISTER:
            state->reg_num = -1;
            state->field_num = -1;
            state->field_cnt = 0;
            axp_debug_clear_cache(state);
            gui_synclist_set_nb_items(lists, AXP_NUM_REGS);
            gui_synclist_select_item(lists, 0);
            break;
#endif
        }
    }

    return action;
}

bool axp_debug_menu(void)
{
    struct axp_debug_menu_state state;
    state.mode = MODE_ADC;
#ifdef AXP_EXTRA_DEBUG
    state.reg_num = -1;
    state.field_num = -1;
    state.field_cnt = 0;
    axp_debug_clear_cache(&state);
#endif

    struct simplelist_info info;
    simplelist_info_init(&info, (char*)axp_modenames[state.mode],
                         AXP_NUM_ADCS, &state);
    info.get_name = axp_debug_menu_get_name;
    info.action_callback = axp_debug_menu_cb;
    return simplelist_show_list(&info);
}
#endif
