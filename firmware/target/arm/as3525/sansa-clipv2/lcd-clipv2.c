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
#include "system.h"
#include "cpu.h"

void lcd_hw_init(void)
{
/* DBOP initialisation, do what OF does */
    CCU_IO |= (1<<12); /* ?? */
    CGU_DBOP |= /*(1<<3)*/ 0x18 | AS3525_DBOP_DIV;

    DBOP_CTRL      = 0x51004;
    DBOP_TIMPOL_01 = 0x36A12F;
    DBOP_TIMPOL_23 = 0xE037E037;

    GPIOB_DIR |= (1<<2)|(1<<5);
    GPIOB_PIN(5) = (1<<5);
}

#define LCD_DELAY 10

void lcd_write_command(int byte)
{
    volatile int i = 0;
    while(i<LCD_DELAY) i++;

    /* unset D/C# (data or command) */
    GPIOB_PIN(2) = 0;
    DBOP_TIMPOL_23 = 0xE0370036;

    /* Write command */
    /* Only bits 15:12 and 3:0 of DBOP_DOUT are meaningful */
    DBOP_DOUT = (byte << 8) | byte;

    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0)
        ;

    DBOP_TIMPOL_23 = 0xE037E037;
}

void lcd_write_data(const fb_data* p_bytes, int count)
{
    volatile int i = 0;
    while(i<LCD_DELAY) i++;

    /* set D/C# (data or command) */
    GPIOB_PIN(2) = (1<<2);

    while (count--)
    {
        /* Write pixels */
        /* Only bits 15:12 and 3:0 of DBOP_DOUT are meaningful */
        DBOP_DOUT = (*p_bytes << 8) | *p_bytes;

        p_bytes++; /* next packed pixels */

        /* Wait if push fifo is full */
        while ((DBOP_STAT & (1<<6)) != 0);
    }
    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0);
}
