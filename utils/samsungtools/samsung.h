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
#ifndef __SAMSUNG_H__
#define __SAMSUNG_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * Low-Level
 */

struct yp_header_t
{
    char signature[12];
    uint32_t pad;
    char version[8];
    char region[4];
    char extra[4];
    char model[20];
    uint32_t datasize;
} __attribute__((packed));

struct yp_md5_t
{
    uint8_t md5[16];
} __attribute__((packed));

#define YP_SIGNATURE    "SAMSUNG YEPP"

/**
 * API
 */

struct samsung_firmware_t
{
    char version[8];
    char region[4];
    char extra[4];
    char model[20];
    void *data;
    int data_size;
};

enum samsung_error_t
{
    SAMSUNG_SUCCESS = 0,
    SAMSUNG_READ_ERROR = -1,
    SAMSUNG_FORMAT_ERROR = -2,
    SAMSUNG_MD5_ERROR = -3,
    SAMSUNG_WRITE_ERROR = -4,
};

typedef int (*samsung_read_t)(void *user, int offset, void *buffer, int size);
typedef int (*samsung_write_t)(void *user, int offset, void *buffer, int size);
typedef void (*samsung_printf_t)(void *user, bool error, const char *fmt, ...);

struct samsung_firmware_t *samsung_read(samsung_read_t read,
    samsung_printf_t printf, void *user, enum samsung_error_t *err);
enum samsung_error_t samsung_write(samsung_write_t write, samsung_printf_t printf,
    void *user, struct samsung_firmware_t *fw);
void samsung_free(struct samsung_firmware_t *fw);

#endif /* __SAMSUNG_H__ */
