/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>

#include "config.h"
#include "debug.h"

#include "debug-ibasso.h"


static const char log_tag[] = "Rockbox";


void debug_init(void)
{}


void debugf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, log_tag, fmt, ap);
    va_end(ap);
}


void ldebugf(const char* file, int line, const char *fmt, ...)
{
    va_list ap;
    /* 13: 5 literal chars and 8 chars for the line number. */
    char buf[strlen(file) + strlen(fmt) + 13];
    snprintf(buf, sizeof(buf), "%s (%d): %s", file, line, fmt);
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, log_tag, buf, ap);
    va_end(ap);
}


void debug_trace(const char* function)
{
    static const char trace_tag[] = "TRACE: ";
    char msg[strlen(trace_tag) + strlen(function) + 1];
    snprintf(msg, sizeof(msg), "%s%s", trace_tag, function);
    __android_log_write(ANDROID_LOG_DEBUG, log_tag, msg);
}
