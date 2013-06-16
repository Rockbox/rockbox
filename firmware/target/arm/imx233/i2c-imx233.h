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
#include "i2c.h"

#include "regs/regs-i2c.h"

#define BM_I2C_CTRL1_ALL_IRQ \
    BM_OR8(I2C_CTRL1, SLAVE_IRQ, SLAVE_STOP_IRQ, MASTER_LOSS_IRQ, EARLY_TERM_IRQ, \
        OVERSIZE_XFER_TERM_IRQ, NO_SLAVE_ACK_IRQ, DATA_ENGINE_CMPLT_IRQ, BUS_FREE_IRQ)

enum imx233_i2c_error_t
{
    I2C_SUCCESS = 0,
    I2C_ERROR = -1,
    I2C_TIMEOUT = -2,
    I2C_MASTER_LOSS = -3,
    I2C_NO_SLAVE_ACK = -4,
    I2C_SLAVE_NAK = -5,
    I2C_BUFFER_FULL = -6,
};

void imx233_i2c_init(void);
/* start building a transfer, will acquire an exclusive lock */
void imx233_i2c_begin(void);
/* add stage */
enum imx233_i2c_error_t imx233_i2c_add(bool start, bool transmit, void *buffer, unsigned size, bool stop);
/* end building a transfer and start the transfer */
enum imx233_i2c_error_t imx233_i2c_end(unsigned timeout);

#endif // __I2C_IMX233_H__
