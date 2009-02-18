/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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
#include "backlight-target.h"
#include "system.h"
#include "backlight.h"
#include "synaptics-mep.h"

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
static const int brightness_vals[16] =
                {255,237,219,201,183,165,147,130,112,94,76,58,40,22,5,0};

void _backlight_set_brightness(int brightness)
{
    outl(0x80000000 | (brightness_vals[brightness-1] << 16), 0x7000a000);
}
#endif

void _backlight_on(void)
{
    GPO32_VAL    &= ~0x1000000;
    GPO32_ENABLE &= ~0x1000000;
}

void _backlight_off(void)
{
    GPO32_VAL    |= 0x1000000;
    GPO32_ENABLE |= 0x1000000;
}

#ifdef HAVE_BUTTON_LIGHT

#define BUTTONLIGHT_MASK 0x7f

static unsigned short buttonight_brightness = DEFAULT_BRIGHTNESS_SETTING - 1;
static unsigned short buttonlight_status = 0;

static void set_buttonlight(int brightness)
{
    int data[6];

    if (syn_get_status())
    {
        syn_int_enable(false);

        /* turn on all touchpad leds */
        data[0] = 0x05;
        data[1] = 0x31;
        data[2] = (brightness & 0xff) << 4;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = BUTTONLIGHT_MASK;
        syn_send(data, 6);

        /* device responds with a single-byte ACK packet */
        syn_read(data, 2);

        syn_int_enable(true);
    }
}

void _buttonlight_on(void)
{
    if (!buttonlight_status)
    {
        set_buttonlight(buttonight_brightness);
        buttonlight_status = 1;
    }
}
 
void _buttonlight_off(void)
{
    if (buttonlight_status)
    {
        set_buttonlight(0);
        buttonlight_status = 0;
    }
}

void _buttonlight_set_brightness(int brightness)
{
    buttonight_brightness = brightness - 1;
    set_buttonlight(buttonight_brightness);
    buttonlight_status = 1;
}
#endif
