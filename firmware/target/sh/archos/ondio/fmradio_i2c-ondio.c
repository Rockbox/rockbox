/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Physical interface of the Philips TEA5767 in Archos Ondio
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include "logf.h"
#include "system.h"

#if (CONFIG_TUNER & TEA5767)

/* cute little functions, atomic read-modify-write */
/* SDA is PB4 */
#define SDA_LO     and_b(~0x10, &PBDRL)
#define SDA_HI     or_b(0x10, &PBDRL)
#define SDA_INPUT  and_b(~0x10, &PBIORL)
#define SDA_OUTPUT or_b(0x10, &PBIORL)
#define SDA        (PBDR & 0x0010)

/* SCL is PB1 */
#define SCL_INPUT  and_b(~0x02, &PBIORL)
#define SCL_OUTPUT or_b(0x02, &PBIORL)
#define SCL_LO     and_b(~0x02, &PBDRL)
#define SCL_HI     or_b(0x02, &PBDRL)
#define SCL        (PBDR & 0x0002)

/* arbitrary delay loop */
#define DELAY   do { int _x; for(_x=0;_x<20;_x++);} while (0)

static void fmradio_i2c_start(void)
{
    SDA_OUTPUT;
    SDA_HI;
    SCL_HI;
    SDA_LO;
    DELAY;
    SCL_LO;
}

static void fmradio_i2c_stop(void)
{
   SDA_LO;
   SCL_HI;
   DELAY;
   SDA_HI;
}


static void fmradio_i2c_ack(bool nack)
{
    /* Here's the deal. The slave is slow, and sometimes needs to wait
       before it can receive the acknowledge. Therefore it forces the clock
       low until it is ready. We need to poll the clock line until it goes
       high before we release the ack. */
    
    SCL_LO;      /* Set the clock low */

    if (nack)
        SDA_HI;
    else
        SDA_LO;
    
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the slave to release it */
        sleep(0);

    DELAY;
    SCL_OUTPUT;
    SCL_LO;
}

static int fmradio_i2c_getack(void)
{
    int ret = 1;

    /* Here's the deal. The slave is slow, and sometimes needs to wait
       before it can send the acknowledge. Therefore it forces the clock
       low until it is ready. We need to poll the clock line until it goes
       high before we read the ack. */

    SDA_INPUT;   /* And set to input */
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the slave to release it */
        sleep(0);
    
    if (SDA)
        /* ack failed */
        ret = 0;
    
    SCL_OUTPUT;
    SCL_LO;
    SDA_HI;
    SDA_OUTPUT;
    return ret;
}

static void fmradio_i2c_outb(unsigned char byte)
{
   int i;

   /* clock out each bit, MSB first */
   for ( i=0x80; i; i>>=1 ) {
      if ( i & byte )
      {
         SDA_HI;
      }
      else
      {
         SDA_LO;
      }
      SCL_HI;
      SCL_LO;
   }

   SDA_HI;
}

static unsigned char fmradio_i2c_inb(void)
{
   int i;
   unsigned char byte = 0;

   /* clock in each bit, MSB first */
   for ( i=0x80; i; i>>=1 ) {
       SDA_INPUT;   /* And set to input */
       SCL_HI;
       if ( SDA )
           byte |= i;
       SCL_LO;
       SDA_OUTPUT;
   }
   
   return byte;
}

int fmradio_i2c_write(int address, const unsigned char* buf, int count)
{
    int i,x=0;

    fmradio_i2c_start();
    fmradio_i2c_outb(address & 0xfe);
    if (fmradio_i2c_getack())
    {
        for (i=0; i<count; i++)
        {
            fmradio_i2c_outb(buf[i]);
            if (!fmradio_i2c_getack())
            {
                x=-2;
                break;
            }
        }
    }
    else
    {
        logf("fmradio_i2c_write() - no ack\n");
        x=-1;
    }
    fmradio_i2c_stop();
    return x;
}

int fmradio_i2c_read(int address, unsigned char* buf, int count)
{
    int i,x=0;
    
    fmradio_i2c_start();
    fmradio_i2c_outb(address | 1);
    if (fmradio_i2c_getack()) {
        for (i=count; i>0; i--)
        {
            *buf++ = fmradio_i2c_inb();
            fmradio_i2c_ack(i == 1);
        }
    }
    else
        x=-1;
    fmradio_i2c_stop();
    return x;
}

#endif
