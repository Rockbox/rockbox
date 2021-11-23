/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef _XF_FLASHMAP_H_
#define _XF_FLASHMAP_H_

#include "xf_stream.h"
#include <stdint.h>

#define XF_MAP_NAMELEN 63

enum {
    XF_MAP_HAS_MD5         = 0x01, /* 'md5' field is valid */
    XF_MAP_HAS_FILE_LENGTH = 0x02, /* 'file_length' field is valid */
};

struct xf_map {
    char name[XF_MAP_NAMELEN+1]; /* file name */
    uint8_t md5[16];             /* MD5 sum of file */
    uint32_t file_length;        /* length of file in bytes */
    uint32_t offset;             /* offset in flash */
    uint32_t length;             /* region length in flash, in bytes */
    int flags;
};

/* Parse a line with space- or tab-delimited fields of the form
 *   <name> <md5> <file_length> <offset> <length>
 *   <name> '-' <offset> <length>
 *
 * - name can be up to XF_FMAP_NAMELEN characters long
 * - md5 is 32 hexadecimal characters (case insensitive)
 * - file_length, offset, and length are 32-bit unsigned integers
 *   and can be given in decimal or (with '0x' prefix) hexadecimal
 *
 * Parsed data is written to *map. Returns zero on success and
 * nonzero on error.
 */
int xf_map_parseline(const char* line, struct xf_map* map);

/* Parse a file calling xf_map_parseline() on each line to populate
 * a map of up to 'maxnum' regions. Blank and comment lines are
 * ignored (comments start with '#').
 *
 * Returns the number of regions in the resulting map if the file was
 * parsed successfully, or a negative value on error.
 */
int xf_map_parse(struct xf_stream* s, struct xf_map* map, int maxnum);

/* Sort the map so its members are in ascending order with the lowest
 * flash offset region first. After sorting, xf_map_validate() is used
 * to check for overlapping regions.
 *
 * The return value is that of xf_map_validate().
 */
int xf_map_sort(struct xf_map* map, int num);

/* Check if the input map is sorted and contains no overlap.
 *
 * Returns 0 if the map is sorted and contains no overlapping regions,
 * -1 if the map isn't sorted, or if an overlapping region is detected,
 * the index of the first overlapping region. (A returned index i is
 * always positive: the two overlapped entries are map[i] and map[i-1].)
 */
int xf_map_validate(const struct xf_map* map, int num);

/* Write the map to a stream. This does not check that the map is valid.
 * Returns the number of bytes written to the stream or a negative value
 * on error. The stream may be NULL, in which case the number of bytes
 * that would be written are returned (similar to snprintf).
 */
int xf_map_write(struct xf_map* map, int num, struct xf_stream* s);

#endif /* _XF_FLASHMAP_H_ */
