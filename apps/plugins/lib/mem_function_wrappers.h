/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nils Wallménius
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __MEM_FUNCTION_WRAPPERS_H__
#define __MEM_FUNCTION_WRAPPERS_H__

/* Use this macro in plugins where gcc tries to optimize by calling
 * these functions directly */

#define MEM_FUNCTION_WRAPPERS(api) \
        void *memcpy(void *dest, const void *src, size_t n) \
        { \
            return (api)->memcpy(dest, src, n); \
        } \
        void *memset(void *dest, int c, size_t n) \
        { \
            return (api)->memset(dest, c, n); \
        } \
        void *memmove(void *dest, const void *src, size_t n) \
        { \
            return (api)->memmove(dest, src, n); \
        } \
        int memcmp(const void *s1, const void *s2, size_t n) \
        { \
            return (api)->memcmp(s1, s2, n); \
        }

#endif /* __MEM_FUNCTION_WRAPPERS_H__ */

