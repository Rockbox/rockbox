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

#define DIAC_FLAG_DIACRITIC (1 << 31)
#define DIAC_FLAG_RTL       (1 << 30)
#define DIAC_MASK           0x000FFFFF

#define DIAC_VAL(i)          (diac_range[(i)] & DIAC_MASK)
#define DIAC_IS_DIACRITIC(i) ((diac_range[(i)] & DIAC_FLAG_DIACRITIC) ? 1 : 0)
#define DIAC_IS_RTL(i)       ((diac_range[(i)] & DIAC_FLAG_RTL) ? 1 : 0)
#define DIAC_NUM_RANGES      (ARRAYLEN(diac_range))

/* Sorted by Unicode value */
static const int diac_range[] =
{
    0x00000,
    0x002ff,
    0x0036f | DIAC_FLAG_DIACRITIC, /* Combining Diacritical Marks */
    0x00482,
    0x00489 | DIAC_FLAG_DIACRITIC, /* Cyrillic */
    0x00590,
    0x005bd | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Hebrew */
    0x005be | DIAC_FLAG_RTL,
    0x005bf | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Hebrew */
    0x005c0 | DIAC_FLAG_RTL,
    0x005c2 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Hebrew */
    0x005c3 | DIAC_FLAG_RTL,
    0x005c5 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Hebrew */
    0x005c6 | DIAC_FLAG_RTL,
    0x005c7 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Hebrew */
    0x0060f | DIAC_FLAG_RTL,
    0x0061a | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Arabic */
    0x0064a | DIAC_FLAG_RTL,
    0x0065e | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Arabic */
    0x0066f | DIAC_FLAG_RTL,
    0x00670 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Arabic */
    0x006d5 | DIAC_FLAG_RTL,
    0x006dc | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Arabic */
    0x006de | DIAC_FLAG_RTL,
    0x006e4 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Arabic */
    0x006e6 | DIAC_FLAG_RTL,
    0x006e8 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Arabic */
    0x006e9 | DIAC_FLAG_RTL,
    0x006ed | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Arabic */
    0x00710 | DIAC_FLAG_RTL,
    0x00711 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Syriac */
    0x0072f | DIAC_FLAG_RTL,
    0x0074a | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Syriac */
    0x007a5 | DIAC_FLAG_RTL,
    0x007b0 | DIAC_FLAG_DIACRITIC | DIAC_FLAG_RTL, /* Thaana */
    0x007c0 | DIAC_FLAG_RTL,
    0x007ea,
    0x007f3 | DIAC_FLAG_DIACRITIC, /* NKo */
    0x00815,
    0x00819 | DIAC_FLAG_DIACRITIC, /* Samaritan */
    0x0081a,
    0x00823 | DIAC_FLAG_DIACRITIC, /* Samaritan */
    0x00824,
    0x00827 | DIAC_FLAG_DIACRITIC, /* Samaritan */
    0x00828,
    0x0082d | DIAC_FLAG_DIACRITIC, /* Samaritan */
    0x008ff,
    0x00903 | DIAC_FLAG_DIACRITIC, /* Devanagari */
    0x0093b,
    0x0093c | DIAC_FLAG_DIACRITIC, /* Devanagari */
    0x0093d,
    0x0094e | DIAC_FLAG_DIACRITIC, /* Devanagari */
    0x00950,
    0x00955 | DIAC_FLAG_DIACRITIC, /* Devanagari */
    0x00961,
    0x00963 | DIAC_FLAG_DIACRITIC, /* Devanagari */
    0x00980,
    0x00983 | DIAC_FLAG_DIACRITIC, /* Bengali */
    0x009bb,
    0x009bc | DIAC_FLAG_DIACRITIC, /* Bengali */
    0x009bd,
    0x009cd | DIAC_FLAG_DIACRITIC, /* Bengali */
    0x009d6,
    0x009d7 | DIAC_FLAG_DIACRITIC, /* Bengali */
    0x009e1,
    0x009e3 | DIAC_FLAG_DIACRITIC, /* Bengali */
    0x00a00,
    0x00a03 | DIAC_FLAG_DIACRITIC, /* Gurmukhi */
    0x00a3b,
    0x00a51 | DIAC_FLAG_DIACRITIC, /* Gurmukhi */
    0x00a6f,
    0x00a71 | DIAC_FLAG_DIACRITIC, /* Gurmukhi */
    0x00a74,
    0x00a75 | DIAC_FLAG_DIACRITIC, /* Gurmukhi */
    0x00a80,
    0x00a83 | DIAC_FLAG_DIACRITIC, /* Gujarati */
    0x00abb,
    0x00abc | DIAC_FLAG_DIACRITIC, /* Gujarati */
    0x00abd,
    0x00acd | DIAC_FLAG_DIACRITIC, /* Gujarati */
    0x00ae1,
    0x00ae3 | DIAC_FLAG_DIACRITIC, /* Gujarati */
    0x00b00,
    0x00b03 | DIAC_FLAG_DIACRITIC, /* Oriya */
    0x00b3b,
    0x00b3c | DIAC_FLAG_DIACRITIC, /* Oriya */
    0x00b3d,
    0x00b57 | DIAC_FLAG_DIACRITIC, /* Oriya */
    0x00b81,
    0x00b82 | DIAC_FLAG_DIACRITIC, /* Tamil */
    0x00bbd,
    0x00bcd | DIAC_FLAG_DIACRITIC, /* Tamil */
    0x00bd6,
    0x00bd7 | DIAC_FLAG_DIACRITIC, /* Tamil */
    0x00c00,
    0x00c03 | DIAC_FLAG_DIACRITIC, /* Telugu */
    0x00c3d,
    0x00c56 | DIAC_FLAG_DIACRITIC, /* Telugu */
    0x00c61,
    0x00c63 | DIAC_FLAG_DIACRITIC, /* Telugu */
    0x00c81,
    0x00c83 | DIAC_FLAG_DIACRITIC, /* Kannada */
    0x00cbb,
    0x00cbc | DIAC_FLAG_DIACRITIC, /* Kannada */
    0x00cbd,
    0x00cd6 | DIAC_FLAG_DIACRITIC, /* Kannada */
    0x00ce1,
    0x00ce3 | DIAC_FLAG_DIACRITIC, /* Kannada */
    0x00d01,
    0x00d03 | DIAC_FLAG_DIACRITIC, /* Malayalam */
    0x00d3d,
    0x00d57 | DIAC_FLAG_DIACRITIC, /* Malayalam */
    0x00d61,
    0x00d63 | DIAC_FLAG_DIACRITIC, /* Malayalam */
    0x00d81,
    0x00d83 | DIAC_FLAG_DIACRITIC, /* Sinhala */
    0x00dc9,
    0x00df3 | DIAC_FLAG_DIACRITIC, /* Sinhala */
    0x00e30,
    0x00e31 | DIAC_FLAG_DIACRITIC, /* Thai */
    0x00e33,
    0x00e3a | DIAC_FLAG_DIACRITIC, /* Thai */
    0x00e46,
    0x00e4e | DIAC_FLAG_DIACRITIC, /* Thai */
    0x00eb0,
    0x00eb1 | DIAC_FLAG_DIACRITIC, /* Lao */
    0x00eb3,
    0x00ebc | DIAC_FLAG_DIACRITIC, /* Lao */
    0x00ec7,
    0x00ecd | DIAC_FLAG_DIACRITIC, /* Lao */
    0x00f17,
    0x00f19 | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x00f34,
    0x00f35 | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x00f36,
    0x00f37 | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x00f38,
    0x00f39 | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x00f3d,
    0x00f3f | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x00f70,
    0x00f84 | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x00f85,
    0x00f87 | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x00f8f,
    0x00fbc | DIAC_FLAG_DIACRITIC, /* Tibetan */
    0x0102a,
    0x0103e | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x01055,
    0x01059 | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x0105d,
    0x01060 | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x01061,
    0x01064 | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x01066,
    0x0106d | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x01070,
    0x01074 | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x01081,
    0x0108d | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x0108e,
    0x0108f | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x01099,
    0x0109d | DIAC_FLAG_DIACRITIC, /* Myanmar */
    0x0135e,
    0x0135f | DIAC_FLAG_DIACRITIC, /* Ethiopic */
    0x01711,
    0x01714 | DIAC_FLAG_DIACRITIC, /* Tagalog */
    0x01731,
    0x01734 | DIAC_FLAG_DIACRITIC, /* Hanunoo */
    0x01751,
    0x01753 | DIAC_FLAG_DIACRITIC, /* Buhid */
    0x01771,
    0x01773 | DIAC_FLAG_DIACRITIC, /* Tagbanwa */
    0x017b5,
    0x017d3 | DIAC_FLAG_DIACRITIC, /* Khmer */
    0x017dc,
    0x017dd | DIAC_FLAG_DIACRITIC, /* Khmer */
    0x018a8,
    0x018a9 | DIAC_FLAG_DIACRITIC, /* Mongolian */
    0x0191f,
    0x0193b | DIAC_FLAG_DIACRITIC, /* Limbu */
    0x019af,
    0x019c0 | DIAC_FLAG_DIACRITIC, /* New Tai Lue */
    0x019c7,
    0x019c9 | DIAC_FLAG_DIACRITIC, /* New Tai Lue */
    0x01a16,
    0x01a1b | DIAC_FLAG_DIACRITIC, /* Buginese */
    0x01a54,
    0x01a7f | DIAC_FLAG_DIACRITIC, /* Tai Tham */
    0x01aff,
    0x01b04 | DIAC_FLAG_DIACRITIC, /* Balinese */
    0x01b33,
    0x01b44 | DIAC_FLAG_DIACRITIC, /* Balinese */
    0x01b6a,
    0x01b73 | DIAC_FLAG_DIACRITIC, /* Balinese */
    0x01b7f,
    0x01b82 | DIAC_FLAG_DIACRITIC, /* Sundanese */
    0x01ba0,
    0x01baa | DIAC_FLAG_DIACRITIC, /* Sundanese */
    0x01c23,
    0x01c37 | DIAC_FLAG_DIACRITIC, /* Lepcha */
    0x01ccf,
    0x01cd2 | DIAC_FLAG_DIACRITIC, /* Vedic Extensions */
    0x01cd3,
    0x01ce8 | DIAC_FLAG_DIACRITIC, /* Vedic Extensions */
    0x01cec,
    0x01ced | DIAC_FLAG_DIACRITIC, /* Vedic Extensions */
    0x01cf1,
    0x01cf2 | DIAC_FLAG_DIACRITIC, /* Vedic Extensions */
    0x01dbf,
    0x01dff | DIAC_FLAG_DIACRITIC, /* Combining Diacritical Marks Supplement */
    0x020cf,
    0x020f0 | DIAC_FLAG_DIACRITIC, /* Combining Diacritical Marks for Symbols */
    0x02cee,
    0x02cf1 | DIAC_FLAG_DIACRITIC, /* Coptic */
    0x02ddf,
    0x02dff | DIAC_FLAG_DIACRITIC, /* Coptic */
    0x03029,
    0x0302f | DIAC_FLAG_DIACRITIC, /* CJK Symbols and Punctuation */
    0x03098,
    0x0309a | DIAC_FLAG_DIACRITIC, /* Hiragana */
    0x0a66e,
    0x0a672 | DIAC_FLAG_DIACRITIC, /* Hiragana */
    0x0a67b,
    0x0a67d | DIAC_FLAG_DIACRITIC, /* Hiragana */
    0x0a6ef,
    0x0a6f1 | DIAC_FLAG_DIACRITIC, /* Bamum */
    0x0a801,
    0x0a802 | DIAC_FLAG_DIACRITIC, /* Syloti Nagri */
    0x0a805,
    0x0a806 | DIAC_FLAG_DIACRITIC, /* Syloti Nagri */
    0x0a80a,
    0x0a80b | DIAC_FLAG_DIACRITIC, /* Syloti Nagri */
    0x0a822,
    0x0a827 | DIAC_FLAG_DIACRITIC, /* Syloti Nagri */
    0x0a87f,
    0x0a881 | DIAC_FLAG_DIACRITIC, /* Saurashtra */
    0x0a8b3,
    0x0a8c4 | DIAC_FLAG_DIACRITIC, /* Saurashtra */
    0x0a8df,
    0x0a8f1 | DIAC_FLAG_DIACRITIC, /* Devanagari Extended */
    0x0a925,
    0x0a92d | DIAC_FLAG_DIACRITIC, /* Kayah Li */
    0x0a946,
    0x0a953 | DIAC_FLAG_DIACRITIC, /* Rejang */
    0x0a97f,
    0x0a983 | DIAC_FLAG_DIACRITIC, /* Javanese */
    0x0a9b2,
    0x0a9c0 | DIAC_FLAG_DIACRITIC, /* Javanese */
    0x0aa28,
    0x0aa36 | DIAC_FLAG_DIACRITIC, /* Cham */
    0x0aa42,
    0x0aa43 | DIAC_FLAG_DIACRITIC, /* Cham */
    0x0aa4b,
    0x0aa4d | DIAC_FLAG_DIACRITIC, /* Cham */
    0x0aa7a,
    0x0aa7b | DIAC_FLAG_DIACRITIC, /* Cham */
    0x0aaaf,
    0x0aab0 | DIAC_FLAG_DIACRITIC, /* Tai Viet */
    0x0aab1,
    0x0aab4 | DIAC_FLAG_DIACRITIC, /* Tai Viet */
    0x0aab6,
    0x0aab8 | DIAC_FLAG_DIACRITIC, /* Tai Viet */
    0x0aabd,
    0x0aabf | DIAC_FLAG_DIACRITIC, /* Tai Viet */
    0x0aac0,
    0x0aac1 | DIAC_FLAG_DIACRITIC, /* Tai Viet */
    0x0abe2,
    0x0abea | DIAC_FLAG_DIACRITIC, /* Meetei Mayek */
    0x0abeb,
    0x0abed | DIAC_FLAG_DIACRITIC, /* Meetei Mayek */
    0x0fb1d,
    0x0fb1e | DIAC_FLAG_DIACRITIC, /* Alphabetic Presentation Forms */
    0x0fe1f,
    0x0fe26 | DIAC_FLAG_DIACRITIC, /* Combining Half Marks */
    0x1107f,
    0x11082 | DIAC_FLAG_DIACRITIC, /* Kaithi */
    0x110af,
    0x110ba | DIAC_FLAG_DIACRITIC, /* Kaithi */
    0x1d164,
    0x1d169 | DIAC_FLAG_DIACRITIC, /* Musical Symbols */
    0x1d16c,
    0x1d182 | DIAC_FLAG_DIACRITIC, /* Musical Symbols */
    0x1d184,
    0x1d18b | DIAC_FLAG_DIACRITIC, /* Musical Symbols */
    0x1d1a9,
    0x1d1ad | DIAC_FLAG_DIACRITIC, /* Musical Symbols */
    0x1d241,
    0x1d244 | DIAC_FLAG_DIACRITIC, /* Ancient Greek Musical Notation */
    0xe01ef,
};

#define MRU_MAX_LEN 32

static unsigned short mru_len = 0;
static unsigned short diacritic_mru[MRU_MAX_LEN];

int is_diacritic(unsigned short char_code, bool *is_rtl)
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
