/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include <system.h>
#include <inttypes.h>
#include "config.h"
#include "gcc_extensions.h"
#include "lcd.h"
#include "backlight.h"
#include "button-target.h"
#include "common.h"
#include "storage.h"
#include "disk.h"
#include "panic.h"
#include "power.h"

int show_logo(void);

void main(void) NORETURN_ATTR;
void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    void(*kernel_entry)(void);
    int ret;

    system_init();
    kernel_init();

    enable_irq();

    lcd_init();
    show_logo();

    backlight_init();

    button_init_device();

    ret = storage_init();
    if(ret < 0)
        error(EATA, ret, true);

    while(!disk_init(IF_MV(0)))
        panicf("disk_init failed!");

    while((ret = disk_mount_all()) <= 0)
    {
        error(EDISK, ret, true);
    }

    printf("Loading firmware");

    loadbuffer = (unsigned char*)DRAM_ORIG; /* DRAM */
    buffer_size = (int)(loadbuffer + DRAM_SIZE - TTB_SIZE);

    while((ret = load_firmware(loadbuffer, BOOTFILE, buffer_size)) < 0)
    {
        error(EBOOTFILE, ret, true);
    }

    kernel_entry = (void*) loadbuffer;
    //cpucache_invalidate();
    printf("Executing");
    kernel_entry();
    printf("ERR: Failed to boot");

    /* never returns */
    while(1) ;
}
