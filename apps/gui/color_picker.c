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
#include "debug.h"
#include "misc.h"
#include "settings.h"
#include "scrollbar.h"
#include "lang.h"
#include "splash.h"
#include "action.h"

#define TEXT_MARGIN display->char_width+2
#define SLIDER_START 20

static const int max_val[3] = {LCD_MAX_RED,LCD_MAX_GREEN,LCD_MAX_BLUE};

static void draw_screen(struct screen *display, char *title,
                        int *rgb_val, int color, int row)
{
    int i;
    char *textrgb = str(LANG_COLOR_RGB_LABELS), rgb_dummy[2] = {0,0};
    int text_top;
    int text_centre, bg_col;
    char buf[32];
    int slider_width = (display->width-SLIDER_START-(display->char_width*5));
    int background_color = global_settings.bg_color;
    int text_color = global_settings.fg_color;
    bool display_three_rows = (display->height/6) > display->char_height;

    display->clear_display();

    if (display->depth > 1) {
        display->set_foreground(text_color);
    }

    i = display->getstringsize(title,0,0);
    display->putsxy((display->width-i)/2,6,title );

    text_top = display->char_height*2;
    bg_col = background_color;
    for (i=0; i<3 ;i++)
    {
        if (!display_three_rows)
            i = row;        

        if (i==row)
        {
            if ((global_settings.invert_cursor) && (display->depth >2))
            {
                display->fillrect(0,text_top-1,display->width,display->char_height+2);
                bg_col = text_color;
            }
            else if (display_three_rows)
            {
                display->putsxy(0,text_top,">");
                display->putsxy(display->width-display->char_width-2,text_top,"<");
                bg_col = background_color;
            }
            if (display->depth > 1)
            {
                if (i==0)
                    display->set_foreground(LCD_RGBPACK(255, 0, 0));
                else if (i==1)
                    display->set_foreground(LCD_RGBPACK(0, 255, 0));
                else if (i==2)
                    display->set_foreground(LCD_RGBPACK(0 ,0 ,255));
            }
        }
        else
        {
            if (display->depth > 1)
                display->set_foreground(text_color);
            bg_col = background_color;
        }

        if (display->depth > 1)
            display->set_background(bg_col);

        text_centre = text_top+(display->char_height/2);
        rgb_dummy[0] = textrgb[i];
        display->putsxy(TEXT_MARGIN,text_top,rgb_dummy);
        snprintf(buf,3,"%02d",rgb_val[i]);
        display->putsxy(display->width-(display->char_width*4),text_top,buf);

        text_top += display->char_height/4;

        gui_scrollbar_draw(display,SLIDER_START,text_top,slider_width,
                           display->char_height/2,
                           max_val[i],0,rgb_val[i],HORIZONTAL);
        if (!display_three_rows)
           break;
        text_top += display->char_height;
    }

    if (display->depth > 1) {
        display->set_background(background_color);
        display->set_foreground(text_color);
    }

    if (text_top + (display->char_height*2) < (display->height-40-display->char_height))
        text_top += (display->char_height*2);
    else text_top += (display->char_height);

    /* Display RGB: #rrggbb */
    snprintf(buf,sizeof(buf),str(LANG_COLOR_RGB_VALUE),
                                 RGB_UNPACK_RED(color),
                                 RGB_UNPACK_GREEN(color),
                                 RGB_UNPACK_BLUE(color));

    display->putsxy((display->width-(display->char_width*11))/2,text_top,buf);

    if (display->depth > 1) {
        display->set_foreground(color);
        display->fillrect(SLIDER_START,LCD_HEIGHT-40,slider_width,35);

        display->set_foreground(LCD_BLACK);
        display->drawrect(SLIDER_START-1,LCD_HEIGHT-41,slider_width+2,37);
    }

    display->update();
}

/***********
 set_color
 returns true if USB was inserted, false otherwise
 color is a pointer to the colour (in native format) to modify
 set banned_color to -1 to allow all
 ***********/
bool set_color(struct screen *display,char *title, int* color, int banned_color)
{
    int exit = 0, button, slider=0;
    int rgb_val[3]; /* native depth r,g,b*/;
    int fgcolor = display->get_foreground();
    int newcolor = *color;
    int i;

#if LCD_PIXELFORMAT == RGB565
    rgb_val[0] = ((*color)&0xf800) >> 11;
    rgb_val[1] = ((*color)&0x07e0) >> 5;
    rgb_val[2] = ((*color)&0x001f);
#elif LCD_PIXELFORMAT == RGB565SWAPPED
    rgb_val[0] = ((swap16(*color))&0xf800) >> 11;
    rgb_val[1] = ((swap16(*color))&0x07e0) >> 5;
    rgb_val[2] = ((swap16(*color))&0x001f);
#endif

    while (!exit)
    {
        /* We need to maintain three versions of the colour:

           rgb_val[3] - the native depth RGB values
           newcolor - the native format packed colour
         */

#if LCD_PIXELFORMAT == RGB565
        newcolor = (rgb_val[0] << 11) | (rgb_val[1] << 5) | (rgb_val[2]);
#elif LCD_PIXELFORMAT == RGB565SWAPPED
        newcolor = swap16((rgb_val[0] << 11) | (rgb_val[1] << 5) | (rgb_val[2]));
#endif
        FOR_NB_SCREENS(i)
        {
            draw_screen(&screens[i], title, rgb_val, newcolor, slider);
        }
        
        button = get_action(CONTEXT_SETTINGS_COLOURCHOOSER,TIMEOUT_BLOCK);
        switch (button)
        {
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                slider = (slider+2)%3;
                break;

            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                slider = (slider+1)%3;
                break;

            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_INCREPEAT:
                if (rgb_val[slider] < max_val[slider])
                    rgb_val[slider]++;
                break;

            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                if (rgb_val[slider] > 0)
                    rgb_val[slider]--;
                break;

            case ACTION_STD_OK:
                if ((banned_color!=-1) && (banned_color == newcolor))
                {
                    gui_syncsplash(HZ*2,true,str(LANG_COLOR_UNACCEPTABLE));
                    break;
                }
                *color = newcolor;
                exit = 1;
                break;

            case ACTION_STD_CANCEL:
                exit = 1;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED) {
                    display->set_foreground(fgcolor);
                    return true;
                }
                break;
        }
    }
    display->set_foreground(fgcolor);
    action_signalscreenchange();
    return false;
}
