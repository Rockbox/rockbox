/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: helloworld.c 12807 2007-03-16 21:56:08Z amiconn $
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

PLUGIN_HEADER

static const struct plugin_api* rb;

#ifdef HAVE_LCD_BITMAP

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
    .width    = LCD_WIDTH,
    .height   = 20,
    .font     = FONT_UI,
    .drawmode = DRMODE_SOLID,
#if LCD_DEPTH > 1
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = BGCOLOR_1,
#endif
#ifdef HAVE_LCD_COLOR
    .lss_pattern = LCD_DEFAULT_BG,
    .lse_pattern = LCD_DEFAULT_BG,
    .lst_pattern = LCD_DEFAULT_BG,
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
#ifdef HAVE_LCD_COLOR
    .lss_pattern = LCD_DEFAULT_BG,
    .lse_pattern = LCD_DEFAULT_BG,
    .lst_pattern = LCD_DEFAULT_BG,
#endif
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
#ifdef HAVE_LCD_COLOR
    .lss_pattern = LCD_DEFAULT_BG,
    .lse_pattern = LCD_DEFAULT_BG,
    .lst_pattern = LCD_DEFAULT_BG,
#endif
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
#ifdef HAVE_LCD_COLOR
    .lss_pattern = LCD_DEFAULT_BG,
    .lse_pattern = LCD_DEFAULT_BG,
    .lst_pattern = LCD_DEFAULT_BG,
#endif
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


enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    (void)parameter;
    char buf[80];
    int i,y;

    rb = api;

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

    rb->button_get(true);

    /* Restore the default viewport */
    rb->screens[SCREEN_MAIN]->set_viewport(NULL);
#ifdef HAVE_REMOTE_LCD
    rb->screens[SCREEN_REMOTE]->set_viewport(NULL);
#endif

    return PLUGIN_OK;
}


#else

/* Charcell version of plugin */

static struct viewport vp0 =
{
    .x        = 0,
    .y        = 0,
    .width    = 5,
    .height   = 1,
};

static struct viewport vp1 =
{
    .x        = 6,
    .y        = 0,
    .width    = 5,
    .height   = 1,
};

static struct viewport vp2 =
{
    .x        = 0,
    .y        = 1,
    .width    = LCD_WIDTH,
    .height   = 1,
};


enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    (void)parameter;

    rb = api;

    rb->screens[SCREEN_MAIN]->set_viewport(&vp0);
    rb->screens[SCREEN_MAIN]->clear_viewport();
    rb->screens[SCREEN_MAIN]->puts_scroll(0,0,"Rockbox");

    rb->screens[SCREEN_MAIN]->set_viewport(&vp1);
    rb->screens[SCREEN_MAIN]->clear_viewport();
    rb->screens[SCREEN_MAIN]->puts_scroll(0,0,"Viewports");

    rb->screens[SCREEN_MAIN]->set_viewport(&vp2);
    rb->screens[SCREEN_MAIN]->clear_viewport();
    rb->screens[SCREEN_MAIN]->puts_scroll(0,0,"Demonstration");

    rb->screens[SCREEN_MAIN]->update();

    rb->button_get(true);

    /* Restore the default viewport */
    rb->screens[SCREEN_MAIN]->set_viewport(NULL);

    return PLUGIN_OK;
}

#endif /* !HAVE_LCD_BITMAP */
