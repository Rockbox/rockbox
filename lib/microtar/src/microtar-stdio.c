/*
 * Copyright (c) 2017 rxi
 * Copyright (c) 2021 Aidan MacDonald
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "microtar-stdio.h"
#include <stdio.h>
#include <string.h>

static int file_read(void* stream, void* data, unsigned size)
{
    unsigned res = fread(data, 1, size, (FILE*)stream);
    if(res != size && ferror((FILE*)stream))
        return MTAR_EREADFAIL;

    return res;
}

static int file_write(void* stream, const void* data, unsigned size)
{
    unsigned res = fwrite(data, 1, size, (FILE*)stream);
    if(res != size && ferror((FILE*)stream))
       return MTAR_EWRITEFAIL;

    return res;
}

static int file_seek(void* stream, unsigned offset)
{
    int res = fseek((FILE*)stream, offset, SEEK_SET);
    return (res == 0) ? MTAR_ESUCCESS : MTAR_ESEEKFAIL;
}

static int file_close(void* stream)
{
    int err = fclose((FILE*)stream);
    return (err == 0 ? MTAR_ESUCCESS : MTAR_EFAILURE);
}

static const mtar_ops_t file_ops = {
    .read = file_read,
    .write = file_write,
    .seek = file_seek,
    .close = file_close,
};

int mtar_open(mtar_t* tar, const char* filename, const char* mode)
{
    /* Determine access mode */
    int access;
    char* read = strchr(mode, 'r');
    char* write = strchr(mode, 'w');
    if(read) {
        if(write)
            return MTAR_EAPI;
        access = MTAR_READ;
    } else if(write) {
        access = MTAR_WRITE;
    } else {
        return MTAR_EAPI;
    }

    /* Open file */
    FILE* file = fopen(filename, mode);
    if(!file)
        return MTAR_EOPENFAIL;

    mtar_init(tar, access, &file_ops, file);
    return MTAR_ESUCCESS;
}
