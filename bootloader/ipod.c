/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
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
#include "hwcompat.h"
#include "usb.h"

#define XSC(X) #X
#define SC(X) XSC(X)

/* Maximum allowed firmware image size. The largest known current 
   (December 2006) firmware is about 7.5MB (Apple's firmware for the ipod video)
   so we set this to 8MB. */
#define MAX_LOADSIZE (8*1024*1024)

/* A buffer to load the Linux kernel or Rockbox into */
unsigned char *loadbuffer = (unsigned char *)DRAM_START;

/* Bootloader version */
char version[] = APPSVERSION;

#if CONFIG_KEYPAD == IPOD_4G_PAD && !defined(IPOD_MINI)
/* check if number of seconds has past */
int timer_check(int clock_start, unsigned int usecs)
{
    if ((USEC_TIMER - clock_start) >= usecs) {
        return 1;
    } else {
        return 0;
    }
}

static void ser_opto_keypad_cfg(int val)
{
    int start_time;

    GPIOB_ENABLE &=~ 0x80;

    outl(inl(0x7000c104) | 0xc000000, 0x7000c104);
    outl(val, 0x7000c120);
    outl(inl(0x7000c100) | 0x80000000, 0x7000c100);

    GPIOB_OUTPUT_VAL &=~ 0x10;
    GPIOB_OUTPUT_EN  |=  0x10;

    start_time = USEC_TIMER;
    do {
        if ((inl(0x7000c104) & 0x80000000) == 0) {
            break;
        }
    } while (timer_check(start_time, 1500) != 0);

    outl(inl(0x7000c100) & ~0x80000000, 0x7000c100);

    GPIOB_ENABLE     |= 0x80;
    GPIOB_OUTPUT_VAL |= 0x10;
    GPIOB_OUTPUT_EN  &=~0x10;

    outl(inl(0x7000c104) | 0xc000000, 0x7000c104);
    outl(inl(0x7000c100) | 0x60000000, 0x7000c100);
}

int opto_keypad_read(void)
{
    int loop_cnt, had_io = 0;

    for (loop_cnt = 5; loop_cnt != 0;)
    {
        int key_pressed = 0;
        int start_time;
        unsigned int key_pad_val;

        ser_opto_keypad_cfg(0x8000023a);

        start_time = USEC_TIMER;
        do {
            if (inl(0x7000c104) & 0x4000000) {
                had_io = 1;
                break;
            }

            if (had_io != 0) {
                break;
            }
        } while (timer_check(start_time, 1500) != 0);

        key_pad_val = inl(0x7000c140);
        if ((key_pad_val & ~0x7fff0000) != 0x8000023a) {
            loop_cnt--;
        } else {
            key_pad_val = (key_pad_val << 11) >> 27;
            key_pressed = 1;
        }

        outl(inl(0x7000c100) | 0x60000000, 0x7000c100);
        outl(inl(0x7000c104) | 0xc000000, 0x7000c104);

        if (key_pressed != 0) {
            return key_pad_val ^ 0x1f;
        }
    }

    return 0;
}
#endif

static int key_pressed(void)
{
    unsigned char state;

#if CONFIG_KEYPAD == IPOD_4G_PAD
#ifdef IPOD_MINI /* mini 1G only */
    state = GPIOA_INPUT_VAL & 0x3f;
    if ((state & 0x10) == 0) return BUTTON_LEFT;
    if ((state & 0x2) == 0) return BUTTON_MENU;
    if ((state & 0x4) == 0) return BUTTON_PLAY;
    if ((state & 0x8) == 0) return BUTTON_RIGHT;
#else
    state = opto_keypad_read();
    if ((state & 0x4) == 0) return BUTTON_LEFT;
    if ((state & 0x10) == 0) return BUTTON_MENU;
    if ((state & 0x8) == 0) return BUTTON_PLAY;
    if ((state & 0x2) == 0) return BUTTON_RIGHT;
#endif
#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    state = GPIOA_INPUT_VAL;
    if ((state & 0x08) == 0) return BUTTON_LEFT;
    if ((state & 0x10) == 0) return BUTTON_MENU;
    if ((state & 0x04) == 0) return BUTTON_PLAY;
    if ((state & 0x01) == 0) return BUTTON_RIGHT;
#endif
    return 0;
}

