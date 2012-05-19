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
#include "system.h"
#include "system-target.h"
#include "lradc-imx233.h"
#include "kernel-imx233.h"
#include "stdlib.h"

/* channels */
static struct channel_arbiter_t channel_arbiter;
/* delay channels */
static struct channel_arbiter_t delay_arbiter;
/* battery is very special, dedicate a channel and a delay to it */
static int battery_chan;
static int battery_delay_chan;
/* irq callbacks */
static lradc_irq_fn_t irq_cb[HW_LRADC_NUM_CHANNELS];

#define define_cb(x) \
    void INT_LRADC_CH##x(void) \
    { \
        INT_LRADC_CH(x); \
    }

void INT_LRADC_CH(int chan)
{
    if(irq_cb[chan])
        irq_cb[chan](chan);
}

define_cb(0)
define_cb(1)
define_cb(2)
define_cb(3)
define_cb(4)
define_cb(5)
define_cb(6)
define_cb(7)

void imx233_lradc_set_channel_irq_callback(int channel, lradc_irq_fn_t cb)
{
    irq_cb[channel] = cb;
}

void imx233_lradc_setup_channel(int channel, bool div2, bool acc, int nr_samples, int src)
{
    __REG_CLR(HW_LRADC_CHx(channel)) = HW_LRADC_CHx__NUM_SAMPLES_BM | HW_LRADC_CHx__ACCUMULATE;
    __REG_SET(HW_LRADC_CHx(channel)) = nr_samples << HW_LRADC_CHx__NUM_SAMPLES_BP |
        acc << HW_LRADC_CHx__ACCUMULATE;
    if(div2)
        __REG_SET(HW_LRADC_CTRL2) = HW_LRADC_CTRL2__DIVIDE_BY_TWO(channel);
    else
        __REG_CLR(HW_LRADC_CTRL2) = HW_LRADC_CTRL2__DIVIDE_BY_TWO(channel);
    __REG_CLR(HW_LRADC_CTRL4) = HW_LRADC_CTRL4__LRADCxSELECT_BM(channel);
    __REG_SET(HW_LRADC_CTRL4) = src << HW_LRADC_CTRL4__LRADCxSELECT_BP(channel);
}

void imx233_lradc_setup_delay(int dchan, int trigger_lradc, int trigger_delays,
    int loop_count, int delay)
{
    HW_LRADC_DELAYx(dchan) =
        trigger_lradc << HW_LRADC_DELAYx__TRIGGER_LRADCS_BP |
        trigger_delays << HW_LRADC_DELAYx__TRIGGER_DELAYS_BP |
        loop_count << HW_LRADC_DELAYx__LOOP_COUNT_BP |
        delay << HW_LRADC_DELAYx__DELAY_BP;
}

void imx233_lradc_clear_channel_irq(int channel)
{
    __REG_CLR(HW_LRADC_CTRL1) = HW_LRADC_CTRL1__LRADCx_IRQ(channel);
}

bool imx233_lradc_read_channel_irq(int channel)
{
    return HW_LRADC_CTRL1 & HW_LRADC_CTRL1__LRADCx_IRQ(channel);
}

void imx233_lradc_enable_channel_irq(int channel, bool enable)
{
    if(enable)
        __REG_SET(HW_LRADC_CTRL1) = HW_LRADC_CTRL1__LRADCx_IRQ_EN(channel);
    else
        __REG_CLR(HW_LRADC_CTRL1) = HW_LRADC_CTRL1__LRADCx_IRQ_EN(channel);
    imx233_lradc_clear_channel_irq(channel);
}

void imx233_lradc_kick_channel(int channel)
{
    imx233_lradc_clear_channel_irq(channel);
    __REG_SET(HW_LRADC_CTRL0) = HW_LRADC_CTRL0__SCHEDULE(channel);
}

void imx233_lradc_kick_delay(int dchan)
{
    __REG_SET(HW_LRADC_DELAYx(dchan)) = HW_LRADC_DELAYx__KICK;
}

void imx233_lradc_wait_channel(int channel)
{
    /* wait for completion */
    while(!imx233_lradc_read_channel_irq(channel))
        yield();
}

int imx233_lradc_read_channel(int channel)
{
    return __XTRACT_EX(HW_LRADC_CHx(channel), HW_LRADC_CHx__VALUE);
}

void imx233_lradc_clear_channel(int channel)
{
    __REG_CLR(HW_LRADC_CHx(channel)) = HW_LRADC_CHx__VALUE_BM;
}

