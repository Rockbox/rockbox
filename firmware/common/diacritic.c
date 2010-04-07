/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Phinitnun Chanasabaeng
 *     Initial work
 * Copyright (C) 2009 Tomer Shalev
 *
 * Rockbox diacritic positioning
 * Based on initial work by Phinitnun Chanasabaeng
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
#include "diacritic.h"
#include "system.h"

#define DIAC_NUM_RANGES      (ARRAYLEN(diac_ranges))

/* Each diac_range_ struct defines a Unicode range that begins with
 * N diacritic characters, and continues with non-diacritic characters up to the
 * base of the next item in the array */
struct diac_range
{
    unsigned base           : 16;
    unsigned num_diacritics :  7;
    unsigned is_rtl         :  1;
};

#define DIAC_RANGE_ENTRY(first_diac, first_non_diac, is_rtl) \
    { first_diac, first_non_diac - first_diac, is_rtl }

/* Sorted by Unicode value */
static const struct diac_range diac_ranges[] =
{
    DIAC_RANGE_ENTRY(0x0000, 0x0000, 0),
    DIAC_RANGE_ENTRY(0x0300, 0x0370, 0),
    DIAC_RANGE_ENTRY(0x0483, 0x048a, 0),
    DIAC_RANGE_ENTRY(0x0591, 0x05be, 1),
    DIAC_RANGE_ENTRY(0x05bf, 0x05c0, 1),
    DIAC_RANGE_ENTRY(0x05c1, 0x05c3, 1),
    DIAC_RANGE_ENTRY(0x05c4, 0x05c6, 1),
    DIAC_RANGE_ENTRY(0x05c7, 0x05c8, 1),
    DIAC_RANGE_ENTRY(0x0610, 0x061b, 1),
    DIAC_RANGE_ENTRY(0x064b, 0x065f, 1),
    DIAC_RANGE_ENTRY(0x0670, 0x0671, 1),
    DIAC_RANGE_ENTRY(0x06d6, 0x06dd, 1),
    DIAC_RANGE_ENTRY(0x06df, 0x06e5, 1),
    DIAC_RANGE_ENTRY(0x06e7, 0x06e9, 1),
    DIAC_RANGE_ENTRY(0x06ea, 0x06ee, 1),
    DIAC_RANGE_ENTRY(0x0711, 0x0712, 1),
    DIAC_RANGE_ENTRY(0x0730, 0x074b, 1),
    DIAC_RANGE_ENTRY(0x07a6, 0x07b1, 1),
    DIAC_RANGE_ENTRY(0x07bf, 0x07c0, 0),
    DIAC_RANGE_ENTRY(0x07eb, 0x07f4, 0),
    DIAC_RANGE_ENTRY(0x0816, 0x081a, 0),
    DIAC_RANGE_ENTRY(0x081b, 0x0824, 0),
    DIAC_RANGE_ENTRY(0x0825, 0x0828, 0),
    DIAC_RANGE_ENTRY(0x0829, 0x082e, 0),
    DIAC_RANGE_ENTRY(0x0900, 0x0904, 0),
    DIAC_RANGE_ENTRY(0x093c, 0x093d, 0),
    DIAC_RANGE_ENTRY(0x093e, 0x094f, 0),
    DIAC_RANGE_ENTRY(0x0951, 0x0956, 0),
    DIAC_RANGE_ENTRY(0x0962, 0x0964, 0),
    DIAC_RANGE_ENTRY(0x0981, 0x0984, 0),
    DIAC_RANGE_ENTRY(0x09bc, 0x09bd, 0),
    DIAC_RANGE_ENTRY(0x09be, 0x09ce, 0),
    DIAC_RANGE_ENTRY(0x09d7, 0x09d8, 0),
    DIAC_RANGE_ENTRY(0x09e2, 0x09e4, 0),
    DIAC_RANGE_ENTRY(0x0a01, 0x0a04, 0),
    DIAC_RANGE_ENTRY(0x0a3c, 0x0a52, 0),
    DIAC_RANGE_ENTRY(0x0a70, 0x0a72, 0),
    DIAC_RANGE_ENTRY(0x0a75, 0x0a76, 0),
    DIAC_RANGE_ENTRY(0x0a81, 0x0a84, 0),
    DIAC_RANGE_ENTRY(0x0abc, 0x0abd, 0),
    DIAC_RANGE_ENTRY(0x0abe, 0x0ace, 0),
    DIAC_RANGE_ENTRY(0x0ae2, 0x0ae4, 0),
    DIAC_RANGE_ENTRY(0x0b01, 0x0b04, 0),
    DIAC_RANGE_ENTRY(0x0b3c, 0x0b3d, 0),
    DIAC_RANGE_ENTRY(0x0b3e, 0x0b58, 0),
    DIAC_RANGE_ENTRY(0x0b82, 0x0b83, 0),
    DIAC_RANGE_ENTRY(0x0bbe, 0x0bce, 0),
    DIAC_RANGE_ENTRY(0x0bd7, 0x0bd8, 0),
    DIAC_RANGE_ENTRY(0x0c01, 0x0c04, 0),
    DIAC_RANGE_ENTRY(0x0c3e, 0x0c57, 0),
    DIAC_RANGE_ENTRY(0x0c62, 0x0c64, 0),
    DIAC_RANGE_ENTRY(0x0c82, 0x0c84, 0),
    DIAC_RANGE_ENTRY(0x0cbc, 0x0cbd, 0),
    DIAC_RANGE_ENTRY(0x0cbe, 0x0cd7, 0),
    DIAC_RANGE_ENTRY(0x0ce2, 0x0ce4, 0),
    DIAC_RANGE_ENTRY(0x0d02, 0x0d04, 0),
    DIAC_RANGE_ENTRY(0x0d3e, 0x0d58, 0),
    DIAC_RANGE_ENTRY(0x0d62, 0x0d64, 0),
    DIAC_RANGE_ENTRY(0x0d82, 0x0d84, 0),
    DIAC_RANGE_ENTRY(0x0dca, 0x0df4, 0),
    DIAC_RANGE_ENTRY(0x0e31, 0x0e32, 0),
    DIAC_RANGE_ENTRY(0x0e34, 0x0e3b, 0),
    DIAC_RANGE_ENTRY(0x0e47, 0x0e4f, 0),
    DIAC_RANGE_ENTRY(0x0eb1, 0x0eb2, 0),
    DIAC_RANGE_ENTRY(0x0eb4, 0x0ebd, 0),
    DIAC_RANGE_ENTRY(0x0ec8, 0x0ece, 0),
    DIAC_RANGE_ENTRY(0x0f18, 0x0f1a, 0),
    DIAC_RANGE_ENTRY(0x0f35, 0x0f36, 0),
    DIAC_RANGE_ENTRY(0x0f37, 0x0f38, 0),
    DIAC_RANGE_ENTRY(0x0f39, 0x0f3a, 0),
    DIAC_RANGE_ENTRY(0x0f3e, 0x0f40, 0),
    DIAC_RANGE_ENTRY(0x0f71, 0x0f85, 0),
    DIAC_RANGE_ENTRY(0x0f86, 0x0f88, 0),
    DIAC_RANGE_ENTRY(0x0f90, 0x0fbd, 0),
    DIAC_RANGE_ENTRY(0x102b, 0x103f, 0),
    DIAC_RANGE_ENTRY(0x1056, 0x105a, 0),
    DIAC_RANGE_ENTRY(0x105e, 0x1061, 0),
    DIAC_RANGE_ENTRY(0x1062, 0x1065, 0),
    DIAC_RANGE_ENTRY(0x1067, 0x106e, 0),
    DIAC_RANGE_ENTRY(0x1071, 0x1075, 0),
    DIAC_RANGE_ENTRY(0x1082, 0x108e, 0),
    DIAC_RANGE_ENTRY(0x108f, 0x1090, 0),
    DIAC_RANGE_ENTRY(0x109a, 0x109e, 0),
    DIAC_RANGE_ENTRY(0x135f, 0x1360, 0),
    DIAC_RANGE_ENTRY(0x1712, 0x1715, 0),
    DIAC_RANGE_ENTRY(0x1732, 0x1735, 0),
    DIAC_RANGE_ENTRY(0x1752, 0x1754, 0),
    DIAC_RANGE_ENTRY(0x1772, 0x1774, 0),
    DIAC_RANGE_ENTRY(0x17b6, 0x17d4, 0),
    DIAC_RANGE_ENTRY(0x17dd, 0x17de, 0),
    DIAC_RANGE_ENTRY(0x18a9, 0x18aa, 0),
    DIAC_RANGE_ENTRY(0x1920, 0x193c, 0),
    DIAC_RANGE_ENTRY(0x19b0, 0x19c1, 0),
    DIAC_RANGE_ENTRY(0x19c8, 0x19ca, 0),
    DIAC_RANGE_ENTRY(0x1a17, 0x1a1c, 0),
    DIAC_RANGE_ENTRY(0x1a55, 0x1a80, 0),
    DIAC_RANGE_ENTRY(0x1b00, 0x1b05, 0),
    DIAC_RANGE_ENTRY(0x1b34, 0x1b45, 0),
    DIAC_RANGE_ENTRY(0x1b6b, 0x1b74, 0),
    DIAC_RANGE_ENTRY(0x1b80, 0x1b83, 0),
    DIAC_RANGE_ENTRY(0x1ba1, 0x1bab, 0),
    DIAC_RANGE_ENTRY(0x1c24, 0x1c38, 0),
    DIAC_RANGE_ENTRY(0x1cd0, 0x1cd3, 0),
    DIAC_RANGE_ENTRY(0x1cd4, 0x1ce9, 0),
    DIAC_RANGE_ENTRY(0x1ced, 0x1cee, 0),
    DIAC_RANGE_ENTRY(0x1cf2, 0x1cf3, 0),
    DIAC_RANGE_ENTRY(0x1dc0, 0x1e00, 0),
    DIAC_RANGE_ENTRY(0x20d0, 0x20f1, 0),
    DIAC_RANGE_ENTRY(0x2cef, 0x2cf2, 0),
    DIAC_RANGE_ENTRY(0x2de0, 0x2e00, 0),
    DIAC_RANGE_ENTRY(0x302a, 0x3030, 0),
    DIAC_RANGE_ENTRY(0x3099, 0x309b, 0),
    DIAC_RANGE_ENTRY(0xa66f, 0xa673, 0),
    DIAC_RANGE_ENTRY(0xa67c, 0xa67e, 0),
    DIAC_RANGE_ENTRY(0xa6f0, 0xa6f2, 0),
    DIAC_RANGE_ENTRY(0xa802, 0xa803, 0),
    DIAC_RANGE_ENTRY(0xa806, 0xa807, 0),
    DIAC_RANGE_ENTRY(0xa80b, 0xa80c, 0),
    DIAC_RANGE_ENTRY(0xa823, 0xa828, 0),
    DIAC_RANGE_ENTRY(0xa880, 0xa882, 0),
    DIAC_RANGE_ENTRY(0xa8b4, 0xa8c5, 0),
    DIAC_RANGE_ENTRY(0xa8e0, 0xa8f2, 0),
    DIAC_RANGE_ENTRY(0xa926, 0xa92e, 0),
    DIAC_RANGE_ENTRY(0xa947, 0xa954, 0),
    DIAC_RANGE_ENTRY(0xa980, 0xa984, 0),
    DIAC_RANGE_ENTRY(0xa9b3, 0xa9c1, 0),
    DIAC_RANGE_ENTRY(0xaa29, 0xaa37, 0),
    DIAC_RANGE_ENTRY(0xaa43, 0xaa44, 0),
    DIAC_RANGE_ENTRY(0xaa4c, 0xaa4e, 0),
    DIAC_RANGE_ENTRY(0xaa7b, 0xaa7c, 0),
    DIAC_RANGE_ENTRY(0xaab0, 0xaab1, 0),
    DIAC_RANGE_ENTRY(0xaab2, 0xaab5, 0),
    DIAC_RANGE_ENTRY(0xaab7, 0xaab9, 0),
    DIAC_RANGE_ENTRY(0xaabe, 0xaac0, 0),
    DIAC_RANGE_ENTRY(0xaac1, 0xaac2, 0),
    DIAC_RANGE_ENTRY(0xabe3, 0xabeb, 0),
    DIAC_RANGE_ENTRY(0xabec, 0xabee, 0),
    DIAC_RANGE_ENTRY(0xfb1e, 0xfb1f, 0),
    DIAC_RANGE_ENTRY(0xfe20, 0xfe27, 0),
    DIAC_RANGE_ENTRY(0xfe70, 0xfe70, 1),
    DIAC_RANGE_ENTRY(0xff00, 0xff00, 0),
    DIAC_RANGE_ENTRY(0xffff, 0xffff, 0),
};

