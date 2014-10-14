/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"
#include "lib/pluginlib_exit.h"
#include <stdarg.h>
#include "util.h"
uint16_t g_debugMask;

#ifdef XWORLD_DEBUG
void debug_real(uint16_t cm, const char *msg, ...) {
#ifdef ROCKBOX_HAS_LOGF
    char buf[1024];
    if (cm & g_debugMask) {
        va_list va;
        va_start(va, msg);
        rb->vsnprintf(buf, 1024, msg, va);
        va_end(va);
        LOGF("%s", buf);
    }
#else
    (void) cm;
    (void) msg;
#endif
}
#endif

void error(const char *msg, ...) {
    char buf[1024];
    va_list va;
    va_start(va, msg);
    rb->vsnprintf(buf, 1024, msg, va);
    va_end(va);
    rb->splashf(HZ * 2, "ERROR: %s!", buf);
    LOGF("ERROR: %s", buf);
    exit(-1);
}

void warning(const char *msg, ...) {
    char buf[1024];
    va_list va;
    va_start(va, msg);
    rb->vsnprintf(buf, 1024, msg, va);
    va_end(va);
    rb->splashf(HZ * 2, "WARNING: %s!", buf);
    LOGF("WARNING: %s", buf);
}

void string_lower(char *p) {
    for (; *p; ++p) {
        if (*p >= 'A' && *p <= 'Z') {
            *p += 'a' - 'A';
        }
    }
}

void string_upper(char *p) {
    for (; *p; ++p) {
        if (*p >= 'a' && *p <= 'z') {
            *p += 'A' - 'a';
        }
    }
}
