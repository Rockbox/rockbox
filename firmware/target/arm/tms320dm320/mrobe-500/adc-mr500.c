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
#include "system-target.h"
#include "button-target.h"

void adc_init(void)
{
    /* Pin 15 appears to be the nPWD pin - make sure it is high otherwise the
     *  touchscreen does not work, audio has not been tested, but it is
     *  expected that is will also not work when low.
     */
     
    /* Setup touchscreen (tsc2100) pins:
     *  14 - input, touchscreen irq
     *  15 - output, touchscreen nPWD? */
    /*  14: input , non-inverted,    irq, falling edge, no-chat, normal */
    dm320_set_io(14, true, false, true, false, false, 0x00);
    
    /*  15: output, non-inverted, no-irq, falling edge, no-chat, normal */
    dm320_set_io(15, false, false, false, false, false, 0x00);
    IO_GIO_BITSET0  =   (1<<15); /* Turn on TSC2100 */

    /* Initialize the touchscreen and the battery readout */
    tsc2100_adc_init();
    
    /* Enable the tsc2100 interrupt */
    IO_INTC_EINT2 |= (1<<3); /* IRQ_GIO14 */
    
    /* Read all registers to make sure they are clear */
    tsc2100_read_data();
}

/* Touchscreen data available interupt */
void GIO14(void)
{
    /* Interrupts work properly when cleared first */
    IO_INTC_IRQ2 = (1<<3); /* IRQ_GIO14 == 35 */

    /* Always read all registers in one go to clear any missed flags */
    tsc2100_read_data();
    
    /* Stop the scan, firmware will initiate another scan with a mode set */
    tsc2100_set_mode(true, 0x00);  
}

