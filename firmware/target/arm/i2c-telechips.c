/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include "system.h"
#include "i2c.h"
#include "i2c-target.h"

/* arbitrary delay loop */
#define DELAY  do { int _x; for(_x=0;_x<20;_x++);} while (0)

static struct mutex i2c_mtx;

void i2c_init(void)
{
    /* nothing to do */
}

void i2c_start(void)
{
    SDA_OUTPUT;
    
    SCL_HI;
    SDA_HI;
    DELAY;
    
    SDA_LO;
    DELAY;
    SCL_LO;
    DELAY;
}

void i2c_stop(void)
{
    SDA_OUTPUT;

    SDA_LO;
    DELAY;
    
    SCL_HI;
    DELAY;
    SDA_HI;
    DELAY;
}

void i2c_outb(unsigned char byte)
{
    int bit;

    SDA_OUTPUT;
    
    for (bit = 0; bit < 8; bit++)
    {
        if ((byte<<bit) & 0x80)
          SDA_HI;
        else
          SDA_LO;
          
        DELAY;

        SCL_HI;
        DELAY;
        SCL_LO;
        DELAY;
    }
}

unsigned char i2c_inb(int ack)
{
    int i;
    unsigned char byte = 0;
    
    SDA_INPUT;
    
    /* clock in each bit, MSB first */
    for ( i=0x80; i; i>>=1 )
    {
        SCL_HI;
        DELAY;
        
        if ( SDA ) byte |= i;
        
        SCL_LO;
        DELAY;
    }
    
    i2c_ack(ack);
    return byte;
}

void i2c_ack(int bit)
{
    SDA_OUTPUT;

    if (bit)
      SDA_HI;
    else
      SDA_LO;

    SCL_HI;
    DELAY;
    SCL_LO;
    DELAY;
}

int i2c_getack(void)
{
    bool ack_bit;
    
    SDA_INPUT;

    SCL_HI;
    DELAY;
    
    ack_bit = SDA;
    DELAY;

    SCL_LO;
    DELAY;
    
    return ack_bit;
}

/* device = 8 bit slave address */
int i2c_write(int device, unsigned char* buf, int count )
{
    int i = 0;
    mutex_lock(&i2c_mtx);
    
    i2c_start();
    i2c_outb(device & 0xfe);
    
    while (!i2c_getack() && i < count)
    {
        i2c_outb(buf[i++]);
    }
    
    i2c_stop();
    mutex_unlock(&i2c_mtx);
    return 0;
}


/* device = 8 bit slave address */
int i2c_readmem(int device, int address, unsigned char* buf, int count )
{
    int i = 0;
    mutex_lock(&i2c_mtx);
    
    i2c_start();
    i2c_outb(device & 0xfe);
    if (i2c_getack()) goto exit;
    
    i2c_outb(address);
    if (i2c_getack()) goto exit;

    i2c_start();
    i2c_outb(device | 1);
    if (i2c_getack()) goto exit;
    
    while (i < count)
    {
        buf[i] = i2c_inb(i == (count-1));
        i++;
    }

exit:
    i2c_stop();
    mutex_unlock(&i2c_mtx);
    return 0;
}
