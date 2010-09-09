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
#include "codecs.h"

struct codec_api *ci DATA_ATTR;

extern unsigned char plugin_bss_start[];
extern unsigned char plugin_end_addr[];

extern enum codec_status codec_main(void);

/* stub, the entry point is called via its reference in __header to
 * avoid warning with certain compilers */
int _start(void) {return 0;}

enum codec_status codec_start(void)
{
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#ifdef USE_IRAM
    extern char iramcopy[], iramstart[], iramend[], iedata[], iend[];
    size_t iram_size = iramend - iramstart;
    size_t ibss_size = iend - iedata;
    if (iram_size > 0 || ibss_size > 0)
    {
        ci->memcpy(iramstart, iramcopy, iram_size);
        ci->memset(iedata, 0, ibss_size);
        /* make the icache (if it exists) up to date with the new code */
        ci->cpucache_invalidate();
        /* barrier to prevent reordering iram copy and BSS clearing,
         * because the BSS segment alias the IRAM copy.
         */
        asm volatile ("" ::: "memory");
    }
#endif /* PLUGIN_USE_IRAM */
    ci->memset(plugin_bss_start, 0, plugin_end_addr - plugin_bss_start);
#endif

    return codec_main();
}

#ifdef CPU_ARM
void __attribute__((naked)) __div0(void)
{
    asm volatile("bx %0" : : "r"(ci->__div0));
}
#endif
