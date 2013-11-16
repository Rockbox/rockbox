/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Andrew Ryabinin
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
#include "backlight.h"

void button_init_device(void) {
    /* setup button gpio as input */
    GPIO_PCCON &= ~(POWEROFF_BUTTON);
}

int button_read_device(void) {
    int adc_val = adc_read(ADC_BUTTONS);
    int gpio_btn = GPIO_PCDR & BUTTON_PLAY;

    if (adc_val < 270) {
        if (adc_val < 110) {             /* 0 - 109 */
            return BUTTON_RETURN | gpio_btn;
        } else {                         /* 110 - 269 */
            return BUTTON_MENU |gpio_btn;
        }
    } else {
        if (adc_val < 420) {                   /* 270 - 419 */
                return BUTTON_BWD | gpio_btn;
        } else if (adc_val < 580) {            /* 420 - 579 */
                return BUTTON_FWD | gpio_btn;
        }
    }
    return gpio_btn;
}


