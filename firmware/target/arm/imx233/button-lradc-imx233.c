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
#include "button-lradc-imx233.h"
#include "lradc-imx233.h"
#include "pinctrl-imx233.h"

#ifndef IMX233_BUTTON_LRADC_CHANNEL
# error You must define IMX233_BUTTON_LRADC_CHANNEL to use button-lradc
#endif

#if defined(HAS_BUTTON_HOLD) && !defined(IMX233_BUTTON_LRADC_HOLD_DET)
# error You must defined IMX233_BUTTON_LRADC_HOLD_DET if you use hold
#endif

#if defined(IMX233_BUTTON_LRADC_HOLD_DET) && \
    IMX233_BUTTON_LRADC_HOLD_DET == BLH_GPIO && \
    (!defined(BLH_GPIO_BANK) || !defined(BLH_GPIO_PIN))
# error You must define BLH_GPIO_BANK and BLH_GPIO_PIN when detecting hold using GPIO
#endif

/* physical channel */
#define CHAN    IMX233_BUTTON_LRADC_CHANNEL
/* number of irq per second */
#ifdef IMX233_BUTTON_LRADC_RATE
# define RATE    IMX233_BUTTON_LRADC_RATE
#else
# define RATE    HZ
#endif
/* number of samples per irq */
#ifdef IMX233_BUTTON_LRADC_SAMPLES
# define SAMPLES    IMX233_BUTTON_LRADC_SAMPLES
#else
# define SAMPLES    10
#endif
/* delay's delay */
#define DELAY   (LRADC_DELAY_FREQ / RATE / SAMPLES)

#ifdef IMX233_BUTTON_LRADC_VDDIO
#define HAS_VDDIO
#endif

static int button_delay;
static int button_chan;
static int button_val[2];
static int button_idx;
static int button_mask;
static int table_size;
static int raw_val;
#ifdef HAS_VDDIO
static int vddio_chan;
static int vddio_val;
#endif
static int delay_chan_mask; // trigger channel mask
static int irq_chan_mask; // triggered channel mask

static int button_find(int val)
{
#ifdef IMX233_BUTTON_LRADC_VDDIO
        val = (val * IMX233_BUTTON_LRADC_VDDIO) / vddio_val;
#endif
    // shortcuts
    struct imx233_button_lradc_mapping_t *table = imx233_button_lradc_mapping;
    /* FIXME use a dichotomy */
    int i = 0;
    while(i < table_size && val >= table[i].adc_val)
        i++;
    // extreme cases
    int btn = 0;
    // if i=n then choose i-1 and otherwise choose best between i-1 and i
    if(i == 0)
        btn = table[0].btn;
    else if(i == table_size)
        btn = table[i - 1].btn;
    // choose best between i-1 and i (note that table[i-1]<=val<table[i]) */
    else if(val - table[i - 1].adc_val < table[i].adc_val - val)
        btn = table[i - 1].btn;
    else
        btn = table[i].btn;
    return btn;
}

static void button_lradc_irq(int chan)
{
    /* read value, clear channel */
#ifdef HAS_VDDIO
    if(chan == vddio_chan)
    {
        vddio_val = imx233_lradc_read_channel(vddio_chan) / SAMPLES;
        vddio_val *= 2; /* VDDIO channel has internal divider */
        imx233_lradc_clear_channel(vddio_chan);
        imx233_lradc_setup_channel(vddio_chan, true, true, SAMPLES - 1, LRADC_SRC_VDDIO);
    }
#endif
    if(chan == button_chan)
    {
        raw_val = imx233_lradc_read_channel(button_chan) / SAMPLES;
        imx233_lradc_clear_channel(button_chan);
        imx233_lradc_setup_channel(button_chan, true, true, SAMPLES - 1, LRADC_SRC(CHAN));
    }
    /* record irq, trigger delay if all IRQs have been fired */
    irq_chan_mask |= 1 << chan;
    if(irq_chan_mask == delay_chan_mask)
    {
        irq_chan_mask = 0;
        imx233_lradc_setup_delay(button_delay, delay_chan_mask, 0, SAMPLES - 1, DELAY);
        imx233_lradc_kick_delay(button_delay);
        /* compute mask, compare to previous one */
        button_val[button_idx] = button_find(raw_val);
        button_idx = 1 - button_idx;
        if(button_val[0] == button_val[1])
            button_mask = button_val[0];
    }
}

