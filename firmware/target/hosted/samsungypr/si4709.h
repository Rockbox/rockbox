/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module header for SI4709 FM Radio Chip, using /dev/si470x (si4709.ko) of Samsung YP-R0
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

#ifndef __SI4709_H__
#define __SI4709_H__

#include "stdint.h"

/* 7bits I2C address */
#define SI4709_I2C_SLAVE_ADDR       0x10

#define SI4702_DEVICEID     0x00
#define SI4702_CHIPID       0x01
#define SI4702_POWERCFG     0x02
#define SI4702_CHANNEL      0x03
#define SI4702_SYSCONFIG1   0x04
#define SI4702_SYSCONFIG2   0x05
#define SI4702_SYSCONFIG3   0x06
#define SI4702_TEST1        0x07
#define SI4702_TEST2        0x08
#define SI4702_B00TCONFIG   0x09
#define SI4702_STATUSRSSI   0x0A
#define SI4702_READCHAN     0x0B
#define SI4709_REG_NUM      0x10
#define SI4702_REG_BYTE     (SI4709_REG_NUM * 2)
#define SI4702_DEVICE_ID    0x1242
#define SI4702_RW_REG_NUM   (SI4702_STATUSRSSI - SI4702_POWERCFG)
#define SI4702_RW_OFFSET    \
    (SI4709_REG_NUM - SI4702_STATUSRSSI + SI4702_POWERCFG)
#define BYTE_TO_WORD(hi, lo)    (((hi) << 8) & 0xFF00) | ((lo) & 0x00FF)

typedef struct {
    int addr;
    uint16_t value;
}__attribute__((packed)) sSi4709_t;

typedef struct {
    int size;
    unsigned char *buf;
}__attribute__((packed)) sSi4709_i2c_t;

typedef enum
{
    IOCTL_SI4709_INIT = 0,
    IOCTL_SI4709_CLOSE,
    IOCTL_SI4709_WRITE_BYTE,
    IOCTL_SI4709_READ_BYTE,
    IOCTL_SI4709_I2C_WRITE,
    IOCTL_SI4709_I2C_READ,

    E_IOCTL_SI4709_MAX
} eSi4709_ioctl_t;

#define DRV_IOCTL_SI4709_MAGIC     'S'

#define IOCTL_SI4709_INIT          _IO(DRV_IOCTL_SI4709_MAGIC, IOCTL_SI4709_INIT)
#define IOCTL_SI4709_CLOSE         _IO(DRV_IOCTL_SI4709_MAGIC, IOCTL_SI4709_CLOSE)
#define IOCTL_SI4709_WRITE_BYTE    _IOW(DRV_IOCTL_SI4709_MAGIC, IOCTL_SI4709_WRITE_BYTE, sSi4709_t)
#define IOCTL_SI4709_READ_BYTE     _IOR(DRV_IOCTL_SI4709_MAGIC, IOCTL_SI4709_READ_BYTE, sSi4709_t)
#define IOCTL_SI4709_I2C_WRITE     _IOW(DRV_IOCTL_SI4709_MAGIC, IOCTL_SI4709_I2C_WRITE, sSi4709_i2c_t)
#define IOCTL_SI4709_I2C_READ      _IOR(DRV_IOCTL_SI4709_MAGIC, IOCTL_SI4709_I2C_READ, sSi4709_i2c_t)

#endif /* __SI4709_H__ */