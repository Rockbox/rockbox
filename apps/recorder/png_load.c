/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
*
* PNG image viewer
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

#include "config.h"
#include "system.h"

#include "plugin.h"
#include "resize.h"
#include "bmp.h"
#include "png_load.h"

int clip_png_fd(int fd,
                unsigned long jpeg_size,
                struct bitmap *bm,
                int maxsize,
                int format,
                const struct custom_format *cformat)
{
	// PNG_TODO
}

int clip_png_file(const char* filename,
                  int offset,
                  unsigned long jpeg_size,
                  struct bitmap *bm,
                  int maxsize,
                  int format,
                  const struct custom_format *cformat)
{
    int fd, ret;
    fd = open(filename, O_RDONLY);
    DEBUGF("read_png_file: filename: %s buffer len: %d cformat: %p\n",
        filename, maxsize, cformat);
    /* Exit if file opening failed */
    if (fd < 0) {
        DEBUGF("read_png_file: can't open '%s', rc: %d\n", filename, fd);
        return fd * 10 - 1;
    }
    lseek(fd, offset, SEEK_SET);
    ret = clip_png_fd(fd, jpeg_size, bm, maxsize, format, cformat);
    close(fd);
    return ret;
}

int read_png_file(const char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format,
                  const struct custom_format *cformat)
{
    return clip_png_file(filename, 0, 0, bm, maxsize, format, cformat);
}

int read_png_fd(int fd,
                struct bitmap *bm,
                int maxsize,
                int format,
                const struct custom_format *cformat)
{
    return clip_jpeg_fd(fd, 0, bm, maxsize, format, cformat);
}
