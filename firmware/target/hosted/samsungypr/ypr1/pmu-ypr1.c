/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "pmu-ypr1.h"
#include "panic.h"

static int pmu_dev = -1;

void pmu_init(void)
{
    pmu_dev = open("/dev/r1Pmu", O_RDONLY);
    if (pmu_dev < 0)
        panicf("/dev/r1Pmu open error!");
}

void pmu_close(void)
{
    if (pmu_dev >= 0)
        close(pmu_dev);
}

int pmu_get_dev(void)
{
    return pmu_dev;
}

int pmu_ioctl(int request, int *data)
{
    return ioctl(pmu_dev, request, data);
}
