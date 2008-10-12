/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Greg White
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

#include <stdlib.h>
#include <stdio.h>
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rbunicode.h"
#include "usb.h"
#include "qt1106.h"
#include "rockboxlogo.h"


#include <stdarg.h>

char version[] = APPSVERSION;
#define LONG_DELAY  200000
#define SHORT_DELAY  50000
#define PAUSE_DELAY  50000

static inline void delay(int duration)
{
    volatile int i;
    for(i=0;i<duration;i++);
}


void bl_debug(bool bit)
{
    if (bit)
    {
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(LONG_DELAY);
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(LONG_DELAY);
    }
    else
    {
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(SHORT_DELAY);
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(SHORT_DELAY);
    }
}

void bl_debug_count(unsigned int input)
{
    unsigned int i;
    delay(SHORT_DELAY*3);
    for (i = 0; i < input; i++)
    {
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(SHORT_DELAY);
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(2*SHORT_DELAY);
    }
}
void bl_debug_int(unsigned int input,unsigned int count)
{
    unsigned int i;
    for (i = 0; i < count; i++)
    {
        bl_debug(input>>i & 1);
    }
    delay(SHORT_DELAY*6);
}

void main(void)
{
    char mystring[64];
    int tmpval;

    /* set fclk = 200MHz, hclk = 100MHz, pclk = 50MHz, others off */
    CLKCON = 0x00800080;
    PLLCON = 0;
    PLL0PMS = 0x1ad200;
    PLL0LCNT = 8100;
    PLLCON = 1;
    while (!(PLLLOCK & 1)) ;
    CLKCON2= 0x80;
    CLKCON = 0x20803180;

    /* mask all interrupts
       this is done, because the lcd framebuffer
       overwrites some stuff, which leads to a freeze
       when an irq is generated after the dfu upload.
       crt0 should have disabled irqs,
       but the bootrom hands us execution in
       user mode so we can't switch interrupts off */
    INTMSK = 0;

    //Set backlight pin to output and enable
    int oldval = PCON0;
    PCON0 = ((oldval & ~(3 << 4)) | (1 << 4));
    PDAT0 |= (1 << 2);

    //Set PLAY to input
    oldval = PCON1;
    PCON1 = ((oldval & ~(0xf << 16)) | (0 << 16));

    asm volatile("mrs %0, cpsr \n\t"
		 : "=r" (tmpval)
    );

    lcd_init();
    snprintf(mystring, 64, "tmpval: %x", tmpval);
    lcd_puts(0,0,mystring);
    lcd_update();

    init_qt1106();

    /* Calibrate the lot */
    qt1106_io(QT1106_MODE_FREE | QT1106_MOD_INF | QT1106_DI \
       | QT1106_SLD_SLIDER | QT1106_CAL_WHEEL | QT1106_CAL_KEYS | QT1106_RES_256);

    lcd_clear_display();
    lcd_bitmap(rockboxlogo, 0, 30, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);
    lcd_update();

    /* Set to maximum sensitivity */
    qt1106_io(QT1106_CT | (0x00 << 8) );

    while(true)
    {
        qt1106_wait();

        int slider = qt1106_io(QT1106_MODE_FREE | QT1106_MOD_INF \
            | QT1106_DI | QT1106_SLD_SLIDER | QT1106_RES_256);
        snprintf(mystring, 64, "%x %2.2x",(slider & 0x008000)>>15, slider&0xff);
        lcd_puts(0,1,mystring);
        lcd_update();
        /*
        if(slider & 0x008000)
            bl_debug_count(((slider&0xff)) + 1);
        */
    }

    //power off
    PDAT1&=~(1<<3);
}

