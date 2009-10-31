/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins, Lyre Project
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

/* Include Standard files */
#include <stdlib.h>
#include <stdio.h>
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "sd.h"
#include "backlight-target.h"
#include "lcd-target.h"
#include "debug-target.h"
#include "dma-target.h"
#include "uart-s3c2440.h"
#include "led-mini2440.h"


int main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    int rc;
    int(*kernel_entry)(void);

    led_init();
    clear_leds(LED_ALL);
    /* NB: something in system_init() prevents H-JTAG from downloading */
/*    system_init(); */ 
    kernel_init();
/*    enable_interrupt(IRQ_FIQ_STATUS); */
    backlight_init();
    lcd_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    dma_init();
    
    uart_init();
    uart_init_device(DEBUG_UART_PORT);

/*    mini2440_test(); */  
        
    /* Show debug messages if button is pressed */
    int touch_data;
    if(button_read_device(&touch_data) & BUTTON_MENU) 
        verbose = true;
        
    printf("Rockbox boot loader");
    printf("Version %s", APPSVERSION);

    rc = storage_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    disk_init(IF_MD(0));
    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    printf("Loading firmware");

    /* Flush out anything pending first */
    cpucache_invalidate();

    loadbuffer = (unsigned char*) 0x31000000;
    buffer_size = (unsigned char*)0x31400000 - loadbuffer;

    rc = load_firmware(loadbuffer, BOOTFILE, buffer_size);
    if(rc < 0)
        error(EBOOTFILE, rc);
    
    printf("Loaded firmware %d\n", rc);
    
/*    storage_close(); */
    system_prepare_fw_start();

    if (rc == EOK)
    {
        cpucache_invalidate();
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }
        
    /* end stop - should not get here */
    led_flash(LED_ALL, LED_NONE);
    while (1); /* avoid warning */
}
