/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * 
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "button.h"
#include "adc.h"
#include "adc-target.h"
#include "backlight.h"
#include "backlight-target.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

#if defined(COWON_D2)
#include "pcf50606.h"
#define LOAD_ADDRESS 0x20000000 /* DRAM_START */
#endif

char version[] = APPSVERSION;

extern int line;

#define MAX_LOAD_SIZE (8*1024*1024) /* Arbitrary, but plenty. */

/* The following function is just test/development code */
#ifdef CPU_TCC77X
void show_debug_screen(void)
{
    int button;
    int power_count = 0;
    int count = 0;
    bool do_power_off = false;

    lcd_puts_scroll(0,0,"this is a very long line to test scrolling");
    while(!do_power_off) {
        line = 1;
        button = button_get(false);

        /* Power-off if POWER button has been held for a long time
           This loop is currently running at about 100 iterations/second
         */
        if (button & POWEROFF_BUTTON) {
            power_count++;
            if (power_count > 200)
               do_power_off = true;
        } else {
            power_count = 0;
        }

        printf("Btn: 0x%08x",button);
        printf("Tick: %d",current_tick);

        printf("GPIOA: 0x%08x",GPIOA);
        printf("GPIOB: 0x%08x",GPIOB);
        printf("GPIOC: 0x%08x",GPIOC);
        printf("GPIOD: 0x%08x",GPIOD);
//        printf("GPIOE: 0x%08x",GPIOE);

#if 0
        int i;
        for (i = 0; i<4; i++)
        {
            printf("ADC%d: 0x%04x",i,adc_read(i));
        }
#endif
        count++;
        printf("Count: %d",count);
        sleep(HZ/10);

    }

    lcd_clear_display();
    line = 0;
    printf("POWER-OFF");

    /* Power-off */
    power_off();

    printf("(NOT) POWERED OFF");
    while (true);
}
#else /* !CPU_TCC77X */
void show_debug_screen(void)
{
    int button;
    int power_count = 0;
    int count = 0;
    bool do_power_off = false;
#ifdef HAVE_BUTTON_DATA
    unsigned int data;
#endif

    while(!do_power_off) {
        line = 0;
        printf("Hello World!");

#ifdef HAVE_BUTTON_DATA
        button = button_read_device(&data);
#else
        button = button_read_device();
#endif

        /* Power-off if POWER button has been held for a long time
           This loop is currently running at about 100 iterations/second
         */
        if (button & POWEROFF_BUTTON) {
            power_count++;
            if (power_count > 200)
               do_power_off = true;
        } else {
            power_count = 0;
        }

        printf("Btn: 0x%08x",button);

        count++;
        printf("Count: %d",count);
    }

    lcd_clear_display();
    line = 0;
    printf("POWER-OFF");

    /* Power-off */
    power_off();

    printf("(NOT) POWERED OFF");
    while (true);
}
#endif

void* main(void)
{
#if defined(COWON_D2) && defined(TCCBOOT)
    int rc;
    unsigned char* loadbuffer = (unsigned char*)LOAD_ADDRESS;
#endif

    power_init();
    system_init();
#ifndef COWON_D2
    /* The D2 doesn't enable threading or interrupts */
    kernel_init();
    enable_irq();
#endif
    lcd_init();

    adc_init();
    button_init();
    backlight_init();

    font_init();
    lcd_setfont(FONT_SYSFIXED);

    _backlight_on();

/* Only load the firmware if TCCBOOT is defined - this ensures SDRAM_START is
   available for loading the firmware. Otherwise display the debug screen. */
#if defined(COWON_D2) && defined(TCCBOOT)
    printf("Rockbox boot loader");
    printf("Version %s", version);

    printf("ATA");
    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    printf("mount");
    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    rc = load_firmware(loadbuffer, BOOTFILE, MAX_LOAD_SIZE);

    if (rc < 0)
    {
        error(EBOOTFILE,rc);
    }
    else if (rc == EOK)
    {
        int(*kernel_entry)(void);

        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }
#else
    show_debug_screen();
#endif

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
