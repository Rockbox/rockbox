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
/* RGB color sets */
#define NUM_COLORSETS   2
static int colorset[NUM_COLORSETS][3] = { { 255, 255, 255 } ,    /* white */
                                          { 255,   0,   0 } };   /* red */
#endif /* HAVE_LCD_COLOR */

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;
    rb = api;

#ifdef HAVE_LCD_COLOR
    int cs = 0;
    bool quit = false;
#endif /* HAVE_LCD_COLOR */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    short old_brightness = rb->global_settings->brightness;
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#if LCD_DEPTH > 1
    unsigned bg_color=rb->lcd_get_background();
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_background(LCD_WHITE);
#endif

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    rb->backlight_set_brightness(MAX_BRIGHTNESS_SETTING);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_LCD_INVERT
#ifdef OLYMPUS_MROBE_100
    /* mrobe-100 has inverted display so invert it for max brightness */
    rb->lcd_set_invert_display(true);
#else
    rb->lcd_set_invert_display(false);
#endif /* OLYMPUS_MROBE_100 */
#endif /* HAVE_LCD_INVERT */

    backlight_force_on(rb);

#ifdef HAVE_LCD_COLOR
    do 
    {
        if((cs < 0) || (cs >= NUM_COLORSETS))
            cs = 0;
        rb->lcd_set_background( LCD_RGBPACK( colorset[cs][0],
                                colorset[cs][1],
                                colorset[cs][2] ) );
        rb->lcd_clear_display();
        rb->lcd_update();

        switch(rb->button_get(true))
        {
            case FLASHLIGHT_RIGHT:
#ifdef FLASHLIGHT_NEXT
            case FLASHLIGHT_NEXT:
#endif /* FLASHLIGHT_NEXT */
                cs++;
                break;

            case FLASHLIGHT_LEFT:
#ifdef FLASHLIGHT_PREV
            case FLASHLIGHT_PREV:
#endif /* FLASHLIGHT_PREV */
                cs--;
                break;

            case (FLASHLIGHT_RIGHT|BUTTON_REPEAT):
            case (FLASHLIGHT_RIGHT|BUTTON_REL):
            case (FLASHLIGHT_LEFT|BUTTON_REPEAT):
            case (FLASHLIGHT_LEFT|BUTTON_REL):
#ifdef FLASHLIGHT_NEXT
            case (FLASHLIGHT_NEXT|BUTTON_REPEAT):
            case (FLASHLIGHT_NEXT|BUTTON_REL):
#endif /* FLASHLIGHT_NEXT */
#ifdef FLASHLIGHT_PREV
            case FLASHLIGHT_PREV:
            case (FLASHLIGHT_PREV|BUTTON_REPEAT):
            case (FLASHLIGHT_PREV|BUTTON_REL):
#endif /* FLASHLIGHT_PREV */
                /* eat these... */
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

#ifdef HAVE_LCD_INVERT
    rb->lcd_set_invert_display(rb->global_settings->invert);
#endif /* HAVE_LCD_INVERT */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    rb->backlight_set_brightness(old_brightness);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#if LCD_DEPTH > 1
    rb->lcd_set_background(bg_color);
#endif
    return PLUGIN_OK;
}
#endif
