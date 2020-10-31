/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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

#include "plugin.h"

#ifdef HAVE_LCD_COLOR
#define BGCOLOR_1 LCD_RGBPACK(255,255,0)
#define BGCOLOR_2 LCD_RGBPACK(0,255,0)
#define FGCOLOR_1 LCD_RGBPACK(0,0,255)
#elif LCD_DEPTH > 1
#define BGCOLOR_1 LCD_DARKGRAY
#define BGCOLOR_2 LCD_LIGHTGRAY
#define FGCOLOR_1 LCD_WHITE
#endif

static struct viewport vp0 =
{
    .x        = 0,
    .y        = 0,
    .width    = LCD_WIDTH/ 2 + LCD_WIDTH / 3,
    .height   = 20,
    .font     = FONT_UI,
    .drawmode = DRMODE_SOLID,
#if LCD_DEPTH > 1
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = BGCOLOR_1,
#endif
};

static struct viewport vp1 =
{
    .x        = LCD_WIDTH / 10,
    .y        = 20,
    .width    = LCD_WIDTH / 3,
    .height   = LCD_HEIGHT / 2,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
#if LCD_DEPTH > 1
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = LCD_DEFAULT_BG,
#endif
};

static struct viewport vp2 =
{
    .x        = LCD_WIDTH / 2,
    .y        = 40,
    .width    = LCD_WIDTH / 3,
    .height   = (LCD_HEIGHT / 2),
    .font     = FONT_UI,
    .drawmode = DRMODE_SOLID,
#if LCD_DEPTH > 1
    .fg_pattern = FGCOLOR_1,
    .bg_pattern = BGCOLOR_2,
#endif
};


static struct viewport vp3 =
{
    .x        = LCD_WIDTH / 4,
    .y        = (5 * LCD_HEIGHT) / 8,
    .width    = LCD_WIDTH / 2,
    .height   = (LCD_HEIGHT / 4),
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
#if LCD_DEPTH > 1
    .fg_pattern = LCD_BLACK,
    .bg_pattern = LCD_WHITE,
#endif
};


#ifdef HAVE_REMOTE_LCD
static struct viewport rvp0 =
{
    .x        = 0,
    .y        = 10,
    .width    = LCD_REMOTE_WIDTH / 3,
    .height   = LCD_REMOTE_HEIGHT - 10,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
#if LCD_REMOTE_DEPTH > 1
    .fg_pattern = LCD_REMOTE_BLACK,
    .bg_pattern = LCD_REMOTE_LIGHTGRAY,
#endif
};

static struct viewport rvp1 =
{
    .x        = LCD_REMOTE_WIDTH / 2,
    .y        = 0,
    .width    = LCD_REMOTE_WIDTH / 3,
    .height   = LCD_REMOTE_HEIGHT - 10,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
#if LCD_REMOTE_DEPTH > 1
    .fg_pattern = LCD_REMOTE_DEFAULT_FG,
    .bg_pattern = LCD_REMOTE_DEFAULT_BG
#endif
};

#endif

