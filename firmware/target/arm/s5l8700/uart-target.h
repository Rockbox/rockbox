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
#ifndef __UART_TARGET_H__
#define __UART_TARGET_H__

/* Define this to show debug data on "View HW Info" */
/* #define UC870X_DEBUG */

void uart_init(void);

/* s5l870x low level routines */
void uart_target_enable_clocks(int uart_id);
void uart_target_disable_clocks(int uart_id);
void uart_target_enable_irq(int uart_id, int port_id);
void uart_target_disable_irq(int uart_id, int port_id);
void uart_target_clear_irq(int uart_id, int port_id);
void uart_target_enable_gpio(int uart_id, int port_id);
void uart_target_disable_gpio(int uart_id, int port_id);

#endif /* __UART_TARGET_H__ */