void imx233_button_lradc_init(void)
{
    table_size = 0;
    while(imx233_button_lradc_mapping[table_size].btn != IMX233_BUTTON_LRADC_END)
        table_size++;

    button_chan = imx233_lradc_acquire_channel(LRADC_SRC(CHAN), TIMEOUT_NOBLOCK);
    if(button_chan < 0)
        panicf("Cannot get channel for button-lradc");
    button_delay = imx233_lradc_acquire_delay(TIMEOUT_NOBLOCK);
    if(button_delay < 0)
        panicf("Cannot get delay for button-lradc");
    imx233_lradc_setup_channel(button_chan, true, true, SAMPLES - 1, LRADC_SRC(CHAN));
    imx233_lradc_enable_channel_irq(button_chan, true);
    imx233_lradc_set_channel_irq_callback(button_chan, button_lradc_irq);
    delay_chan_mask = 1 << button_chan;
#ifdef HAS_VDDIO
    vddio_chan = imx233_lradc_acquire_channel(LRADC_SRC_VDDIO, TIMEOUT_NOBLOCK);
    if(vddio_chan < 0)
        panicf("Cannot get vddio channel for button-lradc");
    imx233_lradc_setup_channel(vddio_chan, true, true, SAMPLES - 1, LRADC_SRC_VDDIO);
    imx233_lradc_enable_channel_irq(vddio_chan, true);
    imx233_lradc_set_channel_irq_callback(vddio_chan, button_lradc_irq);
    delay_chan_mask |= 1 << vddio_chan;
#endif
    imx233_lradc_setup_delay(button_delay, delay_chan_mask, 0, SAMPLES - 1, DELAY);
    imx233_lradc_kick_delay(button_delay);
#if defined(HAS_BUTTON_HOLD) && IMX233_BUTTON_LRADC_HOLD_DET == BLH_GPIO
    imx233_pinctrl_acquire(BLH_GPIO_BANK, BLH_GPIO_PIN, "button_lradc_hold");
    imx233_pinctrl_set_function(BLH_GPIO_BANK, BLH_GPIO_PIN, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(BLH_GPIO_BANK, BLH_GPIO_PIN, false);
# ifdef BLH_GPIO_PULLUP
    imx233_pinctrl_enable_pullup(BLH_GPIO_BANK, BLH_GPIO_PIN, true);
# endif
#endif
}

#if defined(HAS_BUTTON_HOLD) && IMX233_BUTTON_LRADC_HOLD_DET == BLH_ADC
bool imx233_button_lradc_hold(void)
{
    return button_mask == IMX233_BUTTON_LRADC_HOLD;
}
#endif


#if defined(HAS_BUTTON_HOLD) && IMX233_BUTTON_LRADC_HOLD_DET == BLH_GPIO
bool imx233_button_lradc_hold(void)
{
    bool res = imx233_pinctrl_get_gpio(BLH_GPIO_BANK, BLH_GPIO_PIN);
#ifdef BLH_GPIO_INVERTED
    res = !res;
#endif
    return res;
}
#endif

int imx233_button_lradc_read(int others)
{
#ifdef HAS_BUTTON_HOLD
    return imx233_button_lradc_hold() ? 0 : button_mask | others;
#else
    return button_mask | others;
#endif
}

int imx233_button_lradc_read_raw(void)
{
    return raw_val;
}

#ifdef HAS_VDDIO
int imx233_button_lradc_read_vddio(void)
{
    return vddio_val; // the VDDIO channel has an internal divider
}
#endif
