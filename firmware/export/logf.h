/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Daniel Stenberg
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
#ifndef LOGF_H
#define LOGF_H
#include <config.h>
#include <stdbool.h>
#include "../include/_ansi.h"
#include "debug.h"

#ifdef ROCKBOX_HAS_LOGF

#ifndef __PCTOOL__
#define MAX_LOGF_LINES 1000
#define MAX_LOGF_ENTRY 29
#define MAX_LOGF_ONE_LINE_SIZE 200

#define LOGF_TERMINATE_ONE_LINE      0x00
#define LOGF_TERMINATE_CONTINUE_LINE 0x01
#define LOGF_TERMINATE_MULTI_LINE    0x02

extern unsigned char logfbuffer[MAX_LOGF_LINES][MAX_LOGF_ENTRY+1];
extern int logfindex;
extern bool logfwrap;
#endif /* __PCTOOL__ */

#define logf _logf
void _logf(const char *format, ...) ATTRIBUTE_PRINTF(1, 2);

#else /* !ROCKBOX_HAS_LOGF */

/* built without logf() support enabled, replace logf() by DEBUGF() */
#define logf(f,args...) DEBUGF(f"\n",##args)

#endif /* !ROCKBOX_HAS_LOGF */

#endif /* LOGF_H */

/* Allow fine tuning (per file) of the logf output */
#ifndef LOGF_ENABLE
#undef logf
#define logf(...)
#endif
