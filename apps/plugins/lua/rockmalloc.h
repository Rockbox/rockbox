/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
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

#ifndef _ROCKMALLOC_H_
#define _ROCKMALLOC_H_

#undef WIN32
#undef _WIN32
#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H
#define LACKS_STRINGS_H
#define INSECURE 1
#define USE_DL_PREFIX 1
#define MORECORE_CANNOT_TRIM 1
#define HAVE_MMAP 0
#define HAVE_MREMAP 0
#define NO_MALLINFO 1
#define ABORT ((void) 0)
/* #define DEBUG */
#define MORECORE rocklua_morecore

void *rocklua_morecore(int size);
void dlmalloc_stats(void);

#define printf DEBUGF

#endif
