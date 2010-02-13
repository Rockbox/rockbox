
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
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

#ifndef _ROCKLIBC_H_
#define _ROCKLIBC_H_

#include <ctype.h>
#include "plugin.h"

#ifdef SIMULATOR
#include <errno.h>
#define PREFIX(_x_) sim_ ## _x_
#else
extern int errno;
#define EINVAL          22      /* Invalid argument */
#define ERANGE          34      /* Math result not representable */
#define PREFIX
#endif

#define __likely   LIKELY
#define __unlikely UNLIKELY

/* Simple substitutions */
#define memcmp rb->memcmp
#define strlen rb->strlen

extern int PREFIX(fscanf)(int fd, const char *fmt, ...);

#endif /* _ROCKLIBC_H_ */

