/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Jonathan Gordon (2006)
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
#include "config.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "kernel.h"
#include "system.h"
#include "screen_access.h"
#include "font.h"
#include "debug.h"
#include "misc.h"
#include "settings.h"
#include "scrollbar.h"
#include "lang.h"
#include "splash.h"
#include "action.h"
#include "icon.h"
#include "color_picker.h"
#include "viewport.h"

/* structure for color info */
struct rgb_pick
{
    unsigned color;                 /* native color value            */
    union
    {
        unsigned char rgb_val[6];   /* access to components as array */
        struct
        {
            unsigned char r;        /* native red value              */
            unsigned char g;        /* native green value            */
            unsigned char b;        /* native blue value             */
            unsigned char red;      /* 8 bit red value               */
            unsigned char green;    /* 8 bit green value             */
            unsigned char blue;     /* 8 bit blue value              */
        } __attribute__ ((__packed__)); /* assume byte packing       */
    };
};


/* list of primary colors */
#define SB_PRIM 0
#define SB_FILL 1
static const unsigned prim_rgb[][3] =
{
    /* Foreground colors for sliders */
    {
        LCD_RGBPACK(255,   0,   0),
        LCD_RGBPACK(  0, 255,   0),
        LCD_RGBPACK(  0,   0, 255),
    },
    /* Fill colors for sliders */
    {
        LCD_RGBPACK( 85,   0,   0),
        LCD_RGBPACK(  0,  85,   0),
        LCD_RGBPACK(  0,   0,  85),
    },
};

/* maximum values for components */
static const unsigned char rgb_max[3] =
{
    LCD_MAX_RED,
    LCD_MAX_GREEN,
    LCD_MAX_BLUE
};

/* Unpacks the color value into native rgb values and 24 bit rgb values */
static void unpack_rgb(struct rgb_pick *rgb)
{
    unsigned color = rgb->color;
    rgb->red   = RGB_UNPACK_RED(color);
    rgb->green = RGB_UNPACK_GREEN(color);
    rgb->blue  = RGB_UNPACK_BLUE(color);
    rgb->r     = RGB_UNPACK_RED_LCD(color);
    rgb->g     = RGB_UNPACK_GREEN_LCD(color);
    rgb->b     = RGB_UNPACK_BLUE_LCD(color);
}

/* Packs the native rgb colors into a color value */
static inline void pack_rgb(struct rgb_pick *rgb)
{
    rgb->color = LCD_RGBPACK_LCD(rgb->r, rgb->g, rgb->b);
}

/* Returns LCD_BLACK if the color is above a threshold brightness
   else return LCD_WHITE */
static inline unsigned get_black_or_white(const struct rgb_pick *rgb)
{
    return (2*rgb->red + 5*rgb->green + rgb->blue) >= 1024 ?
        LCD_BLACK : LCD_WHITE;
}

#define MARGIN_TOP              2 /* Top margin of screen                 */
#define MARGIN_BOTTOM           6 /* Bottom margin of screen              */
#define SLIDER_TEXT_MARGIN      2 /* Gap between text and sliders         */
#define TITLE_MARGIN_BOTTOM     4 /* Space below title bar                */
#define SELECTOR_TB_MARGIN      1 /* Margin on top and bottom of selector */
#define SWATCH_TOP_MARGIN       4 /* Space between last slider and swatch */
#define SELECTOR_WIDTH          get_icon_width(display->screen_type)
#define SELECTOR_HEIGHT         8 /* Height of > and < bitmaps            */

/* Figure out widest label character in case they vary -
   this function assumes labels are one character */
static int label_get_max_width(struct screen *display)
{
    int max_width, i;
    char buf[4];
    for (i = 0, max_width = 0; i < 3; i++)
    {
        int width;
        buf[0] = str(LANG_COLOR_RGB_LABELS)[i];
        buf[1] = '\0';
        width = display->getstringsize(buf, NULL, NULL);
        if (width > max_width)
            max_width = width;
    }
    return max_width;
}

static void draw_screen(struct screen *display, char *title,
                        struct rgb_pick *rgb, int row)
{
    unsigned  text_color       = LCD_BLACK;
    unsigned  background_color = LCD_WHITE;
    int       i, char_height, line_height;
    int       max_label_width;
    int       text_x, text_top;
    int       slider_x, slider_width;
    int       value_width;
    bool      display_three_rows;
    struct viewport vp;

    viewport_set_defaults(&vp, display->screen_type);
    struct viewport * last_vp = display->set_viewport(&vp);

    display->clear_viewport();

    if (display->depth > 1)
    {
        text_color       = display->get_foreground();
        background_color = display->get_background();
    }

    /* Draw title string */
    display->set_drawinfo(DRMODE_SOLID, text_color, background_color);
    vp.flags |= VP_FLAG_ALIGN_CENTER;
    display->putsxy(0, MARGIN_TOP, title);

