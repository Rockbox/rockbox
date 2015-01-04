/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Bluetooth transport
 *
 * Copyright (C) 2015 Lorenzo Miori
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

#include "stdbool.h"
#include "config.h"

#ifndef __BLUETOOTH_TRANSPORT_H__
#define __BLUETOOTH_TRANSPORT_H__

/**
 * Bluetooth Transport
 * Low-level controller communication
 */

/* Initialize any target-specific hardware */
void transport_init(void) INIT_ATTR;

/* Flush transport */
void transport_reset(bool asserted);

/* Write data */
int transport_write(char* data, int len);

/* Read data */
int transport_read(char* data, int len);

/* Get current speed */
int transport_get_speed(void);

void transport_close(void);

#endif /* __BLUETOOTH_TRANSPORT_H__ */
