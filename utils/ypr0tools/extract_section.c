/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Thomas Martitz
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* A simple replacement program for (
 * dd if=$file1 of=$file2 bs=1 skip=$offset count=$size
 *
 * Written because byte-size operations with dd are unbearably slow.
 */

void usage(void)
{
    fprintf(stderr, "Usage: extract_section <romfile> <outfile> <offset> <byte count>\n");
    exit(1);
}

void die(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

int main(int argc, const char* argv[])
{
    if (argc != 5)
        usage();

    int ifd, ofd;
    ssize_t size = atol(argv[4]);
    long skip = atol(argv[3]);

    if (!size)
        die("invalid byte count\n");

    ifd = open(argv[1], O_RDONLY);
    if (ifd < 0)
        die("Could not open %s for reading!\n", argv[1]);

    ofd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (ofd < 0)
        die("Could not create %s\n", argv[2]);

    void *buf = malloc(size);
    if (!buf) die("OOM\n");

    lseek(ifd, skip, SEEK_SET);
    lseek(ofd, 0, SEEK_SET);
    if (read(ifd, buf, size) != size)
        die("Read failed\n");
    if (write(ofd, buf, size) != size)
        die("write failed\n");

    close(ifd);
    close(ofd);

    exit(EXIT_SUCCESS);
}
