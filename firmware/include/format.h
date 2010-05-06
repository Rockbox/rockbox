/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
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

#ifndef __FORMAT_H__
#define __FORMAT_H__

int format(
    /* call 'push()' for each output letter */
    int (*push)(void *userp, unsigned char data),
    void *userp,
    const char *fmt,
    va_list ap);

/* callback function is called for every output character (byte) with userp and
 * should return 0 when ch is a char other than '\0' that should stop printing */
int vuprintf(int (*push)(void *userp, unsigned char data),
              void *userp, const char *fmt, va_list ap);

#endif /* __FORMAT_H__ */
