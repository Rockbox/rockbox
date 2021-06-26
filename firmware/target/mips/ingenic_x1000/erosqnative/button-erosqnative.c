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

#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "powermgmt.h"
#include "panic.h"
#include "axp-pmu.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "i2c-x1000.h"
#include <string.h>
#include <stdbool.h>

#ifndef BOOTLOADER
# include "lcd.h"
# include "font.h"
#endif

static int hp_detect_tmo_cb(struct timeout* tmo)
{
//     i2c_descriptor* d = (i2c_descriptor*)tmo->data;
//     i2c_async_queue(AXP_PMU_BUS, TIMEOUT_NOBLOCK, I2C_Q_ADD, 0, d);
//     return HPD_POLL_TIME;
}

static void hp_detect_init(void)
{
//     static struct timeout tmo;
//     static const uint8_t gpio_reg = AXP192_REG_GPIOSTATE1;
//     static i2c_descriptor desc = {
//         .slave_addr = AXP_PMU_ADDR,
//         .bus_cond = I2C_START | I2C_STOP,
//         .tran_mode = I2C_READ,
//         .buffer[0] = (void*)&gpio_reg,
//         .count[0] = 1,
//         .buffer[1] = &hp_detect_reg,
//         .count[1] = 1,
//         .callback = NULL,
//         .arg = 0,
//         .next = NULL,
//     };
// 
//     /* Headphone detect is wired to AXP192 GPIO: set it to input state */
//     i2c_reg_write1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP192_REG_GPIO2FUNCTION, 0x01);
// 
//     /* Get an initial reading before startup */
//     int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, gpio_reg);
//     if(r >= 0)
//         hp_detect_reg = r;
// 
//     /* Poll the register every second */
//     timeout_register(&tmo, &hp_detect_tmo_cb, HPD_POLL_TIME, (intptr_t)&desc);
}

/* Rockbox interface */
void button_init_device(void)
{
    /* Set up headphone detect polling */
    hp_detect_init();
}

int button_read_device(void)
{
    int r = 0;

    /* Read GPIOs for physical buttons */
    uint32_t a = REG_GPIO_PIN(GPIO_A);
    uint32_t b = REG_GPIO_PIN(GPIO_B);
    uint32_t c = REG_GPIO_PIN(GPIO_C);
    uint32_t d = REG_GPIO_PIN(GPIO_D);

    /* All buttons are active low */
    if((a & (1 << 16)) == 0) r |= BUTTON_PLAY;
    if((a & (1 << 17)) == 0) r |= BUTTON_VOL_UP;
    if((a & (1 << 19)) == 0) r |= BUTTON_VOL_DOWN;
    
    if((b & (1 << 7)) == 0) r  |= BUTTON_POWER;
    if((b & (1 << 27)) == 0) r |= BUTTON_MENU;
    if((b & (1 << 24)) == 0) r |= BUTTON_SCROLL_FWD;
    if((b & (1 << 23)) == 0) r |= BUTTON_SCROLL_BACK;
    
    if((c & (1 << 24)) == 0) r |= BUTTON_PREV;
    if((d & (1 << 5)) == 0) r  |= BUTTON_BACK;
    
    if((d & (1 << 4)) == 0) r  |= BUTTON_NEXT;
    

    return r;
}
