/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Marcin Bukat
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

unsigned int adc_scan(int channel)
{
    ADC_CTRL = (1<<4) | (1<<3) | (channel & (NUM_ADC_CHANNELS - 1));

    /* wait for conversion ready ~10us */    
    while (ADC_STAT & 0x01);

    /* 10bits result */
    return (ADC_DATA & 0x3ff);
}

void adc_init(void)
{
    /* ADC clock divider to reach max 1MHz */
    SCU_DIVCON1 = (SCU_DIVCON1 & ~(0xff<<10)) | (49<<10);

    /* enable clocks for ADC */
    SCU_CLKCFG |= 3<<23;
}
