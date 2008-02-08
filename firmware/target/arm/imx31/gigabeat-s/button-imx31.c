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
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"
#include "backlight-target.h"
#include "debug.h"
#include "stdio.h"

/* Most code in here is taken from the Linux BSP provided by Freescale
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved. */

void button_init_device(void)
{
    unsigned int reg_val;
    /* Enable keypad clock */
    CLKCTL_CGR1 |= (3 << 2*10);
    
    /* Enable number of rows in keypad (KPCR[7:0])
     * Configure keypad columns as open-drain (KPCR[15:8])
     *
     * Configure the rows/cols in KPP
     * LSB nibble in KPP is for 8 rows
     * MSB nibble in KPP is for 8 cols
     */
#if 0
    KPP_KPCR = (0xff << 8) | 0xff;
    /* Write 0's to KPDR[15:8] */
    reg_val = KPP_KPDR;
    reg_val &= 0x00ff;
    KPP_KPDR = reg_val;

    /* Configure columns as output, rows as input (KDDR[15:0]) */
    KPP_KDDR = 0xff00;
#endif

    KPP_KPSR = (1 << 3) | (1 << 2);
}

inline bool button_hold(void)
{
    return GPIO3_DR & 0x10;
}

int button_read_device(void)
{
    unsigned short reg_val;
    int col, row;
    int button = BUTTON_NONE;

    if(!button_hold()) {
        for (col = 0; col < 3; col++) {  /* Col */
            /* 1. Write 1s to KPDR[15:8] setting column data to 1s */
            reg_val = KPP_KPDR;
            reg_val |= 0xff00;
            KPP_KPDR = reg_val;

            /*
             * 2. Configure columns as totem pole outputs(for quick
             * discharging of keypad capacitance)
             */
            reg_val = KPP_KPCR;
            reg_val &= 0x00ff;
            KPP_KPCR = reg_val;

            /* Give the columns time to discharge */
            udelay(2);

            /* 3. Configure columns as open-drain */
            reg_val = KPP_KPCR;
            reg_val |= ((1 << 8) - 1) << 8;
            KPP_KPCR = reg_val;

            /* 4. Write a single column to 0, others to 1.
             * 5. Sample row inputs and save data. Multiple key presses
             * can be detected on a single column.
             * 6. Repeat steps 1 - 5 for remaining columns.
             */

            /* Col bit starts at 8th bit in KPDR */
            reg_val = KPP_KPDR;
            reg_val &= ~(1 << (8 + col));
            KPP_KPDR = reg_val;

            /* Delay added to avoid propagating the 0 from column to row
             * when scanning. */
            udelay(2);

            /* Read row input */
            reg_val = KPP_KPDR;
            for (row = 0; row < 5; row++) {  /* sample row */
                if (!(reg_val & (1 << row))) {
                    if(row == 0) {
                            if(col == 0) {
                                button |= BUTTON_LEFT;
                            }
                            if(col == 1) {
                                button |= BUTTON_BACK;
                            }
                            if(col == 2) {
                                button |= BUTTON_VOL_UP;
                            }
                    } else if(row == 1) {
                            if(col == 0) {
                                button |= BUTTON_UP;
                            }
                            if(col == 1) {
                                button |= BUTTON_MENU;
                            }
                            if(col == 2) {
                                button |= BUTTON_VOL_DOWN;
                            }
                    } else if(row == 2) {
                            if(col == 0) {
                                button |= BUTTON_DOWN;
                            }
                            if(col == 2) {
                                button |= BUTTON_PREV;
                            }
                    } else if(row == 3) {
                            if(col == 0) {
                                button |= BUTTON_RIGHT;
                            }
                            if(col == 2) {
                                button |= BUTTON_PLAY;
                            }
                    } else if(row == 4) {
                            if(col == 0) {
                                button |= BUTTON_SELECT;
                            }
                            if(col == 2) {
                                button |= BUTTON_NEXT;
                            }
                    }
                }
            }
        }

        /*
         * 7. Return all columns to 0 in preparation for standby mode.
         * 8. Clear KPKD and KPKR status bit(s) by writing to a .1.,
         * set the KPKR synchronizer chain by writing "1" to KRSS register,
         * clear the KPKD synchronizer chain by writing "1" to KDSC register
         */
        reg_val = 0x00;
        KPP_KPDR = reg_val;
        reg_val = KPP_KPDR;
        reg_val = KPP_KPSR;
        reg_val |= 0xF;
        KPP_KPSR = reg_val;
    }

    return button;
}
