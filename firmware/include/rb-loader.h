/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 1
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

int load_firmware(unsigned char* buf, const char* firmware, int buffer_size);

#ifdef HAVE_MULTIBOOT /* defined by config.h */
/* Check in root of this <volume> for rockbox_main.<playername>
 * if this file empty or there is a single slash '/'
 * buf = '<volume#>/<rootdir>/<firmware(name)>\0'
 * If instead '/<*DIRECTORY*>' is supplied
 * addpath will be set to this DIRECTORY buf =
 * '/<volume#>/addpath/<rootdir>/<firmware(name)>\0'
 * On error returns Negative number or 0
 * On success returns bytes from snprintf
 * and generated path will be placed in buf
 * note: if supplied buffer is too small return will be
 * the number of bytes that would have been written
 */
int get_redirect_dir(char* buf, int buffer_size, int volume,
                     const char* rootdir, const char* firmware);
#endif
