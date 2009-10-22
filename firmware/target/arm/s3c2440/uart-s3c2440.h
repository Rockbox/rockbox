/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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

#ifndef __UART_S3C2440_H__
#define __UART_S3C2440_H__

#define UART_DEBUG        0

#define UART_NO_PARITY    0
#define UART_ODD_PARITY   4
#define UART_EVEN_PARITY  5
#define UART_MARK_PARITY  6
#define UART_SPACE_PARITY 7

#define UART_1_STOP_BIT   0
#define UART_2_STOP_BIT   1

bool uart_init (void);
void uart_printf (const char *format, ...);

/* low level routines */
bool uart_init_device (unsigned dev);
bool uart_config (unsigned dev, unsigned speed, unsigned num_bits, unsigned parity, unsigned stop_bits);
bool uart_send (unsigned dev, char *buf, unsigned len);

char uart_read_byte (unsigned dev);
char uart_rx_ready (unsigned dev);


#endif
