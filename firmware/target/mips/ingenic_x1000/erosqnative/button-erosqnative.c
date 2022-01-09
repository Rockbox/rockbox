/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald, Dana Conrad
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
#include "eros_qn_codec.h"
#include <string.h>
#include <stdbool.h>

#ifndef BOOTLOADER
# include "lcd.h"
# include "font.h"
#endif

/* ===========================================
 * | OLD STATE | NEW STATE |  DIRECTION      |
 * |   0  0    |   0  0    |  0: NO CHANGE   |
 * |   0  0    |   0  1    | -1: CCW         |
 * |   0  0    |   1  0    |  1: CW          |
 * |   0  0    |   1  1    |  0: INVALID     |
 * |   0  1    |   0  0    |  1: CW          |
 * |   0  1    |   0  1    |  0: NO CHANGE   |
 * |   0  1    |   1  0    |  0: INVALID     |
 * |   0  1    |   1  1    | -1: CCW         |
 * |   1  0    |   0  0    | -1: CCW         |
 * |   1  0    |   0  1    |  0: INVALID     |
 * |   1  0    |   1  0    |  0: NO CHANGE   |
 * |   1  0    |   1  1    |  1: CW          |
 * |   1  1    |   0  0    |  0: INVALID     |
 * |   1  1    |   0  1    |  1: CW          |
 * |   1  1    |   1  0    | -1: CCW         |
 * |   1  1    |   1  1    |  0: NO CHANGE   |
 * ===========================================
 *
 * Quadrature explanation since it's not plainly obvious how this works:
 *
 * If either of the quadrature lines change, we can look up the combination
 * of previous state and new state in the table above (enc_state[] below)
 * and it tells us whether to add 1, subtract 1, or no change from the sum (enc_position).
 * This also gives us a nice debounce, since each state can only have 1 pin change
 * at a time. I didn't come up with this, but I've used it before and it works well.
 *
 * Old state is 2 higher bits, new state is 2 lower bits of enc_current_state. */

/* list of valid quadrature states and their directions */
signed char enc_state[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
volatile unsigned char enc_current_state = 0;
volatile signed int enc_position = 0;

/* Value of headphone detect register */
static uint8_t hp_detect_reg = 0x00;
static uint8_t hp_detect_reg_old = 0x00;

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

    /* Headphone and LO detects are wired to AXP192 GPIOs 0 and 1,
     * set them to inputs. */
    i2c_reg_write1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP192_REG_GPIO0FUNCTION, 0x01); /* HP detect */
    i2c_reg_write1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP192_REG_GPIO1FUNCTION, 0x01); /* LO detect */

    /* Get an initial reading before startup */
    int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, gpio_reg);
    if(r >= 0)
    {
        hp_detect_reg = r;
        hp_detect_reg_old = hp_detect_reg;
    }

    /* Poll the register every second */
    timeout_register(&tmo, &hp_detect_tmo_cb, HPD_POLL_TIME, (intptr_t)&desc);
}

bool headphones_inserted(void)
{
    /* if the status has changed, set the output volume accordingly */
    if ((hp_detect_reg & 0x30) != (hp_detect_reg_old & 0x30))
    {
        hp_detect_reg_old = hp_detect_reg;
#if !defined(BOOTLOADER)
        pcm5102_set_outputs();
#endif
    }
    return hp_detect_reg & 0x10 ? false : true;
}

bool lineout_inserted(void)
{
    /* if the status has changed, set the output volume accordingly */
    if ((hp_detect_reg & 0x30) != (hp_detect_reg_old & 0x30))
    {
        hp_detect_reg_old = hp_detect_reg;
#if !defined(BOOTLOADER)
        pcm5102_set_outputs();
#endif
    }
    return hp_detect_reg & 0x20 ? false : true;
}

