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

#ifndef __VUPRINTF_H__
#define __VUPRINTF_H__

#include <stdarg.h>

/* callback function is called for every output character (byte) in the
 * output with userp
 *
 * it must return > 0 to continue (increments counter)
 * it may return 0 to stop (increments counter)
 * it may return < 0 to stop (does not increment counter)
 * a zero in the format string stops (does not increment counter)
 *
 * caller is reponsible for stopping formatting in order to keep the return
 * value from overflowing (assuming they have a reason to care)
 */
typedef int (* vuprintf_push_cb)(void *userp, int c);

/*
 * returns the number of times push() was called and returned >= 0
 */
int vuprintf(vuprintf_push_cb push, void *userp, const char *fmt, va_list ap);

#endif /* __VUPRINTF_H__ */
