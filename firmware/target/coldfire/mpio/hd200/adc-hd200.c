/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
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
 * We do read one channel at once
 * 
 * state      FCPU         Fbus        Fadc      bus/Fadc   Fchannelread
 * default 11.2896 MHz  5.6448 MHz  5.6448 MHz      2       172.2656 Hz
 * normal  45.1584 MHz  22.5792 MHz 2.8224 MHz      8       172.2656 Hz
 * max     124.1856 MHz 62.0928 MHz 1.9404 MHz      32      118.4326 Hz
 */

void ADC(void) __attribute__ ((interrupt_handler,section(".icode")));
void ADC(void)
{
    static unsigned int channel IBSS_ATTR;
    /* read current value */
    adc_data[(channel & 0x03)] = ADVALUE;

    /* switch channel
     *
     * set source remark
     * ADCONFIG is 16bit wide so we have to shift data by 16bits left
     * thats why we shift <<24 instead of <<8
     */

    channel++;

    and_l(~(3<<24),&ADCONFIG);
    or_l( (((channel & 0x03) << 8 )|(1<<7))<<16, &ADCONFIG);

}

unsigned short adc_scan(int channel)
{
    /* maybe we can drop &0x03 part */
    return adc_data[(channel&0x03)];
}

void adc_init(void)
{
    /* GPIO38 GPIO39 */
    and_l(~((1<<6)|(1<<7)), &GPIO1_FUNCTION);

    /* ADOUT_SEL = 01
     * SOURCE SELECT = 000
     * CLEAR INTERRUPT FLAG
     * ENABLE INTERRUPT = 1
     * ADOUT_DRIVE = 00
     * ADCLK_SEL = 011 (busclk/8)
     */

    ADCONFIG = (1<<10)|(1<<7)|(1<<6)|(1<<1)|(1<<0);

    /* ADC interrupt level 4.0 */
    or_l((4<<28), &INTPRI8);
}
