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

#include "jztool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/** \brief Allocate a buffer, optionally providing its contents.
 * \param size  Number of bytes to allocate
 * \param data  Initial contents of the buffer, must be at least `size` bytes
 * \return Pointer to buffer or NULL if out of memory.
 * \note The buffer will not take ownership of the `data` pointer, instead it
 *       allocates a fresh buffer and copies the contents of `data` into it.
 */
jz_buffer* jz_buffer_alloc(size_t size, const void* data)
{
    jz_buffer* buf = malloc(sizeof(struct jz_buffer));
    if(!buf)
        return NULL;

    buf->data = malloc(size);
    if(!buf->data) {
        free(buf);
        return NULL;
    }

    if(data)
        memcpy(buf->data, data, size);

    buf->size = size;
    return buf;
}

/** \brief Free a buffer
 */
void jz_buffer_free(jz_buffer* buf)
{
    if(buf) {
        free(buf->data);
        free(buf);
    }
}

/** \brief Load a buffer from a file
 * \param buf       Returns loaded buffer on success, unmodified on error
 * \param filename  Path to the file
 * \return either JZ_SUCCESS, or one of the following errors
 * \retval JZ_ERR_OPEN_FILE     file cannot be opened
 * \retval JZ_ERR_OUT_OF_MEMORY cannot allocate buffer to hold file contents
 * \retval JZ_ERR_FILE_IO       problem reading file data
 */
int jz_buffer_load(jz_buffer** buf, const char* filename)
{
    FILE* f;
    jz_buffer* b;
    int rc;

    f = fopen(filename, "rb");
    if(!f)
        return JZ_ERR_OPEN_FILE;

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    b = jz_buffer_alloc(size, NULL);
    if(!b) {
        rc = JZ_ERR_OUT_OF_MEMORY;
        goto err_fclose;
    }

    if(fread(b->data, size, 1, f) != 1) {
        rc = JZ_ERR_FILE_IO;
        goto err_free_buf;
    }

    rc = JZ_SUCCESS;
    *buf = b;

  err_fclose:
    fclose(f);
    return rc;

  err_free_buf:
    jz_buffer_free(b);
    goto err_fclose;
}

/** \brief Save a buffer to a file
 * \param buf       Buffer to be written out
 * \param filename  Path to the file
 * \return either JZ_SUCCESS, or one of the following errors
 * \retval JZ_ERR_OPEN_FILE     file cannot be opened
 * \retval JZ_ERR_FILE_IO       problem writing file data
 */
int jz_buffer_save(jz_buffer* buf, const char* filename)
{
    int rc;
    FILE* f;

    f = fopen(filename, "wb");
    if(!f)
        return JZ_ERR_OPEN_FILE;

    if(fwrite(buf->data, buf->size, 1, f) != 1) {
        rc = JZ_ERR_FILE_IO;
        goto err_fclose;
    }

    rc = JZ_SUCCESS;

  err_fclose:
    fclose(f);
    return rc;
}
