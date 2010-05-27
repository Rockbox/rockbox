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
    /* Write register */
    DBOP_TIMPOL_23 = 0xa167006e;
    dbop_write_data(&cmd, 1);

    int delay = 4;
    do {
        nop;
    } while(delay--);

    DBOP_TIMPOL_23 = 0xa167e06f;
}

void lcd_write_reg(int reg, int value)
{
    int16_t data = value;

    lcd_write_cmd(reg);
    dbop_write_data(&data, 1);
}


static void as3525_dbop_init(void)
{
    CGU_DBOP = (1<<3) | AS3525_DBOP_DIV;

    DBOP_TIMPOL_01 = 0xe167e167;
    DBOP_TIMPOL_23 = 0xe167006e;

    /* short count: 16 | output data width: 16 | readstrobe line */
    DBOP_CTRL = (1<<18|1<<12|1<<3);

    GPIOB_AFSEL = 0xfc;
    GPIOC_AFSEL = 0xff;

    DBOP_TIMPOL_23 = 0x6000e;

    /* short count: 16|enable write|output data width: 16|read strobe line */
    DBOP_CTRL = (1<<18|1<<16|1<<12|1<<3);
    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;
}


void lcd_init_device(void)
{
    as3525_dbop_init();

    GPIOA_DIR |= (1<<5|1<<4|1<<3);
    GPIOA_PIN(5) = 0;
    GPIOA_PIN(3) = (1<<3);
    GPIOA_PIN(4) = 0;
    GPIOA_PIN(5) = (1<<5);

    fuze_display_on();
}
