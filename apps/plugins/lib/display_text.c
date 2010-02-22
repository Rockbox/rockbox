/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Johannes Schwarz
 * based on Will Robertson code in superdom
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
#include "display_text.h"

#ifdef HAVE_LCD_CHARCELLS
#define MARGIN 0
#else
#define MARGIN 5
#endif

static bool wait_key_press(void)
{
    int button;
    do {
        button = rb->button_get(true);
        if ( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
            return true;
    } while( ( button == BUTTON_NONE )
            || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
    return false;
}

bool display_text(unsigned short words, char** text, struct style_text* style,
                  struct viewport* vp_text, bool wait_key)
{
#ifdef HAVE_LCD_BITMAP
    int prev_drawmode;
#endif
#ifdef HAVE_LCD_COLOR
    int standard_fgcolor;
#endif
    int space_w, width, height;
    unsigned short x , y;
    unsigned short vp_width = LCD_WIDTH;
    unsigned short vp_height = LCD_HEIGHT;
    unsigned short i = 0, style_index = 0;
    if (vp_text != NULL) {
        vp_width = vp_text->width;
        vp_height = vp_text->height;
    }
    rb->screens[SCREEN_MAIN]->set_viewport(vp_text);
#ifdef HAVE_LCD_BITMAP
    prev_drawmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
#ifdef HAVE_LCD_COLOR
    standard_fgcolor = rb->lcd_get_foreground();
#endif
    rb->screens[SCREEN_MAIN]->clear_viewport();
    x = MARGIN;
    y = MARGIN;
    rb->button_clear_queue();
    rb->lcd_getstringsize(" ", &space_w, &height);
    for (i = 0; i < words; i++) {
        rb->lcd_getstringsize(text[i], &width, NULL);
        /* skip to next line if the word is an empty string */
        if (rb->strcmp(text[i], "")==0) {
            x = MARGIN;
            y = y + height;
            continue;
        /* .. or if the current one can't fit the word */
        } else if (x + width > vp_width - MARGIN) {
            x = MARGIN;
            y = y + height;
        }
        /* display the remaining text by button click or exit */
        if (y + height > vp_height - MARGIN) {
            y = MARGIN;
            rb->screens[SCREEN_MAIN]->update_viewport();
            if (wait_key_press())
                return true;
            rb->screens[SCREEN_MAIN]->clear_viewport();
        }
        /* no text formatting available */
        if (style==NULL || style[style_index].index != i) {
            rb->lcd_putsxy(x, y, text[i]);
        } else {
            /* set align */
            if (style[style_index].flags&TEXT_CENTER) {
                x = (vp_width-width)/2;
            }
            /* set font color */
#ifdef HAVE_LCD_COLOR
            switch (style[style_index].flags&TEXT_COLOR_MASK) {
                case C_RED:
                    rb->lcd_set_foreground(LCD_RGBPACK(255,0,0));
                    break;
                case C_YELLOW:
                    rb->lcd_set_foreground(LCD_RGBPACK(255,255,0));
                    break;
                case C_GREEN:
                    rb->lcd_set_foreground(LCD_RGBPACK(0,192,0));
                    break;
                case C_BLUE:
                    rb->lcd_set_foreground(LCD_RGBPACK(0,0,255));
                    break;
                case C_ORANGE:
                    rb->lcd_set_foreground(LCD_RGBPACK(255,192,0));
                    break;
                case STANDARD:
                default:
                    rb->lcd_set_foreground(standard_fgcolor);
                    break;
            }
#endif
            rb->lcd_putsxy(x, y, text[i]);
            /* underline the word */
#ifdef HAVE_LCD_BITMAP
            if (style[style_index].flags&TEXT_UNDERLINE) {
                rb->lcd_hline(x, x+width, y+height-1);
            }
#endif
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(standard_fgcolor);
#endif
            style_index++;
        }
        x += width + space_w;
    }
    rb->screens[SCREEN_MAIN]->update_viewport();
#ifdef HAVE_LCD_BITMAP
    rb->lcd_set_drawmode(prev_drawmode);
#endif
    if (wait_key)
    {
        if (wait_key_press())
            return true;
    }
    return false;
}
