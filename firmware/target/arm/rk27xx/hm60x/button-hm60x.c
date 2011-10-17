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
    if (adc_val < 30) {
        return BUTTON_UP | (GPIO_PCDR & POWEROFF_BUTTON);
    } else if (adc_val < 250) {
        return BUTTON_RIGHT | (GPIO_PCDR & POWEROFF_BUTTON);
    } else if (adc_val < 380) {
        return BUTTON_LEFT | (GPIO_PCDR & POWEROFF_BUTTON);
    } else if (adc_val < 450) {
        return BUTTON_DOWN | (GPIO_PCDR & POWEROFF_BUTTON);
    } else if (adc_val < 560) {
        return BUTTON_PLAY | (GPIO_PCDR & POWEROFF_BUTTON);
    }
    return (GPIO_PCDR & POWEROFF_BUTTON);
}
