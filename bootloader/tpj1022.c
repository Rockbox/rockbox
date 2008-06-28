/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Dave Chapman
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

char version[] = APPSVERSION;

void* main(void)
{
    int i;
    int rc;
    int fd;
    char buffer[80];
    unsigned char* framebuffer = (unsigned char*)0x11e00000;

#if 0
    lcd_init();
    font_init();

    printf("Hello World!");
#endif

    i=ata_init();

    disk_init();
    rc = disk_mount_all();

#if 0
    /* Dump the flash */
    fd=open("/flash.bin",O_CREAT|O_RDWR);
    write(fd,(char*)0,1024*1024);
    close(fd);
#endif

#if 1
    /* Dump what may be the framebuffer */
    fd=open("/framebuffer.bin",O_CREAT|O_RDWR|O_TRUNC);
    write(fd,framebuffer,220*176*4);
    close(fd);
#endif


    fd=open("/gpio.txt",O_CREAT|O_RDWR|O_TRUNC);
    unsigned int gpio_a = GPIOA_INPUT_VAL;
    unsigned int gpio_b = GPIOB_INPUT_VAL;
    unsigned int gpio_c = GPIOC_INPUT_VAL;
    unsigned int gpio_d = GPIOD_INPUT_VAL;
    unsigned int gpio_e = GPIOE_INPUT_VAL;
    unsigned int gpio_f = GPIOF_INPUT_VAL;
    unsigned int gpio_g = GPIOG_INPUT_VAL;
    unsigned int gpio_h = GPIOH_INPUT_VAL;
    unsigned int gpio_i = GPIOI_INPUT_VAL;
    unsigned int gpio_j = GPIOJ_INPUT_VAL;
    unsigned int gpio_k = GPIOK_INPUT_VAL;
    unsigned int gpio_l = GPIOL_INPUT_VAL;

    snprintf(buffer, sizeof(buffer), "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",gpio_a,gpio_b,gpio_c,gpio_d,gpio_e,gpio_f,gpio_g,gpio_h,gpio_i,gpio_j,gpio_k,gpio_l);
    write(fd,buffer,strlen(buffer)+1);
    close(fd);

    /* Wait for FFWD button to be pressed */
    while((GPIOA_INPUT_VAL & 0x04) != 0);


    /* Now reboot */
    DEV_RS |= 0x4;

    return 0;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
