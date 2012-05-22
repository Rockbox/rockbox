/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "button-target.h"
#include "system.h"
#include "system-target.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"
#include "string.h"
#include "usb.h"
#include "backlight.h"
#include "mpr121.h"

static struct mpr121_config_t config =
{
    .ele =
    {
        [0] = {.tth = 5, .rth = 4 },
        [1] = {.tth = 5, .rth = 4 },
        [2] = {.tth = 5, .rth = 4 },
        [3] = {.tth = 5, .rth = 4 },
        [4] = {.tth = 4, .rth = 3 },
        [5] = {.tth = 4, .rth = 3 },
        [6] = {.tth = 5, .rth = 4 },
        [7] = {.tth = 5, .rth = 4 },
        [8] = {.gpio = ELE_GPIO_OUTPUT_OPEN_LED },
    },
    .filters =
    {
        .ele =
        {
            .rising = {.mhd = 1, .nhd = 1, .ncl = 1, .fdl = 1 },
            .falling = {.mhd = 1, .nhd = 1, .ncl = 0xff, .fdl = 2 },
        }
    },
    .autoconf =
    {
        .en = true, .ren = true, .retry = RETRY_NEVER,
        .usl = 0xc4, .lsl = 0x7f, .tl = 0xb0
    },
    .ele_en = ELE_EN0_x(7),
    .cal_lock = CL_TRACK
};

#define MPR121_INTERRUPT    1

static int touchpad_btns = 0;
static long mpr121_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char mpr121_thread_name[] = "mpr121";
static struct event_queue mpr121_queue;

static void mpr121_irq_cb(int bank, int pin)
{
    (void) bank;
    (void) pin;
    /* the callback will not be fired until interrupt is enabled back so
     * the queue will not overflow or contain multiple MPR121_INTERRUPT events */
    queue_post(&mpr121_queue, MPR121_INTERRUPT, 0);
}

static void mpr121_thread(void)
{
    struct queue_event ev;

    while(1)
    {
        queue_wait(&mpr121_queue, &ev);
        /* handle usb connect and ignore all messages except rmi interrupts */
        if(ev.id == SYS_USB_CONNECTED)
        {
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            continue;
        }
        else if(ev.id != MPR121_INTERRUPT)
            continue;
        /* clear interrupt and get status */
        unsigned status;
        touchpad_btns = 0;
        if(!mpr121_get_touch_status(&status))
        {
            /* ELE3: up
             * ELE4: back
             * ELE5: menu
             * ELE6: down
             * ELE7: play */
            if(status & 0x8) touchpad_btns |= BUTTON_UP;
            if(status & 0x10) touchpad_btns |= BUTTON_BACK;
            if(status & 0x20) touchpad_btns |= BUTTON_MENU;
            if(status & 0x40) touchpad_btns |= BUTTON_DOWN;
            if(status & 0x80) touchpad_btns |= BUTTON_PLAY;
        }
        /* enable interrupt */
        imx233_setup_pin_irq(0, 18, true, true, false, &mpr121_irq_cb);
    }
}

/* B0P18 is #IRQ line of the touchpad */
void button_init_device(void)
{
    mpr121_init(0xb4);
    mpr121_soft_reset();
    mpr121_set_config(&config);
    
    queue_init(&mpr121_queue, true);
    create_thread(mpr121_thread, mpr121_stack, sizeof(mpr121_stack), 0,
        mpr121_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
    /* enable interrupt */
    imx233_pinctrl_acquire_pin(0, 18, "mpr121 int");
    imx233_set_pin_function(0, 18, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(0, 18, false);
    imx233_setup_pin_irq(0, 18, true, true, false, &mpr121_irq_cb);
    /* hold button */
    imx233_pinctrl_acquire_pin(0, 4, "hold");
    imx233_set_pin_function(0, 4, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(0, 4, false);
    /* volume down button */
    imx233_pinctrl_acquire_pin(2, 7, "volume down");
    imx233_set_pin_function(2, 7, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(2, 7, false);
    /* volume up button */
    imx233_pinctrl_acquire_pin(2, 8, "volume up");
    imx233_set_pin_function(2, 8, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(2, 8, false);
}

bool button_hold(void)
{
    /* B0P04: #hold */
    return !imx233_get_gpio_input_mask(0, 0x10);
}

int button_read_device(void)
{
    /* since sliding hold will usually trigger power, ignore power button
     * for one second after hold is released */
    static int power_ignore_counter = 0;
    static bool old_hold;
    /* light handling */
    bool hold = button_hold();
    if(hold != old_hold)
    {
        old_hold = hold;
#ifndef BOOTLOADER
        backlight_hold_changed(hold);
#endif /* BOOTLOADER */
        if(!hold)
            power_ignore_counter = HZ;
    }

    if(power_ignore_counter)
        power_ignore_counter--;
    
    int res = 0;
    /* B2P07: #volume-
     * B2P08: #volume+
     * PSWITCH: power */
    uint32_t mask = imx233_get_gpio_input_mask(2, 0x180);
    if(!(mask & 0x80))
        res |= BUTTON_VOL_DOWN;
    if(!(mask & 0x100))
        res |= BUTTON_VOL_UP;
    /* WARNING: it seems that the voltage on PSWITCH depends on whether
     * usb is connected or not ! Thus the value of this field can be 1 or 3 */
    if(__XTRACT(HW_POWER_STS, PSWITCH) != 0 && power_ignore_counter == 0)
        res |= BUTTON_POWER;
    return res | touchpad_btns;
}
