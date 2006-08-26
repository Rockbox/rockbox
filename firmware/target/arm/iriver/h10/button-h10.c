/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Custom written for the H10 based on analysis of the GPIO data */


#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"


void button_init_device(void)
{
    /* No hardware initialisation required as it is done by the bootloader */
}

bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x4)?false:true;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    unsigned char state;
    static bool hold_button = false;

#if 0
    /* light handling */
    if (hold_button && !button_hold())
    {
        backlight_on();
    }
#endif

    hold_button = button_hold();
    if (!hold_button)
    {
        /* Read normal buttons */
        state = GPIOA_INPUT_VAL & 0xf8;
        if ((state & 0x8) == 0) btn |= BUTTON_FF;
        if ((state & 0x10) == 0) btn |= BUTTON_PLAY;
        if ((state & 0x20) == 0) btn |= BUTTON_REW;
        if ((state & 0x40) == 0) btn |= BUTTON_RIGHT;
        if ((state & 0x80) == 0) btn |= BUTTON_LEFT;
        
        /* Read power button */
        if (GPIOB_INPUT_VAL & 0x1) btn |= BUTTON_POWER;
        
        /* Read scroller */
        if ( (GPIOC_INPUT_VAL & 0x8) && (GPIOD_INPUT_VAL & 0x20) )
        {
            /* Scroller is pressed */
            btn |= BUTTON_SCROLL_DOWN;
        }
    }
    
    return btn;
}
