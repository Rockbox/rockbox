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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "logf.h"
#include "inttypes.h"

#include "sw_i2c.h"

#include "pcf50606.h"

void sw_i2c_init(void) 
{
       /* no extra init required */ 
}       

int sw_i2c_write(unsigned char chip, unsigned char location, unsigned char* buf, int count)
{
    int i;

    pcf50606_i2c_start();
    pcf50606_i2c_outb((chip & 0xfe) | SW_I2C_WRITE);
    if (!pcf50606_i2c_getack())
    {
        pcf50606_i2c_stop();
        return -1;
    }
    
    pcf50606_i2c_outb(location);
    if (!pcf50606_i2c_getack())
    {
        pcf50606_i2c_stop();
        return -2;
    }
    
    for (i=0; i<count; i++)
    {
        pcf50606_i2c_outb(buf[i]);
        if (!pcf50606_i2c_getack())
        {
            pcf50606_i2c_stop();
            return -3;
        }
    }

    pcf50606_i2c_stop();
    
    return 0;
}

int sw_i2c_read(unsigned char chip, unsigned char location, unsigned char* buf, int count)
{
    int i;
  
    pcf50606_i2c_start();
    pcf50606_i2c_outb((chip & 0xfe) | SW_I2C_WRITE);
    if (!pcf50606_i2c_getack())
    {
        pcf50606_i2c_stop();
        return -1;
    }

    pcf50606_i2c_outb(location);
    if (!pcf50606_i2c_getack())
    {
        pcf50606_i2c_stop();
        return -2;
    }
    
    pcf50606_i2c_start();
    pcf50606_i2c_outb((chip & 0xfe) | SW_I2C_READ);
    if (!pcf50606_i2c_getack())
    {
        pcf50606_i2c_stop();
        return -3;
    }
    
    for (i=0; i<count-1; i++)
    {
      buf[i] = pcf50606_i2c_inb(true);
    }

    /* 1byte min */
    buf[i] = pcf50606_i2c_inb(false);
    
        
    pcf50606_i2c_stop();
    
    return 0;
}
