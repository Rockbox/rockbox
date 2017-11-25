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

enum tcc_family_t
{
    UNKNOWN,
    TCC77x,
    TCC78x,
    TCC79xx,
};

static enum tcc_family_t g_tcc_family = UNKNOWN;

//uint32_t USB_BASE = 0xf0010000;
//uint32_t TIMER_BASE = 0xF3003000;
#define PCLK_BASE   0xf3000000
#define TIMER_BASE  0xf3003000

/* the following only applies to TCC78x and TCC79xx */
#define PCLK_TCT   (*(volatile unsigned long *)(PCLK_BASE + 0x44))
#define PCLK_DIV(x) ((x) & 0xfff)
#define PCLK_SEL(x) (((x) & 0xf) << 24)
#define PCLK_SEL_XIN    4 /* direct without div */
#define PCLK_EN     (1 << 28)

/* the following applies to TCC77x, TC78x and TCC79xx */
#define TC32EN     (*(volatile unsigned long *)(TIMER_BASE + 0x80))
#define TC32EN_ENABLE       (1 << 24)
#define TC32EN_PRESCALE(x)  ((x) & 0xffffff)
#define TC32MCNT   (*(volatile unsigned long *)(TIMER_BASE + 0x94))

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_TCC,
    "TCC77x / TCC78x / TCC79xx"
};

static struct hwstub_pp_desc_t __attribute__((aligned(2))) pp_descriptor =
{
    sizeof(struct hwstub_pp_desc_t),
    HWSTUB_DT_PP,
    0, {' ', ' '},
};

void target_init(void)
{
    /* Configure timer clock to 12Mhz (direct Xin) */
    PCLK_TCT = PCLK_EN | PCLK_SEL(PCLK_SEL_XIN) | PCLK_DIV(1);
    /* Enable TC32 and set prescaler to 12 so that it runs at Xin/12=1Mhz */
    TC32EN = TC32EN_ENABLE | TC32EN_PRESCALE(12 - 1);
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
    uint32_t end = TC32MCNT + us + 1;
    while(TC32MCNT <= end) {}
}

void target_mdelay(int ms)
{
    return target_udelay(ms * 1000);
}
