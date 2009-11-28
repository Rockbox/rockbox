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

#define DIAC_FLAG_NONE      0
#define DIAC_FLAG_DIACRITIC (1 << 0)
#define DIAC_FLAG_RTL       (1 << 1)

#define DIAC_VAL(i)          (diac_ranges[(i)].last)
#define DIAC_IS_DIACRITIC(i) (diac_ranges[(i)].flags & DIAC_FLAG_DIACRITIC)
#define DIAC_IS_RTL(i)       (diac_ranges[(i)].flags & DIAC_FLAG_RTL)
#define DIAC_NUM_RANGES      (ARRAYLEN(diac_ranges))

struct diac_range
{
    unsigned short last;
    unsigned char flags;
};

/* Sorted by Unicode value */
static const struct diac_range diac_ranges[] =
{
    { 0x0000, DIAC_FLAG_NONE },
    { 0x02ff, DIAC_FLAG_NONE },
    { 0x036f, DIAC_FLAG_DIACRITIC }, /* Combining Diacritical Marks */
    { 0x0482, DIAC_FLAG_NONE },
    { 0x0489, DIAC_FLAG_DIACRITIC }, /* Cyrillic */
    { 0x0590, DIAC_FLAG_NONE },
    { 0x05bd, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Hebrew */
    { 0x05be, DIAC_FLAG_RTL },
    { 0x05bf, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Hebrew */
    { 0x05c0, DIAC_FLAG_RTL },
    { 0x05c2, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Hebrew */
    { 0x05c3, DIAC_FLAG_RTL },
    { 0x05c5, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Hebrew */
    { 0x05c6, DIAC_FLAG_RTL },
    { 0x05c7, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Hebrew */
    { 0x060f, DIAC_FLAG_RTL },
    { 0x061a, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Arabic */
    { 0x064a, DIAC_FLAG_RTL },
    { 0x065e, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Arabic */
    { 0x066f, DIAC_FLAG_RTL },
    { 0x0670, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Arabic */
    { 0x06d5, DIAC_FLAG_RTL },
    { 0x06dc, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Arabic */
    { 0x06de, DIAC_FLAG_RTL },
    { 0x06e4, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Arabic */
    { 0x06e6, DIAC_FLAG_RTL },
    { 0x06e8, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Arabic */
    { 0x06e9, DIAC_FLAG_RTL },
    { 0x06ed, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Arabic */
    { 0x0710, DIAC_FLAG_RTL },
    { 0x0711, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Syriac */
    { 0x072f, DIAC_FLAG_RTL },
    { 0x074a, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Syriac */
    { 0x07a5, DIAC_FLAG_RTL },
    { 0x07b0, DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL }, /* Thaana */
    { 0x07c0, DIAC_FLAG_RTL },
    { 0x07ea, DIAC_FLAG_NONE },
    { 0x07f3, DIAC_FLAG_DIACRITIC }, /* NKo */
    { 0x0815, DIAC_FLAG_NONE },
    { 0x0819, DIAC_FLAG_DIACRITIC }, /* Samaritan */
    { 0x081a, DIAC_FLAG_NONE },
    { 0x0823, DIAC_FLAG_DIACRITIC }, /* Samaritan */
    { 0x0824, DIAC_FLAG_NONE },
    { 0x0827, DIAC_FLAG_DIACRITIC }, /* Samaritan */
    { 0x0828, DIAC_FLAG_NONE },
    { 0x082d, DIAC_FLAG_DIACRITIC }, /* Samaritan */
    { 0x08ff, DIAC_FLAG_NONE },
    { 0x0903, DIAC_FLAG_DIACRITIC }, /* Devanagari */
    { 0x093b, DIAC_FLAG_NONE },
    { 0x093c, DIAC_FLAG_DIACRITIC }, /* Devanagari */
    { 0x093d, DIAC_FLAG_NONE },
    { 0x094e, DIAC_FLAG_DIACRITIC }, /* Devanagari */
    { 0x0950, DIAC_FLAG_NONE },
    { 0x0955, DIAC_FLAG_DIACRITIC }, /* Devanagari */
    { 0x0961, DIAC_FLAG_NONE },
    { 0x0963, DIAC_FLAG_DIACRITIC }, /* Devanagari */
    { 0x0980, DIAC_FLAG_NONE },
    { 0x0983, DIAC_FLAG_DIACRITIC }, /* Bengali */
    { 0x09bb, DIAC_FLAG_NONE },
    { 0x09bc, DIAC_FLAG_DIACRITIC }, /* Bengali */
    { 0x09bd, DIAC_FLAG_NONE },
    { 0x09cd, DIAC_FLAG_DIACRITIC }, /* Bengali */
    { 0x09d6, DIAC_FLAG_NONE },
    { 0x09d7, DIAC_FLAG_DIACRITIC }, /* Bengali */
    { 0x09e1, DIAC_FLAG_NONE },
    { 0x09e3, DIAC_FLAG_DIACRITIC }, /* Bengali */
    { 0x0a00, DIAC_FLAG_NONE },
    { 0x0a03, DIAC_FLAG_DIACRITIC }, /* Gurmukhi */
    { 0x0a3b, DIAC_FLAG_NONE },
    { 0x0a51, DIAC_FLAG_DIACRITIC }, /* Gurmukhi */
    { 0x0a6f, DIAC_FLAG_NONE },
    { 0x0a71, DIAC_FLAG_DIACRITIC }, /* Gurmukhi */
    { 0x0a74, DIAC_FLAG_NONE },
    { 0x0a75, DIAC_FLAG_DIACRITIC }, /* Gurmukhi */
    { 0x0a80, DIAC_FLAG_NONE },
    { 0x0a83, DIAC_FLAG_DIACRITIC }, /* Gujarati */
    { 0x0abb, DIAC_FLAG_NONE },
    { 0x0abc, DIAC_FLAG_DIACRITIC }, /* Gujarati */
    { 0x0abd, DIAC_FLAG_NONE },
    { 0x0acd, DIAC_FLAG_DIACRITIC }, /* Gujarati */
    { 0x0ae1, DIAC_FLAG_NONE },
    { 0x0ae3, DIAC_FLAG_DIACRITIC }, /* Gujarati */
    { 0x0b00, DIAC_FLAG_NONE },
    { 0x0b03, DIAC_FLAG_DIACRITIC }, /* Oriya */
    { 0x0b3b, DIAC_FLAG_NONE },
    { 0x0b3c, DIAC_FLAG_DIACRITIC }, /* Oriya */
    { 0x0b3d, DIAC_FLAG_NONE },
    { 0x0b57, DIAC_FLAG_DIACRITIC }, /* Oriya */
    { 0x0b81, DIAC_FLAG_NONE },
    { 0x0b82, DIAC_FLAG_DIACRITIC }, /* Tamil */
    { 0x0bbd, DIAC_FLAG_NONE },
    { 0x0bcd, DIAC_FLAG_DIACRITIC }, /* Tamil */
    { 0x0bd6, DIAC_FLAG_NONE },
    { 0x0bd7, DIAC_FLAG_DIACRITIC }, /* Tamil */
    { 0x0c00, DIAC_FLAG_NONE },
    { 0x0c03, DIAC_FLAG_DIACRITIC }, /* Telugu */
    { 0x0c3d, DIAC_FLAG_NONE },
    { 0x0c56, DIAC_FLAG_DIACRITIC }, /* Telugu */
    { 0x0c61, DIAC_FLAG_NONE },
    { 0x0c63, DIAC_FLAG_DIACRITIC }, /* Telugu */
    { 0x0c81, DIAC_FLAG_NONE },
    { 0x0c83, DIAC_FLAG_DIACRITIC }, /* Kannada */
    { 0x0cbb, DIAC_FLAG_NONE },
    { 0x0cbc, DIAC_FLAG_DIACRITIC }, /* Kannada */
    { 0x0cbd, DIAC_FLAG_NONE },
    { 0x0cd6, DIAC_FLAG_DIACRITIC }, /* Kannada */
    { 0x0ce1, DIAC_FLAG_NONE },
    { 0x0ce3, DIAC_FLAG_DIACRITIC }, /* Kannada */
    { 0x0d01, DIAC_FLAG_NONE },
    { 0x0d03, DIAC_FLAG_DIACRITIC }, /* Malayalam */
    { 0x0d3d, DIAC_FLAG_NONE },
    { 0x0d57, DIAC_FLAG_DIACRITIC }, /* Malayalam */
    { 0x0d61, DIAC_FLAG_NONE },
    { 0x0d63, DIAC_FLAG_DIACRITIC }, /* Malayalam */
    { 0x0d81, DIAC_FLAG_NONE },
    { 0x0d83, DIAC_FLAG_DIACRITIC }, /* Sinhala */
    { 0x0dc9, DIAC_FLAG_NONE },
    { 0x0df3, DIAC_FLAG_DIACRITIC }, /* Sinhala */
    { 0x0e30, DIAC_FLAG_NONE },
    { 0x0e31, DIAC_FLAG_DIACRITIC }, /* Thai */
    { 0x0e33, DIAC_FLAG_NONE },
    { 0x0e3a, DIAC_FLAG_DIACRITIC }, /* Thai */
    { 0x0e46, DIAC_FLAG_NONE },
    { 0x0e4e, DIAC_FLAG_DIACRITIC }, /* Thai */
    { 0x0eb0, DIAC_FLAG_NONE },
    { 0x0eb1, DIAC_FLAG_DIACRITIC }, /* Lao */
    { 0x0eb3, DIAC_FLAG_NONE },
    { 0x0ebc, DIAC_FLAG_DIACRITIC }, /* Lao */
    { 0x0ec7, DIAC_FLAG_NONE },
    { 0x0ecd, DIAC_FLAG_DIACRITIC }, /* Lao */
    { 0x0f17, DIAC_FLAG_NONE },
    { 0x0f19, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x0f34, DIAC_FLAG_NONE },
    { 0x0f35, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x0f36, DIAC_FLAG_NONE },
    { 0x0f37, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x0f38, DIAC_FLAG_NONE },
    { 0x0f39, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x0f3d, DIAC_FLAG_NONE },
    { 0x0f3f, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x0f70, DIAC_FLAG_NONE },
    { 0x0f84, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x0f85, DIAC_FLAG_NONE },
    { 0x0f87, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x0f8f, DIAC_FLAG_NONE },
    { 0x0fbc, DIAC_FLAG_DIACRITIC }, /* Tibetan */
    { 0x102a, DIAC_FLAG_NONE },
    { 0x103e, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x1055, DIAC_FLAG_NONE },
    { 0x1059, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x105d, DIAC_FLAG_NONE },
    { 0x1060, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x1061, DIAC_FLAG_NONE },
    { 0x1064, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x1066, DIAC_FLAG_NONE },
    { 0x106d, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x1070, DIAC_FLAG_NONE },
    { 0x1074, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x1081, DIAC_FLAG_NONE },
    { 0x108d, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x108e, DIAC_FLAG_NONE },
    { 0x108f, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x1099, DIAC_FLAG_NONE },
    { 0x109d, DIAC_FLAG_DIACRITIC }, /* Myanmar */
    { 0x135e, DIAC_FLAG_NONE },
    { 0x135f, DIAC_FLAG_DIACRITIC }, /* Ethiopic */
    { 0x1711, DIAC_FLAG_NONE },
    { 0x1714, DIAC_FLAG_DIACRITIC }, /* Tagalog */
    { 0x1731, DIAC_FLAG_NONE },
    { 0x1734, DIAC_FLAG_DIACRITIC }, /* Hanunoo */
    { 0x1751, DIAC_FLAG_NONE },
    { 0x1753, DIAC_FLAG_DIACRITIC }, /* Buhid */
    { 0x1771, DIAC_FLAG_NONE },
    { 0x1773, DIAC_FLAG_DIACRITIC }, /* Tagbanwa */
    { 0x17b5, DIAC_FLAG_NONE },
    { 0x17d3, DIAC_FLAG_DIACRITIC }, /* Khmer */
    { 0x17dc, DIAC_FLAG_NONE },
    { 0x17dd, DIAC_FLAG_DIACRITIC }, /* Khmer */
    { 0x18a8, DIAC_FLAG_NONE },
    { 0x18a9, DIAC_FLAG_DIACRITIC }, /* Mongolian */
    { 0x191f, DIAC_FLAG_NONE },
    { 0x193b, DIAC_FLAG_DIACRITIC }, /* Limbu */
    { 0x19af, DIAC_FLAG_NONE },
    { 0x19c0, DIAC_FLAG_DIACRITIC }, /* New Tai Lue */
    { 0x19c7, DIAC_FLAG_NONE },
    { 0x19c9, DIAC_FLAG_DIACRITIC }, /* New Tai Lue */
    { 0x1a16, DIAC_FLAG_NONE },
    { 0x1a1b, DIAC_FLAG_DIACRITIC }, /* Buginese */
    { 0x1a54, DIAC_FLAG_NONE },
    { 0x1a7f, DIAC_FLAG_DIACRITIC }, /* Tai Tham */
    { 0x1aff, DIAC_FLAG_NONE },
    { 0x1b04, DIAC_FLAG_DIACRITIC }, /* Balinese */
    { 0x1b33, DIAC_FLAG_NONE },
    { 0x1b44, DIAC_FLAG_DIACRITIC }, /* Balinese */
    { 0x1b6a, DIAC_FLAG_NONE },
    { 0x1b73, DIAC_FLAG_DIACRITIC }, /* Balinese */
    { 0x1b7f, DIAC_FLAG_NONE },
    { 0x1b82, DIAC_FLAG_DIACRITIC }, /* Sundanese */
    { 0x1ba0, DIAC_FLAG_NONE },
    { 0x1baa, DIAC_FLAG_DIACRITIC }, /* Sundanese */
    { 0x1c23, DIAC_FLAG_NONE },
    { 0x1c37, DIAC_FLAG_DIACRITIC }, /* Lepcha */
    { 0x1ccf, DIAC_FLAG_NONE },
    { 0x1cd2, DIAC_FLAG_DIACRITIC }, /* Vedic Extensions */
    { 0x1cd3, DIAC_FLAG_NONE },
    { 0x1ce8, DIAC_FLAG_DIACRITIC }, /* Vedic Extensions */
    { 0x1cec, DIAC_FLAG_NONE },
    { 0x1ced, DIAC_FLAG_DIACRITIC }, /* Vedic Extensions */
    { 0x1cf1, DIAC_FLAG_NONE },
    { 0x1cf2, DIAC_FLAG_DIACRITIC }, /* Vedic Extensions */
    { 0x1dbf, DIAC_FLAG_NONE },
    { 0x1dff, DIAC_FLAG_DIACRITIC }, /* Combining Diacritical Marks Supplement */
    { 0x20cf, DIAC_FLAG_NONE },
    { 0x20f0, DIAC_FLAG_DIACRITIC }, /* Combining Diacritical Marks for Symbols */
    { 0x2cee, DIAC_FLAG_NONE },
    { 0x2cf1, DIAC_FLAG_DIACRITIC }, /* Coptic */
    { 0x2ddf, DIAC_FLAG_NONE },
    { 0x2dff, DIAC_FLAG_DIACRITIC }, /* Coptic */
    { 0x3029, DIAC_FLAG_NONE },
    { 0x302f, DIAC_FLAG_DIACRITIC }, /* CJK Symbols and Punctuation */
    { 0x3098, DIAC_FLAG_NONE },
    { 0x309a, DIAC_FLAG_DIACRITIC }, /* Hiragana */
    { 0xa66e, DIAC_FLAG_NONE },
    { 0xa672, DIAC_FLAG_DIACRITIC }, /* Hiragana */
    { 0xa67b, DIAC_FLAG_NONE },
    { 0xa67d, DIAC_FLAG_DIACRITIC }, /* Hiragana */
    { 0xa6ef, DIAC_FLAG_NONE },
    { 0xa6f1, DIAC_FLAG_DIACRITIC }, /* Bamum */
    { 0xa801, DIAC_FLAG_NONE },
    { 0xa802, DIAC_FLAG_DIACRITIC }, /* Syloti Nagri */
    { 0xa805, DIAC_FLAG_NONE },
    { 0xa806, DIAC_FLAG_DIACRITIC }, /* Syloti Nagri */
    { 0xa80a, DIAC_FLAG_NONE },
    { 0xa80b, DIAC_FLAG_DIACRITIC }, /* Syloti Nagri */
    { 0xa822, DIAC_FLAG_NONE },
    { 0xa827, DIAC_FLAG_DIACRITIC }, /* Syloti Nagri */
    { 0xa87f, DIAC_FLAG_NONE },
    { 0xa881, DIAC_FLAG_DIACRITIC }, /* Saurashtra */
    { 0xa8b3, DIAC_FLAG_NONE },
    { 0xa8c4, DIAC_FLAG_DIACRITIC }, /* Saurashtra */
    { 0xa8df, DIAC_FLAG_NONE },
    { 0xa8f1, DIAC_FLAG_DIACRITIC }, /* Devanagari Extended */
    { 0xa925, DIAC_FLAG_NONE },
    { 0xa92d, DIAC_FLAG_DIACRITIC }, /* Kayah Li */
    { 0xa946, DIAC_FLAG_NONE },
    { 0xa953, DIAC_FLAG_DIACRITIC }, /* Rejang */
    { 0xa97f, DIAC_FLAG_NONE },
    { 0xa983, DIAC_FLAG_DIACRITIC }, /* Javanese */
    { 0xa9b2, DIAC_FLAG_NONE },
    { 0xa9c0, DIAC_FLAG_DIACRITIC }, /* Javanese */
    { 0xaa28, DIAC_FLAG_NONE },
    { 0xaa36, DIAC_FLAG_DIACRITIC }, /* Cham */
    { 0xaa42, DIAC_FLAG_NONE },
    { 0xaa43, DIAC_FLAG_DIACRITIC }, /* Cham */
    { 0xaa4b, DIAC_FLAG_NONE },
    { 0xaa4d, DIAC_FLAG_DIACRITIC }, /* Cham */
    { 0xaa7a, DIAC_FLAG_NONE },
    { 0xaa7b, DIAC_FLAG_DIACRITIC }, /* Cham */
    { 0xaaaf, DIAC_FLAG_NONE },
    { 0xaab0, DIAC_FLAG_DIACRITIC }, /* Tai Viet */
    { 0xaab1, DIAC_FLAG_NONE },
    { 0xaab4, DIAC_FLAG_DIACRITIC }, /* Tai Viet */
    { 0xaab6, DIAC_FLAG_NONE },
    { 0xaab8, DIAC_FLAG_DIACRITIC }, /* Tai Viet */
    { 0xaabd, DIAC_FLAG_NONE },
    { 0xaabf, DIAC_FLAG_DIACRITIC }, /* Tai Viet */
    { 0xaac0, DIAC_FLAG_NONE },
    { 0xaac1, DIAC_FLAG_DIACRITIC }, /* Tai Viet */
    { 0xabe2, DIAC_FLAG_NONE },
    { 0xabea, DIAC_FLAG_DIACRITIC }, /* Meetei Mayek */
    { 0xabeb, DIAC_FLAG_NONE },
    { 0xabed, DIAC_FLAG_DIACRITIC }, /* Meetei Mayek */
    { 0xfb1d, DIAC_FLAG_NONE },
    { 0xfb1e, DIAC_FLAG_DIACRITIC }, /* Alphabetic Presentation Forms */
    { 0xfe1f, DIAC_FLAG_NONE },
    { 0xfe26, DIAC_FLAG_DIACRITIC }, /* Combining Half Marks */
    /* Currently we don't support chars above U-FFFF */
    { 0xffff, DIAC_FLAG_NONE },
#if 0
    { 0x1107f, DIAC_FLAG_NONE },
    { 0x11082, DIAC_FLAG_DIACRITIC }, /* Kaithi */
    { 0x110af, DIAC_FLAG_NONE },
    { 0x110ba, DIAC_FLAG_DIACRITIC }, /* Kaithi */
    { 0x1d164, DIAC_FLAG_NONE },
    { 0x1d169, DIAC_FLAG_DIACRITIC }, /* Musical Symbols */
    { 0x1d16c, DIAC_FLAG_NONE },
    { 0x1d182, DIAC_FLAG_DIACRITIC }, /* Musical Symbols */
    { 0x1d184, DIAC_FLAG_NONE },
    { 0x1d18b, DIAC_FLAG_DIACRITIC }, /* Musical Symbols */
    { 0x1d1a9, DIAC_FLAG_NONE },
    { 0x1d1ad, DIAC_FLAG_DIACRITIC }, /* Musical Symbols */
    { 0x1d241, DIAC_FLAG_NONE },
    { 0x1d244, DIAC_FLAG_DIACRITIC }, /* Ancient Greek Musical Notation */
    { 0xe01ef, DIAC_FLAG_NONE },
#endif
};

#define MRU_MAX_LEN 32

static unsigned short mru_len = 0;
static unsigned short diacritic_mru[MRU_MAX_LEN];

int is_diacritic(const unsigned short char_code, bool *is_rtl)
{
    unsigned short mru, i;

    /* Search in MRU */
    for (mru = 0; mru < mru_len; mru++)
    {
        i = diacritic_mru[mru];

        /* Found in MRU */
        if (DIAC_VAL(i - 1) < char_code && char_code <= DIAC_VAL(i))
            goto Found;
    }

    /* Search in DB */
    for (i = 1; i < DIAC_NUM_RANGES; i++)
    {
        /* Found */
        if (char_code <= DIAC_VAL(i))
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

    /* Update RTL */
    if (is_rtl)
        *is_rtl = DIAC_IS_RTL(i);

    return DIAC_IS_DIACRITIC(i);
}
