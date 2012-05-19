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

#define HW_LRADC_BASE       0x80050000

#define HW_LRADC_CTRL0      (*(volatile uint32_t *)(HW_LRADC_BASE + 0x0))
#define HW_LRADC_CTRL0__XPLUS_ENABLE        (1 << 16)
#define HW_LRADC_CTRL0__YPLUS_ENABLE        (1 << 17)
#define HW_LRADC_CTRL0__XMINUS_ENABLE       (1 << 18)
#define HW_LRADC_CTRL0__YMINUS_ENABLE       (1 << 19)
#define HW_LRADC_CTRL0__TOUCH_DETECT_ENABLE (1 << 20)
#define HW_LRADC_CTRL0__ONCHIP_GROUNDREF    (1 << 21)
#define HW_LRADC_CTRL0__SCHEDULE(x)         (1 << (x))

#define HW_LRADC_CTRL1      (*(volatile uint32_t *)(HW_LRADC_BASE + 0x10))
#define HW_LRADC_CTRL1__LRADCx_IRQ(x)       (1 << (x))
#define HW_LRADC_CTRL1__TOUCH_DETECT_IRQ    (1 << 8)
#define HW_LRADC_CTRL1__LRADCx_IRQ_EN(x)    (1 << ((x) + 16))
#define HW_LRADC_CTRL1__TOUCH_DETECT_IRQ_EN (1 << 24)

#define HW_LRADC_CTRL2      (*(volatile uint32_t *)(HW_LRADC_BASE + 0x20))
#define HW_LRADC_CTRL2__TEMP_ISRC1_BP           4
#define HW_LRADC_CTRL2__TEMP_ISRC1_BM           0xf0
#define HW_LRADC_CTRL2__TEMP_ISRC0_BP           0
#define HW_LRADC_CTRL2__TEMP_ISRC0_BM           0xf
#define HW_LRADC_CTRL2__TEMP_ISRCx_BP(x)        (4 * (x))
#define HW_LRADC_CTRL2__TEMP_ISRCx_BM(x)        (0xf << (4 * (x)))
#define HW_LRADC_CTRL2__TEMP_ISRC__0uA          0
#define HW_LRADC_CTRL2__TEMP_ISRC__20uA         1
#define HW_LRADC_CTRL2__TEMP_ISRC__300uA        15
#define HW_LRADC_CTRL2__TEMP_SENSOR_IENABLE0    (1 << 8)
#define HW_LRADC_CTRL2__TEMP_SENSOR_IENABLE1    (1 << 9)
#define HW_LRADC_CTRL2__TEMP_SENSOR_IENABLEx(x) (1 << (8 + (x)))
#define HW_LRADC_CTRL2__TEMPSENSE_PWD           (1 << 15)
#define HW_LRADC_CTRL2__DIVIDE_BY_TWO(x)        (1 << ((x) + 24))

#define HW_LRADC_CTRL3      (*(volatile uint32_t *)(HW_LRADC_BASE + 0x30))
#define HW_LRADC_CTRL3__CYCLE_TIME_BM       0x300
#define HW_LRADC_CTRL3__CYCLE_TIME_BP       8
#define HW_LRADC_CTRL3__CYCLE_TIME__6MHz    (0 << 8)
#define HW_LRADC_CTRL3__CYCLE_TIME__4MHz    (1 << 8)
#define HW_LRADC_CTRL3__CYCLE_TIME__3MHz    (2 << 8)
#define HW_LRADC_CTRL3__CYCLE_TIME__2MHz    (3 << 8)

#define HW_LRADC_STATUS     (*(volatile uint32_t *)(HW_LRADC_BASE + 0x40))
#define HW_LRADC_STATUS__TOUCH_DETECT_RAW   (1 << 0)

#define HW_LRADC_CHx(x)     (*(volatile uint32_t *)(HW_LRADC_BASE + 0x50 + (x) * 0x10))
#define HW_LRADC_CHx__NUM_SAMPLES_BM    (0x1f << 24)
#define HW_LRADC_CHx__NUM_SAMPLES_BP    24
#define HW_LRADC_CHx__ACCUMULATE        29
#define HW_LRADC_CHx__VALUE_BM          0x3ffff
#define HW_LRADC_CHx__VALUE_BP          0

#define HW_LRADC_DELAYx(x)  (*(volatile uint32_t *)(HW_LRADC_BASE + 0xD0 + (x) * 0x10))
#define HW_LRADC_DELAYx__DELAY_BP           0
#define HW_LRADC_DELAYx__DELAY_BM           0x7ff
#define HW_LRADC_DELAYx__LOOP_COUNT_BP      11
#define HW_LRADC_DELAYx__LOOP_COUNT_BM      (0x1f << 11)
#define HW_LRADC_DELAYx__TRIGGER_DELAYS_BP  16
#define HW_LRADC_DELAYx__TRIGGER_DELAYS_BM  (0xf << 16)
#define HW_LRADC_DELAYx__KICK               (1 << 20)
#define HW_LRADC_DELAYx__TRIGGER_LRADCS_BP  24
#define HW_LRADC_DELAYx__TRIGGER_LRADCS_BM  (0xff << 24)

