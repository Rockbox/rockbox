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

#include "microtar.h"
#include <limits.h>
#include <string.h>

#ifdef ROCKBOX
/* Rockbox lacks strncpy in its libc */
#define strncpy my_strncpy
static char* my_strncpy(char* dest, const char* src, size_t n) {
    size_t i;

    for(i = 0; i < n && *src; ++i)
        dest[i] = src[i];

    for(; i < n; ++i)
        dest[i] = 0;

    return dest;
}
#endif

enum {
    S_HEADER_VALID   = 1 << 0,
    S_WROTE_HEADER   = 1 << 1,
    S_WROTE_DATA     = 1 << 2,
    S_WROTE_DATA_EOF = 1 << 3,
    S_WROTE_FINALIZE = 1 << 4,
};

enum {
    NAME_OFF     = 0,                           NAME_LEN = 100,
    MODE_OFF     = NAME_OFF+NAME_LEN,           MODE_LEN = 8,
    OWNER_OFF    = MODE_OFF+MODE_LEN,           OWNER_LEN = 8,
    GROUP_OFF    = OWNER_OFF+OWNER_LEN,         GROUP_LEN = 8,
    SIZE_OFF     = GROUP_OFF+GROUP_LEN,         SIZE_LEN = 12,
    MTIME_OFF    = SIZE_OFF+SIZE_LEN,           MTIME_LEN = 12,
    CHKSUM_OFF   = MTIME_OFF+MTIME_LEN,         CHKSUM_LEN = 8,
    TYPE_OFF     = CHKSUM_OFF+CHKSUM_LEN,
    LINKNAME_OFF = TYPE_OFF+1,                  LINKNAME_LEN = 100,

    HEADER_LEN   = 512,
};

static int parse_octal(const char* str, size_t len, unsigned* ret)
{
    unsigned n = 0;

    while(len-- > 0 && *str != 0) {
        if(*str < '0' || *str > '9')
            return MTAR_EOVERFLOW;

        if(n > UINT_MAX/8)
            return MTAR_EOVERFLOW;
        else
            n *= 8;

        char r = *str++ - '0';

        if(n > UINT_MAX - r)
            return MTAR_EOVERFLOW;
        else
            n += r;
    }

    *ret = n;
    return MTAR_ESUCCESS;
}

static int print_octal(char* str, size_t len, unsigned value)
{
    /* move backwards over the output string */
    char* ptr = str + len - 1;
    *ptr = 0;

    /* output the significant digits */
    while(value > 0) {
        if(ptr == str)
            return MTAR_EOVERFLOW;

        --ptr;
        *ptr = '0' + (value % 8);
        value /= 8;
    }

    /* pad the remainder of the field with zeros */
    while(ptr != str) {
        --ptr;
        *ptr = '0';
    }

    return MTAR_ESUCCESS;
}

static unsigned round_up_512(unsigned n)
{
    return (n + 511u) & ~511u;
}

static int tread(mtar_t* tar, void* data, unsigned size)
{
    int ret = tar->ops->read(tar->stream, data, size);
    if(ret >= 0)
        tar->pos += ret;

    return ret;
}

static int twrite(mtar_t* tar, const void* data, unsigned size)
{
    int ret = tar->ops->write(tar->stream, data, size);
    if(ret >= 0)
        tar->pos += ret;

    return ret;
}

static int tseek(mtar_t* tar, unsigned pos)
{
    int err = tar->ops->seek(tar->stream, pos);
    tar->pos = pos;
    return err;
}

static int write_null_bytes(mtar_t* tar, size_t count)
{
    int ret;
    size_t n;

    memset(tar->buffer, 0, sizeof(tar->buffer));
    while(count > 0) {
        n = count < sizeof(tar->buffer) ? count : sizeof(tar->buffer);
        ret = twrite(tar, tar->buffer, n);
        if(ret < 0)
            return ret;
        if(ret != (int)n)
            return MTAR_EWRITEFAIL;

        count -= n;
    }

    return MTAR_ESUCCESS;
}

