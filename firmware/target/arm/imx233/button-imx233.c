/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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

#include <stdlib.h>

#include "cpu.h"
#include "kernel.h"
#include "button-imx233.h"
#include "lradc-imx233.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"
#include "backlight.h"

static int delay_chan = -1; /* delay channel used to trigger */
static int raw_val[LRADC_NUM_CHANNELS]; /* channel values sampled (last two) */
static int chan_mask; /* trigger channel mask */
static int irq_chan_mask; /* triggered channel mask */
static int src_map[LRADC_NUM_SOURCES]; /* physical -> virtual channel map */
static int src_mask; /* sampled source mask */
#ifdef HAS_BUTTON_HOLD
static int hold_idx = -1; /* index of hold button in map */
#endif
#ifdef HAVE_HEADPHONE_DETECTION
static int jack_idx = -1; /* index of jack detect in map */
#endif

/* LRADC margin for buttons */
#ifndef IMX233_BUTTON_LRADC_MARGIN
#define IMX233_BUTTON_LRADC_MARGIN  30
#endif

/* shortcut of button map */
#define MAP imx233_button_map

/* sample rate for LRADC */
#define RATE    HZ
/* number of samples per irq */
#define SAMPLES    10
/* delay's delay */
#define DELAY   (LRADC_DELAY_FREQ / RATE / SAMPLES)

/* correct value for channel with builtin dividers */
static int correct_lradc(int src, int raw)
{
    if(src == LRADC_SRC_VDDIO)
        return raw * 2;
    else
        return raw;
}

/* return raw value for the button */
int imx233_button_read_raw(int idx)
{
    if(MAP[idx].periph == IMX233_BUTTON_GPIO)
        return imx233_pinctrl_get_gpio(MAP[idx].u.gpio.bank, MAP[idx].u.gpio.pin);
    else if(MAP[idx].periph == IMX233_BUTTON_LRADC)
        return correct_lradc(MAP[idx].u.lradc.src, raw_val[src_map[MAP[idx].u.lradc.src]]);
    else if(MAP[idx].periph == IMX233_BUTTON_PSWITCH)
        return imx233_power_read_pswitch();
    else
        return -1;
}

/* return cooked (interpreted) value for the button, ignoring debouncing */
static bool imx233_button_read_cooked(int idx)
{
    int raw = imx233_button_read_raw(idx);
    bool res;
    if(MAP[idx].periph == IMX233_BUTTON_GPIO)
    {
        res = raw;
    }
    else if(MAP[idx].periph == IMX233_BUTTON_LRADC)
    {
        /* correct value in relative mode */
        int rel = MAP[idx].u.lradc.relative;
        if(rel != -1)
            raw = (raw * MAP[rel].u.lradc.value) / imx233_button_read_raw(rel);
        switch(MAP[idx].u.lradc.op)
        {
            case IMX233_BUTTON_EQ:
                res = abs(raw - MAP[idx].u.lradc.value) <= MAP[idx].u.lradc.margin;
                break;
            case IMX233_BUTTON_GT:
                res = raw > MAP[idx].u.lradc.value;
                break;
            case IMX233_BUTTON_LT:
                res = raw < MAP[idx].u.lradc.value;
                break;
            default:
                res = false;
                break;
        }
    }
    else if(MAP[idx].periph == IMX233_BUTTON_PSWITCH)
    {
        res = raw == MAP[idx].u.pswitch.level;
    }
    else
        res = false;
    /* handle inversion */
    if(MAP[idx].flags & IMX233_BUTTON_INVERTED)
        res = !res;
    return res;
}

/* finish round */
static void do_round(void)
{
    for(int i = 0; MAP[i].btn != IMX233_BUTTON_END; i++)
    {
        bool cooked = imx233_button_read_cooked(i);
        if(MAP[i].last_val == cooked)
            MAP[i].rounds = MIN(MAP[i].rounds + 1, MAP[i].threshold);
        else
            MAP[i].rounds = 1;
        MAP[i].last_val = cooked;
    }
}

/* process IRQ */
static void button_lradc_irq(int chan)
{
    /* read value */
    raw_val[chan] = imx233_lradc_read_channel(chan) / SAMPLES;
    imx233_lradc_clear_channel(chan);
    imx233_lradc_setup_sampling(chan, true, SAMPLES - 1);
    /* record irq, trigger delay if all IRQs have been fired */
    irq_chan_mask |= 1 << chan;
    if(irq_chan_mask == chan_mask)
    {
        irq_chan_mask = 0;
        do_round();
        imx233_lradc_setup_delay(delay_chan, chan_mask, 0, SAMPLES - 1, DELAY);
        imx233_lradc_kick_delay(delay_chan);
    }
}

