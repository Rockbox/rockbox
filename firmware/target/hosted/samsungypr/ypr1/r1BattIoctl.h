/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
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

/**
 * This is the wrapper to r1Bat.ko module with the possible
 * ioctl calls, retrieved by RE
 * The "Fuel gauge" - battery controller - is the MAX17040GT
 */

/* A typical read spans 2 registers, so here the suitable struct */
typedef struct {
    uint8_t addr;
    uint8_t reg1;
    uint8_t reg2;
}__attribute__((packed)) max17040_request;


/* Registers are 16-bit wide */
#define GET_BATTERY_VOLTAGE             0x80045800
#define GET_BATTERY_CAPACITY            0x80045801
#define READ_MAX17040_REG               0x80035803
#define WRITE_MAX17040_REG              0x40035802

void max17040_init(void);
void max17040_close(void);
int max17040_ioctl(int request, int *data);
