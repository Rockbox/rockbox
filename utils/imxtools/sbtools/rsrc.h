/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#ifndef __RSRC_H__
#define __RSRC_H__

#include <stdint.h>
#include <stdbool.h>

#include "misc.h"

/**
 * Low-Level
 **/

#define RSRC_SECTOR_SIZE    2048

#define RSRC_TABLE_ENTRY_TYPE(e)    ((e) >> 28)
#define RSRC_TABLE_ENTRY_OFFSET(e)  ((e) & 0xfffffff)

#define RSRC_TYPE_NONE      0 /* empty entry */
#define RSRC_TYPE_NESTED    1 /* nested entry: points to a sub-table */
#define RSRC_TYPE_IMAGE     2 /* image entry */
#define RSRC_TYPE_VALUE     3 /* value stored on 28-bits */
#define RSRC_TYPE_AUDIO     4 /* audio entry */
#define RSRC_TYPE_DATA      5 /* data entry */

/**
 * API
 **/

struct rsrc_entry_t
{
    uint32_t id;
    uint32_t offset; // contains value of RSRC_TYPE_VALUE
    int size;
};

struct rsrc_file_t
{
    void *data;
    int size;

    int nr_entries;
    int capacity;
    struct rsrc_entry_t *entries;
};

enum rsrc_error_t
{
    RSRC_SUCCESS = 0,
    RSRC_ERROR = -1,
    RSRC_OPEN_ERROR = -2,
    RSRC_READ_ERROR = -3,
    RSRC_WRITE_ERROR = -4,
    RSRC_FORMAT_ERROR = -5,
    RSRC_CHECKSUM_ERROR = -6,
    RSRC_NO_VALID_KEY = -7,
    RSRC_CRYPTO_ERROR = -8,
};

enum rsrc_error_t rsrc_write_file(struct rsrc_file_t *rsrc, const char *filename);

typedef void (*rsrc_color_printf)(void *u, bool err, color_t c, const char *f, ...);
struct rsrc_file_t *rsrc_read_file(const char *filename, void *u,
    rsrc_color_printf printf, enum rsrc_error_t *err);
/* use size_t(-1) to use maximum size */
struct rsrc_file_t *rsrc_read_file_ex(const char *filename, size_t offset, size_t size, void *u,
    rsrc_color_printf printf, enum rsrc_error_t *err);
struct rsrc_file_t *rsrc_read_memory(void *buffer, size_t size, void *u,
    rsrc_color_printf printf, enum rsrc_error_t *err);

void rsrc_dump(struct rsrc_file_t *file, void *u, rsrc_color_printf printf);
void rsrc_free(struct rsrc_file_t *file);

#endif /* __RSRC_H__ */

