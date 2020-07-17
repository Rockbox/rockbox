/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Michael Sevakis
 *
 * Helper defines for writing code for pgfx, grey and color targets.
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
#ifndef MYLCD_H
#define MYLCD_H

/***
 * Most functions are, other than color depth, equivalent between pgfx, grey,
 * lcd and xlcd and most of the time the caller need not be concerned with
 * which is actually called, making code nicer to read and maintain.
 *
 * Unbuffered routines revert to standard rb->lcd_XXXX funtions on color
 * targets. On color, mylcd_ub_update_XXXX refer to the proper update
 * functions, otherwise they are no-ops.
 *
 * lib/grey.h should be included before including this
 * header. For bitmap LCD's, defaults to rb->lcd_XXXX otherwise.
 */
#if (LCD_DEPTH < 4) && defined(__GREY_H__)
#define MYLCD_CFG_GREYLIB           /* using greylib */
#define mylcd_(fn)                  grey_##fn
#define myxlcd_(fn)                 grey_##fn
#define mylcd_ub_(fn)               grey_ub_##fn
#define myxlcd_ub_(fn)              grey_ub_##fn
#define mylcd_viewport_(fn)         grey_viewport_##fn

/* Common colors */
#define MYLCD_BLACK                 GREY_BLACK
#define MYLCD_DARKGRAY              GREY_DARKGRAY
#define MYLCD_LIGHTGRAY             GREY_LIGHTGRAY
#define MYLCD_WHITE                 GREY_WHITE
#define MYLCD_DEFAULT_FG            GREY_BLACK
#define MYLCD_DEFAULT_BG            GREY_WHITE

#else

#define MYLCD_CFG_RB_XLCD           /* using standard (X)LCD routines */
#define mylcd_(fn)                  rb->lcd_##fn
#define myxlcd_(fn)                 xlcd_##fn
#define mylcd_ub_(fn)               rb->lcd_##fn
#define myxlcd_ub_(fn)              xlcd_##fn
#define mylcd_viewport_(fn)         rb->viewport_##fn

/* Common colors */
#define MYLCD_BLACK                 LCD_BLACK
#define MYLCD_DARKGRAY              LCD_DARKGRAY
#define MYLCD_LIGHTGRAY             LCD_LIGHTGRAY
#define MYLCD_WHITE                 LCD_WHITE
#define MYLCD_DEFAULT_FG            LCD_DEFAULT_FG
#define MYLCD_DEFAULT_BG            LCD_DEFAULT_BG

#endif /* end LCD type selection */

/* Update functions */
#define mylcd_update                mylcd_(update)
#define mylcd_update_rect           mylcd_(update_rect)

/* Update functions - unbuffered : special handling for these
 * It is desirable to still evaluate arguments even if there will
 * be no function call, just in case they have side-effects.
 */
#if defined (MYLCD_CFG_PGFX)
#define mylcd_ub_update             pgfx_update
static inline void mylcd_ub_update_rect(int x, int y, int w, int h)
    { (void)x; (void)y; (void)w; (void)h; pgfx_update(); }

#elif defined (MYLCD_CFG_GREYLIB)
static inline void mylcd_ub_update(void)
    {}
static inline void mylcd_ub_update_rect(int x, int y, int w, int h)
    { (void)x; (void)y; (void)w; (void)h; }

#else /* color or RB default */
#define mylcd_ub_update             rb->lcd_update
#define mylcd_ub_update_rect        rb->lcd_update_rect
#endif

/* Parameter handling */
#define mylcd_set_drawmode          mylcd_(set_drawmode)
#define mylcd_get_drawmode          mylcd_(get_drawmode)

#define mylcd_set_foreground        mylcd_(set_foreground)
#define mylcd_get_foreground        mylcd_(get_foreground)
#define mylcd_set_background        mylcd_(set_background)
#define mylcd_get_background        mylcd_(get_background)
#define mylcd_set_drawinfo          mylcd_(set_drawinfo)
#define mylcd_setfont               mylcd_(setfont)
#define mylcd_getstringsize         mylcd_(getstringsize)

/* Whole display */
#define mylcd_clear_display         mylcd_(clear_display)

/* Whole display - unbuffered */
#define mylcd_ub_clear_display      mylcd_ub_(clear_display)

/* Pixel */
#define mylcd_drawpixel             mylcd_(drawpixel)

/* Lines */
#define mylcd_drawline              mylcd_(drawline)
#define mylcd_hline                 mylcd_(hline)
#define mylcd_vline                 mylcd_(vline)
#define mylcd_drawrect              mylcd_(drawrect)

/* Filled Primitives */
#define mylcd_fillrect              mylcd_(fillrect)
#define mylcd_filltriangle          myxlcd_(filltriangle)

/* Bitmaps */
#define mylcd_mono_bitmap_part      mylcd_(mono_bitmap_part)
#define mylcd_mono_bitmap           mylcd_(mono_bitmap)

#define mylcd_gray_bitmap_part      myxlcd_(gray_bitmap_part)
#define mylcd_gray_bitmap           myxlcd_(gray_bitmap)
#if 0 /* possible, but not implemented in greylib */
#define mylcd_color_bitmap_part     myxlcd_(color_bitmap_part)
#define mylcd_color_bitmap          myxlcd_(color_bitmap)
#endif

/* Bitmaps - unbuffered */
#define mylcd_ub_gray_bitmap_part   myxlcd_ub_(gray_bitmap_part)
#define mylcd_ub_gray_bitmap        myxlcd_ub_(gray_bitmap)

/* Text */
/* lcd_putsxyofs is static'ed in the core for now on color */
#define mylcd_putsxyofs             mylcd_(putsxyofs)
#define mylcd_putsxy                mylcd_(putsxy)

/* Scrolling */
#define mylcd_scroll_left           myxlcd_(scroll_left)
#define mylcd_scroll_right          myxlcd_(scroll_right)
#define mylcd_scroll_up             myxlcd_(scroll_up)
#define mylcd_scroll_down           myxlcd_(scroll_down)

/* Scrolling - unbuffered */
#define mylcd_ub_scroll_left        myxlcd_ub_(scroll_left)
#define mylcd_ub_scroll_right       myxlcd_ub_(scroll_right)
#define mylcd_ub_scroll_up          myxlcd_ub_(scroll_up)
#define mylcd_ub_scroll_down        myxlcd_ub_(scroll_down)

/* Viewports */
#define mylcd_clear_viewport          mylcd_(clear_viewport)
#define mylcd_set_viewport            mylcd_(set_viewport)
#define mylcd_viewport_set_fullscreen mylcd_viewport_(set_fullscreen)

#endif /* MYLCD_H */
