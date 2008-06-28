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

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#if !defined(__ssize_t_defined) && !defined(_SSIZE_T_) && !defined(ssize_t) && !defined(_SSIZE_T_DECLARED)
#define __ssize_t_defined
#define _SSIZE_T_
#define ssize_t ssize_t
typedef signed long ssize_t;
#endif

#if !defined(__off_t_defined) && !defined(_OFF_T_) && !defined(off_t) && !defined(_OFF_T_DECLARED)
#define __off_t_defined
#define _OFF_T_
#define off_t off_t
typedef signed long off_t;
#endif

#if !defined(__mode_t_defined) && !defined(_MODE_T_) && !defined(mode_t) && !defined(_MODE_T_DECLARED)
#define __mode_t_defined
#define _MODE_T_
#define mode_t mode_t
typedef unsigned int mode_t;
#endif

#if !defined(_SIZE_T) && !defined(_SIZE_T_DECLARED)
#define _SIZE_T
#define _SIZE_T_DECLARED
typedef unsigned long size_t;
#endif

#endif /* _SYS_TYPES_H */
