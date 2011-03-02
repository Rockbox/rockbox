/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bertrik Sikken
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

#include "inttypes.h"
#include "s5l8700.h"
#include "adc.h"
#include "adc-target.h"
#include "kernel.h"

/*  Driver for the built-in ADC of the s5l8700

    The ADC is put into standby mode after each conversion.
 */

#define ADC_PRESCALER   100     /* 1MHz ADC clock, assuming 100 MHz PCLK */
#define ADC_DEFAULT (1<<14) |                   /* prescale enable */   \
                    ((ADC_PRESCALER-1)<<6) |    /* prescaler */         \
                    (0<<3) |                    /* sel_mux */           \
                    (0<<2) |                    /* stdbm */             \
                    (0<<1) |                    /* read_start */        \
                    (0<<0)                      /* enable start */      \


static struct mutex adc_mtx;
static struct semaphore adc_wakeup;

void INT_ADC(void)
{
    semaphore_release(&adc_wakeup);
}

unsigned short adc_read(int channel)
{
    unsigned short data;

    mutex_lock(&adc_mtx);
    
    /* set channel and start conversion */
    ADCCON = ADC_DEFAULT | 
             (channel << 3) |
             (1 << 0);          /* enable start */
    
    /* wait for conversion */
    semaphore_wait(&adc_wakeup, TIMEOUT_BLOCK);
    
    /* get the converted data */
    data = ADCDAT0 & 0x3FF;
    
    /* put ADC back into standby */
    ADCCON |= (1 << 2);

    mutex_unlock(&adc_mtx);
    
    return data;
}

void adc_init(void)
{
    mutex_init(&adc_mtx);
    semaphore_init(&adc_wakeup, 1, 0);

    /* enable clock to ADC */
    PWRCON &= ~(1 << 10);
    
    /* configure ADC and put in standby */
    ADCCON = ADC_DEFAULT | (1 << 2);
    
    /* configure conversion delay (use default) */
    ADCDLY = 0xFF;
    
    /* enable interrupt */
    INTMSK |= (1 << 31);
}

