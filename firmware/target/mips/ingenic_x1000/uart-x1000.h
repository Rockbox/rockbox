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

extern void uart_init(uart_port_t port, int baud);
extern void uart_tx(uart_port_t port, const uint8_t *buf, size_t len);
extern size_t uart_rx(uart_port_t port, uint8_t *buf, size_t len);
extern bool uart_pending_rx(uart_port_t port);
extern void uart_set_baud(uart_port_t port, int baud);
extern void uart_deinit(uart_port_t port);

#endif /* __UART_X1000_H__ */
