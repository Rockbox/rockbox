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

#ifndef __INSTALLER_X1000_H__
#define __INSTALLER_X1000_H__

/* This API is for the bootloader recovery menu and Rockbox utility to handle
 * bootloader installation, backup, and restore.
 *
 * Currently the installer can only handle NAND flash, although the X1000 can
 * boot from NOR flash or SD/MMC. Support for other storage media can be added
 * when there is a target that needs it.
 *
 * Bootloader updates are tarballs, and they can "monkey patch" the flash in
 * a customizable way (but fixed at compile time).
 *
 * Backup and restore simply takes the range of eraseblocks touched by the
 * monkey patch and copies them to or from a regular file.
 */

enum {
    IERR_SUCCESS = 0,
    IERR_OUT_OF_MEMORY,
    IERR_FILE_NOT_FOUND,
    IERR_FILE_IO,
    IERR_BAD_FORMAT,
    IERR_NAND_OPEN,
    IERR_NAND_READ,
    IERR_NAND_WRITE,
};

extern int install_bootloader(const char* filename);
extern int backup_bootloader(const char* filename);
extern int restore_bootloader(const char* filename);
extern const char* installer_strerror(int rc);

#endif /* __INSTALLER_X1000_H__ */
