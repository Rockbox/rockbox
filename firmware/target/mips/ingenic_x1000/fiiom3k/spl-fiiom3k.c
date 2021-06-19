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

#include "system.h"
#include "ucl_decompress.h"
#include "spl-x1000.h"
#include "gpio-x1000.h"
#include "clk-x1000.h"
#include "nand-x1000.h"
#include "x1000/cpm.h"
#include <string.h>
#include <stdint.h>

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

/* Entry point function type, defined to be Linux compatible. */
typedef void(*entry_fn)(int, char**, int, int);

struct spl_boot_option {
    uint32_t nand_addr;
    uint32_t nand_size;
    uint32_t load_addr;
    uint32_t exec_addr;
    const char* cmdline; /* for Linux */
};

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

nand_drv* alloc_nand_drv(uint32_t laddr, uint32_t lsize)
{
    size_t need_size = sizeof(nand_drv) +
                       NAND_DRV_SCRATCHSIZE +
                       NAND_DRV_MAXPAGESIZE;

    /* find a hole to keep the buffers */
    uintptr_t addr;
    if(X1000_SDRAM_BASE + need_size <= laddr)
        addr = X1000_SDRAM_BASE;
    else
        addr = CACHEALIGN_UP(X1000_SDRAM_BASE + laddr + lsize);

    uint8_t* page_buf = (uint8_t*)addr;
    uint8_t* scratch_buf = page_buf + NAND_DRV_MAXPAGESIZE;
    nand_drv* drv = (nand_drv*)(scratch_buf + NAND_DRV_SCRATCHSIZE);

    drv->page_buf = page_buf;
    drv->scratch_buf = scratch_buf;
    drv->refcount = 0;
    return drv;
}

void spl_target_boot(void)
{
    int opt_index = spl_get_boot_option();
    const struct spl_boot_option* opt = &spl_boot_options[opt_index];
    uint8_t* load_addr = (uint8_t*)opt->load_addr;

    /* Clock setup etc. */
    spl_handle_pre_boot(opt_index);

    /* Since the GPIO refactor, the SFC driver no longer assigns its own pins.
     * We don't want to call gpio_init(), to keep code size down. Assign them
     * here manually, we shouldn't need any other pins. */
    gpioz_configure(GPIO_A, 0x3f << 26, GPIOF_DEVICE(1));

    /* Open NAND chip */
    nand_drv* ndrv = alloc_nand_drv(opt->load_addr, opt->nand_size);
    int rc = nand_open(ndrv);
    if(rc)
        spl_error();

    /* For OF only: load DMA coprocessor's firmware from flash */
    if(opt_index != BOOTOPTION_ROCKBOX) {
        rc = nand_read_bytes(ndrv, 0x4000, 0x2000, (uint8_t*)0xb3422000);
        if(rc)
            goto nand_err;
    }

    /* Read the firmware */
    rc = nand_read_bytes(ndrv, opt->nand_addr, opt->nand_size, load_addr);
    if(rc)
        goto nand_err;

    /* Rockbox doesn't need the NAND; for the OF, we should leave it open */
    if(opt_index == BOOTOPTION_ROCKBOX)
        nand_close(ndrv);

    /* Kernel arguments pointer, for Linux only */
    char** kargv = (char**)0x80004000;

    if(!opt->cmdline) {
        /* Uncompress the rockbox bootloader */
        uint32_t out_size = X1000_DRAM_END - opt->exec_addr;
        int rc = ucl_unpack(load_addr, opt->nand_size,
                            (uint8_t*)opt->exec_addr, &out_size);
        if(rc != UCL_E_OK)
            spl_error();
    } else {
        /* Set kernel args */
        kargv[0] = 0;
        kargv[1] = (char*)opt->cmdline;
    }

    entry_fn entry = (entry_fn)opt->exec_addr;
    commit_discard_idcache();
    entry(2, kargv, 0, 0);
    __builtin_unreachable();

  nand_err:
    nand_close(ndrv);
    spl_error();
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
    gpioz_configure(GPIO_A, pinmask, GPIOF_INPUT);

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

    if(bootopt == BOOTOPTION_ROCKBOX) {
        /* We don't use MPLL in Rockbox, so run everything from APLL. */
        clk_set_ccr_div(1, 2, 5, 5, 10);
        clk_set_ccr_mux(CLKMUX_SCLK_A(APLL) | CLKMUX_CPU(SCLK_A) |
                        CLKMUX_AHB0(SCLK_A) | CLKMUX_AHB2(SCLK_A));
        clk_set_ddr(X1000_CLK_SCLK_A, 5);

        /* Turn off MPLL */
        jz_writef(CPM_MPCR, ENABLE(0));
    } else {
        /* Typical ingenic setup -- 1008 MHz APLL, 600 MHz MPLL
         * with APLL driving the CPU/L2 and MPLL driving busses. */
        clk_set_ccr_div(1, 2, 3, 3, 6);
        clk_set_ccr_mux(CLKMUX_SCLK_A(APLL) | CLKMUX_CPU(SCLK_A) |
                        CLKMUX_AHB0(MPLL) | CLKMUX_AHB2(MPLL));

        /* Mimic OF's clock gating setup */
        jz_writef(CPM_CLKGR, PDMA(0), PCM(1), MAC(1), LCD(1),
                  MSC0(1), MSC1(1), OTG(1), CIM(1));

        /* We now need to define the clock tree by fiddling with
         * various CPM registers. Although the kernel has code to
         * set up the clock tree itself, it isn't used and relies
         * on the SPL for this. */
        jz_writef(CPM_I2SCDR, CS_V(EXCLK));
        jz_writef(CPM_PCMCDR, CS_V(EXCLK));

        jz_writef(CPM_MACCDR, CLKSRC_V(MPLL), CE(1), STOP(1), CLKDIV(0xfe));
        while(jz_readf(CPM_MACCDR, BUSY));

        jz_writef(CPM_LPCDR, CLKSRC_V(MPLL), CE(1), STOP(1), CLKDIV(0xfe));
        while(jz_readf(CPM_LPCDR, BUSY));

        jz_writef(CPM_MSC0CDR, CLKSRC_V(MPLL), CE(1), STOP(1), CLKDIV(0xfe));
        while(jz_readf(CPM_MSC0CDR, BUSY));

        jz_writef(CPM_MSC1CDR, CE(1), STOP(1), CLKDIV(0xfe));
        while(jz_readf(CPM_MSC1CDR, BUSY));

        jz_writef(CPM_CIMCDR, CLKSRC_V(MPLL), CE(1), STOP(1), CLKDIV(0xfe));
        while(jz_readf(CPM_CIMCDR, BUSY));

        jz_writef(CPM_USBCDR, CLKSRC_V(EXCLK), CE(1), STOP(1));
        while(jz_readf(CPM_USBCDR, BUSY));

        /* Handle UART initialization */
        gpioz_configure(GPIO_C, 3 << 30, GPIOF_DEVICE(1));
        jz_writef(CPM_CLKGR, UART2(0));

        /* TODO: Stop being lazy and make this human readable */
        volatile uint8_t* ub = (volatile uint8_t*)0xb0032000;
        ub[0x04] = 0;
        ub[0x08] = 0xef;
        ub[0x20] = 0xfc;
        ub[0x0c] = 3;
        uint8_t uv = ub[0x0c];
        ub[0x0c] = uv | 0x80;
        ub[0x04] = 0;
        ub[0x00] = 0x0d;
        ub[0x24] = 0x10;
        ub[0x28] = 0;
        ub[0x0c] = uv & 0x7f;
        ub[0x08] = 0x17;
    }
}
