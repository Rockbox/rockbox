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
#include "synaptics-mep.h"

#define LOGF_ENABLE
#include "logf.h"

#define MEP_BUTTON_HEADER   0x19
#define MEP_BUTTON_ID       0x9
#define MEP_ABSOLUTE_HEADER 0x0b

static int int_btn = BUTTON_NONE;

/*
 * Generate a click sound from the player (not in headphones yet)
 * TODO: integrate this with the "key click" option
 */
void button_click(void)
{
    GPO32_ENABLE |= 0x2000;
    GPIOD_OUTPUT_VAL |= 0x8;
    udelay(1000);
    GPO32_VAL &= ~0x2000;
}

#ifndef BOOTLOADER
static int syn_status = 0;

void button_init_device(void)
{
    /* enable touchpad */
    GPO32_ENABLE     |=  0x80;
    GPO32_VAL        &= ~0x80;
    udelay(1000);

    /* enable ACK, CLK, DATA lines */
    GPIOD_ENABLE |= 0x80;
    GPIOA_ENABLE |= (0x10 | 0x20);

    GPIOD_OUTPUT_EN  |=  0x80; /* ACK  */
    GPIOD_OUTPUT_VAL |=  0x80; /* high */

    GPIOA_OUTPUT_EN  &= ~0x20; /* CLK  */
    
    GPIOA_OUTPUT_EN  |=  0x10; /* DATA */
    GPIOA_OUTPUT_VAL |=  0x10; /* high */

    if (syn_init())
    {
#ifdef ROCKBOX_HAS_LOGF
        syn_info();
#endif
        syn_status = 1;

        /* enable interrupts */
        GPIOA_INT_LEV &= ~0x20;
        GPIOA_INT_CLR |=  0x20;
        GPIOA_INT_EN  |=  0x20;

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
        GPIOA_INT_EN  &= ~0x20;
        GPIOA_INT_CLR |=  0x20;

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
                /* Buttons packet */
                if (data[1] & 0x1)
                    int_btn |= BUTTON_LEFT;
                if (data[1] & 0x2)
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
        GPIOA_INT_LEV &= ~0x20;
        GPIOA_INT_EN  |=  0x20;
    }
}
#else
void button_init_device(void){}
#endif /* bootloader */

bool button_hold(void)
{
    return !(GPIOJ_INPUT_VAL & 0x8);
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    static int btn_old = BUTTON_NONE;
    int btn = int_btn;

    /* Hold */
    if(button_hold())
        return BUTTON_NONE;

    /* Device buttons */
    if (!(GPIOA_INPUT_VAL & 0x01)) btn |= BUTTON_MENU;
    if (!(GPIOA_INPUT_VAL & 0x02)) btn |= BUTTON_VOL_UP;
    if (!(GPIOA_INPUT_VAL & 0x04)) btn |= BUTTON_VOL_DOWN;
    if (!(GPIOA_INPUT_VAL & 0x08)) btn |= BUTTON_VIEW;
    if (!(GPIOD_INPUT_VAL & 0x20)) btn |= BUTTON_PLAYLIST;
    if (!(GPIOD_INPUT_VAL & 0x40)) btn |= BUTTON_POWER;

    if ((btn != btn_old) && (btn != BUTTON_NONE))
        button_click();

    btn_old = btn;

    return btn;
}

bool headphones_inserted(void)
{
    return (GPIOE_INPUT_VAL & 0x80) ? true : false;
}
