/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
 *
 * based on linux/arch/mips/jz4740/i2c.c
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

/*
 * Jz4740 I2C routines.
 * 
 * Copyright (C) 2005,2006 Ingenic Semiconductor Inc.
 * Author: <lhhuang@ingenic.cn>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "system.h"
#include "jz4740.h"
#include "logf.h"


/* I2C protocol */
#define I2C_READ    1
#define I2C_WRITE    0

#define TIMEOUT     1000

/*
 * I2C bus protocol basic routines
 */
static int i2c_put_data(unsigned char data)
{
    unsigned int timeout = TIMEOUT*10;

    __i2c_write(data);
    __i2c_set_drf();
    while (__i2c_check_drf() != 0);
    while (!__i2c_transmit_ended());
    while (!__i2c_received_ack() && timeout)
        timeout--;

    if (timeout)
        return 0;
    else
        return -1;
}

static int i2c_put_data_nack(unsigned char data)
{
    unsigned int timeout = TIMEOUT*10;

    __i2c_write(data);
    __i2c_set_drf();
    while (__i2c_check_drf() != 0);
    while (!__i2c_transmit_ended());
    while (timeout--);
    
    return 0;
}

static int i2c_get_data(unsigned char *data, int ack)
{
    int timeout = TIMEOUT*10;

    if (!ack)
        __i2c_send_nack();
    else
        __i2c_send_ack();

    while (__i2c_check_drf() == 0 && timeout)
        timeout--;

    if (timeout)
    {
        if (!ack)
            __i2c_send_stop();
        *data = __i2c_read();
        __i2c_clear_drf();
        return 0;
    }
    else
        return -1;
}

/*
 * I2C interface
 */
void i2c_open(void)
{
    /* TODO */
    //__i2c_set_clk(jz_clocks.extalclk, 10000); /* default 10 KHz */
    __i2c_enable();
}

void i2c_close(void)
{
    udelay(300); /* wait for STOP goes over. */
    __i2c_disable();
}

void i2c_setclk(unsigned int i2cclk)
{
    /* TODO */
    //__i2c_set_clk(jz_clocks.extalclk, i2cclk);
}

int i2c_read(int device, unsigned char *buf, int count)
{
    int cnt = count;
    int timeout = 5;
    
    device &= 0xFF;

L_try_again:
    if (timeout < 0)
        goto L_timeout;

    __i2c_send_nack();    /* Master does not send ACK, slave sends it */
    
    __i2c_send_start();
    if (i2c_put_data( (device << 1) | I2C_READ ) < 0)
        goto device_err;
    
    __i2c_send_ack();    /* Master sends ACK for continue reading */
    
    while (cnt)
    {
        if (cnt == 1)
        {
            if (i2c_get_data(buf, 0) < 0)
                break;
        }
        else
        {
            if (i2c_get_data(buf, 1) < 0)
                break;
        }
        cnt--;
        buf++;
    }

    __i2c_send_stop();
    return count - cnt;
    
device_err:
    timeout--;
    __i2c_send_stop();
    goto L_try_again;

L_timeout:
    __i2c_send_stop();
    logf("Read I2C device 0x%2x failed.", device);
    return -1;
}

int i2c_write(int device, unsigned char *buf, int count)
{
    int cnt = count;
    int timeout = 5;
    
    device &= 0xFF;

    __i2c_send_nack();    /* Master does not send ACK, slave sends it */

W_try_again:
    if (timeout < 0)
        goto W_timeout;

    cnt = count;

    __i2c_send_start();
    if (i2c_put_data( (device << 1) | I2C_WRITE ) < 0)
        goto device_err;
    
#if 0 //CONFIG_JZ_TPANEL_ATA2508
    if (address == 0xff)
    {
        while (cnt)
        {
            if (i2c_put_data_nack(*buf) < 0)
                break;
            cnt--;
            buf++;
        }
    }
    else
#endif
    {
        while (cnt)
        {
            if (i2c_put_data(*buf) < 0)
                break;
            cnt--;
            buf++;
        }
    }

    __i2c_send_stop();
    return count - cnt;
    
device_err:
    timeout--;
    __i2c_send_stop();
    goto W_try_again;

W_timeout:
    logf("Write I2C device 0x%2x failed.", device);
    __i2c_send_stop();
    return -1;
}

void i2c_init(void)
{
}
