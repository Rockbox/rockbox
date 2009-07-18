/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Dave Chapman
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"

#include "inttypes.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "i2c-s5l8700.h"
#include "kernel.h"
#include "thread.h"
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

char version[] = APPSVERSION;

/* Show the Rockbox logo - in show_logo.c */
extern int show_logo(void);

extern int line;

void main(void)
{
    int i;

    system_init();
    i2c_init();
    kernel_init();

    enable_irq();

    lcd_init();

    _backlight_init();

    lcd_puts_scroll(0,0,"+++ this is a very very long line to test scrolling. ---");
    verbose = 0;
    i = 0;
    while (!button_hold()) {
        line = 1;

        printf("i=%d",i++);
        printf("TBCNT:   %08x",TBCNT);
        printf("GPIO  0: %08x",PDAT0);
        printf("GPIO  1: %08x",PDAT1);
        printf("GPIO  2: %08x",PDAT2);
        printf("GPIO  3: %08x",PDAT3);
        printf("GPIO  4: %08x",PDAT4);
        printf("GPIO  5: %08x",PDAT5);
        printf("GPIO  6: %08x",PDAT6);
        printf("GPIO  7: %08x",PDAT7);
        printf("GPIO 10: %08x",PDAT10);
        printf("GPIO 11: %08x",PDAT11);
        printf("GPIO 13: %08x",PDAT13);
        printf("GPIO 14: %08x",PDAT14);

        lcd_update();
    }

    disable_irq();

    /* Branch back to iBugger entry point */
    asm volatile("ldr pc, =0x08640568");

    /* We never reach here */
    while(1);
}
