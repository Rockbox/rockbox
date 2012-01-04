/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Ulf Ralberg
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

#include "config.h"

struct regs
{
    uint32_t r[8];  /*  0-28 - Registers r4-r11 */
    uint32_t sp;    /*    32 - Stack pointer (r13) */
    uint32_t lr;    /*    36 - r14 (lr) */
    uint32_t start; /*    40 - Thread start address, or NULL when started */
};

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
  #define DEFAULT_STACK_SIZE 0x1000 /* Bytes */
#else
  #define DEFAULT_STACK_SIZE 0x400  /* Bytes */
#endif
