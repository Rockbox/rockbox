/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __SPL_X1000_H__
#define __SPL_X1000_H__

#include "boot-x1000.h"
#include <stddef.h>
#include <stdint.h>

/* Memory allocator. Allocation starts from the top of DRAM and counts down.
 * Allocation sizes are rounded up to a multiple of the cacheline size, so
 * the returned address is always suitably aligned for DMA. */
extern void* spl_alloc(size_t count);

/* Access to boot storage medium, eg. flash or MMC/SD card.
 *
 * Read address and length is given in bytes. To make life easier, no
 * alignment restrictions are placed on the buffer, length, or address.
 * The buffer doesn't even need to be in DRAM.
 */
extern int spl_storage_open(void);
extern void spl_storage_close(void);
extern int spl_storage_read(uint32_t addr, uint32_t length, void* buffer);

/* Called on a fatal error -- it should do something visible to the user
 * like flash the backlight repeatedly. */
extern void spl_error(void) __attribute__((noreturn));

#endif /* __SPL_X1000_H__ */
