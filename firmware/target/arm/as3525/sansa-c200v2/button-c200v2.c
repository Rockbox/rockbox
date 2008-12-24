/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-e200v2.c 19035 2008-11-07 05:31:05Z jdgordon $
 *
 * Copyright (C) 2006 by Barry Wardell
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

/* Taken from button-h10.c by Barry Wardell and reverse engineering by MrH. */

#include "system.h"
#include "button.h"
#include "backlight.h"
#include "powermgmt.h"


#ifndef BOOTLOADER
/* Buttons */
static bool hold_button     = false;
static bool hold_button_old = false;
#define _button_hold() hold_button
#else
#define _button_hold() false  /* FIXME */
#endif /* BOOTLOADER */
static int  int_btn         = BUTTON_NONE;

void button_init_device(void)
{
}

bool button_hold(void)
{
    return _button_hold();
}

/* device buttons */
void button_int(void)
{
    int delay = 0x50;
    int dir_save_c = 0;
    int afsel_save_c = 0;

    int_btn = BUTTON_NONE;

    /* Save the current direction and afsel */
    dir_save_c = GPIOC_DIR;
    afsel_save_c = GPIOC_AFSEL;

    GPIOA_DIR &= ~(1<<2);
    GPIOC_AFSEL &= ~(1<<6|1<<5|1<<4|1<<3);
    GPIOC_DIR |= (1<<6|1<<5|1<<4|1<<3);

    /* These should not be needed with button event interupts */
    /* they are necessary now to clear out lcd data */

    GPIOC_PIN(6) = (1<<6);
    GPIOC_PIN(5) = (1<<5);
    GPIOC_PIN(4) = (1<<4);
    GPIOC_PIN(3) = (1<<3);
    GPIOC_DIR &= ~(1<<6|1<<5|1<<4|1<<3);

    while(delay--);
    
    /* direct GPIO connections */
    if (GPIOA_PIN(3))
        int_btn |= BUTTON_POWER;
    if (!GPIOC_PIN(6))
        int_btn |= BUTTON_RIGHT;
    if (!GPIOC_PIN(5))
        int_btn |= BUTTON_UP;
    if (!GPIOC_PIN(4))
        int_btn |= BUTTON_SELECT;
    if (!GPIOC_PIN(3))
        int_btn |= BUTTON_DOWN;

    /* return to settings needed for lcd */
    GPIOC_DIR = dir_save_c;
    GPIOC_AFSEL = afsel_save_c;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    /* Read buttons directly */
    button_int();
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    return int_btn;
}
