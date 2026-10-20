/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/* The USB boot part of the Rockbox SPL (firmware/target/mips/ingenic_x1000/
 * spl-x1000.c): set up the MPLL and the DDR, then return to the boot ROM,
 * which then takes the stub from jztool. Keep in sync with Rockbox's.
 * SPL_DDR_MEMORYSIZE (32 or 64 MiB) is set by the Makefile. */

#include "x1000.h"

#if SPL_DDR_MEMORYSIZE != 64 && SPL_DDR_MEMORYSIZE != 32
# error "bad memory size"
#endif

#define REG_CPM_MPCR            REG32(0xb0000014)
#define CPM_MPCR_BS             (1u << 31)
#define CPM_MPCR_PLLM(x)        ((x) << 24)
#define CPM_MPCR_ENABLE         (1 << 7)
#define CPM_MPCR_ON             (1 << 0)
#define REG_CPM_DDRCDR          REG32(0xb000002c)
#define CPM_DDRCDR_CLKSRC_MPLL  (2u << 30)
#define CPM_DDRCDR_CE           (1 << 29)
#define CPM_DDRCDR_BUSY         (1 << 28)
#define CPM_DDRCDR_GATE_EN      (1 << 26)
#define REG_CPM_PSWC0ST         REG32(0xb0000090)
#define REG_CPM_PSWC1ST         REG32(0xb0000094)
#define REG_CPM_PSWC2ST         REG32(0xb0000098)
#define REG_CPM_PSWC3ST         REG32(0xb000009c)
#define REG_CPM_DRCG            REG32(0xb00000d0)
#define REG_CPM_CSR             REG32(0xb00000d4)
#define CPM_CSR_DDR_MUX         (1 << 27)
#define CPM_CLKGR_DDR           (1u << 31)

#define REG_DDRPHY_PIR          REG32(0xb3011004)
#define REG_DDRPHY_PGCR         REG32(0xb3011008)
#define REG_DDRPHY_PGSR         REG32(0xb301100c)
#define REG_DDRPHY_DLLGCR       REG32(0xb3011010)
#define REG_DDRPHY_ACDLLCR      REG32(0xb3011014)
#define REG_DDRPHY_PTR0         REG32(0xb3011018)
#define REG_DDRPHY_PTR1         REG32(0xb301101c)
#define REG_DDRPHY_PTR2         REG32(0xb3011020)
#define REG_DDRPHY_ACIOCR       REG32(0xb3011024)
#define REG_DDRPHY_DXCCR        REG32(0xb3011028)
#define REG_DDRPHY_DSGCR        REG32(0xb301102c)
#define REG_DDRPHY_DCR          REG32(0xb3011030)
#define REG_DDRPHY_DTPR0        REG32(0xb3011034)
#define REG_DDRPHY_DTPR1        REG32(0xb3011038)
#define REG_DDRPHY_DTPR2        REG32(0xb301103c)
#define REG_DDRPHY_MR0          REG32(0xb3011040)
#define REG_DDRPHY_MR2          REG32(0xb3011048)
#define REG_DDRPHY_DTAR         REG32(0xb3011054)
#define REG_DDRPHY_DXGCR(n)     REG32(0xb30111c0 + (n) * 0x40)

#define REG_DDRC_APB_CLKSTP_CFG REG32(0xb3012068)
#define REG_DDRC_APB_PHYRST_CFG REG32(0xb3012080)

#define REG_DDRC_STATUS         REG32(0xb34f0000)
#define REG_DDRC_CFG            REG32(0xb34f0004)
#define REG_DDRC_CTRL           REG32(0xb34f0008)
#define REG_DDRC_REFCNT         REG32(0xb34f0018)
#define REG_DDRC_MMAP0          REG32(0xb34f0024)
#define REG_DDRC_MMAP1          REG32(0xb34f0028)
#define REG_DDRC_TIMING(n)      REG32(0xb34f0060 + ((n) - 1) * 4)
#define REG_DDRC_REMAP(n)       REG32(0xb34f009c + ((n) - 1) * 4)
#define REG_DDRC_AUTOSR_EN      REG32(0xb34f0304)

static void mdelay(unsigned ms)
{
    uint32_t start = REG_OST_2CNTL;
    while(REG_OST_2CNTL - start < ms * 1000 * OST_TICKS_PER_US) {}
}

static void init_ost(void)
{
    REG_CPM_CLKGR &= ~CPM_CLKGR_OST;
    REG_OST_CTRL = (REG_OST_CTRL & ~OST_CTRL_PRESCALE2_MASK) |
                   OST_CTRL_PRESCALE2_BY_4;
    REG_OST_CLEAR = OST_OST2;
    REG_OST_2CNTH = 0;
    REG_OST_2CNTL = 0;
    REG_OST_ENABLE_SET = OST_OST2;
}

