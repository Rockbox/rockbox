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
#include "backlight-target.h"

/* Index 0 is a dummy, 1..31 are used */
static unsigned char log_brightness[32] = {
      0,   1,   2,   4,   6,   9,  12,  16,  20,  25,  30,  36,
     42,  49,  56,  64,  72,  81,  90, 100, 110, 121, 132, 144,
    156, 169, 182, 196, 210, 225, 240, 255
};

static unsigned brightness = 100;   /* 1 to 255 */
static bool enabled = false;

/* Handling B03 in _backlight_on() and _backlight_off() makes backlight go off
 * without delay. Not doing that does a short (fixed) fade out. Opt for the
 * latter. */

void _backlight_on(void)
{
    /* brightness full */
    outl(0x80000000 | (brightness << 16), 0x7000a010);
    GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x08);
    enabled = true;
}

void _backlight_off(void)
{
    outl(0x80000000, 0x7000a010);
    GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL, 0x08);
    enabled = false;
}

void _backlight_set_brightness(int val)
{
    brightness = log_brightness[val];
    if (enabled)
        outl(0x80000000 | (brightness << 16), 0x7000a010);
}

bool _backlight_init(void)
{
    GPIO_SET_BITWISE(GPIOB_ENABLE, 0x0c); /* B02 and B03 enable */
    GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x08); /* B03 = 1 */
    GPO32_ENABLE &= ~0x2000000; /* D01 disable, so pwm takes over */
    DEV_EN |= DEV_PWM;   /* PWM enable */

    _backlight_on();
    return true;
}
