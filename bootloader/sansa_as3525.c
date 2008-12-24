/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rafaël Carré
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
#include "lcd.h"
#include "backlight-target.h"
#include "button-target.h"
#include "ascodec-target.h"
#include "common.h"
#include "storage.h"
#include "disk.h"
#include "panic.h"

int show_logo(void);
void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    void(*kernel_entry)(void);
    int ret;

    system_init();
    kernel_init();

#ifdef SANSA_C200V2
    /* stop here */
    while(1);
#endif
    lcd_init();
    show_logo();

    ascodec_init();  /* Required for backlight on e200v2 */
    _backlight_on();

    button_init_device();
    int btn = button_read_device();

    /* Enable bootloader messages if any button is pressed */
    if (btn)
    {
        lcd_clear_display();
        verbose = true;
    }

    enable_irq();

    ret = storage_init();
    if(ret < 0)
        error(EATA,ret);

    if(!disk_init(IF_MV(0)))
        panicf("disk_init failed!");

    ret = disk_mount_all();

    if(ret <= 0)
        error(EDISK, ret);

    printf("Loading firmware");

    loadbuffer = (unsigned char*)0x30000000; /* DRAM */
    buffer_size = (int)(loadbuffer + (MEM * 0x100000));

    ret = load_firmware(loadbuffer, BOOTFILE, buffer_size);
    if(ret < 0)
        error(EBOOTFILE, ret);

    disable_irq(); /* disable irq until we have copied the new vectors */

    if (ret == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        printf("Executing");
        kernel_entry();
        printf("ERR: Failed to boot");
    }

    /* never returns */
    while(1) ;
}
