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
    int button =  0;

    if (adc_val < 480) { /* middle */
        if (adc_val < 200) { /* 200 - 0 */
	    if (adc_val < 30) {
	        button = BUTTON_UP;
	    } else {
	        button =  BUTTON_RIGHT; /* 30 - 200 */
	    }
        } else { /* 200 - 480 */
	    if (adc_val < 370) {
	        button =  BUTTON_SELECT;
	    } else {
 	        button =  BUTTON_DOWN;
	    }
	}
    } else { /*  > 480 */
        if (adc_val < 690) { /* 480 - 690 */
	    if (adc_val < 580) {
	        button =  BUTTON_LEFT;
	    } else {
	        button =  BUTTON_NEXT;
	    }
	} else { /* > 680 */
	    if (adc_val < 840) {
	        button =  BUTTON_PREV;
	    } else {
	        if (adc_val < 920) {
		    button =  BUTTON_PLAY;
		}
	    }
	}
    }
    return button | (GPIO_PCDR & POWEROFF_BUTTON);
}
