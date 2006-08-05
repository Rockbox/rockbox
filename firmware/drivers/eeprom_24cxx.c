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
#include "lcd.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "debug.h"
#include "logf.h"
#include "sprintf.h"
#include "string.h"

#include "eeprom_24cxx.h"

/**
 * I2C-functions are copied and ported from fmradio.c.
 */

#define SW_I2C_WRITE 0
#define SW_I2C_READ  1

/* cute little functions, atomic read-modify-write */

/* SCL is GPIO, 12 */
#define SCL_LO     and_l(~0x00001000, &GPIO_OUT)    // and_b(~0x10, &PBDRL)
#define SCL_HI      or_l( 0x00001000, &GPIO_OUT)    //  or_b( 0x10, &PBDRL)
#define SCL_INPUT  and_l(~0x00001000, &GPIO_ENABLE) // and_b(~0x10, &PBIORL)
#define SCL_OUTPUT  or_l( 0x00001000, &GPIO_ENABLE) //  or_b( 0x10, &PBIORL)
#define SCL             ( 0x00001000 & GPIO_READ)   //      (PBDR & 0x0010)

/* SDA is GPIO1, 13 */
#define SDA_LO     and_l(~0x00002000, &GPIO1_OUT)     // and_b(~0x02, &PBDRL)
#define SDA_HI      or_l( 0x00002000, &GPIO1_OUT)     //  or_b( 0x02, &PBDRL)
#define SDA_INPUT  and_l(~0x00002000, &GPIO1_ENABLE)  // and_b(~0x02, &PBIORL)
#define SDA_OUTPUT  or_l( 0x00002000, &GPIO1_ENABLE)  //  or_b( 0x02, &PBIORL) 
#define SDA             ( 0x00002000 & GPIO1_READ)    // (PBDR & 0x0002)

/* delay loop to achieve 400kHz at 120MHz CPU frequency */
#define DELAY    do { int _x; for(_x=0;_x<22;_x++);} while(0)

static void sw_i2c_init(void)
{
    logf("sw_i2c_init");
    or_l(0x00001000, &GPIO_FUNCTION);
    or_l(0x00002000, &GPIO1_FUNCTION);
    SDA_HI;
    SCL_HI;
    SDA_OUTPUT;
    SCL_OUTPUT;
}

static void sw_i2c_start(void)
{
    SCL_LO;
    SCL_OUTPUT;
    DELAY;
    SDA_HI;
    SDA_OUTPUT;
    DELAY;
    SCL_HI;
    DELAY;
    SDA_LO;
    DELAY;
    SCL_LO;
}

static void sw_i2c_stop(void)
{
   // SCL_LO;
   // DELAY;
   // SDA_LO;
   // DELAY;
    SCL_HI;
    DELAY;
    SDA_HI;
    DELAY;
}


static void sw_i2c_ack(void)
{
    SCL_LO;
    SDA_LO;
    DELAY;
    
    SCL_HI;
    DELAY;
}

static int sw_i2c_getack(void)
{
    int ret = 1;
    int count = 10;

    SCL_LO;
    SDA_INPUT;   /* And set to input */
    DELAY;
    SCL_HI;
    DELAY;

    while (SDA && count--)
        DELAY;
    
    if (SDA)
        /* ack failed */
        ret = 0;
    
    SCL_LO;
    SCL_OUTPUT;
    DELAY;
    SDA_LO;
    SDA_OUTPUT;

    return ret;
}

static void sw_i2c_outb(unsigned char byte)
{
   int i;

   /* clock out each bit, MSB first */
   for ( i=0x80; i; i>>=1 ) {
      SCL_LO;
      DELAY;
      if ( i & byte )
         SDA_HI;
      else
         SDA_LO;
      DELAY;
      SCL_HI;
      DELAY;
   }

   // SDA_LO;
}

static unsigned char sw_i2c_inb(void)
{
    int i;
    unsigned char byte = 0;

    SDA_INPUT;   /* And set to input */
    
    /* clock in each bit, MSB first */
    for ( i=0x80; i; i>>=1 ) 
    {
        SCL_HI;
        DELAY;
        if ( SDA )
            byte |= i;
        SCL_LO;
        DELAY;
    }

    SDA_OUTPUT;
   
    sw_i2c_ack();
    
    return byte;
}

int sw_i2c_write(int location, const unsigned char* buf, int count)
{
    int i;

    sw_i2c_start();
    sw_i2c_outb((EEPROM_ADDR & 0xfe) | SW_I2C_WRITE);
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

int sw_i2c_write_byte(int location, unsigned char byte)
{
    sw_i2c_start();
    sw_i2c_outb((EEPROM_ADDR & 0xfe) | SW_I2C_WRITE);
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
    
    sw_i2c_outb(byte);
    if (!sw_i2c_getack())
    {
        sw_i2c_stop();
        return -3;
    }

    sw_i2c_stop();
    
    return 0;
}

int sw_i2c_read(unsigned char location, unsigned char* byte)
{
    sw_i2c_start();
    sw_i2c_outb((EEPROM_ADDR & 0xfe) | SW_I2C_WRITE);
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
    
    sw_i2c_start();
    sw_i2c_outb((EEPROM_ADDR & 0xfe) | SW_I2C_READ);
    if (!sw_i2c_getack())
    {
        sw_i2c_stop();
        return -3;
    }
    
    *byte = sw_i2c_inb();
    sw_i2c_stop();
    
    return 0;
}

void eeprom_24cxx_init(void)
{
    sw_i2c_init();
}

bool eeprom_24cxx_read_byte(unsigned int address, char *c)
{
    int ret;
    char byte;
    int count = 10;

    if (address >= EEPROM_SIZE)
    {
        logf("EEPROM address: %d", address);
        return false;
    }
    
    *c = 0;
    do {
        ret = sw_i2c_read(address, &byte);
        if (ret < 0)
        {
            logf("EEPROM Fail: %d/%d", ret, address);
        }
    } while (ret < 0 && count--) ;

    if (ret < 0)
    {
        logf("EEPROM RFail: %d/%d", ret, address);
        return false;
    }
    
    *c = byte;
    return true;
}

bool eeprom_24cxx_write_byte(unsigned int address, char c)
{
    int ret;
    int count = 100;

    if (address >= EEPROM_SIZE)
    {
        logf("EEPROM address: %d", address);
        return false;
    }
    
    do {
        ret = sw_i2c_write_byte(address, c);
    } while (ret < 0 && count--) ;

    if (ret < 0)
    {
        logf("EEPROM WFail: %d/%d", ret, address);
        return false;
    }
    
    return true;
}

bool eeprom_24cxx_read(unsigned char address, void *dest, int length)
{
    char *buf = (char *)dest;
    int i;
    
    for (i = 0; i < length; i++)
    {
        if (!eeprom_24cxx_read_byte(address+i, &buf[i]))
            return false;
    }
    
    return true;
}

bool eeprom_24cxx_write(unsigned char address, const void *src, int length)
{
    const char *buf = (const char *)src;
    int count = 10;
    int i, ok;
    
    while (count-- > 0)
    {
        for (i = 0; i < length; i++)
            eeprom_24cxx_write_byte(address+i, buf[i]);
        
        ok = true;
        for (i = 0; i < length; i++)
        {
            char byte;
            
            eeprom_24cxx_read_byte(address+i, &byte);
            if (byte != buf[i])
            {
                logf("Verify failed: %d/%d", address+i, count);
                ok = false;
                break;
            }
        }
        
        if (ok)
            return true;
    }
    
    return false;
}

