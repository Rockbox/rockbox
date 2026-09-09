/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __SPL_X1600_H__
#define __SPL_X1600_H__

#include <stddef.h>
#include <stdint.h>

/* Bootloader storage starts at 0x6800, beyond the vendor partition table
 * at 0x5800. SPL and bootloader share a 128 KiB erase block; installation
 * must preserve the rest of that block. */
#define SPL_BOOT_STORAGE_ADDR   0x6800
#define SPL_BOOT_STORAGE_SIZE   (102 * 1024)

#include "spl-ingenic.h"

#endif /* __SPL_X1600_H__ */
