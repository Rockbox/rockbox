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

enum pp_family_t
{
    UNKNOWN,
    PP502x,
    PP611x
};

static enum pp_family_t g_pp_family = UNKNOWN;

#define USEC_TIMER      (*(volatile unsigned long *)(0x60005010))

// NOTE only valid for PP502x and above */
#define PP_VER1          (*(volatile unsigned long *)(0x70000000))
#define PP_VER2          (*(volatile unsigned long *)(0x70000004))

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_PP,
    "PP502x / PP611x"
};

static struct hwstub_pp_desc_t __attribute__((aligned(2))) pp_descriptor =
{
    sizeof(struct hwstub_pp_desc_t),
    HWSTUB_DT_PP,
    0, {' ', ' '},
};

static uint16_t char2hex(uint8_t c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    else
        return 0xf;
}

void target_init(void)
{
    /* try to read version for PP502x */
    if(PP_VER2 >> 16 != ('P' | 'P' << 8))
    {
        logf("unidentified PP family\n");
        g_pp_family = UNKNOWN;
    }
    else
    {
        pp_descriptor.wChipID = char2hex((PP_VER2 >> 8) & 0xff) << 12 |
            char2hex(PP_VER2 & 0xff) << 8 |
            char2hex((PP_VER1 >> 24) & 0xff) << 4 |
            char2hex((PP_VER1 >> 16) & 0xff);
        pp_descriptor.bRevision[0] = (PP_VER1 >> 8) & 0xff;
        pp_descriptor.bRevision[1] = PP_VER1 & 0xff;
        if(pp_descriptor.wChipID >= 0x6110)
        {
            logf("identified PP611x family\n");
            g_pp_family = PP611x;
        }
        else
        {
            logf("identified PP502x family\n");
            g_pp_family = PP502x;
        }
    }
}

void target_get_desc(int desc, void **buffer)
{
    if(desc == HWSTUB_DT_PP)
        *buffer = &pp_descriptor;
    else
        *buffer = NULL;
}

void target_get_config_desc(void *buffer, int *size)
{
    memcpy(buffer, &pp_descriptor, sizeof(pp_descriptor));
    *size += sizeof(pp_descriptor);
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
