/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Lorenzo Miori
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
#include "uartdbg-imx233.h"
#include "pinctrl-imx233.h"
#include "clkctrl-imx233.h"

#include "regs/uartdbg.h"

void imx233_uartdbg_init(unsigned long baud)
{
    /* Enable UART clock */
    imx233_clkctrl_enable(CLK_UART, true);
    /* Configure UART pins */
    imx233_pinctrl_setup_vpin(VPIN_UARTDBG_TX, "uartdbg tx", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_UARTDBG_RX, "uartdbg rx", PINCTRL_DRIVE_4mA, false);

    /* Set baud rate */
    HW_UARTDBG_IBRD = baud >> 16;
    HW_UARTDBG_FBRD = baud & 0xFFFF;

    /* Set port options (actually needed to set baud rate), 8 bit char, enable FIFO buffer */
    BF_WR(UARTDBG_LCR_H, WLEN(3));
    BF_WR(UARTDBG_LCR_H, FEN(1));

    /* Finally enable UART device, TX enable, RX enable, device enable */
    BF_WR(UARTDBG_CR, TXE(1));
    BF_WR(UARTDBG_CR, RXE(1));
    BF_WR(UARTDBG_CR, UARTEN(1));
}

void imx233_uartdbg_send(unsigned char data)
{
    /* Wait to transmit if TX FIFO buffer is full*/
    while (BF_RD(UARTDBG_FR, TXFF));
    BF_WR(UARTDBG_DR, DATA(data));
}

void uart_tx(const char* data)
{
    while (*data != 0)
        imx233_uartdbg_send(*data++);
}

unsigned int uart_rx(char* rx_buf, unsigned int len)
{
    unsigned int i = 0;

    while(i < len)
    {
        /* Check if the UART Rx Buffer has something into -> RXFE */
        if(!BF_RD(UARTDBG_FR, RXFE))
        {
            rx_buf[i] = HW_UARTDBG_DR;
            i++;
        }
    }
    return i;
}