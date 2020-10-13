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
#include "backlight.h"

enum keyboard_type_t {
    KEYBOARD_V1,
    KEYBOARD_V2,
};

static enum keyboard_type_t kbd_type;

void button_init_device(void) {
    /* setup button gpio as input */
    GPIO_PCCON &= ~(POWEROFF_BUTTON);
    GPIO_PACON &= ~1;


    /* setup button gpio as pulldown */
    SCU_GPIOUPCON |= (1<<17) |
                           1 ;

    /* identify keyboard type */
    SCU_IOMUXB_CON &= ~(1<<2);
    GPIO_PCCON |= (1<<4);
    if (GPIO_PCDR & (1<<4)) {
        kbd_type = KEYBOARD_V1;
    } else {
        kbd_type = KEYBOARD_V2;
    }
}

bool button_hold() {
    return (GPIO_PADR & 1);
}

static int button_read_device_v1(void) {
    int adc_val = adc_read(ADC_BUTTONS);
    int button =  0;

    if (adc_val < 480) { /* middle */
        if (adc_val < 200) { /* 0 - 200 */
            if (adc_val < 30) {
                button = BUTTON_UP;
            } else {  /* 30 - 200 */
                button =  BUTTON_RIGHT;
            }
        } else { /* 200 - 480 */
            if (adc_val < 370) { /* 200 - 370 */
                button =  BUTTON_SELECT;
            } else { /* 370 - 480 */
                button =  BUTTON_DOWN;
            }
        }
    } else { /*  > 480 */
        if (adc_val < 690) { /* 480 - 690 */
            if (adc_val < 580) { /* 480 - 580 */
                button =  BUTTON_LEFT;
            } else { /* 580 - 690 */
                button =  BUTTON_NEXT;
            }
        } else { /* > 680 */
            if (adc_val < 840) { /* 680 - 840 */
                button =  BUTTON_PREV;
            } else {
                if (adc_val < 920) { /* 840 - 920 */
                    button =  BUTTON_PLAY;
                }
            }
        }
    }
    return button | (GPIO_PCDR & POWEROFF_BUTTON);
}

static int button_read_device_v2(void) {
    int adc_val = adc_read(ADC_BUTTONS);
    int adc_val2 = adc_read(ADC_EXTRA);
    int button =  0;

    /* Buttons on front panel */
    if (adc_val < 520) { /* middle */
        if (adc_val < 360) { /* 0 - 360 */
            if (adc_val < 40) { /* 0 - 40 */
                button |= BUTTON_UP;
            } else { /* 40 - 360 */
                button |= BUTTON_RIGHT;
            }
        } else { /* 360 - 520 */
            button |= BUTTON_SELECT;
        }
    } else { /* >= 520 */
        if (adc_val < 770) { /* 520 - 770 */
            if (adc_val < 640) { /* 520 - 640 */
                button |= BUTTON_DOWN;
            } else { /* 640 - 770 */
                button |= BUTTON_LEFT;
            }
        }
    }

    /* Buttons on top */
    if (adc_val2 < 400) { /* 0 - 400 */
        if (adc_val2 < 120) { /* 0 - 120 */
            button |= BUTTON_NEXT;
        } else { /* 120 - 400 */
            button |= BUTTON_PREV;
        }
    } else { /* >= 400 */
        if (adc_val2 < 560) { /* 400 - 560 */
            button |= BUTTON_PLAY;
        }
    }
    return button | (GPIO_PCDR & POWEROFF_BUTTON);
}

int button_read_device(void) {
    static bool hold_button = false;
#ifndef BOOTLOADER
    bool hold_button_old;

    hold_button_old = hold_button;
#endif
    hold_button = button_hold();

#ifndef BOOTLOADER
    if (hold_button != hold_button_old) {
        backlight_hold_changed(hold_button);
    }
#endif

    if (hold_button) {
        return 0;
    } else if (kbd_type == KEYBOARD_V1) {
        return button_read_device_v1();
    } else {
        return button_read_device_v2();
    }
}

