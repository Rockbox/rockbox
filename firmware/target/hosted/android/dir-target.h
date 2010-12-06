/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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

#ifndef __DIR_TARGET_H__
#define __DIR_TARGET_H__

#include <dirent.h>

#define dirent_uncached dirent
#define DIR_UNCACHED DIR
#define opendir_uncached _opendir
#define readdir_uncached _readdir
#define closedir_uncached _closedir
#define mkdir_uncached _mkdir
#define rmdir_uncached rmdir

#define dirent_android dirent
#define DIR_android DIR
#define opendir_android _opendir
#define readdir_android _readdir
#define closedir_android _closedir
#define mkdir_android _mkdir
#define rmdir_android rmdir

extern DIR* _opendir(const char* name);
extern int  _mkdir(const char* name);
extern int  _closedir(DIR* dir);
extern struct dirent *_readdir(DIR* dir);
extern void fat_size(unsigned long *size, unsigned long *free);

#define DIRFUNCTIONS_DEFINED
#define DIRENT_DEFINED
#define DIR_DEFINED

#endif /* __DIR_TARGET_H__ */
