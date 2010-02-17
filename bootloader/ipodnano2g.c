/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Dave Chapman
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"

#include "inttypes.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "i2c-s5l8700.h"
#include "kernel.h"
#include "thread.h"
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

/* Safety measure - maximum allowed firmware image size. 
   The largest known current (October 2009) firmware is about 6.2MB so 
   we set this to 8MB. 
*/
#define MAX_LOADSIZE (8*1024*1024)

/* The buffer to load the firmware into - use an uncached alias of 0x08000000 */
unsigned char *loadbuffer = (unsigned char *)0x48000000;

/* Bootloader version */
char version[] = APPSVERSION;

extern int line;

void fatal_error(void)
{
    extern int line;
    bool holdstatus=false;

    /* System font is 6 pixels wide */
    printf("Hold MENU+SELECT to");
    printf("reboot then SELECT+PLAY");
    printf("for disk mode");
    lcd_update();

    while (1) {
        if (button_hold() != holdstatus) {
            if (button_hold()) {
                holdstatus=true;
                lcd_puts(0, line, "Hold switch on!");
            } else {
                holdstatus=false;
                lcd_puts(0, line, "               ");
            }
            lcd_update();
        }
    }
}

/* aes_decrypt() and readfw() functions taken from iLoader.
   (C) Michael Sparmann and licenced under GPL v2 or later. 
*/

static void aes_decrypt(void* data, uint32_t size)
{
    uint32_t ptr, i;
    uint32_t go = 1;

    PWRCONEXT &= ~0x400;
    AESTYPE = 1;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESKEYLEN = 8;
    AESOUTSIZE = size;
    AESAUXSIZE = 0x10;
    AESINSIZE = 0x10;
    AESSIZE3 = 0x10;
    for (ptr = (size >> 2) - 4; ; ptr -= 4)
    {
        AESOUTADDR = (uint32_t)data + (ptr << 2);
        AESINADDR = (uint32_t)data + (ptr << 2);
        AESAUXADDR = (uint32_t)data + (ptr << 2);
        AESSTATUS = 6;
        AESGO = go;
        go = 3;
        while ((AESSTATUS & 6) == 0);
        if (ptr == 0) break;
        for (i = 0; i < 4; i++)
            ((uint32_t*)data)[ptr + i] ^= ((uint32_t*)data)[ptr + i - 4];
    }
    AESCONTROL = 0;
    PWRCONEXT |= 0x400;
}

static int readfw(char* filename, void* address, int* size)
{
    int i;
    uint32_t startsector = 0;
    uint32_t buffer[0x200];

    if (nand_read_sectors(0, 1, buffer) != 0) 
        return -1;

    if (*((uint16_t*)((uint32_t)buffer + 0x1FE)) != 0xAA55) 
        return -2;

    for (i = 0x1C2; i < 0x200; i += 0x10) {
        if (((uint8_t*)buffer)[i] == 0) {
            startsector = *((uint16_t*)((uint32_t)buffer + i + 4))
                        | (*((uint16_t*)((uint32_t)buffer + i + 6)) << 16);
            break;
        }
    }

    if (startsector == 0)
        return -3;

    if (nand_read_sectors(startsector, 1, buffer) != 0) 
        return -4;

    if (buffer[0x40] != 0x5B68695D) 
        return -5;

    if (nand_read_sectors(startsector + 1 + (buffer[0x41] >> 11), 1, buffer) != 0) 
        return -6;

    for (i = 0; i < 0x1FE; i += 10) {
        if (memcmp(&buffer[i], filename, 8) == 0) {
            uint32_t filesector = startsector + 1 + (buffer[i + 3] >> 11);
            *size = buffer[i + 4];

            if (nand_read_sectors(filesector, ((*size + 0x7FF) >> 11), address) != 0) 
                return -7;

            /* Success! */
            return 0;
        }
    }

    /* Nothing found */
    return -8;
}

void main(void)
{
    int i;
    int btn;
    int size;
    int rc;
    bool button_was_held;

    /* Check the button hold status as soon as possible - to 
       give the user maximum chance to turn it on in order to
       reset the settings in rockbox. */
    button_was_held = button_hold();

    system_init();
    kernel_init();

    i2c_init();

    enable_irq();

    backlight_init(); /* Turns on the backlight */

    lcd_init();
    font_init();

    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();

//    button_init();

    btn=0; /* TODO */

    /* Enable bootloader messages */
    if (btn==BUTTON_RIGHT)
        verbose = true;

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version: %s", version);

    i = storage_init();

    if (i != 0) {
        printf("ATA error: %d", i);
        fatal_error();
    }

    disk_init();
    rc = disk_mount_all();
    if (rc<=0)
    {
        printf("No partition found");
        fatal_error();
    }

    if (button_was_held || (btn==BUTTON_MENU)) {
        /* If either the hold switch was on, or the Menu button was held, then 
           try the Apple firmware */
        printf("Loading original firmware...");

        if ((rc = readfw("DNANkbso", loadbuffer, &size)) < 0) {
            printf("readfw error %d",rc);
            fatal_error();
        }

        /* Now we need to decrypt it */
        printf("Decrypting %d bytes...",size);

        aes_decrypt(loadbuffer, size);
    } else {
        printf("Loading Rockbox...");
        rc=load_firmware(loadbuffer, BOOTFILE, MAX_LOADSIZE);

        if (rc != EOK) {
            printf("Error!");
            printf("Can't load " BOOTFILE ": ");
            printf(strerror(rc));
            fatal_error();
        }

        printf("Rockbox loaded.");
    }


    /* If we get here, we have a new firmware image at 0x08000000, run it */
    printf("Executing...");

    disable_irq();

    /* Remap the bootrom back to zero - that's how the NOR bootloader leaves
       it.
     */
    MIUCON &= ~1;

    /* Disable caches and protection unit */
    asm volatile(
        "mrc 15, 0, r0, c1, c0, 0 \n"
        "bic r0, r0, #0x1000      \n"
        "bic r0, r0, #0x5         \n"
        "mcr 15, 0, r0, c1, c0, 0 \n"
    );

    /* Branch to start of DRAM */
    asm volatile("ldr pc, =0x08000000");
}