static unsigned checksum(const char* raw)
{
    unsigned i;
    unsigned char* p = (unsigned char*)raw;
    unsigned res = 256;

    for(i = 0; i < CHKSUM_OFF; i++)
        res += p[i];
    for(i = TYPE_OFF; i < HEADER_LEN; i++)
        res += p[i];

    return res;
}

static int raw_to_header(mtar_header_t* h, const char* raw)
{
    unsigned chksum;
    int rc;

    /* If the checksum starts with a null byte we assume the record is NULL */
    if(raw[CHKSUM_OFF] == '\0')
        return MTAR_ENULLRECORD;

    /* Compare the checksum */
    if((rc = parse_octal(&raw[CHKSUM_OFF], CHKSUM_LEN, &chksum)))
       return rc;
    if(chksum != checksum(raw))
        return MTAR_EBADCHKSUM;

    /* Load raw header into header */
    if((rc = parse_octal(&raw[MODE_OFF], MODE_LEN, &h->mode)))
        return rc;
    if((rc = parse_octal(&raw[OWNER_OFF], OWNER_LEN, &h->owner)))
        return rc;
    if((rc = parse_octal(&raw[GROUP_OFF], GROUP_LEN, &h->group)))
        return rc;
    if((rc = parse_octal(&raw[SIZE_OFF], SIZE_LEN, &h->size)))
        return rc;
    if((rc = parse_octal(&raw[MTIME_OFF], MTIME_LEN, &h->mtime)))
        return rc;

    h->type = raw[TYPE_OFF];
    if(!h->type)
        h->type = MTAR_TREG;

    memcpy(h->name, &raw[NAME_OFF], NAME_LEN);
    h->name[sizeof(h->name) - 1] = 0;

    memcpy(h->linkname, &raw[LINKNAME_OFF], LINKNAME_LEN);
    h->linkname[sizeof(h->linkname) - 1] = 0;

    return MTAR_ESUCCESS;
}

static int header_to_raw(char* raw, const mtar_header_t* h)
{
    unsigned chksum;
    int rc;

    memset(raw, 0, HEADER_LEN);

    /* Load header into raw header */
    if((rc = print_octal(&raw[MODE_OFF], MODE_LEN, h->mode)))
        return rc;
    if((rc = print_octal(&raw[OWNER_OFF], OWNER_LEN, h->owner)))
        return rc;
    if((rc = print_octal(&raw[GROUP_OFF], GROUP_LEN, h->group)))
        return rc;
    if((rc = print_octal(&raw[SIZE_OFF], SIZE_LEN, h->size)))
        return rc;
    if((rc = print_octal(&raw[MTIME_OFF], MTIME_LEN, h->mtime)))
        return rc;

    raw[TYPE_OFF] = h->type ? h->type : MTAR_TREG;

#if defined(__GNUC__) && (__GNUC__ >= 8)
/* Sigh. GCC wrongly assumes the output of strncpy() is supposed to be
 * a null-terminated string -- which it is not, and we are relying on
 * that fact here -- and tries to warn about 'string truncation' because
 * the null terminator might not be copied. Just suppress the warning. */
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

    strncpy(&raw[NAME_OFF], h->name, NAME_LEN);
    strncpy(&raw[LINKNAME_OFF], h->linkname, LINKNAME_LEN);

#if defined(__GNUC__) && (__GNUC__ >= 8)
# pragma GCC diagnostic pop
#endif

    /* Calculate and write checksum */
    chksum = checksum(raw);
    if((rc = print_octal(&raw[CHKSUM_OFF], CHKSUM_LEN-1, chksum)))
        return rc;

    raw[CHKSUM_OFF + CHKSUM_LEN - 1] = ' ';

    return MTAR_ESUCCESS;
}

static unsigned data_beg_pos(const mtar_t* tar)
{
    return tar->header_pos + HEADER_LEN;
}

