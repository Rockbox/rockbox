/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 Thom Johansen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef RBSPEEX_H
#define RBSPEEX_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned int get_long_le(unsigned char *p);
bool get_wave_metadata(FILE *fd, int *numchan, int *bps, int *sr, int *numsamples);
bool encode_file(FILE *fin, FILE *fout, float quality, int complexity,
                 bool narrowband, float volume, char *errstr, size_t errlen);

void put_ushort_le(unsigned short x, unsigned char *out);
void put_uint_le(unsigned int x, unsigned char *out);

#ifdef __cplusplus
}
#endif
#endif

