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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
