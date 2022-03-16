/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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
#include "boot-x1000.h"
#include "nand-x1000.h"
#include "gpio-x1000.h"
#include "clk-x1000.h"
#include "x1000/cpm.h"
#include "x1000/lcd.h"
#include "x1000/uart.h"
#include <string.h>

#define HDR_BEGIN   128     /* header must begin within this many bytes */
#define HDR_LEN     256     /* header length cannot exceed this */

/* search for header value, label must be a 4-character string.
 * Returns the found value or 0 if the label wasn't found. */
static uint32_t search_header(const unsigned char* source, size_t length,
                              const char* label)
{
    size_t search_len = MIN(length, HDR_BEGIN);
    if(search_len < 8)
        return 0;
    search_len -= 7;

    /* find the beginning marker */
    size_t i;
    for(i = 8; i < search_len; i += 4)
        if(!memcmp(&source[i], "BEGINHDR", 8))
            break;
    if(i >= search_len)
        return 0;
    i += 8;

    /* search within the header */
    search_len = MIN(length, i + HDR_LEN) - 7;
    for(; i < search_len; i += 8) {
        if(!memcmp(&source[i], "ENDH", 4)) {
            break;
        } else if(!memcmp(&source[i], label, 4)) {
            i += 4;
            /* read little-endian value */
            uint32_t ret = source[i];
            ret |= source[i+1] << 8;
            ret |= source[i+2] << 16;
            ret |= source[i+3] << 24;
            return ret;
        }
    }

    return 0;
}

static void iram_memmove(void* dest, const void* source, size_t length)
    __attribute__((section(".icode")));

static void iram_memmove(void* dest, const void* source, size_t length)
{
    unsigned char* d = dest;
    const unsigned char* s = source;

    if(s < d && d < s + length) {
        d += length;
        s += length;
        while(length--)
            *--d = *--s;
    } else {
        while(length--)
            *d++ = *s++;
    }
}

void x1000_boot_rockbox(const void* source, size_t length)
{
    uint32_t load_addr = search_header(source, length, "LOAD");
    if(!load_addr)
        load_addr = X1000_STANDARD_DRAM_BASE;

    disable_irq();

    /* --- Beyond this point, do not call into DRAM --- */

    iram_memmove((void*)load_addr, source, length);
    commit_discard_idcache();

    typedef void(*entry_fn)(void);
    entry_fn fn = (entry_fn)load_addr;
    fn();
    while(1);
}

void x1000_boot_linux(const void* source, size_t length,
                      void* load, void* entry, const char* args)
{
    size_t args_len = strlen(args);

    disable_irq();

    /* --- Beyond this point, do not call into DRAM --- */

    void* safe_mem = (void*)X1000_IRAM_END;

    /* copy argument string to a safe location */
    char* args_copy = safe_mem + 32;
    iram_memmove(args_copy, args, args_len);

    /* generate argv array */
    char** argv = safe_mem;
    argv[0] = NULL;
    argv[1] = args_copy;

    iram_memmove(load, source, length);
    commit_discard_idcache();

    typedef void(*entry_fn)(long, char**, long, long);
    entry_fn fn = (entry_fn)entry;
    fn(2, argv, 0, 0);
    while(1);
}

void rolo_restart(const unsigned char* source, unsigned char* dest, int length)
{
    (void)dest;
    x1000_boot_rockbox(source, length);
}

void x1000_dualboot_cleanup(void)
{
    /* - disable all LCD interrupts since the M3K can't cope with them
     * - disable BEDN bit since it creates garbled graphics on the Q1 */
    jz_writef(CPM_CLKGR, LCD(0));
    jz_writef(LCD_CTRL, BEDN(0), EOFM(0), SOFM(0), IFUM(0), QDM(0));
    jz_writef(CPM_CLKGR, LCD(1));

    /* clear USB PHY voodoo bits, not all kernels use them */
    jz_writef(CPM_OPCR, GATE_USBPHY_CLK(0));
    jz_writef(CPM_USBCDR, PHY_GATE(0));

#if defined(FIIO_M3K) || defined(EROS_QN)
    /*
     * Need to bring up MPLL before booting Linux
     * (Doesn't apply to Q1 since it sticks with the default Ingenic config)
     */

    /* 24 MHz * 25 = 600 MHz */
    jz_writef(CPM_MPCR, BS(1), PLLM(25 - 1), PLLN(0), PLLOD(0), ENABLE(1));
    while(jz_readf(CPM_MPCR, ON) == 0);

    /* 600 MHz / 3 = 200 MHz */
    clk_set_ddr(X1000_CLK_MPLL, 3);

    clk_set_ccr_mux(CLKMUX_SCLK_A(APLL) |
                    CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(MPLL) |
                    CLKMUX_AHB2(MPLL));
    clk_set_ccr_div(CLKDIV_CPU(1) |     /* 1008 MHz */
                    CLKDIV_L2(2) |      /* 504 MHz */
                    CLKDIV_AHB0(3) |    /* 200 MHz */
                    CLKDIV_AHB2(3) |    /* 200 MHz */
                    CLKDIV_PCLK(6));    /* 100 MHz */
#endif
}

void x1000_dualboot_init_clocktree(void)
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

void x1000_dualboot_init_uart2(void)
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

int x1000_dualboot_load_pdma_fw(void)
{
    nand_drv* n = nand_init();
    nand_lock(n);

    int ret = nand_open(n);
    if(ret != NAND_SUCCESS)
        goto err_unlock;

    /* NOTE: hardcoded address is used by all current targets */
    jz_writef(CPM_CLKGR, PDMA(0));
    ret = nand_read_bytes(n, 0x4000, 0x2000, (void*)0xb3422000);

    nand_close(n);
  err_unlock:
    nand_unlock(n);
    return ret;
}
