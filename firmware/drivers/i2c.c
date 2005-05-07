/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "lcd.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "debug.h"
#include "system.h"

/* cute little functions, atomic read-modify-write */
#if CONFIG_I2C == I2C_GMINI

/* This is done like this in the Archos' firmware.
 * However, the TCC370 has an integrated I2C
 * controller (bound to the same lines). It should be
 * possible to use it and thus save some space in flash.
 */
#define SDA_LO     (P3 &= ~0x20)
#define SDA_HI     (P3 |= 0x20)
#define SDA_INPUT  (P3CONH &= ~0x0C)
#define SDA_OUTPUT (P3CONH |= 0x04)
#define SDA        (P3 & 0x20)

#define SCL_LO     (P3 &= ~0x10)
#define SCL_HI     (P3 |= 0x10)
#define SCL_INPUT  (P3CONH &= ~0x03)
#define SCL_OUTPUT (P3CONH |= 0x01)
#define SCL        (P3 & 0x10)

#else /* non Gmini below */

/* SDA is PB7 */
#define SDA_LO     and_b(~0x80, &PBDRL)
#define SDA_HI     or_b(0x80, &PBDRL)
#define SDA_INPUT  and_b(~0x80, &PBIORL)
#define SDA_OUTPUT or_b(0x80, &PBIORL)
#define SDA        (PBDRL & 0x80)

#if CONFIG_I2C == I2C_ONDIO
/* Ondio pinout, SCL moved to PB6 */
#define SCL_INPUT  and_b(~0x40, &PBIORL)
#define SCL_OUTPUT or_b(0x40, &PBIORL)
#define SCL_LO     and_b(~0x40, &PBDRL)
#define SCL_HI     or_b(0x40, &PBDRL)
#define SCL        (PBDRL & 0x40)
#else 
/* "classic" pinout, SCL is PB13 */
#define SCL_INPUT  and_b(~0x20, &PBIORH)
#define SCL_OUTPUT or_b(0x20, &PBIORH)
#define SCL_LO     and_b(~0x20, &PBDRH)
#define SCL_HI     or_b(0x20, &PBDRH)
#define SCL        (PBDRH & 0x20)
#endif
#endif /* ! I2C_GMINI */

/* arbitrary delay loop */
#define DELAY   do { int _x; for(_x=0;_x<20;_x++);} while (0)

static struct mutex i2c_mtx;

void i2c_begin(void)
{
    mutex_lock(&i2c_mtx);
}

void i2c_end(void)
{
    mutex_unlock(&i2c_mtx);
}

void i2c_start(void)
{
    SDA_OUTPUT;
    SDA_HI;
    SCL_HI;
    SDA_LO;
    DELAY;
    SCL_LO;
}

void i2c_stop(void)
{
   SDA_LO;
   SCL_HI;
   DELAY;
   SDA_HI;
}

void i2c_init(void)
{
   int i;

#if CONFIG_I2C == I2C_GMINI
   SCL_INPUT;
   SDA_INPUT;   
#elif CONFIG_I2C == I2C_ONDIO
   /* make PB6 & PB7 general I/O */
   PBCR2 &= ~0xf000;
#else /* not Gmini, not Ondio */
   /* make PB7 & PB13 general I/O */
   PBCR1 &= ~0x0c00; /* PB13 */
   PBCR2 &= ~0xc000; /* PB7 */
#endif

   SCL_OUTPUT;
   SDA_OUTPUT;
   SDA_HI;
   SCL_LO;
   for (i=0;i<3;i++)
      i2c_stop();
}

void i2c_ack(int bit)
{
    /* Here's the deal. The MAS is slow, and sometimes needs to wait
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
    while(!SCL)  /* and wait for the MAS to release it */
        sleep_thread();
    wake_up_thread();

    DELAY;
    SCL_OUTPUT;
    SCL_LO;
}

int i2c_getack(void)
{
    int ret = 1;

    /* Here's the deal. The MAS is slow, and sometimes needs to wait
       before it can send the acknowledge. Therefore it forces the clock
       low until it is ready. We need to poll the clock line until it goes
       high before we read the ack. */

#ifdef HAVE_I2C_LOW_FIRST
    SDA_LO;      /* First, discharge the data line */
#endif
    SDA_INPUT;   /* And set to input */
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the MAS to release it */
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

void i2c_outb(unsigned char byte)
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

unsigned char i2c_inb(int ack)
{
   int i;
   unsigned char byte = 0;

   /* clock in each bit, MSB first */
   for ( i=0x80; i; i>>=1 ) {
#ifdef HAVE_I2C_LOW_FIRST
       /* Tricky business. Here we discharge the data line by driving it low
          and then set it to input to see if it stays low or goes high */
       SDA_LO;      /* First, discharge the data line */
#endif
       SDA_INPUT;   /* And set to input */
       SCL_HI;
       if ( SDA )
           byte |= i;
       SCL_LO;
       SDA_OUTPUT;
   }
   
   i2c_ack(ack);
   
   return byte;
}

int i2c_write(int address, const unsigned char* buf, int count )
{
    int i,x=0;

    i2c_start();
    i2c_outb(address & 0xfe);
    if (i2c_getack())
    {
        for (i=0; i<count; i++)
        {
            i2c_outb(buf[i]);
            if (!i2c_getack())
            {
                x=-2;
                break;
            }
        }
    }
    else
    {
        debugf("i2c_write() - no ack\n");
        x=-1;
    }
    i2c_stop();
    return x;
}

int i2c_read(int address, unsigned char* buf, int count )
{
    int i,x=0;
    
    i2c_start();
    i2c_outb(address | 1);
    if (i2c_getack()) {
        for (i=0; i<count; i++) {
            buf[i] = i2c_inb(0);
        }
    }
    else
        x=-1;
    i2c_stop();
    return x;
}
