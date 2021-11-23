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

#ifndef _XF_PACKAGE_H_
#define _XF_PACKAGE_H_

/* package format
 *
 * - bootloader-info.txt (contains a version label and optional metadata)
 * - flashmap.txt (describes the flash update map)
 * - [package must contain any other files referenced by the flash map]
 * - [other files may be present, but are ignored]
 */

#include "xf_flashmap.h"
#include "xf_stream.h"
#include "microtar.h"

struct xf_package {
    int alloc_handle;
    mtar_t* tar;
    struct xf_map* map;
    int map_size;
    char* metadata;
    size_t metadata_len;
};

/** Open an update package
 *
 * \param pkg           Uninitialized package structure
 * \param file          Name of the package file
 * \param dflt_map      Default flash map for loading old format packages
 * \param dflt_map_size Size of the default flash map
 * \returns XF_E_SUCCESS or a negative error code
 */
int xf_package_open_ex(struct xf_package* pkg, const char* file,
                       const struct xf_map* dflt_map, int dflt_map_size);

/** Close a package which was previously opened successfully */
void xf_package_close(struct xf_package* pkg);

static inline int xf_package_open(struct xf_package* pkg, const char* file)
{
    return xf_package_open_ex(pkg, file, NULL, 0);
}

#endif /* _XF_PACKAGE_H_ */
