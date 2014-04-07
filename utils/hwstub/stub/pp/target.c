/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#include "stddef.h"
#include "target.h"
#include "system.h"
#include "logf.h"
#include "memory.h"

/**
 *
 * Global
 *
 */

/* FIXME wrong for PP500x */
#define USEC_TIMER   (*(volatile unsigned long *)(0x60005010))

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_PP,
    "PP500x / PP502x / PP610x"
};

void target_init(void)
{
}

void target_get_desc(int desc, void **buffer)
{
    *buffer = NULL;
}

void target_get_config_desc(void *buffer, int *size)
{
}

void target_udelay(int us)
{
    uint32_t end = USEC_TIMER + us;
    while(USEC_TIMER <= end);
}

void target_mdelay(int ms)
{
    return target_udelay(ms * 1000);
}
