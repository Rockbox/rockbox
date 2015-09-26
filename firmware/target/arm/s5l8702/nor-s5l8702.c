/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright © 2009 Michael Sparmann
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "config.h"
#include "system.h"
#include "s5l8702.h"
#include "spi-s5l8702.h"
#include "nor-target.h"

static void bootflash_ce(int port, bool state)
{
    spi_ce(port, state);
}

void bootflash_init(int port)
{
    spi_init(port, true);
    bootflash_ce(port, false);
}

void bootflash_close(int port)
{
    spi_init(port, false);
}

static void bootflash_wait_ready(int port)
{
    while (true)
    {
        bootflash_ce(port, true);
        spi_write(port, 5);
        if (!(spi_write(port, 0xff) & 1)) break;
        bootflash_ce(port, false);
    }
    bootflash_ce(port, false);
}

static void bootflash_enable_writing(int port, bool state)
{
    if (!state)
    {
        bootflash_ce(port, true);
        spi_write(port, 4);
        bootflash_ce(port, false);
    }
    bootflash_ce(port, true);
    spi_write(port, 0x50);
    bootflash_ce(port, false);
    bootflash_ce(port, true);
    spi_write(port, 1);
    spi_write(port, state ? 0 : 0x1c);
    bootflash_ce(port, false);
    if (state)
    {
        bootflash_ce(port, true);
        spi_write(port, 6);
        bootflash_ce(port, false);
    }
}

void bootflash_read(int port, uint32_t addr, uint32_t size, void* buf)
{
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_ce(port, true);
    spi_write(port, 3);
    spi_write(port, (addr >> 16) & 0xff);
    spi_write(port, (addr >> 8) & 0xff);
    spi_write(port, addr & 0xff);
    spi_read(port, size, buf);
    bootflash_ce(port, false);
    spi_release(port);
}

static void bootflash_write_internal(int port, uint32_t addr, uint32_t size, void* buf)
{
    uint8_t* buffer = (uint8_t*)buf;
    bool first = true;
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_enable_writing(port, true);
    while (size)
    {
        bootflash_ce(port, true);
        spi_write(port, 0xad);
        if (first)
        {
            spi_write(port, (addr >> 16) & 0xff);
            spi_write(port, (addr >> 8) & 0xff);
            spi_write(port, addr & 0xff);
            first = false;
        }
        spi_write(port, *buffer++);
        spi_write(port, *buffer++);
        bootflash_ce(port, false);
        bootflash_wait_ready(port);
        size -= 2;
    }
    bootflash_enable_writing(port, false);
    spi_release(port);
}

static void bootflash_erase_internal(int port, uint32_t addr)
{
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_enable_writing(port, true);
    bootflash_ce(port, true);
    spi_write(port, 0x20);
    spi_write(port, (addr >> 16) & 0xff);
    spi_write(port, (addr >> 8) & 0xff);
    spi_write(port, addr & 0xff);
    bootflash_ce(port, false);
    bootflash_enable_writing(port, false);
    spi_release(port);
}

void bootflash_erase_blocks(int port, int first, int n)
{
    uint32_t offset = first << 12;
    while (n--)
    {
        bootflash_erase_internal(port, offset);
        offset += 0x1000;
    }
}

int bootflash_compare(int port, int offset, void* addr, int size)
{
    int i;
    int result = 0;
    uint8_t buf[32];
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_ce(port, true);
    spi_write(port, 3);
    spi_write(port, (offset >> 16) & 0xff);
    spi_write(port, (offset >> 8) & 0xff);
    spi_write(port, offset & 0xff);
    while (size > 0)
    {
        spi_read(port, MIN((int)sizeof(buf), size), buf);
        if (memcmp((uint8_t*)addr, buf, MIN((int)sizeof(buf), size))) result |= 1;
        for (i = 0; i < MIN((int)sizeof(buf), size); i += 2)
            if (buf[i] != 0xff) result |= 2;
        addr = (void*)(((uint32_t)addr) + sizeof(buf));
        size -= sizeof(buf);
    }
    bootflash_ce(port, false);
    spi_release(port);
    return result;
}

void bootflash_write(int port, int offset, void* addr, int size)
{
    int i;
    bool needswrite;
    while (size > 0)
    {
        int remainder = MIN(0x1000 - (offset & 0xfff), size);
        int contentinfo = bootflash_compare(port, offset, addr, remainder);
        if (contentinfo & 1)
        {
            if (contentinfo & 2) bootflash_erase_internal(port, offset & ~0xfff);
            needswrite = false;
            for (i = 0; i < remainder; i += 1)
                if (((uint8_t*)addr)[i] != 0xff)
                {
                    needswrite = true;
                    break;
                }
            if (needswrite) bootflash_write_internal(port, offset, remainder, addr);
        }
        addr = (void*)(((uint32_t)addr) + remainder);
        offset += remainder;
        size -= remainder;
    }
}
