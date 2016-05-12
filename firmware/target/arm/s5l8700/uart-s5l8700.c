/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Cástor Muñoz
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
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "system.h"

#include "s5l8700.h"
#include "uc870x.h"


/*
 * XXX: This code is based on datasheets and NEVER TESTED !!!
 */


/*
 * s5l8700 UC870X HW: 1 UARTC, 2 ports
 */
static struct uartc_port *uartc_port_l[UARTC_N_PORTS];
const struct uartc s5l8700_uartc =
{
    .id       = 0,
    .baddr    = UARTC_BASE_ADDR,
    .port_off = UARTC_PORT_OFFSET,
    .n_ports  = UARTC_N_PORTS,
    .port_l   = uartc_port_l,
};

static int intmsk_uart[S5L8700_N_PORTS] = { INTMSK_UART0, INTMSK_UART1 };

/*
 * Device level functions specific to S5L8700
 */
void uart_target_enable_gpio(int uart_id, int port_id)
{
    (void) uart_id;
    switch (port_id) {
        /* configure UARTx Tx/Rx GPIO ports */
        case 0:
            PCON0 = (PCON0 & 0x0fff) | 0xa000;
            break;
        case 1:
            PCON6 = (PCON6 & 0xfff00fff) | 0x00044000;
            break;
    }
}

void uart_target_disable_gpio(int uart_id, int port_id)
{
    (void) uart_id;
    switch (port_id) {
        /* configure default reset values */
        case 0:
            PCON0 = (PCON0 & 0x0fff) | 0x0000;
            break;
        case 1:
            PCON6 = (PCON6 & 0xfff00fff) | 0x00000000;
            break;
    }
}

void uart_target_enable_irq(int uart_id, int port_id)
{
    (void) uart_id;
    INTMSK |= intmsk_uart[port_id];
}

void uart_target_disable_irq(int uart_id, int port_id)
{
    (void) uart_id;
    INTMSK &= ~intmsk_uart[port_id];
}

void uart_target_clear_irq(int uart_id, int port_id)
{
    (void) uart_id;
    SRCPND |= intmsk_uart[port_id];
}

void uart_target_enable_clocks(int uart_id)
{
    (void) uart_id;
    PWRCON &= ~(1 << CLOCKGATE_UARTC);
}

void uart_target_disable_clocks(int uart_id)
{
    (void) uart_id;
    PWRCON |= (1 << CLOCKGATE_UARTC);
}

/*
 * ISRs
 */
void ICODE_ATTR INT_UART0(void)
{
    uartc_callback(&s5l8700_uartc, 0);
}

void ICODE_ATTR INT_UART1(void)
{
    uartc_callback(&s5l8700_uartc, 1);
}

/* Main init */
void uart_init(void)
{
    uartc_open(&s5l8700_uartc);
}
