/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
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
#include <limits.h>
#include "system.h"
#include "config.h"
#include "general.h"

int round_value_to_list32(unsigned long value,
                          const unsigned long list[],
                          int count,
                          bool signd)
{
    unsigned long dmin = ULONG_MAX;
    int idmin = -1, i;

    for (i = 0; i < count; i++)
    {
        unsigned long diff;

        if (list[i] == value)
        {
            idmin = i;
            break;
        }

        if (signd ? ((long)list[i] < (long)value) : (list[i] < value))
            diff = value - list[i];
        else
            diff = list[i] - value;

        if (diff < dmin)
        {
            dmin = diff;
            idmin = i;
        }
    }

    return idmin;
} /* round_value_to_list32 */

/* Number of bits set in src_mask should equal src_list length */
int make_list_from_caps32(unsigned long src_mask,
                          const unsigned long *src_list,
                          unsigned long caps_mask,
                          unsigned long *caps_list)
{
    int i, count;
    unsigned long mask;

    for (mask = src_mask, count = 0, i = 0;
         mask != 0;
         src_mask = mask, i++)
    {
        unsigned long test_bit;
        mask &= mask - 1;           /* Zero lowest bit set */
        test_bit = mask ^ src_mask; /* Isolate the bit */
        if (test_bit & caps_mask)   /* Add item if caps has test bit set */
            caps_list[count++] = src_list ? src_list[i] : (unsigned long)i;
    }

    return count;
} /* make_list_from_caps32 */
