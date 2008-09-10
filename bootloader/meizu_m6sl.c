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

#include <stdarg.h>

char version[] = APPSVERSION;
#define LONG_DELAY  200000
#define SHORT_DELAY  50000
#define PAUSE_DELAY  50000

#define SPIDELAY(_x) void(_x);
#define SETMOSI() PDAT1 |= (1 << 6)
#define CLRMOSI() PDAT1 &= ~(1 << 6)

#define MISO ((PDAT0 >> 3) & 1)
#define RDY (PDAT1 & (1 << 5))

#define SETCLK() PDAT0 |= (1 << 1)
#define CLRCLK() PDAT0 &= ~(1 << 1)

#define SETSS() PDAT0 |= (1 << 0)
#define CLRSS() PDAT0 &= ~(1 << 0)

#define SPISPEED 10000

#define QT_CT 0x00800000

#define QT_AKS_DISABLED 0x00000000
#define QT_AKS_GLOBAL   0x00010000
#define QT_AKS_MODE1    0x00020000
#define QT_AKS_MODE2    0x00030000
#define QT_AKS_MODE3    0x00040000
#define QT_AKS_MODE4    0x00050000

#define QT_SLD_WHEEL    0x00000000
#define QT_SLD_SLIDER   0x00080000

#define QT_KEY7_NORMAL  0x00000000
#define QT_KEY7_PROX    0x00100000

#define QT_MODE_FREE    0x00000000
#define QT_MODE_LP1     0x00000100
#define QT_MODE_LP2     0x00000200
#define QT_MODE_LP3     0x00000300
#define QT_MODE_LP4     0x00000400
#define QT_MODE_SYNC    0x00000500
#define QT_MODE_SLEEP   0x00000600

#define QT_LPB          0x00000800
#define QT_DI           0x00001000

#define QT_MOD_10       0x00000000
#define QT_MOD_20       0x00002000
#define QT_MOD_60       0x00004000
#define QT_MOD_INF      0x00006000

#define QT_CAL_ALL      0x00000000
#define QT_CAL_KEYS     0x00000008
#define QT_CAL_WHEEL    0x00000010
#define QT_RES_4        0x00000020
#define QT_RES_8        0x00000040
#define QT_RES_16       0x00000060
#define QT_RES_32       0x00000080
#define QT_RES_64       0x000000A0
#define QT_RES_128      0x000000C0
#define QT_RES_256      0x000000E0


static inline void delay(int duration)
{
    volatile int i;
    for(i=0;i<duration;i++);
}


void init_qt1106(void)
{
    int oldval;

    oldval = PCON0;
    //Set P0.0 and P0.1 to output, set P0.3 to input
    PCON0 = ((oldval & ~(3 << 0 || 3 << 2 || 3 << 6)) | (1 << 0 | 1 << 2));

    oldval = PCON1;
    //Set P1.5 to input, set P1.6 to input
    PCON1 = ((oldval & ~(0xf << 20 || 0xf << 24)) | (1 << 24));

    SETSS();
    SETCLK();
}

unsigned read_qt1106(unsigned int input)
{
    int output = 0;
    int i;

    while(!RDY) {}

    delay(10);  // < 470 us

    CLRSS();
    delay(13); // > 22 us

    for (i = 0; i < 24; i++) {

        CLRCLK();

        if (input & (1 << 23))
            SETMOSI();
        else
            CLRMOSI();
        input <<= 1;

        delay(20); // >> 6.7 us

        SETCLK();

        output |= MISO;
        output <<= 1;

        delay(20); // >> 6.7 us
    }

    SETSS();

    return (output & 0x00FFFFFF);
}


void bl_debug(bool bit)
{
    if (bit)
    {
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(LONG_DELAY);
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(LONG_DELAY);
        //for(i=0;i<pause_delay;i++);
    }
    else
    {
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(SHORT_DELAY);
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(SHORT_DELAY);
        //for(i=0;i<pause_delay;i++);
    }
}

void bl_debug_int(unsigned int input)
{
    unsigned int i;
    for (i = 0; i < input; i++)
    {
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(SHORT_DELAY);
        PDAT0 ^= (1 << 2); //Toggle backlight
        delay(2*SHORT_DELAY);
    }
    delay(SHORT_DELAY*6);
}

void main(void)
{
    //Set backlight pin to output and enable
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

    /* Calibrate the lot */
    read_qt1106(QT_MODE_FREE | QT_MOD_INF | QT_DI | QT_SLD_SLIDER | QT_CAL_WHEEL | QT_CAL_KEYS | QT_RES_4);

    /* Set to maximum sensitivity */
    read_qt1106(QT_CT | (0x00 << 8) );

    while(true)
    {
        int slider = read_qt1106(QT_MODE_FREE | QT_MOD_INF | QT_DI | QT_SLD_SLIDER | QT_RES_4);
        bl_debug_int(((slider&0xff)) + 1);
    }

    //power off
    PDAT1&=~(1<<3);
}

