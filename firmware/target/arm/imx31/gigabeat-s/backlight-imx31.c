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
#include "system.h"
#include "backlight.h"
#include "mc13783.h"
#include "backlight-target.h"

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
/* Table that uses combinations of current level and pwm fraction to get
 * as many uniquely-visible brightness levels as possible. The lowest current
 * level for any average current is used even though many combinations give
 * duplicate values. Current (I) values are in mA. */
static const struct
{
    unsigned char md;
    unsigned char pwm;
} led_md_pwm_table[] =
{
               /* I-level PWM(x/15) I-Avg */
    { 0,  0 }, /*    0        0      0.0  */
    { 1,  1 }, /*    3        1      0.2  */
    { 1,  2 }, /*    3        2      0.4  */
    { 1,  3 }, /*    3        3      0.6  */ 
    { 1,  4 }, /*    3        4      0.8  */ 
    { 1,  5 }, /*    3        5      1.0  */ 
    { 1,  6 }, /*    3        6      1.2  */ 
    { 1,  7 }, /*    3        7      1.4  */ 
    { 1,  8 }, /*    3        8      1.6  */ 
    { 1,  9 }, /*    3        9      1.8  */ 
    { 1, 10 }, /*    3       10      2.0  */ 
    { 1, 11 }, /*    3       11      2.2  */ 
    { 1, 12 }, /*    3       12      2.4  */ /* default */
    { 1, 13 }, /*    3       13      2.6  */ 
    { 1, 14 }, /*    3       14      2.8  */ 
    { 1, 15 }, /*    3       15      3.0  */ 
    { 2,  9 }, /*    6        9      3.6  */ 
    { 2, 10 }, /*    6       10      4.0  */ 
    { 2, 11 }, /*    6       11      4.4  */ 
    { 2, 12 }, /*    6       12      4.8  */ 
    { 2, 13 }, /*    6       13      5.2  */ 
    { 2, 14 }, /*    6       14      5.6  */ 
    { 2, 15 }, /*    6       15      6.0  */ 
    { 3, 11 }, /*    9       11      6.6  */ 
    { 3, 12 }, /*    9       12      7.2  */ 
    /* Anything higher is just too much   */
};
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

/* Bits always combined with ramping bits */
#define MC13783_LED_CONTROL0_BITS \
    (MC13783_LEDEN | MC13783_BOOSTEN | MC13783_ABMODE_MONCH_LEDMD1234 | \
     MC13783_ABREF_400MV)

static struct mutex backlight_mutex;    /* Block brightness change while
                                         * setting up fading */
static bool backlight_on_status = true; /* Is on or off? */
static uint32_t backlight_pwm_bits;     /* Final PWM setting for fade-in */

/* Backlight ramping settings */
static uint32_t led_ramp_mask = MC13783_LEDMDRAMPDOWN | MC13783_LEDMDRAMPUP;

bool _backlight_init(void)
{
    mutex_init(&backlight_mutex);

    /* Set default LED register value */
    mc13783_write(MC13783_LED_CONTROL0, MC13783_LED_CONTROL0_BITS);

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    /* Our PWM and I-Level is different than retailos (but same apparent
     * brightness), so init to our default. */
    _backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
#else
    /* Use default PWM */
    backlight_pwm_bits = mc13783_read(MC13783_LED_CONTROL2) & MC13783_LEDMDDC;
#endif

    return true;
}

void backlight_set_fade_out(bool value)
{
    if (value)
        led_ramp_mask |= MC13783_LEDMDRAMPDOWN;
    else
        led_ramp_mask &= ~MC13783_LEDMDRAMPDOWN;
}

void backlight_set_fade_in(bool value)
{
    if (value)
        led_ramp_mask |= MC13783_LEDMDRAMPUP;
    else
        led_ramp_mask &= ~MC13783_LEDMDRAMPUP;
}

void _backlight_on(void)
{
    static const char regs[2] =
    {
        MC13783_LED_CONTROL0,
        MC13783_LED_CONTROL2
    };

    uint32_t data[2];

    mutex_lock(&backlight_mutex);

    /* Set/clear LEDRAMPUP bit, clear LEDRAMPDOWN bit */
    data[0] = MC13783_LED_CONTROL0_BITS;

    if (!backlight_on_status)
        data[0] |= led_ramp_mask & MC13783_LEDMDRAMPUP;

    backlight_on_status = true;

    /* Specify final PWM setting */
    data[1] = mc13783_read(MC13783_LED_CONTROL2);

    if (data[1] != MC13783_DATA_ERROR)
    {
        data[1] &= ~MC13783_LEDMDDC;
        data[1] |= backlight_pwm_bits;

        /* Write regs within 30us of each other (requires single xfer) */
        mc13783_write_regset(regs, data, 2);
    }

    mutex_unlock(&backlight_mutex);
}

void _backlight_off(void)
{
    uint32_t ctrl0 = MC13783_LED_CONTROL0_BITS;

    mutex_lock(&backlight_mutex);

    if (backlight_on_status)
        ctrl0 |= led_ramp_mask & MC13783_LEDMDRAMPDOWN;

    backlight_on_status = false;

    /* Set/clear LEDRAMPDOWN bit, clear LEDRAMPUP bit */
    mc13783_write(MC13783_LED_CONTROL0, ctrl0);

    /* Wait 100us - 500ms */
    sleep(HZ/100);

    /* Write final PWM setting */
    mc13783_write_masked(MC13783_LED_CONTROL2, MC13783_LEDMDDCw(0),
                         MC13783_LEDMDDC);

    mutex_unlock(&backlight_mutex);
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
/* Assumes that the backlight has been initialized - parameter should
 * already be range-checked in public interface. */
void _backlight_set_brightness(int brightness)
{
    uint32_t md;

    mutex_lock(&backlight_mutex);

    md = led_md_pwm_table[brightness].md;
    backlight_pwm_bits = backlight_on_status ?
        MC13783_LEDMDDCw(led_md_pwm_table[brightness].pwm) : 0;

    mc13783_write_masked(MC13783_LED_CONTROL2,
                         MC13783_LEDMDw(md) | backlight_pwm_bits,
                         MC13783_LEDMD | MC13783_LEDMDDC);

    mutex_unlock(&backlight_mutex);
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
