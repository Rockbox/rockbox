/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
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

#include <stdio.h>
#include "config.h"
#include "lcd.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "backdrop.h"

static fb_data main_backdrop[LCD_FBHEIGHT][LCD_FBWIDTH]
                __attribute__ ((aligned (16)));
static fb_data skin_backdrop[LCD_FBHEIGHT][LCD_FBWIDTH]
                __attribute__ ((aligned (16)));

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
static fb_remote_data
remote_skin_backdrop[LCD_REMOTE_FBHEIGHT][LCD_REMOTE_FBWIDTH];
static bool remote_skin_backdrop_valid = false;
#endif

static bool main_backdrop_valid = false;
static bool skin_backdrop_valid = false;

/* load a backdrop into a buffer */
static bool load_backdrop(const char* filename, fb_data* backdrop_buffer)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data=(char*)backdrop_buffer;
    ret = read_bmp_file(filename, &bm, sizeof(main_backdrop),
                        FORMAT_NATIVE | FORMAT_DITHER, NULL);

    return ((ret > 0)
            && (bm.width == LCD_WIDTH) && (bm.height == LCD_HEIGHT));
}

static bool load_main_backdrop(const char* filename)
{
    main_backdrop_valid = load_backdrop(filename, &main_backdrop[0][0]);
    return main_backdrop_valid;
}

static inline bool load_skin_backdrop(const char* filename)
{
    skin_backdrop_valid = load_backdrop(filename, &skin_backdrop[0][0]);
    return skin_backdrop_valid;
}

static inline void unload_main_backdrop(void)
{
    main_backdrop_valid = false;
}

static inline void unload_skin_backdrop(void)
{
    skin_backdrop_valid = false;
}

static inline void show_main_backdrop(void)
{
    lcd_set_backdrop(main_backdrop_valid ? &main_backdrop[0][0] : NULL);
}

static void show_skin_backdrop(void)
{
    /* if no wps backdrop, fall back to main backdrop */
    if(skin_backdrop_valid)
    {
        lcd_set_backdrop(&skin_backdrop[0][0]);
    }
    else
    {
        show_main_backdrop();
    }
}

/* api functions */

bool backdrop_load(enum backdrop_type bdrop, const char* filename)
{
    if (bdrop == BACKDROP_MAIN)
        return load_main_backdrop(filename);
    else if (bdrop == BACKDROP_SKIN_WPS)
        return load_skin_backdrop(filename);
    else
        return false;
}

void backdrop_unload(enum backdrop_type bdrop)
{
    if (bdrop == BACKDROP_MAIN)
        unload_main_backdrop();
    else if (bdrop == BACKDROP_SKIN_WPS)
        unload_skin_backdrop();
}

void backdrop_show(enum backdrop_type bdrop)
{
    if (bdrop == BACKDROP_MAIN)
        show_main_backdrop();
    else if (bdrop == BACKDROP_SKIN_WPS)
        show_skin_backdrop();
}

void backdrop_hide(void)
{
    lcd_set_backdrop(NULL);
}

    

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1

static bool load_remote_backdrop(const char* filename,
            fb_remote_data* backdrop_buffer)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data=(char*)backdrop_buffer;
    ret = read_bmp_file(filename, &bm, sizeof(main_backdrop),
                        FORMAT_NATIVE | FORMAT_DITHER | FORMAT_REMOTE, NULL);

    return ((ret > 0)
            && (bm.width == LCD_REMOTE_WIDTH)
            && (bm.height == LCD_REMOTE_HEIGHT));
}

static inline bool load_remote_skin_backdrop(const char* filename)
{
    remote_skin_backdrop_valid =
            load_remote_backdrop(filename, &remote_skin_backdrop[0][0]);
    return remote_skin_backdrop_valid;
}

static inline void unload_remote_skin_backdrop(void)
{
    remote_skin_backdrop_valid = false;
}

static inline void show_remote_main_backdrop(void)
{
    lcd_remote_set_backdrop(NULL);
}

static inline void show_remote_skin_backdrop(void)
{
    /* if no wps backdrop, fall back to main backdrop */
    if(remote_skin_backdrop_valid)
    {
        lcd_remote_set_backdrop(&remote_skin_backdrop[0][0]);
    }
    else
    {
        show_remote_main_backdrop();
    }
}


/* api functions */
bool remote_backdrop_load(enum backdrop_type bdrop,
                                const char *filename)
{
    if (bdrop == BACKDROP_SKIN_WPS)
        return load_remote_skin_backdrop(filename);
    else if (bdrop == BACKDROP_MAIN)
        return true;
    else
        return false;
}

void remote_backdrop_show(enum backdrop_type bdrop)
{
    if (bdrop == BACKDROP_MAIN)
        show_remote_main_backdrop();
    else if (bdrop == BACKDROP_SKIN_WPS)
        show_remote_skin_backdrop();
}

void remote_backdrop_unload(enum backdrop_type bdrop)
{
    if (bdrop != BACKDROP_MAIN)
        unload_remote_skin_backdrop();
}


void remote_backdrop_hide(void)
{
        lcd_remote_set_backdrop(NULL);
}

#endif

    
