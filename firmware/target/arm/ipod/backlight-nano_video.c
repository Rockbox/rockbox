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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

static int brightness = 1;   /* 1 to 32 */
static int current_dim = 16; /* default after enabling the backlight dimmer */
static bool enabled = false;

void _backlight_set_brightness(int val)
{
    int oldlevel;

    if (current_dim < val)
    {
        do
        {
            oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
            GPIO_CLEAR_BITWISE(GPIOD_OUTPUT_VAL, 0x80);
            udelay(10);
            GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x80);
            set_irq_level(oldlevel);
            udelay(10);
        }
        while (++current_dim < val);
    }
    else if (current_dim > val)
    {
        do
        {
            oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
            GPIO_CLEAR_BITWISE(GPIOD_OUTPUT_VAL, 0x80);
            udelay(200);
            GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x80);
            set_irq_level(oldlevel);
            udelay(10);
        }
        while (--current_dim > val);
    }
    brightness = val;
}

void _backlight_hw_enable(bool on)
{
    if (on == enabled)
        return;
        
    if (on)
    {
        GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x08);
        GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x80);
        sleep(HZ/100);
        current_dim = 16;
        _backlight_set_brightness(brightness);
    }
    else
    {
        GPIO_CLEAR_BITWISE(GPIOD_OUTPUT_VAL, 0x80);
        GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL, 0x08);
        sleep(HZ/20);
    }
    enabled = on;
}

/* Switch the backlight on. Works only if the backlight circuit is enabled.
 * Called in ISR context for fading, so it must be fast. */
void _backlight_led_on(void)
{
    GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x80);
}

/* Switch the backlight off. Keeps the backlight circuit enabled.
 * Called in ISR context for fading, so it must be fast. */
void _backlight_led_off(void)
{
    GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x80);
}

bool _backlight_init(void)
{
    GPIO_SET_BITWISE(GPIOB_ENABLE, 0x08);
    GPIO_SET_BITWISE(GPIOB_OUTPUT_EN, 0x08);
    GPIO_SET_BITWISE(GPIOD_ENABLE, 0x80);
    GPIO_SET_BITWISE(GPIOD_OUTPUT_EN, 0x80);
    _backlight_hw_enable(true);
    GPIO_SET_BITWISE(GPIOL_ENABLE, 0x80);
    GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x80);
    GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x80);

    return true;
}