    /* Get slider positions and top starting position */
    max_label_width = label_get_max_width(display);
    char_height  = display->getcharheight();
    text_top     = MARGIN_TOP + char_height +
                   TITLE_MARGIN_BOTTOM + SELECTOR_TB_MARGIN;
    text_x       = SELECTOR_WIDTH;
    slider_x     = text_x + max_label_width + SLIDER_TEXT_MARGIN;
    slider_width = vp.width - text_x - slider_x - SLIDER_TEXT_MARGIN;
    if (display->depth >= 24)
        display->getstringsize("255", &value_width, NULL);
    else
        display->getstringsize("63", &value_width, NULL);
    slider_width -= value_width;
    line_height  = char_height + 2*SELECTOR_TB_MARGIN;

    /* Find out if there's enough room for three sliders or just
       enough to display the selected slider - calculate total height
       of display with three sliders present */
    display_three_rows =
        vp.height >=
            text_top + line_height*3  + /* Title + 3 sliders     */
            SWATCH_TOP_MARGIN         + /* at least 2 lines      */
            char_height*2             + /*  + margins for bottom */
            MARGIN_BOTTOM;              /* colored rectangle     */

    for (i = 0; i < 3; i++)
    {
        unsigned sb_flags = HORIZONTAL;
        int      mode     = DRMODE_SOLID;
        unsigned fg       = text_color;
        unsigned bg       = background_color;

        if (i == row)
        {
            display->set_drawinfo(DRMODE_SOLID, text_color, background_color);

            if (global_settings.cursor_style != 0)
            {
                /* Draw solid bar selection bar */
                display->fillrect(0,
                                  text_top - SELECTOR_TB_MARGIN,
                                  vp.width,
                                  char_height + SELECTOR_TB_MARGIN*2);

                if (display->depth < 16)
                {
                    sb_flags |= FOREGROUND | INNER_FILL;
                    mode     |= DRMODE_INVERSEVID;
                }
            }
            else if (display_three_rows)
            {
                /* Draw ">    <" around sliders */
                int top = text_top + (char_height - SELECTOR_HEIGHT) / 2;
                screen_put_iconxy(display, 0, top, Icon_Cursor);
                screen_put_iconxy(display,
                                  vp.width - SELECTOR_WIDTH,
                                  top, Icon_Cursor);
            }

            if (display->depth >= 16)
            {
                sb_flags |= FOREGROUND | INNER_BGFILL;
                mode      = DRMODE_FG;
                fg        = prim_rgb[SB_PRIM][i];
                bg        = prim_rgb[SB_FILL][i];
            }
        }
        else if (!display_three_rows)
            continue;

        display->set_drawinfo(mode, fg, bg);

        /* Draw label */
        vp.flags &= ~VP_FLAG_ALIGNMENT_MASK;
        display->putsxyf(text_x, text_top, "%c", str(LANG_COLOR_RGB_LABELS)[i]);
        /* Draw color value */
        vp.flags |= VP_FLAG_ALIGN_RIGHT;
        if (display->depth >= 24)
            display->putsxyf(text_x, text_top, "%03d", rgb->rgb_val[i] & 0xFF);
        else
            display->putsxyf(text_x, text_top, "%02d", rgb->rgb_val[i] & 0x3F);



        /* Draw scrollbar */
        gui_scrollbar_draw(display,                     /* screen */
                           slider_x,                    /* x */
                           text_top + char_height / 4,  /* y */
                           slider_width,                /* width */
                           char_height / 2,             /* height */
                           rgb_max[i],                  /* items */
                           0,                           /* min_shown */
                           rgb->rgb_val[i],             /* max_shown */
                           sb_flags);                   /* flags */

        /* Advance to next line */
        text_top += line_height;
    } /* end for */

    vp.flags |= VP_FLAG_ALIGN_CENTER;
    if (display->depth >= 16)
    {
        /* Display color swatch on color screens only */
        int top    = text_top + SWATCH_TOP_MARGIN;
        int width  = vp.width - text_x*2;
        int height = vp.height - top - MARGIN_BOTTOM;

        /* Only draw if room */
        if (height >= char_height + 2)
        {
            /* draw the big rectangle */
            display->set_foreground(rgb->color);
            display->fillrect(text_x, top, width, height);

            /* Draw RGB: #rrggbb in middle of swatch */
            display->set_drawinfo(DRMODE_FG, get_black_or_white(rgb),
                         background_color);
            /* Format RGB: #rrggbb */
            display->putsxyf(0, top + (height - char_height) / 2,
                str(LANG_COLOR_RGB_VALUE), rgb->red, rgb->green, rgb->blue);

            /* Draw border around the rect */
            display->set_drawinfo(DRMODE_SOLID, text_color, background_color);
            display->drawrect(text_x, top, width, height);
        }
    }
    else
    {
        /* Display RGB value only centered on remaining display if room */
        int top = text_top + SWATCH_TOP_MARGIN;
        int height = vp.height - top - MARGIN_BOTTOM;

        if (height >= char_height)
        {
            display->set_drawinfo(DRMODE_SOLID, text_color, background_color);
            /* Format RGB: #rrggbb */
            display->putsxyf(0, top + (height - char_height) / 2,
                    str(LANG_COLOR_RGB_VALUE), rgb->red, rgb->green, rgb->blue);
        }
    }

