/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by James Buren
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

#include "zip.h"
#include <string.h>
#include "strlcpy.h"
#include "file.h"
#include "dir.h"
#include "system.h"
#include "errno.h"
#include "core_alloc.h"
#include "timefuncs.h"
#include "pathfuncs.h"
#include "crc32.h"
#include "rbendian.h"

#define zip_core_alloc(N) core_alloc_ex("zip",(N),&buflib_ops_locked)

enum {
    ZIP_SIG_ED = 0x06054b50,
    ZIP_SIG_CD = 0x02014b50,
    ZIP_SIG_LF = 0x04034b50,
    ZIP_BIT_DD = 0x0008,
    ZIP_METHOD_STORE = 0x0000,
    ZIP_MAX_LENGTH = 0xffff,
    ZIP_BUFFER_SIZE = 4096,
};

enum {
    ZIP_STATE_INITIAL,
    ZIP_STATE_ED_ENTER,
    ZIP_STATE_ED_EXIT,
    ZIP_STATE_CD_ENTER,
    ZIP_STATE_CD_EXIT,
    ZIP_STATE_LF_ENTER,
    ZIP_STATE_LF_EXIT,
};

struct zip_ed_disk {
    uint32_t signature;
    uint16_t disk_number;
    uint16_t disk_number_cd;
    uint16_t disk_entries_cd;
    uint16_t cd_entries;
    uint32_t cd_size;
    uint32_t cd_offset;
    uint16_t comment_length;
    // comment block (variable length)
} __attribute__((packed));

struct zip_ed {
    uint32_t cd_size;
    uint32_t cd_offset;
    uint16_t cd_entries;
};

struct zip_cd_disk {
    uint32_t signature;
    uint16_t version_madeby;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t method;
    uint16_t time;
    uint16_t date;
    uint32_t crc;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t name_length;
    uint16_t extra_length;
    uint16_t comment_length;
    uint16_t disk_number;
    uint16_t internal_attributes;
    uint32_t external_attributes;
    uint32_t lf_offset;
    // name block (variable length)
    // extra block (variable length)
    // comment block (variable length)
} __attribute__((packed));

struct zip_cd {
    uint32_t crc;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint32_t lf_offset;
};

struct zip_lf_disk {
    uint32_t signature;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t method;
    uint16_t time;
    uint16_t date;
    uint32_t crc;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t name_length;
    uint16_t extra_length;
    // name block (variable length)
    // extra block (variable length)
} __attribute__((packed));

struct zip_lf {
    uint16_t flags;
    uint16_t method;
    uint16_t time;
    uint16_t date;
    uint32_t crc;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t name_length;
    char name[MAX_PATH];
};

struct zip {
    ssize_t (*read) (struct zip*, void*, size_t);
    off_t (*seek) (struct zip*, off_t, int);
    off_t (*size) (struct zip*);
    void (*close) (struct zip*);
    int zip_handle;
    int state;
    zip_callback cb;
    struct zip_args args;
    void* ctx;
    struct zip_ed ed;
    int cds_handle;
    struct zip_cd* cds;
    struct zip_lf lf;
};

struct zip_file {
    struct zip base;
    int file;
};

struct zip_mem {
    struct zip base;
    int mem_handle;
    const uint8_t* mem;
    off_t mem_offset;
    off_t mem_size;
};

struct zip_extract {
    zip_callback cb;
    void* ctx;
    size_t name_offset;
    size_t name_size;
    char* name;
    int file;
    char path[MAX_PATH];
};

