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
#include "system.h"
#include "generic_i2c.h"
#include "synaptics-rmi.h"

static int rmi_cur_page;
static int rmi_i2c_addr;
static int rmi_i2c_bus;

/* NOTE:
 * RMI over i2c supports some special aliases on page 0x2 but this driver don't
 * use them */

int rmi_init(int i2c_bus_index, int i2c_dev_addr)
{
    rmi_i2c_bus = i2c_bus_index;
    rmi_i2c_addr = i2c_dev_addr;
    rmi_cur_page = 0x4;
    return 0;
}

static int rmi_select_page(unsigned char page)
{
    /* Lazy page select */
    if(page != rmi_cur_page)
    {
        rmi_cur_page = page;
        return i2c_write_data(rmi_i2c_bus, rmi_i2c_addr, RMI_PAGE_SELECT, &page, 1);
    }
    else
        return 0;
}

int rmi_read(int address, int byte_count, unsigned char *buffer)
{
    if(rmi_select_page(address >> 8) < 0)
        return -1;
    return i2c_read_data(rmi_i2c_bus, rmi_i2c_addr, address & 0xff, buffer, byte_count);
}

int rmi_read_single(int address)
{
    unsigned char c;
    int ret = rmi_read(address, 1, &c);
    return ret < 0 ? ret : c;
}

int rmi_write(int address, int byte_count, const unsigned char *buffer)
{
    if(rmi_select_page(address >> 8) < 0)
        return -1;
    return i2c_write_data(rmi_i2c_bus, rmi_i2c_addr, address & 0xff, buffer, byte_count);
}

int rmi_write_single(int address, unsigned char byte)
{
    return rmi_write(address, 1, &byte);
}
