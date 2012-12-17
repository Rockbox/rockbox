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
 * as published by the Free Software Foundation; either version 1
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

unsigned short adc_read(int channel)
{
    unsigned short result;

    /* ungate lsadc clocks */
    SCU_CLKCFG &= ~(CLKCFG_LSADC|CLKCFG_PCLK_LSADC);

    /* wait a bit for clock to stabilize */
    udelay(10);

    ADC_CTRL = (1<<4)|(1<<3) | (channel & (NUM_ADC_CHANNELS - 1));

    /* Wait for conversion ready.
     * The doc says one should pool ADC_STAT for end of conversion
     * or setup interrupt. Neither of these two methods work as
     * advertised.
     *
     * ~10us should be enough so we wait 20us to be on the safe side
     */
    udelay(20);

    /* 10bits result */
    result = (ADC_DATA & 0x3ff);

    /* turn off lsadc clock when not in use */
    SCU_CLKCFG |= (CLKCFG_LSADC|CLKCFG_PCLK_LSADC);

    return result; 
}

void adc_init(void)
{
    /* ADC clock divider to reach max 1MHz */
    SCU_DIVCON1 = (SCU_DIVCON1 & ~(0xff<<10)) | (49<<10);
}
