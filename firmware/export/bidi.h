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
#define is_rtl_char(c) (((c) > 0x0589 && (c) < 0x0700) || \
                        ((c) >= 0xfb50 && (c) <= 0xfefc))

/* True if the string's first strongly-directional character is RTL, i.e. the
 * string should be laid out right-to-left. */
bool text_is_rtl(const unsigned char *str);

ucschar_t *bidi_l2v(const unsigned char *str, int orientation);

#endif /* BIDI_H */
