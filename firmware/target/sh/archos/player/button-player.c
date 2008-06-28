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
 Player hardware button hookup
 =============================

 Player
 ------
 LEFT:   AN0
 MENU:   AN1
 RIGHT:  AN2
 PLAY:   AN3

 STOP:   PA11
 ON:     PA5

 All buttons are low active
*/

void button_init_device(void)
{
    /* set PA5 and PA11 as input pins */
    PACR1 &= 0xff3f;  /* PA11MD = 00 */
    PACR2 &= 0xfbff;  /* PA5MD = 0 */
    PAIOR &= ~0x0820; /* Inputs */
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;

    /* buttons are active low */
    if (adc_read(ADC_BUTTON_LEFT) < 0x180)
        btn = BUTTON_LEFT;
    if (adc_read(ADC_BUTTON_MENU) < 0x180)
        btn |= BUTTON_MENU;
    if (adc_read(ADC_BUTTON_RIGHT) < 0x180)
        btn |= BUTTON_RIGHT;
    if (adc_read(ADC_BUTTON_PLAY) < 0x180)
        btn |= BUTTON_PLAY;

    /* check port A pins for ON and STOP */
    data = PADR;
    if ( !(data & 0x0020) )
        btn |= BUTTON_ON;
    if ( !(data & 0x0800) )
        btn |= BUTTON_STOP;
        
    return btn;
}
