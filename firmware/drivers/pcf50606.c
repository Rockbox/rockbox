/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "hwcompat.h"
#include "logf.h"
#include "string.h"

/* TODO: merge all bit-banged I2C into a generic I2C driver */


#define SDA_LO     and_l(~0x00002000, &GPIO1_OUT)
#define SDA_HI      or_l( 0x00002000, &GPIO1_OUT)
#define SDA_INPUT  and_l(~0x00002000, &GPIO1_ENABLE)
#define SDA_OUTPUT  or_l( 0x00002000, &GPIO1_ENABLE)
#define SDA             ( 0x00002000 & GPIO1_READ)

/* SCL is GPIO, 3 */
#define SCL_INPUT  and_l(~0x00001000, &GPIO_ENABLE)
#define SCL_OUTPUT  or_l( 0x00001000, &GPIO_ENABLE)
#define SCL_LO     and_l(~0x00001000, &GPIO_OUT)
#define SCL_HI      SCL_INPUT;while(!SCL){};or_l(0x1000, &GPIO_OUT);SCL_OUTPUT
#define SCL             ( 0x00001000 & GPIO_READ)

/* delay loop to achieve 400kHz at 120MHz CPU frequency */
#define DELAY    do { int _x; for(_x=0;_x<22;_x++);} while(0)


static void pcf50606_i2c_start(void)
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

static void pcf50606_i2c_stop(void)
{
   SDA_LO;
   SCL_HI;
   DELAY;
   SDA_HI;
}


static void pcf50606_i2c_ack(bool ack)
{
    SCL_LO;      /* Set the clock low */
    if(ack)
        SDA_LO;
    else
        SDA_HI;

    SCL_HI;

    DELAY;
    SCL_OUTPUT;
    SCL_LO;
}

static int pcf50606_i2c_getack(void)
{
    int ret = 1;

    SDA_INPUT;   /* And set to input */
    DELAY;
    SCL_HI;

    if (SDA)
        /* ack failed */
        ret = 0;
    
    SCL_OUTPUT;
    SCL_LO;
    SDA_HI;
    SDA_OUTPUT;
    DELAY;

    return ret;
}

static void pcf50606_i2c_outb(unsigned char byte)
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

static unsigned char pcf50606_i2c_inb(bool ack)
{
   int i;
   unsigned char byte = 0;

   /* clock in each bit, MSB first */
   for ( i=0x80; i; i>>=1 ) {
       SDA_INPUT;   /* And set to input */
       SCL_HI;
       DELAY;
       if ( SDA )
           byte |= i;
       SCL_LO;
       DELAY;
       SDA_OUTPUT;
   }

   pcf50606_i2c_ack(ack);
   
   return byte;
}

int pcf50606_i2c_write(int address, const unsigned char* buf, int count)
{
    int i,x=0;

    pcf50606_i2c_start();
    pcf50606_i2c_outb(address & 0xfe);
    if (pcf50606_i2c_getack())
    {
        for (i=0; i<count; i++)
        {
            pcf50606_i2c_outb(buf[i]);
            if (!pcf50606_i2c_getack())
            {
                x=-2;
                break;
            }
        }
    }
    else
    {
        logf("pcf50606_i2c_write() - no ack\n");
        x=-1;
    }
    return x;
}

int pcf50606_read_multiple(int address, unsigned char* buf, int count)
{
    int i=0;
    int ret = 0;
    unsigned char obuf[1];

    obuf[0] = address;

    /* send read command */
    if (pcf50606_i2c_write(0x10, obuf, 1) >= 0)
    {
        pcf50606_i2c_start();
        pcf50606_i2c_outb(0x11);
        if (pcf50606_i2c_getack())
        {
            for(i = 0;i < count-1;i++)
                buf[i] = pcf50606_i2c_inb(true);
            
            buf[i] = pcf50606_i2c_inb(false);
        }
        else
        {
            ret = -1;
        }
    }

    pcf50606_i2c_stop();

    return ret;
}

int pcf50606_read(int address)
{
    int ret;
    unsigned char c;

    ret = pcf50606_read_multiple(address, &c, 1);
    if(ret >= 0)
        return c;
    else
        return ret;
}

int pcf50606_write_multiple(int address, const unsigned char* buf, int count)
{
    unsigned char obuf[1];
    int i;
    int ret = 0;

    obuf[0] = address;
    
    /* send write command */
    if (pcf50606_i2c_write(0x10, obuf, 1) >= 0)
    {
        for (i=0; i<count; i++)
        {
            pcf50606_i2c_outb(buf[i]);
            if (!pcf50606_i2c_getack())
            {
                ret = -2;
                break;
            }
        }
    }
    else
    {
        ret = -1;
    }

    pcf50606_i2c_stop();
    return ret;
}

int pcf50606_write(int address, unsigned char val)
{
    return pcf50606_write_multiple(address, &val, 1);
}


/* These voltages were determined by measuring the output of the PCF50606
   on a running H300, and verified by disassembling the original firmware */
static void set_voltages(void)
{
    static const unsigned char buf[5] =
        {
            0xf4,   /* IOREGC = 3.3V, ON in all states */
            0xef,   /* D1REGC = 2.4V, ON in all states */
            0x18,   /* D2REGC = 3.3V, OFF in all states */
            0xf0,   /* D3REGC = 2.5V, ON in all states */
            0xef,   /* LPREGC1 = 2.4V, ON in all states */
        };
    
    pcf50606_write_multiple(0x23, buf, 5);
}

void pcf50606_init(void)
{
    /* Bit banged I2C */
    or_l(0x00002000, &GPIO1_OUT);
    or_l(0x00001000, &GPIO_OUT);
    or_l(0x00002000, &GPIO1_ENABLE);
    or_l(0x00001000, &GPIO_ENABLE);
    or_l(0x00002000, &GPIO1_FUNCTION);
    or_l(0x00001000, &GPIO_FUNCTION);

    set_voltages();

    /* Backlight PWM = 512Hz 50/50 */
    pcf50606_write(0x35, 0x13);
}
