/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Tomasz Moń
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * DM320 I²C driver
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
#include "kernel.h"
#include "i2c-dm320.h"

#ifdef HAVE_SOFTWARE_I2C
#include "generic_i2c.h"
#endif

#ifndef HAVE_SOFTWARE_I2C

static struct mutex i2c_mtx;

static inline void i2c_begin(void)
{
    mutex_lock(&i2c_mtx);
}

static inline void i2c_end(void)
{
    mutex_unlock(&i2c_mtx);
}

#define I2C_SCS_COND_START  0x0001
#define I2C_SCS_COND_STOP   0x0002
#define I2C_SCS_XMIT        0x0004

#define I2C_TX_ACK          (1 << 8)

static inline bool i2c_getack(void)
{
    return (IO_I2C_RXDATA >> 8) & 1;
}

static inline void i2c_ack(void)
{
    IO_I2C_TXDATA |= I2C_TX_ACK;
}

#define WAIT_FOR_I2C    if(IO_I2C_SCS & 0x4){ \
                            while(IO_I2C_SCS & 0x4) { \
                                asm volatile("nop"); \
                            } \
                        } \

static inline void i2c_start(void)
{
    IO_I2C_SCS |= I2C_SCS_XMIT;
    return;
}

int i2c_write(unsigned short address, const unsigned char *buf, int count)
{
    int i;
    int ret=0;
    i2c_begin();
    IO_I2C_TXDATA = ( (address << 1) & 0xFF ) | (address>0x7F ? 0 : 1 ) | I2C_TX_ACK;
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_START; // write 'start condition'
    i2c_start();
    WAIT_FOR_I2C;
    /* experimental */
    if(address>0x7F){ // check if it is 10-bit instead of 7-bit
        IO_I2C_TXDATA = ( (address >> 7) & 0xFF) | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        IO_I2C_TXDATA = ( (address << 1) & 0xFF) | 1 | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //clear conditions
        IO_I2C_SCS |= I2C_SCS_COND_START; //write 'start condition'
        i2c_start();
        WAIT_FOR_I2C;
    }

    for(i=0; i<count; i++){
        IO_I2C_TXDATA = buf[i] | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        if(!i2c_getack())
            ret = -1;
    }
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_STOP; //write 'stop condition'
    i2c_start();
    WAIT_FOR_I2C;
    i2c_end();
    return ret;
}

int i2c_read(unsigned short address, unsigned char* buf, int count)
{
    int i;
    int ack=0;
    i2c_begin();
    IO_I2C_TXDATA = ( (address << 1) & 0xFF ) | (address>0x7F ? 0 : 1 ) | I2C_TX_ACK;
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_START; // write 'start condition'
    i2c_start();
    WAIT_FOR_I2C;
    /* experimental */
    if(address>0x7F){ // check if it is 10-bit instead of 7-bit
        IO_I2C_TXDATA = ( (address >> 7) & 0xFF ) | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        IO_I2C_TXDATA = ( (address << 1) & 0xFF ) | 1 | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //clear conditions
        IO_I2C_SCS |= I2C_SCS_COND_START; //write 'start condition'
        i2c_start();
        WAIT_FOR_I2C;
    }

    for(i=0; i<count; i++){
        unsigned short temp;
        IO_I2C_TXDATA = 0xFF | ( (count-1)==i ? I2C_TX_ACK : 0);
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        temp = IO_I2C_RXDATA;
        buf[i] = temp & 0xFF;
        ack = (temp & 0x100) >> 8;
    }
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_STOP; //write 'stop condition'
    i2c_start();
    WAIT_FOR_I2C;
    i2c_end();
    return ack;
}

void i2c_init(void)
{
    mutex_init(&i2c_mtx);

#ifdef CREATIVE_ZVx //TODO: mimic OF I2C clock settings; currently this is done by the bootloader
    IO_CLK_MOD2 &= ~CLK_MOD2_I2C; // turn I²C clock off (just to be sure)
    IO_CLK_LPCTL1 &= ~1; // set Powerdown mode to off
    IO_CLK_SEL0 &= ~0x800; // set I²C clock to PLLA
    IO_CLK_DIV4 &= ~0x1F; // I²C clock division = 1
    IO_CLK_MOD2 |= CLK_MOD2_I2C; // enable I²C clock
#endif
    IO_I2C_SCS &= ~0x8; //set clock to 100 kHz
    IO_INTC_EINT2 &= ~INTR_EINT2_I2C; // disable I²C interrupt
}