static int zip_read_ed(struct zip* z) {
    const off_t file_size = z->size(z);
    const off_t max_size = sizeof(struct zip_ed_disk) + ZIP_MAX_LENGTH;
    const off_t read_size = MIN(file_size, max_size);
    const uint32_t sig = htole32(ZIP_SIG_ED);
    int mem_handle = -1;
    uint8_t* mem;
    off_t i = read_size - sizeof(struct zip_ed_disk);
    const struct zip_ed_disk* edd;
    uint16_t disk_number;
    uint16_t disk_number_cd;
    uint16_t disk_entries_cd;
    uint16_t cd_entries;
    struct zip_ed* ed = &z->ed;
    int rv;

    z->state = ZIP_STATE_ED_ENTER;

    if (file_size < (off_t) sizeof(struct zip_ed_disk)) {
        rv = -2;
        goto bail;
    }

    if ((mem_handle = zip_core_alloc(read_size)) < 0) {
        rv = -3;
        goto bail;
    }

    mem = core_get_data(mem_handle);

    if (z->seek(z, -read_size, SEEK_END) < 0) {
        rv = -4;
        goto bail;
    }

    if (z->read(z, mem, read_size) != read_size) {
        rv = -5;
        goto bail;
    }

    for (; i >= 0; i--)
        if (memcmp(mem + i, &sig, sizeof(uint32_t)) == 0)
            break;

    if (i < 0) {
        rv = -6;
        goto bail;
    }

    edd = (struct zip_ed_disk*) (mem + i);
    disk_number = letoh16(edd->disk_number);
    disk_number_cd = letoh16(edd->disk_number_cd);
    disk_entries_cd = letoh16(edd->disk_entries_cd);
    cd_entries = letoh16(edd->cd_entries);

    if (disk_number != 0 || disk_number_cd != 0 || disk_entries_cd != cd_entries) {
        rv = -7;
        goto bail;
    }

    ed->cd_size = letoh32(edd->cd_size);
    ed->cd_offset = letoh32(edd->cd_offset);
    ed->cd_entries = cd_entries;

    z->state = ZIP_STATE_ED_EXIT;
    rv = 0;

bail:
    if (mem_handle >= 0)
        core_free(mem_handle);
    return rv;
}

static int zip_read_cd(struct zip* z, bool use_cb) {
    const struct zip_ed* ed = &z->ed;
    const uint32_t cd_size = ed->cd_size;
    const uint32_t cd_offset = ed->cd_offset;
    const uint16_t cd_entries = ed->cd_entries;
    const uint32_t sig = htole32(ZIP_SIG_CD);
    int cds_handle = -1;
    int mem_handle = -1;
    struct zip_cd* cds;
    uint8_t* mem;
    struct zip_lf* lf = &z->lf;
    struct zip_args* args = &z->args;
    struct zip_cd_disk* cdd;
    struct zip_cd* cd;
    uint16_t name_length;
    int rv;

    z->state = ZIP_STATE_CD_ENTER;

    if ((cds_handle = zip_core_alloc(sizeof(struct zip_cd) * cd_entries)) < 0) {
        rv = -7;
        goto bail;
    }

    if ((mem_handle = zip_core_alloc(cd_size)) < 0) {
        rv = -8;
        goto bail;
    }

    cds = core_get_data(cds_handle);
    mem = core_get_data(mem_handle);

    if (z->seek(z, cd_offset, SEEK_SET) < 0) {
        rv = -9;
        goto bail;
    }

    if (z->read(z, mem, cd_size) != (ssize_t) cd_size) {
        rv = -10;
        goto bail;
    }

    if (use_cb) {
        args->entries = cd_entries;
        args->name = lf->name;
        args->block = NULL;
        args->block_size = 0;
        args->read_size = 0;
    }

    cdd = (struct zip_cd_disk*) mem;

    for (uint16_t i = 0; i < cd_entries; i++) {
        if (cdd->signature != sig) {
            rv = -11;
            goto bail;
        }

        cd = &cds[i];

        cd->crc = letoh32(cdd->crc);
        cd->compressed_size = letoh32(cdd->compressed_size);
        cd->uncompressed_size = letoh32(cdd->uncompressed_size);
        cd->lf_offset = letoh32(cdd->lf_offset);

        mem += sizeof(struct zip_cd_disk);
        name_length = letoh16(cdd->name_length);
        if (use_cb) {
            if (name_length >= sizeof(lf->name)) {
                rv = -12;
                goto bail;
            }

            args->entry = i + 1;
            args->file_size = cd->uncompressed_size;
            args->mtime = dostime_mktime(letoh16(cdd->date), letoh16(cdd->time));

            memcpy(lf->name, mem, name_length);
            lf->name[name_length] = '\0';

            if ((rv = z->cb(args, ZIP_PASS_SHALLOW, z->ctx)) > 0)
                goto bail;
        }
        mem += name_length;
        mem += letoh16(cdd->extra_length);
        mem += letoh16(cdd->comment_length);
        cdd = (struct zip_cd_disk*) mem;
    }

    z->cds_handle = cds_handle;
    z->cds = cds;
    z->state = ZIP_STATE_CD_EXIT;
    rv = 0;

bail:
    if (rv != 0 && cds_handle >= 0)
        core_free(cds_handle);
    if (mem_handle >= 0)
        core_free(mem_handle);
    return rv;
}

