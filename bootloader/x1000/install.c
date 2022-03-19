/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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

#include "x1000bootloader.h"
#include "kernel.h"
#include "button.h"
#include "installer-x1000.h"
#include <stdio.h>

enum {
    INSTALL,
    BACKUP,
    RESTORE,
};

static void bootloader_action(int which)
{
    if(check_disk(true) != DISK_PRESENT) {
        splashf(5*HZ, "Install aborted\nCannot access SD card");
        return;
    }

    const char* msg;
    switch(which) {
    case INSTALL: msg = "Installing"; break;
    case BACKUP:  msg = "Backing up"; break;
    case RESTORE: msg = "Restoring";  break;
    default: return; /* can't happen */
    }

    splashf(0, msg);

    int rc;
    switch(which) {
    case INSTALL: rc = install_bootloader("/bootloader." BOOTFILE_EXT); break;
    case BACKUP:  rc = backup_bootloader(BOOTBACKUP_FILE); break;
    case RESTORE: rc = restore_bootloader(BOOTBACKUP_FILE); break;
    default: return;
    }

    static char buf[64];
    snprintf(buf, sizeof(buf), "%s (%d)", installer_strerror(rc), rc);
    const char* msg1 = rc == 0 ? "Success" : buf;
    const char* msg2 = "Press " BL_QUIT_NAME " to continue";
    splashf(0, "%s\n%s", msg1, msg2);

    while(get_button(TIMEOUT_BLOCK) != BL_QUIT);
}

void bootloader_install(void)
{
    bootloader_action(INSTALL);
}

void bootloader_backup(void)
{
    bootloader_action(BACKUP);
}

void bootloader_restore(void)
{
    bootloader_action(RESTORE);
}
