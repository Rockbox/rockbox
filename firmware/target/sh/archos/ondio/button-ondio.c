/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
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
#include "backlight.h"
#include "adc.h"

/*
 Ondio hardware button hookup
 ============================

 LEFT, RIGHT, UP, DOWN:    connected to AN4 through a resistor network

 The voltage on AN4 depends on which keys (or key combo) is pressed

 OPTION: AN2, high active (assigned as MENU)
 ON/OFF: AN3, low active (assigned as OFF)
*/

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;

    /* Check the 4 direction keys */
    data = adc_read(ADC_BUTTON_ROW1);
    if (data >= 165)
    {
        if (data >= 585)
            if (data >= 755)
                btn = BUTTON_LEFT;
            else
                btn = BUTTON_RIGHT;
        else
            if (data >= 415)
                btn = BUTTON_UP;
            else
                btn = BUTTON_DOWN;
    }

    if(adc_read(ADC_BUTTON_OPTION) > 0x200) /* active high */
        btn |= BUTTON_MENU;
    if(adc_read(ADC_BUTTON_ONOFF) < 0x120) /* active low */
        btn |= BUTTON_OFF;
        
    return btn;
}