#else /* Software I2C implementation */

#ifdef SANSA_CONNECT
    /* SDA - GIO35 */
    #define SDA_SET_REG IO_GIO_BITSET2
    #define SDA_CLR_REG IO_GIO_BITCLR2
    #define SOFTI2C_SDA (1 << 3)
    /* SCL - GIO36 */
    #define SCL_SET_REG IO_GIO_BITSET2
    #define SCL_CLR_REG IO_GIO_BITCLR2
    #define SOFTI2C_SCL (1 << 4)
#else
    #error Configure SDA and SCL lines
#endif

static int dm320_i2c_bus;

static void dm320_scl_dir(bool out)
{
    if (out)
    {
        IO_GIO_DIR2 &= ~(SOFTI2C_SCL);
    }
    else
    {
        IO_GIO_DIR2 |= SOFTI2C_SCL;
    }
}

static void dm320_sda_dir(bool out)
{
    if (out)
    {
        IO_GIO_DIR2 &= ~(SOFTI2C_SDA);
    }
    else
    {
        IO_GIO_DIR2 |= SOFTI2C_SDA;
    }
}

static void dm320_scl_out(bool high)
{
    if (high)
    {
        SCL_SET_REG = SOFTI2C_SCL;
    }
    else
    {
        SCL_CLR_REG = SOFTI2C_SCL;
    }
}

static void dm320_sda_out(bool high)
{
    if (high)
    {
        SDA_SET_REG = SOFTI2C_SDA;
    }
    else
    {
        SDA_CLR_REG = SOFTI2C_SDA;
    }
}

static bool dm320_scl_in(void)
{
    return (SCL_SET_REG & SOFTI2C_SCL);
}

static bool dm320_sda_in(void)
{
    return (SDA_SET_REG & SOFTI2C_SDA);
}

/* simple delay */
static void dm320_i2c_delay(int delay)
{
    udelay(delay);
}

/* interface towards the generic i2c driver */
static const struct i2c_interface dm320_i2c_interface = {
    .scl_dir = dm320_scl_dir,
    .sda_dir = dm320_sda_dir,
    .scl_out = dm320_scl_out,
    .sda_out = dm320_sda_out,
    .scl_in = dm320_scl_in,
    .sda_in = dm320_sda_in,
    .delay = dm320_i2c_delay,

    /* uncalibrated */
    .delay_hd_sta = 2,
    .delay_hd_dat = 2,
    .delay_su_dat = 2,
    .delay_su_sto = 2,
    .delay_su_sta = 2,
    .delay_thigh = 2
};

void i2c_init(void)
{
#ifdef SANSA_CONNECT
    IO_GIO_FSEL3 &= 0xFF0F; /* GIO35, GIO36 as normal GIO */
    IO_GIO_INV2 &= ~(SOFTI2C_SDA | SOFTI2C_SCL); /* not inverted */
#endif

    /* generic_i2c takes care of setting direction */
    dm320_i2c_bus = i2c_add_node(&dm320_i2c_interface);
}

int i2c_write(unsigned short address, const unsigned char* buf, int count)
{
    return i2c_write_data(dm320_i2c_bus, address, -1, buf, count);
}

int i2c_read(unsigned short address, unsigned char* buf, int count)
{
    return i2c_read_data(dm320_i2c_bus, address, -1, buf, count);
}

int i2c_read_bytes(unsigned short address, unsigned short reg,
                   unsigned char* buf, int count)
{
    return i2c_read_data(dm320_i2c_bus, address, reg, buf, count);
}

int i2c_write_read_bytes(unsigned short address,
                         const unsigned char* buf_write, int count_write,
                         unsigned char* buf_read, int count_read)
{
    return i2c_write_read_data(dm320_i2c_bus, address, buf_write, count_write,
                               buf_read, count_read);
}

#endif
