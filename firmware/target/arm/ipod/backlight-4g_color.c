/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "i2c.h"
#include "debug.h"
#include "rtc.h"
#include "usb.h"
#include "power.h"
#include "system.h"
#include "button.h"
#include "timer.h"
#include "backlight.h"

void _backlight_on(void)
{
    /* brightness full */
    outl(0x80000000 | (0xff << 16), 0x7000a010);

    /* set port b bit 3 on */
    GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x08);
}

void _backlight_off(void)
{
    /* fades backlight off on 4g */
    GPO32_ENABLE &= ~0x2000000;
    outl(0x80000000, 0x7000a010);
}

bool _backlight_init(void)
{
    GPIOB_ENABLE |= 0x4; /* B02 enable */
    GPIOB_ENABLE |= 0x8; /* B03 enable */
    GPO32_ENABLE |= 0x2000000; /* D01 enable */
    GPO32_VAL |= 0x2000000;  /* D01 =1 */
    DEV_EN |= DEV_PWM;   /* PWM enable */

    _backlight_on();
    return true;
}
