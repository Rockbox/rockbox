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

#ifndef __SPRINTF_H__
#define __SPRINTF_H__

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

int snprintf (char *buf, size_t size, const char *fmt, ...);
char *strtok_r (char *, const char *, char **);

int rockbox_fprintf (int fd, const char *fmt, ...);
#define fprintf rockbox_fprintf

int rockbox_vsnprintf (char *buf, int size, const char *fmt, va_list ap);
#define vsnprintf rockbox_vsnprintf

#endif /* __SPRINTF_H__ */
