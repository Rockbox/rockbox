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

#include "logf.h"
#include "string.h"
#include "inttypes.h"

#include "sw_i2c.h"

#include "eeprom_24cxx.h"

/* Use cache to speedup writing to the chip. */
static char data_cache[EEPROM_SIZE];
static uint8_t cached_bitfield[EEPROM_SIZE/8];

#define IS_CACHED(addr) (cached_bitfield[addr/8] & (1 << (addr % 8)))
#define SET_CACHED(addr) (cached_bitfield[addr/8] |= 1 << (addr % 8)) 

void eeprom_24cxx_init(void)
{
    sw_i2c_init();
    memset(cached_bitfield, 0, sizeof cached_bitfield);
}

static int eeprom_24cxx_read_byte(unsigned int address, char *c)
{
    int ret;
    char byte;
    int count = 0;

    if (address >= EEPROM_SIZE)
    {
        logf("EEPROM address: %d", address);
        return -9;
    }
    
    /* Check from cache. */
    if (IS_CACHED(address))
    {
        logf("EEPROM RCached: %d", address);
        *c = data_cache[address];
        return 0;
    }
    
    *c = 0;
    do
    {
        ret = sw_i2c_read(EEPROM_ADDR, address, &byte, 1);
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
    
    /* Cache the byte. */
    data_cache[address] = byte;
    SET_CACHED(address);
    
    *c = byte;
    return 0;
}

static int eeprom_24cxx_write_byte(unsigned int address, char c)
{
    int ret;
    int count = 0;

    if (address >= EEPROM_SIZE)
    {
        logf("EEPROM address: %d", address);
        return -9;
    }
    
    /* Check from cache. */
    if (IS_CACHED(address) && data_cache[address] == c)
    {
        logf("EEPROM WCached: %d", address);
        return 0;
    }
    
    do
    {
        ret = sw_i2c_write(EEPROM_ADDR, address, &c, 1);
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
    
    SET_CACHED(address);
    data_cache[address] = c;
    
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
    int i;
    
    for (i = 0; i < length; i++)
    {
        if (eeprom_24cxx_write_byte(address+i, buf[i]) < 0)
            return -1;
    }
    
    return 0;
}

