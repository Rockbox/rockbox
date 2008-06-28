/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "adc.h"

static unsigned short adcdata[NUM_ADC_CHANNELS];

/* Scan ADC so that adcdata[channel] gets updated */
unsigned short adc_scan(int channel)
{
    unsigned int adc_data_1;
    unsigned int adc_data_2;

    /* Initialise */
    ADC_ADDR=0x130;
    ADC_STATUS=0;   /* 4 bytes, 1 per channel. Each byte is 0 if the channel is
                       off, 0x40 if the channel is on */
    
    /* Enable Channel */
    ADC_ADDR |= (0x1000000<<channel);
    
    /* Start? */
    ADC_ADDR |= 0x20000000;
    ADC_ADDR |= 0x80000000;
    
    /* ADC_DATA_1 and ADC_DATA_2 are both four bytes, one byte per channel.
       For each channel, ADC_DATA_1 stores the 8-bit msb, ADC_DATA_2 stores the
       2-bit lsb (in bits 0 and 1). Each channel is 10 bits total. */
    adc_data_1 = ((ADC_DATA_1 >> (8*channel)) & 0xff);
    adc_data_2 = ((ADC_DATA_2 >> (8*channel+6)) & 0x3);
    
    adcdata[channel] = (adc_data_1<<2 | adc_data_2);
    
    return adcdata[channel];
}

/* Read 10-bit channel data */
unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

static int adc_counter;

static void adc_tick(void)
{
    if(++adc_counter == HZ)
    {
        adc_counter = 0;
        adc_scan(ADC_BATTERY);
        adc_scan(ADC_UNKNOWN_1);
        adc_scan(ADC_UNKNOWN_2);
        adc_scan(ADC_SCROLLPAD);
    }
}

void adc_init(void)
{
    /* Enable ADC */
    ADC_ENABLE_ADDR |= ADC_ENABLE;
    
    /* Initialise */
    ADC_INIT=0;
    ADC_ADDR=0x130;
    ADC_STATUS=0;
    
    /* Enable Channels 1-4 */
    ADC_ADDR |= 0x1000000;
    ADC_ADDR |= 0x2000000;
    ADC_ADDR |= 0x4000000;
    ADC_ADDR |= 0x8000000;
    
    /* Start? */
    ADC_ADDR |= 0x20000000;
    ADC_ADDR |= 0x80000000;
    
    /* Wait 50ms for things to settle */
    sleep(HZ/20);
    
    tick_add_task(adc_tick);
}
