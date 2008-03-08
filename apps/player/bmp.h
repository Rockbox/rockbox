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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _BMP_H_
#define _BMP_H_

/*********************************************************************
 * read_bmp_file(), minimalistic version for charcell displays
 *
 * Reads a 1 bit BMP file and puts the data in a horizontal packed
 * 1 bit-per-pixel char array. Width must be <= 8 pixels.
 * Returns < 0 for error, or number of bytes used from the bitmap
 * buffer, which equals bitmap height.
 *
 **********************************************/
int read_bmp_file(const char* filename,
                  unsigned char *bitmap,
                  int maxsize);

#endif
