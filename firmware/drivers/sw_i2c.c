/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Miika Pekkarinen
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
#include "logf.h"
#include "inttypes.h"

#include "sw_i2c.h"

/**
 * I2C-functions are copied and ported from fmradio.c.
 * later fixed, adapted and moved to a seperate file so they can be re-used 
 * by the rtc-ds1339c and later by the m:robe-100 code by Robert Kukla
 */

/* cute little functions, atomic read-modify-write */

#ifdef MROBE_100

/* SCL is GPIOC, 4 */
#define SCL       (GPIOC_INPUT_VAL &  0x00000010)
#define SCL_OUT_LO GPIOC_OUTPUT_VAL&=~0x00000010
#define SCL_LO     GPIOC_OUTPUT_EN |= 0x00000010
#define SCL_HI     GPIOC_OUTPUT_EN &=~0x00000010

/* SDA is GPIOC, 5 */
#define SDA       (GPIOC_INPUT_VAL &  0x00000020)
#define SDA_OUT_LO GPIOC_OUTPUT_VAL&=~0x00000020
#define SDA_LO     GPIOC_OUTPUT_EN |= 0x00000020
#define SDA_HI     GPIOC_OUTPUT_EN &=~0x00000020

#define DELAY  do { volatile int _x; for(_x=0;_x<22;_x++);} while(0)

#else

/* SCL is GPIO, 12 */
#define SCL             ( 0x00001000 & GPIO_READ)
#define SCL_OUT_LO and_l(~0x00001000, &GPIO_OUT)
#define SCL_LO      or_l( 0x00001000, &GPIO_ENABLE)
#define SCL_HI     and_l(~0x00001000, &GPIO_ENABLE)

/* SDA is GPIO1, 13 */
#define SDA             ( 0x00002000 & GPIO1_READ)
#define SDA_OUT_LO and_l(~0x00002000, &GPIO1_OUT)
#define SDA_LO      or_l( 0x00002000, &GPIO1_ENABLE)
#define SDA_HI     and_l(~0x00002000, &GPIO1_ENABLE)

/* delay loop to achieve 400kHz at 120MHz CPU frequency */
#define DELAY   \
    ({                                \
        int _x_;                      \
        asm volatile (                \
            "move.l #21, %[_x_] \r\n" \
        "1:                     \r\n" \
            "subq.l #1, %[_x_]  \r\n" \
            "bhi.b  1b          \r\n" \
            : [_x_]"=&d"(_x_)         \
        );                            \
    })

#endif

void sw_i2c_init(void)
{
#ifndef MROBE_100
    or_l(0x00001000, &GPIO_FUNCTION);
    or_l(0x00002000, &GPIO1_FUNCTION);
#endif
  
    SDA_HI;
    SCL_HI;
    SDA_OUT_LO;
    SCL_OUT_LO;
}

/*   in: C=? D=?
 *  out: C=L D=L
 */
static void sw_i2c_start(void)
{
    SCL_LO;
    DELAY;
    SDA_HI;
    DELAY;
    SCL_HI;
    DELAY;
    SDA_LO;
    DELAY;
    SCL_LO;
}

/*   in: C=L D=?
 *  out: C=H D=H
 */
static void sw_i2c_stop(void)
{
    SDA_LO;
    DELAY;
    SCL_HI;
    DELAY;
    SDA_HI;
}

/*   in: C=L D=H
 *  out: C=L D=L
 */
static void sw_i2c_ack(void)
{
    SDA_LO;
    DELAY;
    
    SCL_HI;
    DELAY;
    SCL_LO;
}

/*   in: C=L D=H
 *  out: C=L D=H
 */
static void sw_i2c_nack(void)
{
    SDA_HI;          /* redundant */
    DELAY;
    
    SCL_HI;
    DELAY;
    SCL_LO;
}

/*   in: C=L D=?
 *  out: C=L D=H
 */
static bool sw_i2c_getack(void)
{
    bool ret = true;
/*    int count = 10; */

    SDA_HI;   /* sets to input */
    DELAY;
    SCL_HI;
    DELAY;

/*    while (SDA && count--) */
/*        DELAY; */
    
    if (SDA)
        /* ack failed */
        ret = false;
    
    SCL_LO;

    return ret;
}

/*   in: C=L D=?
 *  out: C=L D=?
 */
static void sw_i2c_outb(unsigned char byte)
{
    int i;

    /* clock out each bit, MSB first */
    for ( i=0x80; i; i>>=1 )
    {
        if ( i & byte )
            SDA_HI;
        else
            SDA_LO;
        DELAY;

        SCL_HI;
        DELAY;
        SCL_LO;
    }
}

/*   in: C=L D=?
 *  out: C=L D=H
 */
static unsigned char sw_i2c_inb(void)
{
    int i;
    unsigned char byte = 0;

    SDA_HI;   /* sets to input */
    
    /* clock in each bit, MSB first */
    for ( i=0x80; i; i>>=1 ) 
    {
        DELAY;
        do {
          SCL_HI;
          DELAY;
        }
        while(SCL==0);    /* wait for any SCL clock stretching */
        if ( SDA )
            byte |= i;
        SCL_LO;
    }

    return byte;
}

int sw_i2c_write(unsigned char chip, unsigned char location, unsigned char* buf, int count)
{
    int i;

    sw_i2c_start();
    sw_i2c_outb((chip & 0xfe) | SW_I2C_WRITE);
    if (!sw_i2c_getack())
    {
        sw_i2c_stop();
        return -1;
    }

#ifdef MROBE_100 /* does not use register addressing */
    (void) location;
#else    
    sw_i2c_outb(location);
    if (!sw_i2c_getack())
    {
        sw_i2c_stop();
        return -2;
    }
#endif  

    for (i=0; i<count; i++)
    {
        sw_i2c_outb(buf[i]);
        if (!sw_i2c_getack())
        {
            sw_i2c_stop();
            return -3;
        }
    }

    sw_i2c_stop();
    
    return 0;
}

int sw_i2c_read(unsigned char chip, unsigned char location, unsigned char* buf, int count)
{
    int i;

#ifdef MROBE_100 /* does not use register addressing */
    (void) location;
#else    
    sw_i2c_start();
    sw_i2c_outb((chip & 0xfe) | SW_I2C_WRITE);
    if (!sw_i2c_getack())
    {
        sw_i2c_stop();
        return -1;
    }

    sw_i2c_outb(location);
    if (!sw_i2c_getack())
    {
        sw_i2c_stop();
        return -2;
    }
#endif  

    sw_i2c_start();
    sw_i2c_outb((chip & 0xfe) | SW_I2C_READ);
    if (!sw_i2c_getack())
    {
        sw_i2c_stop();
        return -3;
    }
    
    for (i=0; i<count-1; i++)
    {
      buf[i] = sw_i2c_inb();
      sw_i2c_ack();
    }

    /* 1byte min */
    buf[i] = sw_i2c_inb();
    sw_i2c_nack();
        
    sw_i2c_stop();
    
    return 0;
}
