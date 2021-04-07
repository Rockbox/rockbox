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

#include "config.h"
#include "nand-x1000.h"
#include "gpio-x1000.h"
#include "mmu-mips.h"
#include <string.h>

/* "fiio" in little endian */
#define BOOTMAGIC 0x6f696966

/* Argument structure needed by Linux */
struct linux_kargs {
    void* arg0;
    void* arg1;
};

#define LINUX_KARGSADDR 0x80004000

static const char recovery_cmdline[] = "mem=xxM@0x0\
 no_console_suspend\
 console=ttyS2,115200n8\
 lpj=5009408\
 ip=off";

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

#define BOOTOPTION_ROCKBOX   0
#define BOOTOPTION_FIIOLINUX 1
#define BOOTOPTION_RECOVERY  2
#define NUM_BOOTOPTIONS      3

static const struct bootoption {
    uint32_t nand_addr;
    uint32_t nand_size;
    unsigned long load_addr;
    unsigned long exec_addr;
    const char* cmdline;
} boot_options[NUM_BOOTOPTIONS] = {
    {
        /* Rockbox: the first unused NAND page is 26 KiB in, and the
         * remainder of the block is unused, giving us 102 KiB to use.
         */
        .nand_addr = 0x6800,
        .nand_size = 0x19800,
        .load_addr = 0x80003ff8, /* first 8 bytes are bootloader ID */
        .exec_addr = 0x80004000,
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

/* Simple diagnostic if something goes wrong -- a little nicer than wondering
 * what's going on when the machine hangs
 */
void die(void)
{
    const int pin = (1 << 24);

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

/* Boot select button state must remain stable for this duration
 * before the choice will be accepted. Currently 100ms.
 */
#define BTN_STABLE_TIME (100 * (X1000_EXCLK_FREQ / 4000))

int get_boot_option(void)
{
    const uint32_t pinmask = (1 << 17) | (1 << 19);

    uint32_t pin = 1, lastpin = 0;
    uint32_t deadline = 0;

    /* Configure the button GPIOs as inputs */
    gpio_config(GPIO_A, pinmask, GPIO_INPUT);

    /* Poll the pins for a short duration to detect a keypress */
    do {
        lastpin = pin;
        pin = ~REG_GPIO_PIN(GPIO_A) & pinmask;
        if(pin != lastpin) {
            /* This will always be set on the first iteration */
            deadline = __ost_read32() + BTN_STABLE_TIME;
        }
    } while(__ost_read32() < deadline);

    /* Play button boots original firmware */
    if(pin == (1 << 17))
        return BOOTOPTION_FIIOLINUX;

    /* Volume up boots recovery */
    if(pin == (1 << 19))
        return BOOTOPTION_RECOVERY;

    /* Default is to boot Rockbox */
    return BOOTOPTION_ROCKBOX;
}

void spl_main(void)
{
    /* Get user boot option */
    int booti = get_boot_option();
    const struct bootoption* opt = &boot_options[booti];

    /* Load selected firmware from flash */
    if(nand_open())
        die();
    if(nand_read_bytes(opt->nand_addr, opt->nand_size, (void*)opt->load_addr))
        die();

    if(booti == BOOTOPTION_ROCKBOX) {
        /* If bootloader is not installed, return back to boot ROM.
         * Also read in the first eraseblock of NAND flash so it can be
         * dumped back over USB.
         */
        if(*(unsigned*)(opt->load_addr + 4) != BOOTMAGIC) {
            nand_read_bytes(0, 128 * 1024, (void*)0x80000000);
            commit_discard_idcache();
            return;
        }
    } else {
        /* TODO: Linux boot not implemented yet
         *
         * - Have to initialize UART2, as it's used for the serial console
         * - Must initialize APLL and change clocks over
         * - There are some other clocks which need to be initialized
         * - We should turn off OST since the OF SPL does not turn it on
         */
        die();
    }

    if(boot_options[booti].cmdline) {
        /* Handle Linux command line arguments */
        struct linux_kargs* kargs = (struct linux_kargs*)LINUX_KARGSADDR;
        kargs->arg0 = 0;
        kargs->arg1 = (void*)boot_options[booti].cmdline;
    }

    /* Flush caches and jump to address */
    void* execaddr = (void*)opt->exec_addr;
    commit_discard_idcache();
    __asm__ __volatile__ ("jr %0\n"
                          "nop\n"
                          :: "r"(execaddr));
    __builtin_unreachable();
}
