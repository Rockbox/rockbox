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
#include "clk-x1000.h"
#include "system.h"
#include "x1000/cpm.h"
#include "x1000/ost.h"
#include "x1000/ddrc.h"
#include "x1000/ddrc_apb.h"
#include "x1000/ddrphy.h"

#ifdef FIIO_M3K
# define SPL_DDR_MEMORYSIZE  64
# define SPL_DDR_AUTOSR_EN   1
# define SPL_DDR_NEED_BYPASS 1
#else
# error "please add SPL memory definitions"
#endif

/* Note: This is based purely on disassembly of the SPL from the FiiO M3K.
 * The code there is somewhat generic and corresponds roughly to Ingenic's
 * U-Boot code, but isn't entirely the same.
 *
 * I converted all the runtime conditionals to compile-time ones in order to
 * save code space, since they should be constant for any given target.
 *
 * I haven't bothered to decode all the register fields. Some of the values
 * written are going to bits documented as "Reserved" by Ingenic, but their
 * documentation doesn't seem completely reliable, so either these are bits
 * which _do_ have a purpose, or they're only defined on other Ingenic CPUs.
 *
 * The DDR PHY registers appear to be from Synopsys "PHY Utility Block Lite".
 * These aren't documented by Ingenic, but the addresses and names can be found
 * in their U-Boot code.
 */
static void ddr_init(void)
{
    REG_CPM_DRCG = 0x73;
    mdelay(3);
    REG_CPM_DRCG = 0x71;
    mdelay(3);
    REG_DDRC_APB_PHYRST_CFG = 0x1a00001;
    mdelay(3);
    REG_DDRC_APB_PHYRST_CFG = 0;
    mdelay(3);
    REG_DDRC_CTRL = 0xf00000;
    mdelay(3);
    REG_DDRC_CTRL = 0;
    mdelay(3);

    REG_DDRC_CFG = 0xa468a6c;
    REG_DDRC_CTRL = 2;
    REG_DDRPHY_DTAR = 0x150000;
    REG_DDRPHY_DCR = 0;
    REG_DDRPHY_MR0 = 0x42;
    REG_DDRPHY_MR2 = 0x98;
    REG_DDRPHY_PTR0 = 0x21000a;
    REG_DDRPHY_PTR1 = 0xa09c40;
    REG_DDRPHY_PTR2 = 0x280014;
    REG_DDRPHY_DTPR0 = 0x1a69444a;
    REG_DDRPHY_DTPR1 = 0x180090;
    REG_DDRPHY_DTPR2 = 0x1ff99428;
    REG_DDRPHY_DXGCR(0) = 0x90881;
    REG_DDRPHY_DXGCR(1) = 0x90881;
    REG_DDRPHY_DXGCR(2) = 0x90e80;
    REG_DDRPHY_DXGCR(3) = 0x90e80;
    REG_DDRPHY_PGCR = 0x1042e03;
    REG_DDRPHY_ACIOCR = 0x30c00813;
    REG_DDRPHY_DXCCR = 0x4912;

    int i = 10000;
    while(i > 0 && REG_DDRPHY_PGSR != 7 && REG_DDRPHY_PGSR != 0x1f)
        i -= 1;
    if(i == 0)
        spl_error();

#if SPL_DDR_NEED_BYPASS
    REG_DDRPHY_ACDLLCR = 0x80000000;
    REG_DDRPHY_DSGCR &= ~0x10;
    REG_DDRPHY_DLLGCR |= 0x800000;
    REG_DDRPHY_PIR = 0x20020041;
#else
    REG_DDRPHY_PIR = 0x41;
#endif

    while(i > 0 && REG_DDRPHY_PGSR != 0xf && REG_DDRPHY_PGSR != 0x1f)
        i -= 1;
    if(i == 0)
        spl_error();

    REG_DDRC_APB_PHYRST_CFG = 0x400000;
    mdelay(3);
    REG_DDRC_APB_PHYRST_CFG = 0;
    mdelay(3);

    REG_DDRC_CFG = 0xa468aec;
    REG_DDRC_CTRL = 2;
#if SPL_DDR_NEED_BYPASS
    REG_DDRPHY_PIR = 0x20020081;
#else
    REG_DDRPHY_PIR = 0x85;
#endif

    i = 500000;
    while(REG_DDRPHY_PGSR != 0x1f) {
        if(REG_DDRPHY_PGSR & 0x70)
            break;
        i -= 1;
    }

    if(i == 0)
        spl_error();

    if((REG_DDRPHY_PGSR & 0x60) != 0 && REG_DDRPHY_PGSR != 0)
        spl_error();

    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 10;
    REG_DDRC_CTRL = 0;
    REG_DDRC_CFG  = 0xa468a6c;
    REG_DDRC_TIMING1 = 0x2050501;
    REG_DDRC_TIMING2 = 0x4090404;
    REG_DDRC_TIMING3 = 0x2704030d;
    REG_DDRC_TIMING4 = 0xb7a0251;
    REG_DDRC_TIMING5 = 0xff090200;
    REG_DDRC_TIMING6 = 0xa0a0202;
#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_MMAP0   = 0x20fc;
    REG_DDRC_MMAP1   = 0x2400;
#elif SPL_DDR_MEMORYSIZE == 32
    REG_DDRC_MMAP0   = 0x20fe;
    REG_DDRC_MMAP1   = 0x2200;
#else
# error "Unsupported DDR_MEMORYSIZE"
#endif
    REG_DDRC_CTRL    = 10;
    REG_DDRC_REFCNT  = 0x2f0003;
    REG_DDRC_CTRL    = 0xc91e;

#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_REMAP1 = 0x03020c0b;
    REG_DDRC_REMAP2 = 0x07060504;
    REG_DDRC_REMAP3 = 0x000a0908;
    REG_DDRC_REMAP4 = 0x0f0e0d01;
    REG_DDRC_REMAP5 = 0x13121110;
#elif SPL_DDR_MEMORYSIZE == 32
    REG_DDRC_REMAP1 = 0x03020b0a;
    REG_DDRC_REMAP2 = 0x07060504;
    REG_DDRC_REMAP3 = 0x01000908;
    REG_DDRC_REMAP4 = 0x0f0e0d0c;
    REG_DDRC_REMAP5 = 0x13121110;
#else
# error "Unsupported DDR_MEMORYSIZE"
#endif

    REG_DDRC_STATUS &= ~0x40;

#if SPL_DDR_AUTOSR_EN
#if SPL_DDR_NEED_BYPASS
    jz_writef(CPM_DDRCDR, GATE_EN(1));
    REG_DDRC_APB_CLKSTP_CFG = 0x9000000f;
#else
    REG_DDRC_DLP = 0;
#endif
#endif

    REG_DDRC_AUTOSR_EN = SPL_DDR_AUTOSR_EN;
}