static unsigned data_end_pos(const mtar_t* tar)
{
    return tar->end_pos;
}

static int ensure_header(mtar_t* tar)
{
    int ret, err;

    if(tar->state & S_HEADER_VALID)
        return MTAR_ESUCCESS;

    if(tar->pos > UINT_MAX - HEADER_LEN)
        return MTAR_EOVERFLOW;

    tar->header_pos = tar->pos;
    tar->end_pos = data_beg_pos(tar);

    ret = tread(tar, tar->buffer, HEADER_LEN);
    if(ret < 0)
        return ret;
    if(ret != HEADER_LEN)
        return MTAR_EREADFAIL;

    err = raw_to_header(&tar->header, tar->buffer);
    if(err)
        return err;

    if(tar->end_pos > UINT_MAX - tar->header.size)
        return MTAR_EOVERFLOW;
    tar->end_pos += tar->header.size;

    tar->state |= S_HEADER_VALID;
    return MTAR_ESUCCESS;
}

const char* mtar_strerror(int err)
{
    switch(err) {
    case MTAR_ESUCCESS:     return "success";
    case MTAR_EFAILURE:     return "failure";
    case MTAR_EOPENFAIL:    return "could not open";
    case MTAR_EREADFAIL:    return "could not read";
    case MTAR_EWRITEFAIL:   return "could not write";
    case MTAR_ESEEKFAIL:    return "could not seek";
    case MTAR_ESEEKRANGE:   return "seek out of bounds";
    case MTAR_EBADCHKSUM:   return "bad checksum";
    case MTAR_ENULLRECORD:  return "null record";
    case MTAR_ENOTFOUND:    return "file not found";
    case MTAR_EOVERFLOW:    return "overflow";
    case MTAR_EAPI:         return "API usage error";
    case MTAR_ENAMETOOLONG: return "name too long";
    case MTAR_EWRONGSIZE:   return "wrong amount of data written";
    case MTAR_EACCESS:      return "wrong access mode";
    default:                return "unknown error";
    }
}

void mtar_init(mtar_t* tar, int access, const mtar_ops_t* ops, void* stream)
{
    memset(tar, 0, sizeof(mtar_t));
    tar->access = access;
    tar->ops = ops;
    tar->stream = stream;
}

int mtar_close(mtar_t* tar)
{
    int err = tar->ops->close(tar->stream);
    tar->ops = NULL;
    tar->stream = NULL;
    return err;
}

int mtar_is_open(mtar_t* tar)
{
    return (tar->ops != NULL) ? 1 : 0;
}

mtar_header_t* mtar_get_header(mtar_t* tar)
{
    if(tar->state & S_HEADER_VALID)
        return &tar->header;
    else
        return NULL;
}

int mtar_access_mode(const mtar_t* tar)
{
    return tar->access;
}

int mtar_rewind(mtar_t* tar)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_READ)
        return MTAR_EAPI;
#endif

    int err = tseek(tar, 0);
    tar->state = 0;
    return err;
}

int mtar_next(mtar_t* tar)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_READ)
        return MTAR_EACCESS;
#endif

    if(tar->state & S_HEADER_VALID) {
        tar->state &= ~S_HEADER_VALID;

        /* seek to the next header */
        int err = tseek(tar, round_up_512(data_end_pos(tar)));
        if(err)
            return err;
    }

    return ensure_header(tar);
}

int mtar_foreach(mtar_t* tar, mtar_foreach_cb cb, void* arg)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_READ)
        return MTAR_EACCESS;
#endif

    int err = mtar_rewind(tar);
    if(err)
        return err;

    while((err = mtar_next(tar)) == MTAR_ESUCCESS)
        if((err = cb(tar, &tar->header, arg)) != 0)
            return err;

    if(err == MTAR_ENULLRECORD)
        err = MTAR_ESUCCESS;

    return err;
}

