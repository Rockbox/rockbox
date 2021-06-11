/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "ft6x06.h"
#include "kernel.h"
#include "i2c-async.h"
#include <string.h>

struct ft6x06_driver {
    /* i2c bus data */
    int i2c_cookie;
    i2c_descriptor i2c_desc;

    /* callback for touch events */
    ft6x06_event_cb event_cb;

    /* buffer for I2C transfers */
    uint8_t raw_data[6];
};

static struct ft6x06_driver ft_drv;
struct ft6x06_state ft6x06_state;

static void ft6x06_i2c_callback(int status, i2c_descriptor* desc)
{
    (void)desc;
    if(status != I2C_STATUS_OK)
        return;

    int evt = ft_drv.raw_data[1] >> 6;
    int tx = ft_drv.raw_data[2] | ((ft_drv.raw_data[1] & 0xf) << 8);
    int ty = ft_drv.raw_data[4] | ((ft_drv.raw_data[3] & 0xf) << 8);

    ft6x06_state.event = evt;
#ifdef FT6x06_SWAP_AXES
    ft6x06_state.pos_x = ty;
    ft6x06_state.pos_y = tx;
#else
    ft6x06_state.pos_x = tx;
    ft6x06_state.pos_y = ty;
#endif

    ft_drv.event_cb(evt, ft6x06_state.pos_x, ft6x06_state.pos_y);
}

static void ft6x06_dummy_event_cb(int evt, int tx, int ty)
{
    (void)evt;
    (void)tx;
    (void)ty;
}

void ft6x06_init(void)
{
    /* Initialize stuff */
    memset(&ft_drv, 0, sizeof(ft_drv));
    ft_drv.event_cb = ft6x06_dummy_event_cb;

    ft6x06_state.event = FT6x06_EVT_NONE;
    ft6x06_state.pos_x = 0;
    ft6x06_state.pos_y = 0;

    /* Reserve bus management cookie */
    ft_drv.i2c_cookie = i2c_async_reserve_cookies(FT6x06_BUS, 1);

    /* Prep an I2C descriptor to read touch data */
    ft_drv.i2c_desc.slave_addr = FT6x06_ADDR;
    ft_drv.i2c_desc.bus_cond   = I2C_START | I2C_STOP;
    ft_drv.i2c_desc.tran_mode  = I2C_READ;
    ft_drv.i2c_desc.buffer[0]  = &ft_drv.raw_data[5];
    ft_drv.i2c_desc.count[0]   = 1;
    ft_drv.i2c_desc.buffer[1]  = &ft_drv.raw_data[0];
    ft_drv.i2c_desc.count[1]   = 5;
    ft_drv.i2c_desc.callback   = ft6x06_i2c_callback;
    ft_drv.i2c_desc.arg        = 0;
    ft_drv.i2c_desc.next       = NULL;

    /* Set I2C register address */
    ft_drv.raw_data[5] = 0x02;
}

void ft6x06_set_event_cb(ft6x06_event_cb cb)
{
    ft_drv.event_cb = cb ? cb : ft6x06_dummy_event_cb;
}

void ft6x06_enable(bool en)
{
    i2c_reg_write1(FT6x06_BUS, FT6x06_ADDR, 0xa5, en ? 0 : 3);
}

void ft6x06_irq_handler(void)
{
    /* We don't care if this fails, there's not much we can do about it */
    i2c_async_queue(FT6x06_BUS, TIMEOUT_NOBLOCK, I2C_Q_ONCE,
                    ft_drv.i2c_cookie, &ft_drv.i2c_desc);
}
