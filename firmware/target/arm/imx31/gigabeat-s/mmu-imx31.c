/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Will Robertson
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
#include "cpu.h"
#include "mmu-imx31.h"
#include "mmu-arm.h"

void memory_init(void) {
#if 0
    ttb_init();
    set_page_tables();
    enable_mmu();
#endif
}

void set_page_tables() {
#if 0
    map_section(0, 0, 0x1000, CACHE_NONE); /* map every memory region to itself */
    /*This pa *might* change*/
    map_section(0x80000000, 0, 64, CACHE_ALL); /* map RAM to 0 and enable caching for it */
    map_section((int)FRAME1, (int)FRAME1, 1, BUFFERED); /* enable buffered writing for the framebuffer */
    map_section((int)FRAME2, (int)FRAME2, 1, BUFFERED);
#endif
}

