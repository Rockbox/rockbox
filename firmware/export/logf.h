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

#ifdef ROCKBOX_HAS_LOGF

#define MAX_LOGF_LINES 1000
#define MAX_LOGF_DATASIZE (16*MAX_LOGF_LINES)

extern unsigned char logfbuffer[MAX_LOGF_LINES][16];
extern int logfindex;
extern bool logfwrap;

void logf(const char *format, ...);
#else
/* built without logf() support enabled */
#define logf(...)
#endif

#endif /* LOGF_H */
