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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <inttypes.h>

/* find_first_set_bit() - this is a C version of the ffs algorithm devised
 * by D.Seal and posted to comp.sys.arm on  16 Feb 1994.
 *
 * Find the index of the least significant set bit in the word.
 * return values:
 *   0  - bit 0 is set
 *   1  - bit 1 is set
 *   ...
 *   31 - bit 31 is set
 *   32 - no bits set
 */

/* Table shared with assembly code */
const uint8_t L_ffs_table[64] ICONST_ATTR =
{
/*   0   1   2   3   4   5   6   7           */
/* ----------------------------------------- */
    32,  0,  1, 12,  2,  6,  0, 13, /*  0- 7 */
     3,  0,  7,  0,  0,  0,  0, 14, /*  8-15 */
    10,  4,  0,  0,  8,  0,  0, 25, /* 16-23 */
     0,  0,  0,  0,  0, 21, 27, 15, /* 24-31 */
    31, 11,  5,  0,  0,  0,  0,  0, /* 32-39 */
     9,  0,  0, 24,  0,  0, 20, 26, /* 40-47 */
    30,  0,  0,  0,  0, 23,  0, 19, /* 48-55 */
    29,  0, 22, 18, 28, 17, 16,  0, /* 56-63 */
};

#if !defined(CPU_COLDFIRE)
int find_first_set_bit(uint32_t val)
{
    return L_ffs_table[((val & -val)*0x0450fbaf) >> 26];
}
#endif
