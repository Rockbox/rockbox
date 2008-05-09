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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

/* Most code in here is taken from the Linux BSP provided by Freescale
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved. */

static bool headphones_detect = false;
static uint32_t int_btn     = BUTTON_NONE;
static bool hold_button     = false;
static bool hold_button_old = false;
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
        int i;

        /* 2. Write 1s to KPDR[10:8] setting column data to 1s */
        KPP_KPDR |= (0x7 << 8);

        /* 3. Configure columns as totem pole outputs(for quick
         * discharging of keypad capacitance) */
        KPP_KPCR &= ~(0x7 << 8);

        /* Give the columns time to discharge */
        for (i = 0; i < 128; i++) /* TODO: find minimum safe delay */
            asm volatile ("");

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
        for (i = 0; i < 128; i++) /* TODO: find minimum safe delay */
            asm volatile ("");

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

void button_init_device(void)
{
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
     * 6. Set the KDIE control bit, and set the KRIE control
     *    bit (to force immediate scan). */
    KPP_KPSR = KPP_KPSR_KRIE | KPP_KPSR_KDIE | KPP_KPSR_KRSS |
               KPP_KPSR_KDSC | KPP_KPSR_KPKR | KPP_KPSR_KPKD;

    /* KPP IRQ at priority 3 */
    avic_enable_int(KPP, IRQ, 3, KPP_HANDLER);
}

bool button_hold(void)
{
    return _button_hold();
}

int button_read_device(void)
{
    /* Simple poll of GPIO status */
    hold_button = _button_hold();

    /* Backlight hold handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }

    /* Enable the keypad interrupt to cause it to fire if a key is down.
     * KPP_HANDLER will clear and disable it after the scan. If no key
     * is depressed then this bit will already be set in waiting for the
     * first key down event. */
    KPP_KPSR |= KPP_KPSR_KDIE;

    /* If hold, ignore any pressed button */
    return hold_button ? BUTTON_NONE : int_btn;
}

/* This is called from the mc13783 interrupt thread */
void button_power_set_state(bool pressed)
{
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

/* This is called from the mc13783 interrupt thread */
void set_headphones_inserted(bool inserted)
{
    headphones_detect = inserted;
}

/* This is called from the mc13783 interrupt thread */
/* TODO: Just do a post to the button queue directly - implement the
 * appropriate variant in the driver. */
bool headphones_inserted(void)
{
    return headphones_detect;
}
