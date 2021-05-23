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
 * Copyright (C) 2021 Dana Conrad
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
#include "touchscreen.h"
#include "ft6x06.h"
#include "axp-pmu.h"
#include "kernel.h"
#include "backlight.h"
#include "powermgmt.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "i2c-x1000.h"
#include <stdbool.h>

/* Volume wheel rotation */
static volatile int wheel_pos = 0;

/* Value of headphone detect register */
static uint8_t hp_detect_reg = 0x00;

/* Interval to poll the register */
#define HPD_POLL_TIME (HZ/2)

static int hp_detect_tmo_cb(struct timeout* tmo)
{
    i2c_descriptor* d = (i2c_descriptor*)tmo->data;
    i2c_async_queue(AXP_PMU_BUS, TIMEOUT_NOBLOCK, I2C_Q_ADD, 0, d);
    return HPD_POLL_TIME;
}

static void hp_detect_init(void)
{
    /* TODO: replace this copy paste cruft with an API in axp-pmu */
    static struct timeout tmo;
    static const uint8_t gpio_reg = AXP192_REG_GPIOSTATE1;
    static i2c_descriptor desc = {
        .slave_addr = AXP_PMU_ADDR,
        .bus_cond = I2C_START | I2C_STOP,
        .tran_mode = I2C_READ,
        .buffer[0] = (void*)&gpio_reg,
        .count[0] = 1,
        .buffer[1] = &hp_detect_reg,
        .count[1] = 1,
        .callback = NULL,
        .arg = 0,
        .next = NULL,
    };

    /* Headphone detect is wired to AXP192 GPIO: set it to input state */
    i2c_reg_write1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP192_REG_GPIO1FUNCTION, 0x01);

    /* Get an initial reading before startup */
    int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, gpio_reg);
    if(r >= 0)
        hp_detect_reg = r;

    /* Poll the register every second */
    timeout_register(&tmo, &hp_detect_tmo_cb, HPD_POLL_TIME, (intptr_t)&desc);
}

void button_init_device(void)
{
    /* Setup interrupts for the volume wheel */
    gpio_set_function(GPIO_WHEEL1, GPIOF_IRQ_EDGE(0));
    gpio_set_function(GPIO_WHEEL2, GPIOF_IRQ_EDGE(0));
    gpio_flip_edge_irq(GPIO_WHEEL1);
    gpio_flip_edge_irq(GPIO_WHEEL2);
    gpio_enable_irq(GPIO_WHEEL1);
    gpio_enable_irq(GPIO_WHEEL2);

    /* Init touchscreen driver */
    i2c_x1000_set_freq(FT6x06_BUS, I2C_FREQ_400K);
    ft6x06_init();

    /* Reset touch controller */
    gpio_set_level(GPIO_FT6x06_POWER, 1);
    gpio_set_level(GPIO_FT6x06_RESET, 0);
    mdelay(5);
    gpio_set_level(GPIO_FT6x06_RESET, 1);

    /* Enable ft6x06 interrupt */
    system_set_irq_handler(GPIO_TO_IRQ(GPIO_FT6x06_INTERRUPT), ft6x06_irq_handler);
    gpio_set_function(GPIO_FT6x06_INTERRUPT, GPIOF_IRQ_EDGE(0));
    gpio_enable_irq(GPIO_FT6x06_INTERRUPT);

    /* Headphone detection */
    hp_detect_init();
}

int button_read_device(int* data)
{
    int r = 0;

    /* Read GPIO buttons, these are all active low */
    uint32_t b = REG_GPIO_PIN(GPIO_B);
    if((b & (1 << 21)) == 0) r |= BUTTON_PREV;
    if((b & (1 << 22)) == 0) r |= BUTTON_NEXT;
    if((b & (1 << 28)) == 0) r |= BUTTON_PLAY;
    if((b & (1 << 31)) == 0) r |= BUTTON_POWER;

    /* Check the wheel */
    int wheel_btn = 0;
    int whpos = wheel_pos;
    if(whpos > 3)
        wheel_btn = BUTTON_VOL_DOWN;
    else if(whpos < -3)
        wheel_btn = BUTTON_VOL_UP;

    if(wheel_btn) {
        wheel_pos = 0;

        /* Post the event (rapid motion is more reliable this way) */
        queue_post(&button_queue, wheel_btn, 0);
        queue_post(&button_queue, wheel_btn|BUTTON_REL, 0);

        /* Poke the backlight */
        backlight_on();
        reset_poweroff_timer();
    }

    /* Handle touchscreen
     *
     * TODO: Support 2-point multitouch (useful for 3x3 grid mode)
     * TODO: Support simple gestures by converting them to fake buttons
     */
    int t = touchscreen_to_pixels(ft6x06_state.pos_x, ft6x06_state.pos_y, data);
    if(ft6x06_state.event == FT6x06_EVT_PRESS ||
       ft6x06_state.event == FT6x06_EVT_CONTACT) {
        /* Only set the button bit if the screen is being touched. */
        r |= t;
    }

    return r;
}

void touchscreen_enable_device(bool en)
{
    ft6x06_enable(en);
    /* TODO: check if it's worth shutting off the power pin */
}

bool headphones_inserted(void)
{
    /* TODO: Also check if the headset button is detectable via an ADC.
     * The AXP driver should probably get proper interrupt handling,
     * that would be useful for more things than just GPIO polling. */
    return hp_detect_reg & 0x20 ? true : false;
}

static void handle_wheel_irq(void)
{
    /* Wheel stuff adapted from button-erosqnative.c */
    static const int delta[16] = { 0, -1,  1,  0,
                                   1,  0,  0, -1,
                                  -1,  0,  0,  1,
                                   0,  1, -1,  0 };
    static uint32_t state = 0;
    state <<= 2;
    state |= (REG_GPIO_PIN(GPIO_D) >> 2) & 3;
    state &= 0xf;

    wheel_pos += delta[state];
}

void GPIOD02(void)
{
    handle_wheel_irq();
    gpio_flip_edge_irq(GPIO_WHEEL1);
}

void GPIOD03(void)
{
    handle_wheel_irq();
    gpio_flip_edge_irq(GPIO_WHEEL2);
}
