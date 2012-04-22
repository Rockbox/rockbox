/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *
 *
 * Copyright (C) 2012 by Jonathan Gordon
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

#ifndef _FILESCANNER_H_
#define _FILESCANNER_H_

#include <sys/types.h>
#include "config.h"
#include "events.h"


enum {
    /* Events related to the async filesystem scanner */
    /* Scan about to start, do any initialising you need */
    FILE_SCANNER_START = (EVENT_CLASS_DISK | 64),
    /* About to enter a directory.
     * Passes a struct file_scan_dir_data * for data.
     */
    FILE_SCANNER_ENTER_DIRECTORY,
    /* Finished scanning a directory.
     * Passes a struct file_scan_file_data * for data.
     */
    FILE_SCANNER_EXIT_DIRECTORY,
    /* Called for each file (EXCLUDING . and .. in a directory */
    FILE_SCANNER_FILE,
    /* Scan finished. *data == true means the scan was killed */
    FILE_SCANNER_FINISH,
};

struct file_scan_dir_data {
    char *path;
    DIR *dir;
};

struct file_scan_file_data {
    char *filename;
    struct dirinfo *info;
    DIR *dir;
};

void filescanner_init(void);
void filescanner_scan(long);
void filescanner_abort(void);

int filescanner_request_stack_var(size_t size);
void *filescanner_get_stack_var(int handle, size_t *size);

#endif
