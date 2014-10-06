 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#include "xrick/system/system.h"

#include "xrick/config.h"
#include "xrick/draw.h"
#include "xrick/game.h"
#include "xrick/data/img.h"
#include "xrick/debug.h"

#include "plugin.h"
#include "lib/helper.h"

/*
 * Global variables
 */
U8 *sysvid_fb = NULL; /* xRick generic 320x200 8bpp frame buffer */

/*
 * Local variables
 */
static fb_data palette[256] IBSS_ATTR;
static bool isVideoInitialised = false;
#ifndef HAVE_LCD_COLOR
#  include "lib/grey.h"
GREY_INFO_STRUCT_IRAM
static unsigned char greybuffer[LCD_HEIGHT * LCD_WIDTH] IBSS_ATTR; /* off screen buffer */
static unsigned char *gbuf;
#  if LCD_PIXELFORMAT == HORIZONTAL_PACKING
enum { GREYBUFSIZE = (((LCD_WIDTH+7)/8)*LCD_HEIGHT*16+200) };
#  else
enum { GREYBUFSIZE = (LCD_WIDTH*((LCD_HEIGHT+7)/8)*16+200) };
#  endif
#endif /* ndef HAVE_LCD_COLOR */

#if (LCD_HEIGHT < SYSVID_HEIGHT)
enum { ROW_RESIZE_STEP = (LCD_HEIGHT << 16) / SYSVID_HEIGHT };

static bool rowsToSkip[SYSVID_HEIGHT];

/*
 *
 */
static void calculateRowsToSkip(void)
{
    U32 currentRow, prevResizedRow;

    prevResizedRow = 0;
    rowsToSkip[0] = false;

    for (currentRow = 1; currentRow < SYSVID_HEIGHT; ++currentRow)
    {
        U32 resizedRow = (currentRow * ROW_RESIZE_STEP) >> 16;
        if (resizedRow == prevResizedRow)
        {
            rowsToSkip[currentRow] = true;
        }
        prevResizedRow = resizedRow;
    }
}
#endif /* (LCD_HEIGHT < SYSVID_HEIGHT) */

#if (LCD_WIDTH < SYSVID_WIDTH)
enum { COLUMN_RESIZE_STEP = (LCD_WIDTH << 16) / (SYSVID_WIDTH + (DRAW_XYMAP_SCRLEFT*2)) };

static bool columnsToSkip[SYSVID_WIDTH + (DRAW_XYMAP_SCRLEFT*2)];

/*
 *
 */
static void calculateColumnsToSkip(void)
{
    U32 currentColumn, prevResizedColumn;

    prevResizedColumn = 0;
    columnsToSkip[0] = false;

    for (currentColumn = 1; currentColumn < (SYSVID_WIDTH + (DRAW_XYMAP_SCRLEFT*2)); ++currentColumn)
    {
        U32 resizedColumn = (currentColumn * COLUMN_RESIZE_STEP) >> 16;
        if (resizedColumn == prevResizedColumn)
        {
            columnsToSkip[currentColumn] = true;
        }
        prevResizedColumn = resizedColumn;
    }
}
#endif /* (LCD_WIDTH < SYSVID_WIDTH) */

/*
 *
 */
void sysvid_setPalette(img_color_t *pal, U16 n)
{
    U16 i;

    for (i = 0; i < n; i++)
    {
#ifdef HAVE_LCD_COLOR
        palette[i] = LCD_RGBPACK(pal[i].r, pal[i].g, pal[i].b);
#else
        palette[i] = ((3 * pal[i].r) + (6 * pal[i].g) + pal[i].b) / 10;
#endif
    }
}

/*
 *
 */
void sysvid_setGamePalette()
{
    sysvid_setPalette(game_colors, game_color_count);
}

/*
 * Update screen
 */
