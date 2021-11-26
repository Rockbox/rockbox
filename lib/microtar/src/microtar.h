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

#ifndef MICROTAR_H
#define MICROTAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>  /* SEEK_SET et al. */

enum mtar_error {
    MTAR_ESUCCESS     =  0,
    MTAR_EFAILURE     = -1,
    MTAR_EOPENFAIL    = -2,
    MTAR_EREADFAIL    = -3,
    MTAR_EWRITEFAIL   = -4,
    MTAR_ESEEKFAIL    = -5,
    MTAR_ESEEKRANGE   = -6,
    MTAR_EBADCHKSUM   = -7,
    MTAR_ENULLRECORD  = -8,
    MTAR_ENOTFOUND    = -9,
    MTAR_EOVERFLOW    = -10,
    MTAR_EAPI         = -11,
    MTAR_ENAMETOOLONG = -12,
    MTAR_EWRONGSIZE   = -13,
    MTAR_EACCESS      = -14,
};

enum mtar_type {
    MTAR_TREG   = '0',
    MTAR_TLNK   = '1',
    MTAR_TSYM   = '2',
    MTAR_TCHR   = '3',
    MTAR_TBLK   = '4',
    MTAR_TDIR   = '5',
    MTAR_TFIFO  = '6',
};

enum mtar_access {
    MTAR_READ,
    MTAR_WRITE,
};

typedef struct mtar_header mtar_header_t;
typedef struct mtar mtar_t;
typedef struct mtar_ops mtar_ops_t;

typedef int(*mtar_foreach_cb)(mtar_t*, const mtar_header_t*, void*);

struct mtar_header {
    unsigned mode;
    unsigned owner;
    unsigned group;
    unsigned size;
    unsigned mtime;
    unsigned type;
    char name[101]; /* +1 byte in order to ensure null termination */
    char linkname[101];
};

struct mtar_ops {
    int(*read)(void* stream, void* data, unsigned size);
    int(*write)(void* stream, const void* data, unsigned size);
    int(*seek)(void* stream, unsigned pos);
    int(*close)(void* stream);
};

struct mtar {
    char buffer[512];       /* IO buffer, put first to allow library users to
                             * control its alignment */
    int state;              /* Used to simplify the API and verify API usage */
    int access;             /* Access mode */
    unsigned pos;           /* Current position in file */
    unsigned end_pos;       /* End position of the current file */
    unsigned header_pos;    /* Position of the current header */
    mtar_header_t header;   /* Most recently parsed header */
    const mtar_ops_t* ops;  /* Stream operations */
    void* stream;           /* Stream handle */
};

const char* mtar_strerror(int err);

void mtar_init(mtar_t* tar, int access, const mtar_ops_t* ops, void* stream);
int mtar_close(mtar_t* tar);
int mtar_is_open(mtar_t* tar);

mtar_header_t* mtar_get_header(mtar_t* tar);
int mtar_access_mode(const mtar_t* tar);

int mtar_rewind(mtar_t* tar);
int mtar_next(mtar_t* tar);
int mtar_foreach(mtar_t* tar, mtar_foreach_cb cb, void* arg);
int mtar_find(mtar_t* tar, const char* name);

int mtar_read_data(mtar_t* tar, void* ptr, unsigned size);
int mtar_seek_data(mtar_t* tar, int offset, int whence);
unsigned mtar_tell_data(mtar_t* tar);
int mtar_eof_data(mtar_t* tar);

int mtar_write_header(mtar_t* tar, const mtar_header_t* h);
int mtar_update_header(mtar_t* tar, const mtar_header_t* h);
int mtar_write_file_header(mtar_t* tar, const char* name, unsigned size);
int mtar_write_dir_header(mtar_t* tar, const char* name);
int mtar_write_data(mtar_t* tar, const void* ptr, unsigned size);
int mtar_update_file_size(mtar_t* tar);
int mtar_end_data(mtar_t* tar);
int mtar_finalize(mtar_t* tar);

#ifdef __cplusplus
}
#endif

#endif
