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

#include <stdio.h>
#include "config.h"
#include <lcd.h>
#ifdef HAVE_REMOTE_LCD
#include <lcd-remote.h>
#endif
#include "scroll_engine.h"
#include <font.h>
#include <button.h>
#include <settings.h>
#include <kernel.h>
#include <icons.h>

#include "backlight.h"
#include "screen_access.h"
#include "backdrop.h"
#include "viewport.h"

/* some helper functions to calculate metrics on the fly */
static int screen_helper_getcharwidth(void)
{
    return font_get(lcd_getfont())->maxwidth;
}

static int screen_helper_getcharheight(void)
{
    return font_get(lcd_getfont())->height;
}

static int screen_helper_getnblines(void)
{
    int height=screens[0].lcdheight;
    if(global_settings.statusbar != STATUSBAR_OFF)
        height -= STATUSBAR_HEIGHT;
    return height / screens[0].getcharheight();
}

void screen_helper_setfont(int font)
{
    (void)font;
    if (font == FONT_UI)
        font = global_status.font_id[SCREEN_MAIN];
    lcd_setfont(font);
}

static int screen_helper_getuifont(void)
{
    return global_status.font_id[SCREEN_MAIN];
}

static void screen_helper_setuifont(int font)
{
    global_status.font_id[SCREEN_MAIN] = font;
}

static void screen_helper_set_drawmode(int mode)
{
    lcd_set_drawmode(mode);
}

static void screen_helper_put_line(int x, int y, struct line_desc *line,
                                   const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vput_line(&screens[SCREEN_MAIN], x, y, line, fmt, ap);
    va_end(ap);
}

void screen_helper_lcd_viewport_set_buffer(struct viewport *vp, struct frame_buffer_t *buffer)
{
    viewport_set_buffer(vp, buffer, SCREEN_MAIN);
}

#if NB_SCREENS == 2
void screen_helper_lcd_remote_viewport_set_buffer(struct viewport *vp, struct frame_buffer_t *buffer)
{
    viewport_set_buffer(vp, buffer, SCREEN_REMOTE);
}

static int screen_helper_remote_getcharwidth(void)
{
    return font_get(lcd_remote_getfont())->maxwidth;
}

static int screen_helper_remote_getcharheight(void)
{
    return font_get(lcd_remote_getfont())->height;
}

static int screen_helper_remote_getnblines(void)
{
    int height=screens[1].lcdheight;
    if(global_settings.statusbar != STATUSBAR_OFF)
        height -= STATUSBAR_HEIGHT;
    return height / screens[1].getcharheight();
}

void screen_helper_remote_setfont(int font)
{
    if (font == FONT_UI)
        font = global_status.font_id[SCREEN_REMOTE];
    lcd_remote_setfont(font);
}

static int screen_helper_remote_getuifont(void)
{
    return global_status.font_id[SCREEN_REMOTE];
}

static void screen_helper_remote_setuifont(int font)
{
    global_status.font_id[SCREEN_REMOTE] = font;
}

static void screen_helper_remote_put_line(int x, int y, struct line_desc *line,
                                          const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vput_line(&screens[SCREEN_REMOTE], x, y, line, fmt, ap);
    va_end(ap);
}

#endif

