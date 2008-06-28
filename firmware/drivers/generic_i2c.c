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
#include "logf.h"
#include "generic_i2c.h"

int i2c_num_ifs = 0;
struct i2c_interface *i2c_if[5];

static void i2c_start(struct i2c_interface *iface)
{
    iface->sda_hi();
    iface->scl_hi();
    iface->sda_lo();
    iface->delay_hd_sta();
    iface->scl_lo();
}

static void i2c_stop(struct i2c_interface *iface)
{
    iface->sda_lo();
    iface->scl_hi();
    iface->delay_su_sto();
    iface->sda_hi();
}

static void i2c_ack(struct i2c_interface *iface, bool ack)
{
    iface->scl_lo();
    if ( ack )
        iface->sda_lo();
    else
        iface->sda_hi();

    iface->delay_su_dat();
    iface->scl_hi();
    iface->delay_thigh();
    iface->scl_lo();
}

static int i2c_getack(struct i2c_interface *iface)
{
    int ret = 1;

    iface->sda_input();
    iface->delay_su_dat();
    iface->scl_hi();
    
    if (iface->sda())
        ret = 0; /* ack failed */
    
    iface->delay_thigh();
    iface->scl_lo();
    iface->sda_hi();
    iface->sda_output();
    iface->delay_hd_dat();
    return ret;
}

static unsigned char i2c_inb(struct i2c_interface *iface, bool ack)
{
    int i;
    unsigned char byte = 0;
    
    /* clock in each bit, MSB first */
    for ( i=0x80; i; i>>=1 ) {
        iface->sda_input();
        iface->scl_hi();
        iface->delay_thigh();
        if (iface->sda())
            byte |= i;
        iface->scl_lo();
        iface->delay_hd_dat();
        iface->sda_output();
    }
    
    i2c_ack(iface, ack);
    
    return byte;
}

static void i2c_outb(struct i2c_interface *iface, unsigned char byte)
{
   int i;

   /* clock out each bit, MSB first */
   for (i=0x80; i; i>>=1) {
       if (i & byte)
           iface->sda_hi();
       else
           iface->sda_lo();
       iface->delay_su_dat();
       iface->scl_hi();
       iface->delay_thigh();
       iface->scl_lo();
       iface->delay_hd_dat();
   }

   iface->sda_hi();
}

static struct i2c_interface *find_if(int address)
{
    int i;

    for(i = 0;i < i2c_num_ifs;i++) {
        if(i2c_if[i]->address == address)
            return i2c_if[i];
    }
    return NULL;
}

int i2c_write_data(int bus_address, int address,
                   const unsigned char* buf, int count)
{
    int i;
    int ret = 0;
    struct i2c_interface *iface = find_if(bus_address);
    if(!iface)
        return -1;
    
    i2c_start(iface);
    i2c_outb(iface, iface->address & 0xfe);
    if (i2c_getack(iface)) {
        i2c_outb(iface, address);
        if (i2c_getack(iface)) {
            for(i = 0;i < count;i++) {
                i2c_outb(iface, buf[i]);
                if (!i2c_getack(iface)) {
                    ret = -3;
                    break;
                }
            }
        } else {
            ret = -2;
        }
    } else {
        logf("i2c_write_data() - no ack\n");
        ret = -1;
    }
    
    i2c_stop(iface);
    return ret;
}

int i2c_read_data(int bus_address, int address,
                  unsigned char* buf, int count)
{
    int i;
    int ret = 0;
    struct i2c_interface *iface = find_if(bus_address);
    if(!iface)
        return -1;
    
    i2c_start(iface);
    i2c_outb(iface, iface->address & 0xfe);
    if (i2c_getack(iface)) {
        i2c_outb(iface, address);
        if (i2c_getack(iface)) {
            i2c_start(iface);
            i2c_outb(iface, iface->address | 1);
            if (i2c_getack(iface)) {
                for(i = 0;i < count-1;i++)
                    buf[i] = i2c_inb(iface, true);
                
                buf[i] = i2c_inb(iface, false);
            } else {
                ret = -3;
            }
        } else {
            ret = -2;
        }
    } else {
        ret = -1;
    }
    
    i2c_stop(iface);
    return ret;
}

void i2c_add_node(struct i2c_interface *iface)
{
    i2c_if[i2c_num_ifs++] = iface;
}
