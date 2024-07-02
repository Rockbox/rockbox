/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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
#ifndef FILEOP_H
#define FILEOP_H

#include "file.h"

/* result codes of various file operations */
enum fileop_result_code
{
    FORC_PATH_EXISTS      = -8,
    FORC_READ_FAILURE     = -7,
    FORC_WRITE_FAILURE    = -6,
    FORC_NO_BUFFER_AVAIL  = -5,
    FORC_TOO_MANY_SUBDIRS = -4,
    FORC_PATH_TOO_LONG    = -3,
    FORC_PATH_NOT_EXIST   = -2,
    FORC_UNKNOWN_FAILURE  = -1,
    /* Anything < 0 is failure */
    FORC_SUCCESS   = 0,     /* All operations completed successfully */
    FORC_NOOP      = 1,     /* Operation didn't need to do anything */
    FORC_CANCELLED = 2,     /* Operation was cancelled by user */
    FORC_NOOVERWRT = 3,
};

enum file_op_flags
{
    PASTE_CUT       = 0x00, /* Is a move (cut) operation (default) */
    PASTE_COPY      = 0x01, /* Is a copy operation */
    PASTE_OVERWRITE = 0x02, /* Overwrite destination */
    PASTE_EXDEV     = 0x04, /* Actually copy/move across volumes */
};

enum file_op_current
{
    FOC_NONE = 0,
    FOC_COUNT,
    FOC_MOVE,
    FOC_COPY,
    FOC_DELETE,
};

int create_dir(void);

int rename_file(const char *selected_file);

int delete_fileobject(const char *selected_file);

int copy_move_fileobject(const char *src_path,
                         const char *dst_path,
                           unsigned int flags);

#endif /* FILEOP_H */
