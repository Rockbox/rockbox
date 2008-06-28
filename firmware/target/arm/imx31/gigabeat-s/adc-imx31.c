/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
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
#include "config.h"
#include "system.h"
#include "mc13783.h"
#include "adc-target.h"
#include "kernel.h"

/* Do this so we may read all channels in a single SPI message */
static const unsigned char reg_array[4] =
{
    MC13783_ADC2,
    MC13783_ADC2,
    MC13783_ADC2,
    MC13783_ADC2,
};

static uint32_t channels[2][4];
static struct wakeup adc_wake;
static struct mutex adc_mtx;
static long last_adc_read[2]; /* One for each input group */

/* Read 10-bit ADC channel */
unsigned short adc_read(int channel)
{
    uint32_t data;
    int input_select;

    if ((unsigned)channel >= NUM_ADC_CHANNELS)
        return ADC_READ_ERROR;

    input_select = channel >> 3;

    mutex_lock(&adc_mtx);

    /* Limit the traffic through here */
    if (current_tick != last_adc_read[input_select])
    {
        /* Keep enable, start conversion, increment from channel 0,
         * increment from channel 4 */
        uint32_t adc1 = MC13783_ADEN | MC13783_ASC | MC13783_ADA1w(0) |
                        MC13783_ADA2w(4);

        if (input_select == 1)
            adc1 |= MC13783_ADSEL; /* 2nd set of inputs */

        /* Start conversion */
        mc13783_write(MC13783_ADC1, adc1);

        /* Wait for done signal */
        wakeup_wait(&adc_wake, TIMEOUT_BLOCK);

        /* Read all 8 channels that are converted - two channels in each
         * word. */
        mc13783_read_regset(reg_array, channels[input_select], 4);

        last_adc_read[input_select] = current_tick;
    }

    data = channels[input_select][channel & 3];

    mutex_unlock(&adc_mtx);

    /* Channels 0-3/8-11 in ADD1, 4-7/12-15 in ADD2 */
    return (channel & 4) ? MC13783_ADD2r(data) : MC13783_ADD1r(data);
}

/* Called by mc13783 interrupt thread when conversion is complete */
void adc_done(void)
{
    wakeup_signal(&adc_wake);
}

void adc_init(void) 
{
    wakeup_init(&adc_wake);
    mutex_init(&adc_mtx);

    /* Init so first reads get data */
    last_adc_read[0] = last_adc_read[1] = current_tick-1;

    /* Enable increment-by-read, thermistor, charge current */
    mc13783_write(MC13783_ADC0, MC13783_ADINC2 | MC13783_ADINC1 |
                  MC13783_RTHEN | MC13783_CHRGICON);
    /* Enable ADC, set multi-channel mode */
    mc13783_write(MC13783_ADC1, MC13783_ADEN);

    /* Enable ADCDONE event */
    mc13783_write(MC13783_INTERRUPT_STATUS0, MC13783_ADCDONEI);
    mc13783_enable_event(MC13783_ADCDONE_EVENT);
}
