/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __LIBRARY_MEMORY_H__
#  error "This header file must be included ONLY from memory.h."
#endif
#ifndef __LIBRARY_MEMORY_DEFINES_H__
#  define __LIBRARY_MEMORY_DEFINES_H__
#  ifndef MEMORY_PAGE_MINIMAL_ORDER
#    define MEMORY_PAGE_MINIMAL_ORDER (9) /* 512 bytes */
#  endif
#  ifndef MEMORY_PAGE_MAXIMAL_ORDER
#    define MEMORY_PAGE_MAXIMAL_ORDER (21) /* 2 Mbytes */
#  endif
#  ifndef MEMORY_PAGE_MINIMAL_SIZE
#    define MEMORY_PAGE_MINIMAL_SIZE (1 << MEMORY_PAGE_MINIMAL_ORDER)
#  endif
#  ifndef MEMORY_PAGE_MAXIMAL_SIZE
#    define MEMORY_PAGE_MAXIMAL_SIZE (1 << MEMORY_PAGE_MAXIMAL_ORDER)
#  endif
#  define MEMORY_TOTAL_PAGES (MEMORY_PAGE_MAXIMAL_SIZE / MEMORY_PAGE_MINIMAL_SIZE)
#  define MEMORY_TOTAL_BYTES (MEMORY_PAGE_MAXIMAL_SIZE)
#  define MEMORY_TOTAL_ORDERS (1 + MEMORY_PAGE_MAXIMAL_ORDER - MEMORY_PAGE_MINIMAL_ORDER)
#endif  