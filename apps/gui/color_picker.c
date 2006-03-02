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

#define TEXT_MARGIN display->char_width+2
#define SLIDER_START 20

#if (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SLIDER_UP        BUTTON_UP
#define SLIDER_DOWN      BUTTON_DOWN
#define SLIDER_LEFT      BUTTON_LEFT
#define SLIDER_RIGHT     BUTTON_RIGHT
#define SLIDER_OK        BUTTON_ON
#define SLIDER_CANCEL    BUTTON_OFF

#define SLIDER_RC_UP     BUTTON_RC_REW
#define SLIDER_RC_DOWN   BUTTON_RC_FF
#define SLIDER_RC_LEFT   BUTTON_RC_SOURCE
#define SLIDER_RC_RIGHT  BUTTON_RC_BITRATE
#define SLIDER_RC_OK     BUTTON_RC_ON
#define SLIDER_RC_CANCEL BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define SLIDER_UP        BUTTON_UP
#define SLIDER_DOWN      BUTTON_DOWN
#define SLIDER_LEFT      BUTTON_LEFT
#define SLIDER_RIGHT     BUTTON_RIGHT
#define SLIDER_OK        BUTTON_POWER
#define SLIDER_CANCEL    BUTTON_A

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)
#define SLIDER_UP        BUTTON_LEFT
#define SLIDER_DOWN      BUTTON_RIGHT
#define SLIDER_LEFT      BUTTON_SCROLL_BACK
#define SLIDER_RIGHT     BUTTON_SCROLL_FWD
#define SLIDER_OK        BUTTON_SELECT
#define SLIDER_CANCEL    BUTTON_MENU

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define SLIDER_UP        BUTTON_UP
#define SLIDER_DOWN      BUTTON_DOWN
#define SLIDER_LEFT      BUTTON_LEFT
#define SLIDER_RIGHT     BUTTON_RIGHT
#define SLIDER_OK        BUTTON_SELECT
#define SLIDER_CANCEL    BUTTON_PLAY

#endif

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

    display->clear_display();

    if (display->depth > 1) {
        display->set_foreground(text_color);
    }

    i = display->getstringsize(title,0,0);
    display->putsxy((display->width-i)/2,6,title );
    
    for (i=0;i<3;i++)
    {
        text_top =display->char_height*((i*2)+2);
        
        if (i==row)
        {
            if (global_settings.invert_cursor)
            {
                display->fillrect(0,text_top-1,display->width,display->char_height+2);
                bg_col = text_color;
            }
            else 
            {
                display->putsxy(0,text_top,">");
                display->putsxy(display->width-TEXT_MARGIN,text_top,"<");
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
    }

    if (display->depth > 1) {
        display->set_background(background_color);
        display->set_foreground(text_color);
    }

    if (text_top + (display->char_height*2) < (LCD_HEIGHT-40-display->char_height))
        text_top += (display->char_height*2);
    else text_top += (display->char_height);

    /* Display RGB: #rrggbb */
    snprintf(buf,sizeof(buf),str(LANG_COLOR_RGB_VALUE),
                                 RGB_UNPACK_RED(color),
                                 RGB_UNPACK_GREEN(color),
                                 RGB_UNPACK_BLUE(color));

    display->putsxy((LCD_WIDTH-(display->char_width*21))/2,text_top,buf);

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
 ***********/
bool set_color(struct screen *display,char *title, int* color)
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
            draw_screen(&screens[i], title, rgb_val, newcolor, slider);

        button = button_get(true);
        switch (button) 
        {
            case SLIDER_UP:
#ifdef HAVE_LCD_REMOTE
            case SLIDER_RC_UP:
#endif
                slider = (slider+2)%3;
                break;

            case SLIDER_DOWN:
#ifdef HAVE_LCD_REMOTE
            case SLIDER_RC_DOWN:
#endif
                slider = (slider+1)%3;
                break;

            case SLIDER_RIGHT:
            case SLIDER_RIGHT|BUTTON_REPEAT:
#ifdef HAVE_LCD_REMOTE
            case SLIDER_RC_RIGHT:
            case SLIDER_RC_RIGHT|BUTTON_REPEAT:
#endif
                if (rgb_val[slider] < max_val[slider])
                    rgb_val[slider]++;
                break;

            case SLIDER_LEFT:
            case SLIDER_LEFT|BUTTON_REPEAT:
#ifdef HAVE_LCD_REMOTE
            case SLIDER_RC_LEFT:
            case SLIDER_RC_LEFT|BUTTON_REPEAT:
#endif
                if (rgb_val[slider] > 0)
                    rgb_val[slider]--;
                break;
            
            case SLIDER_OK:
#ifdef HAVE_LCD_REMOTE
            case SLIDER_RC_OK:
#endif
                *color = newcolor;
                exit = 1;
                break;

            case SLIDER_CANCEL:
#ifdef HAVE_LCD_REMOTE
            case SLIDER_RC_CANCEL:
#endif
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

    return false;
}
