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

/*large memory devices spin up the disk much less often*/
#if MEMORYSIZE > 32
 #define MAX_LOGDISKF_SIZE 8192
#elif MEMORYSIZE > 8
 #define MAX_LOGDISKF_SIZE 4096
#else
 #define MAX_LOGDISKF_SIZE 1024
#endif

extern unsigned char logdiskfbuffer[MAX_LOGDISKF_SIZE];
extern int logfdiskindex;

#define LOGDISK_LEVEL 1

#if LOGDISK_LEVEL > 0       /*serious errors or problems*/
 #define ERRORF(...) _logdiskf(__func__,'E', __VA_ARGS__)
#else
 #define ERRORF(...)
#endif

#if LOGDISK_LEVEL > 1       /*matters of concern*/
 #define WARNF(...) _logdiskf(__func__, 'W', __VA_ARGS__)
#else
 #define WARNF(...)
#endif

#if LOGDISK_LEVEL > 2       /*useful for debug only*/
 #define NOTEF(...) _logdiskf(__func__, 'N', __VA_ARGS__)
#else
 #define NOTEF(...)     /*TODO:  rename DEBUGF later*/
#endif

/*__FILE__ would be better but is difficult to make work with the build system*/
//#define logdiskf(...) _logdiskf(__func__, 1, __VA_ARGS__)

void _logdiskf(const char* file, const char level,
                const char *format, ...) ATTRIBUTE_PRINTF(3, 4);

#else /* !ROCKBOX_HAS_LOGDISKF */

/* built without logdiskf() support enabled, replace logdiskf() by DEBUGF() */
#define ERRORF DEBUGF
#define WARNF DEBUGF
#define NOTEF DEBUGF

#endif
#endif /* LOGDISKF_H */
    
    
