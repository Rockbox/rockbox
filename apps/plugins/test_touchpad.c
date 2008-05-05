/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rob Purchase
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
#include "grey.h"

PLUGIN_HEADER

#if (CONFIG_KEYPAD == COWOND2_PAD)
#define TOUCHPAD_QUIT   BUTTON_POWER
#define TOUCHPAD_TOGGLE BUTTON_MENU
#elif (CONFIG_KEYPAD == MROBE500_PAD)
#define TOUCHPAD_QUIT   BUTTON_POWER
#define TOUCHPAD_TOGGLE BUTTON_RC_MODE
#endif

static struct plugin_api* rb;

/* plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button = 0;
    enum touchpad_mode mode = TOUCHPAD_BUTTON;

    /* standard stuff */
    (void)parameter;
    rb = api;
    
    rb->touchpad_set_mode(mode);

    /* wait until user closes plugin */
    do
    {
        short x = 0;
        short y = 0;
        bool draw_rect = false;
        
        button = rb->button_get(true);

        if (button & BUTTON_TOPLEFT)
        {
            draw_rect = true;
            x = 0; y = 0;
        }
        else if (button & BUTTON_TOPMIDDLE)
        {
            draw_rect = true;
            x = LCD_WIDTH/3; y = 0;
        }
        else if (button & BUTTON_TOPRIGHT)
        {
            draw_rect = true;
            x = 2*(LCD_WIDTH/3); y = 0;
        }
        else if (button & BUTTON_MIDLEFT)
        {
            draw_rect = true;
            x = 0; y = LCD_HEIGHT/3;
        }
        else if (button & BUTTON_CENTER)
        {
            draw_rect = true;
            x = LCD_WIDTH/3; y = LCD_HEIGHT/3;
        }
        else if (button & BUTTON_MIDRIGHT)
        {
            draw_rect = true;
            x = 2*(LCD_WIDTH/3); y = LCD_HEIGHT/3;
        }
        else if (button & BUTTON_BOTTOMLEFT)
        {
            draw_rect = true;
            x = 0; y = 2*(LCD_HEIGHT/3);
        }
        else if (button & BUTTON_BOTTOMMIDDLE)
        {
            draw_rect = true;
            x = LCD_WIDTH/3; y = 2*(LCD_HEIGHT/3);
        }
        else if (button & BUTTON_BOTTOMRIGHT)
        {
            draw_rect = true;
            x = 2*(LCD_WIDTH/3); y = 2*(LCD_HEIGHT/3);
        }

        if (button & TOUCHPAD_TOGGLE && (button & BUTTON_REL))
        {
            mode = (mode == TOUCHPAD_POINT) ? TOUCHPAD_BUTTON : TOUCHPAD_POINT;
            rb->touchpad_set_mode(mode);
        }
        
        if (button & BUTTON_REL) draw_rect = false;

        rb->lcd_clear_display();

        if (draw_rect)
        {
            rb->lcd_set_foreground(LCD_RGBPACK(0xc0, 0, 0));
            rb->lcd_fillrect(x, y, LCD_WIDTH/3, LCD_HEIGHT/3);
        }

        if (draw_rect || button & BUTTON_TOUCHPAD)
        {
            intptr_t button_data = rb->button_get_data();
            x = button_data >> 16;
            y = button_data & 0xffff;

            rb->lcd_set_foreground(LCD_RGBPACK(0, 0, 0xc0));
            rb->lcd_fillrect(x-7, y-7, 14, 14);
            
            /* in stylus mode, show REL position in black */
            if (mode == TOUCHPAD_POINT && (button & BUTTON_REL))
                rb->lcd_set_foreground(LCD_BLACK);
            else
                rb->lcd_set_foreground(LCD_WHITE);

            rb->lcd_hline(x-5, x+5, y);
            rb->lcd_vline(x, y-5, y+5);
        }
        rb->lcd_update();

    } while (button != TOUCHPAD_QUIT);

    return PLUGIN_OK;
}
