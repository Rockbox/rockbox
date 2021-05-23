/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "spl-x1000.h"
#include "gpio-x1000.h"
#include "clk-x1000.h"
#include "system.h"
#include <string.h>

/* TODO: this is a stub and there's probably going to be a lot of code shared
 * with the M3K (and probably, other X1000 targets would work the same way) */

/* Available boot options */
#define BOOTOPTION_ROCKBOX  0

const struct spl_boot_option spl_boot_options[] = {
    {
        .nand_addr = 0x6800,
        .nand_size = 0x19800,
        .load_addr = X1000_DRAM_END - 0x19800,
        .exec_addr = X1000_DRAM_BASE,
        .cmdline = NULL,
    },
};

void spl_error(void)
{
    while(1);
}

int spl_get_boot_option(void)
{
    return BOOTOPTION_ROCKBOX;
}

void spl_handle_pre_boot(int bootopt)
{
    /* Move system to EXCLK so we can manipulate the PLLs */
    clk_set_ccr_mux(CLKMUX_SCLK_A(EXCLK) | CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(SCLK_A) | CLKMUX_AHB2(SCLK_A));
    clk_set_ccr_div(1, 1, 1, 1, 1);

    /* Enable APLL @ 1008 MHz (24 MHz EXCLK * 42 = 1008 MHz) */
    jz_writef(CPM_APCR, BS(1), PLLM(41), PLLN(0), PLLOD(0), ENABLE(1));
    while(jz_readf(CPM_APCR, ON) == 0);

    /* System clock setup -- common to Rockbox and OF
     * ----
     * CPU at 1 GHz, L2 cache at 500 MHz
     * AHB0 and AHB2 at 200 MHz
     * PCLK at 100 MHz
     * DDR at 200 MHz
     */
    clk_set_ccr_div(1, 2, 5, 5, 10);
    clk_set_ccr_mux(CLKMUX_SCLK_A(APLL) | CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(SCLK_A) | CLKMUX_AHB2(SCLK_A));

    if(bootopt == BOOTOPTION_ROCKBOX) {
        /* We don't use MPLL in Rockbox, so switch DDR memory to APLL */
        clk_set_ddr(X1000_CLK_SCLK_A, 5);

        /* Turn off MPLL */
        jz_writef(CPM_MPCR, ENABLE(0));
    } else {

    }
}
