/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
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

#ifndef __DEV_GPIO_IOCTL_H__
#define __DEV_GPIO_IOCTL_H__

/* Specifies device name and ioctl magic */
#include "gpio-target.h"
#include "sys/ioctl.h"
#include "stdbool.h"

// set to debug pinctrl use
#define GPIO_DEBUG

#ifdef GPIO_DEBUG
void imx233_pinctrl_acquire(unsigned bank, unsigned pin, const char *name);
void imx233_pinctrl_release(unsigned bank, unsigned pin, const char *name);
const char *imx233_pinctrl_blame(unsigned bank, unsigned pin);
#else
#define imx233_pinctrl_acquire(...)
#define imx233_pinctrl_release(...)
#define imx233_pinctrl_get_pin_use(...) NULL
#endif

struct gpio_info {
    int num;
    int mode;
    int val;
} __attribute__((packed));

#define E_IOCTL_GPIO_SET_MUX            0
#define E_IOCTL_GPIO_UNSET_MUX          1
#define E_IOCTL_GPIO_SET_TYPE           2
#define E_IOCTL_GPIO_SET_OUTPUT         3
#define E_IOCTL_GPIO_SET_INPUT          4
#define E_IOCTL_GPIO_SET_HIGH           5
#define E_IOCTL_GPIO_SET_LOW            6
#define E_IOCTL_GPIO_GET_VAL            7
#define E_IOCTL_GPIO_IS_HIGH            8
#define E_IOCTL_GPIO_MAX_NR             9

#define DEV_CTRL_GPIO_SET_MUX           _IOW(GPIO_IOCTL_MAGIC, 0, struct gpio_info)
#define DEV_CTRL_GPIO_UNSET_MUX         _IOW(GPIO_IOCTL_MAGIC, 1, struct gpio_info)
#define DEV_CTRL_GPIO_SET_TYPE          _IOW(GPIO_IOCTL_MAGIC, 2, struct gpio_info)
#define DEV_CTRL_GPIO_SET_OUTPUT        _IOW(GPIO_IOCTL_MAGIC, 3, struct gpio_info)
#define DEV_CTRL_GPIO_SET_INPUT         _IOW(GPIO_IOCTL_MAGIC, 4, struct gpio_info)
#define DEV_CTRL_GPIO_SET_HIGH          _IOW(GPIO_IOCTL_MAGIC, 5, struct gpio_info)
#define DEV_CTRL_GPIO_SET_LOW           _IOW(GPIO_IOCTL_MAGIC, 6, struct gpio_info)
#define DEV_CTRL_GPIO_GET_VAL           _IOW(GPIO_IOCTL_MAGIC, 7, struct gpio_info)
#define DEV_CTRL_GPIO_IS_HIGH           _IOW(GPIO_IOCTL_MAGIC, 8, struct gpio_info)