bool imx233_button_read_btn(int idx)
{
    return MAP[idx].rounds >= MAP[idx].threshold ? MAP[idx].last_val : false;
}

int imx233_button_read(int others)
{
    int res = others;
#ifdef HAS_BUTTON_HOLD
    if(imx233_button_read_hold())
        return 0;
#endif
    for(int i = 0; MAP[i].btn != IMX233_BUTTON_END; i++)
    {
        if(MAP[i].btn >= 0 && imx233_button_read_btn(i))
            res |= MAP[i].btn;
    }
    return res;
}

#ifdef HAS_BUTTON_HOLD
bool imx233_button_read_hold(void)
{
    return imx233_button_read_btn(hold_idx);
}

bool button_hold(void)
{
    bool hold_button = imx233_button_read_hold();
#ifndef BOOTLOADER
    static bool hold_button_old = false;
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */
    return hold_button;
}
#endif

#ifdef HAVE_HEADPHONE_DETECTION
bool imx233_button_read_jack(void)
{
    return imx233_button_read_btn(jack_idx);
}

bool headphones_inserted(void)
{
    return imx233_button_read_jack();
}
#endif

/* return number of debouncing rounds by type of peripheral */
static int threshold_by_periph(int periph)
{
    if(periph == IMX233_BUTTON_GPIO) return 1; // no debouncing
    if(periph == IMX233_BUTTON_LRADC) return 2; // 2 times at HZ gives ~10 ms hold
    if(periph == IMX233_BUTTON_PSWITCH) return 10; // PSWITCH is very slow to ramp
    return 1; // other ?
}

void imx233_button_init(void)
{
    /* go through the table and init stuff which needs to be */
    for(int i = 0; MAP[i].btn != IMX233_BUTTON_END; i++)
    {
        MAP[i].threshold = threshold_by_periph(MAP[i].periph);
        if(MAP[i].periph == IMX233_BUTTON_GPIO)
        {
            imx233_pinctrl_acquire(MAP[i].u.gpio.bank, MAP[i].u.gpio.pin, MAP[i].name);
            imx233_pinctrl_set_function(MAP[i].u.gpio.bank, MAP[i].u.gpio.pin, PINCTRL_FUNCTION_GPIO);
            imx233_pinctrl_enable_gpio(MAP[i].u.gpio.bank, MAP[i].u.gpio.pin, false);
            bool pullup = !!(MAP[i].flags & IMX233_BUTTON_PULLUP);
            imx233_pinctrl_enable_pullup(MAP[i].u.gpio.bank, MAP[i].u.gpio.pin, pullup);
        }
        else if(MAP[i].periph == IMX233_BUTTON_LRADC)
        {
            /* use default value for margin */
            if(MAP[i].u.lradc.margin == 0)
                MAP[i].u.lradc.margin = IMX233_BUTTON_LRADC_MARGIN;
            int src = MAP[i].u.lradc.src;
            /* if channel was already acquired, there is nothing to do */
            if(src_mask & (1 << src))
                continue;
            src_map[src] = imx233_lradc_acquire_channel(LRADC_SRC(src), TIMEOUT_NOBLOCK);
            if(src_map[src] < 0)
                panicf("Cannot get channel for %s", MAP[i].name);
            imx233_lradc_setup_source(src_map[src], true, src);
            imx233_lradc_setup_sampling(src_map[src], true, SAMPLES - 1);
            imx233_lradc_enable_channel_irq(src_map[src], true);
            imx233_lradc_set_channel_irq_callback(src_map[src], button_lradc_irq);
            src_mask |= 1 << src;
            chan_mask |= 1 << src_map[src];
        }
#ifdef HAS_BUTTON_HOLD
        if(MAP[i].btn == IMX233_BUTTON_HOLD)
            hold_idx = i;
#endif
#ifdef HAVE_HEADPHONE_DETECTION
        if(MAP[i].btn == IMX233_BUTTON_JACK)
            jack_idx = i;
#endif
    }
#ifdef HAS_BUTTON_HOLD
    if(hold_idx == -1)
        panicf("No hold entry found");
#endif
#ifdef HAVE_HEADPHONE_DETECTION
    if(jack_idx == -1)
        panicf("No jack entry found");
#endif
    /* create delay channel if necessary
     * NOTE other buttons are polled as part of the delay irq processing */
    if(src_mask != 0)
    {
        delay_chan = imx233_lradc_acquire_delay(TIMEOUT_NOBLOCK);
        if(delay_chan < 0)
            panicf("Cannot get delay channel");
        imx233_lradc_setup_delay(delay_chan, chan_mask, 0, SAMPLES - 1, DELAY);
        imx233_lradc_kick_delay(delay_chan);
    }
    /* otherwise we need to regularly poll for other buttons */
    else
        tick_add_task(do_round);
}
