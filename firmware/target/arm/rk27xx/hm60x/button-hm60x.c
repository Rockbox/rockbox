/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Andrew Ryabinin
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

void button_init_device(void) {
    /* setup button gpio as input */
    GPIO_PCCON &= ~(POWEROFF_BUTTON);
}

int button_read_device(void) {
    int adc_val = adc_read(ADC_BUTTONS);
    int gpio_btn = GPIO_PCDR & BUTTON_POWER;

    if (adc_val < 380) { /* 0 - 379 */
        if (adc_val < 250) { /* 0 - 249 */
            if (adc_val < 30) { /* 0 - 29 */
                return BUTTON_UP | gpio_btn;
            } else { /* 30 - 249 */
                return BUTTON_RIGHT | gpio_btn;
            }
        } else { /* 250 - 379 */
            return BUTTON_LEFT | gpio_btn;
        }
    } else { /* > 380 */
        if (adc_val < 460) { /* 380 - 459 */
            return BUTTON_DOWN | gpio_btn;
        } else { /* > 460 */
            if (adc_val < 560) {
                return BUTTON_SELECT | gpio_btn;
            }
        }
    }
    return gpio_btn;
}