#define HW_LRADC_CONVERSION (*(volatile uint32_t *)(HW_LRADC_BASE + 0x130))
#define HW_LRADC_CONVERSION__SCALED_BATT_VOLTAGE_BP 0
#define HW_LRADC_CONVERSION__SCALED_BATT_VOLTAGE_BM 0x3ff
#define HW_LRADC_CONVERSION__SCALE_FACTOR_BM        (3 << 16)
#define HW_LRADC_CONVERSION__SCALE_FACTOR_BP        16
#define HW_LRADC_CONVERSION__SCALE_FACTOR__LI_ION   (2 << 16)
#define HW_LRADC_CONVERSION__AUTOMATIC              (1 << 20)

#define HW_LRADC_CTRL4      (*(volatile uint32_t *)(HW_LRADC_BASE + 0x140))
#define HW_LRADC_CTRL4__LRADCxSELECT_BM(x)  (0xf << ((x) * 4))
#define HW_LRADC_CTRL4__LRADCxSELECT_BP(x)  ((x) * 4)

#define HW_LRADC_VERSION    (*(volatile uint32_t *)(HW_LRADC_BASE + 0x150))

#define HW_LRADC_NUM_CHANNELS   8
#define HW_LRADC_NUM_DELAYS     4

#define HW_LRADC_CHANNEL(x)         (x)
#define HW_LRADC_CHANNEL_XPLUS      HW_LRADC_CHANNEL(2)
#define HW_LRADC_CHANNEL_YPLUS      HW_LRADC_CHANNEL(3)
#define HW_LRADC_CHANNEL_XMINUS     HW_LRADC_CHANNEL(4)
#define HW_LRADC_CHANNEL_YMINUS     HW_LRADC_CHANNEL(5)
#define HW_LRADC_CHANNEL_VDDIO      HW_LRADC_CHANNEL(6)
#define HW_LRADC_CHANNEL_BATTERY    HW_LRADC_CHANNEL(7)
#define HW_LRADC_CHANNEL_PMOS_THIN  HW_LRADC_CHANNEL(8)
#define HW_LRADC_CHANNEL_NMOS_THIN  HW_LRADC_CHANNEL(9)
#define HW_LRADC_CHANNEL_NMOS_THICK HW_LRADC_CHANNEL(10)
#define HW_LRADC_CHANNEL_PMOS_THICK HW_LRADC_CHANNEL(11)
#define HW_LRADC_CHANNEL_PMOS_THICK HW_LRADC_CHANNEL(11)
#define HW_LRADC_CHANNEL_USB_DP     HW_LRADC_CHANNEL(12)
#define HW_LRADC_CHANNEL_USB_DN     HW_LRADC_CHANNEL(13)
#define HW_LRADC_CHANNEL_VBG        HW_LRADC_CHANNEL(14)
#define HW_LRADC_CHANNEL_5V         HW_LRADC_CHANNEL(15)

typedef void (*lradc_irq_fn_t)(int chan);

void imx233_lradc_init(void);
void imx233_lradc_setup_channel(int channel, bool div2, bool acc, int nr_samples, int src);
void imx233_lradc_setup_delay(int dchan, int trigger_lradc, int trigger_delays,
    int loop_count, int delay);
void imx233_lradc_clear_channel_irq(int channel);
bool imx233_lradc_read_channel_irq(int channel);
void imx233_lradc_enable_channel_irq(int channel, bool enable);
void imx233_lradc_set_channel_irq_callback(int channel, lradc_irq_fn_t cb);
void imx233_lradc_kick_channel(int channel);
void imx233_lradc_kick_delay(int dchan);
void imx233_lradc_wait_channel(int channel);
int imx233_lradc_read_channel(int channel);
void imx233_lradc_clear_channel(int channel);
// acquire a virtual channel, returns -1 on timeout, channel otherwise */
int imx233_lradc_acquire_channel(int timeout);
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

/* enable sensing and return temperature in kelvin,
 * channels needs not to be configured */
int imx233_lradc_sense_die_temperature(int nmos_chan, int pmos_chan);
/* return *raw* external temperature, might need some transformation
 * channel needs not to be configured */
int imx233_lradc_sense_ext_temperature(int chan, int sensor);

void imx233_lradc_setup_battery_conversion(bool automatic, unsigned long scale_factor);
// read scaled voltage, only available after proper setup
int imx233_lradc_read_battery_voltage(void);

#endif /* __lradc_imx233__ */
