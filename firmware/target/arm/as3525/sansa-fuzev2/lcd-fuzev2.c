/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Dave Chapman
 * Copyright (C) 2010 by Thomas Martitz
 *
 * LCD driver for the Sansa Fuze - controller unknown
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
#include "lcd.h"
#include "system.h"
#include "clock-target.h"
#include "dbop-as3525.h"
#include "lcd-fuze.h"

void lcd_write_cmd(int16_t cmd)
{
    unsigned short data = swap16(cmd);
    DBOP_TIMPOL_23 = 0xA12F0036;
    dbop_write_data(&data, 1);

    int delay = 32;
    do {
        nop;
    } while(delay--);

    DBOP_TIMPOL_23 = 0xA12FE037;
}

void lcd_write_reg(int reg, int value)
{
    int16_t data = swap16(value);
    lcd_write_cmd(reg);
    dbop_write_data(&data, 1);
}


static void as3525_dbop_init(void)
{
    bitset32(&CCU_IO, 1<<12);
    CGU_DBOP |= (1<<4) | (1<<3) | AS3525_DBOP_DIV;
    DBOP_TIMPOL_01 = 0xE12FE12F;
    DBOP_TIMPOL_23 = 0xE12F0036;
    DBOP_CTRL = 0x41004;
    DBOP_TIMPOL_23 = 0x60036;
    DBOP_CTRL = 0x51004;
    DBOP_TIMPOL_01 = 0x60036;
    DBOP_TIMPOL_23 = 0xA12FE037;
}


void lcd_init_device(void)
{
    as3525_dbop_init();

    GPIOA_DIR |= (0x20|0x1);
    GPIOA_DIR &= ~(1<<3);
    GPIOA_PIN(3) = 0;
    GPIOA_PIN(0) = 1;
    GPIOA_PIN(4) = 0;

    GPIOB_DIR |= (1<<0)|(1<<2)|(1<<3);
    GPIOB_PIN(0) = 1<<0;
    GPIOB_PIN(2) = 1<<2;
    GPIOB_PIN(3) = 1<<3;

    GPIOA_PIN(4) = 1<<4;
    GPIOA_PIN(5) = 1<<5;

    fuze_display_on();
}
