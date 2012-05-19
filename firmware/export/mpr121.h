/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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

/** Driver for the Freescale MPR121 Capacitive Proximity Sensor */
#include "system.h"

#define ELECTRODE_COUNT 12
#define ELE_GPIO_FIRST  4
#define ELE_GPIO_LAST   11

/* gpio config (encoding: [0]=en,[1]=dir,[2]=ctl[1],[3]=ctl[0]) */
#define ELE_GPIO_DISABLE        0
#define ELE_GPIO_INPUT          1
#define ELE_GPIO_INPUT_PULLDOWN 9 /* input with pull-down */
#define ELE_GPIO_INPUT_PULLUP   13 /* input with pull-up */
#define ELE_GPIO_OUTPUT         3
#define ELE_GPIO_OUTPUT_OPEN     11 /* open drain low-side */
#define ELE_GPIO_OUTPUT_OPEN_LED 15 /* open drain high-side (led driver) */

/* internal use */
#define ELE_GPIO_EN(val)    ((val) & 1)
#define ELE_GPIO_DIR(val)   (((val) >> 1) & 1)
#define ELE_GPIO_CTL0(val)  (((val) >> 3) & 1)
#define ELE_GPIO_CTL1(val)  (((val) >> 1) & 1)

struct mpr121_electrode_config_t
{
    uint8_t bv; /* baseline value */
    uint8_t tth; /* touch threshold */
    uint8_t rth; /* release threshold */
    uint8_t cdc; /* charge current (optional if auto-conf) */
    uint8_t cdt; /* charge time (optional if auto-conf) */
    int gpio; /* gpio config */
};

struct mpr121_baseline_filter_config_t
{
    uint8_t mhd; /* max half delta (except for touched) */
    uint8_t nhd; /* noise half delta */
    uint8_t ncl; /* noise count limit */
    uint8_t fdl; /* filter delay count limit */
};

struct mpr121_baseline_filters_config_t
{
    struct mpr121_baseline_filter_config_t rising;
    struct mpr121_baseline_filter_config_t falling;
    struct mpr121_baseline_filter_config_t touched;
};

struct mpr121_debounce_config_t
{
    uint8_t dt; /* debounce count for touch */
    uint8_t dr; /* debounce count for release */
};

/* first filter iterations */
#define FFI_6_SAMPLES   0
#define FFI_10_SAMPLES  1
#define FFI_18_SAMPLES  2
#define FFI_34_SAMPLES  3
/* charge discharge current */
#define CDC_DISABLE     0
#define CDC_uA(ua)      (ua)
/* charge discharge time */
#define CDT_DISABLE     0
#define CDT_log_us(lus) (lus) /* actual value = 2^{us-2} µs */
/* second filter iterations */
#define SFI_4_SAMPLES   0
#define SFI_6_SAMPLES   1
#define SFI_10_SAMPLES  2
#define SFI_18_SAMPLES  3
/* Eletrode sample interval */
#define ESI_log_ms(lms) (lms) /* actual value = 2^{lms} ms */

struct mpr121_global_config_t
{
    uint8_t ffi; /* first filter iterations */
    uint8_t cdc; /* global charge discharge current */
    uint8_t cdt; /* global charge discharge time */
    uint8_t sfi; /* second first iterations */
    uint8_t esi; /* electrode sample interval */
};

#define RETRY_NEVER     0
#define RETRY_2_TIMES   1
#define RETRY_4_TIMES   2
#define RETRY_8_TIMES   3

struct mpr121_auto_config_t
{
    bool en; /* auto-conf enable */
    bool ren; /* auto-reconf enable */
    uint8_t retry; /* retry count */
    bool scts; /* skip charge time search */
    uint8_t usl; /* upper-side limit */
    uint8_t lsl; /* lower-side limit */
    uint8_t tl; /* target level */
    bool acfie; /* auto-conf fail interrupt en */
    bool arfie; /* auto-reconf fail interrupt en */
    bool oorie; /* out of range interrupt en */
};

/* electrode mode */
#define ELE_DISABLE     0
#define ELE_EN0_x(x)    ((x) + 1)
/* eleprox mode */
#define ELEPROX_DISABLE 0
#define ELEPROX_EN0_1   1
#define ELEPROX_EN0_3   2
#define ELEPROX_EN0_11  3
/* calibration lock */
#define CL_SLOW_TRACK   0
#define CL_DISABLE      1
#define CL_TRACK        2
#define CL_FAST_TRACK   3

struct mpr121_config_t
{
    struct mpr121_electrode_config_t ele[ELECTRODE_COUNT];
    struct mpr121_electrode_config_t eleprox;
    struct
    {
        struct mpr121_baseline_filters_config_t ele;
        struct mpr121_baseline_filters_config_t eleprox;
    }filters;
    struct mpr121_debounce_config_t debounce;
    struct mpr121_global_config_t global;
    struct mpr121_auto_config_t autoconf;
    uint8_t ele_en; /* eletroce mode */
    uint8_t eleprox_en; /* proximity mode */
    uint8_t cal_lock; /* calibration lock */
};

/* gpio value */
#define ELE_GPIO_CLR    0
#define ELE_GPIO_SET    1
#define ELE_GPIO_TOG    2
/* pwm value */
#define ELE_PWM_DISABLE 0
#define ELE_PWM_DUTY(x) (x)
#define ELE_PWM_MIN_DUTY    1
#define ELE_PWM_MAX_DUTY    15

int mpr121_init(int dev_i2c_addr);
int mpr121_soft_reset(void);
int mpr121_set_config(struct mpr121_config_t *conf);
/* gpios are only implemented for electrode>=4 */
int mpr121_set_gpio_output(int ele, int gpio_val);
int mpr121_set_gpio_pwm(int ele, int pwm);
/* get electrode status (bitmap) */
int mpr121_get_touch_status(unsigned *status);
