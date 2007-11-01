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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

Results of button testing:

HOLD:  GPIOA & 0x0002 (0=pressed, 0x0002 = released)
POWER: GPIOA & 0x8000 (0=pressed, 0x8000 = released)

ADC[0]: (approx values)

RIGHT - 0x37
LEFT  - 0x7f
JOYSTICK PRESS - 0xc7
UP             - 0x11e
DOWN           - 0x184
MODE           - 0x1f0/0x1ff
PRESET         - 0x268/0x269
REC            - 0x2dd

Values of ADC[0] tested in OF disassembly: 0x50, 0x96, 0xdc, 0x208, 0x384

*/

void button_init_device(void)
{
    /* Nothing to do */
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int adc;
 
    adc = adc_read(ADC_BUTTONS);

    if (adc < 0x384) {
        if (adc < 0x140) {
            if (adc < 0x96) {
                if (adc < 0x50) {
                    btn |= BUTTON_RIGHT;      /* 0x00..0x4f */
                } else {
                    btn |= BUTTON_LEFT;       /* 0x50..0x95 */
                }
            } else {
                if (adc < 0xe0) {
                    btn |= BUTTON_SELECT;     /* 0x96..0xdf */
                } else {
                    btn |= BUTTON_UP;         /* 0xe0..0x13f */
                }
            }
        } else {
            if (adc < 0x208) {
                if (adc < 0x1b0) {
                    btn |= BUTTON_DOWN;       /* 0x140..0x1af */
                } else {
                    btn |= BUTTON_MODE;       /* 0x1b0..0x207 */
                }
            } else {
                if (adc < 0x290) {
                    btn |= BUTTON_PRESET;     /* 0x208..0x28f */
                } else {
                    btn |= BUTTON_REC;        /* 0x290..0x383 */
                }
            }
        }
    }

    if (!(GPIOA & 0x2))
        btn |= BUTTON_HOLD;

    if (!(GPIOA & 0x8000))
        btn |= BUTTON_POWERPLAY;

    return btn;
}
