/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#  ifndef __LIBRARY_MEMORY_FUNCTIONS_H__
#  define __LIBRARY_MEMORY_FUNCTIONS_H__
extern void memory_copy (void *target,void const *source,unsigned int count);
extern void memory_set (void *target,int byte,unsigned int count);
extern int  memory_release_page (void *);
extern void *memory_allocate_page (int);
extern void memory_setup (void);
#  ifdef TEST
void memory_spy_page (void *address);
void memory_dump (int order);
void memory_check (int order);
#  endif
#endif
