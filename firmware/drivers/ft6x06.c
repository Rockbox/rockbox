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

#define BYTES_PER_POINT 6

#ifdef FT6x06_SWAP_AXES
# define POS_X pos_y
# define POS_Y pos_x
#else
# define POS_X pos_x
# define POS_Y pos_y
#endif

struct ft6x06_driver {
    /* i2c bus data */
    int i2c_cookie;
    i2c_descriptor i2c_desc;

    /* callback for touch events */
    ft6x06_event_cb event_cb;

    /* buffer for I2C transfers */
    uint8_t raw_data[1 + 2 + BYTES_PER_POINT * FT6x06_NUM_POINTS];
};

static struct ft6x06_driver ft_drv;
struct ft6x06_state ft6x06_state;

static inline void ft6x06_convert_point(const uint8_t* raw,
                                        struct ft6x06_point* pt)
{
    pt->event    = (raw[0] >> 6) & 0x3;
    pt->touch_id = (raw[2] >> 4) & 0xf;
    pt->POS_X    = ((raw[0] & 0xf) << 8) | raw[1];
    pt->POS_Y    = ((raw[2] & 0xf) << 8) | raw[3];
    pt->weight   = raw[4];
    pt->area     = (raw[5] >> 4) & 0xf;
}

static void ft6x06_i2c_callback(int status, i2c_descriptor* desc)
{
    (void)desc;
    if(status != I2C_STATUS_OK)
        return;

    ft6x06_state.gesture = ft_drv.raw_data[1];
    ft6x06_state.nr_points = ft_drv.raw_data[2] & 0xf;
    for(int i = 0; i < FT6x06_NUM_POINTS; ++i) {
        ft6x06_convert_point(&ft_drv.raw_data[3 + i * BYTES_PER_POINT],
                             &ft6x06_state.points[i]);
    }

    ft_drv.event_cb(&ft6x06_state);
}

static void ft6x06_dummy_event_cb(struct ft6x06_state* state)
{
    (void)state;
}

void ft6x06_init(void)
{
    /* Initialize stuff */
    memset(&ft_drv, 0, sizeof(ft_drv));
    ft_drv.event_cb = ft6x06_dummy_event_cb;

    memset(&ft6x06_state, 0, sizeof(struct ft6x06_state));
    ft6x06_state.gesture = -1;
    for(int i = 0; i < FT6x06_NUM_POINTS; ++i)
        ft6x06_state.points[i].event = FT6x06_EVT_NONE;

    /* Reserve bus management cookie */
    ft_drv.i2c_cookie = i2c_async_reserve_cookies(FT6x06_BUS, 1);

    /* Prep an I2C descriptor to read touch data */
    ft_drv.i2c_desc.slave_addr = FT6x06_ADDR;
    ft_drv.i2c_desc.bus_cond   = I2C_START | I2C_STOP;
    ft_drv.i2c_desc.tran_mode  = I2C_READ;
    ft_drv.i2c_desc.buffer[0]  = &ft_drv.raw_data[0];
    ft_drv.i2c_desc.count[0]   = 1;
    ft_drv.i2c_desc.buffer[1]  = &ft_drv.raw_data[1];
    ft_drv.i2c_desc.count[1]   = sizeof(ft_drv.raw_data) - 1;
    ft_drv.i2c_desc.callback   = ft6x06_i2c_callback;
    ft_drv.i2c_desc.arg        = 0;
    ft_drv.i2c_desc.next       = NULL;

    /* Set I2C register address */
    ft_drv.raw_data[0] = 0x01;
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
