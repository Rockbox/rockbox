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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

/* maximum values for components */
static const unsigned char max_val[3] =
{
    LCD_MAX_RED,
    LCD_MAX_GREEN,
    LCD_MAX_BLUE
};

/* list of primary colors */
static const unsigned prim_rgb[3] =
{
    LCD_RGBPACK(255, 0, 0),
    LCD_RGBPACK(0, 255, 0),
    LCD_RGBPACK(0, 0 ,255),
};

/* Unpacks the color value into native rgb values and 24 bit rgb values */
static void unpack_rgb(struct rgb_pick *rgb)
{
    unsigned color = rgb->color;
#if LCD_PIXELFORMAT == RGB565SWAPPED
    color = swap16(color);
#endif
    rgb->red   = _RGB_UNPACK_RED(color);
    rgb->green = _RGB_UNPACK_GREEN(color);
    rgb->blue  = _RGB_UNPACK_BLUE(color);
    rgb->r     = (color & 0xf800) >> 11;
    rgb->g     = (color & 0x07e0) >> 5;
    rgb->b     = (color & 0x001f);
}

/* Packs the native rgb colors into a color value */
static void pack_rgb(struct rgb_pick *rgb)
{
    unsigned color = (rgb->r & 0x1f) << 11 |
                     (rgb->g & 0x3f) << 5 |
                     (rgb->b & 0x1f);
#if LCD_PIXELFORMAT == RGB565SWAPPED
    color = swap16(color);
#endif
    rgb->color = color;
}

/* Returns LCD_BLACK if the color is above a threshold brightness
   else return LCD_WHITE */
static inline unsigned get_black_or_white(const struct rgb_pick *rgb)
{
    return (4*rgb->r + 5*rgb->g + 2*rgb->b) >= 256 ?
        LCD_BLACK : LCD_WHITE;
}

#define MARGIN_LEFT             2 /* Left margin of screen                */
#define MARGIN_TOP              4 /* Top margin of screen                 */
#define MARGIN_RIGHT            2 /* Right margin of screen               */
#define MARGIN_BOTTOM           4 /* Bottom margin of screen              */
#define SLIDER_MARGIN_LEFT      2 /* Gap to left of sliders               */
#define SLIDER_MARGIN_RIGHT     2 /* Gap to right of sliders              */
#define TITLE_MARGIN_BOTTOM     4 /* Space below title bar                */
#define SELECTOR_LR_MARGIN      1 /* Margin between ">" and text          */
#define SELECTOR_TB_MARGIN      1 /* Margin on top and bottom of selector */
#define SWATCH_TOP_MARGIN       4 /* Space between last slider and swatch */

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

    /* Find out if there's enought room for three slider or just
       enought to display the selected slider */
    display_three_rows = display->height >= display->char_height*4 +
                                            MARGIN_TOP +
                                            TITLE_MARGIN_BOTTOM +
                                            MARGIN_BOTTOM;

    /* Figure out widest label character in case they vary */
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
    display->putsxy((display->width - x) / 2, MARGIN_TOP, title);

    /* Get slider positions and top starting position */
    text_top     = MARGIN_TOP + y + TITLE_MARGIN_BOTTOM + SELECTOR_TB_MARGIN;
    slider_left  = MARGIN_LEFT + SELECTOR_LR_MARGIN + display->char_width +
                   max_label_width + SLIDER_MARGIN_LEFT;
    slider_width = display->width - slider_left - SLIDER_MARGIN_RIGHT -
                   SELECTOR_LR_MARGIN - display->char_width*3 - MARGIN_RIGHT;

    for (i = 0; i < 3; i++)
    {
        int      mode = DRMODE_SOLID;
        unsigned fg   = text_color;
        unsigned bg   = background_color;

        if (!display_three_rows)
            i = row;        

        if (i == row)
        {
            set_drawinfo(display, DRMODE_SOLID, text_color,
                         background_color);

            if (global_settings.invert_cursor)
            {
                /* Draw solid bar selection bar */
                display->fillrect(0,
                                  text_top - SELECTOR_TB_MARGIN,
                                  display->width,
                                  display->char_height +
                                  SELECTOR_TB_MARGIN*2);

                if (display->depth == 1)
                {
                    /* Just invert for low mono display */
                    mode |= DRMODE_INVERSEVID;
                }
                else
                {
                    if (display->depth >= 16)
                    {
                        /* Backdrops will show through text in
                           DRMODE_SOLID */
                        mode = DRMODE_FG;
                        fg = prim_rgb[i];
                    }
                    else
                    {
                        fg = background_color;
                    }

                    bg = text_color;
                }
            }
            else if (display_three_rows)
            {
                /* Draw ">    <" around sliders */
                display->putsxy(MARGIN_LEFT,  text_top, ">");
                display->putsxy(display->width-display->char_width -
                                MARGIN_RIGHT, text_top, "<");
                if (display->depth >= 16)
                    fg = prim_rgb[i];
            }
        }
 
        set_drawinfo(display, mode, fg, bg);

        /* Draw label - assumes labels are one character */
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
                           max_val[i],
                           0,
                           rgb->rgb_val[i],
                           HORIZONTAL);

        /* Advance to next line */
        text_top += display->char_height + 2*SELECTOR_TB_MARGIN;

        if (!display_three_rows)
            break;
    }

    /* Format RGB: #rrggbb */
    snprintf(buf, sizeof(buf), str(LANG_COLOR_RGB_VALUE),
                               rgb->red, rgb->green, rgb->blue);

    if (display->depth >= 16)
    {
        /* Display color swatch on color screens only */
        int left   = slider_left;
        int top    = text_top + SWATCH_TOP_MARGIN;
        int width  = display->width - slider_left - left;
        int height = display->height - top - MARGIN_BOTTOM;

        display->setfont(FONT_SYSFIXED);

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
            display->drawrect(left - 1, top - 1, width + 2, height + 2);
        }

        display->setfont(FONT_UI);
    }
    else
    {
        /* Display RGB value only centered on remaining display if room */
        display->getstringsize(buf, &x, &y);
        i = text_top + SWATCH_TOP_MARGIN;

        if (i + y <= display->height - MARGIN_BOTTOM)
        {
            set_drawinfo(display, DRMODE_SOLID, text_color, background_color);
            x = (display->width - x) / 2;
            y = (i + display->height - MARGIN_BOTTOM - y) / 2;
            display->putsxy(x, y, buf);
        }
    }

    display->update();
    /* Be sure screen mode is reset */
    set_drawinfo(display, DRMODE_SOLID, text_color, background_color);
}

/***********
 set_color
 returns true if USB was inserted, false otherwise
 color is a pointer to the colour (in native format) to modify
 set banned_color to -1 to allow all
 ***********/
bool set_color(struct screen *display, char *title, int* color, int banned_color)
{
    int exit = 0, button, slider = 0;
    int i;
    struct rgb_pick rgb;
    (void)display;

    rgb.color = *color;

    while (!exit)
    {
        unpack_rgb(&rgb);

        FOR_NB_SCREENS(i)
        {
            draw_screen(&screens[i], title, &rgb, slider);
        }

        button = get_action(CONTEXT_SETTINGS_COLOURCHOOSER, TIMEOUT_BLOCK);

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
                if (rgb.rgb_val[slider] < max_val[slider])
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
                if (banned_color != -1 && (unsigned)banned_color == rgb.color)
                {
                    gui_syncsplash(HZ*2, true, str(LANG_COLOR_UNACCEPTABLE));
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

    action_signalscreenchange();
    return false;
}
