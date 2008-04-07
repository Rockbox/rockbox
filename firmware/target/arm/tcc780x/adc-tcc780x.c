/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "string.h"
#include "adc.h"

static unsigned short adcdata[8];

#ifndef BOOTLOADER

/* Tick task */
static void adc_tick(void)
{
    int i;
    
    PCLK_ADC |= PCK_EN;   /* Enable ADC clock */
    
    /* Start converting the first 4 channels */
    for (i = 0; i < 4; i++)
        ADCCON = i;

}

/* IRQ handler */
void ADC(void)
{
    int num;
    uint32_t adc_status;

    do
    {
        adc_status = ADCSTATUS;
        num = (adc_status>>24) & 7;
        if (num) adcdata[(adc_status >> 16) & 0x7] = adc_status & 0x3ff;
    } while (num);

    PCLK_ADC &= ~PCK_EN;   /* Disable ADC clock */
}
#endif /* BOOTLOADER */


unsigned short adc_read(int channel)
{
#ifdef BOOTLOADER
    /* IRQs aren't enabled in the bootloader - just do the read directly */
    int i;
    uint32_t adc_status;

    PCLK_ADC |= PCK_EN;   /* Enable ADC clock */

    /* Start converting the first 4 channels */
    for (i = 0; i < 4; i++)
        ADCCON = i;

    /* Now read the values back */
    for (i=0; i < 4; i++)
    {
        /* Wait for data to become stable */
        while ((ADCDATA & 0x1) == 0);
        
        adc_status = ADCSTATUS;
        adcdata[(adc_status >> 16) & 0x7] = adc_status & 0x3ff;
    }

    PCLK_ADC &= ~PCK_EN;   /* Disable ADC clock */
#endif

    return adcdata[channel];
}

void adc_init(void)
{
    /* consider configuring PCK_ADC source here */
    
    ADCCON = (1<<4);         /* Enter standby mode */
    ADCCFG |= 0x00000003;    /* Single-mode, auto power-down */

#ifndef BOOTLOADER
    ADCCFG |= (1<<3);        /* Request IRQ on ADC completion */
    IEN |= ADC_IRQ_MASK;     /* Unmask ADC IRQs */
    
    tick_add_task(adc_tick);
    
    sleep(2);    /* Ensure valid readings when adc_init returns */
#endif
}
