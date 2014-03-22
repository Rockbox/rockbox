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
#include "button-imx233.h"
#include "string.h"
#include "usb.h"
#include "backlight.h"
#include "mpr121.h"

#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3640), "vddio"), /* we need VDDIO for relative */
    IMX233_BUTTON_(HOLD, GPIO(0, 4), "jack", INVERTED),
    IMX233_BUTTON(VOL_DOWN, GPIO(2, 7), "vol_down", INVERTED),
    IMX233_BUTTON(VOL_UP, GPIO(2, 8), "vol_up", INVERTED),
    IMX233_BUTTON_(JACK, LRADC_REL(5, 3520, I_VDDIO), "jack"),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON_(END, END(), "")
};

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

static void mpr121_irq_cb(int bank, int pin, intptr_t user)
{
    (void) bank;
    (void) pin;
    (void) user;
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
        imx233_pinctrl_setup_irq(0, 18, true, true, false, &mpr121_irq_cb, 0);
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
    imx233_pinctrl_acquire(0, 18, "mpr121_int");
    imx233_pinctrl_set_function(0, 18, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 18, false);
    imx233_pinctrl_setup_irq(0, 18, true, true, false, &mpr121_irq_cb, 0);
    /* generic part */
    imx233_button_init();
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
        if(!hold)
            power_ignore_counter = HZ;
    }
    int res = imx233_button_read(touchpad_btns);
    if(power_ignore_counter > 0)
    {
        power_ignore_counter--;
        res &= ~BUTTON_POWER;
    }
    return res;
}