int imx233_lradc_acquire_channel(int timeout)
{
    return arbiter_acquire(&channel_arbiter, timeout);
}

void imx233_lradc_release_channel(int chan)
{
    return arbiter_release(&channel_arbiter, chan);
}

void imx233_lradc_reserve_channel(int channel)
{
    return arbiter_reserve(&channel_arbiter, channel);
}

int imx233_lradc_acquire_delay(int timeout)
{
    return arbiter_acquire(&delay_arbiter, timeout);
}

void imx233_lradc_release_delay(int chan)
{
    return arbiter_release(&delay_arbiter, chan);
}

void imx233_lradc_reserve_delay(int channel)
{
    return arbiter_reserve(&delay_arbiter, channel);
}

int imx233_lradc_sense_die_temperature(int nmos_chan, int pmos_chan)
{
    imx233_lradc_setup_channel(nmos_chan, false, false, 0, HW_LRADC_CHANNEL_NMOS_THIN);
    imx233_lradc_setup_channel(pmos_chan, false, false, 0, HW_LRADC_CHANNEL_PMOS_THIN);
    // mux sensors
    __REG_CLR(HW_LRADC_CTRL2) = HW_LRADC_CTRL2__TEMPSENSE_PWD;
    imx233_lradc_clear_channel(nmos_chan);
    imx233_lradc_clear_channel(pmos_chan);
    // schedule both channels
    imx233_lradc_kick_channel(nmos_chan);
    imx233_lradc_kick_channel(pmos_chan);
    // wait completion
    imx233_lradc_wait_channel(nmos_chan);
    imx233_lradc_wait_channel(pmos_chan);
    // mux sensors
    __REG_SET(HW_LRADC_CTRL2) = HW_LRADC_CTRL2__TEMPSENSE_PWD;
    // do the computation
    int diff = imx233_lradc_read_channel(nmos_chan) - imx233_lradc_read_channel(pmos_chan);
    // return diff * 1.012 / 4
    return (diff * 1012) / 4000;
}

/* set to 0 to disable current source */
static void imx233_lradc_set_temp_isrc(int sensor, int value)
{
    if(sensor < 0 || sensor > 1)
        panicf("imx233_lradc_set_temp_isrc: invalid sensor");
    unsigned mask = HW_LRADC_CTRL2__TEMP_ISRCx_BM(sensor);
    unsigned bp = HW_LRADC_CTRL2__TEMP_ISRCx_BP(sensor);
    unsigned en = HW_LRADC_CTRL2__TEMP_SENSOR_IENABLEx(sensor);

    __REG_CLR(HW_LRADC_CTRL2) = mask;
    __REG_SET(HW_LRADC_CTRL2) = value << bp;
    if(value != 0)
    {
        __REG_SET(HW_LRADC_CTRL2) = en;
        udelay(100);
    }
    else
        __REG_CLR(HW_LRADC_CTRL2) = en;
}

int imx233_lradc_sense_ext_temperature(int chan, int sensor)
{
#define EXT_TEMP_ACC_COUNT  5
    /* setup channel */
    imx233_lradc_setup_channel(chan, false, false, 0, sensor);
    /* set current source to 300µA */
    imx233_lradc_set_temp_isrc(sensor, HW_LRADC_CTRL2__TEMP_ISRC__300uA);
    /* read value and accumulate */
    int a = 0;
    for(int i = 0; i < EXT_TEMP_ACC_COUNT; i++)
    {
        imx233_lradc_clear_channel(chan);
        imx233_lradc_kick_channel(chan);
        imx233_lradc_wait_channel(chan);
        a += imx233_lradc_read_channel(chan);
    }
    /* setup channel for small accumulation */
    /* set current source to 20µA */
    imx233_lradc_set_temp_isrc(sensor, HW_LRADC_CTRL2__TEMP_ISRC__20uA);
    /* read value */
    int b = 0;
    for(int i = 0; i < EXT_TEMP_ACC_COUNT; i++)
    {
        imx233_lradc_clear_channel(chan);
        imx233_lradc_kick_channel(chan);
        imx233_lradc_wait_channel(chan);
        b += imx233_lradc_read_channel(chan);
    }
    /* disable sensor current */
    imx233_lradc_set_temp_isrc(sensor, HW_LRADC_CTRL2__TEMP_ISRC__0uA);
    
    return (abs(b - a) / EXT_TEMP_ACC_COUNT) * 1104 / 1000;
}

