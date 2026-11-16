/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#define YPP2_POWER_HOLD (1u << 2)

/* Board power wiring from the P2 schematic and stock shutdown sequence:
 * A2=PWR_ON latch, C20=POWER input, C21=external-power detect.
 * ClearPad uses C26 for its clock. */

static void ypp2_power_hold(bool on)
{
    /* Preload PWR_ON high before enabling its output on startup.
     * Stock firmware's final power-off step enables output then drives low. */
    if (on)
        GPIOA |= YPP2_POWER_HOLD;
    GPIOA_DIR |= YPP2_POWER_HOLD;
    if (!on)
        GPIOA &= ~YPP2_POWER_HOLD;
}

void power_init(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    ypp2_power_hold(true); /* Keep the player on when the USB cable is removed. */

    /* EXT_PWR is EINT1 / GPIOC21 on the P2, active high. */
    GPIOC_DIR &= ~(1u << 21);

    /* Clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);

    /* TODO: verify the inherited PCF50606 regulator setup against P2 wiring. */
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
    /* Storage shutdown precedes the normal power-off call. Keep the final
     * hardware state stable with IRQs and FIQs masked. Use busy delays so
     * this path also works during forced shutdown with interrupts masked. */
    disable_interrupt(IRQ_FIQ_STATUS);

    /* Samsung's LCD-off path clears GPIOC28, waits 4ms, then clears GPIOA26.
     * Use the actual DATA registers as the stock firmware does. This avoids
     * leaving the backlight on while the display supplies decay. */
    GPIOC &= ~(1u << 28);
    udelay(4000);
    GPIOA &= ~(1u << 26);

    /* Release the active-high PWR_ON latch for the AAT2550/AAT2512 supplies. */
    ypp2_power_hold(false);

    /* Stay quiescent while external power keeps the board supplied. */
    while (true);
}

#ifndef BOOTLOADER
void EXT3(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* Clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);
}
#endif
#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    /* Samsung EXT_PWR input: GPIOC21, active high. */
    return (GPIOC & (1u << 21)) ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}
#endif
