/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Amaury Pouly
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
#ifndef __FW_H__
#define __FW_H__

#include <stdint.h>

/* Unpack an AFI file: the callback function will be called once for each file in the archive with
 * its name and content. If the callback returns a nonzero value, the function will stop and return
 * that value. Returns 0 on success */
typedef int (*fw_extract_callback_t)(const char *name, uint8_t *buf, size_t size);
int fw_unpack(uint8_t *buf, size_t size, fw_extract_callback_t cb, bool big_endian);
/* Check if a file looks like an AFI file */
bool fw_check(uint8_t *buf, size_t size);
/* Find file 'name' (8.3, any case) in an LFI image (f2 variant): *data
 * points into buf, *data_size is its real length. Returns 0 if found. */
int fw_find(const uint8_t *buf, size_t size, const char *name,
            const uint8_t **data, size_t *data_size);
/* Replace the content of file 'name' (8.3, any case) in an LFI image (f2
 * variant): the other files stay where they are, the file goes to its old
 * place if it fits, else after the last file; the checksums BREC checks are
 * recomputed. *out is malloc()ed. Returns 0 on success. */
int fw_replace(const uint8_t *buf, size_t size, const char *name,
               const uint8_t *data, size_t data_size, uint8_t **out,
               size_t *out_size);

#endif /* __FW_H__ */ 
