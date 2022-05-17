/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#ifndef SYSTEM_MIPS_H
#define SYSTEM_MIPS_H

#include <stdint.h>

struct mips_exception_frame {
    uint32_t gpr[29];       /* GPRs $1-$25, $28-$31 */
    uint32_t lo;
    uint32_t hi;
    uint32_t c0_status;
    uint32_t c0_epc;
};

void intr_handler(void);
void exception_handler(void* frame, unsigned long epc);
void cache_error_handler(void* frame, unsigned long epc);
void tlb_refill_handler(void* frame, unsigned long epc);

#endif /* SYSTEM_MIPS_H */
