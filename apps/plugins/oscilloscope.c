/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Oscilloscope, with the thought-to-be impossible horizontal aspect!
*
* Copyright (C) 2004 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#if CONFIG_CODEC != SWCODEC /* only for MAS-targets */
#include "xlcd.h"

PLUGIN_HEADER

/* The different drawing modes */
#define DRAW_MODE_FILLED  0
#define DRAW_MODE_OUTLINE 1
#define DRAW_MODE_PIXEL   2
#define DRAW_MODE_COUNT   3

#define MAX_PEAK 0x8000

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define OSCILLOSCOPE_QUIT BUTTON_OFF
#define OSCILLOSCOPE_SCROLL BUTTON_F1
#define OSCILLOSCOPE_MODE BUTTON_F2
#define OSCILLOSCOPE_PAUSE BUTTON_PLAY
#define OSCILLOSCOPE_VOL_UP BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define OSCILLOSCOPE_QUIT BUTTON_OFF
#define OSCILLOSCOPE_SCROLL BUTTON_RIGHT
#define OSCILLOSCOPE_MODE BUTTON_MENU
#define OSCILLOSCOPE_PAUSE BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN BUTTON_DOWN

#endif

/* unsigned 16 bit multiplication (a single instruction on the SH) */
#define MULU16(a, b) ((unsigned long) \
                     (((unsigned short) (a)) * ((unsigned short) (b))))

/* global variables */

struct plugin_api* rb; /* global api struct pointer */
int x = 0;
int draw_mode = DRAW_MODE_FILLED;
bool scroll = true;
int left_val;
int right_val;
bool new_val = false;

/* prototypes */

void lcd_scroll_left(int count, bool black_border);
void timer_isr(void);
void cleanup(void *parameter);
enum plugin_status plugin_start(struct plugin_api* api, void* parameter);

/* implementation */

void timer_isr(void)
{
    static int last_left, last_right;
    bool full_update = false;

    if (new_val)
    {   
        if ((unsigned)x >= LCD_WIDTH)
        {
            if (scroll)
            {
                xlcd_scroll_left(1);
                x = LCD_WIDTH-1;
                full_update = true;
            }
            else
                x = 0;
        }

        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_vline(x, 0, LCD_HEIGHT-1);
        rb->lcd_set_drawmode(DRMODE_SOLID);

        switch (draw_mode)
        {
            case DRAW_MODE_FILLED:
                rb->lcd_vline(x, LCD_HEIGHT/2+1, LCD_HEIGHT/2+1 + left_val);
                rb->lcd_vline(x, LCD_HEIGHT/2-1, LCD_HEIGHT/2-1 - right_val);
                break;
                
            case DRAW_MODE_OUTLINE:
                if (x > 0)
                {
                    rb->lcd_drawline(x - 1, LCD_HEIGHT/2+1 + last_left,
                                     x, LCD_HEIGHT/2+1 + left_val);
                    rb->lcd_drawline(x - 1, LCD_HEIGHT/2-1 - last_right,
                                     x, LCD_HEIGHT/2-1 - right_val);
                    break;
                }
                /* else fall through */
                
            case DRAW_MODE_PIXEL:
                rb->lcd_drawpixel(x, LCD_HEIGHT/2+1 + left_val);
                rb->lcd_drawpixel(x, LCD_HEIGHT/2-1 - right_val);
                break;
        }

        if (full_update)
            rb->lcd_update();
        else
            rb->lcd_update_rect(MAX(x - 1, 0), 0, 2, LCD_HEIGHT);
            
        last_left = left_val;
        last_right = right_val;
        x++;
        new_val = false;
    }
}

void cleanup(void *parameter)
{
    (void)parameter;

    rb->timer_unregister();
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button, vol;
    bool exit = false;
    bool paused = false;

    (void)parameter;
    rb = api;

    xlcd_init(rb);
    
    rb->timer_register(1, NULL, FREQ / 67, 1, timer_isr);

    while (!exit)
    {
        if (!paused)
        {
            /* read the volume info from MAS */
            left_val = rb->mas_codec_readreg(0xC) / (MAX_PEAK / (LCD_HEIGHT/2-2));
            right_val = rb->mas_codec_readreg(0xD) / (MAX_PEAK / (LCD_HEIGHT/2-2));
            new_val = true;
        }
        rb->yield();

        button = rb->button_get(paused);
        switch (button)
        {
            case OSCILLOSCOPE_QUIT:
                exit = true;
                break;

            case OSCILLOSCOPE_SCROLL:
                scroll = !scroll;
                break;

            case OSCILLOSCOPE_MODE:
                draw_mode++;
                draw_mode = draw_mode % DRAW_MODE_COUNT;
                break;

            case OSCILLOSCOPE_PAUSE:
                paused = !paused;
                break;
                
            case OSCILLOSCOPE_VOL_UP:
            case OSCILLOSCOPE_VOL_UP | BUTTON_REPEAT:
                vol = rb->global_settings->volume;
                if (vol < rb->sound_max(SOUND_VOLUME))
                {
                    vol++;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }
                break;

            case OSCILLOSCOPE_VOL_DOWN:
            case OSCILLOSCOPE_VOL_DOWN | BUTTON_REPEAT:
                vol = rb->global_settings->volume;
                if (vol > rb->sound_min(SOUND_VOLUME))
                {
                    vol--;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }
                break;

            default:
                if (rb->default_event_handler_ex(button, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }

    }
    
    cleanup(NULL);
    return PLUGIN_OK;
}
#endif /* if using MAS */
#endif /* if HAVE_LCD_BITMAP */
#endif /* SIMULATOR */
