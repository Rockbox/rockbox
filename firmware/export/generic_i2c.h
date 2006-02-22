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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _GEN_I2C_
#define _GEN_I2C_

struct i2c_interface
{
    unsigned char address; /* Address of the chip that this interface
                              describes */

    void (*scl_hi)(void);  /* Drive SCL high, might sleep on clk stretch */
    void (*scl_lo)(void);  /* Drive SCL low */
    void (*sda_hi)(void);  /* Drive SDA high */
    void (*sda_lo)(void);  /* Drive SDA low */
    void (*sda_input)(void);  /* Set SDA as input */
    void (*sda_output)(void); /* Set SDA as output */
    void (*scl_input)(void);  /* Set SCL as input */
    void (*scl_output)(void); /* Set SCL as output */
    int (*scl)(void);      /* Read SCL, returns 0 or non-zero */
    int (*sda)(void);      /* Read SDA, returns 0 or non-zero */

    /* These are the delays specified in the I2C specification, the
       time pairs to the right are the minimum 100kHz and 400kHz specs */
    void (*delay_hd_sta)(void); /* START SDA hold (tHD:SDA)    4.0us/0.6us */
    void (*delay_hd_dat)(void); /* SDA hold (tHD:DAT)          5.0us/0us   */
    void (*delay_su_dat)(void); /* SDA setup (tSU:DAT)         250ns/100ns */
    void (*delay_su_sto)(void); /* STOP setup (tSU:STO)        4.0us/0.6us */
    void (*delay_su_sta)(void); /* Rep. START setup (tSU:STA)  4.7us/0.6us */
    void (*delay_thigh)(void);  /* SCL high period (tHIGH)     4.0us/0.6us */
};

extern void i2c_add_node(struct i2c_interface *iface);
extern int i2c_write_data(int bus_address, int address,
                          const unsigned char* buf, int count);
extern int i2c_read_data(int bus_address, int address,
                         unsigned char* buf, int count);
#endif