bool button_hold(void)
{
#if CONFIG_KEYPAD == IPOD_1G2G_PAD
    return (GPIOA_INPUT_VAL & 0x20);
#else
    return !(GPIOA_INPUT_VAL & 0x20);
#endif
}

void fatal_error(void)
{
    extern int line;
    bool holdstatus=false;

    /* System font is 6 pixels wide */
#if defined(IPOD_1G2G) || defined(IPOD_3G)
    printf("Insert Firewire cable, or");
    printf("hold MENU+PLAY to reboot");
    printf("then REW+FF for disk mode");
#elif LCD_WIDTH >= (30*6)
    printf("Insert USB cable, or");
    printf("hold MENU+SELECT to reboot");
    printf("then SELECT+PLAY for disk mode");
#else
    printf("Insert USB cable, or");
    printf("hold MENU+SELECT to");
    printf("reboot then SELECT+PLAY");
    printf("for disk mode");
#endif
    lcd_update();

    usb_init();
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
        if (usb_detect() == USB_INSERTED) {
            ata_sleepnow(); /* Immediately spindown the disk. */
            sleep(HZ*2);
#if CONFIG_CPU == PP5020
            memcpy((void *)0x40017f00, "diskmode\0\0hotstuff\0\0\1", 21);
#elif CONFIG_CPU == PP5022
            memcpy((void *)0x4001ff00, "diskmode\0\0hotstuff\0\0\1", 21);
#elif CONFIG_CPU == PP5002
            memcpy((void *)0x40017f00, "diskmodehotstuff\1", 17);
#endif /* CONFIG_CPU */
            system_reboot(); /* Reboot */
        }
        udelay(100000); /* 100ms */
    }

}

/* The bootloader is started from the OSOS image on the firmware
 * partition. There are several ways it can be installed there:
 * appended to the Apple firmware, on its own, or appended to
 * Rockbox itself. The Apple ROM loader loads the entire OSOS
 * image to DRAM_START, whatever it contains. If the bootloader
 * is appended to another image then it will've modified the
 * entry point in the OSOS header such that the ROM will call the
 * bootloader rather than the main image.
 *
 * So, once the bootloader has control:
 *
 * 1) If the hold switch is on, or the menu button is being held,
 *    try to boot the Apple firmware.
 *   1a) First, it looks for apple_os.ipod on the FAT32 partition,
 *       in .rockbox or the root directory. If found it loads that
 *       without further checking and runs it.
 *   1b) Next, it checks to see if the OSOS image already loaded
 *       into RAM is in fact the Apple firmware with the bootloader
 *       appended. It looks at DRAM_START+0x20 for the string
 *       "portalplayer", and if it's there, just jumps back to
 *       DRAM_START where the entry point was before the bootloader
 *       was appended.
 *   1c) If neither of those worked, it displays an error and dies.
 *
 * 2) If the play button is being held, try to boot Linux. It looks
 *    for linux.bin in the root directory, and if it's not there,
 *    it displays an error and dies.
 *
 * 3) Otherwise, try to boot Rockbox.
 *   3a) First, it looks for rockbox.ipod on the FAT32 partition,
 *       in .rockbox or the root directory. If found it loads that
 *       without further checking and runs it.
 *   3b) Next, it checks to see if the OSOS image already loaded
 *       into RAM is in fact Rockbox with the bootloader appended.
 *       It looks at DRAM_START+0x20 for the string "Rockbox\1"
 *       (which is inserted there in crt0-pp.S), and if it's there,
 *       just humps back to DRAM_START where the entry point was
 *       before the bootloader was appended.
 *   3c) If neither of those worked, it displays an error and dies.
 *
 * The result is that any of the three install configurations work,
 * and that images of apple_os.ipod or rockbox.ipod on the FAT32
 * partition take priority over the contents of OSOS (this avoids
 * upgrades failing to work if OSOS is not updated).
 *
 * Loading from OSOS is somewhat faster than loading from FAT32,
 * because the Apple ROM doesn't have to deal with filesystems or
 * fragmentation, and is already loading from OSOS anyway. Thus,
 * the fastest boot configuration that still allows dual booting
 * is to install Rockbox into OSOS with the bootloader appended
 * (and delete/rename rockbox.ipod from the FAT32 partition).
 *
 * It is of course faster to just install Rockbox to OSOS alone,
 * but then it's impossible to boot the Apple firmware.
 */

