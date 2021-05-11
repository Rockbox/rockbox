/*
 * Copyright (c) 2017 rxi
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

#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>

#include "microtar.h"

#ifdef ROCKBOX
/* Rockbox lacks strncpy in its libc */
static char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;

    for(i = 0; i < n && *src; ++i)
        dest[i] = src[i];

    for(; i < n; ++i)
        dest[i] = 0;

    return dest;
}
#endif


static int parse_octal(const char* str, size_t len, unsigned* ret) {
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


static int print_octal(char* str, size_t len, unsigned value) {
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


static unsigned round_up(unsigned n, unsigned incr) {
  return n + (incr - n % incr) % incr;
}


static unsigned checksum(const mtar_raw_header_t* rh) {
  unsigned i;
  unsigned char *p = (unsigned char*) rh;
  unsigned res = 256;
  for (i = 0; i < offsetof(mtar_raw_header_t, checksum); i++) {
    res += p[i];
  }
  for (i = offsetof(mtar_raw_header_t, type); i < sizeof(*rh); i++) {
    res += p[i];
  }
  return res;
}


static int tread(mtar_t *tar, void *data, unsigned size) {
  int err = tar->read(tar, data, size);
  tar->pos += size;
  return err;
}


static int twrite(mtar_t *tar, const void *data, unsigned size) {
  int err = tar->write(tar, data, size);
  tar->pos += size;
  return err;
}


static int write_null_bytes(mtar_t *tar, int n) {
  int i, err;
  char nul = '\0';
  for (i = 0; i < n; i++) {
    err = twrite(tar, &nul, 1);
    if (err) {
      return err;
    }
  }
  return MTAR_ESUCCESS;
}


static int raw_to_header(mtar_header_t *h, const mtar_raw_header_t *rh) {
  unsigned chksum1, chksum2;
  int rc;

  /* If the checksum starts with a null byte we assume the record is NULL */
  if (*rh->checksum == '\0') {
    return MTAR_ENULLRECORD;
  }

  /* Build and compare checksum */
  chksum1 = checksum(rh);
  if((rc = parse_octal(rh->checksum, sizeof(rh->checksum), &chksum2)))
      return rc;
  if (chksum1 != chksum2) {
    return MTAR_EBADCHKSUM;
  }

  /* Load raw header into header */
  if((rc = parse_octal(rh->mode, sizeof(rh->mode), &h->mode)))
    return rc;
  if((rc = parse_octal(rh->owner, sizeof(rh->owner), &h->owner)))
    return rc;
  if((rc = parse_octal(rh->group, sizeof(rh->group), &h->group)))
    return rc;
  if((rc = parse_octal(rh->size, sizeof(rh->size), &h->size)))
    return rc;
  if((rc = parse_octal(rh->mtime, sizeof(rh->mtime), &h->mtime)))
    return rc;

  h->type = rh->type;

  memcpy(h->name, rh->name, sizeof(rh->name));
  h->name[sizeof(h->name) - 1] = 0;

  memcpy(h->linkname, rh->linkname, sizeof(rh->linkname));
  h->linkname[sizeof(h->linkname) - 1] = 0;

  return MTAR_ESUCCESS;
}


static int header_to_raw(mtar_raw_header_t *rh, const mtar_header_t *h) {
  unsigned chksum;
  int rc;

  /* Load header into raw header */
  memset(rh, 0, sizeof(*rh));
  if((rc = print_octal(rh->mode, sizeof(rh->mode), h->mode)))
    return rc;
  if((rc = print_octal(rh->owner, sizeof(rh->owner), h->owner)))
    return rc;
  if((rc = print_octal(rh->group, sizeof(rh->group), h->group)))
    return rc;
  if((rc = print_octal(rh->size, sizeof(rh->size), h->size)))
    return rc;
  if((rc = print_octal(rh->mtime, sizeof(rh->mtime), h->mtime)))
    return rc;
  rh->type = h->type ? h->type : MTAR_TREG;
  strncpy(rh->name, h->name, sizeof(rh->name));
  strncpy(rh->linkname, h->linkname, sizeof(rh->linkname));

  /* Calculate and write checksum */
  chksum = checksum(rh);
  if((rc = print_octal(rh->checksum, 7, chksum)))
     return rc;
  rh->checksum[7] = ' ';

  return MTAR_ESUCCESS;
}


const char* mtar_strerror(int err) {
  switch (err) {
    case MTAR_ESUCCESS     : return "success";
    case MTAR_EFAILURE     : return "failure";
    case MTAR_EOPENFAIL    : return "could not open";
    case MTAR_EREADFAIL    : return "could not read";
    case MTAR_EWRITEFAIL   : return "could not write";
    case MTAR_ESEEKFAIL    : return "could not seek";
    case MTAR_EBADCHKSUM   : return "bad checksum";
    case MTAR_ENULLRECORD  : return "null record";
    case MTAR_ENOTFOUND    : return "file not found";
    case MTAR_EOVERFLOW    : return "overflow";
  }
  return "unknown error";
}


int mtar_close(mtar_t *tar) {
  return tar->close(tar);
}


int mtar_seek(mtar_t *tar, unsigned pos) {
  int err = tar->seek(tar, pos);
  tar->pos = pos;
  return err;
}


int mtar_rewind(mtar_t *tar) {
  tar->remaining_data = 0;
  tar->last_header = 0;
  return mtar_seek(tar, 0);
}


int mtar_next(mtar_t *tar) {
  int err, n;
  /* Load header */
  err = mtar_read_header(tar, &tar->header);
  if (err) {
    return err;
  }
  /* Seek to next record */
  n = round_up(tar->header.size, 512) + sizeof(mtar_raw_header_t);
  return mtar_seek(tar, tar->pos + n);
}


int mtar_find(mtar_t *tar, const char *name, mtar_header_t *h) {
  int err;
  /* Start at beginning */
  err = mtar_rewind(tar);
  if (err) {
    return err;
  }
  /* Iterate all files until we hit an error or find the file */
  while ( (err = mtar_read_header(tar, &tar->header)) == MTAR_ESUCCESS ) {
    if ( !strcmp(tar->header.name, name) ) {
      if (h) {
        *h = tar->header;
      }
      return MTAR_ESUCCESS;
    }
    err = mtar_next(tar);
    if (err) {
      return err;
    }
  }
  /* Return error */
  if (err == MTAR_ENULLRECORD) {
    err = MTAR_ENOTFOUND;
  }
  return err;
}


int mtar_read_header(mtar_t *tar, mtar_header_t *h) {
  int err;
  /* Save header position */
  tar->last_header = tar->pos;
  /* Read raw header */
  err = tread(tar, &tar->raw_header, sizeof(tar->raw_header));
  if (err) {
    return err;
  }
  /* Seek back to start of header */
  err = mtar_seek(tar, tar->last_header);
  if (err) {
    return err;
  }
  /* Load raw header into header struct and return */
  return raw_to_header(h, &tar->raw_header);
}


int mtar_read_data(mtar_t *tar, void *ptr, unsigned size) {
  int err;
  /* If we have no remaining data then this is the first read, we get the size,
   * set the remaining data and seek to the beginning of the data */
  if (tar->remaining_data == 0) {
    /* Read header */
    err = mtar_read_header(tar, &tar->header);
    if (err) {
      return err;
    }
    /* Seek past header and init remaining data */
    err = mtar_seek(tar, tar->pos + sizeof(mtar_raw_header_t));
    if (err) {
      return err;
    }
    tar->remaining_data = tar->header.size;
  }
  /* Ensure caller does not read too much */
  if(size > tar->remaining_data)
    return MTAR_EOVERFLOW;
  /* Read data */
  err = tread(tar, ptr, size);
  if (err) {
    return err;
  }
  tar->remaining_data -= size;
  /* If there is no remaining data we've finished reading and seek back to the
   * header */
  if (tar->remaining_data == 0) {
    return mtar_seek(tar, tar->last_header);
  }
  return MTAR_ESUCCESS;
}


int mtar_write_header(mtar_t *tar, const mtar_header_t *h) {
  /* Build raw header and write */
  header_to_raw(&tar->raw_header, h);
  tar->remaining_data = h->size;
  return twrite(tar, &tar->raw_header, sizeof(tar->raw_header));
}


int mtar_write_file_header(mtar_t *tar, const char *name, unsigned size) {
  /* Build header */
  memset(&tar->header, 0, sizeof(tar->header));
  /* Ensure name fits within header */
  if(strlen(name) > sizeof(tar->header.name))
    return MTAR_EOVERFLOW;
  strncpy(tar->header.name, name, sizeof(tar->header.name));
  tar->header.size = size;
  tar->header.type = MTAR_TREG;
  tar->header.mode = 0664;
  /* Write header */
  return mtar_write_header(tar, &tar->header);
}


int mtar_write_dir_header(mtar_t *tar, const char *name) {
  /* Build header */
  memset(&tar->header, 0, sizeof(tar->header));
  /* Ensure name fits within header */
  if(strlen(name) > sizeof(tar->header.name))
    return MTAR_EOVERFLOW;
  strncpy(tar->header.name, name, sizeof(tar->header.name));
  tar->header.type = MTAR_TDIR;
  tar->header.mode = 0775;
  /* Write header */
  return mtar_write_header(tar, &tar->header);
}


int mtar_write_data(mtar_t *tar, const void *data, unsigned size) {
  int err;

  /* Ensure we are writing the correct amount of data */
  if(size > tar->remaining_data)
    return MTAR_EOVERFLOW;

  /* Write data */
  err = twrite(tar, data, size);
  if (err) {
    return err;
  }
  tar->remaining_data -= size;
  /* Write padding if we've written all the data for this file */
  if (tar->remaining_data == 0) {
    return write_null_bytes(tar, round_up(tar->pos, 512) - tar->pos);
  }
  return MTAR_ESUCCESS;
}


int mtar_finalize(mtar_t *tar) {
  /* Write two NULL records */
  return write_null_bytes(tar, sizeof(mtar_raw_header_t) * 2);
}
