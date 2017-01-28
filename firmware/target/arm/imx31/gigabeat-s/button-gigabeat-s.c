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
#include "backlight-target.h"
#include "avic-imx31.h"
#include "ccm-imx31.h"
#include "mc13783.h"

/* Most code in here is taken from the Linux BSP provided by Freescale
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved. */
#ifdef BOOTLOADER
static bool initialized     = false;
#endif

static unsigned long ext_btn = BUTTON_NONE; /* Buttons not on KPP */
static bool hold_button     = false;
#ifndef BOOTLOADER
static bool hold_button_old = false;
#endif

#define _button_hold() (GPIO3_DR & 0x10)

/* Scan the keypad port and return the pressed buttons */
static int kpp_scan(void)
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

    int button = BUTTON_NONE;
    int oldlevel = disable_irq_save();
    int col;

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

    restore_irq(oldlevel);

    return button;
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
    ext_btn = (ext_btn & ~BUTTON_REMOTE) | button;
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

    int button = ext_btn;

    /* Check status for key down flag and scan port if so indicated. */
    if (KPP_KPSR & KPP_KPSR_KPKD)
        button |= kpp_scan();

#ifdef HAVE_HEADPHONE_DETECTION
    /* If hold, ignore any pressed button. Remote has its own hold
     * switch, so return state regardless. */
    return hold_button ? (button & BUTTON_REMOTE) : button;
#else
    /* If hold, ignore any pressed button.  */
    return hold_button ? BUTTON_NONE : button;
#endif
}

/* Helper to update the power button status */
static void power_button_update(bool pressed)
{
    bitmod32(&ext_btn, pressed ? BUTTON_POWER : 0, BUTTON_POWER);
}

/* Power button event - called from PMIC ISR */
void MC13783_EVENT_CB_ONOFD1(void)
{
    power_button_update(!mc13783_event_sense());
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
    ccm_module_clock_gating(CG_KPP, CGM_ON_RUN_WAIT);

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
    KPP_KPSR = KPP_KPSR_KRSS | KPP_KPSR_KDSC | KPP_KPSR_KPKD;

    power_button_update(!(mc13783_read(MC13783_INTERRUPT_SENSE1)
                            & MC13783_ONOFD1S));
    mc13783_enable_event(MC13783_INT_ID_ONOFD1, true);

#ifdef HAVE_HEADPHONE_DETECTION
    headphone_init();
#endif
}

#ifdef BUTTON_DRIVER_CLOSE
void button_close_device(void)
{
    if (!initialized)
        return;

    /* Assumes HP detection is not available */
    initialized = false;

    mc13783_enable_event(MC13783_INT_ID_ONOFD1, false);
    ext_btn = BUTTON_NONE;
}
#endif /* BUTTON_DRIVER_CLOSE */
