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

/* h1x0 needs its own i2c driver,
   h3x0 uses the pcf i2c driver */

#ifdef IRIVER_H100_SERIES

/* cute little functions, atomic read-modify-write */

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
#define DELAY    do { int _x; for(_x=0;_x<22;_x++);} while(0)

static void sw_i2c_init(void)
{
    logf("sw_i2c_init");
    or_l(0x00001000, &GPIO_FUNCTION);
    or_l(0x00002000, &GPIO1_FUNCTION);
    SDA_HI;
    SCL_HI;
    SDA_OUT_LO;
    SCL_OUT_LO;
}

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

static void sw_i2c_stop(void)
{
    SCL_HI;
    DELAY;
    SDA_HI;
    DELAY;
}

static void sw_i2c_ack(void)
{
    SCL_LO;
    DELAY;
    SDA_LO;
    DELAY;
    
    SCL_HI;
    DELAY;
}

static bool sw_i2c_getack(void)
{
    bool ret = true;
    int count = 10;

    SCL_LO;
    DELAY;
    SDA_HI;   /* sets to input */
    DELAY;
    SCL_HI;
    DELAY;

    while (SDA && count--)
        DELAY;
    
    if (SDA)
        /* ack failed */
        ret = false;
    
    SCL_LO;
    DELAY;
    SDA_LO;

    return ret;
}

static void sw_i2c_outb(unsigned char byte)
{
    int i;

    /* clock out each bit, MSB first */
    for ( i=0x80; i; i>>=1 )
    {
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
}

static unsigned char sw_i2c_inb(void)
{
    int i;
    unsigned char byte = 0;

    SDA_HI;   /* sets to input */
    
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

    sw_i2c_ack();
    
    return byte;
}

#else

#include "pcf50606.h"

#define sw_i2c_init()       /* no extra init required */
#define sw_i2c_start()      pcf50606_i2c_start()
#define sw_i2c_stop()       pcf50606_i2c_stop()
#define sw_i2c_ack()        pcf50606_i2c_ack(true)
#define sw_i2c_getack()     pcf50606_i2c_getack()
#define sw_i2c_outb(x)      pcf50606_i2c_outb(x)
#define sw_i2c_inb()        pcf50606_i2c_inb(false)

#endif /* IRIVER_H100_SERIES */


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

int eeprom_24cxx_read_byte(unsigned int address, char *c)
{
    int ret;
    char byte;
    int count = 0;

    if (address >= EEPROM_SIZE)
    {
        logf("EEPROM address: %d", address);
        return -9;
    }
    
    *c = 0;
    do
    {
        ret = sw_i2c_read(address, &byte);
    } while (ret < 0 && count++ < 200);

    if (ret < 0)
    {
        logf("EEPROM RFail: %d/%d/%d", ret, address, count);
        return ret;
    }
    
    if (count)
    {
        /* keep between {} as logf is whitespace in normal builds */
        logf("EEPROM rOK: %d retries", count);
    }
        
    *c = byte;
    return 0;
}

int eeprom_24cxx_write_byte(unsigned int address, char c)
{
    int ret;
    int count = 0;

    if (address >= EEPROM_SIZE)
    {
        logf("EEPROM address: %d", address);
        return -9;
    }
    
    do
    {
        ret = sw_i2c_write_byte(address, c);
    } while (ret < 0 && count++ < 200) ;

    if (ret < 0)
    {
        logf("EEPROM WFail: %d/%d", ret, address);
        return ret;
    }
    
    if (count)
    {
        /* keep between {} as logf is whitespace in normal builds */
        logf("EEPROM wOK: %d retries", count);
    }
    
    return 0;
}

int eeprom_24cxx_read(unsigned char address, void *dest, int length)
{
    char *buf = (char *)dest;
    int ret = 0;
    int i;
    
    for (i = 0; i < length; i++)
    {
        ret = eeprom_24cxx_read_byte(address+i, &buf[i]);
        if (ret < 0)
            return ret;
    }
    
    return ret;
}

int eeprom_24cxx_write(unsigned char address, const void *src, int length)
{
    const char *buf = (const char *)src;
    int count = 5;
    int i;
    bool ok;
    
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
            return 0;
    }
    
    return -1;
}

