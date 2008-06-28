/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * PP502X and PP5002 I2C driver
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in November 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/hardware.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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

/*
 * PortalPlayer i2c driver
 *
 */
 
#ifndef _I2C_PP_H
#define _I2C_PP_H

#define I2C_CTRL    (*(volatile unsigned char*)(I2C_BASE+0x00))
#define I2C_ADDR    (*(volatile unsigned char*)(I2C_BASE+0x04))
#define I2C_DATA(X) (*(volatile unsigned char*)(I2C_BASE+0xc+(4*X)))
#define I2C_STATUS  (*(volatile unsigned char*)(I2C_BASE+0x1c))

/* I2C_CTRL bit definitions */
#define I2C_SEND    0x80

/* I2C_STATUS bit definitions */
#define I2C_BUSY    (1<<6)

/* TODO: Fully implement i2c driver */

/* To be used by drivers that need to do multiple i2c operations
   atomically */
void i2c_lock(void);
void i2c_unlock(void);

void i2c_init(void);
int i2c_readbyte(unsigned int dev_addr, int addr);
int pp_i2c_send(unsigned int addr, int data0, int data1);
int i2c_readbytes(unsigned int dev_addr, int addr, int len, unsigned char *data);

#endif
