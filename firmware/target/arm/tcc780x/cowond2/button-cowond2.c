/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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

static enum touchpad_mode current_mode = TOUCHPAD_POINT;
void touchpad_set_mode(enum touchpad_mode mode)
{
    current_mode = mode;
}
enum touchpad_mode touchpad_get_mode(void)
{
    return current_mode;
}

void button_init_device(void)
{
    /* Nothing to do */
}

bool button_hold(void)
{
    return (GPIOA & 0x8) ? false : true;
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int adc;

    if (GPIOB & 0x4)
    {
        adc = adc_read(ADC_BUTTONS);

        /* The following contains some arbitrary, but working, guesswork */
        if (adc < 0x038) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x048) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS);
        } else if (adc < 0x058) {
            btn |= (BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x070) {
            btn |= BUTTON_PLUS;
        } else if (adc < 0x090) {
            btn |= (BUTTON_MINUS  | BUTTON_MENU);
        } else if (adc < 0x150) {
            btn |= BUTTON_MINUS;
        } else if (adc < 0x200) {
            btn |= BUTTON_MENU;
        }
    }

    /* TODO: Read 'fake' buttons based on touchscreen quadrants. 
       Question: How can I read from the PCF chip (I2C) in a tick task? */

    if (!(GPIOA & 0x4))
        btn |= BUTTON_POWER;

    return btn;
}
