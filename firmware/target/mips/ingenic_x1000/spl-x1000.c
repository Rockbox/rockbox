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
#include "spl-x1000.h"
#include "clk-x1000.h"
#include "nand-x1000.h"
#include "gpio-x1000.h"
#include "boot-x1000.h"
#include "x1000/cpm.h"
#include "x1000/ost.h"
#include "x1000/uart.h"
#include "x1000/ddrc.h"
#include "x1000/ddrc_apb.h"
#include "x1000/ddrphy.h"
#include "ucl_decompress.h"
#include <string.h>

#if defined(FIIO_M3K)
/* Size of memory, either 64 or 32 is legal. */
# define SPL_DDR_MEMORYSIZE     64
/* Pin to flash on spl_error(). Should be a backlight. */
# define SPL_ERROR_PIN          GPIO_PC(24)
/* Address and size of the bootloader on the storage medium used by the SPL */
# define BOOT_STORAGE_ADDR      0x6800
# define BOOT_STORAGE_SIZE      (102 * 1024)
#elif defined(SHANLING_Q1)
# define SPL_DDR_MEMORYSIZE     64
# define SPL_ERROR_PIN          GPIO_PC(25)
# define BOOT_STORAGE_ADDR      0x6800
# define BOOT_STORAGE_SIZE      (102 * 1024)
#elif defined(EROS_QN)
# define SPL_DDR_MEMORYSIZE     32
# define SPL_ERROR_PIN          GPIO_PC(25)
# define BOOT_STORAGE_ADDR      0x6800
# define BOOT_STORAGE_SIZE      (102 * 1024)
#else
# error "please define SPL config"
#endif

/* Hardcode this since the SPL is considered part of the bootloader,
 * and should never get built or updated separately. */
#define BOOT_LOAD_ADDR  X1000_DRAM_BASE
#define BOOT_EXEC_ADDR  BOOT_LOAD_ADDR

/* Whether the bootloader is UCL-compressed */
#ifndef SPL_USE_UCLPACK
# define SPL_USE_UCLPACK 1
#endif

/* Whether auto-self-refresh should be enabled (seems it always should be?) */
#ifndef SPL_DDR_AUTOSR_EN
# define SPL_DDR_AUTOSR_EN 1
#endif

/* Whether DLL bypass is necessary (probably always?) */
#ifndef SPL_DDR_NEED_BYPASS
# define SPL_DDR_NEED_BYPASS 1
#endif

static void* heap = (void*)(X1000_SDRAM_BASE + X1000_SDRAM_SIZE);

void* spl_alloc(size_t count)
{
    heap -= CACHEALIGN_UP(count);
    memset(heap, 0, CACHEALIGN_UP(count));
    return heap;
}

void spl_error(void)
{
    int level = 0;
    while(1) {
        gpio_set_function(SPL_ERROR_PIN, GPIOF_OUTPUT(level));
        mdelay(100);
        level = 1 - level;
    }
}

static void init_ost(void)
{
    /* NOTE: the prescaler needs to be the same as in system-x1000.c */
    jz_writef(CPM_CLKGR, OST(0));
    jz_writef(OST_CTRL, PRESCALE2_V(BY_4));
    jz_overwritef(OST_CLEAR, OST2(1));
    jz_write(OST_2CNTH, 0);
    jz_write(OST_2CNTL, 0);
    jz_setf(OST_ENABLE, OST2);
}

/* NOTE: This is originally based on disassembly of the FiiO M3K SPL, which
 * is in fact the GPL'd Ingenic X-Loader SPL. Similar stuff can be found in
 * Ingenic's U-boot code.
 *
 * The source code for the Ingenic X-Loader SPL can be found in these repos:
 * - https://github.com/JaminCheung/x-loader
 * - https://github.com/YuanhuanLiang/X1000
 *
 * Runtime conditionals based on the SoC type, looked up by OTP bits in EFUSE,
 * are converted to compile-time conditionals here, as they are constant for a
 * given target and there is no point in wasting precious space on dead code.
 *
 * I didn't decode the register fields; note that some values are documented as
 * "reserved" in the X1000 PM. The X-Loader source might shed more light on it;
 * it's likely these bits have meaning only on other Ingenic SoCs.
 *
 * The DDR PHY registers match Synopsys's "PHY Utility Block Lite." The names
 * of those registers & their fields can also be found in the X-Loader code,
 * but they're not documented by Ingenic.
 */