static int zip_read_lf(struct zip* z, uint16_t i) {
    const uint32_t sig = htole32(ZIP_SIG_LF);
    const struct zip_cd* cd = &z->cds[i];
    struct zip_lf* lf = &z->lf;
    struct zip_lf_disk lfd;
    uint16_t name_length;

    if (z->seek(z, cd->lf_offset, SEEK_SET) < 0)
        return -14;

    if (z->read(z, &lfd, sizeof(struct zip_lf_disk)) != sizeof(struct zip_lf_disk))
        return -15;

    if (lfd.signature != sig)
        return -16;

    name_length = letoh16(lfd.name_length);

    if (name_length >= sizeof(lf->name))
        return -17;

    if (z->read(z, lf->name, name_length) != name_length)
        return -18;

    if (z->seek(z, letoh16(lfd.extra_length), SEEK_CUR) < 0)
        return -19;

    lf->flags = letoh16(lfd.flags);
    lf->method = letoh16(lfd.method);
    lf->time = letoh16(lfd.time);
    lf->date = letoh16(lfd.date);
    lf->crc = letoh32(lfd.crc);
    lf->compressed_size = letoh32(lfd.compressed_size);
    lf->uncompressed_size = letoh32(lfd.uncompressed_size);
    lf->name_length = name_length;
    lf->name[name_length] = '\0';

    if ((lf->flags & ZIP_BIT_DD) == ZIP_BIT_DD) {
        lf->crc = cd->crc;
        lf->compressed_size = cd->compressed_size;
        lf->uncompressed_size = cd->uncompressed_size;
    }

    return 0;
}

static int zip_read_store(struct zip* z, void* mem, uint32_t mem_size) {
    const struct zip_lf* lf = &z->lf;
    struct zip_args* args = &z->args;
    uint32_t file_size = lf->uncompressed_size;
    uint32_t block_size = mem_size;
    uint32_t crc = 0xffffffff;
    int rv;

    if (lf->compressed_size != lf->uncompressed_size)
        return -21;

    args->block = mem;
    args->block_size = block_size;
    args->read_size = 0;

    do {
        if (block_size > file_size) {
            args->block_size = block_size = file_size;
        }

        if (z->read(z, mem, block_size) != (off_t) block_size)
            return -22;

        args->read_size += block_size;
        crc = crc_32r(mem, block_size, crc);

        if ((rv = z->cb(args, ZIP_PASS_DATA, z->ctx)) != 0)
            return (rv < 0) ? 0 : rv;

        file_size -= block_size;
    } while (file_size > 0);

    if (~crc != lf->crc)
        return -24;

    return 0;
}

static int zip_read_entry(struct zip* z, uint16_t i, void* mem, uint32_t mem_size) {
    const struct zip_lf* lf = &z->lf;
    struct zip_args* args = &z->args;
    int rv;

    if ((rv = zip_read_lf(z, i)) != 0)
        return rv;

    args->entry = i + 1;
    args->file_size = lf->uncompressed_size;
    args->mtime = dostime_mktime(lf->date, lf->time);
    args->block = NULL;
    args->block_size = 0;
    args->read_size = 0;

    if ((rv = z->cb(&z->args, ZIP_PASS_START, z->ctx)) != 0)
        return (rv < 0) ? 0 : rv;

    if (lf->uncompressed_size == 0)
        goto skip_data;

    if (lf->method == ZIP_METHOD_STORE) {
        if ((rv = zip_read_store(z, mem, mem_size)) != 0)
            return rv;
    } else {
        return -20;
    }

skip_data:
    args->block = NULL;
    args->block_size = 0;
    args->read_size = 0;

    if ((rv = z->cb(args, ZIP_PASS_END, z->ctx)) != 0)
        return (rv < 0) ? 0 : rv;

    return 0;
}

static int zip_read_entries(struct zip* z) {
    const struct zip_ed* ed = &z->ed;
    const uint16_t cd_entries = ed->cd_entries;
    struct zip_lf* lf = &z->lf;
    struct zip_args* args = &z->args;
    uint32_t mem_size = ZIP_BUFFER_SIZE;
    int mem_handle;
    void* mem;
    int rv;

    z->state = ZIP_STATE_LF_ENTER;

    if ((mem_handle = zip_core_alloc(mem_size)) < 0) {
        rv = -13;
        goto bail;
    }

    mem = core_get_data(mem_handle);

    args->entries = cd_entries;
    args->name = lf->name;

    for (uint16_t i = 0; i < cd_entries; i++)
        if ((rv = zip_read_entry(z, i, mem, mem_size)) > 0)
            goto bail;

    z->state = ZIP_STATE_LF_EXIT;
    rv = 0;

bail:
    if (mem_handle >= 0)
        core_free(mem_handle);
    return rv;
}

