/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 François Dinel
 * Copyright (C) 2008-2009 Rafaël Carré
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

#include "lcd.h"
#include "lcd-clip.h"
#include "system.h"
#include "cpu.h"

int lcd_hw_init(void)
{
    bitset32(&CGU_PERI, CGU_SSP_CLOCK_ENABLE);

    SSP_CPSR = AS3525_SSP_PRESCALER;    /* OF = 0x10 */
    SSP_CR0 = (1<<7) | (1<<6) | 7;  /* Motorola SPI frame format, 8 bits */
    SSP_CR1 = (1<<3) | (1<<1);  /* SSP Operation enabled */
    SSP_IMSC = 0;       /* No interrupts */

    /* configure GPIO B2 (display D/C#) as output */
    GPIOB_DIR |= (1<<2);

    /* configure GPIO B3 (display type detect) as input */
    GPIOB_DIR &= ~(1<<3);

    /* reset display using GPIO A5 (display RESET#) */
    GPIOA_DIR |= (1<<5);
    GPIOA_PIN(5) = 0;
    udelay(1000);
    GPIOA_PIN(5) = (1<<5);

    /* detect display type on GPIO B3 */    
    return GPIOB_PIN(3) ? 1 : 0;
}

void lcd_write_command(int byte)
{
    while(SSP_SR & (1<<4))  /* BSY flag */
        ;

    /* LCD command mode */
    GPIOB_PIN(2) = 0;
    
    SSP_DATA = byte;
    while(SSP_SR & (1<<4))  /* BSY flag */
        ;
}

void lcd_write_data(const fb_data* p_bytes, int count)
{
    /* LCD data mode */
    GPIOB_PIN(2) = (1<<2);

    while (count--)
    {
        while(!(SSP_SR & (1<<1)))   /* wait until transmit FIFO is not full */
            ;

        SSP_DATA = *p_bytes++;
    }
}

void lcd_enable_power(bool onoff)
{
    (void) onoff;
}

