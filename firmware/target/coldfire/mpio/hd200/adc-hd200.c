/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Marcin Bukat
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

volatile unsigned short adc_data[NUM_ADC_CHANNELS] IBSS_ATTR;

/* Reading takes 4096 adclk ticks
 * 1) tick task is created that enables ADC interrupt
 * 2) On interrupt single channel is readed and
 *    ADC is prepared for next channel
 * 3) When all 4 channels are scanned ADC interrupt is disabled
 */

void ADC(void) __attribute__ ((interrupt_handler,section(".icode")));
void ADC(void)
{
    static unsigned char channel IBSS_ATTR;
    /* read current value */
    adc_data[(channel&0x03)] = ADVALUE;

    /* switch channel
     *
     * set source remark
     * ADCONFIG is 16bit wide so we have to shift data by 16bits left
     * thats why we shift <<24 instead of <<8
     */

    channel++;

    and_l(~(0x03<<24),&ADCONFIG);
    or_l( (((channel&0x03) << 8 )|(1<<7))<<16, &ADCONFIG);
    
    if ( (channel & 0x03) == 0 )
        /* disable ADC interrupt */
        and_l((~(1<<6))<<16,&ADCONFIG);
}

unsigned short adc_scan(int channel)
{
    /* maybe we can drop &0x03 part */
    return adc_data[(channel&0x03)];
}

void adc_tick(void)
{
    /* enable ADC interrupt */
    or_l( ((1<<6))<<16, &ADCONFIG);
}

void adc_init(void)
{
    /* GPIO38 GPIO39 */
    and_l(~((1<<6)|(1<<7)), &GPIO1_FUNCTION);

    /* ADOUT_SEL = 01
     * SOURCE SELECT = 000
     * CLEAR INTERRUPT FLAG
     * ENABLE INTERRUPT = 0
     * ADOUT_DRIVE = 00
     * ADCLK_SEL = 011 (busclk/8)
     */

    ADCONFIG = (1<<10)|(1<<8)|(1<<7)|0x03;

    /* ADC interrupt level 4.0 */
    or_l((4<<28), &INTPRI8);

    /* create tick task which enables ADC interrupt */
    tick_add_task(adc_tick);

    /* let the interrupt handler fill readout array */
    sleep(2);
}
