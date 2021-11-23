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

#ifndef _XF_UPDATE_H_
#define _XF_UPDATE_H_

#include "xf_package.h"
#include "xf_nandio.h"
#include "xf_flashmap.h"

typedef int(*xf_update_open_stream_cb)(void* arg, const char* file,
                                       struct xf_stream* stream);

enum xf_update_mode {
    XF_UPDATE,
    XF_BACKUP,
    XF_VERIFY,
};

/** The main updater entry point
 *
 * \param mode          Operational mode
 * \param nio           Initialized NAND I/O object.
 * \param map           Flash map describing what regions to update.
 * \param map_size      Number of entries in the map.
 * \param open_stream   Callback used to open a stream for each map entry.
 * \param arg           Argument passed to the `open_stream` callback.
 *
 * \returns XF_E_SUCCESS on success or a negative error code on failure.
 */
int xf_updater_run(enum xf_update_mode mode, struct xf_nandio* nio,
                   struct xf_map* map, int map_size,
                   xf_update_open_stream_cb open_stream, void* arg);

#endif /* _XF_UPDATE_H_ */
