/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __FILE_WIN32_H__
#define __FILE_WIN32_H__

#include <io.h>

struct direnttag
{
    long            d_ino; /* inode number */
    long            d_off; /* offset to the next dirent */
    unsigned short  d_reclen;/* length of this record */
    unsigned char   d_type; /* type of file */
    char            d_name[256]; /* filename */
};
typedef struct direnttag dirent;

struct DIRtag
{
    dirent          fd;
    intptr_t        handle;
};
typedef struct DIRtag DIR;

#endif // #ifndef __FILE_WIN32_H__