static void zip_init(struct zip* z, int zip_handle) {
    z->zip_handle = zip_handle;
    z->state = ZIP_STATE_INITIAL;
    z->cb = NULL;
    memset(&z->args, 0, sizeof(struct zip_args));
    z->ctx = NULL;
    memset(&z->ed, 0, sizeof(struct zip_ed));
    z->cds_handle = -1;
    z->cds = NULL;
    memset(&z->lf, 0, sizeof(struct zip_lf));
}

static ssize_t zip_file_read(struct zip* zh, void* mem, size_t mem_size) {
    struct zip_file* z = (struct zip_file*) zh;

    return read(z->file, mem, mem_size);
}

static off_t zip_file_seek(struct zip* zh, off_t offset, int whence) {
    struct zip_file* z = (struct zip_file*) zh;

    return lseek(z->file, offset, whence);
}

static off_t zip_file_size(struct zip* zh) {
    struct zip_file* z = (struct zip_file*) zh;

    return filesize(z->file);
}

static void zip_file_close(struct zip* zh) {
    struct zip_file* z = (struct zip_file*) zh;

    close(z->file);
}

static void zip_file_init(struct zip_file* z, int zip_handle, int file) {
    struct zip* zh = &z->base;

    zh->read = zip_file_read;
    zh->seek = zip_file_seek;
    zh->size = zip_file_size;
    zh->close = zip_file_close;
    zip_init(zh, zip_handle);

    z->file = file;
}

static ssize_t zip_mem_read(struct zip* zh, void* mem, size_t mem_size) {
    struct zip_mem* z = (struct zip_mem*) zh;
    off_t bytes = z->mem_size - z->mem_offset;
    off_t read_size = MIN(bytes, (off_t) mem_size);

    memcpy(mem, z->mem + z->mem_offset, read_size);
    z->mem_offset += read_size;

    return read_size;
}

static off_t zip_mem_seek(struct zip* zh, off_t offset, int whence) {
    struct zip_mem* z = (struct zip_mem*) zh;
    off_t new_offset;

    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;

        case SEEK_CUR:
            new_offset = z->mem_offset + offset;
            break;

        case SEEK_END:
            new_offset = z->mem_size + offset;
            break;

        default:
            new_offset = -1;
            break;
    }

    if (new_offset < 0 || new_offset > z->mem_size)
        return -1;

    z->mem_offset = new_offset;

    return new_offset;
}

static off_t zip_mem_size(struct zip* zh) {
    struct zip_mem* z = (struct zip_mem*) zh;

    return z->mem_size;
}

static void zip_mem_close(struct zip* zh) {
    struct zip_mem* z = (struct zip_mem*) zh;

    core_free(z->mem_handle);
}

static void zip_mem_init(struct zip_mem* z, int zip_handle, int mem_handle, const void* mem, off_t mem_size) {
    struct zip* zh = &z->base;

    zh->read = zip_mem_read;
    zh->seek = zip_mem_seek;
    zh->size = zip_mem_size;
    zh->close = zip_mem_close;
    zip_init(zh, zip_handle);

    z->mem_handle = mem_handle;
    z->mem = mem;
    z->mem_offset = 0;
    z->mem_size = mem_size;
}

static int zip_extract_start(const struct zip_args* args, struct zip_extract* ze) {
    size_t name_length;
    const char* dir;
    size_t dir_length;

    if ((name_length = strlcpy(ze->name, args->name, ze->name_size)) >= ze->name_size)
        return 5;

    if ((dir_length = path_dirname(ze->name, &dir)) > 0) {
        char c = ze->name[dir_length];

        ze->name[dir_length] = '\0';

        if (!dir_exists(ze->path)) {
            const char* path = ze->name;
            const char* name;

            while (parse_path_component(&path, &name) > 0) {
                size_t offset = path - ze->name;
                char c = ze->name[offset];

                ze->name[offset] = '\0';

                if (mkdir(ze->path) < 0 && errno != EEXIST)
                    return 6;

                ze->name[offset] = c;
            }
        }

        ze->name[dir_length] = c;
    }

    if (ze->name[name_length - 1] == PATH_SEPCH) {
        if (mkdir(ze->path) < 0 && errno != EEXIST)
            return 7;

        return 0;
    }

    if ((ze->file = creat(ze->path, 0666)) < 0)
        return 8;

    return 0;
}

static int zip_extract_data(const struct zip_args* args, struct zip_extract* ze) {
    if (write(ze->file, args->block, args->block_size) != (ssize_t) args->block_size) {
        return 9;
    }

    return 0;
}

static int zip_extract_end(const struct zip_args* args, struct zip_extract* ze) {
    int rv;

    if (ze->file >= 0) {
        rv = close(ze->file);

        ze->file = -1;

        if (rv < 0)
            return 10;
    }

    if (modtime(ze->path, args->mtime) < 0)
        return 11;

    return 0;
}

