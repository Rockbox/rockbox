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
static int wheel_pos = 0;

void button_init_device(void)
{
    /* Set up volume wheel interrupt */
    gpio_set_function(GPIO_WHEEL1, GPIOF_IRQ_EDGE(0));
    gpio_enable_irq(GPIO_WHEEL1);

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
}

/* WHEEL1 interrupt */
void GPIOD02(void)
{
    /* Use one pin as an interrupt and sample the other pin to get direction.
     * Seems to be very reliable, taken from @Evan's answer here:
     * https://electronics.stackexchange.com/questions/121564/encoder-sampling-rate-realization
     */
    if(gpio_get_level(GPIO_WHEEL2))
        wheel_pos += 1;
    else
        wheel_pos -= 1;
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

    /* FIXME: Need acceleration for the volume wheel, like list acceleration.
     * It seems wheel_pos works great but the reporting here is bad, what we
     * need is a way to report how fast the wheel is turning and skip volume
     * steps, similar to how list acceleration skips list items. */
    if(wheel_pos > 0)
        r |= BUTTON_VOL_UP;
    else if(wheel_pos < 0)
        r |= BUTTON_VOL_DOWN;
    wheel_pos = 0;

    /* Handle touchscreen */
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
    /* TODO: implement this; it is hooked onto an AXP GPIO somehow. */
    return true;
}
