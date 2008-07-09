/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "pcf50606.h"
#include "button-target.h"
#include "tuner.h"

#ifndef SIMULATOR

void power_init(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* Clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);
    
    /* Set outputs as per OF - further investigation required. */
    pcf50606_write(PCF5060X_DCDEC1,  0xe4);
    pcf50606_write(PCF5060X_IOREGC,  0xf5);
    pcf50606_write(PCF5060X_D1REGC1, 0xf5);
    pcf50606_write(PCF5060X_D2REGC1, 0xe9);
    pcf50606_write(PCF5060X_D3REGC1, 0xf8); /* WM8985 3.3v */
    pcf50606_write(PCF5060X_DCUDC1,  0xe7);
    pcf50606_write(PCF5060X_LPREGC1, 0x0);
    pcf50606_write(PCF5060X_LPREGC2, 0x2);

#ifndef BOOTLOADER
    IEN |= EXT3_IRQ_MASK;   /* Unmask EXT3 */
#endif
}

void ide_power_enable(bool on)
{
    (void)on;
}

bool ide_powered(void)
{
    return true;
}

void power_off(void)
{
    /* Forcibly cut power to SoC & peripherals by putting the PCF to sleep */
    pcf50606_write(PCF5060X_OOCC1, GOSTDBY | CHGWAK | EXTONWAK);
}

#ifndef BOOTLOADER
void EXT3(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* Clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);

    if (data[0] & 0x04)
    {
        /* ONKEY1S: don't reset the timeout, because we want a way to power off
           the player in the event of a crashed plugin or UIE/panic, etc. */
#if 0
        /* ONKEY1S: reset timeout as we're using SW poweroff */
        pcf50606_write(0x08, pcf50606_read(0x08) | 0x02); /* OOCC1: TOTRST=1 */
#endif
    }

    if (data[2] & 0x08)
    {
        /* Touchscreen event, do something about it */
        button_set_touch_available();
    }
}
#endif

#if CONFIG_CHARGING
bool charger_inserted(void)
{
    return (GPIOC & (1<<26)) ? false:true;
}
#endif

#if CONFIG_TUNER

/** Tuner **/
static bool powered = false;

bool tuner_power(bool status)
{
    bool old_status;
    lv24020lp_lock();

    old_status = powered;

    if (status != old_status)
    {
        if (status)
        {
            /* When power up, host should initialize the 3-wire bus
               in host read mode: */

            /* 1. Set direction of the DATA-line to input-mode. */
            GPIOC_DIR &= ~(1 << 30); 

            /* 2. Drive NR_W low */
            GPIOC_CLEAR = (1 << 31); 
            GPIOC_DIR |= (1 << 31); 

            /* 3. Drive CLOCK high */
            GPIOC_SET = (1 << 29); 
            GPIOC_DIR |= (1 << 29); 

            lv24020lp_power(true);
        }
        else
        {
            lv24020lp_power(false);

            /* set all as inputs */
            GPIOC_DIR &= ~((1 << 29) | (1 << 30) | (1 << 31));
        }

        powered = status;
    }

    lv24020lp_unlock();
    return old_status;
}

#endif /* CONFIG_TUNER */

#else /* SIMULATOR */

bool charger_inserted(void)
{
    return false;
}

void charger_enable(bool on)
{
    (void)on;
}

void power_off(void)
{
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */
