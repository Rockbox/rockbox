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

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#if !defined(__ssize_t_defined) && !defined(_SSIZE_T_) && !defined(ssize_t)
#define __ssize_t_defined
#define _SSIZE_T_
#define ssize_t ssize_t
typedef signed long ssize_t;
#endif

#if !defined(__off_t_defined) && !defined(_OFF_T_) && !defined(off_t)
#define __off_t_defined
#define _OFF_T_
#define off_t off_t
typedef signed long off_t;
#endif

#if !defined(__mode_t_defined) && !defined(_MODE_T_) && !defined(mode_t)
#define __mode_t_defined
#define _MODE_T_
#define mode_t mode_t
typedef unsigned int mode_t;
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

#endif /* _SYS_TYPES_H */
