/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __INSTALLER_FIIOM3K_H__
#define __INSTALLER_FIIOM3K_H__

#include <stddef.h>

#define INSTALL_SUCCESS                 0
#define INSTALL_ERR_OUT_OF_MEMORY       (-1)
#define INSTALL_ERR_FILE_NOT_FOUND      (-2)
#define INSTALL_ERR_FILE_IO             (-3)
#define INSTALL_ERR_BAD_FORMAT          (-4)
#define INSTALL_ERR_NAND_OPEN           (-5)
#define INSTALL_ERR_NAND_IDENTIFY       (-6)
#define INSTALL_ERR_NAND_READ           (-7)
#define INSTALL_ERR_NAND_ENABLE_WRITES  (-8)
#define INSTALL_ERR_NAND_ERASE          (-9)
#define INSTALL_ERR_NAND_WRITE          (-10)
#define INSTALL_ERR_TAR_OPEN            (-11)
#define INSTALL_ERR_TAR_FIND            (-12)
#define INSTALL_ERR_TAR_READ            (-13)
#define INSTALL_ERR_MTAR(x,y)           ((INSTALL_ERR_##x)*100 + (y))
#define INSTALL_ERR_FLASH(x,y)          ((INSTALL_ERR_##x)*100 + (y))

/* Install the Rockbox bootloader from a bootloader.m3k image */
extern int install_boot(const char* srcfile);

/* Backup or restore the bootloader from a raw NAND image */
extern int backup_boot(const char* destfile);
extern int restore_boot(const char* srcfile);

#endif /* __INSTALLER_FIIOM3K_H__ */
