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
    IMX233_PART_USER,
#if (IMX233_PARTITIONS & IMX233_FREESCALE)
    IMX233_PART_BOOT,
#endif
#if (IMX233_PARTITIONS & IMX233_CREATIVE)
    IMX233_PART_CFS,
    IMX233_PART_MINIFS,
#endif
};

/* Enable/Disable window computations for internal storage following the
 * Freescale convention */
void imx233_partitions_enable_window(bool enable);
bool imx233_partitions_is_window_enabled(void);
int imx233_partitions_compute_window(IF_MD(int drive,) enum imx233_part_t part,
    unsigned *start, unsigned *end);

#endif /* __PARTITIONS_IMX233__ */