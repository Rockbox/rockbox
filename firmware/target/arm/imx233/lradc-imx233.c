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

#include "regs/lradc.h"

/** additional defines */
#define BP_LRADC_CTRL4_LRADCxSELECT(x)  (4 * (x))
#define BM_LRADC_CTRL4_LRADCxSELECT(x)  (0xf << (4 * (x)))
#define BF_LRADC_CTRL4_LRADCxSELECT(x, v)   (((v) << BP_LRADC_CTRL4_LRADCxSELECT(x)) & BM_LRADC_CTRL4_LRADCxSELECT(x))
#define BFM_LRADC_CTRL4_LRADCxSELECT(x, v)  BM_LRADC_CTRL4_LRADCxSELECT(x)

#define BP_LRADC_CTRL1_LRADCx_IRQ(x)    (x)
#define BM_LRADC_CTRL1_LRADCx_IRQ(x)    (1 << (x))

#define BP_LRADC_CTRL1_LRADCx_IRQ_EN(x)    (16 + (x))
#define BM_LRADC_CTRL1_LRADCx_IRQ_EN(x)    (1 << (16 + (x)))

/* channels */
#if IMX233_SUBTARGET >= 3700
static struct channel_arbiter_t channel_arbiter;
#else
static struct semaphore channel_sema[LRADC_NUM_CHANNELS];
#endif
/* delay channels */
static struct channel_arbiter_t delay_arbiter;
/* battery is very special, dedicate a channel and a delay to it */
static int battery_chan;
static int battery_delay_chan;
/* irq callbacks */
static lradc_irq_fn_t irq_cb[LRADC_NUM_CHANNELS];

#define define_cb(x) \
    void INT_LRADC_CH##x(void) \
    { \
        INT_LRADC_CH(x); \
    }

void INT_LRADC_CH(int chan)
{
    if(irq_cb[chan])
        irq_cb[chan](chan);
    imx233_lradc_clear_channel_irq(chan);
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
    imx233_icoll_enable_interrupt(INT_SRC_LRADC_CHx(channel), cb != NULL);
}

void imx233_lradc_setup_source(int channel, bool div2, int src)
{
    if(div2)
        BF_WR(LRADC_CTRL2_SET, DIVIDE_BY_TWO(1 << channel));
    else
        BF_WR(LRADC_CTRL2_CLR, DIVIDE_BY_TWO(1 << channel));
#if IMX233_SUBTARGET >= 3700
    BF_CS(LRADC_CTRL4, LRADCxSELECT(channel, src));
#else
    if(channel == 6)
        BF_CS(LRADC_CTRL2, LRADC6SELECT(src));
    else if(channel == 7)
        BF_CS(LRADC_CTRL2, LRADC7SELECT(src));
    else if(channel != src)
        panicf("cannot configure channel %d for source %d", channel, src);
#endif
}

void imx233_lradc_setup_sampling(int channel, bool acc, int nr_samples)
{
    BF_CS(LRADC_CHn(channel), NUM_SAMPLES(nr_samples), ACCUMULATE(acc));
}

void imx233_lradc_setup_delay(int dchan, int trigger_lradc, int trigger_delays,
    int loop_count, int delay)
{
    BF_WR_ALL(LRADC_DELAYn(dchan), TRIGGER_LRADCS(trigger_lradc),
        TRIGGER_DELAYS(trigger_delays), LOOP_COUNT(loop_count), DELAY(delay));
}

void imx233_lradc_clear_channel_irq(int channel)
{
    BF_CLR(LRADC_CTRL1, LRADCx_IRQ(channel));
}

bool imx233_lradc_read_channel_irq(int channel)
{
    return BF_RD(LRADC_CTRL1, LRADCx_IRQ(channel));
}

void imx233_lradc_enable_channel_irq(int channel, bool enable)
{
    if(enable)
        BF_SET(LRADC_CTRL1, LRADCx_IRQ_EN(channel));
    else
        BF_CLR(LRADC_CTRL1, LRADCx_IRQ_EN(channel));
    imx233_lradc_clear_channel_irq(channel);
}