static int zip_extract_callback(const struct zip_args* args, int pass, void* ctx) {
    struct zip_extract* ze = ctx;
    int rv;

    if (ze->cb != NULL && (rv = ze->cb(args, pass, ze->ctx)) != 0)
        return rv;

    switch (pass) {
        case ZIP_PASS_START:
            return zip_extract_start(args, ze);

        case ZIP_PASS_DATA:
            return zip_extract_data(args, ze);

        case ZIP_PASS_END:
            return zip_extract_end(args, ze);

        default:
            return 1;
    }
}

struct zip* zip_open(const char* name, bool try_mem) {
    int file = -1;
    int mem_handle = -1;
    int zip_handle = -1;
    off_t mem_size;
    void* mem;
    void* zip;

    if (name == NULL || name[0] == '\0')
        goto bail;

    if ((file = open(name, O_RDONLY)) < 0)
        goto bail;

    if (try_mem && (mem_handle = zip_core_alloc(mem_size = filesize(file))) >= 0) {
        if ((zip_handle = zip_core_alloc(sizeof(struct zip_mem))) < 0)
            goto bail;

        mem = core_get_data(mem_handle);

        if (read(file, mem, mem_size) != mem_size)
            goto bail;

        close(file);

        zip = core_get_data(zip_handle);

        zip_mem_init(zip, zip_handle, mem_handle, mem, mem_size);
    } else {
        if ((zip_handle = zip_core_alloc(sizeof(struct zip_file))) < 0)
            goto bail;

        zip = core_get_data(zip_handle);

        zip_file_init(zip, zip_handle, file);
    }

    return zip;

bail:
    if (file >= 0)
        close(file);
    if (mem_handle >= 0)
        core_free(mem_handle);
    if (zip_handle >= 0)
        core_free(zip_handle);
    return NULL;
}

int zip_read_shallow(struct zip* z, zip_callback cb, void* ctx) {
    int rv;

    if (z == NULL || z->state != ZIP_STATE_INITIAL || cb == NULL)
        return -1;

    z->cb = cb;
    z->ctx = ctx;

    if ((rv = zip_read_ed(z)) != 0)
        return rv;

    return zip_read_cd(z, true);
}

int zip_read_deep(struct zip* z, zip_callback cb, void* ctx) {
    int rv;

    if (z == NULL || (z->state != ZIP_STATE_INITIAL && z->state != ZIP_STATE_CD_EXIT) || cb == NULL)
        return -1;

    z->cb = cb;
    z->ctx = ctx;

    if (z->state == ZIP_STATE_CD_EXIT)
        goto read_entries;

    if ((rv = zip_read_ed(z)) != 0)
        return rv;

    if ((rv = zip_read_cd(z, false)) != 0)
        return rv;

read_entries:
    return zip_read_entries(z);
}

int zip_extract(struct zip* z, const char* root, zip_callback cb, void* ctx) {
    int rv;
    int ze_handle = -1;
    struct zip_extract* ze;
    char* path;
    size_t size;
    size_t length;

    if (root == NULL || root[0] == '\0')
        root = PATH_ROOTSTR;

    if (root[0] != PATH_SEPCH) {
        rv = -1;
        goto bail;
    }

    if (!dir_exists(root)) {
        rv = 1;
        goto bail;
    }

    if ((ze_handle = zip_core_alloc(sizeof(struct zip_extract))) < 0) {
        rv = 2;
        goto bail;
    }

    ze = core_get_data(ze_handle);
    ze->cb = cb;
    ze->ctx = ctx;
    ze->file = -1;

    path = ze->path;
    size = sizeof(ze->path);
    length = strlcpy(path, root, size);

    if (length >= size) {
        rv = 3;
        goto bail;
    }

    path += length;
    size -= length;

    if (path[-1] != PATH_SEPCH) {
        length = strlcpy(path, PATH_SEPSTR, size);

        if (length >= size) {
            rv = 4;
            goto bail;
        }

        path += length;
        size -= length;
    }

    ze->name_offset = path - ze->path;
    ze->name_size = size;
    ze->name = path;

    rv = zip_read_deep(z, zip_extract_callback, ze);

bail:
    if (ze_handle >= 0) {
        if (ze->file >= 0)
            close(ze->file);

        core_free(ze_handle);
    }
    return rv;
}

void zip_close(struct zip* z) {
    if (z == NULL)
        return;

    z->close(z);

    if (z->cds_handle >= 0)
        core_free(z->cds_handle);

    core_free(z->zip_handle);
}
