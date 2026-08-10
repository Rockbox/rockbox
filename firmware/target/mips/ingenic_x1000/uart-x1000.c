/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Skye Green
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

#include "gpio-ingenic.h"
#include "irq-x1000.h"
#include "dma-ingenic.h"
#include "kernel.h"
#include "panic.h"
#include "semaphore.h"
#include "system.h"
#include "uart-x1000.h"
#include "thread.h"
#include "x1000/macro.h"
#include "x1000/uart.h"
#include "x1000/cpm.h"
#include <assert.h>

static dma_desc tx_dma_desc;
static struct semaphore tx_done_sem;

static uint8_t *rx_buf;
static size_t rx_available;

static struct semaphore rx_done_sem;


static void uart_tx_dma_cb(int evt)
{
    if (evt != DMA_EVENT_COMPLETE) return;;
    semaphore_release(&tx_done_sem);
}

void uart_init(uart_port_t port, int baud) {
    switch (port) {
        case PORT_UART0:
            jz_writef(CPM_CLKGR, UART0(0));
            system_enable_irq(IRQ_UART0);
            break;
        case PORT_UART1:
            jz_writef(CPM_CLKGR, UART1(0));
            system_enable_irq(IRQ_UART1);
            break;
        case PORT_UART2:
            jz_writef(CPM_CLKGR, UART2(0));
            system_enable_irq(IRQ_UART2);
            break;
        default:
            panicf("invalid UART port %d", port);
    }

    semaphore_init(&tx_done_sem, 1, 0);
    semaphore_init(&rx_done_sem, 1, 0);

    jz_write(UART_ULCR(port), 0);
    jz_write(UART_UIER(port), 0);

    jz_overwritef(UART_ULCR(port), WLS(0b11));
    uart_set_baud(port, baud);
    jz_overwritef(UART_UMCR(port), MDCE(1), FCM(1));
    jz_overwritef(UART_UFCR(port), RDTR(0b11), UME(1), TFRT(1), RFRT(1), FME(1), DME(1));
    dma_set_callback(DMA_CHANNEL_UART_TX, uart_tx_dma_cb);
}

void uart_deinit(uart_port_t port) {
    jz_write(UART_ULCR(port), 0);
    jz_write(UART_UIER(port), 0);
    jz_writef(UART_UFCR(port), UME(0));
    semaphore_release(&tx_done_sem);
    semaphore_release(&rx_done_sem);

    switch (port) {
        case PORT_UART0:
            system_disable_irq(IRQ_UART0);
            jz_writef(CPM_CLKGR, UART0(1));
            break;
        case PORT_UART1:
            system_disable_irq(IRQ_UART1);
            jz_writef(CPM_CLKGR, UART1(1));
            break;
        case PORT_UART2:
            system_disable_irq(IRQ_UART2);
            jz_writef(CPM_CLKGR, UART2(1));
            break;
        default:
            panicf("invalid UART port %d", port);
    }
}

void uart_set_baud(uart_port_t port, int baud) {
    uint32_t udllr, udlhr, umr, uacr;
    switch (baud) {
#if X1000_EXCLK_FREQ == 24000000
        case 115200:
            udllr = 13;
            udlhr = 0;
            umr = 16;
            uacr = 0;
            break;
        case 1500000:
            udllr = 1;
            udlhr = 0;
            umr = 16;
            uacr = 0;
            break;
        case 2000000:
            udllr = 1;
            udlhr = 0;
            umr = 12;
            uacr = 0;
            break;
        case 3000000:
            udllr = 1;
            udlhr = 0;
            umr = 8;
            uacr = 0;
            break;
        case 4000000:
            udllr = 1;
            udlhr = 0;
            umr = 6;
            uacr = 0;
            break;
#endif
        default:
            panicf("UART(%d): unsupported baud %d", port, baud);
    }

    jz_writef(UART_ULCR(port), DLAB(1));
    jz_write(UART_UDLLR(port), udllr);
    jz_write(UART_UDLHR(port), udlhr);
    jz_writef(UART_ULCR(port), DLAB(0));
    jz_write(UART_UMR(port), umr);
    jz_write(UART_UACR(port), uacr);
}

