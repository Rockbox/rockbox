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
#include "gpio-x1000.h"
#include "i2c-x1000.h"

/* TODO: improve the FT6x06 driver by starting I2C ops asynchronously
 *
 * This will eliminate the separate thread used right now. This is only needed
 * because the I2C driver relies on a thread context to perform multi-step
 * transactions.
 *
 * A better method is submitting a list of descriptors which the I2C driver
 * can step through -- see button-fuzeplus.c for an example of this.
 */

/* FT6x06 mini-driver */
#define FT_CHN  1
#define FT_ADDR 0x38

#define FT_RST_PIN (1 << 15)
#define FT_INT_PIN (1 << 12)
#define ft_interrupt GPIOB12

static volatile int ft_state = 0;
static volatile bool ft_active = false;
static struct semaphore ft_ready;
static char ft_stack[DEFAULT_STACK_SIZE];
static const char ft_thread_name[] = "ft6x06";
volatile int ft_last_x = 0, ft_last_y = 0;
volatile int ft_last_evt = 0;
volatile long ft_last_int = 0;

static int ft_touch_to_button(int x, int y)
{
    if(y == 900) {
        /* Right strip */
        if(x == 80)
            return BUTTON_BACK;
        else if(x == 240)
            return BUTTON_RIGHT;
        else
            return 0;
    } else if(y < 80) {
        /* Left strip */
        if(x < 80)
            return BUTTON_MENU;
        else if(x > 190)
            return BUTTON_LEFT;
        else
            return 0;
    } else {
        /* Middle strip */
        if(x < 110)
            return BUTTON_UP;
        else if(x > 190)
            return BUTTON_DOWN;
        else
            return BUTTON_SELECT;
    }
}

/* TODO - implement FiiO M3K scroll bar
 * Relevant defines: HAVE_SCROLLWHEEL and HAVE_WHEEL_ACCELERATION.
 */

static int ft_read_touch(void)
{
    int ret;
    unsigned char buf[5] = { 0x02 };

    /* No need if we're not active */
    if(!ft_active)
        return 0;

    /* Read touch from I2C interface */
    i2c_lock(FT_CHN);
    if(ret = i2c_write(FT_CHN, FT_ADDR, I2C_CONTINUE, 1, &buf[0]))
        goto _end;
    if(ret = i2c_read(FT_CHN, FT_ADDR, I2C_RESTART|I2C_STOP, 5, &buf[0]))
        goto _end;
  _end:
    i2c_unlock(FT_CHN);
    if(ret)
        return 0;

    /* Check touch is actually present */
    if(buf[0] == 0)
        return 0;

    /* Parse coordinate values */
    int evt = buf[1] >> 6;
    int tx = buf[2] | ((buf[1] & 0xf) << 8);
    int ty = buf[4] | ((buf[3] & 0xf) << 8);

    /* Save latest touch */
    ft_last_evt = evt;
    ft_last_x = tx;
    ft_last_y = ty;

    /* Report the touched button */
    if(evt == 0 || evt == 2)
        return ft_touch_to_button(tx, ty);
    else
        return 0;
}

static void ft_write(int reg, int val)
{
    unsigned char buf[2] = {reg, val};
    i2c_lock(FT_CHN);
    i2c_write(FT_CHN, FT_ADDR, I2C_STOP, 2, &buf[0]);
    i2c_unlock(FT_CHN);
}

static void ft_set_active(bool en)
{
    if(ft_active == en)
        return;

    if(en) {
        /* Active mode */
        ft_write(0xa5, 0);
    } else {
        /* Hibernate mode */
        ft_write(0xa5, 3);
        ft_state = 0;
    }

    ft_active = en;
}

static void ft_thread(void)
{
    while(1) {
        semaphore_wait(&ft_ready, TIMEOUT_BLOCK);
        ft_last_int = current_tick;
        ft_state = ft_read_touch();
    }
}

static void ft_init(void)
{
    /* Start driver thread */
    semaphore_init(&ft_ready, 1, 0);
    create_thread(ft_thread, ft_stack, sizeof(ft_stack), 0,
                  ft_thread_name IF_PRIO(, PRIORITY_SYSTEM)
                  IF_COP(, CPU));

    /* Bring up I2C bus */
    i2c_set_freq(FT_CHN, I2C_FREQ_400K);

    /* Reset chip */
    gpio_config(GPIO_B, FT_RST_PIN|FT_INT_PIN, GPIO_OUTPUT(0));
    mdelay(5);
    gpio_out_level(GPIO_B, FT_RST_PIN, 1);
    gpio_config(GPIO_B, FT_INT_PIN, GPIO_IRQ_EDGE(0));
    gpio_enable_irq(GPIO_B, FT_INT_PIN);

    /* Set active state */
    ft_active = true;
}

void ft_interrupt(void)
{
    /* Signal thread to wake and read data */
    semaphore_release(&ft_ready);
}

/* Rockbox interface */
void button_init_device(void)
{
    /* Configure physical button GPIOs */
    gpio_config(GPIO_A, (1 << 17) | (1 << 19), GPIO_INPUT);
    gpio_config(GPIO_B, (1 << 28) | (1 << 31), GPIO_INPUT);

    /* Initialize touchpad */
    ft_init();
}

void touchpad_enable_device(bool en)
{
    ft_set_active(en);
}

int button_read_device(void)
{
    /* Copy over touchpad button state */
    int r = ft_state;

    /* Read GPIOs for physical buttons */
    unsigned a = REG_GPIO_PIN(GPIO_A);
    unsigned b = REG_GPIO_PIN(GPIO_B);

    /* All buttons are active low */
    if((a & (1 << 17)) == 0) r |= BUTTON_PLAY;
    if((a & (1 << 19)) == 0) r |= BUTTON_VOL_UP;
    if((b & (1 << 28)) == 0) r |= BUTTON_VOL_DOWN;
    if((b & (1 << 31)) == 0) r |= BUTTON_POWER;

    return r;
}

bool headphones_inserted()
{
    /* TODO: implement this. The headphone presence detect is writed to an
     * undocumented GPIO on the AXP173 -- or at least it was undocumented on
     * the datasheets I managed to dig up. The same GPIO registers are on
     * the very similar AXP192 chip though.
     *
     * From FiiO's kernel source, the GPIO is reg 0x94 bit 6, active high.
     */
    return true;
}
