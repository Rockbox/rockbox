/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "adc.h"


#define CS_LO  and_l(~0x80, &GPIO_OUT)
#define CS_HI  or_l(0x80, &GPIO_OUT)
#define CLK_LO and_l(~0x00400000, &GPIO_OUT)
#define CLK_HI or_l(0x00400000, &GPIO_OUT)
#define DO     (GPIO_READ & 0x80000000)
#define DI_LO  and_l(~0x00200000, &GPIO_OUT)
#define DI_HI  or_l(0x00200000, &GPIO_OUT)

/* delay loop */
#define DELAY   \
    ({                                \
        int _x_;                      \
        asm volatile (                \
            "move.l #11, %[_x_] \r\n" \
        "1:                     \r\n" \
            "subq.l #1, %[_x_]  \r\n" \
            "bhi.b  1b          \r\n" \
            : [_x_]"=&d"(_x_)         \
        );                            \
    })

unsigned short adc_scan(int channel)
{
    int level = disable_irq_save();
    unsigned char data = 0;
    int i;
    
    CS_LO;

    DI_HI;  /* Start bit */
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;
    
    DI_HI;  /* Single channel */
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;

    if(channel & 1) /* LSB of channel number */
        DI_HI;
    else
        DI_LO;
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;
    
    if(channel & 2) /* MSB of channel number */
        DI_HI;
    else
        DI_LO;
    DELAY;
    CLK_HI;
    DELAY;
    CLK_LO;

    DELAY;

    for(i = 0;i < 8;i++) /* 8 bits of data */
    {
        CLK_HI;
        DELAY;
        CLK_LO;
        DELAY;
        data <<= 1;
        data |= DO?1:0;
    }

    CS_HI;

    restore_irq(level);
    return data;
}

void adc_init(void)
{
    or_l(0x80600080, &GPIO_FUNCTION); /* GPIO7:  CS
                                         GPIO21: Data In (to the ADC)
                                         GPIO22: CLK
                                         GPIO31: Data Out (from the ADC) */
    or_l(0x00600080, &GPIO_ENABLE);
    or_l(0x80, &GPIO_OUT);          /* CS high */
    and_l(~0x00400000, &GPIO_OUT);  /* CLK low */
}
