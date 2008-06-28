/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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

#include <lcd.h>
#include <lcd-remote.h>
#include <scroll_engine.h>
#include "backlight.h"
#include <font.h>
#include <button.h>
#include <sprintf.h>
#include <settings.h>
#include <kernel.h>
#include <icons.h>

#include "screen_access.h"

struct screen screens[NB_SCREENS] =
{
    {
        .screen_type=SCREEN_MAIN,
        .width=LCD_WIDTH,
        .height=LCD_HEIGHT,
        .depth=LCD_DEPTH,
#if defined(HAVE_LCD_COLOR)
        .is_color=true,
#else
        .is_color=false,
#endif
#ifdef HAVE_LCD_BITMAP
        .pixel_format=LCD_PIXELFORMAT,
#endif
#if (CONFIG_LED == LED_VIRTUAL)
        .has_disk_led=false,
#elif defined(HAVE_REMOTE_LCD)
        .has_disk_led=true,
#endif
        .set_viewport=&lcd_set_viewport,
        .getwidth=&lcd_getwidth,
        .getheight=&lcd_getheight,
        .getstringsize=&lcd_getstringsize,
#ifdef HAVE_LCD_BITMAP
        .setfont=&lcd_setfont,
        .getfont=&lcd_getfont,
        .mono_bitmap=&lcd_mono_bitmap,
        .mono_bitmap_part=&lcd_mono_bitmap_part,
        .set_drawmode=&lcd_set_drawmode,
        .bitmap=(screen_bitmap_func*)&lcd_bitmap,
        .bitmap_part=(screen_bitmap_part_func*)&lcd_bitmap_part,
#if LCD_DEPTH <= 2
        /* No transparency yet for grayscale and mono lcd */
        .transparent_bitmap=(screen_bitmap_func*)&lcd_bitmap,
        .transparent_bitmap_part=(screen_bitmap_part_func*)&lcd_bitmap_part,
#else
        .transparent_bitmap=(screen_bitmap_func*)&lcd_bitmap_transparent,
        .transparent_bitmap_part=(screen_bitmap_part_func*)&lcd_bitmap_transparent_part,
#endif
#if LCD_DEPTH > 1
#if defined(HAVE_LCD_COLOR) && defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1
        .color_to_native=&lcd_color_to_native,
#endif
        .get_background=&lcd_get_background,
        .get_foreground=&lcd_get_foreground,
        .set_background=&lcd_set_background,
        .set_foreground=&lcd_set_foreground,
#ifdef HAVE_LCD_COLOR
        .set_selector_start=&lcd_set_selector_start,
        .set_selector_end=&lcd_set_selector_end,
        .set_selector_text=&lcd_set_selector_text,
#endif
#endif /* LCD_DEPTH > 1 */
        .update_rect=&lcd_update_rect,
        .update_viewport_rect=&lcd_update_viewport_rect,
        .fillrect=&lcd_fillrect,
        .drawrect=&lcd_drawrect,
        .drawpixel=&lcd_drawpixel,
        .drawline=&lcd_drawline,
        .vline=&lcd_vline,
        .hline=&lcd_hline,
        .scroll_step=&lcd_scroll_step,
        .puts_style_offset=&lcd_puts_style_offset,
        .puts_scroll_style=&lcd_puts_scroll_style,
        .puts_scroll_style_offset=&lcd_puts_scroll_style_offset,
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_CHARCELLS
        .double_height=&lcd_double_height,
        .putc=&lcd_putc,
        .get_locked_pattern=&lcd_get_locked_pattern,
        .define_pattern=&lcd_define_pattern,
        .unlock_pattern=&lcd_unlock_pattern,
        .icon=&lcd_icon,
#endif /* HAVE_LCD_CHARCELLS */

        .putsxy=&lcd_putsxy,
        .puts=&lcd_puts,
        .puts_offset=&lcd_puts_offset,
        .puts_scroll=&lcd_puts_scroll,
        .puts_scroll_offset=&lcd_puts_scroll_offset,
        .scroll_speed=&lcd_scroll_speed,
        .scroll_delay=&lcd_scroll_delay,
        .stop_scroll=&lcd_stop_scroll,
        .clear_display=&lcd_clear_display,
        .clear_viewport=&lcd_clear_viewport,
        .scroll_stop=&lcd_scroll_stop,
        .scroll_stop_line=&lcd_scroll_stop_line,
        .update=&lcd_update,
        .update_viewport=&lcd_update_viewport,
        .backlight_on=&backlight_on,
        .backlight_off=&backlight_off,
        .is_backlight_on=&is_backlight_on,
        .backlight_set_timeout=&backlight_set_timeout,
#ifdef HAVE_BUTTONBAR
        .has_buttonbar=false
#endif
    },
#if NB_SCREENS == 2
    {
        .screen_type=SCREEN_REMOTE,
        .width=LCD_REMOTE_WIDTH,
        .height=LCD_REMOTE_HEIGHT,
        .depth=LCD_REMOTE_DEPTH,
        .is_color=false,/* No color remotes yet */
        .pixel_format=LCD_REMOTE_PIXELFORMAT,
        .has_disk_led=false,
        .set_viewport=&lcd_remote_set_viewport,
        .getwidth=&lcd_remote_getwidth,
        .getheight=&lcd_remote_getheight,
        .getstringsize=&lcd_remote_getstringsize,
#if 1 /* all remote LCDs are bitmapped so far */
        .setfont=&lcd_remote_setfont,
        .getfont=&lcd_remote_getfont,
        .mono_bitmap=&lcd_remote_mono_bitmap,
        .mono_bitmap_part=&lcd_remote_mono_bitmap_part,
        .bitmap=(screen_bitmap_func*)&lcd_remote_bitmap,
        .bitmap_part=(screen_bitmap_part_func*)&lcd_remote_bitmap_part,
        .set_drawmode=&lcd_remote_set_drawmode,
#if LCD_REMOTE_DEPTH <= 2
        /* No transparency yet for grayscale and mono lcd */
        .transparent_bitmap=(screen_bitmap_func*)&lcd_remote_bitmap,
        .transparent_bitmap_part=(screen_bitmap_part_func*)&lcd_remote_bitmap_part,
        /* No colour remotes yet */
#endif
#if LCD_REMOTE_DEPTH > 1
#if defined(HAVE_LCD_COLOR)
        .color_to_native=&lcd_remote_color_to_native,
#endif
        .get_background=&lcd_remote_get_background,
        .get_foreground=&lcd_remote_get_foreground,
        .set_background=&lcd_remote_set_background,
        .set_foreground=&lcd_remote_set_foreground,
#endif /* LCD_REMOTE_DEPTH > 1 */
        .update_rect=&lcd_remote_update_rect,
        .update_viewport_rect=&lcd_remote_update_viewport_rect,
        .fillrect=&lcd_remote_fillrect,
        .drawrect=&lcd_remote_drawrect,
        .drawpixel=&lcd_remote_drawpixel,
        .drawline=&lcd_remote_drawline,
        .vline=&lcd_remote_vline,
        .hline=&lcd_remote_hline,
        .scroll_step=&lcd_remote_scroll_step,
        .puts_style_offset=&lcd_remote_puts_style_offset,
        .puts_scroll_style=&lcd_remote_puts_scroll_style,
        .puts_scroll_style_offset=&lcd_remote_puts_scroll_style_offset,
#endif /* 1 */

#if 0 /* no charcell remote LCDs so far */
        .double_height=&lcd_remote_double_height,
        .putc=&lcd_remote_putc,
        .get_locked_pattern=&lcd_remote_get_locked_pattern,
        .define_pattern=&lcd_remote_define_pattern,
        .icon=&lcd_remote_icon,
#endif /* 0 */
        .putsxy=&lcd_remote_putsxy,
        .puts=&lcd_remote_puts,
        .puts_offset=&lcd_remote_puts_offset,
        .puts_scroll=&lcd_remote_puts_scroll,
        .puts_scroll_offset=&lcd_remote_puts_scroll_offset,
        .scroll_speed=&lcd_remote_scroll_speed,
        .scroll_delay=&lcd_remote_scroll_delay,
        .stop_scroll=&lcd_remote_stop_scroll,
        .clear_display=&lcd_remote_clear_display,
        .clear_viewport=&lcd_remote_clear_viewport,
        .scroll_stop=&lcd_remote_scroll_stop,
        .scroll_stop_line=&lcd_remote_scroll_stop_line,
        .update=&lcd_remote_update,
        .update_viewport=&lcd_remote_update_viewport,
        .backlight_on=&remote_backlight_on,
        .backlight_off=&remote_backlight_off,
        .is_backlight_on=&is_remote_backlight_on,
        .backlight_set_timeout=&remote_backlight_set_timeout
    }
#endif /* HAVE_REMOTE_LCD */
};

#ifdef HAVE_LCD_BITMAP
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height)
{
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(xstart, ystart, width, height);
    display->set_drawmode(DRMODE_SOLID);
}
#endif

void screen_access_init(void)
{
    int i;
    struct screen *display;
    FOR_NB_SCREENS(i)
    {
        display = &screens[i];
#ifdef HAVE_LCD_BITMAP
        ((struct screen*)&screens[i])->setfont(FONT_UI);
#endif
        
        int height=display->height;
#ifdef HAVE_LCD_BITMAP
        if(global_settings.statusbar)
            height -= STATUSBAR_HEIGHT;
#ifdef HAVE_BUTTONBAR
        if(global_settings.buttonbar && display->has_buttonbar)
            height -= BUTTONBAR_HEIGHT;
#endif
        display->getstringsize((unsigned char *)"A", &display->char_width,
                                &display->char_height);
#else
        display->char_width  = 1;
        display->char_height = 1;
#endif
        display->nb_lines = height / display->char_height;
    }
}
