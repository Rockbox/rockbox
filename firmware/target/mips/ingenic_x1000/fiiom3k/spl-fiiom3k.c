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

/* Boot select button state must remain stable for this duration
 * before the choice will be accepted. Currently 100ms.
 */
#define BTN_STABLE_TIME (100 * (X1000_EXCLK_FREQ / 4000))

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
        /* Rockbox: the first unused NAND page is 26 KiB in, and the
         * remainder of the block is unused, giving us 102 KiB to use.
         */
        .nand_addr = 0x6800,
        .nand_size = 0x19800,
        .load_addr = X1000_DRAM_END - 0x19800,
        .exec_addr = X1000_DRAM_BASE,
        .cmdline = NULL,
    },
    {
        /* Original firmware */
        .nand_addr = 0x20000,
        .nand_size = 0x400000,
        .load_addr = 0x80efffc0,
        .exec_addr = 0x80f00000,
        .cmdline = normal_cmdline,
    },
    {
        /* Recovery image */
        .nand_addr = 0x420000,
        .nand_size = 0x500000,
        .load_addr = 0x80efffc0,
        .exec_addr = 0x80f00000,
        .cmdline = recovery_cmdline,
    },
};

void spl_error(void)
{
    const uint32_t pin = (1 << 24);

    /* Turn on button light */
    jz_clr(GPIO_INT(GPIO_C), pin);
    jz_set(GPIO_MSK(GPIO_C), pin);
    jz_clr(GPIO_PAT1(GPIO_C), pin);
    jz_set(GPIO_PAT0(GPIO_C), pin);

    while(1) {
        /* Turn it off */
        mdelay(100);
        jz_set(GPIO_PAT0(GPIO_C), pin);

        /* Turn it on */
        mdelay(100);
        jz_clr(GPIO_PAT0(GPIO_C), pin);
    }
}

int spl_get_boot_option(void)
{
    const uint32_t pinmask = (1 << 17) | (1 << 19);

    uint32_t pin = 1, lastpin = 0;
    uint32_t deadline = 0;
    /* Iteration count guards against unlikely case of broken buttons
     * which never stabilize; if this occurs, we always boot Rockbox. */
    int iter_count = 0;
    const int max_iter_count = 30;

    /* Configure the button GPIOs as inputs */
    gpio_config(GPIO_A, pinmask, GPIO_INPUT);

    /* Poll the pins for a short duration to detect a keypress */
    do {
        lastpin = pin;
        pin = ~REG_GPIO_PIN(GPIO_A) & pinmask;
        if(pin != lastpin) {
            /* This will always be set on the first iteration */
            deadline = __ost_read32() + BTN_STABLE_TIME;
            iter_count += 1;
        }
    } while(iter_count < max_iter_count && __ost_read32() < deadline);

    if(iter_count < max_iter_count && (pin & (1 << 17))) {
        if(pin & (1 << 19))
            return BOOTOPTION_RECOVERY; /* Play+Volume Up */
        else
            return BOOTOPTION_ORIG_FW; /* Play */
    } else {
        return BOOTOPTION_ROCKBOX; /* Volume Up or no buttons */
    }
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

    /* System clock setup -- common to Rockbox and FiiO firmware
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
