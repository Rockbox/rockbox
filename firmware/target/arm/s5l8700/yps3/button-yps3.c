/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Bertrik Sikken
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

#include <stdbool.h>
#include "config.h"

#include "inttypes.h"
#include "s5l8700.h"
#include "button.h"
#include "button-target.h"

/*  Button driver for the touch keys on the Samsung YP-S3

    The exact controller is not known, but it is likely from Melfas.
    
    The protocol is as follows:
    * the communication is done using three signals: DRDY, DCLK and DOUT
    * in the idle state these signals are all high.
    * when a key is touched or released, the key controller pulls down DRDY
      and outputs the first bit of a 20-bit word on its DOUT signal.
    * the CPU stores the bit, then acknowledges it by toggling the DCLK signal.
    * the key controller prepares the next bit, then toggles its DRDY output,
      unless all 20 bits have been transferred (in that case it stays high).
    * the 20-bit word contains separate bits for each button, some fixed bits
      and a bit indicating the number of keys pressed (modulo 2).
 */


void button_init_device(void)
{
    /* P0.5/P1.0 power switch input */
    PCON0 &= ~(3 << 10);    
    PCON1 &= ~0x0000000F;
    
    /* P1.3 headphones detect input */
    PCON1 &= ~0x0000F000;

    /* P1.5 DATA, P1.6 DRDY inputs (touch key controller) */
    PCON1 &= ~0x0FF00000; 
    
    /* P3.4 DCLK output (touch key controller) */
    PCON3 = (PCON3 & ~0x000F0000) | 0x00010000;
    PDAT3 |= (1 << 4);
    
    /* P4.3 hold switch input */
    PCON4 &= ~0x0000F000;
}

/* returns the raw 20-bit word from the touch key controller */
static int tkey_read(void)
{
    static int value = 0;
    int i;

    /* check activity */
    if (PDAT1 & (1 << 6)) {
        return value;
    }

    /* get key bits */
    value = 0;
    for (i = 0; i < 10; i++) {
        /* sample bit from falling edge of DRDY */
        while ((PDAT1 & (1 << 6)) != 0);
        value <<= 1;
        if (PDAT1 & (1 << 5)) {
            value |= 1;
        }

        /* acknowledge on DCLK */
        PDAT3 &= ~(1 << 4);

        /* sample bit from rising edge of DRDY */
        while ((PDAT1 & (1 << 6)) == 0);
        value <<= 1;
        if (PDAT1 & (1 << 5)) {
            value |= 1;
        }

        /* acknowledge on DCLK */
        PDAT3 |= (1 << 4);
    }
    return value;
}


int button_read_device(void)
{
    int buttons = 0;
    int tkey_data;

    /* hold switch */
    if (button_hold()) {
        return 0;
    }

    /* power button */
    if (PDAT1 & (1 << 0)) {
        buttons |= BUTTON_POWER;
    }
    
    /* touch keys */
    tkey_data = tkey_read();
    if (tkey_data & (1 << 9)) {
        buttons |= BUTTON_BACK;
    }
    if (tkey_data & (1 << 8)) {
        buttons |= BUTTON_UP;
    }
    if (tkey_data & (1 << 7)) {
        buttons |= BUTTON_MENU;
    }
    if (tkey_data & (1 << 6)) {
        buttons |= BUTTON_LEFT;
    }
    if (tkey_data & (1 << 5)) {
        buttons |= BUTTON_SELECT;
    }
    if (tkey_data & (1 << 4)) {
        buttons |= BUTTON_RIGHT;
    }
    if (tkey_data & (1 << 3)) {
        buttons |= BUTTON_DOWN;
    }
    
    return buttons;
}

bool button_hold(void)
{
    return (PDAT4 & (1 << 3));
}

bool headphones_inserted(void)
{
    return ((PDAT1 & (1 << 3)) == 0);
}

