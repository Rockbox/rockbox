/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "nwz_lib.h"
#include "nwz_plattools.h"
#include <time.h>
#include <errno.h>

/* all images must have the following size */
#define ICON_WIDTH  130
#define ICON_HEIGHT 130

/* images */
#include "data/rockbox_icon.h"
#if BMPWIDTH_rockbox_icon != ICON_WIDTH || BMPHEIGHT_rockbox_icon != ICON_HEIGHT
#error rockbox_icon has the wrong resolution
#endif
#include "data/tools_icon.h"
#if BMPWIDTH_tools_icon != ICON_WIDTH || BMPHEIGHT_tools_icon != ICON_HEIGHT
#error tools_icon has the wrong resolution
#endif
/* buffer for Sony image, filled from NVP */
unsigned short sony_icon[ICON_WIDTH * ICON_HEIGHT];
/* resolution */
static int width, height, bpp;

/* return icon y position (x is always centered) */
int get_icon_y(void)
{
    /* adjust so that this contains the Sony logo and produces a nice logo
     * when used with rockbox */
    if(height == 320)
        return 70;
    else if(height == 320)
        return 100;
    else
        return height / 2 - ICON_HEIGHT + 30; /* guess, probably won't work */
}

/* Sony logo extraction */
bool extract_sony_logo(void)
{
    /* only support bpp of 16 */
    if(bpp != 16)
        return false;
    /* load the entire image from the nvp */
    int bti_size = nwz_nvp_read(NWZ_NVP_BTI, NULL);
    if(bti_size < 0)
        return false;
    unsigned short *bti = malloc(bti_size);
    if(nwz_nvp_read(NWZ_NVP_BTI, bti) != bti_size)
        return false;
    /* compute the offset in the image of the logo itself */
    int x_off = (width - ICON_WIDTH) / 2; /* logo is centered horizontally */
    int y_off = get_icon_y();
    /* extract part of the image */
    for(int y = 0; y < ICON_HEIGHT; y++)
    {
        memcpy(sony_icon + ICON_WIDTH * y,
            bti + width * (y + y_off) + x_off, ICON_WIDTH * sizeof(unsigned short));
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
    BOOT_OF
};

void draw_icon(int left, int top, const unsigned short *icon, unsigned short *fb_mmap)
{
    for(int y = 0; y < ICON_HEIGHT; y++)
    {
        memcpy(fb_mmap + width * (y + top) + left, icon + ICON_WIDTH * y,
            ICON_WIDTH * sizeof(unsigned short));
    }
}

enum boot_mode get_boot_mode(void)
{
    if(bpp != 16)
    {
        nwz_lcdmsg(true, 0, 2, "Unsupported bpp");
        sleep(2);
        return BOOT_OF;
    }
    /* open framebuffer */
    int fb_fd = nwz_fb_open(true);
    if(fb_fd < 0)
    {
        nwz_lcdmsg(true, 0, 2, "Cannot open input device");
        sleep(2);
        return BOOT_OF;
    }
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_fb_close(fb_fd);
        nwz_lcdmsg(true, 0, 2, "Cannot open input device");
        sleep(2);
        return BOOT_OF;
    }
    int fb_size = width * height * bpp / 2;
    void *fb_mmap = nwz_fb_mmap(fb_fd, 0, fb_size);
    void *fb_mmap_p1 = nwz_fb_mmap(fb_fd, NWZ_FB_LCD_PAGE_OFFSET, fb_size);
    if(fb_mmap == NULL || fb_mmap_p1 == NULL)
    {
        nwz_fb_close(fb_fd);
        nwz_key_close(input_fd);
        nwz_lcdmsg(true, 0, 2, "Cannot map framebuffer");
        sleep(2);
        return BOOT_OF;
    }
    /* wait for user action */
    enum boot_mode mode = BOOT_OF;
    /* NOTE on drawing: since screen is redrawn automatically, and we invoke
     * external programs to draw, we can't hope to fit it in the frame time
     * and it will flicker. To avoid this, we use the fact that all programs
     * only write to page 0. So we setup the lcd to update from page 1. When
     * we need to update the screen, we ask it to draw from page 0, then copy
     * page 0 to page 1 and then switch back to page 1 */
    memset(fb_mmap_p1, 0xff, fb_size); /* clear page 1 */
    nwz_fb_set_page(fb_fd, 1);
    bool redraw = true;
    while(true)
    {
        if(redraw)
        {
            /* redraw screen on page 0: clear screen */
            memset(fb_mmap, 0, fb_size);
            /* display top text */
            nwz_display_text_center(width, 0, true, NWZ_COLOR(255, 201, 0),
                NWZ_COLOR(0, 0, 0), 0, "SELECT PLAYER");
            /* display icon */
            const unsigned short *icon = (mode == BOOT_OF) ? sony_icon :
                (mode == BOOT_ROCKBOX) ? rockbox_icon : tools_icon;
            draw_icon((width - ICON_WIDTH) / 2, get_icon_y(), icon, fb_mmap);
            /* display bottom description */
            const char *desc = (mode == BOOT_OF) ? "SONY" :
                (mode == BOOT_ROCKBOX) ? "ROCKBOX" : "DEBUG TOOLS";
            nwz_display_text_center(width, get_icon_y() + ICON_HEIGHT + 30, true,
                NWZ_COLOR(255, 201, 0), NWZ_COLOR(0, 0, 0), 0, desc);
            /* display arrows */
            int arrow_y = get_icon_y() + ICON_HEIGHT / 2 - NWZ_FONT_H(true) / 2;
            nwz_display_text(NWZ_FONT_W(true) / 2, arrow_y, true,
                NWZ_COLOR(255, 201, 0), NWZ_COLOR(0, 0, 0), 0, "<");
            nwz_display_text(width - 3 * NWZ_FONT_W(true) / 2, arrow_y, true,
                NWZ_COLOR(255, 201, 0), NWZ_COLOR(0, 0, 0), 0, ">");
            /* switch to page 1 */
            nwz_fb_set_page(fb_fd, 0);
            /* copy page 0 to page 1 */
            memcpy(fb_mmap_p1, fb_mmap, fb_size);
            /* switch back to page 1 */
            nwz_fb_set_page(fb_fd, 1);

            redraw = false;
        }

        /* wait for a key  */
        int ret = nwz_key_wait_event(input_fd, -1);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        /* only act on release */
        if(nwz_key_event_is_press(&evt))
            continue;
        int key_code = nwz_key_event_get_keycode(&evt);
        /* play -> stop loop and return mode */
        if(key_code == NWZ_KEY_PLAY)
            break;
        /* left/right/up/down: change mode */
        if(key_code == NWZ_KEY_LEFT || key_code == NWZ_KEY_DOWN)
        {
            if(mode == BOOT_ROCKBOX)
                mode = BOOT_OF;
            else if(mode == BOOT_OF)
                mode = BOOT_TOOLS;
            else
                mode = BOOT_ROCKBOX;
            redraw = true;
        }
        if(key_code == NWZ_KEY_RIGHT || key_code == NWZ_KEY_UP)
        {
            if(mode == BOOT_ROCKBOX)
                mode = BOOT_TOOLS;
            else if(mode == BOOT_OF)
                mode = BOOT_ROCKBOX;
            else
                mode = BOOT_OF;
            redraw = true;
        }
    }
    /* switch back to page 0 */
    nwz_fb_set_page(fb_fd, 0);
    nwz_key_close(input_fd);
    nwz_fb_close(fb_fd);
    return mode;
}

