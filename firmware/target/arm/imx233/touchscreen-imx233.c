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
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include "touchscreen-imx233.h"
#include "stdlib.h"

/* Description:
 * the driver basically has 2 modes:
 * - wait mode: use the hardware touch detect mechanism to wait for an edge
 * - measurement mode: use pull up/down and adc to measure X/Y
 * In measurement mode, we start by measuring X then Y then we check if
 * the hardware still detect a touch. When no touch is detected anymore, we
 * go back to wait mode.
 * For each axis, we handle stabilization by repeatedly measuring the position
 * until it is stable. We consider a set of measures stable when
 * no measure is further away from the average than DEBOUNCE_THRESHOLD and
 * we have at least SAMPLES_THRESHOLD measures. This avoids using magic
 * timing constants specific to the touchscreen stabilization time and
 * it is much more robust.
 *
 * Inspired by linux touchscreen driver for the stmp37xx.
 */

enum touch_state_t
{
    TOUCH_STATE_WAIT, /* wait for touch */
    TOUCH_STATE_MEASURE_X, /* measure x */
    TOUCH_STATE_MEASURE_Y, /* measure y */
    TOUCH_STATE_VERIFY, /* verify touch */
};

#define DEBOUNCE_THRESHOLD  100
#define SAMPLES_THRESHOLD   4

static enum touch_state_t touch_state;
static int touch_chan = -1;
static int touch_delay = -1;
static int touch_x, touch_y, nr_samples;
/* once a touch is confirmed, the parameters are copied to these value for
 * instant readout by button code. */
static bool old_touch_detect = false;
static int old_touch_x, old_touch_y;

static void kick_measure(bool pull_x, bool pull_y, bool detect, int lradc_phys)
{
    /* setup measurement: x- pull down and x+ pull up */
    imx233_lradc_setup_touch(pull_x, pull_y, pull_x, pull_y, detect);
    imx233_lradc_enable_touch_detect_irq(false);
    imx233_lradc_enable_channel_irq(touch_chan, true);
    /* measure channel, no accumulation */
    imx233_lradc_setup_channel(touch_chan, false, false, 0, lradc_phys);
    /* use a delay */
    imx233_lradc_setup_delay(touch_delay, 1 << touch_chan, 0, 0, 8);
    imx233_lradc_kick_delay(touch_delay);
}

static void enter_state(enum touch_state_t state)
{
    touch_state = state;
    switch(state)
    {
        case TOUCH_STATE_WAIT:
            imx233_lradc_setup_touch(false, false, false, false, true);
            imx233_lradc_enable_channel_irq(touch_chan, false);
            imx233_lradc_enable_touch_detect_irq(true);
            break;
        case TOUCH_STATE_MEASURE_X:
            kick_measure(true, false, false, HW_LRADC_CHANNEL_YPLUS);
            break;
        case TOUCH_STATE_MEASURE_Y:
            kick_measure(false, true, false, HW_LRADC_CHANNEL_XPLUS);
            break;
        case TOUCH_STATE_VERIFY:
            kick_measure(false, false, true, HW_LRADC_CHANNEL_YPLUS);
            break;
    }
}

static void process(void)
{
    int val;
    switch(touch_state)
    {
        case TOUCH_STATE_WAIT:
            /* a touch has happened, check if it's still valid */
            if(imx233_lradc_read_touch_detect())
                enter_state(TOUCH_STATE_MEASURE_X);
            else
            {
                old_touch_detect = false;
                /* clear detect interrupt */
                imx233_lradc_clear_touch_detect_irq();
            }
            break;
        case TOUCH_STATE_MEASURE_X:
            /* read value */
            val = imx233_lradc_read_channel(touch_chan);
            /* if value is too far from average, restart */
            if(nr_samples > 0 && abs(val - touch_x) > DEBOUNCE_THRESHOLD)
                nr_samples = 0;
            touch_x = (touch_x * nr_samples + val) / (nr_samples + 1);
            nr_samples++;
            /* if we have enough samples, measure Y */
            if(nr_samples > SAMPLES_THRESHOLD)
                enter_state(TOUCH_STATE_MEASURE_Y);
            else
                imx233_lradc_kick_delay(touch_delay);
            break;
        case TOUCH_STATE_MEASURE_Y:
            /* read value */
            val = imx233_lradc_read_channel(touch_chan);
            /* if value is too far from average, restart */
            if(nr_samples > 0 && abs(val - touch_y) > DEBOUNCE_THRESHOLD)
                nr_samples = 0;
            touch_y = (touch_y * nr_samples + val) / (nr_samples + 1);
            nr_samples++;
            /* if we have enough samples, verify touch */
            if(nr_samples > SAMPLES_THRESHOLD)
                enter_state(TOUCH_STATE_VERIFY);
            else
                imx233_lradc_kick_delay(touch_delay);
            break;
        case TOUCH_STATE_VERIFY:
            if(imx233_lradc_read_touch_detect())
            {
                old_touch_detect = true;
                old_touch_x = touch_x;
                old_touch_y = touch_y;
                enter_state(TOUCH_STATE_MEASURE_X);
            }
            else
            {
                old_touch_detect = false;
                enter_state(TOUCH_STATE_WAIT);
            }
            break;
    }
}

void INT_TOUCH_DETECT(void)
{
    process();
}

static void touch_channel_irq(int chan)
{
    (void) chan;
    process();
}

void imx233_touchscreen_init(void)
{
    touch_chan = imx233_lradc_acquire_channel(TIMEOUT_NOBLOCK);
    touch_delay = imx233_lradc_acquire_delay(TIMEOUT_NOBLOCK);
    if(touch_chan < 0 || touch_delay < 0)
        panicf("Cannot acquire channel and delays for touchscreen measurement");
    imx233_touchscreen_enable(false);
}

void imx233_touchscreen_enable(bool enable)
{
    enter_state(TOUCH_STATE_WAIT);
    imx233_lradc_set_channel_irq_callback(touch_chan, &touch_channel_irq);
    imx233_icoll_enable_interrupt(INT_SRC_LRADC_CHx(touch_chan), enable);
    imx233_icoll_enable_interrupt(INT_SRC_TOUCH_DETECT, enable);
}

bool imx233_touchscreen_get_touch(int *raw_x, int *raw_y)
{
    if(raw_x) *raw_x = old_touch_x;
    if(raw_y) *raw_y = old_touch_y;
    return old_touch_detect;
}