void* main(void)
{
    char buf[256];
    int i;
    int btn;
    int rc;
    bool haveramos;
    bool button_was_held;
    struct partinfo* pinfo;
    unsigned short* identify_info;

    /* Check the button hold status as soon as possible - to 
       give the user maximum chance to turn it off in order to
       reset the settings in rockbox. */
    button_was_held = button_hold();

    system_init();
    kernel_init();

#ifndef HAVE_BACKLIGHT_INVERSION
    backlight_init(); /* Turns on the backlight */
#endif

    lcd_init();
    font_init();

#ifdef HAVE_LCD_COLOR
    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();
#endif

#if 0
    /* ADC and button drivers are not yet implemented */
    adc_init();
    button_init();
#endif

    btn=key_pressed();

    /* Enable bootloader messages */
    if (btn==BUTTON_RIGHT)
        verbose = true;

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version: %s", version);
    printf("IPOD version: 0x%08x", IPOD_HW_REVISION);

    i=ata_init();
    if (i==0) {
      identify_info=ata_get_identify();
      /* Show model */
      for (i=0; i < 20; i++) {
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);
      }
      buf[40]=0;
      for (i=39; i && buf[i]==' '; i--) {
        buf[i]=0;
      }
      printf(buf);
    } else {
      printf("ATA: %d", i);
    }

    disk_init();
    rc = disk_mount_all();
    if (rc<=0)
    {
        printf("No partition found");
        fatal_error();
    }

    pinfo = disk_partinfo(1);
    printf("Partition 1: 0x%02x %ld sectors", 
           pinfo->type, pinfo->size);

    if (button_was_held || (btn==BUTTON_MENU)) {
        /* If either the hold switch was on, or the Menu button was held, then 
           try the Apple firmware */

        printf("Loading original firmware...");
    
        /* First try an apple_os.ipod file on the FAT32 partition
           (either in .rockbox or the root) 
         */
    
        rc=load_firmware(loadbuffer, "apple_os.ipod", MAX_LOADSIZE);
    
        if (rc == EOK) {
            printf("apple_os.ipod loaded.");
            return (void*)DRAM_START;
        } else if (rc == EFILE_NOT_FOUND) {
            /* If apple_os.ipod doesn't exist, then check if there is an Apple 
               firmware image in RAM  */
            haveramos = (memcmp((void*)(DRAM_START+0x20),"portalplayer",12)==0);
            if (haveramos) {
                /* We have a copy of the retailos in RAM, lets just run it. */
                return (void*)DRAM_START;
            }
        } else if (rc < EFILE_NOT_FOUND) {
            printf("Error!");
            printf("Can't load apple_os.ipod:");
            printf(strerror(rc));
        }
        
        /* Everything failed - just loop forever */
        printf("No RetailOS detected");
        
    } else if (btn==BUTTON_PLAY) {
        printf("Loading Linux...");
        rc=load_raw_firmware(loadbuffer, "/linux.bin", MAX_LOADSIZE);
        if (rc < EOK) {
            printf("Error!");
            printf("Can't load linux.bin:");
            printf(strerror(rc));
        } else {
            return (void*)DRAM_START;
        }
    } else {
        printf("Loading Rockbox...");
        rc=load_firmware(loadbuffer, BOOTFILE, MAX_LOADSIZE);
        if (rc == EOK) {
            printf("Rockbox loaded.");
            return (void*)DRAM_START;
        } else if (rc == EFILE_NOT_FOUND) {
            /* if rockbox.ipod doesn't exist, then check if there is a Rockbox
               image in RAM  */
            haveramos = (memcmp((void*)(DRAM_START+0x20),"Rockbox\1",8)==0);
            if (haveramos) {
                /* We have a copy of Rockbox in RAM, lets just run it. */
                return (void*)DRAM_START;
            }
        }

        printf("Error!");
        printf("Can't load " BOOTFILE ": ");
        printf(strerror(rc));
    }
    
    /* If we get to here, then we haven't been able to load any firmware */
    fatal_error();
    
    /* We never get here, but keep gcc happy */
    return (void*)0;
}
