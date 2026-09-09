/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

/* Initialise DRAM and return to the BootROM USB loop. Preserve its
 * registers and avoid using the display or USB controller. */

#include "config.h"
#include "x1600.h"
#include "system.h"
#include "ddr-x1600.h"
#include "boot-x1600.h"
#include "x1600/cpm.h"
#include "ost-ingenic.h"

/* Scratch block the host reads back; the assignments below are the layout */
#define STAGE1_RESULT_ADDR  0x8000f000
#define STAGE1_MAGIC        0xc0def00d
#define STAGE1_RESULT_WORDS 14

void usb_stage1_main(void)
{
    volatile uint32_t* result = (volatile uint32_t*)STAGE1_RESULT_ADDR;
    int rc, i;

    for (i = 0; i < STAGE1_RESULT_WORDS; ++i)
        result[i] = 0;
    result[0] = STAGE1_MAGIC;
    result[1] = 0xffffffff;     /* "still running" */

    /* No clk_init_early(): x1600_ddr_init() owns the DDR clock path */

    init_ost();

    rc = x1600_ddr_init(&x1600_ddr_param_hibyr1);

    result[1] = (uint32_t)rc;
    result[2] = ddr_last_status.magic;
    result[3] = ddr_last_status.step;
    result[4] = ddr_last_status.err;
    for (i = 0; i < 8; ++i)
        result[5 + i] = ddr_last_status.reg_snapshot[i];

    /* Reset inherited boot flags as in spl_main(). Leave CLK_INIT clear
     * and set USB_BOOT for recovery. */
    result[13] = REG_CPM_CPSPR;     /* sample BEFORE we overwrite it */

    init_boot_flags();
    set_boot_flag(BOOT_FLAG_USB_BOOT);

    /* Return to the BootROM; spl-start.S's epilogue restores every register */
}
