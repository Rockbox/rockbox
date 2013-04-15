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

extern unsigned short pca9555_in_ports;

/* upper bounds of key's voltage values */
#define ADV_KEYUP       0x05B + 0x10
#define ADV_KEYDOWN     0x0F3 + 0x10
#define ADV_KEYLEFT     0x190 + 0x10
#define ADV_KEYRIGHT    0x22C + 0x10
#define ADV_KEYMENU     0x2CA + 0x10


void button_init_device(void) {
    GPIO_PCCON &= ~(1<<1);
    pca9555_init();
}

int button_read_device(void) {
    int adc_val = adc_read(ADC_BUTTONS);
    int button = 0;

    if (adc_val < ADV_KEYLEFT) {
        if (adc_val < ADV_KEYDOWN) {
            if (adc_val < ADV_KEYUP) {
                button = BUTTON_UP;
            } else {
                button =  BUTTON_DOWN;
            }
        } else {
            button = BUTTON_LEFT;
        }
    } else {
        if (adc_val < ADV_KEYRIGHT) {
            button = BUTTON_RIGHT;
        } else if (adc_val < ADV_KEYMENU) {
            button = BUTTON_MENU;
        }
    }

    if (GPIO_PCDR & (1<<1)) {
        button |= BUTTON_PLAY;
    }
    if (((GPIO_PFDR&(1<<2)) == 0) &&
        ((pca9555_in_ports & (1<<15)) == 0)) {
        button |= BUTTON_BACK;
    }
    return button;
}
