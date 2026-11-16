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
 ****************************************************************************/
#ifndef SDBOOT_H
#define SDBOOT_H

/* BROM resident boot media driver (copied by BROM to IRAM) */
#define BROM_DRV_ADDR       0xb4040200
#define BROM_DRV_SD_READ    3

/* BROM ADFU launcher image, MBRC copies it to IRAM on boot failure */
#define BROM_ADFU_SRC       0xbfc05400
#define BROM_ADFU_DST       0xb4046800
#define BROM_ADFU_SIZE      0x2000

/* IRAM 0xb4041200..0xb4042a00 is unused while BROM SD driver is resident
 * (driver code and variables end below 0xb4041200) */
#define STAGE15_ADDR        0xb4041400
#define STAGE15_MAX_SIZE    0x1600
#define STAGE15_LBA         2
#define STAGE15_SECTORS     (STAGE15_MAX_SIZE / 512)
#define PAYLOAD_LBA         (STAGE15_LBA + STAGE15_SECTORS)

/* stack below BREC load address (BREC sets up its own sp at 0xb4047000) */
#define STAGE1_STACK        0xb4048000

#define SDBOOT_MAGIC        0x54424453 /* 'SDBT' */

/* header flags */
#define SDBOOT_SDRAM_INIT   (1 << 0) /* replicate MBRC SDRAM setup */
#define SDBOOT_PAYLOAD_SD   (1 << 1) /* payload on SD at hdr.lba, else appended */

#ifndef __ASSEMBLER__
#include <stdint.h>

struct sdboot_hdr_t
{
    uint32_t magic;
    uint32_t load;      /* load address of payload */
    uint32_t entry;     /* entry point */
    uint32_t size;      /* payload size in bytes */
    uint32_t lba;       /* first payload sector on SD */
    uint32_t flags;
};
#endif

#endif