enum
{
    GPIO1_0 = 0,    /* GPIO group 1 start */
    GPIO1_1,
    GPIO1_2,
    GPIO1_3,
    GPIO1_4,
    GPIO1_5,
    GPIO1_6,
    GPIO1_7,
    GPIO1_8,
    GPIO1_9,
    GPIO1_10,
    GPIO1_11,
    GPIO1_12,
    GPIO1_13,
    GPIO1_14,
    GPIO1_15,
    GPIO1_16,
    GPIO1_17,
    GPIO1_18,
    GPIO1_19,
    GPIO1_20,
    GPIO1_21,
    GPIO1_22,
    GPIO1_23,
    GPIO1_24,
    GPIO1_25,
    GPIO1_26,
    GPIO1_27,
    GPIO1_28,
    GPIO1_29,
    GPIO1_30,
    GPIO1_31,
    GPIO2_0,    /* GPIO group 2 start */
    GPIO2_1,
    GPIO2_2,
    GPIO2_3,
    GPIO2_4,
    GPIO2_5,
    GPIO2_6,
    GPIO2_7,
    GPIO2_8,
    GPIO2_9,
    GPIO2_10,
    GPIO2_11,
    GPIO2_12,
    GPIO2_13,
    GPIO2_14,
    GPIO2_15,
    GPIO2_16,
    GPIO2_17,
    GPIO2_18,
    GPIO2_19,
    GPIO2_20,
    GPIO2_21,
    GPIO2_22,
    GPIO2_23,
    GPIO2_24,
    GPIO2_25,
    GPIO2_26,
    GPIO2_27,
    GPIO2_28,
    GPIO2_29,
    GPIO2_30,
    GPIO2_31,
    GPIO3_0,    /* GPIO group 3 start */
    GPIO3_1,
    GPIO3_2,
    GPIO3_3,
    GPIO3_4,
    GPIO3_5,
    GPIO3_6,
    GPIO3_7,
    GPIO3_8,
    GPIO3_9,
    GPIO3_10,
    GPIO3_11,
    GPIO3_12,
    GPIO3_13,
    GPIO3_14,
    GPIO3_15,
    GPIO3_16,
    GPIO3_17,
    GPIO3_18,
    GPIO3_19,
    GPIO3_20,
    GPIO3_21,
    GPIO3_22,
    GPIO3_23,
    GPIO3_24,
    GPIO3_25,
    GPIO3_26,
    GPIO3_27,
    GPIO3_28,
    GPIO3_29,
    GPIO3_30,
    GPIO3_31,
};

enum
{
    CONFIG_ALT0,
    CONFIG_ALT1,
    CONFIG_ALT2,
    CONFIG_ALT3,
    CONFIG_ALT4,
    CONFIG_ALT5,
    CONFIG_ALT6,
    CONFIG_ALT7,
    CONFIG_GPIO,
    CONFIG_SION = 0x01 << 4,
    CONFIG_DEFAULT
};

enum
{
    PAD_CTL_SRE_SLOW = 0x0 << 0,
    PAD_CTL_SRE_FAST = 0x1 << 0,
    PAD_CTL_DRV_LOW = 0x0 << 1,
    PAD_CTL_DRV_MEDIUM = 0x1 << 1,
    PAD_CTL_DRV_HIGH = 0x2 << 1,
    PAD_CTL_DRV_MAX = 0x3 << 1,
    PAD_CTL_ODE_OPENDRAIN_NONE = 0x0 << 3,
    PAD_CTL_ODE_OPENDRAIN_ENABLE = 0x1 << 3,
    PAD_CTL_100K_PD = 0x0 << 4,
    PAD_CTL_47K_PU = 0x1 << 4,
    PAD_CTL_100K_PU = 0x2 << 4,
    PAD_CTL_22K_PU = 0x3 << 4,
    PAD_CTL_PUE_KEEPER = 0x0 << 6,
    PAD_CTL_PUE_PULL = 0x1 << 6,
    PAD_CTL_PKE_NONE = 0x0 << 7,
    PAD_CTL_PKE_ENABLE = 0x1 << 7,
    PAD_CTL_HYS_NONE = 0x0 << 8,
    PAD_CTL_HYS_ENABLE = 0x1 << 8,
    PAD_CTL_DDR_INPUT_CMOS = 0x0 << 9,
    PAD_CTL_DDR_INPUT_DDR = 0x1 << 9,
    PAD_CTL_DRV_VOT_LOW = 0x0 << 13,
    PAD_CTL_DRV_VOT_HIGH = 0x1 << 13,
};

/* Driver-related functions */
void gpio_init(void);
void gpio_close(void);

/* Exported API */
void gpio_set_iomux(int pin, int iomux);
void gpio_free_iomux(int pin, int iomux);
void gpio_set_pad(int pin, int type);
void gpio_direction_output(int pin);
void gpio_direction_input(int pin);
bool gpio_get(int pin);
void gpio_set(int pin, bool value);
#endif
