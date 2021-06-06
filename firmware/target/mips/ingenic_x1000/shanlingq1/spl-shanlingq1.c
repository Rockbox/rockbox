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

/* Available boot options */
#define BOOTOPTION_ROCKBOX  0
#define BOOTOPTION_ORIG_FW  1
#define BOOTOPTION_RECOVERY 2

// nb. these are the same as the M3K
// The OF's SPL is also the "X-loader" which afaik is ingenic code
// it can be dug up from github but provenance unknown
static const char normal_cmdline[] = "mem=64M@0x0\
 no_console_suspend\
 console=ttyS2,115200n8\
 lpj=5009408\
 ip=off\
 init=/linuxrc\
 ubi.mtd=3\
 root=ubi0:rootfs\
 ubi.mtd=4\
 rootfstype=ubifs\
 rw\
 loglevel=8";

static const char recovery_cmdline[] = "mem=64M@0x0\
 no_console_suspend\
 console=ttyS2,115200n8\
 lpj=5009408\
 ip=off";

const struct spl_boot_option spl_boot_options[] = {
    {
        /* 1st unused NAND page at 26 KiB
         * Partition size is 1 MiB, we only use 102 KiB for the
         * sake of uniformity with the M3K */
        .nand_addr = 0x6800,
        .nand_size = 0x19800,
        .load_addr = X1000_DRAM_END - 0x19800,
        .exec_addr = X1000_DRAM_BASE,
        .cmdline = NULL,
    },
    {
        /* Original firmware */
        .nand_addr = 0,
        .nand_size = 0,
        .load_addr = 0,
        .exec_addr = 0,
        .cmdline = normal_cmdline,
    },
    {
        /* Recovery image */
        .nand_addr = 0,
        .nand_size = 0,
        .load_addr = 0,
        .exec_addr = 0,
        .cmdline = recovery_cmdline,
    },
};

void spl_error(void)
{
    // FIXME(q1): implement
    while(1);
}

int spl_get_boot_option(void)
{
    // FIXME(q1): implement
    return BOOTOPTION_ROCKBOX;
}

void spl_handle_pre_boot(int bootopt)
{
    // FIXME(q1): check and verify

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
        /* TODO: Original firmware needs a lot of other clocks turned on */
    }
}
