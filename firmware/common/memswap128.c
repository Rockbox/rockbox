/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <string.h>
#include <inttypes.h>

void memswap128(void *a, void *b, size_t len)
{
    for (len >>= 4; len > 0; len--, a += 16, b += 16)
    {
        int32_t a0 = *((int32_t *)a + 0);
        int32_t a1 = *((int32_t *)a + 1);
        int32_t a2 = *((int32_t *)a + 2);
        int32_t a3 = *((int32_t *)a + 3);
        int32_t b0 = *((int32_t *)b + 0);
        int32_t b1 = *((int32_t *)b + 1);
        int32_t b2 = *((int32_t *)b + 2);
        int32_t b3 = *((int32_t *)b + 3);
        *((int32_t *)b + 0) = a0;
        *((int32_t *)b + 1) = a1;
        *((int32_t *)b + 2) = a2;
        *((int32_t *)b + 3) = a3;
        *((int32_t *)a + 0) = b0;
        *((int32_t *)a + 1) = b1;
        *((int32_t *)a + 2) = b2;
        *((int32_t *)a + 3) = b3;
    }
}
