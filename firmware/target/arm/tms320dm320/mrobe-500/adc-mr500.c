/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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

#include "cpu.h"
#include "adc.h"
#include "adc-target.h"
#include "kernel.h"
#include "tsc2100.h"
#include "button-target.h"

void read_battery_inputs(void);

void adc_init(void)
{
    /* Initialize the touchscreen and the battery readout */
    tsc2100_adc_init();
    
    /* Enable the tsc2100 interrupt */
    IO_INTC_EINT2 |= (1<<3); /* IRQ_GIO14 */
}

/* Touchscreen data available interupt */
void GIO14(void)
{
    short tsadc = tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS);
    short adscm = (tsadc&TSADC_ADSCM_MASK)>>TSADC_ADSCM_SHIFT;
    
    /* Always read all registers in one go to clear any missed flags */
    tsc2100_read_data();
    
    switch (adscm)
    {
        case 1:
        case 2:
            touch_read_coord();
            break;
        case 0x0B:
            read_battery_inputs();
            break;
    }
    IO_INTC_IRQ2 = (1<<3); /* IRQ_GIO14 == 35 */
}
