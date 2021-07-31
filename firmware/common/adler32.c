/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 James Buren (adaptations from tinf/zlib)
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

/* Copyright (c) 2003-2019 Joergen Ibsen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software. If you use this
 *    software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include "adler32.h"
#include "system.h"

/* adler_32 (derived from tinf adler32 which was taken from zlib)
 * Adler-32 algorithm taken from the zlib source, which is
 * Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
 */
uint32_t adler_32(const void *src, uint32_t len, uint32_t adler32)
{
    const unsigned char *buf = (const unsigned char *)src;
    uint32_t s1 = (adler32 & 0xffff);
    uint32_t s2 = (adler32 >> 16);

    enum {
        A32_BASE = 65521,
        A32_NMAX = 5552,
    };

    while (len > 0) {
        uint32_t k = MIN(len, A32_NMAX);
        uint32_t i;

        for (i = k / 16; i; --i, buf += 16) {
            s2 += s1 += buf[0];
            s2 += s1 += buf[1];
            s2 += s1 += buf[2];
            s2 += s1 += buf[3];
            s2 += s1 += buf[4];
            s2 += s1 += buf[5];
            s2 += s1 += buf[6];
            s2 += s1 += buf[7];
            s2 += s1 += buf[8];
            s2 += s1 += buf[9];
            s2 += s1 += buf[10];
            s2 += s1 += buf[11];
            s2 += s1 += buf[12];
            s2 += s1 += buf[13];
            s2 += s1 += buf[14];
            s2 += s1 += buf[15];
        }

        for (i = k % 16; i; --i) {
            s1 += *buf++;
            s2 += s1;
        }

        s1 %= A32_BASE;
        s2 %= A32_BASE;

        len -= k;
    }

    return (s1 | (s2 << 16));
}
