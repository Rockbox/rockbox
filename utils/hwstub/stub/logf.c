/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "logf.h"
#include "format.h"
#include "string.h"
#include "memory.h"

static unsigned char logfbuffer[MAX_LOGF_SIZE];
static int logfread = 0;
static int logfwrite = 0;
static int logfen = true;

void enable_logf(bool en)
{
    logfen = en;
}

static int logf_push(void *userp, unsigned char c)
{
    (void)userp;
    
    logfbuffer[logfwrite++] = c;
    if(logfwrite == MAX_LOGF_SIZE)
        logfwrite = 0;
    return true;
}

void logf(const char *fmt, ...)
{
    if(!logfen) return;
    va_list ap;
    va_start(ap, fmt);
    vuprintf(logf_push, NULL, fmt, ap);
    va_end(ap);
}

size_t logf_readback(char *buf, size_t max_size)
{
    if(logfread == logfwrite)
        return 0;
    if(logfread < logfwrite)
        max_size = MIN(max_size, (size_t)(logfwrite - logfread));
    else
        max_size = MIN(max_size, (size_t)(MAX_LOGF_SIZE - logfread));
    memcpy(buf, &logfbuffer[logfread], max_size);
    logfread += max_size;
    if(logfread == MAX_LOGF_SIZE)
        logfread = 0;
    return max_size;
}
