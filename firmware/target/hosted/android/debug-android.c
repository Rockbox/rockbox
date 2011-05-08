/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Thomas Martitz
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



#include "config.h"
#ifdef DEBUG
#include <android/log.h>
#include <stdarg.h>
#include <stdio.h>

#define LOG_TAG "Rockbox"

void debug_init(void) {}

void debugf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ap);
    va_end(ap);
}

void ldebugf(const char* file, int line, const char *fmt, ...)
{
    va_list ap;
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s:%d %s", file, line, fmt);
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG " L", buf, ap);
    va_end(ap);
}

#endif
