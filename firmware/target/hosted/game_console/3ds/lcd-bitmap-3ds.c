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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds/gfx.h>
#ifdef RGB565
#undef RGB565
#endif

#include <3ds/console.h>
#include <3ds/services/cfgu.h>

#include "debug.h"
#include "system.h"
#include "button-console.h"
#include "screendump.h"
#include "lcd-target.h"
#include "kernel.h"
#include "lcd-gpu-3ds.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

static u8 system_model = CFG_MODEL_3DS;

/*
 * The bottom screen is presented with the GPU (lcd-gpu-3ds.c): the frame is
 * copied into a staging buffer that always holds the whole screen, so a
 * rect only copies its own rows. With interrupts on, waiting for the GPU
 * or for the last frame's VBlank lets the other threads run. With
 * interrupts off (early boot, panic) yield() and sleep() can't be used:
 * the frame waits for the GPU and lcd_gpu_end() for the VBlank instead.
 */
static void update_framebuffer_rect(const fb_data *src, int x, int y,
                                    int w, int h)
{
    if (!lcd_gpu_ready())
        return;

    int oldlevel = disable_irq_save();
    restore_irq(oldlevel);

    if (oldlevel == 0) {
        while (!lcd_gpu_try_begin())
            yield();

        /* At over ~60 updates/s the last frame is still waiting for its
         * VBlank. Sleep this thread instead of blocking all of them in
         * lcd_gpu_end(); nothing else presents meanwhile, and another
         * thread's update waits in lcd_gpu_try_begin() above. */
        while (lcd_gpu_present_pending())
            sleep(1);
    } else if (!lcd_gpu_begin()) {
        /* a frame is already open: a panic inside an update */
        return;
    }

    /* copy last, after any sleep, so the newest pixels are shown */
    lcd_gpu_copy_rows(src, LCD_WIDTH, x, y, w, h);
    lcd_gpu_end();
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

    update_framebuffer_rect(FBADDR(0,0), x, y, width, height);
}

void lcd_update(void)
{
    update_framebuffer_rect(FBADDR(0,0), 0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void sys_console_init(void)
{
    gfxInit(GSP_BGR8_OES, GSP_RGB565_OES, false);
    consoleInit(GFX_TOP, NULL);
}

void lcd_init_device(void)
{
    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    if (!lcd_gpu_init(LCD_WIDTH, LCD_HEIGHT)) {
        /* the top screen console doesn't need the GPU */
        printf("LCD: GPU init failed, can't show the display.\n"
               "Close the app with HOME.\n");
        system_exception_wait();
    }

    /* hidInit(); */

    /* Set dpi value from system model */
    CFGU_GetSystemModel(&system_model);
}

void lcd_shutdown(void)
{
    /* hidExit(); */
    lcd_gpu_exit();
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
