/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
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
#include "cpu.h"
#include "button.h"
#include "adc.h"

/* 

Results of button testing (viewing ADC values whilst pressing buttons):

HOLD:  GPIOB & 0x0200 (0=hold active, 0x0200 = hold inactive)

ADC[1]: (approx values)

Idle        - 0x3ff
MENU        - unknown

REPEAT/AB   - 0x03?
LEFT        - 0x07?-0x08?
SELECT      - 0x0c?
RIGHT       - 0x11?

PLAY/PAUSE  - 0x17?-0x018?
VOL UP      - 0x1e?-0x01f?
VOL DOWN    - 0x26?

*/

void button_init_device(void)
{
    /* Nothing to do */
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int adc;
 
    /* TODO - determine how to detect BUTTON_MENU - it doesn't appear to 
              be connected to a GPIO or to an ADC 
     */

    adc = adc_read(ADC_BUTTONS);

    if (adc < 0x384) {
        if (adc < 0x140) {
            if (adc < 0x96) {
                if (adc < 0x50) {
                    btn |= BUTTON_REPEATAB;   /* 0x00..0x4f */
                } else {
                    btn |= BUTTON_LEFT;       /* 0x50..0x95 */
                }
            } else {
                if (adc < 0xe0) {
                    btn |= BUTTON_SELECT;     /* 0x96..0xdf */
                } else {
                    btn |= BUTTON_RIGHT;      /* 0xe0..0x13f */
                }
            }
        } else {
            if (adc < 0x208) {
                if (adc < 0x1b0) {
                    btn |= BUTTON_PLAYPAUSE;  /* 0x140..0x1af */
                } else {
                    btn |= BUTTON_VOLUP;      /* 0x1b0..0x207 */
                }
            } else {
                btn |= BUTTON_VOLDOWN;        /* 0x209..0x383 */
            }
        }
    }

    return btn;
}

bool button_hold(void)
{
    return (GPIOB & 0x200)?false:true;
}
