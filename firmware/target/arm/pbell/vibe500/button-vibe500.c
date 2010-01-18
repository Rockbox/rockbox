/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2009 Szymon Dziok
 * Based on the Iriver H10 and the Philips HD1630 code
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

#include "system.h"
#include "button.h"
#include "backlight.h"
#include "synaptics-mep.h"

static int int_btn = BUTTON_NONE;
static int old_pos = -1;

void button_init_device(void)
{
}

/*
 * Button interrupt handler
 */
void button_int(void)
{
    char data[4];
    int val;

    int_btn = BUTTON_NONE;

    val = touchpad_read_device(data, 4);

    if (val == MEP_BUTTON_HEADER)
    {
        /* Buttons packet */
        if (data[1] & 0x1)
            int_btn |= BUTTON_MENU;
        if (data[1] & 0x2)
            int_btn |= BUTTON_PLAY;
        if (data[1] & 0x4)
            int_btn |= BUTTON_NEXT;
        if (data[1] & 0x8)
            int_btn |= BUTTON_PREV;
    }
    else if (val == MEP_ABSOLUTE_HEADER)
    {
        /* Absolute packet - the finger is on the vertical strip.
           Position ranges from 1-4095, with 1 at the bottom. */
        val = ((data[1] >> 4) << 8) | data[2]; /* position */

        if (val > 0)
	{
	    int scr_pos = val >> 8; /* split the scrollstrip into 16 regions */
	    if ((old_pos<scr_pos)&&(old_pos!=-1)) int_btn = BUTTON_DOWN;
	    if ((old_pos>scr_pos)&&(old_pos!=-1)) int_btn = BUTTON_UP;
	    old_pos = scr_pos;
	}
        else old_pos=-1;
    }
}

int button_read_device(void)
{
    int buttons = int_btn;
    unsigned char state;
    static bool hold_button = false;
    bool hold_button_old;

    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    if (hold_button != hold_button_old)
    {
        backlight_hold_changed(hold_button);
    }
#endif

    /* device buttons */
    if (!hold_button)
    {
        /* Read Record, OK, C */
	state = GPIOA_INPUT_VAL;
	if ((state & 0x01)==0) buttons|=BUTTON_REC;
	if ((state & 0x40)==0) buttons|=BUTTON_OK;
	if ((state & 0x08)==0) buttons|=BUTTON_CANCEL;

        /* Read POWER button */
	if ((GPIOD_INPUT_VAL & 0x40)==0) buttons|=BUTTON_POWER;
    }
    else return BUTTON_NONE;
    return buttons;
}

bool button_hold(void)
{
    /* GPIOK 01000000B - HOLD when bit not set */
    return (GPIOK_INPUT_VAL & 0x40)?false:true;
}

