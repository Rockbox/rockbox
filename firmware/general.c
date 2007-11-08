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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

/* Only needed for cache aligning atm */
#ifdef PROC_NEEDS_CACHEALIGN
/* Align a buffer and size to a size boundary while remaining within
 * the original boundaries */
size_t align_buffer(void **start, size_t size, size_t align)
{
    void *newstart = *start;
    void *newend = newstart + size;

    /* Align the end down and the start up */
    newend = (void *)ALIGN_DOWN((intptr_t)newend, align);
    newstart = (void *)ALIGN_UP((intptr_t)newstart, align);

    /* Hmmm - too small for this */
    if (newend <= newstart)
        return 0;

    /* Return adjusted pointer and size */
    *start = newstart;
    return newend - newstart;
}
#endif /* PROC_NEEDS_CACHEALIGN */
