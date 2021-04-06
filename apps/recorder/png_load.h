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

#include "resize.h"
#include "bmp.h"

#ifndef _PNG_LOAD_H
#define _PNG_LOAD_H

int read_png_file(const char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format,
                  const struct custom_format *cformat);

int read_png_fd(int fd,
                struct bitmap *bm,
                int maxsize,
                int format,
                const struct custom_format *cformat);

/**
 * read embedded png files as above. Needs an offset and length into
 * the file
 **/
int clip_png_file(const char* filename,
                  int offset,
                  unsigned long jpeg_size,
                  struct bitmap *bm,
                  int maxsize,
                   int format,
                   const struct custom_format *cformat);

/**
 * read embedded png files as above. Needs an open file descripter, and
 * assumes the caller has lseek()'d to the start of the png blob
 **/
int clip_png_fd(int fd,
                unsigned long jpeg_size,
                struct bitmap *bm,
                int maxsize,
                int format,
                const struct custom_format *cformat);

#endif /* _PNG_LOAD_H */
