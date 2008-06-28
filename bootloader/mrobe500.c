/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
 
#include "inttypes.h"
#include "string.h"
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
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rbunicode.h"
#include "usb.h"
#include "spi.h"
#include "uart-target.h"
#include "tsc2100.h"
#include "time.h"
#include "system-arm.h"

#define MRDEBUG

#if defined(MRDEBUG)

extern int line;
#if 0
struct touch_calibration_point tl, br;

void touchpad_get_one_point(struct touch_calibration_point *p)
{
    int data = 0;
    int start = current_tick;
    while (TIME_AFTER(start+(HZ/3), current_tick))
    {
        if (button_read_device()&BUTTON_TOUCHPAD)
        {
            data = button_get_last_touch();
            p->val_x = data>>16;
            p->val_y = data&0xffff;
            start = current_tick;
        }
        else if (data == 0)
            start = current_tick;
    }
}

#define MARGIN 25
#define LEN    7
void touchpad_calibrate_screen(void)
{
    reset_screen();
    printf("touch the center of the crosshairs to calibrate");
    /* get the topleft value */
    lcd_hline(MARGIN-LEN, MARGIN+LEN, MARGIN);
    lcd_vline(MARGIN, MARGIN-LEN, MARGIN+LEN);
    lcd_update();
    tl.px_x = MARGIN; tl.px_y = MARGIN;
    touchpad_get_one_point(&tl);
    reset_screen();
    printf("touch the center of the crosshairs to calibrate");
    /* get the topright value */
    lcd_hline(LCD_WIDTH-MARGIN-LEN, LCD_WIDTH-MARGIN+LEN, LCD_HEIGHT-MARGIN);
    lcd_vline(LCD_WIDTH-MARGIN, LCD_HEIGHT-MARGIN-LEN, LCD_HEIGHT-MARGIN+LEN);
    lcd_update();
    br.px_x = LCD_WIDTH-MARGIN; br.px_y = LCD_HEIGHT-MARGIN;
    touchpad_get_one_point(&br);
    reset_screen();
    line++;
    printf("tl %d %d", tl.val_x, tl.val_y);
    printf("br %d %d", br.val_x, br.val_y);
    line++;
    set_calibration_points(&tl, &br);
}
#endif
static uint8_t bl_command[] = {0xa4, 0x00, 0x00, 0xbb};
int brightness = 0;

void mrdebug(void)
{
    int button=0;
#if 0
    use_calibration(false);
    touchpad_calibrate_screen();
    use_calibration(true);
#endif

    while(true)
    {
#if 0
        struct tm *t = get_time();
        printf("%d:%d:%d %d %d %d", t->tm_hour, t->tm_min, t->tm_sec, t->tm_mday, t->tm_mon, t->tm_year);
        printf("time: %d", mktime(t));
#endif
       button = button_get(false);
        if (button == BUTTON_POWER)
        {
            printf("reset");
            IO_GIO_BITSET1|=1<<10;
        }
        if (button==BUTTON_RC_VOL_DOWN) 
        {
            brightness = (brightness - 5) & 0x7f;
            bl_command[2] = brightness;
            spi_block_transfer(SPI_target_BACKLIGHT, bl_command, 4, 0, 0);
        } 
        else if (button==BUTTON_RC_VOL_UP)
        {
            brightness = (brightness + 5) & 0x7f;
            bl_command[2] = brightness;
            spi_block_transfer(SPI_target_BACKLIGHT, bl_command, 4, 0, 0);
        }
//         {
//             short x,y,z1,z2;
//             tsc2100_read_values(&x, &y, &z1, &z2);
//             printf("x: %04x y: %04x z1: %04x z2: %04x", x, y, z1, z2);
//             printf("tsadc: %4x", tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS)&0xffff);
// //            tsc2100_keyclick(); /* doesnt work :( */
//             line -= 6;
//         }
        else if (button == BUTTON_RC_HEART)
        {
            printf("POINT");
            touchpad_set_mode(TOUCHPAD_POINT);
        }
        else if (button == BUTTON_RC_MODE)
        {
            printf("BUTTON");
            touchpad_set_mode(TOUCHPAD_BUTTON);
        }
#if 1
        else if (button&BUTTON_TOUCHPAD)
        {
            if (button&BUTTON_REL)
                continue;
            unsigned int data = button_get_data();
            int x = (data&0xffff0000)>>16, y = data&0x0000ffff;
            reset_screen();
            line = 9;
            printf("BB: %x %d %d", button, x,y);
            lcd_hline(x-5, x+5, y);
            lcd_vline(x, y-5, y+5);
            lcd_update();
        }
        else if (button == BUTTON_RC_PLAY)
        {
            reset_screen();
        }
            
        else if (button)
        {
          //  if (button&BUTTON_REL)
            {
                printf("%08x %s\n", button, (button&BUTTON_REL)?"yes":"no");
            }
        }
            
#endif
    }
}
#endif

void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    int rc;
    int(*kernel_entry)(void);

    power_init();
    lcd_init();
    system_init();
    kernel_init();
    
    enable_irq();
    enable_fiq();

    adc_init();
    button_init();
    backlight_init();

    font_init();

    lcd_setfont(FONT_SYSFIXED);

    /* Show debug messages if button is pressed */
//    if(button_read_device())
        verbose = true;

    printf("Rockbox boot loader");
    printf("Version %s", APPSVERSION);

    usb_init();

    /* Enter USB mode without USB thread */
    if(usb_detect())
    {
        const char msg[] = "Bootloader USB mode";
        reset_screen();
        lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                    (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
        lcd_update();

        ide_power_enable(true);
        ata_enable(false);
        sleep(HZ/20);
        usb_enable(true);

        while (usb_detect())
        {
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);
        }

        usb_enable(false);

        reset_screen();
        lcd_update();
    }
#if defined(MRDEBUG)
    mrdebug();
#endif
    printf("ATA");
    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    printf("disk");
    disk_init();

    printf("mount");
    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    printf("Loading firmware");

    loadbuffer = (unsigned char*) 0x00900000;
    buffer_size = (unsigned char*)0x01900000 - loadbuffer;

    rc = load_firmware(loadbuffer, BOOTFILE, buffer_size);
    if(rc < 0)
        error(EBOOTFILE, rc);

    if (rc == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }
}
