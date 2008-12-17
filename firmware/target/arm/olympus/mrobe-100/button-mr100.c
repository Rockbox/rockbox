/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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
#include "backlight-target.h"
#include "synaptics-mep.h"

#define LOGF_ENABLE
#include "logf.h"

#define MEP_BUTTON_HEADER   0x1a
#define MEP_BUTTON_ID       0x9
#define MEP_ABSOLUTE_HEADER 0x0b

static int int_btn = BUTTON_NONE;

#ifndef BOOTLOADER
static int syn_status = 0;

void button_init_device(void)
{
    /* enable touchpad leds */
    GPIOA_ENABLE     |= BUTTONLIGHT_ALL;
    GPIOA_OUTPUT_EN  |= BUTTONLIGHT_ALL;
    
    /* enable touchpad */
    GPO32_ENABLE     |=  0x40000000;
    GPO32_VAL        &= ~0x40000000;

    /* enable ACK, CLK, DATA lines */
    GPIOD_ENABLE     |=  (0x1 | 0x2 | 0x4);

    GPIOD_OUTPUT_EN  |=  0x1; /* ACK  */
    GPIOD_OUTPUT_VAL |=  0x1; /* high */

    GPIOD_OUTPUT_EN  &= ~0x2; /* CLK  */
    
    GPIOD_OUTPUT_EN  |=  0x4; /* DATA */
    GPIOD_OUTPUT_VAL |=  0x4; /* high */

    if (syn_init())
    {
#ifdef ROCKBOX_HAS_LOGF
        syn_info();
#endif

        syn_status = 1;

        /* enable interrupts */
        GPIOD_INT_LEV &= ~0x2;
        GPIOD_INT_CLR |=  0x2;
        GPIOD_INT_EN  |=  0x2;

        CPU_INT_EN    |= HI_MASK;
        CPU_HI_INT_EN |= GPIO0_MASK;
    }
}

/*
 * Button interrupt handler
 */
void button_int(void)
{
    int data[4];
    int val, id;

    int_btn = BUTTON_NONE;

    if (syn_status)
    {
        /* disable interrupt while we read the touchpad */
        GPIOD_INT_EN  &= ~0x2;
        GPIOD_INT_CLR |=  0x2;

        val = syn_read_device(data, 4);
        if (val > 0)
        {
            val = data[0] & 0xff;      /* packet header */
            id = (data[1] >> 4) & 0xf; /* packet id */

            logf("button_read_device...");
            logf("  data[0] = 0x%08x", data[0]);
            logf("  data[1] = 0x%08x", data[1]);
            logf("  data[2] = 0x%08x", data[2]);
            logf("  data[3] = 0x%08x", data[3]);

            if ((val == MEP_BUTTON_HEADER) && (id == MEP_BUTTON_ID))
            {
                /* Buttons packet - touched one of the 5 "buttons" */
                if (data[1] & 0x1)
                    int_btn |= BUTTON_PLAY;
                if (data[1] & 0x2)
                    int_btn |= BUTTON_MENU;
                if (data[1] & 0x4)
                    int_btn |= BUTTON_LEFT;
                if (data[1] & 0x8)
                    int_btn |= BUTTON_DISPLAY;
                if (data[2] & 0x1)
                    int_btn |= BUTTON_RIGHT;

                /* An Absolute packet should follow which we ignore */
                val = syn_read_device(data, 4);

                logf("  int_btn = 0x%04x", int_btn);
            }
            else if (val == MEP_ABSOLUTE_HEADER)
            {
                /* Absolute packet - the finger is on the vertical strip.
                   Position ranges from 1-4095, with 1 at the bottom. */
                val = ((data[1] >> 4) << 8) | data[2]; /* position */

                logf(" pos %d", val);
                logf(" z %d", data[3]);
                logf(" finger %d", data[1] & 0x1);
                logf(" gesture %d", data[1] & 0x2);
                logf(" RelPosVld %d", data[1] & 0x4);

                if(data[1] & 0x1)  /* if finger on touch strip */
                {
                    if ((val > 0) && (val <= 1365))
                        int_btn |= BUTTON_DOWN;
                    else if ((val > 1365) && (val <= 2730))
                        int_btn |= BUTTON_SELECT;
                    else if ((val > 2730) && (val <= 4095))
                        int_btn |= BUTTON_UP;
                }
            }
        }

        /* re-enable interrupts */
        GPIOD_INT_LEV &= ~0x2;
        GPIOD_INT_EN  |=  0x2;
    }
}
#else
void button_init_device(void){}
#endif /* bootloader */

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = int_btn;

    if(button_hold())
        return BUTTON_NONE;
    
    if (~GPIOA_INPUT_VAL & 0x40)
        btn |= BUTTON_POWER;

    return btn;
}

bool button_hold(void)
{
    return (GPIOD_INPUT_VAL & 0x10) ? false : true;
}

bool headphones_inserted(void)
{
    return (GPIOD_INPUT_VAL & 0x80) ? false : true;
}