void uart_empty_rx(uart_port_t port) {
    int slots = jz_read(UART_URCR(port));
    while (slots > 0 && rx_available > 0) {
        *rx_buf = jz_read(UART_URBR(port));
        rx_available--;
        rx_buf++;
        slots--;
    }
}

void uart_tx(uart_port_t port, const uint8_t *buf, size_t len) {
    assert(buf != NULL);
    assert(len > 0);
    tx_dma_desc.cm = jz_orf(DMA_CHN_CM, SAI(1), DAI(0), RDIL(7),
                              SP_V(8BIT), DP_V(8BIT), TSZ_V(AUTO),
                              STDE(0), TIE(1), LINK(0));
    tx_dma_desc.sa = PHYSADDR(buf);
    tx_dma_desc.ta = PHYSADDR(JA_UART_UTHR(port));
    tx_dma_desc.tc = len;
    tx_dma_desc.sd = 0;
    switch (port) {
        case PORT_UART0:
            tx_dma_desc.rt = jz_orf(DMA_CHN_RT, TYPE_V(UART0_TX));
            break;
        case PORT_UART1:
            tx_dma_desc.rt = jz_orf(DMA_CHN_RT, TYPE_V(UART1_TX));
            break;
        case PORT_UART2:
            tx_dma_desc.rt = jz_orf(DMA_CHN_RT, TYPE_V(UART2_TX));
            break;
        default:
            panicf("invalid UART port %d", port);
    }
    commit_dcache_range(buf, len);
    commit_dcache_range(&tx_dma_desc, sizeof(dma_desc));
    REG_DMA_CHN_DA(DMA_CHANNEL_UART_TX) = PHYSADDR(&tx_dma_desc);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_UART_TX), DES8(1), NDES(0));
    jz_set(DMA_DB, 1 << DMA_CHANNEL_UART_TX);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_UART_TX), CTE(1));
    semaphore_wait(&tx_done_sem, TIMEOUT_BLOCK);
    while (jz_readf(UART_ULSR(port), TEMP) == 0) {
        yield();
    }
}

size_t uart_rx(uart_port_t port, uint8_t *buf, size_t len) {
    rx_buf = buf;
    rx_available = len;
    jz_writef(UART_UIER(port), RTOIE(1), RDRIE(1));
    semaphore_wait(&rx_done_sem, TIMEOUT_BLOCK);
    size_t rem = rx_available;
    rx_available = 0;
    return len - rem;
}

bool uart_pending_rx(uart_port_t port) {
    return jz_read(UART_URCR(port)) > 0;
}

void uart_irq(uart_port_t port) {
    uint32_t uiir = jz_read(UART_UIIR(port));
    if (jz_vreadf(uiir, UART_UIIR, INPEND) != 0) {
        return;
    }

    uint32_t ulsr = jz_read(UART_ULSR(port));

    if (jz_vreadf(ulsr, UART_ULSR, DRY) != 0) {
        bool was_avail = rx_available > 0;
        uart_empty_rx(port);
        if (was_avail && rx_available == 0) {
            jz_writef(UART_UIER(port), RDRIE(0), RLSIE(0), RTOIE(0));
            semaphore_release(&rx_done_sem);
            return;
        }
    }

    if (jz_vreadf(uiir, UART_UIIR, INID) == 0b110) {
        jz_writef(UART_UIER(port), RDRIE(0), RLSIE(0), RTOIE(0));
        semaphore_release(&rx_done_sem);
    }
}

void UART0(void)
{
    uart_irq(PORT_UART0);
}

void UART1(void)
{
    uart_irq(PORT_UART1);
}

void UART2(void)
{
    uart_irq(PORT_UART2);
}