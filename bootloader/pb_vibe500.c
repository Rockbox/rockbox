/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Szymon Dziok
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

/*
The bootloader does nothing and it's not needed (it was used to test different
stuff only), because the original bootloader stored in the flash has ability to 
boot three different images in the SYSTEM directory:
jukebox.mi4 - when Power is pressed,
blupd.mi4 - when Power+C combo is used,
tester.mi4 - when Power+OK combo is used.

So we can use it to dual boot (for example renaming original jukebox.mi4 to 
tester.mi4 and the rockbox.mi4 to jukebox.mi4).
*/


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"

#include "inttypes.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
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
#include "i2c.h"

/* #define UNK_01 (*(volatile unsigned long*)(0x7000a010)) */

char version[] = APPSVERSION;

extern int show_logo(void);

void main(void)
{
    system_init();
    kernel_init();
    disable_irq();
    lcd_init();

    show_logo();
    sleep(HZ*2);

    while(1)
    {
       /* Power off bit */
       if ((button_read_device()&BUTTON_POWER)!=0)
       GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL,0x80);
    }
}
