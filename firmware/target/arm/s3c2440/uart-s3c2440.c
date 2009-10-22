/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"

#include "uart-s3c2440.h"

#define FCLK 405000000
#define HCLK (FCLK/4)   /* = 101,250,000 */
#define PCLK (HCLK/2)   /* =  50,625,000 */

#define MAX_TX_BUF        1024


/****************************************************************************
 * General purpose debug function
 ****************************************************************************/

void uart_printf (const char *format, ...)
{
    static bool debug_uart_init = false;
    static char tx_buf [MAX_TX_BUF];

    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = tx_buf;
    len = vsnprintf(ptr, sizeof(tx_buf), format, ap);
    va_end(ap);

    if (!debug_uart_init)
    {
      uart_init_device(UART_DEBUG);
      debug_uart_init = true;
    }
    
    uart_send (UART_DEBUG, tx_buf, len);
}

/****************************************************************************
 * Device level functions specific to S3C2440
 *****************************************************************************/

bool uart_init (void)
{
    /* anything ? */
    return true;
}

bool uart_init_device (unsigned dev)
{
    /* set GPIOs, clock enable? etc */

    switch (dev)
    {
        case 0:
        {
            S3C2440_GPIO_CONFIG (GPHCON, 2, GPIO_FUNCTION);
            S3C2440_GPIO_CONFIG (GPHCON, 3, GPIO_FUNCTION);
            S3C2440_GPIO_PULLUP (GPHUP, 2, GPIO_PULLUP_DISABLE);
            S3C2440_GPIO_PULLUP (GPHUP, 3, GPIO_PULLUP_DISABLE);
            break;
        }
        case 1:
        {
            S3C2440_GPIO_CONFIG (GPHCON, 4, GPIO_FUNCTION);
            S3C2440_GPIO_CONFIG (GPHCON, 5, GPIO_FUNCTION);
            S3C2440_GPIO_PULLUP (GPHUP, 4, GPIO_PULLUP_DISABLE);
            S3C2440_GPIO_PULLUP (GPHUP, 5, GPIO_PULLUP_DISABLE);
            break;
        }
        case 2:
        {
            S3C2440_GPIO_CONFIG (GPHCON, 6, GPIO_FUNCTION);
            S3C2440_GPIO_CONFIG (GPHCON, 7, GPIO_FUNCTION);
            S3C2440_GPIO_PULLUP (GPHUP, 6, GPIO_PULLUP_DISABLE);
            S3C2440_GPIO_PULLUP (GPHUP, 7, GPIO_PULLUP_DISABLE);
            break;
        }
        default:
            return false;
    }
    
    /* set a default configuration */
    uart_config (dev, 115200, 8, UART_NO_PARITY, UART_1_STOP_BIT); 
    return true;
}

bool uart_config (unsigned dev, unsigned speed, unsigned num_bits, 
                  unsigned parity, unsigned stop_bits)
{
    switch (dev)
    {
    case 0:
        ULCON0  = (parity << 3) + (stop_bits << 2) + (num_bits-5);
        UCON0   = (1 << 2) + (1 << 0);   /* enable TX, RX, use PCLK */
        UBRDIV0 = PCLK / (speed*16);
        break;

    case 1:
        ULCON1  = (parity << 3) + (stop_bits << 2) + (num_bits-5);
        UCON1   = (1 << 2) + (1 << 0);   /* enable TX, RX, use PCLK */
        UBRDIV1 = PCLK / (speed*16);
        break;

    case 2:
        ULCON2  = (parity << 3) + (stop_bits << 2) + (num_bits-5);
        UCON2   = (1 << 2) + (1 << 0);   /* enable TX, RX, use PCLK */
        UBRDIV2 = PCLK / (speed*16);
        break;
    }
    
    return true;
}

bool uart_send_byte (unsigned dev, char ch)
{
    switch (dev)
    {
    case 0:
        /* wait for transmit buffer empty */
        while ((UTRSTAT0 & 0x02) == 0)
          ;
        UTXH0 = ch;
        break;

    case 1:
        /* wait for transmit buffer empty */
        while ((UTRSTAT1 & 0x02) == 0)
          ;
        UTXH1 = ch;
        break;

    case 2:
        /* wait for transmit buffer empty */
        while ((UTRSTAT2 & 0x02) == 0)
          ;
        UTXH2 = ch;
        break;
    }
    
    return true;
}

char uart_rx_ready (unsigned dev)
{
    switch (dev)
    {
    case 0:
        /* wait for receive buffer data ready */
        if (UTRSTAT0 & 0x01)
            return true;
        else
            return false;
        break;
    case 1:
        /* wait for receive buffer data ready */
        if (UTRSTAT1 & 0x01)
            return true;
        else
            return false;
        break;
    case 2:
        /* wait for receive buffer data ready */
        if (UTRSTAT2 & 0x01)
            return true;
        else
            return false;
        break;
    }
    return false;
}

char uart_read_byte (unsigned dev)
{
    switch (dev)
    {
    case 0:
        while (!uart_rx_ready(dev))
            ;
        return URXH0;
        break;
    case 1:
        while (!uart_rx_ready(dev))
            ;
        return URXH1;
        break;
    case 2:
        while (!uart_rx_ready(dev))
            ;
        return URXH2;
        break;
    }
    
    return true;
}

/****************************************************************************
 * General
 *****************************************************************************/

bool uart_send (unsigned dev, char *buf, unsigned len)
{
    unsigned index=0;
    while (index<len)
    {
        uart_send_byte (dev, buf[index]);
        
        /* hack for ASCII terminals */
        if (buf[index] == '\n')
          uart_send_byte (dev, '\r');
          
        index++;
    }
    return true;
}


/****************************************************************************/
