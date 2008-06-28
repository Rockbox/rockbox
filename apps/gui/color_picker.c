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
static const fb_data prim_rgb[][3] =
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
    unsigned color = _LCD_UNSWAP_COLOR(rgb->color);
    rgb->red   = _RGB_UNPACK_RED(color);
    rgb->green = _RGB_UNPACK_GREEN(color);
    rgb->blue  = _RGB_UNPACK_BLUE(color);
    rgb->r     = _RGB_UNPACK_RED_LCD(color);
    rgb->g     = _RGB_UNPACK_GREEN_LCD(color);
    rgb->b     = _RGB_UNPACK_BLUE_LCD(color);
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

#define MARGIN_LEFT             0 /* Left margin of screen                */
#define MARGIN_TOP              2 /* Top margin of screen                 */
#define MARGIN_RIGHT            0 /* Right margin of screen               */
#define MARGIN_BOTTOM           6 /* Bottom margin of screen              */
#define SLIDER_MARGIN_LEFT      2 /* Gap to left of sliders               */
#define SLIDER_MARGIN_RIGHT     2 /* Gap to right of sliders              */
#define TITLE_MARGIN_BOTTOM     4 /* Space below title bar                */
#define SELECTOR_LR_MARGIN      0 /* Margin between ">" and text          */
#define SELECTOR_TB_MARGIN      1 /* Margin on top and bottom of selector */
#define SWATCH_TOP_MARGIN       4 /* Space between last slider and swatch */
#define SELECTOR_WIDTH          get_icon_width(display->screen_type)
#define SELECTOR_HEIGHT         8 /* Height of > and < bitmaps            */

/* dunno why lcd_set_drawinfo should be left out of struct screen */
static void set_drawinfo(struct screen *display, int mode,
                         unsigned foreground, unsigned background)
{
    display->set_drawmode(mode);
    if (display->depth > 1)
    {
        display->set_foreground(foreground);
        display->set_background(background);
    }
}

static void draw_screen(struct screen *display, char *title,
                        struct rgb_pick *rgb, int row)
{
    unsigned  text_color       = LCD_BLACK;
    unsigned  background_color = LCD_WHITE;
    char      buf[32];
    int       i, x, y;
    int       text_top;
    int       slider_left, slider_width;
    bool      display_three_rows;
    int       max_label_width;

    display->clear_display();

    if (display->depth > 1)
    {
        text_color       = display->get_foreground();
        background_color = display->get_background();
    }

    /* Find out if there's enough room for three sliders or just
       enough to display the selected slider - calculate total height
       of display with three sliders present */
    display_three_rows =
        display->getheight() >= MARGIN_TOP             +
                                display->char_height*4 + /* Title + 3 sliders */
                                TITLE_MARGIN_BOTTOM    +
                                SELECTOR_TB_MARGIN*6   + /* 2 margins/slider  */
                                MARGIN_BOTTOM;

    /* Figure out widest label character in case they vary -
       this function assumes labels are one character */
    for (i = 0, max_label_width = 0; i < 3; i++)
    {
        buf[0] = str(LANG_COLOR_RGB_LABELS)[i];
        buf[1] = '\0';
        x = display->getstringsize(buf, NULL, NULL);
        if (x > max_label_width)
            max_label_width = x;
    }

    /* Draw title string */
    set_drawinfo(display, DRMODE_SOLID, text_color, background_color);
    display->getstringsize(title, &x, &y);
    display->putsxy((display->getwidth() - x) / 2, MARGIN_TOP, title);

    /* Get slider positions and top starting position */
    text_top     = MARGIN_TOP + y + TITLE_MARGIN_BOTTOM + SELECTOR_TB_MARGIN;
    slider_left  = MARGIN_LEFT + SELECTOR_WIDTH + SELECTOR_LR_MARGIN +
                   max_label_width + SLIDER_MARGIN_LEFT;
    slider_width = display->getwidth() - slider_left - SLIDER_MARGIN_RIGHT -
                   display->char_width*2 - SELECTOR_LR_MARGIN - SELECTOR_WIDTH -
                   MARGIN_RIGHT;

