/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006,2007 by Greg White
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

/*  This file MUST be included in your system-target.h file if you want arm
 *  cache coherence functions to be called (I.E. during codec load, etc).   
 */

#ifndef MMU_S5L8700_H
#define MMU_S5L8700_H

/* Cleans entire DCache */
void clean_dcache(void) ICODE_ATTR;

/* Invalidate entire DCache */
/* will do writeback */
void invalidate_dcache(void) ICODE_ATTR;

/* Invalidate entire ICache and DCache */
/* will do writeback */
void invalidate_idcache(void) ICODE_ATTR;

#define HAVE_CPUCACHE_INVALIDATE
#define HAVE_CPUCACHE_FLUSH

#endif /* MMU_S5L8700_H */
