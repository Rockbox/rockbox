/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "gpio-x1000.h"
#include "irq-x1000.h"
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

const uint8_t *tx_buf[PORT_MAX];
size_t tx_available[PORT_MAX];

struct semaphore tx_done_sem[PORT_MAX];

uint8_t *rx_buf[PORT_MAX];
size_t rx_available[PORT_MAX];

struct semaphore rx_done_sem[PORT_MAX];

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

    tx_available[port] = 0;
    rx_available[port] = 0;

    semaphore_init(&tx_done_sem[port], 1, 0);
    semaphore_init(&rx_done_sem[port], 1, 0);

    jz_write(UART_ULCR(port), 0);
    jz_write(UART_UIER(port), 0);

    jz_overwritef(UART_ULCR(port), WLS(0b11));
    uart_set_baud(port, baud);
    jz_overwritef(UART_UMCR(port), MDCE(1), FCM(1));
    jz_overwritef(UART_UFCR(port), RDTR(0b11), UME(1), TFRT(1), RFRT(1), FME(1));
}

void uart_deinit(uart_port_t port) {
    jz_write(UART_ULCR(port), 0);
    jz_write(UART_UIER(port), 0);
    jz_writef(UART_UFCR(port), UME(0));

    if (tx_available[port] > 0) {
        semaphore_release(&tx_done_sem[port]);
    }

    if (rx_available[port] > 0) {
        semaphore_release(&rx_done_sem[port]);
    }

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

void uart_refill_tx(uart_port_t port) {
    for (int i = 0; i < 32 && tx_available[port] > 0; i++) {
        uint8_t byte = *tx_buf[port];
        jz_write(UART_UTHR(port), byte);
        tx_available[port]--;
        tx_buf[port]++;
    }
}

void uart_empty_rx(uart_port_t port) {
    int slots = jz_read(UART_URCR(port));
    while (slots > 0 && rx_available[port] > 0) {
        *rx_buf[port] = jz_read(UART_URBR(port));
        rx_available[port]--;
        rx_buf[port]++;
        slots--;
    }
}

void uart_tx(uart_port_t port, const uint8_t *buf, size_t len) {
    assert(buf != NULL);
    assert(len > 0);
    tx_buf[port] = buf;
    tx_available[port] = len;
    jz_writef(UART_UIER(port), TDRIE(1));
    semaphore_wait(&tx_done_sem[port], TIMEOUT_BLOCK);
    while (jz_readf(UART_ULSR(port), TEMP) == 0) {
        yield();
    }
}

size_t uart_rx(uart_port_t port, uint8_t *buf, size_t len) {
    rx_buf[port] = buf;
    rx_available[port] = len;
    jz_writef(UART_UIER(port), RTOIE(1), RDRIE(1));
    semaphore_wait(&rx_done_sem[port], TIMEOUT_BLOCK);
    size_t rem = rx_available[port];
    rx_available[port] = 0;
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

    if (jz_vreadf(ulsr, UART_ULSR, TDRQ) != 0) {
        uart_refill_tx(port);
        if (tx_available[port] == 0) {
            jz_writef(UART_UIER(port), TDRIE(0));
            semaphore_release(&tx_done_sem[port]);
        }
    }

    if (jz_vreadf(ulsr, UART_ULSR, DRY) != 0) {
        bool was_avail = rx_available[port] > 0;
        uart_empty_rx(port);
        if (was_avail && rx_available[port] == 0) {
            jz_writef(UART_UIER(port), RDRIE(0), RLSIE(0), RTOIE(0));
            semaphore_release(&rx_done_sem[port]);
            return;
        }
    }

    if (jz_vreadf(uiir, UART_UIIR, INID) == 0b110) {
        jz_writef(UART_UIER(port), RDRIE(0), RLSIE(0), RTOIE(0));
        semaphore_release(&rx_done_sem[port]);
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