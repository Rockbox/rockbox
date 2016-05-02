/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __I2C_IMX233_H__
#define __I2C_IMX233_H__

#include "cpu.h"
#include "system.h"
#include "system-target.h"

enum imx233_i2c_error_t
{
    I2C_SUCCESS = 0,
    I2C_ERROR = -1,
    I2C_TIMEOUT = -2,
    I2C_MASTER_LOSS = -3,
    I2C_NO_SLAVE_ACK = -4,
    I2C_SLAVE_NAK = -5,
    I2C_BUFFER_FULL = -6,
    I2C_SKIP = -7, /* transfer skipped before of previous error in transaction */
};

/** Important notes on the driver.
 *
 * The driver supports both synchronous and asynchronous transfers. Asynchronous
 * transfer functions can safely be called from IRQ context. Beware that the completion
 * callback of asynchronous transfers may be called from IRQ context.
 *
 * The driver supports queuing so it is safe to call several transfer functions
 * concurrently.
 */

void imx233_i2c_init(void);

/** legacy API:
 * read/write functions return 0 on success */
int i2c_write(int device, const unsigned char* buf, int count);
int i2c_read(int device, unsigned char* buf, int count);
int i2c_readmem(int device, int address, unsigned char* buf, int count);
int i2c_writemem(int device, int address, const unsigned char* buf, int count);

/** Advanced API */
struct imx233_i2c_xfer_t;
typedef void (*imx233_i2c_cb_t)(struct imx233_i2c_xfer_t *xfer, enum imx233_i2c_error_t status);

/** Transfer mode. There are currently two types of transfers, to make
 * programming simpler:
 * - write: write count[0] bytes from data[0], and then count[1] bytes from data[1]
 *          (if count[1] > 0). The second stage is useful to avoid allocating a single
 *          buffer to hold address + data for example.
 * - read: write count[0] bytes from data[0], and then read count[1] bytes from data[1].
 *         If count[0] = 0 then the write stage is ignored.
 */
enum imx233_i2x_xfer_mode_t
{
    I2C_WRITE,
    I2C_READ,
};

/** This data structure represents one transfer. The exact shape of the transfer
 * depends on the mode.
 * NOTE a single transfer is limited to 512 bytes of data (RX + TX)
 * A transaction is made up of several transfers, chained together.
 */
struct imx233_i2c_xfer_t
{
    struct imx233_i2c_xfer_t *next; /* next transfer, or NULL */
    imx233_i2c_cb_t callback; /* NULL for no callack */
    int dev_addr; /* device address */
    bool fast_mode; /* 400 kHz 'fast' mode or 100 kHz 'normal' mode */
    enum imx233_i2x_xfer_mode_t mode; /* transfer mode */
    int count[2]; /* count: depends on mode */
    void *data[2]; /* data: depends on mode */
    unsigned tmo_ms; /* timeout in milliseconds (0 means infinite) */
    /* internal */
    struct imx233_i2c_xfer_t *last; /* last in transaction */
};

/* Queue one or more tranfer. If several transfer are queued (transaction)
 * they are guaranteed to be executed "as one" (ie without interleaving). Furthermore,
 * if a transfer of the transfaction fails, the remaining transfers of the transactions
 * will NOT be executed (and their callback will be called with SKIP status).
 * Return 0 if queueing was successful. Note that the transfer may still fail,
 * in which case the callback will have a nonzero status code. */
void imx233_i2c_transfer(struct imx233_i2c_xfer_t *xfer);

#endif // __I2C_IMX233_H__
