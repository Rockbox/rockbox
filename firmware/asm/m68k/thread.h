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

struct regs
{
    uint32_t macsr; /*     0 - EMAC status register */
    uint32_t d[6];  /*  4-24 - d2-d7 */
    uint32_t a[5];  /* 28-44 - a2-a6 */
    uint32_t sp;    /*    48 - Stack pointer (a7) */
    uint32_t start; /*    52 - Thread start address, or NULL when started */
};

#define DEFAULT_STACK_SIZE 0x400 /* Bytes */
