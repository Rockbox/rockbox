/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "bbfuncs.h"

#if defined(ROCKBOX)
#include "codeclib.h"
#endif

void error_die(const char* msg)
{	
    (void)msg;
}

void error_msg(const char* msg)
{
	(void)msg;
}

size_t safe_read(struct mbreader_t *md, void *buf, size_t count)
{
    ssize_t n;

    do {
        n = mbread(md, buf, count);
    } while (n < 0&&n!=-1);

    return n;
}

/*
 * Read all of the supplied buffer from a file This does multiple reads as
 *necessary. Returns the amount read, or -1 on an error. A short read is
 *returned on an end of file.
 */
ssize_t full_read(struct mbreader_t *md, void *buf, size_t len)
{
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len)
    {
        cc = safe_read(md, buf, len);

        if (cc < 0)
            return cc;    /* read() returns -1 on failure. */

        if (cc == 0)
            break;

        buf = ((char *)buf) + cc;
        total += cc;
        len -= cc;
    }

    return total;
}

/* Die with an error message if we can't read the entire buffer. */
void xread(struct mbreader_t *md, void *buf, ssize_t count)
{
    if (count)
    {
        ssize_t size = full_read(md, buf, count);
        if (size != count)
            error_die("short read");
    }
}

/* Die with an error message if we can't read one character. */
unsigned char xread_char(struct mbreader_t *md)
{
    unsigned char tmp;

    xread(md, &tmp, 1);

    return tmp;
}

void check_header_gzip(struct mbreader_t *src_md)
{
    union {
        unsigned char raw[8];
        struct {
            unsigned char method;
            unsigned char flags;
            unsigned int mtime;
            unsigned char xtra_flags;
            unsigned char os_flags;
        } formatted;
    } header;

    xread(src_md, header.raw, 8);

    /* Check the compression method */
    if (header.formatted.method != 8)
        error_die("Unknown compression method");

    if (header.formatted.flags & 0x04)
    {
        /* bit 2 set: extra field present */
        unsigned char extra_short;

        extra_short = xread_char(src_md) + (xread_char(src_md) << 8);
        while (extra_short > 0)
        {
            /* Ignore extra field */
            xread_char(src_md);
            extra_short--;
        }
    }

    /* Discard original name if any */
    if (header.formatted.flags & 0x08)
        /* bit 3 set: original file name present */
        while(xread_char(src_md) != 0) ;

    /* Discard file comment if any */
    if (header.formatted.flags & 0x10)
        /* bit 4 set: file comment present */
        while(xread_char(src_md) != 0) ;

    /* Read the header checksum */
    if (header.formatted.flags & 0x02)
    {
        xread_char(src_md);
        xread_char(src_md);
    }
}
