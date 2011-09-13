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

static struct semaphore free_bm_sema;
static struct mutex free_bm_mutex;
static unsigned free_bm;

void imx233_lradc_setup_channel(int channel, bool div2, bool acc, int nr_samples, int src)
{
    __REG_CLR(HW_LRADC_CHx(channel)) =HW_LRADC_CHx__NUM_SAMPLES_BM | HW_LRADC_CHx__ACCUMULATE;
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

void imx233_lradc_kick_channel(int channel)
{
    __REG_CLR(HW_LRADC_CTRL1) = HW_LRADC_CTRL1__LRADCx_IRQ(channel);
    __REG_SET(HW_LRADC_CTRL0) = HW_LRADC_CTRL0__SCHEDULE(channel);
}

void imx233_lradc_kick_delay(int dchan)
{
    __REG_SET(HW_LRADC_DELAYx(dchan)) = HW_LRADC_DELAYx__KICK;
}

void imx233_lradc_wait_channel(int channel)
{
    /* wait for completion */
    while(!(HW_LRADC_CTRL1 & HW_LRADC_CTRL1__LRADCx_IRQ(channel)))
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
    int w = semaphore_wait(&free_bm_sema, timeout);
    if(w == OBJ_WAIT_TIMEDOUT)
        return w;
    mutex_lock(&free_bm_mutex);
    int chan = find_first_set_bit(free_bm);
    if(chan >= HW_LRADC_NUM_CHANNELS)
        panicf("imx233_lradc_acquire_channel cannot find a free channel !");
    free_bm &= ~(1 << chan);
    mutex_unlock(&free_bm_mutex);
    return chan;
}

void imx233_lradc_release_channel(int chan)
{
    mutex_lock(&free_bm_mutex);
    free_bm |= 1 << chan;
    mutex_unlock(&free_bm_mutex);
    semaphore_release(&free_bm_sema);
}

void imx233_lradc_reserve_channel(int channel)
{
    semaphore_wait(&free_bm_sema, TIMEOUT_NOBLOCK);
    mutex_lock(&free_bm_mutex);
    free_bm &= ~(1 << channel);
    mutex_unlock(&free_bm_mutex);
}

int imx233_lradc_sense_die_temperature(int nmos_chan, int pmos_chan)
{
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

void imx233_lradc_init(void)
{
    mutex_init(&free_bm_mutex);
    semaphore_init(&free_bm_sema, HW_LRADC_NUM_CHANNELS, HW_LRADC_NUM_CHANNELS);
    free_bm = (1 << HW_LRADC_NUM_CHANNELS) - 1;
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
}
