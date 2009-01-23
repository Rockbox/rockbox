/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "backlight.h"
#include "system.h"
#include "backlight-target.h"
#include "avic-imx31.h"
#include "clkctl-imx31.h"
#include "mc13783.h"

/* Most code in here is taken from the Linux BSP provided by Freescale
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved. */
static uint32_t int_btn     = BUTTON_NONE;
static bool hold_button     = false;
#ifdef BOOTLOADER
static bool initialized     = false;
#else
static bool hold_button_old = false;
#endif
#define _button_hold() (GPIO3_DR & 0x10)

static __attribute__((interrupt("IRQ"))) void KPP_HANDLER(void)
{
    static const struct key_mask_shift
    {
        uint8_t mask;
        uint8_t shift;
    } kms[3] =
    {
        { 0x1f, 0 }, /* BUTTON_LEFT...BUTTON_SELECT */
        { 0x03, 5 }, /* BUTTON_BACK...BUTTON_MENU   */
        { 0x1f, 7 }, /* BUTTON_VOL_UP...BUTTON_NEXT */
    };

    int col;
    /* Power button is handled separately on PMIC */
    int button = int_btn & BUTTON_POWER;

    int oldlevel = disable_irq_save();

    /* 1. Disable both (depress and release) keypad interrupts. */
    KPP_KPSR &= ~(KPP_KPSR_KRIE | KPP_KPSR_KDIE);

    for (col = 0; col < 3; col++) /* Col */
    {
        /* 2. Write 1s to KPDR[10:8] setting column data to 1s */
        KPP_KPDR |= (0x7 << 8);

        /* 3. Configure columns as totem pole outputs(for quick
         * discharging of keypad capacitance) */
        KPP_KPCR &= ~(0x7 << 8);

        /* Give the columns time to discharge */
        udelay(2);

        /* 4. Configure columns as open-drain */
        KPP_KPCR |= (0x7 << 8);

        /* 5. Write a single column to 0, others to 1.
         * 6. Sample row inputs and save data. Multiple key presses
         *    can be detected on a single column.
         * 7. Repeat steps 2 - 6 for remaining columns. */

        /* Col bit starts at 8th bit in KPDR */
        KPP_KPDR &= ~(0x100 << col);

        /* Delay added to avoid propagating the 0 from column to row
         * when scanning. */
        udelay(2);

        /* Read row input */
        button |= (~KPP_KPDR & kms[col].mask) << kms[col].shift;
    }

    /* 8. Return all columns to 0 in preparation for standby mode. */
    KPP_KPDR &= ~(0x7 << 8);

    /* 9. Clear KPKD and KPKR status bit(s) by writing to a .1.,
     *    set the KPKR synchronizer chain by writing "1" to KRSS register,
     *    clear the KPKD synchronizer chain by writing "1" to KDSC register */
    KPP_KPSR = KPP_KPSR_KRSS | KPP_KPSR_KDSC | KPP_KPSR_KPKR | KPP_KPSR_KPKD;

    /* 10. Re-enable the appropriate keypad interrupt(s) so that the KDIE
     *     detects a key hold condition, or the KRIE detects a key-release
     *     event. */
    if ((button & ~BUTTON_POWER) != BUTTON_NONE)
        KPP_KPSR |= KPP_KPSR_KRIE;
    else
        KPP_KPSR |= KPP_KPSR_KDIE;

    restore_irq(oldlevel);

    int_btn = button;
}

bool button_hold(void)
{
    return _button_hold();
}

#ifdef HAVE_HEADPHONE_DETECTION
/* Headphone driver pushes the data here */
void button_headphone_set(int button)
{
    int oldstatus = disable_irq_save();
    int_btn = (int_btn & ~BUTTON_REMOTE) | button;
    restore_irq(oldstatus);
}
#endif

int button_read_device(void)
{
    /* Simple poll of GPIO status */
    hold_button = _button_hold();

#ifndef BOOTLOADER
    /* Backlight hold handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif

    /* Enable the keypad interrupt to cause it to fire if a key is down.
     * KPP_HANDLER will clear and disable it after the scan. If no key
     * is depressed then this bit will already be set in waiting for the
     * first key down event. */
    KPP_KPSR |= KPP_KPSR_KDIE;

#ifdef HAVE_HEADPHONE_DETECTION
    /* If hold, ignore any pressed button. Remote has its own hold
     * switch, so return state regardless. */
    return hold_button ? (int_btn & BUTTON_REMOTE) : int_btn;
#else
    /* If hold, ignore any pressed button.  */
    return hold_button ? BUTTON_NONE : int_btn;
#endif
}

/* This is called from the mc13783 interrupt thread */
void button_power_event(void)
{
    bool pressed =
        (mc13783_read(MC13783_INTERRUPT_SENSE1) & MC13783_ONOFD1S) == 0;

    /* Prevent KPP_HANDLER from changing things */
    int oldlevel = disable_irq_save();

    if (pressed)
    {
        int_btn |= BUTTON_POWER;
    }
    else
    {
        int_btn &= ~BUTTON_POWER;
    }

    restore_irq(oldlevel);
}

void button_init_device(void)
{
#ifdef BOOTLOADER
    /* Can be called more than once in the bootloader */
    if (initialized)
        return;

    initialized = true;
#endif

    /* Enable keypad clock */
    imx31_clkctl_module_clock_gating(CG_KPP, CGM_ON_ALL);

    /* 1. Enable number of rows in keypad (KPCR[4:0])
     *
     * Configure the rows/cols in KPP
     * LSB nybble in KPP is for 5 rows
     * MSB nybble in KPP is for 3 cols */
    KPP_KPCR |= 0x1f;

    /* 2. Write 0's to KPDR[10:8] */
    KPP_KPDR &= ~(0x7 << 8);

    /* 3. Configure the keypad columns as open-drain (KPCR[10:8]). */
    KPP_KPCR |= (0x7 << 8);

    /* 4. Configure columns as output, rows as input (KDDR[10:8,4:0]) */
    KPP_KDDR = (KPP_KDDR | (0x7 << 8)) & ~0x1f;

    /* 5. Clear the KPKD Status Flag and Synchronizer chain.
     * 6. Set the KDIE control bit bit. */
    KPP_KPSR = KPP_KPSR_KDIE | KPP_KPSR_KRSS | KPP_KPSR_KDSC | KPP_KPSR_KPKD;

    /* KPP IRQ at priority 3 */
    avic_enable_int(KPP, IRQ, 3, KPP_HANDLER);

    button_power_event();
    mc13783_enable_event(MC13783_ONOFD1_EVENT);

#ifdef HAVE_HEADPHONE_DETECTION
    headphone_init();
#endif
}

#ifdef BUTTON_DRIVER_CLOSE
void button_close_device(void)
{
    int oldlevel = disable_irq_save();

    avic_disable_int(KPP);
    KPP_KPSR &= ~(KPP_KPSR_KRIE | KPP_KPSR_KDIE);
    int_btn = BUTTON_NONE;

    restore_irq(oldlevel);
}
#endif /* BUTTON_DRIVER_CLOSE */
