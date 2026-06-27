/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 by Gadi Cohen
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
#ifndef BIDI_H
#define BIDI_H

#include <stdbool.h>

/* True for code points that belong to a right-to-left script (Hebrew, Arabic
 * and the Arabic presentation forms). Shared by the bidi engine and by callers
 * that need to know a string's direction. */
static inline bool is_rtl_char(ucschar_t c)
{
   return ((c >= 0x0590 && c <= 0x06ff) /* Hebrew + Arabic */ ||
           (c >= 0x0750 && c <= 0x077f) /* Arabic Supp */ ||
           (c >= 0x0870 && c <= 0x08ff) /* Arabic Ext B+A */ ||
           (c >= 0xfb50 && c <= 0xfdff) /* Arabic Pres A */ ||
           (c >= 0xfe70 && c <= 0xfefc) /* Arabic Pres B */
#ifdef UNICODE32
           || (c >= 0x10e60 && c <= 0x107ef) /* Rumi Numerals */
           || (c >= 0x10ec0 && c <= 0x10eff) /* Arabic Ext C */
           || (c >= 0x1ec70 && c <= 0x1ecbf) /* Indic Siyaq Numerals */
           || (c >= 0x1ed00 && c <= 0x1ed4f) /* Ottoman Siyaq Numerals */
           || (c >= 0x1ee00 && c <= 0x1eeff) /* Arabic Mathenatical Symbols */
#endif
           );
}

/* True if the string's first strongly-directional character is RTL, i.e. the
 * string should be laid out right-to-left. */
bool text_is_rtl(const unsigned char *str);

ucschar_t *bidi_l2v(const unsigned char *str, int orientation);

#endif /* BIDI_H */
