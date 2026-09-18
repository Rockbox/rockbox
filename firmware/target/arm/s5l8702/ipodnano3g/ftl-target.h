/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Michael Sparmann
 *
 * Interface modelled on
 * firmware/target/arm/s5l8700/ipodnano2g/ftl-target.h.
 *
 * Copyright (C) 2026 by Andrew Rice
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

#ifndef __FTL_TARGET_H__
#define __FTL_TARGET_H__

#include <stdint.h>
#include <stdbool.h>

/* Mount Apple's FTL. nand_init() must have succeeded. Returns 0, or a
 * negative number saying which stage failed: -4 if the chip's layout or
 * page size is not supported, -5 if its VFL context does not fit the chip
 * table row. The FTL is mounted read-only, so that writes fail and
 * ftl_sync() does nothing, in the bootloader, on a chip whose row is not
 * validated, and if anything a commit must reproduce could not be
 * loaded. */
int ftl_init(void);

/* Read count logical sectors of NAND_PAGE_SIZE bytes, one or two to a NAND
 * page. Returns 0 or a negative number. */
int ftl_read(uint32_t sector, uint32_t count, void *buffer);

/* Write count logical sectors of NAND_PAGE_SIZE bytes. The data is on the
 * medium when this returns, but the block map that finds it is not: call
 * ftl_sync() to commit that. Returns 0 or a negative number. */
int ftl_write(uint32_t sector, uint32_t count, const void *buffer);

/* Commit the block map, so the writes above survive a remount. Returns 0 or
 * a negative number; after a failure the FTL refuses further writes. */
int ftl_sync(void);

/* Number of logical sectors, 0 if the FTL is not mounted */
uint32_t ftl_num_sectors(void);

/* Whether the mounted FTL refuses writes */
bool ftl_readonly_mount(void);

#endif /* __FTL_TARGET_H__ */
