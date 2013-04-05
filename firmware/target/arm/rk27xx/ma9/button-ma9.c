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
#include "pca9555.h"

#define ADV_KEYUP       0x5B
#define ADV_KEYDOWN     0xF3
#define ADV_KEYLEFT     0x190
#define ADV_KEYRIGHT    0x22C
#define ADV_KEYMENU     0x2CA


void button_init_device(void) {
    GPIO_PCCON &= ~(0x02);
    GPIO_PFCON &= ~(0x04);      // PF2 for PCA9555 input INT
    pca9555_init();

}

int button_read_device(void) {
    int adc_val = adc_read(ADC_BUTTONS);
    int button =  0;

    if (adc_val < ADV_KEYLEFT + 0x10) {
        if (adc_val < ADV_KEYDOWN +0x10) {
            if (adc_val < ADV_KEYUP+0x10) {
                button = BUTTON_UP;
            } else {
                button =  BUTTON_DOWN;
            }
        } else {
            button = BUTTON_LEFT;
        }
    } else {
        if (adc_val < ADV_KEYRIGHT + 0x10) {
            button = BUTTON_RIGHT;
        } else if (adc_val < ADV_KEYMENU +0x10) {
            button = BUTTON_MENU;
        }
    }
    return button | ((GPIO_PCDR & 0x02) ? BUTTON_PLAY:0)| (pca9555_get_back_key()? BUTTON_BACK:0);
}
