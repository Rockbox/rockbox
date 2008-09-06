/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Vitja Makarov
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
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "power.h"

#include "pcf50606.h"

void power_init(void)
{
    pcf50606_write(PCF5060X_DCDC1, 0x90);
    pcf50606_write(PCF5060X_DCDC2, 0x48);
    pcf50606_write(PCF5060X_DCDC3, 0xfc);
    pcf50606_write(PCF5060X_DCDC4, 0xb1);

    pcf50606_write(PCF5060X_IOREGC, 0xe9);
    /* 3.3V, touch-panel */
    pcf50606_write(PCF5060X_D1REGC1, 0xf8);
    pcf50606_write(PCF5060X_D2REGC1, 0xf2);
    pcf50606_write(PCF5060X_D3REGC1, 0xf5);

    pcf50606_write(PCF5060X_LPREGC1, 0x00);
    pcf50606_write(PCF5060X_LPREGC2, 0x02);

    pcf50606_write(PCF5060X_DCUDC1, 0xe6);
    pcf50606_write(PCF5060X_DCUDC2, 0x30);

    pcf50606_write(PCF5060X_DCDEC1, 0xe7);
    pcf50606_write(PCF5060X_DCDEC2, 0x02);

    pcf50606_write(PCF5060X_INT1M, 0x5b);
    pcf50606_write(PCF5060X_INT1M, 0xaf);
    pcf50606_write(PCF5060X_INT1M, 0x8f);

    pcf50606_write(PCF5060X_OOCC1, 0x40);
    pcf50606_write(PCF5060X_OOCC2, 0x05);

    pcf50606_write(PCF5060X_MBCC3, 0x3a);
    pcf50606_write(PCF5060X_GPOC1, 0x00);
    pcf50606_write(PCF5060X_BBCC, 0xf8);
}

/* Control leds on ata2501 board */
void power_touch_panel(bool on)
{
    if (on)
        pcf50606_write(PCF5060X_D1REGC1, 0xf8);
    else
        pcf50606_write(PCF5060X_D1REGC1, 0x00);
}

void ide_power_enable(bool on)
{
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

#if CONFIG_TUNER
#include "tuner.h"

/** Tuner **/
static bool powered = false;

#define TUNNER_CLK  (1 << 5)
#define TUNNER_DATA (1 << 6)
#define TUNNER_NR_W (1 << 7)

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
            GPIOA_DIR &= ~TUNNER_DATA; 

            /* 2. Drive NR_W low */
            GPIOA &= ~TUNNER_NR_W;
            GPIOA_DIR |= TUNNER_NR_W; 

            /* 3. Drive CLOCK high */
            GPIOA |= TUNNER_CLK; 
            GPIOA_DIR |= TUNNER_CLK; 

            lv24020lp_power(true);
        }
        else
        {
            lv24020lp_power(false);

            /* set all as inputs */
            GPIOC_DIR &= ~(TUNNER_CLK | TUNNER_DATA | TUNNER_NR_W);
        }

        powered = status;
    }

    lv24020lp_unlock();
    return old_status;
}

#endif /* CONFIG_TUNER */

bool charger_inserted(void)
{
    return (GPIOA & 0x1) ? true : false;
}
