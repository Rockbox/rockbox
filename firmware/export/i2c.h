/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#ifndef I2C_H
#define I2C_H

extern void i2c_init(void);
extern void i2c_begin(void);
extern void i2c_end(void);
extern int i2c_write(int device, const unsigned char* buf, int count );
extern int i2c_read(int device, unsigned char* buf, int count );
extern int i2c_readmem(int device, int address, unsigned char* buf, int count );
extern void i2c_outb(unsigned char byte);
extern unsigned char i2c_inb(int ack);
extern void i2c_start(void);
extern void i2c_stop(void);
extern void i2c_ack(int bit);
extern int i2c_getack(void);

#endif