static int find_foreach_cb(mtar_t* tar, const mtar_header_t* h, void* arg)
{
    (void)tar;
    const char* name = (const char*)arg;
    return strcmp(name, h->name) ? 0 : 1;
}

int mtar_find(mtar_t* tar, const char* name)
{
    int err = mtar_foreach(tar, find_foreach_cb, (void*)name);
    if(err == 1)
        err = MTAR_ESUCCESS;
    else if(err == MTAR_ESUCCESS)
        err = MTAR_ENOTFOUND;

    return err;
}

int mtar_read_data(mtar_t* tar, void* ptr, unsigned size)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(!(tar->state & S_HEADER_VALID))
        return MTAR_EAPI;
#endif

    /* have we reached end of file? */
    unsigned data_end = data_end_pos(tar);
    if(tar->pos >= data_end)
        return 0;

    /* truncate the read if it would go beyond EOF */
    unsigned data_left = data_end - tar->pos;
    if(data_left < size)
        size = data_left;

    return tread(tar, ptr, size);
}

int mtar_seek_data(mtar_t* tar, int offset, int whence)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(!(tar->state & S_HEADER_VALID))
        return MTAR_EAPI;
#endif

    unsigned data_beg = data_beg_pos(tar);
    unsigned data_end = data_end_pos(tar);
    unsigned newpos;

    switch(whence) {
    case SEEK_SET:
        if(offset < 0)
            return MTAR_ESEEKRANGE;

        newpos = data_beg + offset;
        break;

    case SEEK_CUR:
        if((offset > 0 && (unsigned) offset > data_end - tar->pos) ||
           (offset < 0 && (unsigned)-offset > tar->pos - data_beg))
            return MTAR_ESEEKRANGE;

        newpos = tar->pos + offset;
        break;

    case SEEK_END:
        if(offset > 0)
            return MTAR_ESEEKRANGE;

        newpos = data_end + offset;
        break;

    default:
        return MTAR_EAPI;
    }

    return tseek(tar, newpos);
}

unsigned mtar_tell_data(mtar_t* tar)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(!(tar->state & S_HEADER_VALID))
        return MTAR_EAPI;
#endif

    return tar->pos - data_beg_pos(tar);
}

int mtar_eof_data(mtar_t* tar)
{
    /* API usage error, but just claim EOF. */
    if(!(tar->state & S_HEADER_VALID))
        return 1;

    return tar->pos >= data_end_pos(tar) ? 1 : 0;
}

int mtar_write_header(mtar_t* tar, const mtar_header_t* h)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(((tar->state & S_WROTE_DATA) && !(tar->state & S_WROTE_DATA_EOF)) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    tar->state &= ~(S_HEADER_VALID | S_WROTE_HEADER |
                    S_WROTE_DATA | S_WROTE_DATA_EOF);

    /* ensure we have enough space to write the declared amount of data */
    if(tar->pos > UINT_MAX - HEADER_LEN - round_up_512(h->size))
        return MTAR_EOVERFLOW;

    tar->header_pos = tar->pos;
    tar->end_pos = data_beg_pos(tar);

    if(h != &tar->header)
        tar->header = *h;

    int err = header_to_raw(tar->buffer, &tar->header);
    if(err)
        return err;

    int ret = twrite(tar, tar->buffer, HEADER_LEN);
    if(ret < 0)
        return ret;
    if(ret != HEADER_LEN)
        return MTAR_EWRITEFAIL;

    tar->state |= (S_HEADER_VALID | S_WROTE_HEADER);
    return MTAR_ESUCCESS;
}

int mtar_update_header(mtar_t* tar, const mtar_header_t* h)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(!(tar->state & S_WROTE_HEADER) ||
       (tar->state & S_WROTE_DATA_EOF) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    unsigned beg_pos = data_beg_pos(tar);
    if(beg_pos > UINT_MAX - h->size)
        return MTAR_EOVERFLOW;

    unsigned old_pos = tar->pos;
    int err = tseek(tar, tar->header_pos);
    if(err)
        return err;

    if(h != &tar->header)
        tar->header = *h;

    err = header_to_raw(tar->buffer, &tar->header);
    if(err)
        return err;

    int len = twrite(tar, tar->buffer, HEADER_LEN);
    if(len < 0)
        return len;
    if(len != HEADER_LEN)
        return MTAR_EWRITEFAIL;

    return tseek(tar, old_pos);
}