static int init_dram(void)
{
#if SPL_DDR_MEMORYSIZE != 64 && SPL_DDR_MEMORYSIZE != 32
# error "bad memory size"
#endif

    jz_writef(CPM_CLKGR, DDR(0));

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
#elif SPL_DDR_MEMORYSIZE == 32
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
        return 2;

    REG_DDRC_APB_PHYRST_CFG = 0x400000;
    mdelay(3);
    REG_DDRC_APB_PHYRST_CFG = 0;
    mdelay(3);

#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_CFG = 0xa468aec;
#elif SPL_DDR_MEMORYSIZE == 32
    REG_DDRC_CFG = 0xa4689ec;
#endif
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
        return 3;

    if((REG_DDRPHY_PGSR & 0x60) != 0 && REG_DDRPHY_PGSR != 0)
        return 4;

    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 10;
    REG_DDRC_CTRL = 0;
#if SPL_DDR_MEMORYSIZE == 64
    REG_DDRC_CFG  = 0xa468a6c;
#elif SPL_DDR_MEMORYSIZE == 32
    REG_DDRC_CFG  = 0xa46896c;
#endif
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
#endif
    REG_DDRC_CTRL    = 10;
    REG_DDRC_REFCNT  = 0x2f0003; /* is this adjustable for 32M? */
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
    return 0;
}

static void* get_load_buffer(void)
{
    /* read to a temporary location if we need to decompress,
     * otherwise simply read directly to the load address. */
    if(SPL_USE_UCLPACK)
        return spl_alloc(BOOT_STORAGE_SIZE);
    else
        return (void*)BOOT_LOAD_ADDR;
}

/* Mapping of boot_sel[1:0] pins.
 * See X1000 PM pg. 687, "XBurst Boot ROM Specification" */
enum {
    BSEL_MSC = 1,
    BSEL_USB = 2,
    BSEL_SFC = 3,
};

static uint32_t get_boot_sel(void)
{
    /* This variable holds the level of the boot_sel[2:0] pins at boot time,
     * and is defined by the maskrom.
     *
     * We use it to detect when we're USB booting, but this isn't totally
     * accurate because it only reflects the selected boot mode at reset and
     * not the current mode -- if the original selection fails and we fall
     * back to USB, this variable will only return the original selection.
     */
    return (*(uint32_t*)0xf40001ec) & 3;
}

void spl_main(void)
{
    int rc;
    void* load_buffer;

    /* magic */
    REG_CPM_PSWC0ST = 0x00;
    REG_CPM_PSWC1ST = 0x10;
    REG_CPM_PSWC2ST = 0x18;
    REG_CPM_PSWC3ST = 0x08;

    /* set up boot flags */
    init_boot_flags();

    /* early clock and DRAM init */
    clk_init_early();
    init_ost();
    if(init_dram() != 0)
        spl_error();

    /* USB boot stops here */
    if(get_boot_sel() == BSEL_USB) {
        set_boot_flag(BOOT_FLAG_USB_BOOT);
        return;
    }

    /* finish up clock init */
    clk_init();

    /* load the image from storage */
    rc = spl_storage_open();
    if(rc != 0)
        spl_error();

    load_buffer = get_load_buffer();
    rc = spl_storage_read(BOOT_STORAGE_ADDR, BOOT_STORAGE_SIZE, load_buffer);
    if(rc != 0)
        spl_error();

    /* decompress */
    if(SPL_USE_UCLPACK) {
        uint32_t out_size = X1000_SDRAM_END - BOOT_LOAD_ADDR;
        rc = ucl_unpack((uint8_t*)load_buffer, BOOT_STORAGE_SIZE,
                        (uint8_t*)BOOT_LOAD_ADDR, &out_size);
    } else {
        rc = 0;
    }

    if(rc != 0)
        spl_error();

    /* close off storage access */
    spl_storage_close();

    /* jump to the entry point */
    typedef void(*entry_fn)(void);
    entry_fn fn = (entry_fn)BOOT_EXEC_ADDR;
    commit_discard_idcache();
    fn();
}
