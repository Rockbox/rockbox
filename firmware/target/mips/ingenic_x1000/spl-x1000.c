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

#if defined(FIIO_M3K) || defined(SHANLING_Q1)
# define SPL_DDR_MEMORYSIZE  64
# define SPL_DDR_AUTOSR_EN   1
# define SPL_DDR_NEED_BYPASS 1
#elif defined(EROS_QN)
# define SPL_DDR_MEMORYSIZE 32
# define SPL_DDR_AUTOSR_EN   1
# define SPL_DDR_NEED_BYPASS 1
#else
# error "please define DRAM settings"
#endif

static void* heap = (void*)(X1000_SDRAM_BASE + X1000_SDRAM_SIZE);
static nand_drv* ndrv = NULL;

void* spl_alloc(size_t count)
{
    heap -= CACHEALIGN_UP(count);
    memset(heap, 0, CACHEALIGN_UP(count));
    return heap;
}

int spl_storage_open(void)
{
    /* We need to assign the GPIOs manually */
    gpioz_configure(GPIO_A, 0x3f << 26, GPIOF_DEVICE(1));

    /* Allocate NAND driver manually in DRAM */
    ndrv = spl_alloc(sizeof(nand_drv));
    ndrv->page_buf = spl_alloc(NAND_DRV_MAXPAGESIZE);
    ndrv->scratch_buf = spl_alloc(NAND_DRV_SCRATCHSIZE);
    ndrv->refcount = 0;

    return nand_open(ndrv);
}

void spl_storage_close(void)
{
    nand_close(ndrv);
}

int spl_storage_read(uint32_t addr, uint32_t length, void* buffer)
{
    return nand_read_bytes(ndrv, addr, length, buffer);
}

/* Used by:
 * - FiiO M3K
 * - Shanling Q1
 *
 * Amend it and add #ifdefs for other targets if needed.
 */
void spl_dualboot_init_clocktree(void)
{
    /* Make sure these are gated to match the OF behavior. */
    jz_writef(CPM_CLKGR, PCM(1), MAC(1), LCD(1), MSC0(1), MSC1(1), OTG(1), CIM(1));

    /* Set clock sources, and make sure every clock starts out stopped */
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
}

void spl_dualboot_init_uart2(void)
{
    /* Ungate the clock and select UART2 device function */
    jz_writef(CPM_CLKGR, UART2(0));
    gpioz_configure(GPIO_C, 3 << 30, GPIOF_DEVICE(1));

    /* Disable all interrupts */
    jz_write(UART_UIER(2), 0);

    /* FIFO configuration */
    jz_overwritef(UART_UFCR(2),
                  RDTR(3), /* FIFO trigger level = 60? */
                  UME(0),  /* UART module disable */
                  DME(1),  /* DMA mode enable? */
                  TFRT(1), /* transmit FIFO reset */
                  RFRT(1), /* receive FIFO reset */
                  FME(1)); /* FIFO mode enable */

    /* IR mode configuration */
    jz_overwritef(UART_ISR(2),
                  RDPL(1),      /* Zero is negative pulse for receive */
                  TDPL(1),      /* ... and for transmit */
                  XMODE(1),     /* Pulse width 1.6us */
                  RCVEIR(0),    /* Disable IR for recieve */
                  XMITIR(0));   /* ... and for transmit */

    /* Line configuration */
    jz_overwritef(UART_ULCR(2), DLAB(0),
                  WLS_V(8BITS),         /* 8 bit words */
                  SBLS_V(1_STOP_BIT),   /* 1 stop bit */
                  PARE(0),              /* no parity */
                  SBK(0));              /* don't set break */

    /* Set the baud rate... not too sure how this works. (Docs unclear!) */
    const unsigned divisor = 0x0004;
    jz_writef(UART_ULCR(2), DLAB(1));
    jz_write(UART_UDLHR(2), (divisor >> 8) & 0xff);
    jz_write(UART_UDLLR(2), divisor & 0xff);
    jz_write(UART_UMR(2), 16);
    jz_write(UART_UACR(2), 0);
    jz_writef(UART_ULCR(2), DLAB(0));

    /* Enable UART */
    jz_overwritef(UART_UFCR(2),
                  RDTR(0),  /* FIFO trigger level = 1 */
                  DME(0),   /* DMA mode disable */
                  UME(1),   /* UART module enable */
                  TFRT(1),  /* transmit FIFO reset */
                  RFRT(1),  /* receive FIFO reset */
                  FME(1));  /* FIFO mode enable */
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

static void* get_load_buffer(const struct spl_boot_option* opt)
{
    /* read to a temporary location if we need to decompress,
     * otherwise simply read directly to the load address. */
    if(opt->flags & BOOTFLAG_UCLPACK)
        return spl_alloc(opt->storage_size);
    else
        return (void*)opt->load_addr;
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

typedef void(*entry_fn)(int, char**, int, int) __attribute__((noreturn));

void spl_main(void)
{
    int rc, boot_option;
    const struct spl_boot_option* opt;
    void* load_buffer;
    char** kargv = NULL;
    int kargc = 0;

    /* magic */
    REG_CPM_PSWC0ST = 0x00;
    REG_CPM_PSWC1ST = 0x10;
    REG_CPM_PSWC2ST = 0x18;
    REG_CPM_PSWC3ST = 0x08;

    /* set up boot flags */
    init_boot_flags();
    set_boot_option(BOOT_OPTION_ROCKBOX);

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

    /* find out what we should boot */
    boot_option = spl_get_boot_option();
    opt = &spl_boot_options[boot_option];
    load_buffer = get_load_buffer(opt);

    /* save the selection for later */
    set_boot_option(boot_option);

    /* finish up clock init */
    clk_init();

    /* load the image from storage */
    rc = spl_storage_open();
    if(rc != 0)
        spl_error();

    rc = spl_storage_read(opt->storage_addr, opt->storage_size, load_buffer);
    if(rc != 0)
        spl_error();

    /* handle compression */
    switch(opt->flags & BOOTFLAG_COMPRESSED) {
    case BOOTFLAG_UCLPACK: {
        uint32_t out_size = X1000_DRAM_END - opt->load_addr;
        rc = ucl_unpack((uint8_t*)load_buffer, opt->storage_size,
                        (uint8_t*)opt->load_addr, &out_size);
    } break;

    default:
        break;
    }

    if(rc != 0)
        spl_error();

    /* call the setup hook */
    if(opt->setup) {
        rc = opt->setup();
        if(rc != 0)
            spl_error();
    }

    /* close off storage access */
    spl_storage_close();

    /* handle kernel command line, if specified */
    if(opt->cmdline) {
        kargv = (char**)opt->cmdline_addr;
        kargv[kargc++] = 0;
        kargv[kargc++] = (char*)opt->cmdline;
    }

    /* jump to the entry point */
    entry_fn fn = (entry_fn)opt->exec_addr;
    commit_discard_idcache();
    fn(kargc, kargv, 0, 0);
}
