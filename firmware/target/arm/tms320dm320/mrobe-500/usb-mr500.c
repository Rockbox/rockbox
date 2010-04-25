/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007, 2009 by Karl Kurbjun
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
/*#define LOGF_ENABLE*/

#include "config.h"
#include "logf.h"
#include "cpu.h"
#include "system.h"

#include "m66591.h"

void usb_init_device(void) {
    logf("mxx: SOC Init");

    /* The following EMIF timing values are from the OF:
     *      IO_EMIF_CS4CTRL1 = 0x66AB;
     *      IO_EMIF_CS4CTRL2 = 0x4220;
     *
     * More agressive numbers may be possible, but it depends on the clocking
     *  setup. 
     *
     * When USB_TRANS_BLOCK is not defined the USB driver does not work unless
     *  the values from the OF are used.
     */
    
    IO_EMIF_CS4CTRL1 = 0x66AB;
    IO_EMIF_CS4CTRL2 = 0x4220; 

    /* Setup the m66591 reset signal */
    /*  2: output, non-inverted, no-irq, falling edge, no-chat, normal */
    dm320_set_io(2, false, false, false, false, false, 0x00);

    /* Setup the m66591 interrupt signal */
    /*  3: input , non-inverted,    irq, falling edge, no-chat, normal */
    dm320_set_io(3, true, false, true, false, false, 0x00);
    
    udelay(100);
    
    /* Drive the reset pin low */
    IO_GIO_BITCLR0  = 1<<2;

    /* Wait a bit */
    udelay(100);

    /* Release the reset (drive it high) */
    IO_GIO_BITSET0  = 1<<2;
    
    udelay(500);

    /* Enable the MXX interrupt */
    IO_INTC_EINT1 |= (1<<8); /* IRQ_GIO3 */
}

/* This is the initial interupt handler routine for the USB controller */
void GIO3 (void)  __attribute__ ((section(".icode")));
void GIO3 (void) {
    /* Clear the interrupt, this is critical to do before running the full
     *  handler otherwise you might miss an interrupt and everything will stop
     *  working.
     *
     * The M66591 interrupt line is attached to GPIO3.
     */
    IO_INTC_IRQ1 = (1<<8); 
    
    /* Start the full handler which is located in the driver */
    USB_DEVICE();
}

