/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Michael Giacomelli
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
#ifndef LOGDISKF_H
#define LOGDISKF_H
#include <config.h>
#include <stdbool.h>
#include "gcc_extensions.h"
#include "debug.h"

#ifdef ROCKBOX_HAS_LOGDISKF

void init_logdiskf(void);

#ifndef __PCTOOL__

#define MAX_LOGDISKF_SIZE 2048

extern unsigned char logdiskfbuffer[MAX_LOGDISKF_SIZE];
extern int logfdiskindex;
#endif /* __PCTOOL__ */

#define LOGDISKF_LEVEL   0x01  /*releases should only log serious errors*/

#define LOG_ERROR        0x01  /*serious error*/
#define LOG_WARNING      0x02  /*matter of concern*/
#define LOG_DEBUG        0x03  /*useful for debug only*/

/*__FILE__ would be better but is difficult to make work with the build system*/
#define logdiskf(...) _logdiskf(__func__, 1, __VA_ARGS__)

void _logdiskf(const char* file, const int level,
               const char *format, ...) ATTRIBUTE_PRINTF(3, 4);

#else /* !ROCKBOX_HAS_LOGF */

/* built without logdiskf() support enabled, replace logdiskf() by DEBUGF() */
#define logdiskf(f,args...) DEBUGF(f"\n",##args)

#endif /* !ROCKBOX_HAS_LOGDISKF */

#endif /* LOGDISKF_H */
