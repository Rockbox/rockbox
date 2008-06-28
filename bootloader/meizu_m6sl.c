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

void main(void)
{
    //Set backlight pin to output and enable
    int oldval = PCON0;
    PCON0 = ((oldval & ~(3 << 4)) | (1 << 4));
    PDAT0 |= (1 << 2);

    //Set PLAY to input
    oldval = PCON1;
    PCON1 = ((oldval & ~(0xf << 16)) | (0 << 16));

    //Set the piezo pins to output
    oldval = PCON5;
    PCON5 = ((oldval & ~((0xf << 4) | (0xf << 8))) | ((1 << 0) | (1 << 4)));
    PDAT5 &= ~((1 << 1) | (1 << 2)); //should not be needed

    PDAT5 |= (1 << 1); //Toggle piezo +

    //toggle backlight on PLAY
    while(true)
    {
        // Wait for play to be pressed
        while(!(PDAT1 & (1 << 4)))
        {
        }

        PDAT5 ^= (1 << 1); //Toggle piezo +
        PDAT0 ^= (1 << 2); //Toggle packlight

        // Wait for play to be released
        while(PDAT1 & (1 << 4))
        {
        }
    }
}

