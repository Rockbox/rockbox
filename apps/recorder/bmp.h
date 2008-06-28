/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
#ifndef _BMP_H_
#define _BMP_H_

#include "config.h"
#include "lcd.h"

/*********************************************************************
 * read_bmp_file()
 *
 * Reads a 8bit BMP file and puts the data in a 1-pixel-per-byte
 * array.
 * Returns < 0 for error, or number of bytes used from the bitmap buffer
 *
 **********************************************/
int read_bmp_file(const char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format);

int read_bmp_fd(int fd,
                struct bitmap *bm,
                int maxsize,
                int format);
#endif