static void init(void)
{
    /* from original firmware SPL */
    REG_CPM_PSWC0ST = 0x00;
    REG_CPM_PSWC1ST = 0x10;
    REG_CPM_PSWC2ST = 0x18;
    REG_CPM_PSWC3ST = 0x08;

    /* enable MPLL */
#if X1000_EXCLK_FREQ == 24000000
    /* 24 * (24+1) = 600 MHz */
    jz_writef(CPM_MPCR, ENABLE(1), BS(1), PLLN(0), PLLM(24), PLLOD(0));
#elif X1000_EXCLK_FREQ == 26000000
    /* 26 * (22+1) = 598 MHz */
    jz_writef(CPM_MPCR, ENABLE(1), BS(1), PLLN(0), PLLM(22), PLLOD(0));
#else
# error "unknown EXCLK frequency"
#endif
    while(jz_readf(CPM_MPCR, ON) == 0);

    /* set DDR clock to MPLL/3 = 200 MHz */
    jz_writef(CPM_CLKGR, DDR(0));
    clk_set_ddr(X1000_CLK_MPLL, 3);

    /* start OST so we can use mdelay/udelay */
    jz_writef(CPM_CLKGR, OST(0));
    jz_writef(OST_CTRL, PRESCALE2_V(BY_4));
    jz_writef(OST_CLEAR, OST2(1));
    jz_write(OST_2CNTH, 0);
    jz_write(OST_2CNTL, 0);
    jz_setf(OST_ENABLE, OST2);

    /* init DDR memory */
    ddr_init();
}

/* This variable is defined by the maskrom. It's simply the level of the
 * boot_sel[2:0] pins (GPIOs B28-30) at boot time. Meaning of the bits:
 *
 * boot_sel[2] boot_sel[1] boot_sel[0]      Description
 * -----------------------------------------------------------------
 * 1           X           X                EXCLK is 26 MHz
 * 0           X           X                EXCLK is 24 MHz
 * X           1           1                Boot from SFC0
 * X           0           1                Boot from MSC0
 * X           1           0                Boot from USB 2.0 device
 * -----------------------------------------------------------------
 * Source: X1000 PM pg. 687, "XBurst Boot ROM Specification"
 */
extern const uint32_t boot_sel;

void spl_main(void)
{
    /* Basic hardware init */
    init();

    /* If doing a USB boot, host PC will upload 2nd stage itself,
     * we should not load anything from flash or change clocks. */
    if((boot_sel & 3) == 2)
        return;

    /* Just pass control to the target... */
    spl_target_boot();
}
