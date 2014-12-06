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

/* Include Standard files */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"

#include "s5l8702.h"
#include "uc8702.h"
#include "uart-s5l8702.h"


/* s5l8702 UART configuration */
struct uartc s5l8702_uart = {
     .baddr = S5L8702_UART_BASE
};

/*
 * Device level functions specific to S5L8702
 */
void uart_gpio_control(int port_id, bool onoff)
{
    if (onoff) {
        switch (port_id) {
            case 0:
                /* configure UART0 Tx/Rx GPIO ports */
                PCON0 = (PCON0 & 0xff00ffff) | 0x00220000;
                break;
            case 1:
                /* configure UART1 GPIO ports, including RTS/CTS signals */
                PCOND = (PCOND & 0xff0000ff) | 0x00222200;
                break;
            case 2:
            case 3:
                /* unknown, probably UART3/4 not routed on s5l8702 */
            default:
                break;
        }
    }
    else {
        /* configure minimal power consumption */
        switch (port_id) {
            case 0:
                PCON0 = (PCON0 & 0xff00ffff) | 0x00ee0000;
                break;
            case 1:
                PCOND = (PCOND & 0xff0000ff) | 0x00eeee00;
                break;
            case 2:
            case 3:
            default:
                break;
        }
    }
}

/* reset s5l8702 uart related hardware */
static void s5l8702_uart_hw_init(void)
{
    for (int id = 0; id < S5L8702_UART_PORT_MAX; id++) {
        VIC0INTENCLEAR = 1 << IRQ_UART(id); /* mask INT */
        uart_gpio_control(id, 0);
    }
}

void uart_init(void)
{
    s5l8702_uart_hw_init();
    PWRCON(1) &= ~(1 << (CLOCKGATE_UART - 32)); /* on */
    uartc_open(&s5l8702_uart);
}

void uart_close(void)
{
    uartc_close(&s5l8702_uart);
    PWRCON(1) |= (1 << (CLOCKGATE_UART - 32)); /* off */
    s5l8702_uart_hw_init();
}

void uart_port_init(struct uartc_port *port)
{
    uart_gpio_control(port->id, 1);
    uartc_port_open(port);
    VIC0INTENABLE = 1 << IRQ_UART(port->id); /* unmask INT */
}

void uart_port_close(struct uartc_port *port)
{
    VIC0INTENCLEAR = 1 << IRQ_UART(port->id); /* mask INT */
    uartc_port_close(port);
    uart_gpio_control(port->id, 0);
}

/* ISRs */
void ICODE_ATTR INT_UART0(void)
{
    uartc_callback(&s5l8702_uart, 0);
}

void ICODE_ATTR INT_UART1(void)
{
    uartc_callback(&s5l8702_uart, 1);
}
