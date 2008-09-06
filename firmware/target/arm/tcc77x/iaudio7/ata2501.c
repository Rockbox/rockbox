/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Vitja Makarov
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
#include "button.h"

#include "ata2501.h"

#define STB   (1<<5)
#define SDATA (1<<4)
#define RESET (1<<6)
#define SIFMD (1<<7)
#define STB_DELAY 200

#define udelay _udelay

/* do we really need it? */
static void _udelay(int cycles)
{
    cycles /= 8;
    while (cycles--) {
        asm("nop;nop;");
    }
}

/*
  TODO: sensitivity using GPIOS
*/
void ata2501_init(void)
{
    GPIOD_DIR |= (RESET | STB | SIFMD | (1 << 8) | (1 << 9));
    GPIOD_DIR &= ~(SDATA);

    GPIOD &= ~RESET;
    udelay(1000);

    GPIOD |= RESET;

    GPIOD &= ~STB;

#if 1
    GPIOD &= ~((1 << 9) | (1 << 8));
    GPIOD |= ((1 << 8) | SIFMD) | (1 << 9);
#else
    GPIOD |= ((1 << 9) | (1 << 8));
    GPIOD &= ~(SIFMD);
#endif
}

unsigned short ata2501_read(void)
{
    unsigned short ret = 0;
    int i;

    for (i = 0; i < 12; i++) {
        GPIOD |= STB;
        udelay(50);

        ret <<= 1;
        if (GPIOD & SDATA)
            ret |= 1;
        udelay(50);
        GPIOD &= ~STB;
        udelay(100);
    }

    return ret;
}

#define ATA2501_TEST
#ifdef ATA2501_TEST
#include "lcd.h"
#include "sprintf.h"

static
void bits(char *str, unsigned short val)
{
    int i;

    for (i = 0; i < 12; i++)
        str[i] = (val & (1 << i)) ? '1' : '0';
    str[i] = 0;
}

void ata2501_test(void)
{
    char buf[100];
    ata2501_init();

    while (1) {
        unsigned short data;
        int i, line = 0;

        data = ata2501_read();
        lcd_clear_display();
        lcd_puts(0, line++, "ATA2501 test");

        bits(buf, data);
        lcd_puts(0, line++, buf);

        lcd_update();
        udelay(2000);
    }
}
#endif
