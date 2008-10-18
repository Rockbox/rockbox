/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Frank Gevaerts
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

#include "qt1106.h"
#include "cpu.h"
#include "system.h"


#define SPIDELAY(_x) void(_x);
#define SETMOSI() (PDAT1 |= (1 << 6))
#define CLRMOSI() (PDAT1 &= ~(1 << 6))

#define MISO ((PDAT0 >> 3) & 1)
#define RDY (PDAT1 & (1 << 5))

#define SETCLK() (PDAT0 |= (1 << 1))
#define CLRCLK() (PDAT0 &= ~(1 << 1))

#define SETSS() (PDAT0 |= (1 << 0))
#define CLRSS() (PDAT0 &= ~(1 << 0))

#define CHANGE (PDAT0 & (1 << 4))


void init_qt1106(void)
{
    int oldval;

    oldval = PCON0;
    //Set P0.0 and P0.1 to output, set P0.3 and P0.4 to input
    PCON0 = ((oldval & ~(3 << 0 | 3 << 2 | 3 << 6 | 3 << 8)) | (1 << 0 | 1 << 2));

    oldval = PCON1;
    //Set P1.5 to input, set P1.6 to output
    PCON1 = ((oldval & ~(0xf << 20 | 0xf << 24)) | (1 << 24));


    SETSS();
    SETCLK();
}

static inline void delay(int duration)
{
    volatile int i;
    for(i=0;i<duration;i++);
}

void qt1106_wait(void)
{
    while(!(PDAT0 & (1 << 4))) {}
}

unsigned int qt1106_io(unsigned int output)
{
    unsigned int input = 0;
    int i;

    while(!RDY) {}

    delay(10*100);  // < 470 us

    CLRSS();
    delay(13*100); // > 22 us

    for (i = 0; i < 24; i++) {

        CLRCLK();

        if (output & (1 << 23))
            SETMOSI();
        else
            CLRMOSI();
        output <<= 1;

        delay(20*100); // >> 6.7 us

        SETCLK();

        input <<= 1;
        input |= MISO;

        delay(20*100); // >> 6.7 us
    }

    SETSS();

    return (input);
}


