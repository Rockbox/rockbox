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
#include "button-lradc-imx233.h"
#include "stdlib.h"
#include "lradc-imx233.h"

#ifndef IMX233_BUTTON_LRADC_CHANNEL
#error You must define IMX233_BUTTON_LRADC_CHANNEL to use button-lradc
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

static int button_delay;
static int button_chan;
static int button_val[2];
static int button_idx;
static int button_mask;
static int table_size;

static int button_find(int val)
{
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
    // choose best between i-1 and i (note that table[i-1]<=val<=table[i]) */
    else if(val - table[i - 1].adc_val < table[i].adc_val - val)
        btn = table[i - 1].btn;
    else
        btn = table[i].btn;
    return btn;
}

int button_value;

static void button_lradc_irq(int chan)
{
    (void) chan;
    /* read value, kick channel */
    button_val[button_idx] = imx233_lradc_read_channel(button_chan) / SAMPLES;
    imx233_lradc_clear_channel(button_chan);
    imx233_lradc_setup_channel(button_chan, true, true, SAMPLES - 1, LRADC_SRC(CHAN));
    imx233_lradc_setup_delay(button_delay, 1 << button_chan, 0, SAMPLES - 1, DELAY);
    imx233_lradc_kick_delay(button_delay);
    button_value = button_val[button_idx];
    /* compute mask, compare to previous one */
    button_val[button_idx] = button_find(button_val[button_idx]);
    button_idx = 1 - button_idx;
    if(button_val[0] == button_val[1])
        button_mask = button_val[0];
}

void imx233_button_lradc_init(void)
{
    button_chan = imx233_lradc_acquire_channel(LRADC_SRC(CHAN), TIMEOUT_NOBLOCK);
    if(button_chan < 0)
        panicf("Cannot get channel for button-lradc");
    button_delay = imx233_lradc_acquire_delay(TIMEOUT_NOBLOCK);
    if(button_delay < 0)
        panicf("Cannot get delay for button-lradc");
    imx233_lradc_setup_channel(button_chan, true, true, SAMPLES - 1, LRADC_SRC(CHAN));
    imx233_lradc_setup_delay(button_delay, 1 << button_chan, 0, SAMPLES - 1, DELAY);
    imx233_lradc_enable_channel_irq(button_chan, true);
    imx233_lradc_set_channel_irq_callback(button_chan, button_lradc_irq);
    imx233_lradc_kick_delay(button_delay);

    table_size = 0;
    while(imx233_button_lradc_mapping[table_size].btn != IMX233_BUTTON_LRADC_END)
        table_size++;
}

bool imx233_button_lradc_hold(void)
{
    return button_mask == IMX233_BUTTON_LRADC_HOLD;
}

int imx233_button_lradc_read(void)
{
    return button_mask == IMX233_BUTTON_LRADC_HOLD ? 0 : button_mask;
}
