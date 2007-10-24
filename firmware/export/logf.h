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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#define MAX_LOGF_ENTRY 30
#define MAX_LOGF_DATASIZE (MAX_LOGF_ENTRY*MAX_LOGF_LINES)

extern unsigned char logfbuffer[MAX_LOGF_LINES][MAX_LOGF_ENTRY];
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
