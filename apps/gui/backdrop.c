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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#if LCD_DEPTH >= 8
static fb_data main_backdrop[LCD_HEIGHT][LCD_WIDTH]  __attribute__ ((aligned (16)));
static fb_data wps_backdrop[LCD_HEIGHT][LCD_WIDTH]  __attribute__ ((aligned (16)));
#elif LCD_DEPTH == 2
static fb_data main_backdrop[LCD_FBHEIGHT][LCD_FBWIDTH];
static fb_data wps_backdrop[LCD_FBHEIGHT][LCD_FBWIDTH];
#endif

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
static fb_remote_data remote_wps_backdrop[LCD_REMOTE_FBHEIGHT][LCD_REMOTE_FBWIDTH];
static bool remote_wps_backdrop_valid = false;
#endif

static bool main_backdrop_valid = false;
static bool wps_backdrop_valid = false;

/* load a backdrop into a buffer */
static bool load_backdrop(const char* filename, fb_data* backdrop_buffer)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data=(char*)backdrop_buffer;
    ret = read_bmp_file(filename, &bm, sizeof(main_backdrop),
                        FORMAT_NATIVE | FORMAT_DITHER);

    if ((ret > 0) && (bm.width == LCD_WIDTH) && (bm.height == LCD_HEIGHT))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool load_main_backdrop(const char* filename)
{
    main_backdrop_valid = load_backdrop(filename, &main_backdrop[0][0]);
    return main_backdrop_valid;
}

bool load_wps_backdrop(const char* filename)
{
    wps_backdrop_valid = load_backdrop(filename, &wps_backdrop[0][0]);
    return wps_backdrop_valid;
}

void unload_main_backdrop(void)
{
    main_backdrop_valid = false;
}

void unload_wps_backdrop(void)
{
    wps_backdrop_valid = false;
}

void show_main_backdrop(void)
{
    lcd_set_backdrop(main_backdrop_valid ? &main_backdrop[0][0] : NULL);
}

void show_wps_backdrop(void)
{
    /* if no wps backdrop, fall back to main backdrop */
    if(wps_backdrop_valid)
    {
        lcd_set_backdrop(&wps_backdrop[0][0]);
    }
    else
    {
        show_main_backdrop();
    }
}

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1

static bool load_remote_backdrop(const char* filename, fb_remote_data* backdrop_buffer)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data=(char*)backdrop_buffer;
    ret = read_bmp_file(filename, &bm, sizeof(main_backdrop),
                        FORMAT_NATIVE | FORMAT_DITHER | FORMAT_REMOTE);

    if ((ret > 0) && (bm.width == LCD_REMOTE_WIDTH) && (bm.height == LCD_REMOTE_HEIGHT))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool load_remote_wps_backdrop(const char* filename)
{
    remote_wps_backdrop_valid = load_remote_backdrop(filename, &remote_wps_backdrop[0][0]);
    return remote_wps_backdrop_valid;
}

void unload_remote_wps_backdrop(void)
{
    remote_wps_backdrop_valid = false;
}

void show_remote_wps_backdrop(void)
{
    /* if no wps backdrop, fall back to main backdrop */
    if(remote_wps_backdrop_valid)
    {
        lcd_remote_set_backdrop(&remote_wps_backdrop[0][0]);
    }
    else
    {
        show_remote_main_backdrop();
    }
}

void show_remote_main_backdrop(void)
{
    lcd_remote_set_backdrop(NULL);
}
#endif
