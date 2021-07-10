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
#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include <stdlib.h>
#include "debug.h"
#include "generic_i2c.h"

#define MAX_I2C_INTERFACES 5

static int i2c_num_ifs = 0;
static const struct i2c_interface *i2c_if[MAX_I2C_INTERFACES];

static void i2c_start(const struct i2c_interface *iface)
{
    iface->sda_dir(true);

    iface->sda_out(1);
    iface->scl_out(1);
    iface->delay(iface->delay_su_sta);
    iface->sda_out(0);
    iface->delay(iface->delay_hd_sta);
    iface->scl_out(0);
    iface->delay(iface->delay_hd_dat);
}

static void i2c_stop(const struct i2c_interface *iface)
{
    iface->sda_dir(true);

    iface->sda_out(0);
    iface->delay(iface->delay_su_dat);
    iface->scl_out(1);
    iface->delay(iface->delay_su_sto);
    iface->sda_out(1);
}

static void i2c_ack(const struct i2c_interface *iface, bool ack)
{
    iface->sda_dir(true);
    iface->sda_out(!ack);
    iface->delay(iface->delay_su_dat);
    iface->scl_out(1);
    iface->delay(iface->delay_thigh);
    iface->scl_out(0);
    iface->delay(iface->delay_hd_dat);
}

static int i2c_getack(const struct i2c_interface *iface)
{
    int ret = 1;

    iface->sda_dir(false);
    iface->delay(iface->delay_su_dat);
    iface->scl_out(1);
    iface->delay(iface->delay_thigh);
    if (iface->sda_in())
        ret = 0; /* ack failed */
    iface->scl_out(0);
    iface->delay(iface->delay_hd_dat);
    return ret;
}

static unsigned char i2c_inb(const struct i2c_interface *iface, bool ack)
{
    int i;
    unsigned char byte = 0;

    iface->sda_dir(false);

    /* clock in each bit, MSB first */
    for ( i=0x80; i; i>>=1 ) {
        iface->delay(iface->delay_su_dat);
        iface->scl_out(1);
        iface->delay(iface->delay_thigh);
        if (iface->sda_in())
            byte |= i;
        iface->scl_out(0);
        iface->delay(iface->delay_hd_dat);
    }

    i2c_ack(iface, ack);

    return byte;
}

static int i2c_outb(const struct i2c_interface *iface, unsigned char byte)
{
    int i;

    iface->sda_dir(true);

    /* clock out each bit, MSB first */
    for (i=0x80; i; i>>=1) {
        iface->sda_out(i & byte);
        iface->delay(iface->delay_su_dat);
        iface->scl_out(1);
        iface->delay(iface->delay_thigh);
        iface->scl_out(0);
        iface->delay(iface->delay_hd_dat);
   }

   return i2c_getack(iface);
}

int i2c_write_data(int bus_index, int bus_address, int address,
                   const unsigned char* buf, int count)
{
    int i;
    int ret = 0;
    const struct i2c_interface *iface = i2c_if[bus_index];

    i2c_start(iface);
    if (!i2c_outb(iface, bus_address & 0xfe))
    {
        ret = -2;
        goto end;
    }

    if (address != -1)
    {
        if (!i2c_outb(iface, address))
        {
            ret = -3;
            goto end;
        }
    }

    for(i = 0;i < count;i++)
    {
        if (!i2c_outb(iface, buf[i]))
        {
            ret = -4;
            break;
        }
    }

end:
    i2c_stop(iface);
    return ret;
}

int i2c_read_data(int bus_index, int bus_address, int address,
                  unsigned char* buf, int count)
{
    int i;
    int ret = 0;
    const struct i2c_interface *iface = i2c_if[bus_index];

    if (address != -1)
    {
        i2c_start(iface);
        if (!i2c_outb(iface, bus_address & 0xfe))
        {
            ret = -2;
            goto end;
        }
        if (!i2c_outb(iface, address))
        {
            ret = -3;
            goto end;
        }
    }

    i2c_start(iface);
    if (!i2c_outb(iface, bus_address | 1))
    {
        ret = -4;
        goto end;
    }

    for(i = 0;i < count-1;i++)
        buf[i] = i2c_inb(iface, true);

    buf[i] = i2c_inb(iface, false);

end:
    i2c_stop(iface);
    return ret;
}

int i2c_write_read_data(int bus_index, int bus_address,
                        const unsigned char* buf_write, int count_write,
                        unsigned char* buf_read, int count_read)
{
    int i;
    int ret = 0;
    const struct i2c_interface *iface = i2c_if[bus_index];

    i2c_start(iface);
    if (!i2c_outb(iface, bus_address))
    {
        ret = -2;
        goto end;
    }

    for(i = 0;i < count_write;i++)
    {
        if (!i2c_outb(iface, buf_write[i]))
        {
            ret = -3;
            goto end;
        }
    }

    for(i = 0;i < count_read-1;i++)
        buf_read[i] = i2c_inb(iface, true);

    buf_read[i] = i2c_inb(iface, false);

end:
    i2c_stop(iface);
    return ret;
}

/* returns bus index which can be used as a handle, or <0 on error */
int i2c_add_node(const struct i2c_interface *iface)
{
    int bus_index;

    if (i2c_num_ifs == MAX_I2C_INTERFACES)
        return -1;

    bus_index = i2c_num_ifs++;
    i2c_if[bus_index] = iface;

    iface->scl_dir(true);

    return bus_index;
}
