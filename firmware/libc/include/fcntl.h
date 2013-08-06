/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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

#ifndef __FCNTL_H__
#define __FCNTL_H__

#ifndef O_RDONLY
#define O_RDONLY    0x0000      /* open for reading only */
#define O_WRONLY    0x0001      /* open for writing only */
#define O_RDWR      0x0002      /* open for reading and writing */
#define O_ACCMODE   0x0003      /* mask for above modes */
#define O_APPEND    0x0008      /* set append mode */
#define O_CREAT     0x0200      /* create if nonexistent */
#define O_TRUNC     0x0400      /* truncate to zero length */
#define O_EXCL      0x0800      /* error if already exists */
#endif

#ifndef SEEK_SET
#define SEEK_SET    0           /* set file offset to offset */
#define SEEK_CUR    1           /* set file offset to current plus offset */
#define SEEK_END    2           /* set file offset to EOF plus offset */
#endif

#endif /* __FCNTL_H__ */
