/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Original source:
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 *
 * Rockbox adaptation:
 * Copyright (c) 2010 by Marcin Bukat
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

/*
 * tinfzlib  -  tiny zlib decompressor
 *
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 * All Rights Reserved
 *
 * http://www.ibsensoftware.com/
 *
 * This software is provided 'as-is', without any express
 * or implied warranty.  In no event will the authors be
 * held liable for any damages arising from the use of
 * this software.
 *
 * Permission is granted to anyone to use this software
 * for any purpose, including commercial applications,
 * and to alter it and redistribute it freely, subject to
 * the following restrictions:
 *
 * 1. The origin of this software must not be
 *    misrepresented; you must not claim that you
 *    wrote the original software. If you use this
 *    software in a product, an acknowledgment in
 *    the product documentation would be appreciated
 *    but is not required.
 *
 * 2. Altered source versions must be plainly marked
 *    as such, and must not be misrepresented as
 *    being the original software.
 *
 * 3. This notice may not be removed or altered from
 *    any source distribution.
 */

#include "tinf.h"

/* This routine do not check adler32 checksum
 * as it is tailored to be used as PNG decompressor
 * and PNG has crc32 check of the compressed block
 * already
 */
int tinf_zlib_uncompress(void *dest, unsigned int *destLen,
                         const void *source, unsigned int sourceLen)
{
   unsigned char *src = (unsigned char *)source;
   unsigned char *dst = (unsigned char *)dest;
   int res;
   unsigned char cmf, flg;

   /* -- get header bytes -- */

   cmf = src[0];
   flg = src[1];

   /* -- check format -- */

   /* check checksum */
   if (((cmf << 8) + flg) % 31) return TINF_DATA_ERROR;

   /* check method is deflate */
   if ((cmf & 0x0f) != 8) return TINF_DATA_ERROR;

   /* check window size is valid */
   if ((cmf >> 4) > 7) return TINF_DATA_ERROR;

   /* check there is no preset dictionary */
   if (flg & 0x20) return TINF_DATA_ERROR;

   /* -- inflate -- */

   res = tinf_uncompress(dst, destLen, src + 2, sourceLen - 6);

   if (res != TINF_OK) return TINF_DATA_ERROR;

   return TINF_OK;
}
