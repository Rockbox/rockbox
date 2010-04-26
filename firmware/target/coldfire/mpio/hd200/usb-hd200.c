/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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
#include <stdbool.h>
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"

void usb_init_device(void)
{
    /* GPIO42 is USB detect input
     * but it also serves as MCLK2 for DAC
     */
    and_l(~(1<<4), &GPIO1_OUT);
    or_l((1<<4)|(1<<18), &GPIO1_ENABLE);   /* GPIO36 GPIO50 */
    or_l((1<<4)|(1<<18), &GPIO1_FUNCTION); 

     /* GPIO22 GPIO30*/
     /* GPIO31 has to be low to ATA work */
    or_l((1<<22)|(1<<30), &GPIO_OUT);
    or_l((1<<22)|(1<<30)|(1<<31), &GPIO_ENABLE);
    or_l((1<<22)|(1<<30)|(1<<31), &GPIO_FUNCTION);
}

int usb_detect(void)
{
    /* GPIO42 active low*/
    return (GPIO1_READ & (1<<10)) ? USB_EXTRACTED : USB_INSERTED;
}

void usb_enable(bool on)
{
   
    if(on)
    {
        or_l((1<<18),&GPIO1_OUT); /* GPIO50 high */

        and_l(~(1<<30),&GPIO_OUT);  /* GPIO30 low */
        /* GPIO36 low delay GPIO36 high delay */
        and_l(~(1<<4),&GPIO1_OUT);
        or_l((1<<4),&GPIO1_OUT);

        and_l(~(1<<18),&GPIO1_OUT); /* GPIO50 low */
        sleep(HZ/5); /* delay 200 ms */
        and_l(~(1<<22),&GPIO_OUT); /* GPIO22 low */
    }
    else
    {
        /* GPIO36 low delay GPIO36 high delay */
        and_l(~(1<<4),&GPIO1_OUT);
        sleep(HZ/100);
        or_l((1<<4),&GPIO1_OUT);
        sleep(HZ/100);

        or_l((1<<22),&GPIO_OUT);  /* GPIO22 high */
        or_l((1<<30),&GPIO_OUT);  /* GPIO30 high */

        and_l(~(1<<4),&GPIO1_OUT); /* GPIO36 low */

       //or_l((1<<18),&GPIO1_OUT);  /* GPIO50 high */
    }
}
