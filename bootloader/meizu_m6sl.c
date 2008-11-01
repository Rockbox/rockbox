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
#include "storage.h"
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

/* These functions are supposed to be static in lcd-m6sl.c, but
   we use them here for testing */
void init_lcd_spi(void);
unsigned int lcd_read_id(void);

void main(void)
{
    //Set backlight pin to output and enable
    unsigned int model;
    int oldval = PCON0;
    PCON0 = ((oldval & ~(3 << 4)) | (1 << 4));
    PDAT0 |= (1 << 2);

    //Set PLAY to input
    oldval = PCON1;
    PCON1 = ((oldval & ~(0xf << 16)) | (0 << 16));

    init_qt1106();

    // Wait for play to be pressed
    while(!(PDAT1 & (1 << 4)));
    // Wait for play to be released
    while((PDAT1 & (1 << 4)));
    PDAT0 ^= (1 << 2); //Toggle backlight
    delay(LONG_DELAY);

    init_lcd_spi();
    model=lcd_read_id();
    bl_debug_count((model&0xf000)>>12);
    bl_debug_count((model&0xf00)>>8);
    bl_debug_count((model&0xf0)>>4);
    bl_debug_count(model&0xf);

    /* Calibrate the lot */
    qt1106_io(QT1106_MODE_FREE | QT1106_MOD_INF | QT1106_DI \
       | QT1106_SLD_SLIDER | QT1106_CAL_WHEEL | QT1106_CAL_KEYS | QT1106_RES_4);

    /* Set to maximum sensitivity */
    qt1106_io(QT1106_CT | (0x00 << 8) );

    while(true)
    {
        qt1106_wait();

        int slider = qt1106_io(QT1106_MODE_FREE | QT1106_MOD_INF \
            | QT1106_DI | QT1106_SLD_SLIDER | QT1106_RES_4);
        if(slider & 0x008000)
            bl_debug_count(((slider&0xff)) + 1);
    }

    //power off
    PDAT1&=~(1<<3);
}

