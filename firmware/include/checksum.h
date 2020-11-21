/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 James Buren
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
#include <stdint.h>
#include <stdbool.h>

#ifndef _CHECKSUM_H
#define _CHECKSUM_H

/* rockbox firmware checksum algorithm */
static inline uint32_t calc_checksum(uint32_t sum, const uint8_t* buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        sum += buf[i];
    return sum;
}

/* similar to above but only used for verification */
static inline bool verify_checksum(uint32_t cs, const uint8_t* buf, size_t len)
{
    return (calc_checksum(MODEL_NUMBER, buf, len) == cs);
}

#endif
