/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "kernel.h"
#include "pcf50606.h"
#include "button-target.h"
#include "logf.h"

/* Used by the bootloader to find out what caused the device to start */
unsigned char pcf50606_intregs[3];

static bool usb_ch_enabled = false;

/* These voltages were determined by measuring the output of the PCF50606
   on a running H300, and verified by disassembling the original firmware */
static void set_voltages(void)
{
    static const unsigned char buf[5] =
    {
        0xf4,   /* IOREGC  = 2.9V, ON in all states  */
        0xef,   /* D1REGC  = 2.4V, ON in all states  */
        0x18,   /* D2REGC  = 3.3V, OFF in all states */
        0xf0,   /* D3REGC  = 2.5V, ON in all states  */
        0xef,   /* LPREGC1 = 2.4V, ON in all states  */
    };

    pcf50606_write_multiple(0x23, buf, 5);
}

static void init_pmu_interrupts(void)
{
    /* inital data is interrupt masks */
    unsigned char data[3] =
    {
        ~0x00,
        ~0x00,
        ~0x06, /* unmask ACDREM, ACDINS  */
    };

    /* make sure GPI6 interrupt is off before unmasking anything */
    and_l(~0x0f000000, &INTPRI5);  /* INT38 - Priority 0 (Off) */

    /* unmask the PMU interrupts we want to service */
    pcf50606_write_multiple(0x05, data, 3);
    /* clear INT1-3 as these are left set after standby */
    pcf50606_read_multiple(0x02, pcf50606_intregs, 3);

    /* Set to read pcf50606 INT but keep GPI6 off until init completes */
    and_l(~0x00000040, &GPIO_ENABLE);
    or_l(0x00000040, &GPIO_FUNCTION);
    or_l(0x00004000, &GPIO_INT_EN);     /* GPI6 H-L */
}

static inline void enable_pmu_interrupts(void)
{
    /* clear pending GPI6 interrupts first or it may miss the first
       H-L transition */
    or_l(0x00004000, &GPIO_INT_CLEAR);
    or_l(0x03000000, &INTPRI5); /* INT38 - Priority 3 */
}

/* enables/disables USB charging
 * ATTENTION: make sure to set the irq level
 *   to highest before calling this function! */
void pcf50606_set_usb_charging(bool on)
{
    /* USB charging is controlled by GPOOD0:
       High-Z: Charge enable
       Pulled down: Charge disable
    */
    if (on)
        pcf50606_write(0x39, 0x00);
    else
        pcf50606_write(0x39, 0x07);

    usb_ch_enabled = on;

    logf("pcf50606_set_usb_charging(%s)\n", on ? "on" : "off" );
}

bool pcf50606_usb_charging_enabled(void)
{
    /* TODO: read the state of the GPOOD2 register... */
    return usb_ch_enabled;
}

void pcf50606_init(void)
{
    pcf50606_i2c_init();

    /* initialize pmu interrupts but don't service them yet */
    init_pmu_interrupts();
    set_voltages();

    pcf50606_write(0x08, 0x60); /* Wake on USB and charger insertion */
    pcf50606_write(0x09, 0x05); /* USB and ON key debounce: 14ms */
    pcf50606_write(0x29, 0x1C); /* Disable the unused MBC module */

    pcf50606_set_usb_charging(false); /* Disable USB charging atm. */

    pcf50606_write(0x35, 0x13); /* Backlight PWM = 512Hz 50/50 */
    pcf50606_write(0x3a, 0x3b); /* PWM output on GPOOD1 */

    pcf50606_write(0x33, 0x8c); /* Accessory detect: ACDAPE=1, THRSHLD=2.20V */

    enable_pmu_interrupts();    /* allow GPI6 interrupts from PMU now */
}

/* PMU interrupt */
void GPI6(void) __attribute__ ((interrupt_handler));
void GPI6(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* Clear pending GPI6 interrupts */
    or_l(0x00004000, &GPIO_INT_CLEAR);

    /* clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);

    if (data[2] & 0x06)
    {
        /* ACDINS/ACDREM */
        /* Check if the button driver should actually scan main buttons or not
           - bias towards "yes" out of paranoia. */
        button_enable_scan((data[2] & 0x02) != 0 ||
                           (pcf50606_read(0x33) & 0x01) != 0);
    }
}