    display->update_viewport();
    display->set_viewport(last_vp);
}

#ifdef HAVE_TOUCHSCREEN
static int touchscreen_slider(struct screen *display,
                                struct rgb_pick *rgb,
                                int *selected_slider)
{
    short     x, y;
    int       char_height, line_height;
    int       max_label_width, value_width;
    int       text_top, slider_x, slider_width;
    bool      display_three_rows;
    int       button;
    int       pressed_slider;
    struct viewport vp;

    viewport_set_defaults(&vp, display->screen_type);
    struct viewport *last_vp = display->set_viewport(&vp);

    button = action_get_touchscreen_press_in_vp(&x, &y, &vp);
    if (button == ACTION_UNKNOWN || button == BUTTON_NONE)
        return ACTION_NONE;
    /* Get slider positions and top starting position
     * need vp.y here, because of the statusbar, since touchscreen
     * coordinates are absolute */
    max_label_width = label_get_max_width(display);
    char_height  = display->getcharheight();
    text_top     = MARGIN_TOP + char_height +
                   TITLE_MARGIN_BOTTOM + SELECTOR_TB_MARGIN;
    slider_x     = SELECTOR_WIDTH + max_label_width + SLIDER_TEXT_MARGIN;
    slider_width = vp.width - SELECTOR_WIDTH - slider_x - SLIDER_TEXT_MARGIN;
    if (display->depth >= 24)
        display->getstringsize("255", &value_width, NULL);
    else
        display->getstringsize("63", &value_width, NULL);
    slider_width -= value_width;
    line_height  = char_height + 2*SELECTOR_TB_MARGIN;

    /* same logic as in draw_screen */
    /* Find out if there's enough room for three sliders or just
       enough to display the selected slider - calculate total height
       of display with three sliders present */
    display_three_rows =
        vp.height >=
            text_top + line_height*3  + /* Title + 3 sliders     */
            SWATCH_TOP_MARGIN         + /* at least 2 lines      */
            char_height*2             + /*  + margins for bottom */
            MARGIN_BOTTOM;              /* colored rectangle     */

    display->set_viewport(last_vp);

    if (y < text_top)
    {
        if (button == BUTTON_REL)
            return ACTION_STD_CANCEL;
        else
            return ACTION_NONE;
    }

    if (y >= text_top + line_height * (display_three_rows ? 3:1))
    {   /* touching the color area means accept */
        if (button == BUTTON_REL)
            return ACTION_STD_OK;
        else
            return ACTION_NONE;
    }
    /* y is relative to the original viewport */
    pressed_slider = (y - text_top)/line_height;
    if (pressed_slider != *selected_slider)
        *selected_slider = pressed_slider;
    /* add max_label_width to overcome integer division limits,
     * cap value later since that may lead to an overflow */
    if (x < slider_x + (slider_width+max_label_width) && x > slider_x)
    {
        char computed_val;
        x -= slider_x;
        computed_val = (x*rgb_max[pressed_slider]/(slider_width));
        rgb->rgb_val[pressed_slider] =
                        MIN(computed_val,rgb_max[pressed_slider]);
        pack_rgb(rgb);
    }
    return ACTION_NONE;
}
#endif
/***********
 set_color
 returns true if USB was inserted, false otherwise
 color is a pointer to the colour (in native format) to modify
 set banned_color to -1 to allow all
 ***********/
bool set_color(struct screen *display, char *title,
               unsigned *color, unsigned banned_color)
{
    int exit = 0, slider = 0;
    struct rgb_pick rgb;

    rgb.color = *color;

    while (!exit)
    {
        int button;

        unpack_rgb(&rgb);

        if (display != NULL)
        {
            draw_screen(display, title, &rgb, slider);
        }
        else
        {
            FOR_NB_SCREENS(i)
                draw_screen(&screens[i], title, &rgb, slider);
        }

        button = get_action(CONTEXT_SETTINGS_COLOURCHOOSER, TIMEOUT_BLOCK);
#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN
                && display->screen_type == SCREEN_MAIN)
            button = touchscreen_slider(display, &rgb, &slider);
#endif

        switch (button)
        {
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                slider = (slider + 2) % 3;
                break;

            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                slider = (slider + 1) % 3;
                break;

            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_INCREPEAT:
                if (rgb.rgb_val[slider] < rgb_max[slider])
                    rgb.rgb_val[slider]++;
                pack_rgb(&rgb);
                break;

            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                if (rgb.rgb_val[slider] > 0)
                    rgb.rgb_val[slider]--;
                pack_rgb(&rgb);
                break;

            case ACTION_STD_OK:
                if (banned_color != (unsigned)-1 &&
                    banned_color == rgb.color)
                {
                    splash(HZ*2, ID2P(LANG_COLOR_UNACCEPTABLE));
                    break;
                }
                *color = rgb.color;
                exit = 1;
                break;

            case ACTION_STD_CANCEL:
                exit = 1;
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    return false;
}
