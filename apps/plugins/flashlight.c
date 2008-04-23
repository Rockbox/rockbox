/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Vuong Minh Hiep (vmh)
 * Copyright (C) 2008 Thomas Martitz (kugel.)
 * Copyright (C) 2008 Alexander Papst
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "helper.h"

PLUGIN_HEADER

#if defined(HAVE_BACKLIGHT)
/* variable button definitions - only targets with a colour display */
#if defined(HAVE_LCD_COLOR)
#if (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT
#   define FLASHLIGHT_NEXT       BUTTON_SCROLL_FWD
#   define FLASHLIGHT_PREV       BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT
#   define FLASHLIGHT_NEXT       BUTTON_SCROLL_FWD
#   define FLASHLIGHT_PREV       BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT
#   define FLASHLIGHT_NEXT       BUTTON_SCROLL_UP
#   define FLASHLIGHT_PREV       BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == MROBE500_PAD
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT

#elif CONFIG_KEYPAD == COWOND2_PAD
#   define FLASHLIGHT_LEFT       BUTTON_LEFT
#   define FLASHLIGHT_RIGHT      BUTTON_RIGHT

#else
#   error Missing key definitions for this keypad
#endif
#endif

static struct plugin_api* rb; /* global api struct pointer */

#ifdef HAVE_LCD_COLOR
void hsv_to_rgb(float hue, float sat, float v, 
                float *red, float *green, float *blue)
{
    float r, g, b;

    if (hue > 0.17 && hue <= 0.33)    /* green/red */
    {
        g = 1.0;
        r = ((float)0.33 - hue) / (float)0.16;
        b = 0.0;
    }
    else if (hue > 0.33 && hue <= 0.5)    /* green/blue */
    {
        g = 1.0;
        b = (hue - (float)0.33) / (float)0.17;
        r = 0.0;
    }
    else if (hue > 0.5 && hue <= 0.67)    /* blue/green */
    {
        b = 1.0;
        g = ((float)0.67 - hue) / (float)0.17;
        r = 0.0;
    }
    else if (hue > 0.67 && hue <= 0.83)    /* blue/red */
    {
        b = 1.0;
        r = (hue - (float)0.67) / (float)0.16;
        g = 0.0;
    } 
    else if (hue > 0.83 && hue <= 1.0)    /* red/blue */
    {
        r = 1.0;
        b = ((float)1.0 - hue) / (float)0.17;
        g = 0.0;
    } 
    else                /* red/green */
    {
        r = 1.0;
        g = hue / (float)0.17;
        b = 0.0;
    }

    *red = (sat * r + ((float)1.0 - sat)) * v;
    *green = (sat * g + ((float)1.0 - sat)) * v;
    *blue = (sat * b + ((float)1.0 - sat)) * v;
}
#endif

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;
    rb = api;

#ifdef HAVE_LCD_COLOR
    int h = 0;
    float var_r, var_g, var_b;
    bool quit = false;
#endif /* HAVE_LCD_COLOR */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    short old_brightness = rb->global_settings->brightness;
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    unsigned bg_color=rb->lcd_get_background();
    rb->lcd_set_background(LCD_WHITE);
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    rb->backlight_set_brightness(MAX_BRIGHTNESS_SETTING);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

    backlight_force_on(rb);

#ifdef HAVE_LCD_COLOR
    do 
    {
        if (h != 0)
        {
            hsv_to_rgb( ((float)(h - 5) / 360.0), 1, 1, &var_r, &var_g, &var_b);
            rb->lcd_set_background( LCD_RGBPACK( ((int)(var_r * 255)),
                                                 ((int)(var_g * 255)),
                                                 ((int)(var_b * 255)) ) );
        }
        else
        { /* Whilte screen */
            rb->lcd_set_background(LCD_WHITE);
        }
        rb->lcd_clear_display();
        rb->lcd_update();

        switch(rb->button_get(true))
        {
            case FLASHLIGHT_RIGHT:
            case (FLASHLIGHT_RIGHT|BUTTON_REPEAT):
            case (FLASHLIGHT_RIGHT|BUTTON_REL):
#ifdef FLASHLIGHT_NEXT
            case FLASHLIGHT_NEXT:
            case (FLASHLIGHT_NEXT|BUTTON_REPEAT):
            case (FLASHLIGHT_NEXT|BUTTON_REL):
#endif /* FLASHLIGHT_NEXT */
                h = (h + 5) % 365;
                break;
            case FLASHLIGHT_LEFT:
            case (FLASHLIGHT_LEFT|BUTTON_REPEAT):
            case (FLASHLIGHT_LEFT|BUTTON_REL):
#ifdef FLASHLIGHT_PREV
            case FLASHLIGHT_PREV:
            case (FLASHLIGHT_PREV|BUTTON_REPEAT):
            case (FLASHLIGHT_PREV|BUTTON_REL):
#endif /* FLASHLIGHT_PREV */
                h = (h + 360) % 365;
                break;

            default:
                quit = true;
        }
    } while (!quit);

#else /* HAVE_LCD_COLOR */
    rb->lcd_clear_display();
    rb->lcd_update();
    /* wait */
    while(rb->button_get(false) == BUTTON_NONE)
    {
        rb->yield();
    }

#endif /*HAVE_LCD_COLOR */

    /* restore */
    backlight_use_settings(rb);

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    rb->backlight_set_brightness(old_brightness);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#if LCD_DEPTH > 1
    rb->lcd_set_background(bg_color);
#endif
    return PLUGIN_OK;
}
#endif
