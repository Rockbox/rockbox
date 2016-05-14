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
 * s5l8701 UC870X HW: 3 UARTC, 1 port per UARTC
 */
static struct uartc_port *uartc0_port_l[UARTC0_N_PORTS];
const struct uartc s5l8701_uartc0 =
{
    .id       = 0,
    .baddr    = UARTC0_BASE_ADDR,
    .port_off = UARTC0_PORT_OFFSET,
    .n_ports  = UARTC0_N_PORTS,
    .port_l   = uartc0_port_l,
};

/* UARTC1,2 not used on Nano2G */
#ifndef IPOD_NANO2G
static struct uartc_port *uartc1_port_l[UARTC1_N_PORTS];
const struct uartc s5l8701_uartc1 =
{
    .id       = 1,
    .baddr    = UARTC1_BASE_ADDR,
    .port_off = UARTC1_PORT_OFFSET,
    .n_ports  = UARTC1_N_PORTS,
    .port_l   = uartc1_port_l,
};

static struct uartc_port *uartc2_port_l[UARTC2_N_PORTS];
const struct uartc s5l8701_uartc2 =
{
    .id       = 2,
    .baddr    = UARTC2_BASE_ADDR,
    .port_off = UARTC2_PORT_OFFSET,
    .n_ports  = UARTC2_N_PORTS,
    .port_l   = uartc2_port_l,
};
#endif

static uint8_t clockgate_uartc[S5L8701_N_UARTC] = {
    CLOCKGATE_UARTC0, CLOCKGATE_UARTC1, CLOCKGATE_UARTC2 };

static int intmsk_uart[S5L8701_N_UARTC] = {
    INTMSK_EINTG0, INTMSK_UART1, INTMSK_UART2 };

/*
 * Device level functions specific to S5L8701
 */
void uart_target_enable_gpio(int uart_id, int port_id)
{
    (void) port_id;
    switch (uart_id) {
        /* configure UARTx Tx/Rx GPIO ports */
        case 0:
            PCON1 = (PCON1 & 0xffffff00) | 0x00000044;
            break;
        case 1:
        case 2:
            /* unknown */
        default:
            break;
    }
}

void uart_target_disable_gpio(int uart_id, int port_id)
{
    (void) port_id;
    switch (uart_id) {
        /* configure minimal power consumption */
        case 0:
            PCON1 = (PCON1 & 0xffffff00) | 0x00000011;
            break;
        case 1:
        case 2:
        default:
            break;
    }
}

void uart_target_enable_irq(int uart_id, int port_id)
{
    (void) port_id;
    if (uart_id == 0) GPIOIC_INTEN(0) = 0x200;
    INTMSK |= intmsk_uart[uart_id];
}

void uart_target_disable_irq(int uart_id, int port_id)
{
    (void) port_id;
    INTMSK &= ~intmsk_uart[uart_id];
    if (uart_id == 0) GPIOIC_INTEN(0) = 0;
}

void uart_target_clear_irq(int uart_id, int port_id)
{
    (void) port_id;
    if (uart_id == 0) GPIOIC_INTSTAT(0) = 0x200;
    SRCPND |= intmsk_uart[uart_id];
}

void uart_target_enable_clocks(int uart_id)
{
    PWRCON &= ~(1 << clockgate_uartc[uart_id]);
}

void uart_target_disable_clocks(int uart_id)
{
    PWRCON |= (1 << clockgate_uartc[uart_id]);
}

/*
 * ISRs
 */

/* On Nano2G, PORT0 interrupts are not used when iAP is disabled */
#if !defined(IPOD_NANO2G) || defined(IPOD_ACCESSORY_PROTOCOL)
/*
 * UART0 IRQ is connected to EINTG0, this is a quick patch, a "real"
 * EINT handler will be needed if in future we use more than one IRQ
 * on this group.
 */
void ICODE_ATTR EINT_G0(void)
{
    GPIOIC_INTSTAT(0) = 0x200;  /* clear external IRQ */
    uartc_callback(&s5l8701_uartc0, 0);
}
#endif

/* UARTC1,2 not used on Nano2G */
#ifndef IPOD_NANO2G
void ICODE_ATTR INT_UART1(void)
{
    uartc_callback(&s5l8701_uartc1, 0);
}

void ICODE_ATTR INT_UART2(void)
{
    uartc_callback(&s5l8701_uartc2, 0);
}
#endif

/* Main init */
void uart_init(void)
{
    uartc_open(&s5l8701_uartc0);
}