#define MRU_MAX_LEN 32

static unsigned short mru_len = 0;
static unsigned short diacritic_mru[MRU_MAX_LEN];

bool is_diacritic(const unsigned short char_code, bool *is_rtl)
{
    unsigned short mru, i;
    const struct diac_range *diac;

    /* Search in MRU */
    for (mru = 0; mru < mru_len; mru++)
    {
        i = diacritic_mru[mru];

        /* Found in MRU */
        if (diac_ranges[i].base <= char_code &&
                char_code < diac_ranges[i + 1].base)
        {
            goto Found;
        }
    }

    /* Search in DB */
    for (i = 0; i < DIAC_NUM_RANGES - 1; i++)
    {
        /* Found */
        if (char_code < diac_ranges[i + 1].base)
            break;
    }

    /* Add MRU entry, or overwrite LRU if MRU array is full */
    if (mru_len < MRU_MAX_LEN)
        mru_len++;
    else
        mru--;

Found:
    /* Promote MRU item to top of MRU */
    for ( ; mru > 0; mru--)
        diacritic_mru[mru] = diacritic_mru[mru - 1];
    diacritic_mru[0] = i;

    diac = &diac_ranges[i];

    /* Update RTL */
    if (is_rtl)
        *is_rtl = diac->is_rtl;

    return (char_code < diac->base + diac->num_diacritics);
}
