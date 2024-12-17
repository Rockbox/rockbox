/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 William Wilgus
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
#include "plugin.h"
#include "kbd_helper.h"

/*  USAGE:
    ucschar_t kbd[64];
    ucschar_t *kbd_p = kbd;
    if (!kbd_create_layout("ABCD1234\n", kbd, sizeof(kbd)))
        kbd_p = NULL;

    rb->kbd_input(buf,sizeof(buf), kbd_p);
*/

/* create a custom keyboard layout for kbd_input
 * success returns size of buffer used
 * failure returns 0
*/
int kbd_create_layout(const char *layout, ucschar_t *buf, int bufsz)
{
    ucschar_t *pbuf;
    const unsigned char *p = layout;
    int len = 0;
    int total_len = 0;
    pbuf = buf;
    while (*p && (pbuf - buf + (ptrdiff_t) sizeof(ucschar_t)) < bufsz)
    {
        p = rb->utf8decode(p, &pbuf[len+1]);
        if (pbuf[len+1] == '\n')
        {
            *pbuf = len;
            pbuf += len+1;
            total_len += len + 1;
            len = 0;
        }
        else
            len++;
    }

    if (len+1 < bufsz)
    {
        *pbuf = len;
        pbuf[len+1] = 0xFEFF;   /* mark end of characters */
        total_len += len + 1;
        return total_len * sizeof(ucschar_t);
    }

    return 0;
}
