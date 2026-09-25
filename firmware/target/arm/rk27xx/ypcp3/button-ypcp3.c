/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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
#include "button.h"
#include "adc.h"

/* Samsung YP-CP3 keys.
 *
 * Eight keys sit on two resistor ladders read by the LSADC, one key per
 * ladder at a time; an idle ladder reads near full scale. Measured on a
 * unit, the steps are about 133 counts apart:
 *
 *   ADC_BUTTONS (1)   up 158, down 295, select 435, left 561, right 687
 *   ADC_EXTRA   (2)   menu 155, back 292, user (Rec) 431
 *
 * Each key owns the range from half a step below its reading to half a step
 * above. The original firmware ignores anything from 0x2ee (750) up, which
 * is where the pad's range ends here too.
 *
 * Power is on its own pin, GPIO C1, active high. */

#define POWER_PIN           (1 << 1)    /* GPIO C1 */

struct ladder_key
{
    unsigned short below;   /* upper bound, exclusive */
    int button;
};

static const struct ladder_key pad_keys[] =
{
    { 227, BUTTON_UP     },
    { 365, BUTTON_DOWN   },
    { 498, BUTTON_SELECT },
    { 624, BUTTON_LEFT   },
    { 750, BUTTON_RIGHT  },
};

static const struct ladder_key side_keys[] =
{
    { 224, BUTTON_MENU },
    { 362, BUTTON_BACK },
    { 500, BUTTON_USER },
};

static int ladder_button(int channel, const struct ladder_key *keys, int n)
{
    unsigned short val = adc_read(channel);
    int button = 0;
    int i;

    for (i = 0; i < n && button == 0; i++)
    {
        if (val < keys[i].below)
        {
            button = keys[i].button;
        }
    }

    return button;
}

void button_init_device(void)
{
    GPIO_PCCON &= ~POWER_PIN;           /* input */
}

int button_read_device(void)
{
    int buttons = 0;

    buttons |= ladder_button(ADC_BUTTONS, pad_keys, ARRAYLEN(pad_keys));
    buttons |= ladder_button(ADC_EXTRA, side_keys, ARRAYLEN(side_keys));

    if (GPIO_PCDR & POWER_PIN)
    {
        buttons |= BUTTON_POWER;
    }

    return buttons;
}