static void init_clk(void)
{
    REG_CPM_MPCR &= ~CPM_MPCR_ENABLE;
    while(REG_CPM_MPCR & CPM_MPCR_ON) {}

    /* 24 MHz * 25 = 600 MHz */
    REG_CPM_MPCR = CPM_MPCR_BS | CPM_MPCR_PLLM(25 - 1) | CPM_MPCR_ENABLE;
    while(!(REG_CPM_MPCR & CPM_MPCR_ON)) {}

    /* DDR: 600 MHz / 3 = 200 MHz */
    REG_CPM_DDRCDR = (REG_CPM_DDRCDR & ~0xc000000f) |
                     CPM_DDRCDR_CLKSRC_MPLL | CPM_DDRCDR_CE | (3 - 1);
    while(!(REG_CPM_CSR & CPM_CSR_DDR_MUX)) {}
    while(REG_CPM_DDRCDR & CPM_DDRCDR_BUSY) {}
    REG_CPM_DDRCDR &= ~CPM_DDRCDR_CE;
}

/* Same sequence as Rockbox's SPL, with auto-self-refresh and DLL bypass */
static int init_dram(void)
{
    REG_CPM_CLKGR &= ~CPM_CLKGR_DDR;

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

#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_CFG = 0xa468a6c;
#else
    REG_DDRC_CFG = 0xa46896c;
#endif
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
        return 1;

    REG_DDRPHY_ACDLLCR = 0x80000000;
    REG_DDRPHY_DSGCR &= ~0x10;
    REG_DDRPHY_DLLGCR |= 0x800000;
    REG_DDRPHY_PIR = 0x20020041;

    while(i > 0 && REG_DDRPHY_PGSR != 0xf && REG_DDRPHY_PGSR != 0x1f)
        i -= 1;
    if(i == 0)
        return 2;

    REG_DDRC_APB_PHYRST_CFG = 0x400000;
    mdelay(3);
    REG_DDRC_APB_PHYRST_CFG = 0;
    mdelay(3);

#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_CFG = 0xa468aec;
#else
    REG_DDRC_CFG = 0xa4689ec;
#endif
    REG_DDRC_CTRL = 2;
    REG_DDRPHY_PIR = 0x20020081;

    i = 500000;
    while(REG_DDRPHY_PGSR != 0x1f) {
        if(REG_DDRPHY_PGSR & 0x70)
            break;
        i -= 1;
    }

    if(i == 0)
        return 3;

    if((REG_DDRPHY_PGSR & 0x60) != 0 && REG_DDRPHY_PGSR != 0)
        return 4;

    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 10;
    REG_DDRC_CTRL = 0;
#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_CFG  = 0xa468a6c;
#else
    REG_DDRC_CFG  = 0xa46896c;
#endif
    REG_DDRC_TIMING(1) = 0x2050501;
    REG_DDRC_TIMING(2) = 0x4090404;
    REG_DDRC_TIMING(3) = 0x2704030d;
    REG_DDRC_TIMING(4) = 0xb7a0251;
    REG_DDRC_TIMING(5) = 0xff090200;
    REG_DDRC_TIMING(6) = 0xa0a0202;
#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_MMAP0   = 0x20fc;
    REG_DDRC_MMAP1   = 0x2400;
#else
    REG_DDRC_MMAP0   = 0x20fe;
    REG_DDRC_MMAP1   = 0x2200;
#endif
    REG_DDRC_CTRL    = 10;
    REG_DDRC_REFCNT  = 0x2f0003;
    REG_DDRC_CTRL    = 0xc91e;

#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_REMAP(1) = 0x03020c0b;
    REG_DDRC_REMAP(2) = 0x07060504;
    REG_DDRC_REMAP(3) = 0x000a0908;
    REG_DDRC_REMAP(4) = 0x0f0e0d01;
    REG_DDRC_REMAP(5) = 0x13121110;
#else
    REG_DDRC_REMAP(1) = 0x03020b0a;
    REG_DDRC_REMAP(2) = 0x07060504;
    REG_DDRC_REMAP(3) = 0x01000908;
    REG_DDRC_REMAP(4) = 0x0f0e0d0c;
    REG_DDRC_REMAP(5) = 0x13121110;
#endif

    REG_DDRC_STATUS &= ~0x40;

    REG_CPM_DDRCDR |= CPM_DDRCDR_GATE_EN;
    REG_DDRC_APB_CLKSTP_CFG = 0x9000000f;
    REG_DDRC_AUTOSR_EN = 1;
    return 0;
}

void spl_main(void)
{
    /* magic */
    REG_CPM_PSWC0ST = 0x00;
    REG_CPM_PSWC1ST = 0x10;
    REG_CPM_PSWC2ST = 0x18;
    REG_CPM_PSWC3ST = 0x08;

    init_clk();
    init_ost();
    /* there is no way to report this, and without DDR the stub can't run */
    if(init_dram() != 0)
        while(1) {}
}
