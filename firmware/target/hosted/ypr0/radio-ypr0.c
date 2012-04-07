/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for SI4709 FM Radio Chip, using /dev/si470x (si4709.ko) of Samsung YP-R0
 *
 * Copyright (c) 2012 Lorenzo Miori
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "stdint.h"
#include "string.h"

#include "radio-ypr0.h"

static int radio_dev = -1;

void radiodev_open(void) {
    radio_dev = open("/dev/si470x", O_RDWR);
}

void radiodev_close(void) {
    close(radio_dev);
}

/* High-level registers access */
void si4709_write_reg(int addr, uint16_t value) {
    sSi4709_t r = { .addr = addr, .value = value };
    ioctl(radio_dev, IOCTL_SI4709_WRITE_BYTE, &r);
}

uint16_t si4709_read_reg(int addr) {
    sSi4709_t r = { .addr = addr, .value = 0 };
    ioctl(radio_dev, IOCTL_SI4709_READ_BYTE, &r);
    return r.value;
}

/* Low-level i2c channel access */
int fmradio_i2c_write(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    sSi4709_i2c_t r = { .size = count, .buf = buf };
    return ioctl(radio_dev, IOCTL_SI4709_I2C_WRITE, &r);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    sSi4709_i2c_t r = { .size = count, .buf = buf };
    return ioctl(radio_dev, IOCTL_SI4709_I2C_READ, &r);
}