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

#ifndef __UTIL_H__
#define __UTIL_H__

#include "intern.h"

#ifdef ROCKBOX_HAS_LOGF
#define XWORLD_DEBUG
#endif

#ifdef XWORLD_DEBUG
#define debug(m,f,...) debug_real(m, f, ##__VA_ARGS__)
#define XWORLD_DEBUGMASK ~0
#else
#define debug(m,f,...)
#define XWORLD_DEBUGMASK 0
#endif

enum {
    DBG_VM = 1 << 0,
    DBG_BANK  = 1 << 1,
    DBG_VIDEO = 1 << 2,
    DBG_SND   = 1 << 3,
    DBG_SER   = 1 << 4,
    DBG_INFO  = 1 << 5,
    DBG_RES   = 1 << 6,
    DBG_FILE  = 1 << 7,
    DBG_SYS   = 1 << 8,
    DBG_ENG   = 1 << 9
};

extern uint16_t g_debugMask;
#ifdef XWORLD_DEBUG
extern void debug_real(uint16_t cm, const char *msg, ...);
#endif
extern void error(const char *msg, ...);
extern void warning(const char *msg, ...);

extern void string_lower(char *p);
extern void string_upper(char *p);

#endif
