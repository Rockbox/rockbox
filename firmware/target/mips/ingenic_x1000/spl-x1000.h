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

#include "spl-x1000-defs.h"

#define SPL_ARGUMENTS ((struct x1000_spl_arguments*)SPL_ARGUMENTS_ADDRESS)
#define SPL_STATUS    ((struct x1000_spl_status*)SPL_STATUS_ADDRESS)

struct spl_boot_option {
    uint32_t nand_addr;
    uint32_t nand_size;
    uint32_t load_addr;
    uint32_t exec_addr;
    const char* cmdline; /* for Linux */
};

/* Defined by target, indices are 0 = ROCKBOX, 1 = ORIG_FW, etc... */
extern const struct spl_boot_option spl_boot_options[];

/* Called on a fatal error */
extern void spl_error(void) __attribute__((noreturn));

/* When SPL boots with SPL_BOOTOPTION_CHOOSE, this function is invoked
 * to let the target figure out the boot option based on buttons the
 * user is pressing */
extern int spl_get_boot_option(void);

/* Do any setup/initialization needed for the given boot option, this
 * will be called right before flushing caches + jumping to the image.
 * Typical use is to set up system clocks, etc.
 */
extern void spl_handle_pre_boot(int bootopt);

#endif /* __SPL_X1000_H__ */
