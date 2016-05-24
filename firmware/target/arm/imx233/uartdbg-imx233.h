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
#include "system.h"

/* This values below are valid with a XCLK of 24MHz */
#define BAUD_9600               (uint32_t)(156 << 16 | 16)
#define BAUD_19200              (uint32_t)(78 << 16 | 2)
#define BAUD_38400              (uint32_t)(39 << 16 | 4)
#define BAUD_57600              (uint32_t)(26 << 16 | 3)
#define BAUD_115200             (uint32_t)(13 << 16 | 1)

void imx233_uartdbg_init(unsigned long baud);
void imx233_uartdbg_send(unsigned char data);

void uart_tx(const char* data);
unsigned int uart_rx(char* rx_buf, unsigned int len);