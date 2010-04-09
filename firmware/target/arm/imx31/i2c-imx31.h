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

/* Module interface map structure */
struct i2c_map
{
    volatile uint16_t iadr;    /* 0x00 */
    volatile uint16_t unused1;
    volatile uint16_t ifdr;    /* 0x04 */
    volatile uint16_t unused2;
    volatile uint16_t i2cr;    /* 0x08 */
    volatile uint16_t unused3;
    volatile uint16_t i2sr;    /* 0x0C */
    volatile uint16_t unused4;
    volatile uint16_t i2dr;    /* 0x10 */
};

struct i2c_node
{
    enum i2c_module_number num; /* Module that this node uses */
    unsigned int ifdr;          /* Maximum frequency for node */
    unsigned char addr;         /* Slave address on module */
};

void i2c_init(void);
/* Enable or disable the node - modules will be switch on/off accordingly. */
void i2c_enable_node(struct i2c_node *node, bool enable);
/* If addr < 0, then raw read */
int i2c_read(struct i2c_node *node, int addr, unsigned char *data, int count);
int i2c_write(struct i2c_node *node, const unsigned char *data, int count);
/* Gain mutually-exclusive access to the node and module to perform multiple
 * operations atomically */
void i2c_lock_node(struct i2c_node *node);
void i2c_unlock_node(struct i2c_node *node);

#endif /* I2C_IMX31_H */
