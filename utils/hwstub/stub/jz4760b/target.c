/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Marcin Bukat
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
#include "jz4760b.h"

#define PIN_BL  (4 * 32 + 1)

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_JZ,
    "JZ4760(B)"
};

static struct hwstub_jz_desc_t jz_descriptor =
{
    sizeof(struct hwstub_jz_desc_t),
    HWSTUB_DT_JZ,
    0x4760,
    'B'
};


void target_udelay(int us)
{
    /* use OS timer running at 3MHz */
    uint32_t end = REG_OST_OSTCNTL + 3 * us;
    while(REG_OST_OSTCNTL < end) {}
}

void ost_init(void)
{
    /* Init OS Timer: don't compare, use EXTCLK (12MHz) and set prescaler to 4
     * so that it increases at 3MHz */
    REG_TCU_TECR = TECR_OST; /* disable OST */
    REG_OST_OSTCSR = OSTCSR_CNT_MD | OSTCSR_PRESCALE4 | OSTCSR_EXT_EN;
    REG_OST_OSTCNTL = 0;
    REG_TCU_TESR = TESR_OST; /* enable OST */
}

void target_mdelay(int ms)
{
    return target_udelay(ms * 1000);
}

void target_init(void)
{
    ost_init();
}

void target_get_desc(int desc, void **buffer)
{
    if(desc == HWSTUB_DT_JZ)
        *buffer = &jz_descriptor;
    else
        *buffer = NULL;
}

void target_get_config_desc(void *buffer, int *size)
{
    (void) buffer;
    (void) size;
}
