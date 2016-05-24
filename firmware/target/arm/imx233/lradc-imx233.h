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
#ifndef __lradc_imx233__
#define __lradc_imx233__

#include "config.h"
#include "cpu.h"

#include "cpu.h"
#include "system.h"
#include "system-target.h"

#define LRADC_NUM_CHANNELS  8
#define LRADC_NUM_DELAYS    4
#define LRADC_NUM_SOURCES   16

#define LRADC_SRC(x)            (x)
#define LRADC_SRC_XPLUS         LRADC_SRC(2)
#define LRADC_SRC_YPLUS         LRADC_SRC(3)
#define LRADC_SRC_XMINUS        LRADC_SRC(4)
#define LRADC_SRC_YMINUS        LRADC_SRC(5)
#define LRADC_SRC_VDDIO         LRADC_SRC(6)
#define LRADC_SRC_BATTERY       LRADC_SRC(7)
#define LRADC_SRC_PMOS_THIN     LRADC_SRC(8)
#define LRADC_SRC_NMOS_THIN     LRADC_SRC(9)
#define LRADC_SRC_NMOS_THICK    LRADC_SRC(10)
#define LRADC_SRC_PMOS_THICK    LRADC_SRC(11)
#define LRADC_SRC_PMOS_THICK    LRADC_SRC(11)
#if IMX233_SUBTARGET >= 3700
#define LRADC_SRC_USB_DP        LRADC_SRC(12)
#define LRADC_SRC_USB_DN        LRADC_SRC(13)
#define LRADC_SRC_VBG           LRADC_SRC(14)
#define LRADC_SRC_5V            LRADC_SRC(15)
#endif

/* frequency of the delay counter */
#define LRADC_DELAY_FREQ    2000

/* maximum value of a sample (without accumulation), defines the precision */
#define LRADC_MAX_VALUE     4096

typedef void (*lradc_irq_fn_t)(int chan);

void imx233_lradc_init(void);
void imx233_lradc_setup_source(int channel, bool div2, int src);
/* variant of the above only for accumulation changes */
void imx233_lradc_setup_sampling(int channel, bool acc, int nr_samples);
void imx233_lradc_setup_delay(int dchan, int trigger_lradc, int trigger_delays,
    int loop_count, int delay);
void imx233_lradc_clear_channel_irq(int channel);
bool imx233_lradc_read_channel_irq(int channel);
void imx233_lradc_enable_channel_irq(int channel, bool enable);
/* a non-null cb will enable the icoll interrupt, a null one will disable it
 * NOTE the channel irq is automatically cleared */
void imx233_lradc_set_channel_irq_callback(int channel, lradc_irq_fn_t cb);
void imx233_lradc_kick_channel(int channel);
void imx233_lradc_kick_delay(int dchan);
void imx233_lradc_wait_channel(int channel);
int imx233_lradc_read_channel(int channel);
void imx233_lradc_clear_channel(int channel);
/* acquire a channel, returns -1 on timeout, channel otherwise
 * the returned channel is garanteed to be able measure source src */
int imx233_lradc_acquire_channel(int src, int timeout);
void imx233_lradc_release_channel(int chan);
// doesn't check that channel is in use!
void imx233_lradc_reserve_channel(int channel);

int imx233_lradc_acquire_delay(int timeout);
// doesn't check that delay channel is in use!
void imx233_lradc_reserve_delay(int dchannel);
void imx233_lradc_release_delay(int dchan);

/* Setup touch x/yminus pull donws and x/yplus pull ups */
void imx233_lradc_setup_touch(bool xminus_enable, bool yminus_enable,
    bool xplus_enable, bool yplus_enable, bool touch_detect);
void imx233_lradc_enable_touch_detect_irq(bool enable);
void imx233_lradc_clear_touch_detect_irq(void);
bool imx233_lradc_read_touch_detect(void);

#if IMX233_SUBTARGET >= 3700
/* enable sensing and return temperature in kelvin,
 * channels needs not to be configured */
int imx233_lradc_sense_die_temperature(int nmos_chan, int pmos_chan);
#endif
/* return *raw* external temperature, might need some transformation
 * channel needs not to be configured */
int imx233_lradc_sense_ext_temperature(int chan, int sensor);

void imx233_lradc_setup_battery_conversion(bool automatic, unsigned long scale_factor);
// read scaled voltage, only available after proper setup
int imx233_lradc_read_battery_voltage(void);

#endif /* __lradc_imx233__ */