void sysvid_update(const rect_t *rects)
{
    unsigned sourceRow, sourceLastRow;
    unsigned sourceColumn, sourceLastColumn;
    unsigned resizedRow, resizedColumn;
    unsigned resizedWidth, resizedHeight;
    unsigned x, y;
    U8 *sourceBuf, *sourceTemp;
    fb_data *destBuf, *destTemp;

    if (!rects)
    {
        return;
    }

    while (rects)
    {
        sourceRow = rects->y;
        sourceLastRow = sourceRow + rects->height;
        sourceColumn = rects->x;
        sourceLastColumn = sourceColumn + rects->width;

#if (LCD_WIDTH < SYSVID_WIDTH)
        /* skip black borders */
        if (sourceColumn < -DRAW_XYMAP_SCRLEFT)
        {
            sourceColumn = -DRAW_XYMAP_SCRLEFT;
        }
        if (sourceLastColumn > (SYSVID_WIDTH + DRAW_XYMAP_SCRLEFT))
        {
            sourceLastColumn = SYSVID_WIDTH + DRAW_XYMAP_SCRLEFT;
        }

        /* skip unwanted columns */
        while (columnsToSkip[sourceColumn + DRAW_XYMAP_SCRLEFT] /* && sourceColumn < (SYSVID_WIDTH + DRAW_XYMAP_SCRLEFT) */)
        {
            ++sourceColumn;
        }

        resizedColumn = ((sourceColumn + DRAW_XYMAP_SCRLEFT) * COLUMN_RESIZE_STEP) >> 16;
        resizedWidth = 0;
#else
        resizedColumn = sourceColumn;
        resizedWidth = rects->width;
#endif /* (LCD_WIDTH < SYSVID_WIDTH) */

#if (LCD_HEIGHT < SYSVID_HEIGHT)
        /* skip unwanted rows */
        while (rowsToSkip[sourceRow] /* && sourceRow < SYSVID_HEIGHT */)
        {
            ++sourceRow;
        }

        resizedRow = (sourceRow * ROW_RESIZE_STEP) >> 16;
        resizedHeight = 0;
#else
        resizedRow = sourceRow;
        resizedHeight = rects->height;
#endif /* (LCD_HEIGHT < SYSVID_HEIGHT) */

        sourceBuf = sysvid_fb;
        sourceBuf += sourceColumn + sourceRow * SYSVID_WIDTH;

#ifdef HAVE_LCD_COLOR
        destBuf = rb->lcd_framebuffer;
#else
        destBuf = greybuffer;
#endif /* HAVE_LCD_COLOR */
        destBuf += resizedColumn + resizedRow * LCD_WIDTH;

#if (LCD_WIDTH < SYSVID_WIDTH)
        sourceColumn += DRAW_XYMAP_SCRLEFT;
        sourceLastColumn += DRAW_XYMAP_SCRLEFT;
#endif /* (LCD_WIDTH < SYSVID_WIDTH) */

        for (y = sourceRow; y < sourceLastRow; ++y)
        {
#if (LCD_HEIGHT < SYSVID_HEIGHT)
            if (rowsToSkip[y])
            {
                sourceBuf += SYSVID_WIDTH;
                continue;
            }

            ++resizedHeight;
#endif /* (LCD_HEIGHT < SYSVID_HEIGHT) */

            sourceTemp = sourceBuf;
            destTemp = destBuf;
            for (x = sourceColumn; x < sourceLastColumn; ++x)
            {
#if (LCD_WIDTH < SYSVID_WIDTH)
                if (columnsToSkip[x])
                {
                    ++sourceTemp;
                    continue;
                }

                if (y == sourceRow)
                {
                    ++resizedWidth;
                }
#endif /* (LCD_WIDTH < SYSVID_WIDTH) */

                *destTemp = palette[*sourceTemp];

                ++sourceTemp;
                ++destTemp;
            }

            sourceBuf += SYSVID_WIDTH;
            destBuf += LCD_WIDTH;
        }

#ifdef HAVE_LCD_COLOR
        IFDEBUG_VIDEO2(
            for (y = resizedRow; y < resizedRow + resizedHeight; ++y)
            {
                destBuf = rb->lcd_framebuffer + resizedColumn + y * LCD_WIDTH;
                *destBuf = palette[0x01];
                *(destBuf + resizedWidth - 1) = palette[0x01];
            }

            for (x = resizedColumn; x < resizedColumn + resizedWidth; ++x)
            {
                destBuf = rb->lcd_framebuffer + x + resizedRow * LCD_WIDTH;
                *destBuf = palette[0x01];
                *(destBuf + (resizedHeight - 1) * LCD_WIDTH) = palette[0x01];
            }
        );

        rb->lcd_update_rect(resizedColumn, resizedRow, resizedWidth, resizedHeight);
#else
        grey_ub_gray_bitmap_part(greybuffer, resizedColumn, resizedRow, LCD_WIDTH, resizedColumn, resizedRow, resizedWidth, resizedHeight);
#endif /* HAVE_LCD_COLOR */

       rects = rects->next;
    }
}

/*
 * Clear screen
 * (077C)
 */
void sysvid_clear(void)
{
    rb->memset(sysvid_fb, 0, sizeof(U8) * SYSVID_WIDTH * SYSVID_HEIGHT);
}

/*
 * Initialise video
 */
bool sysvid_init()
{
    bool success;

    if (isVideoInitialised)
    {
        return true;
    }

    IFDEBUG_VIDEO(sys_printf("xrick/video: start\n"););

    success = false;
    do
    {
        /* allocate xRick generic frame buffer into memory */
        sysvid_fb = sysmem_push(sizeof(U8) * SYSVID_WIDTH * SYSVID_HEIGHT);
        if (!sysvid_fb)
        {
            sys_error("(video) unable to allocate frame buffer");
            break;
        }

#ifndef HAVE_LCD_COLOR
        gbuf = sysmem_push(GREYBUFSIZE);
        if (!gbuf)
        {
            sys_error("(video) unable to allocate buffer for greyscale functions");
            break;
        }

        if (!grey_init(gbuf, GREYBUFSIZE, GREY_ON_COP, LCD_WIDTH, LCD_HEIGHT, NULL))
        {
            sys_error("(video) not enough memory to initialise greyscale functions");
            break;
        }
#endif /* ndef HAVE_LCD_COLOR */

        success = true;
    } while (false);

    if (!success)
    {
#ifndef HAVE_LCD_COLOR
        sysmem_pop(gbuf);
#endif
        sysmem_pop(sysvid_fb);
        return false;
    }

#if (LCD_HEIGHT < SYSVID_HEIGHT)
    calculateRowsToSkip();
#endif
#if (LCD_WIDTH < SYSVID_WIDTH)
    calculateColumnsToSkip();
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_ignore_timeout();

    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();

#ifdef HAVE_LCD_COLOR
    rb->lcd_update();
#else
    /* switch on greyscale overlay */
    grey_show(true);
#endif /* HAVE_LCD_COLOR */

    isVideoInitialised = true;
    IFDEBUG_VIDEO(sys_printf("xrick/video: ready\n"););
    return true;
}

/*
 * Shutdown video
 */
void sysvid_shutdown(void)
{
    if (!isVideoInitialised)
    {
        return;
    }

#ifndef HAVE_LCD_COLOR
    grey_show(false);
    grey_release();

    sysmem_pop(gbuf);
#endif /* ndef HAVE_LCD_COLOR */
    sysmem_pop(sysvid_fb);

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();

    isVideoInitialised = false;
    IFDEBUG_VIDEO(sys_printf("xrick/video: stop\n"););
}

/* eof */
