/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef DEBUG_H
#define DEBUG_H

#include <_ansi.h>

extern void debug_init(void);
extern void debugf(const char* fmt,...) ATTRIBUTE_PRINTF(1, 2);
extern void ldebugf(const char* file, int line, const char *fmt, ...)
                    ATTRIBUTE_PRINTF(3, 4);

#ifdef __GNUC__

/*  */
#if defined(SIMULATOR) && !defined(__PCTOOL__)
#define DEBUGF	debugf
#define LDEBUGF(...) ldebugf(__FILE__, __LINE__, __VA_ARGS__)
#else
#if defined(DEBUG)

#ifdef HAVE_GDB_API
void breakpoint(void);
#endif

#define DEBUGF	debugf
#define LDEBUGF debugf
#else
#define DEBUGF(...)
#define LDEBUGF(...)
#endif
#endif


#else

#define DEBUGF debugf
#define LDEBUGF debugf

#endif /* GCC */


#endif
