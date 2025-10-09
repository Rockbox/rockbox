/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "system.h"
#include "button-ctru.h"
#include "screendump.h"
#include "lcd-target.h"

#include <3ds/gfx.h>
#include <3ds/allocator/linear.h>
#include <3ds/console.h>
#include <3ds/services/cfgu.h>

/*#define LOGF_ENABLE*/
#include "logf.h"

fb_data *dev_fb = 0;

static u8 system_model = CFG_MODEL_3DS;

typedef struct
{
    int width, height;
} dimensions_t;

int get_dest_offset(int x, int y, int dest_width)
{
    return dest_width - y - 1 + dest_width * x;
}

int get_source_offset(int x, int y, int source_width)
{
    return x + y * source_width;
}

void copy_framebuffer_16(u16 *dest, const dimensions_t dest_dim, const u16 *source, const dimensions_t source_dim)
{
    int rows = MIN(dest_dim.width, source_dim.height);
    int cols = MIN(dest_dim.height, source_dim.width);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const u16 *s = source + get_source_offset(x, y, source_dim.width);
            u16 *d = dest + get_dest_offset(x, y, dest_dim.width);
            *d = *s;
        }
    }
}

void copy_framebuffer_24(u8 *dest, const dimensions_t dest_dim, const u8 *source, const dimensions_t source_dim)
{
    int rows = MIN(dest_dim.width, source_dim.height);
    int cols = MIN(dest_dim.height, source_dim.width);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const u8 *s = source + get_source_offset(x, y, source_dim.width) * 3;
            u8 *d = dest + get_dest_offset(x, y, dest_dim.width) * 3;
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
        }
    }
}

void copy_framebuffer_32(u32 *dest, const dimensions_t dest_dim, const u32 *source, const dimensions_t source_dim)
{
    int rows = MIN(dest_dim.width, source_dim.height);
    int cols = MIN(dest_dim.height, source_dim.width);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const u32 *s = source + get_source_offset(x, y, source_dim.width);
            u32 *d = dest + get_dest_offset(x, y, dest_dim.width);
            *d = *s;
        }
    }
}

void update_framebuffer(void)
{
    u16 fb_width, fb_height;
    u32 bufsize;
	u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &fb_width, &fb_height);
	bufsize = fb_width * fb_height * 2;

    /* lock_window_mutex() */

    if (FB_DATA_SZ == 2) {
        copy_framebuffer_16((void *)fb, (dimensions_t){ fb_width, fb_height },
                                 (u16 *)dev_fb, (dimensions_t){ LCD_WIDTH, LCD_HEIGHT });
    }
    else if (FB_DATA_SZ == 3) {
        copy_framebuffer_24((void *)fb, (dimensions_t){ fb_width, fb_height },
                                 (u8 *)dev_fb, (dimensions_t){ LCD_WIDTH, LCD_HEIGHT });
    }
    else {
        copy_framebuffer_32((void *)fb, (dimensions_t){ fb_width, fb_height },
                                 (u32 *)dev_fb, (dimensions_t){ LCD_WIDTH, LCD_HEIGHT });
    }

    // Flush and swap framebuffers
    /* gfxFlushBuffers(); */
    GSPGPU_FlushDataCache(fb, bufsize);
    gfxSwapBuffers();
    gspWaitForVBlank();

    /* unlock_window_mutex() */
}

/* lcd-as-memframe.c */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    dst = LCD_FRAMEBUF_ADDR(x, y);
    src = FBADDR(x,y);

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }

    update_framebuffer();
}

void lcd_update(void)
{
    /* update a full screen rect */
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void sys_console_init(void)
{
    gfxInit(GSP_BGR8_OES, GSP_RGB565_OES, false);
    consoleInit(GFX_TOP, NULL);
}

void lcd_init_device(void)
{
    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    /* hidInit(); */

    u16 fb_width, fb_height;
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &fb_width, &fb_height);
    u32 bufsize = fb_width * fb_height * 2;

    dev_fb = (fb_data *) linearAlloc(bufsize);
    if (dev_fb == NULL) {
        logf("could not allocate dev_fb size %d bytes\n",
               bufsize);
        exit(EXIT_FAILURE);
    }

    memset(dev_fb, 0x00, bufsize);
    
    /* Set dpi value from system model */
    CFGU_GetSystemModel(&system_model);
}

void lcd_shutdown(void)
{
    if (dev_fb) {
        linearFree(dev_fb);
        dev_fb = 0;
    }

    /* hidExit(); */
    gfxExit();
}

int lcd_get_dpi(void)
{
    /* link: https://www.reddit.com/r/nintendo/comments/2uzk5y/informative_post_about_dpi_on_the_new_3ds/ */
    /* all values for bottom lcd */
    float dpi = 96.0f;
    switch(system_model) {
    case CFG_MODEL_3DS:
        dpi = 132.45f;
    break;
    case CFG_MODEL_3DSXL:
        dpi = 95.69f;
     break;
    case CFG_MODEL_N3DS:
        dpi = 120.1f;
    break;
    case CFG_MODEL_2DS:
        dpi = 132.45f;
    break;
    case CFG_MODEL_N3DSXL:
        dpi = 95.69f;
    break;
    case CFG_MODEL_N2DSXL:
        dpi = 95.69f;
    break;
    default:
    break;
    };
    return (int) roundf(dpi);
}