struct screen screens[NB_SCREENS] =
{
    {
        .screen_type=SCREEN_MAIN,
        .lcdwidth=LCD_WIDTH,
        .lcdheight=LCD_HEIGHT,
        .depth=LCD_DEPTH,
        .getnblines=&screen_helper_getnblines,
#if defined(HAVE_LCD_COLOR)
        .is_color=true,
#else
        .is_color=false,
#endif
        .pixel_format=LCD_PIXELFORMAT,
        .getcharwidth=screen_helper_getcharwidth,
        .getcharheight=screen_helper_getcharheight,
#if (CONFIG_LED == LED_VIRTUAL)
        .has_disk_led=false,
#elif defined(HAVE_REMOTE_LCD)
        .has_disk_led=true,
#endif
        .set_drawmode=&screen_helper_set_drawmode,
        .init_viewport=&lcd_init_viewport,
        .set_viewport=&lcd_set_viewport,
        .set_viewport_ex=&lcd_set_viewport_ex,
        .viewport_set_buffer = &screen_helper_lcd_viewport_set_buffer,
        .current_viewport = &lcd_current_viewport,
        .getwidth=&lcd_getwidth,
        .getheight=&lcd_getheight,
        .getstringsize=&lcd_getstringsize,
        .setfont=screen_helper_setfont,
        .getuifont=screen_helper_getuifont,
        .setuifont=screen_helper_setuifont,
        .mono_bitmap=&lcd_mono_bitmap,
        .mono_bitmap_part=&lcd_mono_bitmap_part,
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
        .bmp = &lcd_bmp,
        .bmp_part = &lcd_bmp_part,
        .nine_segment_bmp = &lcd_nine_segment_bmp,
#if LCD_DEPTH > 1
#if defined(HAVE_LCD_COLOR) && defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1
        .color_to_native=&lcd_color_to_native,
#endif
        .get_background=&lcd_get_background,
        .get_foreground=&lcd_get_foreground,
        .set_background=&lcd_set_background,
        .set_foreground=&lcd_set_foreground,
#endif /* LCD_DEPTH > 1 */
        .update_rect=&lcd_update_rect,
        .update_viewport_rect=&lcd_update_viewport_rect,
        .fillrect=&lcd_fillrect,
        .drawrect=&lcd_drawrect,
        .draw_border_viewport=&lcd_draw_border_viewport,
        .fill_viewport=&lcd_fill_viewport,
        .drawpixel=&lcd_drawpixel,
        .drawline=&lcd_drawline,
        .vline=&lcd_vline,
        .hline=&lcd_hline,
        .scroll_step=&lcd_scroll_step,

        .putsxy=&lcd_putsxy,
        .puts=&lcd_puts,
        .putsf=&lcd_putsf,
        .puts_scroll=&lcd_puts_scroll,
        .putsxy_scroll_func=&lcd_putsxy_scroll_func,
        .scroll_speed=&lcd_scroll_speed,
        .scroll_delay=&lcd_scroll_delay,
        .clear_display=&lcd_clear_display,
        .clear_viewport=&lcd_clear_viewport,
        .scroll_stop_viewport_rect=&lcd_scroll_stop_viewport_rect,
        .scroll_stop=&lcd_scroll_stop,
        .scroll_stop_viewport=&lcd_scroll_stop_viewport,
        .update=&lcd_update,
        .update_viewport=&lcd_update_viewport,
        .backlight_on=&backlight_on,
        .backlight_off=&backlight_off,
        .is_backlight_on=&is_backlight_on,
        .backlight_set_timeout=&backlight_set_timeout,
#if LCD_DEPTH > 1
        .backdrop_load=&backdrop_load,
        .backdrop_show=&backdrop_show,
#endif
#if defined(HAVE_LCD_COLOR)
        .gradient_fillrect = lcd_gradient_fillrect,
        .gradient_fillrect_part = lcd_gradient_fillrect_part,
#endif
        .put_line = screen_helper_put_line,
    },
#if NB_SCREENS == 2
    {
        .screen_type=SCREEN_REMOTE,
        .lcdwidth=LCD_REMOTE_WIDTH,
        .lcdheight=LCD_REMOTE_HEIGHT,
        .depth=LCD_REMOTE_DEPTH,
        .getnblines=&screen_helper_remote_getnblines,
        .is_color=false,/* No color remotes yet */
        .pixel_format=LCD_REMOTE_PIXELFORMAT,
        .getcharwidth=screen_helper_remote_getcharwidth,
        .getcharheight=screen_helper_remote_getcharheight,
        .has_disk_led=false,
        .set_drawmode=&lcd_remote_set_drawmode,
        .init_viewport=&lcd_remote_init_viewport,
        .set_viewport=&lcd_remote_set_viewport,
        .set_viewport_ex=&lcd_remote_set_viewport_ex,
        .viewport_set_buffer = &screen_helper_lcd_remote_viewport_set_buffer,
        .current_viewport = &lcd_remote_current_viewport,
        .getwidth=&lcd_remote_getwidth,
        .getheight=&lcd_remote_getheight,
        .getstringsize=&lcd_remote_getstringsize,
#if 1 /* all remote LCDs are bitmapped so far */
        .setfont=screen_helper_remote_setfont,
        .getuifont=screen_helper_remote_getuifont,
        .setuifont=screen_helper_remote_setuifont,
        .mono_bitmap=&lcd_remote_mono_bitmap,
        .mono_bitmap_part=&lcd_remote_mono_bitmap_part,
        .bitmap=(screen_bitmap_func*)&lcd_remote_bitmap,
        .bitmap_part=(screen_bitmap_part_func*)&lcd_remote_bitmap_part,
#if LCD_REMOTE_DEPTH <= 2
        /* No transparency yet for grayscale and mono lcd */
        .transparent_bitmap=(screen_bitmap_func*)&lcd_remote_bitmap,
        .transparent_bitmap_part=(screen_bitmap_part_func*)&lcd_remote_bitmap_part,
        /* No colour remotes yet */
#endif
        .bmp = &lcd_remote_bmp,
        .bmp_part = &lcd_remote_bmp_part,
        .nine_segment_bmp = &lcd_remote_nine_segment_bmp,
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
        .draw_border_viewport=&lcd_remote_draw_border_viewport,
        .fill_viewport=&lcd_remote_fill_viewport,
        .drawpixel=&lcd_remote_drawpixel,
        .drawline=&lcd_remote_drawline,
        .vline=&lcd_remote_vline,
        .hline=&lcd_remote_hline,
        .scroll_step=&lcd_remote_scroll_step,
#endif /* 1 */

        .putsxy=&lcd_remote_putsxy,
        .puts=&lcd_remote_puts,
        .putsf=&lcd_remote_putsf,
        .puts_scroll=&lcd_remote_puts_scroll,
        .putsxy_scroll_func=&lcd_remote_putsxy_scroll_func,
        .scroll_speed=&lcd_remote_scroll_speed,
        .scroll_delay=&lcd_remote_scroll_delay,
        .clear_display=&lcd_remote_clear_display,
        .clear_viewport=&lcd_remote_clear_viewport,
        .scroll_stop_viewport_rect=&lcd_remote_scroll_stop_viewport_rect,
        .scroll_stop=&lcd_remote_scroll_stop,
        .scroll_stop_viewport=&lcd_remote_scroll_stop_viewport,
        .update=&lcd_remote_update,
        .update_viewport=&lcd_remote_update_viewport,
        .backlight_on=&remote_backlight_on,
        .backlight_off=&remote_backlight_off,
        .is_backlight_on=&is_remote_backlight_on,
        .backlight_set_timeout=&remote_backlight_set_timeout,

#if LCD_DEPTH > 1
        .backdrop_load=&remote_backdrop_load,
        .backdrop_show=&remote_backdrop_show,
#endif
        .put_line = screen_helper_remote_put_line,
    }
#endif /* NB_SCREENS == 2 */
};

void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height)
{
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(xstart, ystart, width, height);
    display->set_drawmode(DRMODE_SOLID);
}