void imx233_lradc_kick_channel(int channel)
{
    imx233_lradc_clear_channel_irq(channel);
    BF_WR(LRADC_CTRL0_SET, SCHEDULE(1 << channel));
}

void imx233_lradc_kick_delay(int dchan)
{
    BF_SET(LRADC_DELAYn(dchan), KICK);
}

void imx233_lradc_wait_channel(int channel)
{
    /* wait for completion */
    while(!imx233_lradc_read_channel_irq(channel))
        yield();
}

int imx233_lradc_read_channel(int channel)
{
    return BF_RD(LRADC_CHn(channel), VALUE);
}

void imx233_lradc_clear_channel(int channel)
{
    BF_CLR(LRADC_CHn(channel), VALUE);
}

#if IMX233_SUBTARGET >= 3700
int imx233_lradc_acquire_channel(int src, int timeout)
{
    (void) src;
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
#else
int imx233_lradc_acquire_channel(int src, int timeout)
{
    int channel = src <= LRADC_SRC_BATTERY ? src : 6;
    if(semaphore_wait(&channel_sema[channel], timeout) == OBJ_WAIT_TIMEDOUT)
        return -1;
    return channel;
}

void imx233_lradc_release_channel(int chan)
{
    semaphore_release(&channel_sema[chan]);
}

void imx233_lradc_reserve_channel(int channel)
{
    if(imx233_lradc_acquire_channel(channel, 0) == -1)
        panicf("Cannot reserve a used channel");
}
#endif

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

#if IMX233_SUBTARGET >= 3700
int imx233_lradc_sense_die_temperature(int nmos_chan, int pmos_chan)
{
    imx233_lradc_setup_source(nmos_chan, false, LRADC_SRC_NMOS_THIN);
    imx233_lradc_setup_sampling(nmos_chan, false, 0);
    imx233_lradc_setup_source(pmos_chan, false, LRADC_SRC_PMOS_THIN);
    imx233_lradc_setup_sampling(pmos_chan, false, 0);
    // mux sensors
    BF_CLR(LRADC_CTRL2, TEMPSENSE_PWD);
    imx233_lradc_clear_channel(nmos_chan);
    imx233_lradc_clear_channel(pmos_chan);
    // schedule both channels
    imx233_lradc_kick_channel(nmos_chan);
    imx233_lradc_kick_channel(pmos_chan);
    // wait completion
    imx233_lradc_wait_channel(nmos_chan);
    imx233_lradc_wait_channel(pmos_chan);
    // mux sensors
    BF_SET(LRADC_CTRL2, TEMPSENSE_PWD);
    // do the computation
    int diff = imx233_lradc_read_channel(nmos_chan) - imx233_lradc_read_channel(pmos_chan);
    // return diff * 1.012 / 4
    return (diff * 1012) / 4000;
}
#endif

/* set to 0 to disable current source */
static void imx233_lradc_set_temp_isrc(int sensor, int value)
{
    if(sensor < 0 || sensor > 1)
        panicf("imx233_lradc_set_temp_isrc: invalid sensor");
    unsigned mask = sensor ? BM_LRADC_CTRL2_TEMP_ISRC0 : BM_LRADC_CTRL2_TEMP_ISRC1;
    unsigned bp = sensor ? BP_LRADC_CTRL2_TEMP_ISRC0 : BP_LRADC_CTRL2_TEMP_ISRC1;
    unsigned en = sensor ? BM_LRADC_CTRL2_TEMP_SENSOR_IENABLE0 : BM_LRADC_CTRL2_TEMP_SENSOR_IENABLE1;

    HW_LRADC_CTRL2_CLR = mask;
    HW_LRADC_CTRL2_SET = value << bp;
    if(value != 0)
    {
        HW_LRADC_CTRL2_SET = en;
        udelay(100);
    }
    else
        HW_LRADC_CTRL2_CLR = en;
}

int imx233_lradc_sense_ext_temperature(int chan, int sensor)
{
#define EXT_TEMP_ACC_COUNT  5
    /* setup channel */
    imx233_lradc_setup_source(chan, false, sensor);
    imx233_lradc_setup_sampling(chan, false, 0);
    /* set current source to 300µA */
    imx233_lradc_set_temp_isrc(sensor, BV_LRADC_CTRL2_TEMP_ISRC0__300);
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
    imx233_lradc_set_temp_isrc(sensor, BV_LRADC_CTRL2_TEMP_ISRC0__20);
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
    imx233_lradc_set_temp_isrc(sensor, BV_LRADC_CTRL2_TEMP_ISRC0__ZERO);

    return (abs(b - a) / EXT_TEMP_ACC_COUNT) * 1104 / 1000;
}

void imx233_lradc_setup_battery_conversion(bool automatic, unsigned long scale_factor)
{
    BF_CS(LRADC_CONVERSION, SCALE_FACTOR(scale_factor));
    if(automatic)
        BF_SET(LRADC_CONVERSION, AUTOMATIC);
    else
        BF_CLR(LRADC_CONVERSION, AUTOMATIC);
}

int imx233_lradc_read_battery_voltage(void)
{
    return BF_RD(LRADC_CONVERSION, SCALED_BATT_VOLTAGE);
}

void imx233_lradc_setup_touch(bool xminus_enable, bool yminus_enable,
    bool xplus_enable, bool yplus_enable, bool touch_detect)
{
    BF_CS(LRADC_CTRL0, XMINUS_ENABLE(xminus_enable),
        YMINUS_ENABLE(yminus_enable), XPLUS_ENABLE(xplus_enable),
        YPLUS_ENABLE(yplus_enable), TOUCH_DETECT_ENABLE(touch_detect));
}

void imx233_lradc_enable_touch_detect_irq(bool enable)
{
    if(enable)
        BF_SET(LRADC_CTRL1, TOUCH_DETECT_IRQ_EN);
    else
        BF_CLR(LRADC_CTRL1, TOUCH_DETECT_IRQ_EN);
    imx233_lradc_clear_touch_detect_irq();
}

void imx233_lradc_clear_touch_detect_irq(void)
{
    BF_CLR(LRADC_CTRL1, TOUCH_DETECT_IRQ);
}

bool imx233_lradc_read_touch_detect(void)
{
    return BF_RD(LRADC_STATUS, TOUCH_DETECT_RAW);
}

void imx233_lradc_init(void)
{
    /* On STMP3700+, any channel can measure any source but on STMP3600 only
     * channels 6 and 7 can measure all sources. Channel 7 being dedicated to
     * battery, only channel 6 is available for free use */
#if IMX233_SUBTARGET >= 3700
    arbiter_init(&channel_arbiter, LRADC_NUM_CHANNELS);
#else
    for(int i = 0; i < LRADC_NUM_CHANNELS; i++)
        semaphore_init(&channel_sema[i], 1, 1);
#endif
    arbiter_init(&delay_arbiter, LRADC_NUM_DELAYS);
    // enable block
    imx233_reset_block(&HW_LRADC_CTRL0);
    // disable ground ref
    BF_CLR(LRADC_CTRL0, ONCHIP_GROUNDREF);
    // disable temperature sensors
    BF_CLR(LRADC_CTRL2, TEMP_SENSOR_IENABLE0);
    BF_CLR(LRADC_CTRL2, TEMP_SENSOR_IENABLE1);
#if IMX233_SUBTARGET >= 3700
    BF_SET(LRADC_CTRL2, TEMPSENSE_PWD);
#endif
    // set frequency
    BF_CS(LRADC_CTRL3, CYCLE_TIME_V(6MHZ));
    // setup battery
    battery_chan = 7;
    imx233_lradc_reserve_channel(battery_chan);
    /* setup them for the simplest use: no accumulation, no division*/
    imx233_lradc_setup_source(battery_chan, false, LRADC_SRC_BATTERY);
    imx233_lradc_setup_sampling(battery_chan, false, 0);
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
    imx233_lradc_setup_battery_conversion(true, BV_LRADC_CONVERSION_SCALE_FACTOR__LI_ION);
}
