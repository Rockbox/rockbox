/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef __PARTITIONS_IMX233__
#define __PARTITIONS_IMX233__

#include "system.h"
#include "storage.h"

#ifndef IMX233_PARTITIONS
#error You must define IMX233_PARTITIONS
#endif

enum imx233_part_t
{
#if (IMX233_PARTITIONS & IMX233_FREESCALE)
    IMX233_PART_BOOT,
    IMX233_PART_DATA,
    IMX233_PART_USER = IMX233_PART_DATA,
#endif
#if (IMX233_PARTITIONS & IMX233_CREATIVE)
    IMX233_PART_CFS,
    IMX233_PART_MINIFS,
    IMX233_PART_USER = IMX233_PART_CFS,
#endif
};

/** The computation function can be called very early in the boot, at which point
 * usual storage read/write function may not be available. To workaround this
 * issue, one must provide a read function. */
typedef int (*part_read_fn_t)(intptr_t user, unsigned long start, int count, void* buf);
/* Enable/Disable window computations for internal storage following the
 * Freescale/Creative convention */
void imx233_partitions_enable_window(bool enable);
bool imx233_partitions_is_window_enabled(void);
/* Compute the window size. The *start and *end parameters should contain
 * the initial window in which the computation is done. So in particular,
 * for a whole disk, *end should be the size of the disk when the function is
 * called */
int imx233_partitions_compute_window(intptr_t user, part_read_fn_t read_fn,
    enum imx233_part_t part, unsigned *start, unsigned *end);

#endif /* __PARTITIONS_IMX233__ */