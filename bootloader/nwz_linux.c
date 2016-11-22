/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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

#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "button-target.h"
#include "button.h"
#include "../kernel/kernel-internal.h"
#include "filesystem-app.h"
#include "nvp-nwz.h"
#include "lcd.h"
#include "font.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* all images must have the following size */
#define ICON_WIDTH  130
#define ICON_HEIGHT 130

/* images */
#include "bitmaps/rockboxicon.h"
#include "bitmaps/toolsicon.h"

/* don't issue an error when parsing the file for dependencies */
#if defined(BMPWIDTH_rockboxicon) && (BMPWIDTH_rockboxicon != ICON_WIDTH || \
    BMPHEIGHT_rockboxicon != ICON_HEIGHT)
#error rockboxicon has the wrong resolution
#endif
#if defined(BMPWIDTH_toolsicon) && (BMPWIDTH_toolsicon != ICON_WIDTH || \
    BMPHEIGHT_toolsicon != ICON_HEIGHT)
#error toolsicon has the wrong resolution
#endif

/* buffer for Sony image, filled from NVP */
unsigned short sonyicon[ICON_WIDTH * ICON_HEIGHT];
const struct bitmap bm_sonyicon =
{
    .width = ICON_WIDTH,
    .height = ICON_HEIGHT,
    .format = FORMAT_NATIVE,
    .data = (unsigned char*)sonyicon
};

/* return icon y position (x is always centered) */
int get_icon_y(void)
{
    /* adjust so that this contains the Sony logo and produces a nice logo
     * when used with rockbox */
    if(LCD_HEIGHT == 320)
        return 70;
    else if(LCD_HEIGHT == 400)
        return 100;
    else
        return LCD_HEIGHT / 2 - ICON_HEIGHT + 30; /* guess, probably won't work */
}

/* Sony logo extraction */
bool extract_sony_logo(void)
{
    /* load the entire image from the nvp */
    int bti_size = nwz_nvp_read(NWZ_NVP_BTI, NULL);
    if(bti_size < 0)
        return false;
    unsigned short *bti = malloc(bti_size);
    if(nwz_nvp_read(NWZ_NVP_BTI, bti) != bti_size)
        return false;
    /* compute the offset in the image of the logo itself */
    int x_off = (LCD_WIDTH - ICON_WIDTH) / 2; /* logo is centered horizontally */
    int y_off = get_icon_y();
    /* extract part of the image */
    for(int y = 0; y < ICON_HEIGHT; y++)
    {
        memcpy(sonyicon + ICON_WIDTH * y,
            bti + LCD_WIDTH * (y + y_off) + x_off, ICON_WIDTH * sizeof(unsigned short));
    }
    free(bti);
    return true;
}

/* Important Note: this bootloader is carefully written so that in case of
 * error, the OF is run. This seems like the safest option since the OF is
 * always there and might do magic things. */

enum boot_mode
{
    BOOT_ROCKBOX,
    BOOT_TOOLS,
    BOOT_OF,
    BOOT_COUNT
};

static void display_text_center(int y, const char *text)
{
    int width;
    lcd_getstringsize(text, &width, NULL);
    lcd_putsxy(LCD_WIDTH / 2 - width / 2, y, text);
}

enum boot_mode get_boot_mode(void)
{
    /* wait for user action */
    enum boot_mode mode = BOOT_OF;
    bool redraw = true;
    while(true)
    {
        if(redraw)
        {
            lcd_clear_display();
            /* display top text */
            lcd_set_foreground(LCD_RGBPACK(255, 201, 0));
            display_text_center(0, "SELECT PLAYER");
            /* display icon */
            const struct bitmap *icon = (mode == BOOT_OF) ? &bm_sonyicon :
                (mode == BOOT_ROCKBOX) ? &bm_rockboxicon : &bm_toolsicon;
            lcd_bmp(icon, (LCD_WIDTH - ICON_WIDTH) / 2, get_icon_y());
            /* display bottom description */
            const char *desc = (mode == BOOT_OF) ? "SONY" :
                (mode == BOOT_ROCKBOX) ? "ROCKBOX" : "DEBUG TOOLS";
            display_text_center(get_icon_y() + ICON_HEIGHT + 30, desc);
            /* display arrows */
            int arrow_width, arrow_height;
            lcd_getstringsize("<", &arrow_width, &arrow_height);
            int arrow_y = get_icon_y() + ICON_HEIGHT / 2 - arrow_height / 2;
            lcd_putsxy(arrow_width / 2, arrow_y, "<");
            lcd_putsxy(LCD_WIDTH - 3 * arrow_width / 2, arrow_y, ">");

            lcd_update();

            redraw = false;
        }

        /* wait for a key  */
        int btn = button_get(true);
        /* ignore release, allow repeat */
        if(btn & BUTTON_REL)
            continue;
        if(btn & BUTTON_REPEAT)
            btn &= ~BUTTON_REPEAT;
        /* play -> stop loop and return mode */
        if(btn == BUTTON_PLAY)
            break;
        /* left/right/up/down: change mode */
        if(btn == BUTTON_LEFT || btn == BUTTON_DOWN)
        {
            mode = (mode + BOOT_COUNT - 1) % BOOT_COUNT;
            redraw = true;
        }
        if(btn == BUTTON_RIGHT || btn == BUTTON_UP)
        {
            mode = (mode + 1) % BOOT_COUNT;
            redraw = true;
        }
    }

    return mode;
}

static char *boot_rb_argv[] =
{
    "rockbox.sony",
    NULL
};

void error_screen(const char *msg)
{
    lcd_clear_display();
    lcd_putsf(0, 0, msg);
    lcd_update();
}

void create_sony_logo(void)
{
    for(int y = 0; y < ICON_HEIGHT; y++)
        for(int x = 0; x < ICON_WIDTH; x++)
            sonyicon[y * ICON_WIDTH + x] = 0xf81f;
}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    system_init();
    core_allocator_init();
    kernel_init();
    paths_init();
    lcd_init();
    font_init();
    button_init();
    backlight_init();
    backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    /* try to load the extra font we install on the device */
    int font_id = font_load("/contents/bootloader.fnt");
    if(font_id >= 0)
        lcd_setfont(font_id);
    fprintf(stderr, "font_id: %d\n", font_id);
    /* extract logo */
    if(!extract_sony_logo())
        create_sony_logo();
    /* run all tools menu */
    enum boot_mode mode = get_boot_mode();
    if(mode == BOOT_TOOLS)
    {
        /* run tools and then run OF */
    }
    else if(mode == BOOT_ROCKBOX)
    {
        /* Rockbox expects /.rockbox to contain themes, rocks, etc, but we
         * cannot easily create this symlink because the root filesystem is
         * mounted read-only. Although we could remount it read-write temporarily,
         * this is neededlessly complicated and we defer this job to the dualboot
         * install script */
        execvp("/contents/.rockbox/rockbox.sony", boot_rb_argv);
        /* fallback to OF in case of failure */
        error_screen("Cannot boot Rockbox");
        sleep(5 * HZ);
    }
    /* boot OF */
    execvp("/usr/local/bin/SpiderApp.of", argv);
    error_screen("Cannot boot OF");
    sleep(5 * HZ);
    /* if we reach this point, everything failed, so return an error so that
     * sysmgrd knows something is wrong */
    return 1;
}
