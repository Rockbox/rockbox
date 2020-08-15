/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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

#include <stdint.h>

#define MI4_HEADER_SIZE     0x200

/* mi4 header structure */
struct mi4header_t {
    unsigned char magic[4];
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
    uint32_t enctype;
    uint32_t mi4size;
    uint32_t plaintext;
    uint32_t dsa_key[10];
    uint32_t pad[109];
    unsigned char type[4];
    unsigned char model[4];
};

struct tea_key {
  const char * name;
  uint32_t     key[4];
};

#define NUM_KEYS (sizeof(tea_keytable)/sizeof(tea_keytable[0]))

int load_mi4(unsigned char* buf, const char* firmware, unsigned int buffer_size);
const char *mi4_strerror(int8_t errno);

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

/* TODO needs mapped back to debug_menu if root redirect ever becomes a reality */
int get_redirect_dir(char* buf, int buffer_size, int volume,
                     const char* rootdir, const char* firmware);
#endif
