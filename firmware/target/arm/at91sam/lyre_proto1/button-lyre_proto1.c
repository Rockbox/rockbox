/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2009 by Jorge Pinto
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

#include "at91sam9260.h"
#include "button.h"

#define BUTTON_01 AT91C_PIO_PB4
#define BUTTON_02 AT91C_PIO_PB5
#define BUTTON_03 AT91C_PIO_PB27
#define BUTTON_04 AT91C_PIO_PB26
#define BUTTON_05 AT91C_PIO_PB25
#define BUTTON_06 AT91C_PIO_PB24
#define BUTTON_07 AT91C_PIO_PB22
#define BUTTON_08 AT91C_PIO_PB23

void button_init_device(void)
{
    /* Enable the periph clock for the PIO controller */
    /* This is mandatory when PIO are configured as input */
    AT91C_PMC_PCER = (1 << AT91C_ID_PIOB);

    /* Set the PIO line in input */
    AT91C_PIOB_ODR = (BUTTON_01 |
                      BUTTON_02 |
                      BUTTON_03 |
                      BUTTON_04 |
                      BUTTON_05 |
                      BUTTON_06 |
                      BUTTON_07 |
                      BUTTON_08);

    /* Set the PIO controller in PIO mode instead of peripheral mode */
    AT91C_PIOB_PER = (BUTTON_01 |
                      BUTTON_02 |
                      BUTTON_03 |
                      BUTTON_04 |
                      BUTTON_05 |
                      BUTTON_06 |
                      BUTTON_07 |
                      BUTTON_08);
}

bool button_hold(void)
{
    return (0);
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    uint32_t buttons = AT91C_PIOB_PDSR;
    uint32_t ret = 0;

    if ((buttons & BUTTON_01) == 0)
        ret |= BUTTON_UP;

    if ((buttons & BUTTON_02) == 0)
        ret |= BUTTON_RIGHT;

    if ((buttons & BUTTON_03) == 0)
        ret |= BUTTON_PLAY;

    if ((buttons & BUTTON_04) == 0)
        ret |= BUTTON_SELECT;

    if ((buttons & BUTTON_05) == 0)
        ret |= BUTTON_LEFT;

    if ((buttons & BUTTON_06) == 0)
        ret |= BUTTON_DOWN;

    if ((buttons & BUTTON_07) == 0)
        ret |= BUTTON_STOP;

    if ((buttons & BUTTON_08) == 0)
        ret |= BUTTON_MENU;

    return ret;
}
