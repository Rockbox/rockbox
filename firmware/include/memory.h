/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <sys/types.h>

void memset16(void *dst, int val, size_t len);

/**
 * memswap128
 *
 * Exchanges the contents of two buffers.
 * For maximum efficiency, this function performs no aligning of addresses
 * and buf1, buf2 and len should be 16 byte (128 bit) aligned. Not being at
 * least longword aligned will fail on some architechtures. Any len mod 16
 * at the end is not swapped.
 */
void memswap128(void *buf1, void *buf2, size_t len);

#endif /* _MEMORY_H_ */
