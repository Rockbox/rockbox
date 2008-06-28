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
 Recorder hardware button hookup
 ===============================

 F1, F2, F3, UP:           connected to AN4 through a resistor network
 DOWN, PLAY, LEFT, RIGHT:  likewise connected to AN5

 The voltage on AN4/ AN5 depends on which keys (or key combo) is pressed

 ON:     PB8, low active
 OFF:    PB4, low active
*/

void button_init_device(void)
{
    /* Set PB4 and PB8 as input pins */
    PBCR1 &= 0xfffc;  /* PB8MD = 00 */
    PBCR2 &= 0xfcff;  /* PB4MD = 00 */
    PBIOR &= ~0x0110; /* Inputs */
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;
    static int off_button_count = 0;

    /* check F1..F3 and UP */
    data = adc_read(ADC_BUTTON_ROW1);
    if (data >= 250)
    {
        if (data >= 700)
            if (data >= 900)
                btn = BUTTON_F3;
            else
                btn = BUTTON_UP;
        else
            if (data >= 500)
                btn = BUTTON_F2;
            else
                btn = BUTTON_F1;
    }

    /* Some units have mushy keypads, so pressing UP also activates
       the Left/Right buttons. Let's combat that by skipping the AN5
       checks when UP is pressed. */
    if(!(btn & BUTTON_UP))
    {
        /* check DOWN, PLAY, LEFT, RIGHT */
        data = adc_read(ADC_BUTTON_ROW2);
        if (data >= 250)
        {
            if (data >= 700)
                if (data >= 900)
                    btn |= BUTTON_DOWN;
                else
                    btn |= BUTTON_PLAY;
            else
                if (data >= 500)
                    btn |= BUTTON_LEFT;
                else
                    btn |= BUTTON_RIGHT;
        }
    }

    /* check port B pins for ON and OFF */
    data = PBDR;
    if ((data & 0x0100) == 0)
        btn |= BUTTON_ON;

    if ((data & 0x0010) == 0)
    {
        /* When the batteries are low, the low-battery shutdown logic causes
         * spurious OFF events due to voltage fluctuation on some units.
         * Only accept OFF when read several times in sequence. */
        if (++off_button_count > 3)
            btn |= BUTTON_OFF;
    }
    else
        off_button_count = 0;
        
    return btn;
}
