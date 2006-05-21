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
#include "backdrop.h"
#include "splash.h" /* debugging */

fb_data main_backdrop[LCD_HEIGHT][LCD_WIDTH];
fb_data wps_backdrop[LCD_HEIGHT][LCD_WIDTH];
bool main_backdrop_valid = false;
bool wps_backdrop_valid = false;

/* load a backdrop into a buffer */
bool load_backdrop(char* filename, fb_data* backdrop_buffer)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data=(char*)backdrop_buffer;
    ret = read_bmp_file(filename, &bm, sizeof(main_backdrop), FORMAT_NATIVE);

    if ((ret > 0) && (bm.width == LCD_WIDTH) && (bm.height == LCD_HEIGHT))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool load_main_backdrop(char* filename)
{
    main_backdrop_valid = load_backdrop(filename, &main_backdrop[0][0]);
/*    gui_syncsplash(100, true, "MAIN backdrop load: %s", main_backdrop_valid ? "OK" : "FAIL");*/
    return main_backdrop_valid;
}

bool load_wps_backdrop(char* filename)
{
    wps_backdrop_valid = load_backdrop(filename, &wps_backdrop[0][0]);
/*    gui_syncsplash(100, true, "WPS backdrop load: %s", main_backdrop_valid ? "OK" : "FAIL");*/
    return wps_backdrop_valid;
}

void unload_main_backdrop(void)
{
    main_backdrop_valid = false;
/*    gui_syncsplash(100, true, "MAIN backdrop unload");*/
}

void unload_wps_backdrop(void)
{
    wps_backdrop_valid = false;
/*    gui_syncsplash(100, true, "WPS backdrop unload");*/
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
/*        gui_syncsplash(100, true, "WPS backdrop show: fallback to MAIN");*/
        show_main_backdrop();
    }
}
