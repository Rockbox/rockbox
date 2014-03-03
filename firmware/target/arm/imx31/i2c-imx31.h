/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#ifndef I2C_IMX31_H
#define I2C_IMX31_H

#include <stdbool.h>
#include "semaphore.h"

/* I2C module usage masks */
#define USE_I2C1_MODULE         (1 << 0)
#define USE_I2C2_MODULE         (1 << 1)
#define USE_I2C3_MODULE         (1 << 2)

enum i2c_module_number
{
    __I2C_NUM_START = -1,
#if (I2C_MODULE_MASK & USE_I2C1_MODULE)
    I2C1_NUM,
#endif
#if (I2C_MODULE_MASK & USE_I2C2_MODULE)
    I2C2_NUM,
#endif
#if (I2C_MODULE_MASK & USE_I2C3_MODULE)
    I2C3_NUM,
#endif
    I2C_NUM_I2C,
};

struct i2c_node
{
    enum i2c_module_number num; /* Module that this node uses */
    unsigned int ifdr;          /* Maximum frequency for node */
    unsigned char addr;         /* Slave address on module */
};

struct i2c_transfer_desc;

typedef void (*i2c_transfer_cb_fn_type)(struct i2c_transfer_desc *);

/* Basic transfer descriptor for normal asynchronous read/write */
struct i2c_transfer_desc
{
    struct i2c_node     *node;
    const unsigned char *txdata;
    int                  txcount;
    unsigned char       *rxdata;
    int                  rxcount;
    i2c_transfer_cb_fn_type callback;
    struct i2c_transfer_desc *next;
};

/* Extended transfer descriptor for synchronous read/write that handles
   thread wait and wakeup */
struct i2c_sync_transfer_desc
{
    struct i2c_transfer_desc xfer;
    struct semaphore sema;
};

/* One-time init of i2c driver */
void i2c_init(void);

/* Enable or disable the node - modules will be switched on/off accordingly. */
void i2c_enable_node(struct i2c_node *node, bool enable);

/* Send and/or receive data on the specified node asynchronously */
bool i2c_transfer(struct i2c_transfer_desc *xfer);

/* Read bytes from a device on the I2C bus, with optional sub-addressing
 * If addr < 0, then raw read without sub-addressing */
int i2c_read(struct i2c_node *node, int addr, unsigned char *data,
             int data_count);

/* Write bytes to a device on the I2C bus */
int i2c_write(struct i2c_node *node, const unsigned char *data,
              int data_count);

#endif /* I2C_IMX31_H */
