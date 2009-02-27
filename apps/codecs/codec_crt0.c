/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Tomasz Malesinski
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

#include "config.h"
#include "codeclib.h"

struct codec_api *ci DATA_ATTR;

extern unsigned char iramcopy[];
extern unsigned char iramstart[];
extern unsigned char iramend[];
extern unsigned char iedata[];
extern unsigned char iend[];
extern unsigned char plugin_bss_start[];
extern unsigned char plugin_end_addr[];

extern enum codec_status codec_main(void);

enum codec_status codec_start(void)
{
#ifndef SIMULATOR
#ifdef USE_IRAM
    ci->memcpy(iramstart, iramcopy, iramend - iramstart);
    ci->memset(iedata, 0, iend - iedata);
#endif
    ci->memset(plugin_bss_start, 0, plugin_end_addr - plugin_bss_start);
#endif
#if NUM_CORES > 1
    /* writeback cleared iedata and bss areas */
    ci->cpucache_flush();
#endif
    return codec_main();
}

#ifdef CPU_ARM
void __attribute__((naked)) __div0(void)
{
    asm volatile("bx %0" : : "r"(ci->__div0));
}
#endif
