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

#ifndef __UART_X1000_H__
#define __UART_X1000_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PORT_UART0 = 0,
    PORT_UART1 = 1,
    PORT_UART2 = 2,
    PORT_MAX = 3,
} uart_port_t;

typedef enum {
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
} uart_parity_t;

extern void uart_init(uart_port_t port, int baud);
extern void uart_tx(uart_port_t port, const uint8_t *buf, size_t len);
/* Returns false if the data couldn't be sent within 'timeout' ticks */
extern bool uart_tx_timeout(uart_port_t port, const uint8_t *buf, size_t len,
                            int timeout);
extern size_t uart_rx(uart_port_t port, uint8_t *buf, size_t len);
/* Like uart_rx(), but gives up after 'timeout' ticks. Returns the number
 * of bytes received, which may be zero. */
extern size_t uart_rx_timeout(uart_port_t port, uint8_t *buf, size_t len,
                              int timeout);
extern bool uart_pending_rx(uart_port_t port);
extern void uart_set_baud(uart_port_t port, int baud);
extern void uart_set_parity(uart_port_t port, uart_parity_t parity);
/* Hardware RTS/CTS flow control, enabled by uart_init() */
extern void uart_set_flow_control(uart_port_t port, bool enable);
extern void uart_deinit(uart_port_t port);

#endif /* __UART_X1000_H__ */
