/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Szymon Dziok
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
SANSA VIEW: TESTING CODE
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
#include "../kernel-internal.h"
#include "storage.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

void main(void)
{
    system_init();
    kernel_init();
    disable_irq();

    while(1)
    {
       backlight_hw_on();
       buttonlight_hw_off();
       sleep(HZ/4);
       backlight_hw_off();
       buttonlight_hw_on();
       sleep(HZ/4);
    }
}
