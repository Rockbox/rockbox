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

void button_init_device(void)
{
    GPIOA_DIR |= 0xC;
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
	
	if (!button_hold()){
        GPIOA |= 0x4;
        GPIOA &= ~0x8;
    	
        int i=20; while (i--);
        	    
        if (GPIOA & 0x10) btn |= BUTTON_PLAYPAUSE; /* up */
        if (GPIOA & 0x20) btn |= BUTTON_RIGHT;
        if (GPIOA & 0x40) btn |= BUTTON_LEFT;

        GPIOA |= 0x8;
        GPIOA &= ~0x4;
	
        i=20; while (i--);
	
        if (GPIOA & 0x10) btn |= BUTTON_VOLUP;
        if (GPIOA & 0x20) btn |= BUTTON_VOLDOWN;
        if (GPIOA & 0x40) btn |= BUTTON_REPEATAB; /* down */
	
        if (GPIOA & 0x80) btn |= BUTTON_SELECT;  
        if (GPIOA & 0x100) btn |= BUTTON_MENU;
	}
	return btn;
}

bool button_hold(void)
{
    return (GPIOA & 0x2);
}
