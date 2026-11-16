/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * Stage 1.5: does what MBRC does before handing over to BREC (SDRAM and
 * cache setup), then loads payload either from SD (BROM driver) or from
 * the data appended to this image (adfu variant) and jumps to it.
 *
 ****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "sdboot.h"

#define REG32(a)    (*(volatile uint32_t *)(a))
#define REG16(a)    (*(volatile int16_t *)(a))

#define CMU_DEVCLKEN    REG32(0xb0010080)
#define RTCWDT_WDCTL    REG32(0xb0018014)
#define GPIO_MFCTL1     REG32(0xb01c001c)
#define SDR_CTL         REG32(0xb0070000)
#define SDR_EN          REG32(0xb0070008)
#define SDR_CMD         REG32(0xb007000c)
#define SDR_STAT        REG32(0xb0070010)
#define SDR_RFSH        REG32(0xb0070014)
#define SDR_MODE        REG32(0xb0070018)

/* boot status word shared by BROM/MBRC/BREC, we use 0x4 tag */
#define BOOT_STATUS     REG32(0xb40487fc)
#define STATUS_TAG      0x40000000

#define KSEG1(a)        (((a) & 0x1fffffff) | 0xa0000000)

extern struct sdboot_hdr_t hdr;
extern char payload_start[];
extern char image_start[], image_end[];

int sd_read_sector(uint32_t lba, void *dst);
void cache_init(void);
void jump(uint32_t entry, void (*ret)(void)) __attribute__((noreturn));

static inline void wdt_feed(void)
{
    RTCWDT_WDCTL |= 1;
}

/* SDRAM setup and size probe exactly as done by MBRC */
static bool sdram_init(void)
{
    bool e1, e2, e3, e4, e5;

    CMU_DEVCLKEN |= 0x60;
    SDR_EN = 1;
    SDR_RFSH = 0x1c4;
    SDR_CTL = 0xc0150;
    SDR_MODE = (SDR_MODE & ~0x70) | 0x20;
    SDR_STAT = 1;
    SDR_CMD = 0;
    SDR_CMD = 0xa0;
    while (!(SDR_STAT & 1))
    {
    }
    SDR_STAT = 1;

    REG16(0xa03ff9fe) = 1; REG16(0xa0000000) = 1;
    REG16(0xa07ff9fe) = 2; REG16(0xa0000002) = 2;
    REG16(0xa07ffbfe) = 3; REG16(0xa0000004) = 3;
    REG16(0xa0fffbfe) = 4; REG16(0xa0000006) = 4;
    REG16(0xa0fffffe) = 5; REG16(0xa0000008) = 5;

    e1 = REG16(0xa03ff9fe) == REG16(0xa0000000);
    e2 = REG16(0xa07ff9fe) == REG16(0xa0000002);
    e3 = REG16(0xa07ffbfe) == REG16(0xa0000004);
    e4 = REG16(0xa0fffbfe) == REG16(0xa0000006);
    e5 = REG16(0xa0fffffe) == REG16(0xa0000008);

    if (!e1)
    {
        if (!(e2 && e3 && e4 && e5))
        {
            return false;
        }
        SDR_CTL = 0xc0100;
        SDR_RFSH = 0x1b4;
    }
    else if (!e2)
    {
        if (!(e3 && e4 && e5))
        {
            return false;
        }
        SDR_CTL = 0xc0120;
        SDR_RFSH = 0x1b4;
    }
    else if (!e3)
    {
        if (!(e4 && e5))
        {
            return false;
        }
        SDR_CTL = 0xc0130;
        SDR_RFSH = 0x1b4;
    }
    else if (!e4)
    {
        if (!e5)
        {
            return false;
        }
        SDR_CTL = 0xc0140;
    }
    else if (!e5)
    {
        return false;
    }

    return true;
}

static void brom_adfu(void)
{
    volatile uint32_t *src = (volatile uint32_t *)BROM_ADFU_SRC;
    volatile uint32_t *dst = (volatile uint32_t *)BROM_ADFU_DST;
    unsigned i;

    for (i = 0; i < BROM_ADFU_SIZE / 4; i++)
    {
        dst[i] = src[i];
    }

    jump(BROM_ADFU_DST, 0);
}

static void fail(uint32_t code)
{
    BOOT_STATUS = STATUS_TAG | code;
    brom_adfu();
}

static bool overlaps(uint32_t a, uint32_t alen, uint32_t b, uint32_t blen)
{
    a &= 0x1fffffff;
    b &= 0x1fffffff;
    return a < b + blen && b < a + alen;
}

/* payload must not overwrite us (including appended payload),
 * our stack or BROM SD driver */
static bool payload_fits(uint32_t load, uint32_t size)
{
    uint32_t img = (uint32_t)image_start;
    uint32_t img_size = (uint32_t)image_end - img;

    if (hdr.flags & SDBOOT_PAYLOAD_SD)
    {
        if (overlaps(load, size, BROM_DRV_ADDR, 0x1000))
        {
            return false;
        }
    }
    else
    {
        img_size += hdr.size;
    }

    if (overlaps(load, size, img, img_size))
    {
        return false;
    }

    return !overlaps(load, size, STAGE1_STACK - 0x800, 0x800);
}

void sdboot_main(void) __attribute__((noreturn));
void sdboot_main(void)
{
    uint8_t *dst = (uint8_t *)KSEG1(hdr.load);
    uint32_t i;

    wdt_feed();

    if (hdr.magic != SDBOOT_MAGIC || hdr.size == 0)
    {
        fail(1);
    }

    if (!payload_fits(hdr.load, hdr.size))
    {
        fail(2);
    }

    /* same as MBRC does before SDRAM setup */
    CMU_DEVCLKEN |= 0x6000000;
    GPIO_MFCTL1 |= 0x80001000;

    if ((hdr.flags & SDBOOT_SDRAM_INIT) && !sdram_init())
    {
        fail(3);
    }

    cache_init();

    if (hdr.flags & SDBOOT_PAYLOAD_SD)
    {
        for (i = 0; i < (hdr.size + 511) / 512; i++)
        {
            wdt_feed();
            if (sd_read_sector(hdr.lba + i, dst + i * 512) != 1)
            {
                fail(4);
            }
        }
    }
    else
    {
        uint8_t *src = (uint8_t *)KSEG1((uint32_t)payload_start);
        for (i = 0; i < hdr.size; i++)
        {
            dst[i] = src[i];
        }
    }

    wdt_feed();
    /* if payload returns (BREC does on failure) fall back to ADFU */
    jump(hdr.entry, brom_adfu);
}
