/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#ifndef _GEN_I2C_
#define _GEN_I2C_

#include <stdbool.h>

struct i2c_interface
{
    void (*scl_dir)(bool out);  /* Set SCL as input or output */
    void (*sda_dir)(bool out);  /* Set SDA as input or output */
    void (*scl_out)(bool high); /* Drive SCL, might sleep on clk stretch */
    void (*sda_out)(bool high); /* Drive SDA */
    bool (*scl_in)(void);       /* Read SCL, returns true if high */
    bool (*sda_in)(void);       /* Read SDA, returns true if high */
    void (*delay)(int delay);   /* Delay for the specified amount */
    
    /* These are the delays specified in the I2C specification, the
       time pairs to the right are the minimum 100kHz and 400kHz specs */
    int delay_hd_sta;           /* START SDA hold (tHD:SDA)    4.0us/0.6us */
    int delay_hd_dat;           /* SDA hold (tHD:DAT)          5.0us/0us   */
    int delay_su_dat;           /* SDA setup (tSU:DAT)         250ns/100ns */
    int delay_su_sto;           /* STOP setup (tSU:STO)        4.0us/0.6us */
    int delay_su_sta;           /* Rep. START setup (tSU:STA)  4.7us/0.6us */
    int delay_thigh;            /* SCL high period (tHIGH)     4.0us/0.6us */
};

int i2c_add_node(const struct i2c_interface *iface);
int i2c_write_data(int bus_index, int bus_address, int address,
                   const unsigned char* buf, int count);
int i2c_read_data(int bus_index, int bus_address, int address,
                  unsigned char* buf, int count);

/* Special function for devices that can appear on I2C bus but do not
 * comply to I2C specification. Such devices include AT88SC6416C crypto
 * memory. To read data from AT88SC6416C, a write I2C transaction starts,
 * 3 bytes are written and then, in the middle of transaction, the device
 * starts sending data.
 */
int i2c_write_read_data(int bus_index, int bus_address,
                        const unsigned char* buf_write, int count_write,
                        unsigned char* buf_read, int count_read);

#endif /* _GEN_I2C_ */