void imx233_lradc_setup_battery_conversion(bool automatic, unsigned long scale_factor)
{
    __REG_CLR(HW_LRADC_CONVERSION) = HW_LRADC_CONVERSION__SCALE_FACTOR_BM;
    __REG_SET(HW_LRADC_CONVERSION) = scale_factor;
    if(automatic)
        __REG_SET(HW_LRADC_CONVERSION) = HW_LRADC_CONVERSION__AUTOMATIC;
    else
        __REG_CLR(HW_LRADC_CONVERSION) = HW_LRADC_CONVERSION__AUTOMATIC;
}

int imx233_lradc_read_battery_voltage(void)
{
    return __XTRACT(HW_LRADC_CONVERSION, SCALED_BATT_VOLTAGE);
}

void imx233_lradc_setup_touch(bool xminus_enable, bool yminus_enable,
    bool xplus_enable, bool yplus_enable, bool touch_detect)
{
    __FIELD_SET_CLR(HW_LRADC_CTRL0, XMINUS_ENABLE, xminus_enable);
    __FIELD_SET_CLR(HW_LRADC_CTRL0, YMINUS_ENABLE, yminus_enable);
    __FIELD_SET_CLR(HW_LRADC_CTRL0, XPLUS_ENABLE, xplus_enable);
    __FIELD_SET_CLR(HW_LRADC_CTRL0, YPLUS_ENABLE, yplus_enable);
    __FIELD_SET_CLR(HW_LRADC_CTRL0, TOUCH_DETECT_ENABLE, touch_detect);
}

void imx233_lradc_enable_touch_detect_irq(bool enable)
{
    __FIELD_SET_CLR(HW_LRADC_CTRL1, TOUCH_DETECT_IRQ_EN, enable);
    imx233_lradc_clear_touch_detect_irq();
}

void imx233_lradc_clear_touch_detect_irq(void)
{
    __REG_CLR(HW_LRADC_CTRL1) = HW_LRADC_CTRL1__TOUCH_DETECT_IRQ;
}

bool imx233_lradc_read_touch_detect(void)
{
    return HW_LRADC_STATUS & HW_LRADC_STATUS__TOUCH_DETECT_RAW;
}

void imx233_lradc_init(void)
{
    arbiter_init(&channel_arbiter, HW_LRADC_NUM_CHANNELS);
    arbiter_init(&delay_arbiter, HW_LRADC_NUM_DELAYS);
    // enable block
    imx233_reset_block(&HW_LRADC_CTRL0);
    // disable ground ref
    __REG_CLR(HW_LRADC_CTRL0) = HW_LRADC_CTRL0__ONCHIP_GROUNDREF;
    // disable temperature sensors
    __REG_CLR(HW_LRADC_CTRL2) = HW_LRADC_CTRL2__TEMP_SENSOR_IENABLE0 |
        HW_LRADC_CTRL2__TEMP_SENSOR_IENABLE1;
    __REG_SET(HW_LRADC_CTRL2) = HW_LRADC_CTRL2__TEMPSENSE_PWD;
    // set frequency
    __REG_CLR(HW_LRADC_CTRL3) = HW_LRADC_CTRL3__CYCLE_TIME_BM;
    __REG_SET(HW_LRADC_CTRL3) = HW_LRADC_CTRL3__CYCLE_TIME__6MHz;
    // setup battery
    battery_chan = 7;
    imx233_lradc_reserve_channel(battery_chan);
    /* setup them for the simplest use: no accumulation, no division*/
    imx233_lradc_setup_channel(battery_chan, false, false, 0, HW_LRADC_CHANNEL_BATTERY);
    /* setup delay channel for battery for automatic reading and scaling */
    battery_delay_chan = 0;
    imx233_lradc_reserve_delay(battery_delay_chan);
    /* setup delay to trigger battery channel and retrigger itself.
     * The counter runs at 2KHz so a delay of 200 will trigger 10
     * conversions per seconds */
    imx233_lradc_setup_delay(battery_delay_chan, 1 << battery_chan,
        1 << battery_delay_chan, 0, 200);
    imx233_lradc_kick_delay(battery_delay_chan);
    /* enable automatic conversion, use Li-Ion type battery */
    imx233_lradc_setup_battery_conversion(true, HW_LRADC_CONVERSION__SCALE_FACTOR__LI_ION);
}
