/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 François Dinel
 * Copyright ©   2008-2009 Rafaël Carré
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
#include "button-target.h"
#include "as3525v2.h"
#include "kernel.h"

void button_init_device(void)
{
    /*  Set pins to input for reading buttons */
    GPIOC_DIR = 0;                      /* All C pins input */
    GPIOA_DIR &= ~(1<<1|1<<6|1<<7);     /* Pins A1,A6,A7 input */
                                        /* OF does not set D6 to input  */
    GPIOB_DIR |= (1<<6);                     /*  Pin B6 output  */
    GPIOB_DIR |= (1<<0);                /* Pin B0 set output */
}

int button_read_device(void)
{
    int buttons = 0;

    /*  Buttons do not appear to need reset */
    /*  D6 needs special handling though   */

    GPIOB_PIN(0) = 1;                   /* set B0  */

    int delay = 500;
    do {
        asm volatile("nop\n");
    } while (delay--);

    if GPIOD_PIN(6)                     /* read D6  */
        buttons |= BUTTON_POWER;

    GPIOB_PIN(0) = 0;                   /* unset B0  */

    delay = 240;
    do {
        asm volatile("nop\n");
    } while (delay--);

    if GPIOA_PIN(1)
        buttons |= BUTTON_HOME;
    if GPIOA_PIN(6)
        buttons |= BUTTON_VOL_DOWN;
    if GPIOA_PIN(7)
        buttons |= BUTTON_VOL_UP;
    if GPIOC_PIN(2)
        buttons |= BUTTON_UP;
    if GPIOC_PIN(3)
        buttons |= BUTTON_LEFT;
    if GPIOC_PIN(4)
        buttons |= BUTTON_SELECT;
    if GPIOC_PIN(5)
        buttons |= BUTTON_RIGHT;
    if GPIOC_PIN(1)
        buttons |= BUTTON_DOWN;

  return buttons;
}
