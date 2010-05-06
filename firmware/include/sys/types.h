/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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

/**
 * provide a sys/types.h for compatibility with imported code
 **/

#ifndef __TYPES_H__
#define __TYPES_H__


/*
 * include string.h for size_t for convinence */
#include <string.h>
/* make some (debian, ubuntu...) systems happy that inappropriately include
 * sys/types.h to get intN_t ... */
#include <inttypes.h>

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

#endif /* __TYPES_H__ */
