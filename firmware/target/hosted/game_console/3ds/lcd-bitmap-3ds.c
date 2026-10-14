/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#include <3ds/gfx.h>
#ifdef RGB565
#undef RGB565
#endif

#include <3ds/allocator/linear.h>
#include <3ds/console.h>
#include <3ds/services/cfgu.h>

#include "debug.h"
#include "system.h"
#include "button-console.h"
#include "screendump.h"
#include "lcd-target.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

static u8 system_model = CFG_MODEL_3DS;

typedef struct
{
    int width, height;
} dimensions_t;

typedef struct
{
    int x, y, w, h;
} rect_t;

static inline void copy_framebuffer_16_rect(u16 *dest, const dimensions_t dest_dim,
    const u16 *src, const dimensions_t src_dim, const rect_t src_rect)
{
    const int dst_stride = dest_dim.width;
    const int src_stride = src_dim.width;

    const u16 *s = src +
                   src_rect.y * src_stride +
                   src_rect.x;

    u16 *d = dest +
             src_rect.x * dst_stride +
             (dst_stride - src_rect.y - 1);

    for (int row = 0; row < src_rect.h; ++row) {
        const u16 *sp = s;
        u16 *dp = d;

        int col = src_rect.w;

        while (col >= 4) {
            dp[0]              = sp[0];
            dp[dst_stride]     = sp[1];
            dp[dst_stride * 2] = sp[2];
            dp[dst_stride * 3] = sp[3];

            sp += 4;
            dp += dst_stride * 4;
            col -= 4;
        }

        while (col--) {
            *dp = *sp++;
            dp += dst_stride;
        }

        s += src_stride;
        --d;
    }
}

void update_framebuffer_rect(const u16 *src, const rect_t src_rect)
{
    u16 fb_width, fb_height;
    u32 bufsize;
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &fb_width, &fb_height);
    bufsize = fb_width * fb_height * 2;

    copy_framebuffer_16_rect((void *)fb, (dimensions_t){ fb_width, fb_height },
                             src, (dimensions_t){ LCD_WIDTH, LCD_HEIGHT }, src_rect);

    // do not swap buffers for a partial blit
    GSPGPU_FlushDataCache(fb, bufsize);
    /* gspWaitForVBlank(); */
}

void update_framebuffer(const u16 *src)
{
    u16 fb_width, fb_height;
    u32 bufsize;
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &fb_width, &fb_height);
    bufsize = fb_width * fb_height * 2;

    copy_framebuffer_16_rect((void *)fb, (dimensions_t){ fb_width, fb_height },
                             src, (dimensions_t){ LCD_WIDTH, LCD_HEIGHT }, (rect_t){0, 0, LCD_WIDTH, LCD_HEIGHT});

    // flush cache and swap framebuffer
    /* gfxFlushBuffers(); */
    GSPGPU_FlushDataCache(fb, bufsize);
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void lcd_update_rect(int x, int y, int width, int height)
{
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

    fb_data *src = FBADDR(0,0);
    update_framebuffer_rect(src, (rect_t){x, y, width, height});
}

void lcd_update(void)
{
    /* update a full screen rect */
    fb_data *src = FBADDR(0,0);
    update_framebuffer(src);
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

    /* Set dpi value from system model */
    CFGU_GetSystemModel(&system_model);
}

void lcd_shutdown(void)
{
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