    for (i = 0; i < 3; i++)
    {
        unsigned sb_flags = HORIZONTAL;
        int      mode     = DRMODE_SOLID;
        unsigned fg       = text_color;
        unsigned bg       = background_color;

        if (!display_three_rows)
            i = row;

        if (i == row)
        {
            set_drawinfo(display, DRMODE_SOLID, text_color,
                         background_color);

            if (global_settings.cursor_style != 0)
            {
                /* Draw solid bar selection bar */
                display->fillrect(0,
                                  text_top - SELECTOR_TB_MARGIN,
                                  display->getwidth(),
                                  display->char_height +
                                  SELECTOR_TB_MARGIN*2);

                if (display->depth < 16)
                {
                    sb_flags |= FOREGROUND | INNER_FILL;
                    mode     |= DRMODE_INVERSEVID;
                }
            }
            else if (display_three_rows)
            {
                /* Draw ">    <" around sliders */
                int top = text_top + (display->char_height -
                                      SELECTOR_HEIGHT) / 2;
                screen_put_iconxy(display, MARGIN_LEFT, top, Icon_Cursor);
                screen_put_iconxy(display, 
                                  display->getwidth() - MARGIN_RIGHT -
                                          get_icon_width(display->screen_type),
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

        set_drawinfo(display, mode, fg, bg);

        /* Draw label */
        buf[0] = str(LANG_COLOR_RGB_LABELS)[i];
        buf[1] = '\0';
        display->putsxy(slider_left - display->char_width -
                        SLIDER_MARGIN_LEFT, text_top, buf);

        /* Draw color value */
        snprintf(buf, 3, "%02d", rgb->rgb_val[i]);
        display->putsxy(slider_left + slider_width + SLIDER_MARGIN_RIGHT,
                        text_top, buf);

        /* Draw scrollbar */
        gui_scrollbar_draw(display,
                           slider_left,
                           text_top + display->char_height / 4,
                           slider_width,
                           display->char_height / 2,
                           rgb_max[i],
                           0,
                           rgb->rgb_val[i],
                           sb_flags);

        /* Advance to next line */
        text_top += display->char_height + 2*SELECTOR_TB_MARGIN;

        if (!display_three_rows)
            break;
    } /* end for */

    /* Draw color value in system font */
    display->setfont(FONT_SYSFIXED);

    /* Format RGB: #rrggbb */
    snprintf(buf, sizeof(buf), str(LANG_COLOR_RGB_VALUE),
                               rgb->red, rgb->green, rgb->blue);

    if (display->depth >= 16)
    {
        /* Display color swatch on color screens only */
        int left   = MARGIN_LEFT + SELECTOR_WIDTH + SELECTOR_LR_MARGIN;
        int top    = text_top + SWATCH_TOP_MARGIN;
        int width  = display->getwidth() - left - SELECTOR_LR_MARGIN -
                     SELECTOR_WIDTH - MARGIN_RIGHT;
        int height = display->getheight() - top - MARGIN_BOTTOM;

        /* Only draw if room */
        if (height >= display->char_height + 2)
        {
            display->set_foreground(rgb->color);
            display->fillrect(left, top, width, height);

            /* Draw RGB: #rrggbb in middle of swatch */
            display->set_drawmode(DRMODE_FG);
            display->getstringsize(buf, &x, &y);
            display->set_foreground(get_black_or_white(rgb));

            x = left + (width  - x) / 2;
            y = top  + (height - y) / 2;

            display->putsxy(x, y, buf);
            display->set_drawmode(DRMODE_SOLID);

            /* Draw border */
            display->set_foreground(text_color);
            display->drawrect(left, top, width, height);
        }
    }
    else
    {
        /* Display RGB value only centered on remaining display if room */
        display->getstringsize(buf, &x, &y);
        i = text_top + SWATCH_TOP_MARGIN;

        if (i + y <= display->getheight() - MARGIN_BOTTOM)
        {
            set_drawinfo(display, DRMODE_SOLID, text_color, background_color);
            x = (display->getwidth() - x) / 2;
            y = (i + display->getheight() - MARGIN_BOTTOM - y) / 2;
            display->putsxy(x, y, buf);
        }
    }

    display->setfont(FONT_UI);

    display->update();
    /* Be sure screen mode is reset */
    set_drawinfo(display, DRMODE_SOLID, text_color, background_color);
}

#ifdef HAVE_TOUCHPAD
static int touchpad_slider(struct rgb_pick *rgb, int *selected_slider)
{
    short     x,y;
    int       text_top,i,x1;
    int       slider_left, slider_width;
    unsigned  button = action_get_touchpad_press(&x, &y);
    bool      display_three_rows;
    int       max_label_width;
    struct screen *display = &screens[SCREEN_MAIN];
    int      pressed_slider;
    char buf[2];
    
    if (button == BUTTON_NONE)
        return ACTION_NONE;
    /* same logic as draw_screen */
    /* Figure out widest label character in case they vary -
       this function assumes labels are one character */
    for (i = 0, max_label_width = 0; i < 3; i++)
    {
        buf[0] = str(LANG_COLOR_RGB_LABELS)[i];
        buf[1] = '\0';
        x1 = display->getstringsize(buf, NULL, NULL);
        if (x1 > max_label_width)
            max_label_width = x1;
    }
    /* Get slider positions and top starting position */
    text_top     = MARGIN_TOP + display->char_height + TITLE_MARGIN_BOTTOM + SELECTOR_TB_MARGIN;
    slider_left  = MARGIN_LEFT + SELECTOR_WIDTH + SELECTOR_LR_MARGIN +
                   max_label_width + SLIDER_MARGIN_LEFT;
    slider_width = display->getwidth() - slider_left - SLIDER_MARGIN_RIGHT -
                   display->char_width*2 - SELECTOR_LR_MARGIN - SELECTOR_WIDTH -
                   MARGIN_RIGHT;
    display_three_rows =
        display->getheight() >= MARGIN_TOP        +
                           display->char_height*4 + /* Title + 3 sliders */
                           TITLE_MARGIN_BOTTOM    +
                           SELECTOR_TB_MARGIN*6   + /* 2 margins/slider  */
                           MARGIN_BOTTOM;
    if (y < MARGIN_TOP+display->char_height)
    {
        if (button == BUTTON_REL)
            return ACTION_STD_CANCEL;
    }
    y -= text_top;
    pressed_slider = y/display->char_height;
    if (pressed_slider > (display_three_rows?2:0))
    {
        if (button == BUTTON_REL)
            return ACTION_STD_OK;
    }
    if (pressed_slider != *selected_slider)
        *selected_slider = pressed_slider;
    if (x < slider_left+slider_width && 
        x > slider_left)
    {
        x -= slider_left;
        rgb->rgb_val[pressed_slider] = 
            (x*rgb_max[pressed_slider]/(slider_width-slider_left));
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
bool set_color(struct screen *display, char *title, unsigned *color,
               unsigned banned_color)
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
            int i;
            FOR_NB_SCREENS(i)
                draw_screen(&screens[i], title, &rgb, slider);
        }

        button = get_action(CONTEXT_SETTINGS_COLOURCHOOSER, TIMEOUT_BLOCK);
#ifdef HAVE_TOUCHPAD
        if (button == ACTION_TOUCHPAD)
            button = touchpad_slider(&rgb, &slider);
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
                    gui_syncsplash(HZ*2, ID2P(LANG_COLOR_UNACCEPTABLE));
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
