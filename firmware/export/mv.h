/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Frank Gevaerts
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

#ifndef __MV_H__
#define __MV_H__

#include "config.h"

/* FixMe: These macros are a bit nasty and perhaps misplaced here.
   We'll get rid of them once decided on how to proceed with multivolume. */

/* Drives are things like a disk, a card, a flash chip. They can have volumes on them */
#ifdef HAVE_MULTIDRIVE
#define IF_MD(x) x /* optional drive parameter */
#define IF_MD2(x,y) x,y /* same, for a list of arguments */
#define IF_MD_NONVOID(x) x /* for prototype with sole volume parameter */
#else /* empty definitions if no multi-drive */
#define IF_MD(x)
#define IF_MD2(x,y)
#define IF_MD_NONVOID(x) void
#endif

/* Volumes mean things that have filesystems on them, like partitions */
#ifdef HAVE_MULTIVOLUME
#define IF_MV(x) x /* optional volume parameter */
#define IF_MV2(x,y) x,y /* same, for a list of arguments */
#define IF_MV_NONVOID(x) x /* for prototype with sole volume parameter */
#else /* empty definitions if no multi-volume */
#define IF_MV(x)
#define IF_MV2(x,y)
#define IF_MV_NONVOID(x) void
#endif


#endif
