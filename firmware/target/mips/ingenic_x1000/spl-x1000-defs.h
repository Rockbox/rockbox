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

#ifndef __SPL_X1000_DEFS_H__
#define __SPL_X1000_DEFS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPL_CMD_BOOT        0
#define SPL_CMD_FLASH_READ  1
#define SPL_CMD_FLASH_WRITE 2

#define SPL_BOOTOPT_CHOOSE   0
#define SPL_BOOTOPT_ROCKBOX  1
#define SPL_BOOTOPT_ORIG_FW  2
#define SPL_BOOTOPT_RECOVERY 3
#define SPL_BOOTOPT_NONE     4

#define SPL_FLAG_SKIP_INIT   (1 << 0)

#define SPL_MAX_SIZE          (12 * 1024)
#define SPL_LOAD_ADDRESS      0xf4001000
#define SPL_EXEC_ADDRESS      0xf4001800
#define SPL_ARGUMENTS_ADDRESS 0xf40011f0
#define SPL_STATUS_ADDRESS    0xf40011e0
#define SPL_BUFFER_ADDRESS    0xa0004000

struct x1000_spl_arguments {
    uint32_t command;
    uint32_t param1;
    uint32_t param2;
    uint32_t flags;
};

struct x1000_spl_status {
    int err_code;
    int reserved;
};

#ifdef __cplusplus
}
#endif

#endif /* __SPL_X1000_DEFS_H__ */