int mtar_write_file_header(mtar_t* tar, const char* name, unsigned size)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(((tar->state & S_WROTE_DATA) && !(tar->state & S_WROTE_DATA_EOF)) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    size_t namelen = strlen(name);
    if(namelen > NAME_LEN)
        return MTAR_ENAMETOOLONG;

    tar->header.mode = 0644;
    tar->header.owner = 0;
    tar->header.group = 0;
    tar->header.size = size;
    tar->header.mtime = 0;
    tar->header.type = MTAR_TREG;
    memcpy(tar->header.name, name, namelen + 1);
    tar->header.linkname[0] = '\0';

    return mtar_write_header(tar, &tar->header);
}

int mtar_write_dir_header(mtar_t* tar, const char* name)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(((tar->state & S_WROTE_DATA) && !(tar->state & S_WROTE_DATA_EOF)) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    size_t namelen = strlen(name);
    if(namelen > NAME_LEN)
        return MTAR_ENAMETOOLONG;

    tar->header.mode = 0755;
    tar->header.owner = 0;
    tar->header.group = 0;
    tar->header.size = 0;
    tar->header.mtime = 0;
    tar->header.type = MTAR_TDIR;
    memcpy(tar->header.name, name, namelen + 1);
    tar->header.linkname[0] = '\0';

    return mtar_write_header(tar, &tar->header);
}

int mtar_write_data(mtar_t* tar, const void* ptr, unsigned size)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(!(tar->state & S_WROTE_HEADER) ||
       (tar->state & S_WROTE_DATA_EOF) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    tar->state |= S_WROTE_DATA;

    int err = twrite(tar, ptr, size);
    if(tar->pos > tar->end_pos)
        tar->end_pos = tar->pos;

    return err;
}

int mtar_update_file_size(mtar_t* tar)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(!(tar->state & S_WROTE_HEADER) ||
       (tar->state & S_WROTE_DATA_EOF) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    unsigned new_size = data_end_pos(tar) - data_beg_pos(tar);
    if(new_size == tar->header.size)
        return MTAR_ESUCCESS;
    else {
        tar->header.size = new_size;
        return mtar_update_header(tar, &tar->header);
    }
}

int mtar_end_data(mtar_t* tar)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(!(tar->state & S_WROTE_HEADER) ||
       (tar->state & S_WROTE_DATA_EOF) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    int err;

    /* ensure the caller wrote the correct amount of data */
    unsigned expected_end = data_beg_pos(tar) + tar->header.size;
    if(tar->end_pos != expected_end)
        return MTAR_EWRONGSIZE;

    /* ensure we're positioned at the end of the stream */
    if(tar->pos != tar->end_pos) {
        err = tseek(tar, tar->end_pos);
        if(err)
            return err;
    }

    /* write remainder of the 512-byte record */
    err = write_null_bytes(tar, round_up_512(tar->pos) - tar->pos);
    if(err)
        return err;

    tar->state |= S_WROTE_DATA_EOF;
    return MTAR_ESUCCESS;
}

int mtar_finalize(mtar_t* tar)
{
#ifndef MICROTAR_DISABLE_API_CHECKS
    if(tar->access != MTAR_WRITE)
        return MTAR_EACCESS;
    if(((tar->state & S_WROTE_DATA) && !(tar->state & S_WROTE_DATA_EOF)) ||
       (tar->state & S_WROTE_FINALIZE))
        return MTAR_EAPI;
#endif

    tar->state |= S_WROTE_FINALIZE;
    return write_null_bytes(tar, 1024);
}
