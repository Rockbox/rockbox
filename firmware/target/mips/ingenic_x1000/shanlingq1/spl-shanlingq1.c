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
#include "clk-x1000.h"
#include "spl-x1000.h"
#include "gpio-x1000.h"

#define CMDLINE_COMMON \
    "mem=64M@0x0 no_console_suspend console=ttyS2,115200n8 lpj=5009408 ip=off"
#define CMDLINE_NORMAL \
    " init=/linuxrc ubi.mtd=5 root=ubi0:rootfs ubi.mtd=6 rootfstype=ubifs rw"

static int dualboot_setup(void)
{
    spl_dualboot_init_clocktree();
    spl_dualboot_init_uart2();

    /* load PDMA MCU firmware */
    jz_writef(CPM_CLKGR, PDMA(0));
    return spl_storage_read(0x4000, 0x2000, (void*)0xb3422000);
}

const struct spl_boot_option spl_boot_options[] = {
    [BOOT_OPTION_ROCKBOX] = {
        .storage_addr = 0x6800,
        .storage_size = 102 * 1024,
        .load_addr = X1000_DRAM_BASE,
        .exec_addr = X1000_DRAM_BASE,
        .flags = BOOTFLAG_UCLPACK,
    },
    [BOOT_OPTION_OFW_PLAYER] = {
        .storage_addr = 0x140000,
        .storage_size = 8 * 1024 * 1024,
        .load_addr = 0x80efffc0,
        .exec_addr = 0x80f00000,
        .cmdline = CMDLINE_COMMON CMDLINE_NORMAL,
        .cmdline_addr = 0x80004000,
        .setup = dualboot_setup,
    },
    [BOOT_OPTION_OFW_RECOVERY] = {
        .storage_addr = 0x940000,
        .storage_size = 10 * 1024 * 1024,
        .load_addr = 0x80efffc0,
        .exec_addr = 0x80f00000,
        .cmdline = CMDLINE_COMMON,
        .cmdline_addr = 0x80004000,
        .setup = dualboot_setup,
    },
};

int spl_get_boot_option(void)
{
    /* Button debounce time in OST clock cycles */
    const uint32_t btn_stable_time = 100 * (X1000_EXCLK_FREQ / 4000);

    /* Buttons to poll */
    const unsigned port = GPIO_B;
    const uint32_t recov_pin = (1 << 22);   /* Next */
    const uint32_t orig_fw_pin = (1 << 21); /* Prev */

    uint32_t pin = -1, lastpin = 0;
    uint32_t deadline = 0;
    int iter_count = 30; /* to avoid an infinite loop */

    /* set GPIOs to input state */
    gpioz_configure(port, recov_pin|orig_fw_pin, GPIOF_INPUT);

    /* Poll until we get a stable reading */
    do {
        lastpin = pin;
        pin = ~REG_GPIO_PIN(port) & (recov_pin|orig_fw_pin);
        if(pin != lastpin) {
            deadline = __ost_read32() + btn_stable_time;
            iter_count -= 1;
        }
    } while(iter_count > 0 && __ost_read32() < deadline);

    if(iter_count >= 0 && (pin & orig_fw_pin)) {
        if(pin & recov_pin)
            return BOOT_OPTION_OFW_RECOVERY;
        else
            return BOOT_OPTION_OFW_PLAYER;
    }

    return BOOT_OPTION_ROCKBOX;
}

void spl_error(void)
{
    /* Flash the backlight */
    int level = 0;
    while(1) {
        gpio_set_function(GPIO_PC(25), GPIOF_OUTPUT(level));
        mdelay(100);
        level = 1 - level;
    }
}
