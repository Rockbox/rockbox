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
#include "ascodec.h"

#define LCD_DELAY 1

int lcd_hw_init(void)
{
/* DBOP initialisation, do what OF does */
    CGU_DBOP = (1<<3) | AS3525_DBOP_DIV;

    GPIOB_AFSEL = 0x08; /* DBOP on pin 3 */
    GPIOC_AFSEL = 0x0f; /* DBOP on pins 3:0 */

    DBOP_CTRL      = 0x51008;
    DBOP_TIMPOL_01 = 0x6E167;
    DBOP_TIMPOL_23 = 0xA167E06F;

    GPIOA_DIR |= 0x33; /* pins 5:4 and 1:0 out */
    GPIOB_DIR |= 0x40; /* pin 6 out */

    GPIOA_PIN(1) = (1<<1);
    GPIOA_PIN(0) = (1<<0);
    GPIOA_PIN(4) = 0;
    GPIOB_PIN(6) = (1<<6);

    return 0;
}

void lcd_write_command(int byte)
{
    volatile int i = 0;
    while(i<LCD_DELAY) i++;

    /* unset D/C# (data or command) */
    GPIOA_PIN(5) = 0;

    /* Write command */
    /* Only bits 15:12 and 3:0 of DBOP_DOUT are meaningful */
    DBOP_DOUT = (byte << 8) | byte;

    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0)
        ;
}

void lcd_write_cmd_triplet(int cmd1, int cmd2, int cmd3)
{
#ifndef LCD_USE_FIFO_FOR_COMMANDS
    lcd_write_command(cmd1);
    lcd_write_command(cmd2);
    lcd_write_command(cmd3);
#else
    /* combine writes to data register */

    while ((DBOP_STAT & (1<<10)) == 0) /* While push fifo is not empty */
        ;
    /* FIFO is empty at this point */

    /* unset D/C# (data or command) */
    GPIOA_PIN(5) = 0;

    /* Write command */
    /* !!makes assumption FIFO is at least (3) levels deep! */
    /* Only bits 15:12 and 3:0 of DBOP_DOUT are meaningful */
    DBOP_DOUT = (cmd1 << 8) | cmd1;
    DBOP_DOUT = (cmd2 << 8) | cmd2;
    DBOP_DOUT = (cmd3 << 8) | cmd3;
    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0)
        ;
#endif
}

void lcd_write_data(const fb_data* p_bytes, int count)
{
    volatile int i = 0;
    while(i<LCD_DELAY) i++;

    /* set D/C# (data or command) */
    GPIOA_PIN(5) = (1<<5);

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

void lcd_enable_power(bool onoff)
{
    ascodec_write(AS3514_DCDC15, onoff ? 1 : 0);
}

