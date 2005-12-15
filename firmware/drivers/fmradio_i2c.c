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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "logf.h"
#include "system.h"
#include "string.h"

#if (CONFIG_TUNER & TEA5767)
#if (CONFIG_I2C == I2C_COLDFIRE)
/* cute little functions, atomic read-modify-write */
/* SDA is GPIO1,23 */

#ifdef IRIVER_H300_SERIES

/* SDA is GPIO57 */
#define SDA_LO     and_l(~0x02000000, &GPIO1_OUT)
#define SDA_HI      or_l( 0x02000000, &GPIO1_OUT)
#define SDA_INPUT  and_l(~0x02000000, &GPIO1_ENABLE)
#define SDA_OUTPUT  or_l( 0x02000000, &GPIO1_ENABLE)
#define SDA             ( 0x02000000 & GPIO1_READ)

/* SCL is GPIO56 */
#define SCL_INPUT  and_l(~0x01000000, &GPIO1_ENABLE)
#define SCL_OUTPUT  or_l( 0x01000000, &GPIO1_ENABLE)
#define SCL_LO     and_l(~0x01000000, &GPIO1_OUT)
#define SCL_HI      or_l( 0x01000000, &GPIO1_OUT)
#define SCL             ( 0x01000000 & GPIO1_READ)

#else

/* SDA is GPIO55 */
#define SDA_LO     and_l(~0x00800000, &GPIO1_OUT)
#define SDA_HI      or_l( 0x00800000, &GPIO1_OUT)
#define SDA_INPUT  and_l(~0x00800000, &GPIO1_ENABLE)
#define SDA_OUTPUT  or_l( 0x00800000, &GPIO1_ENABLE)
#define SDA             ( 0x00800000 & GPIO1_READ)

/* SCL is GPIO3 */
#define SCL_INPUT  and_l(~0x00000008, &GPIO_ENABLE)
#define SCL_OUTPUT  or_l( 0x00000008, &GPIO_ENABLE)
#define SCL_LO     and_l(~0x00000008, &GPIO_OUT)
#define SCL_HI      or_l( 0x00000008, &GPIO_OUT)
#define SCL             ( 0x00000008 & GPIO_READ)
#endif

/* delay loop to achieve 400kHz at 120MHz CPU frequency */
#define DELAY    do { int _x; for(_x=0;_x<22;_x++);} while(0)


static void fmradio_i2c_start(void)
{
    SDA_OUTPUT;
    SCL_OUTPUT;
    SDA_HI;
    SCL_HI;
    DELAY;
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


static void fmradio_i2c_ack(void)
{
    /* Here's the deal. The slave is slow, and sometimes needs to wait
       before it can receive the acknowledge. Therefore it forces the clock
       low until it is ready. We need to poll the clock line until it goes
       high before we release the ack.

       In their infinite wisdom, iriver didn't pull up the SCL line, so
       we have to drive the SCL high repeatedly to simulate a pullup. */
    
    SCL_LO;      /* Set the clock low */
    SDA_LO;
    
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the slave to release it */
    {
        SCL_HI;
        SCL_OUTPUT;  /* Set the clock to output */
        SCL_INPUT;   /* Set the clock to input */
        DELAY;
    }

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
       high before we read the ack.

       In their infinite wisdom, iriver didn't pull up the SCL line, so
       we have to drive the SCL high repeatedly to simulate a pullup. */

    SDA_INPUT;   /* And set to input */
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the slave to release it */
    {
        SCL_HI;
        SCL_OUTPUT;  /* Set the clock to output */
        SCL_INPUT;   /* Set the clock to input */
        DELAY;
    }

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
      DELAY;
      SCL_HI;
      DELAY;
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
       DELAY;
       DELAY;
       if ( SDA )
           byte |= i;
       SCL_HI;
       DELAY;
       SCL_LO;
       DELAY;
       SDA_OUTPUT;
   }

   fmradio_i2c_ack();
   
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

    if (fmradio_i2c_getack())
    {
        for (i=0; i<count; i++)
        {
            buf[i] = fmradio_i2c_inb();
        }
    }
    else
        x=-1;
    fmradio_i2c_stop();
    return x;
}
#else
/* cute little functions, atomic read-modify-write */
/* SDA is PB4 */
#define SDA_LO     and_b(~0x10, &PBDRL)
#define SDA_HI     or_b(0x10, &PBDRL)
#define SDA_INPUT  and_b(~0x10, &PBIORL)
#define SDA_OUTPUT or_b(0x10, &PBIORL)
#define SDA     (PBDR & 0x0010)

/* SCL is PB1 */
#define SCL_INPUT  and_b(~0x02, &PBIORL)
#define SCL_OUTPUT or_b(0x02, &PBIORL)
#define SCL_LO     and_b(~0x02, &PBDRL)
#define SCL_HI     or_b(0x02, &PBDRL)
#define SCL     (PBDR & 0x0002)

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


static void fmradio_i2c_ack(int bit)
{
    /* Here's the deal. The slave is slow, and sometimes needs to wait
       before it can receive the acknowledge. Therefore it forces the clock
       low until it is ready. We need to poll the clock line until it goes
       high before we release the ack. */
    
    SCL_LO;      /* Set the clock low */
    if ( bit )
    {
        SDA_HI;
    }
    else
    {
        SDA_LO;
    }
    
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the slave to release it */
        sleep_thread();
    wake_up_thread();

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
        sleep_thread();
    wake_up_thread();
    
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

static unsigned char fmradio_i2c_inb(int ack)
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
   
   fmradio_i2c_ack(ack);
   
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
        for (i=0; i<count; i++) {
            buf[i] = fmradio_i2c_inb(0);
        }
    }
    else
        x=-1;
    fmradio_i2c_stop();
    return x;
}

#endif
#endif
