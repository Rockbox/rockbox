/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
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
#include <inttypes.h>

/*
 * Find the index of the least significant set bit in the word.
 * return values:
 *   0  - bit 0 is set
 *   1  - bit 1 is set
 *   ...
 *   31 - bit 31 is set
 *   32 - no bits set
 */
int find_first_set_bit(uint32_t val)
{
    if (val == 0)
        return 32;

    /* __builtin_ctz[l[l]]: Returns the number of trailing 0-bits in x,
     * starting at the least significant bit position. If x is 0, the result
     * is undefined. */
    return __builtin_ctz(val);
}
