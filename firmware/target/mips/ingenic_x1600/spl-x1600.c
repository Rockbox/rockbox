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

/* X1600 SPL initialises DRAM and loads the bootloader from storage. USB
 * stage1 is a separate binary because the SPL link address overlaps the
 * live BootROM USB code. */

#include "config.h"
#include "boot-x1600.h"
#include "x1600.h"
#include "system.h"
#include "spl-x1600.h"
#include "ddr-x1600.h"
#include "x1600/cpm.h"
#include "x1600/wdt.h"
#include "x1600/tcu.h"
#include "ost-ingenic.h"
#include "ucl_decompress.h"
#include <string.h>

/* Whether the bootloader image is UCL-compressed (it is, see x1600boot.make) */
#ifndef SPL_USE_UCLPACK
# define SPL_USE_UCLPACK 1
#endif

/* Drive the PC01 error LED directly to avoid GPIO-driver dependencies in
 * the SPL. */
#define GPIO_PC_BASE    0xb0010200
#define SPL_ERROR_BIT   (1 << 1)        /* PC01, red LED */

static void spl_error_pin_output(void)
{
    *(volatile uint32_t*)(GPIO_PC_BASE + 0x18) = SPL_ERROR_BIT; /* PXINTC  */
    *(volatile uint32_t*)(GPIO_PC_BASE + 0x24) = SPL_ERROR_BIT; /* PXMSKS  */
    *(volatile uint32_t*)(GPIO_PC_BASE + 0x38) = SPL_ERROR_BIT; /* PXPAT1C */
}

static void spl_error_pin_set(int level)
{
    *(volatile uint32_t*)(GPIO_PC_BASE + (level ? 0x44 : 0x48)) = SPL_ERROR_BIT;
}

void spl_error(void)
{
    int level = 0;
    spl_error_pin_output();
    while (1) {
        spl_error_pin_set(level);
        mdelay(100);
        level = 1 - level;
    }
}

/* Repeat code short flashes followed by a gap. DDR errors are also
 * recorded in ddr_last_status. */
static void spl_error_code(int code)
{
    int i;

    spl_error_pin_output();
    if (code <= 0)
        spl_error();

    while (1) {
        for (i = 0; i < code; ++i) {
            spl_error_pin_set(1);
            mdelay(200);
            spl_error_pin_set(0);
            mdelay(200);
        }
        mdelay(1500);
    }
}

/* Heap, allocated downwards from the top of DRAM as on the X1000. */
static void* heap = (void*)(X1600_SDRAM_BASE + X1600_SDRAM_SIZE);

void* spl_alloc(size_t count)
{
    heap -= CACHEALIGN_UP(count);
    memset(heap, 0, CACHEALIGN_UP(count));
    return heap;
}

static void* get_load_buffer(void)
{
    /* Read to a scratch buffer if we have to decompress, else read straight
     * to the final load address. */
    if (SPL_USE_UCLPACK)
        return spl_alloc(SPL_BOOT_STORAGE_SIZE);
    else
        return (void*)X1600_BOOT_LOAD_ADDR;
}

void spl_main(void)
{
    int rc;
    void* load_buffer;

    /* Stop the inherited watchdog and gate its clock before
     * initialisation. */
    REG_WDT_ENABLE = 0;             /* stop the counter   */
    jz_set(TCU_STOP, 1u << 16);     /* and gate its clock */

    /* Reset inherited boot flags, then initialise the timebase and DRAM. */
    init_boot_flags();

    /* Save the boot straps before drivers reconfigure port C. */
    if(x1600_boot_sel() == X1600_BOOT_SEL_USB)
        set_boot_flag(BOOT_FLAG_USB_BOOT);

    init_ost();

    rc = x1600_ddr_init(&x1600_ddr_param_hibyr1);
    if (rc != 0)
        spl_error_code(rc);

    /* Retain the entry CPU clock during SPL storage loading. */

    rc = spl_storage_open();
    if (rc != 0)
        spl_error();

    load_buffer = get_load_buffer();
    rc = spl_storage_read(SPL_BOOT_STORAGE_ADDR, SPL_BOOT_STORAGE_SIZE,
                          load_buffer);
    if (rc != 0)
        spl_error();

    /* Allow decompression up to the top of DRAM. The compressed source
     * buffer is also allocated there, so the two ranges must not overlap. */
    if (SPL_USE_UCLPACK) {
        uint32_t out_size = X1600_SDRAM_END - X1600_BOOT_LOAD_ADDR;
        rc = ucl_unpack((uint8_t*)load_buffer, SPL_BOOT_STORAGE_SIZE,
                        (uint8_t*)X1600_BOOT_LOAD_ADDR, &out_size);
    } else {
        rc = 0;
    }

    if (rc != 0)
        spl_error();

    spl_storage_close();

    /* The loaded bootloader will execute from KSEG0. */
    typedef void(*entry_fn)(void);
    entry_fn fn = (entry_fn)X1600_BOOT_EXEC_ADDR;
    commit_discard_idcache();
    fn();
}