static void *test_address_fn(int x, int y)
{
/* Address lookup function
 * core will use this to get an address from x/y coord
 * depending on the lcd function core sometimes uses this for
 * only the first and last address
 * and handles subsequent address based on stride */

    struct frame_buffer_t *fb = vp0.buffer;
/* LCD_STRIDEFORMAT & LCD_NATIVE_STRIDE macros allow Horiz screens to work with RB */
#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
    size_t element = (x * LCD_NATIVE_STRIDE(fb->stride)) + y;
#else
    size_t element = (y * LCD_NATIVE_STRIDE(fb->stride)) + x;
#endif
    /* use mod fb->elems to protect from buffer ovfl */
    return fb->fb_ptr + (element % fb->elems);
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    char buf[80];
    int i,y;

    size_t plugin_buf_len;
    void* plugin_buf = (unsigned char *)rb->plugin_get_buffer(&plugin_buf_len);

/* Here we will test if viewports of non standard size work with the rb core */
    struct frame_buffer_t fb;

    rb->font_getstringsize("W", NULL, &vp0.height, vp0.font);
    fb.elems = LCD_NBELEMS(vp0.width, vp0.height);

    /* set stride based on the with or height of our buffer (macro picks the proper one) */
    fb.stride = STRIDE_MAIN(vp0.width, vp0.height);

    /*set the framebuffer pointer to our buffer (union - pick appropriate data type) */
    fb.data = plugin_buf;
    /* Valid data types for fb union
      void*          - data
      char*          - ch_ptr,
      fb_data*       - fb_ptr,
      fb_remote_data - fb_remote_ptr;
    */
    if (fb.elems * sizeof(fb_data) > plugin_buf_len)
        return PLUGIN_ERROR;

    plugin_buf += fb.elems * sizeof(fb_data);
    plugin_buf_len -= fb.elems * sizeof(fb_data); /* buffer bookkeeping */

    /* set a frame buffer address lookup function */
    fb.get_address_fn = &test_address_fn;

    /* set our newly built buffer to the viewport */
    rb->viewport_set_buffer(&vp0, &fb, SCREEN_MAIN);

    rb->screens[SCREEN_MAIN]->set_viewport(&vp0);
    rb->screens[SCREEN_MAIN]->clear_viewport();

    rb->screens[SCREEN_MAIN]->puts_scroll(0,0,"Viewport testing plugin - this is a scrolling title");

    rb->screens[SCREEN_MAIN]->set_viewport(&vp1);
    rb->screens[SCREEN_MAIN]->clear_viewport();

    for (i = 0 ; i < 3; i++)
    {
        rb->snprintf(buf,sizeof(buf),"Left text, scrolling_line %d",i);
        rb->screens[SCREEN_MAIN]->puts_scroll(0,i,buf);
    }

    rb->screens[SCREEN_MAIN]->set_viewport(&vp2);
    rb->screens[SCREEN_MAIN]->clear_viewport();
    for (i = 1 ; i < 3; i++)
    {
        rb->snprintf(buf,sizeof(buf),"Right text, scrolling line %d",i);
        rb->screens[SCREEN_MAIN]->puts_scroll(1,i,buf);
    }

    y = -10;
    for (i = -10; i < vp2.width + 10; i += 5)
    {
        rb->screens[SCREEN_MAIN]->drawline(i, y, i, vp2.height - y);
    }

    rb->screens[SCREEN_MAIN]->set_viewport(&vp3);
    rb->screens[SCREEN_MAIN]->clear_viewport();
    for (i = 1 ; i < 2; i++)
    {
        rb->snprintf(buf,sizeof(buf),"Bottom text, a scrolling line %d",i);
        rb->screens[SCREEN_MAIN]->puts_scroll(2,i,buf);
    }
    rb->screens[SCREEN_MAIN]->puts_scroll(4,i,"Short line");
    rb->screens[SCREEN_MAIN]->update();


#ifdef HAVE_REMOTE_LCD
    rb->screens[SCREEN_REMOTE]->set_viewport(&rvp0);
    rb->screens[SCREEN_REMOTE]->clear_viewport();

    for (i = 0 ; i < 5; i++)
    {
        rb->snprintf(buf,sizeof(buf),"Left text, scrolling_line %d",i);
        rb->screens[SCREEN_REMOTE]->puts_scroll(0,i,buf);
    }
    rb->screens[SCREEN_REMOTE]->puts(1,i,"Static");

    rb->screens[SCREEN_REMOTE]->set_viewport(&rvp1);
    rb->screens[SCREEN_REMOTE]->clear_viewport();
    for (i = 1 ; i < 3; i++)
    {
        rb->snprintf(buf,sizeof(buf),"Right text, scrolling line %d",i);
        rb->screens[SCREEN_REMOTE]->puts_scroll(1,i,buf);
    }

    y = -10;
    for (i = -10; i < rvp1.width + 10; i += 5)
    {
        rb->screens[SCREEN_REMOTE]->drawline(i, y, i, rvp1.height - y);
    }

    rb->screens[SCREEN_REMOTE]->update();
#endif
    rb->button_clear_queue();
    while(rb->button_get(true) <= BUTTON_NONE)
     ;;

    rb->button_get(true);

    /* Restore the default viewport */
    rb->screens[SCREEN_MAIN]->set_viewport(NULL);
#ifdef HAVE_REMOTE_LCD
    rb->screens[SCREEN_REMOTE]->set_viewport(NULL);
#endif

    return PLUGIN_OK;
}