/* Rockbox interface */
void button_init_device(void)
{
    /* set both quadrature lines to interrupts */
    gpio_set_function(GPIO_BTN_SCROLL_A, GPIOF_IRQ_EDGE(1));
    gpio_set_function(GPIO_BTN_SCROLL_B, GPIOF_IRQ_EDGE(1));

    /* set interrupts to fire on the next edge based on current state */
    gpio_flip_edge_irq(GPIO_BTN_SCROLL_A);
    gpio_flip_edge_irq(GPIO_BTN_SCROLL_B);

    /* get current state of both encoder gpios */
    enc_current_state = (REG_GPIO_PIN(GPIO_B)>>21) & 0x0c;

    /* enable quadrature interrupts */
    gpio_enable_irq(GPIO_BTN_SCROLL_A);
    gpio_enable_irq(GPIO_BTN_SCROLL_B);

    /* Set up headphone and line out detect polling */
    hp_detect_init();
}

/* wheel Quadrature line A interrupt */
void GPIOB24(void)
{
    /* fill state with previous (2 higher bits) and current (2 lower bits) */
    enc_current_state = (enc_current_state & 0x0c) | ((REG_GPIO_PIN(GPIO_B)>>23) & 0x03);

    /* look up in table */
    enc_position = enc_position + enc_state[(enc_current_state)];

    /* move current state to previous state if valid data */
    if (enc_state[(enc_current_state)] != 0)
        enc_current_state = (enc_current_state << 2);

    /* we want the other edge next time */
    gpio_flip_edge_irq(GPIO_BTN_SCROLL_A);
}

/* wheel Quadrature line B interrupt */
void GPIOB23(void)
{
    /* fill state with previous (2 higher bits) and current (2 lower bits) */
    enc_current_state = (enc_current_state & 0x0c) | ((REG_GPIO_PIN(GPIO_B)>>23) & 0x03);

    /* look up in table */
    enc_position = enc_position + enc_state[(enc_current_state)];

    /* move current state to previous state if valid data */
    if (enc_state[(enc_current_state)] != 0)
        enc_current_state = (enc_current_state << 2);

    /* we want the other edge next time */
    gpio_flip_edge_irq(GPIO_BTN_SCROLL_B);
}

int button_read_device(void)
{
    int r = 0;

    /* Read GPIOs for normal buttons */
    uint32_t a = REG_GPIO_PIN(GPIO_A);
    uint32_t b = REG_GPIO_PIN(GPIO_B);
    uint32_t c = REG_GPIO_PIN(GPIO_C);
    uint32_t d = REG_GPIO_PIN(GPIO_D);

    /* All buttons are active low */
    if((a & (1 << 16)) == 0) r  |= BUTTON_PLAY;
    if((a & (1 << 17)) == 0) r  |= BUTTON_VOL_UP;
    if((a & (1 << 19)) == 0) r  |= BUTTON_VOL_DOWN;

    if((b & (1 <<  7)) == 0) r  |= BUTTON_POWER;
    if((b & (1 << 28)) == 0) r  |= BUTTON_MENU;
    if((b & (1 << 28)) == 0) r  |= BUTTON_MENU;

    if((d & (1 <<  4)) == 0) r  |= BUTTON_PREV;

    if((d & (1 <<  5)) == 0) r  |= BUTTON_BACK;
    if((c & (1 << 24)) == 0) r  |= BUTTON_NEXT;

    /* check encoder - from testing, each indent is 2 state changes or so */
    if (enc_position > 1)
    {
        /* need to use queue_post() in order to do BUTTON_SCROLL_*,
         * Rockbox treats these buttons differently. */
        queue_post(&button_queue, BUTTON_SCROLL_FWD, 0);
        enc_position = 0;
    }
    else if (enc_position < -1)
    {
        /* need to use queue_post() in order to do BUTTON_SCROLL_*,
         * Rockbox treats these buttons differently. */
        queue_post(&button_queue, BUTTON_SCROLL_BACK, 0);
        enc_position = 0;
    }

    return r;
}

