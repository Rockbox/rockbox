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

#ifndef __MIPSR2_ENDIAN_H__
#define __MIPSR2_ENDIAN_H__

#include <stdint.h>

static inline uint32_t swap32_hw(uint32_t value)
{
    register uint32_t out;
    __asm__ ("wsbh %0, %1\n"
             "rotr %0, %0, 16\n"
             : "=r"(out) : "r"(value));
    return out;
}

static inline uint16_t swap16_hw(uint16_t value)
{
    register uint32_t out, in;
    in = value;
    __asm__ ("wsbh %0, %1" : "=r"(out) : "r"(in));
    return (uint16_t)out;
}

static inline uint32_t swap_odd_even32_hw(uint32_t value)
{
    register uint32_t out;
    __asm__ ("wsbh %0, %1" : "=r"(out) : "r"(value));
    return out;
}

#endif /* __MIPSR2_ENDIAN_H__ */
