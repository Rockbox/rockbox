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
#include "backlight-target.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

#if defined(COWON_D2)
#include "i2c.h"
#endif

char version[] = APPSVERSION;

extern int line;

void* main(void)
{
    int button;
    int power_count = 0;
    int count = 0;
    bool do_power_off = false;
    
#if defined(COWON_D2)
    int i,rc,fd,len;
    int* buf = (int*)0x21000000; /* Unused DRAM */
#endif

    power_init();
    lcd_init();
    system_init();
    
#if defined(COWON_D2)
    kernel_init();
#endif

    adc_init();
    button_init();
    backlight_init();

    font_init();

    lcd_setfont(FONT_SYSFIXED);

#if defined(COWON_D2)
    lcd_enable(true);
#endif

    _backlight_on();

#if defined(COWON_D2)
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

#if 0
    printf("opening test file...");

    fd = open("/test.bin", O_RDONLY);
    if (fd < 0) panicf("could not open test file");
    
    len = filesize(fd);
    printf("Length: %x", len);

    lseek(fd, 0, SEEK_SET);
    read(fd, buf, len);
    close(fd);
    
    printf("testing contents...");
    
    i = 0;
    while (buf[i] == i && i<(len/4)) { i++; }

    if (i < len/4)
    {
        printf("mismatch at %x [0x%x]", i, buf[i]);
    }
    else
    {
        printf("passed!");
    }
    while (!button_read_device()) {};
    while (button_read_device()) {};
#endif
#endif

    while(!do_power_off) {
        line = 0;
        printf("Hello World!");

        button = button_read_device();

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

#if defined(COWON_D2)
        printf("GPIOA: 0x%08x",GPIOA);
        printf("GPIOB: 0x%08x",GPIOB);
        printf("GPIOC: 0x%08x",GPIOC);
        printf("GPIOD: 0x%08x",GPIOD);
        printf("GPIOE: 0x%08x",GPIOE);
        
        for (i = 0; i<4; i++)
        {
            printf("ADC%d: 0x%04x",i,adc_read(i));
        }

        /* TODO: Move this stuff out to a touchscreen driver and establish
           how such a beast is going to work. Since it needs I2C read/write,
           it can't easily go on an interrupt-based tick task. */
        {
            unsigned char buf[] = { 0x2f, (0xE<<1) | 1, /* ADC start for X+Y */
                                    0, 0, 0 };
            int x,y;
            i2c_write(0x10, buf, 2);
            i2c_readmem(0x10, 0x2e, buf, 5);
            x = (buf[2] << 2) | (buf[3] & 3);
            y = (buf[4] << 2) | ((buf[3] & 0xC) >> 2);
            printf("X: 0x%03x Y: 0x%03x",x,y);

            x = (x*LCD_WIDTH) / 1024;
            y = (y*LCD_HEIGHT) / 1024;
            lcd_hline(x-5, x+5, y);
            lcd_vline(x, y-5, y+5);
            
            buf[0] = 0x2f;
            buf[1] = (0xF<<1) | 1;   /* ADC start for P1+P2 */
            i2c_write(0x10, buf, 2);
            i2c_readmem(0x10, 0x2e, buf, 5);
            x = (buf[2] << 2) | (buf[3] & 3);
            y = (buf[4] << 2) | ((buf[3] & 0xC) >> 2);
            printf("P1: 0x%03x P2: 0x%03x",x,y);
        }
#endif

        count++;
        printf("Count: %d",count);
    }

    lcd_clear_display();
    line = 0;
    printf("POWER-OFF");

    /* Power-off */
    power_off();

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