static char *boot_rb_argv[] =
{
    "rockbox.sony",
    NULL
};

int NWZ_TOOL_MAIN(all_tools)(int argc, char **argv);

void error_screen(const char *msg)
{
    nwz_lcdmsg(true, 0, 0, msg);
    sleep(3);
}

void create_sony_logo(void)
{
    for(int y = 0; y < ICON_HEIGHT; y++)
        for(int x = 0; x < ICON_WIDTH; x++)
            sony_icon[y * ICON_WIDTH + x] = 0xf81f;
}

int main(int argc, char **argv)
{
    /* make sure backlight is on and we are running the standard lcd mode */
    int fb_fd = nwz_fb_open(true);
    if(fb_fd >= 0)
    {
        struct nwz_fb_brightness bl;
        nwz_fb_get_brightness(fb_fd, &bl);
        bl.level = NWZ_FB_BL_MAX_LEVEL;
        nwz_fb_set_brightness(fb_fd, &bl);
        nwz_fb_set_standard_mode(fb_fd);
        /* get resolution */
        /* we also need to get the native resolution */
        if(nwz_fb_get_resolution(fb_fd, &width, &height, &bpp) != 0)
        {
            /* safe one */
            width = 240;
            height = 320;
            bpp = 16;
        }
        nwz_fb_close(fb_fd);
    }
    /* extract logo */
    if(!extract_sony_logo())
        create_sony_logo();
    /* run all tools menu */
    enum boot_mode mode = get_boot_mode();
    if(mode == BOOT_TOOLS)
    {
        /* run tools and then run OF */
        NWZ_TOOL_MAIN(all_tools)(argc, argv);
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
        sleep(5);
    }
    /* boot OF */
    execvp("/usr/local/bin/SpiderApp.of", argv);
    error_screen("Cannot boot OF");
    sleep(5);
    /* if we reach this point, everything failed, so return an error so that
     * sysmgrd knows something is wrong */
    return 1;
}
