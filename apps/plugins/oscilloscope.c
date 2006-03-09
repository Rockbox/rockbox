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
* Copyright (C) 2004-2006 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "xlcd.h"

PLUGIN_HEADER

/* The different drawing modes */
#define OSC_MODE_FILLED  0
#define OSC_MODE_OUTLINE 1
#define OSC_MODE_PIXEL   2
#define OSC_MODE_COUNT   3

#define MAX_PEAK 0x8000

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define OSCILLOSCOPE_QUIT BUTTON_OFF
#define OSCILLOSCOPE_MODE BUTTON_F1
#define OSCILLOSCOPE_SCROLL BUTTON_F2
#define OSCILLOSCOPE_PAUSE BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define OSCILLOSCOPE_QUIT BUTTON_OFF
#define OSCILLOSCOPE_MODE_PRE BUTTON_MENU
#define OSCILLOSCOPE_MODE (BUTTON_MENU | BUTTON_REL)
#define OSCILLOSCOPE_SCROLL (BUTTON_MENU | BUTTON_RIGHT)
#define OSCILLOSCOPE_PAUSE (BUTTON_MENU | BUTTON_OFF)
#define OSCILLOSCOPE_SPEED_UP BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define OSCILLOSCOPE_QUIT BUTTON_OFF
#define OSCILLOSCOPE_MODE BUTTON_SELECT
#define OSCILLOSCOPE_SCROLL BUTTON_MODE
#define OSCILLOSCOPE_PAUSE BUTTON_ON
#define OSCILLOSCOPE_SPEED_UP BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define OSCILLOSCOPE_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define OSCILLOSCOPE_MODE (BUTTON_SELECT | BUTTON_PLAY)
#define OSCILLOSCOPE_SCROLL (BUTTON_SELECT | BUTTON_RIGHT)
#define OSCILLOSCOPE_PAUSE BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP BUTTON_SCROLL_FWD
#define OSCILLOSCOPE_VOL_DOWN BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define OSCILLOSCOPE_QUIT BUTTON_POWER
#define OSCILLOSCOPE_MODE BUTTON_SELECT
#define OSCILLOSCOPE_SCROLL BUTTON_MENU
#define OSCILLOSCOPE_PAUSE BUTTON_A
#define OSCILLOSCOPE_SPEED_UP BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define OSCILLOSCOPE_QUIT BUTTON_POWER
#define OSCILLOSCOPE_MODE BUTTON_SELECT
#define OSCILLOSCOPE_SCROLL BUTTON_REC
#define OSCILLOSCOPE_PAUSE BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN BUTTON_DOWN

#endif

#if defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
#define mas_codec_readreg(x) rand()%MAX_PEAK
#endif

/* global variables */

struct plugin_api* rb;  /* global api struct pointer */

int  osc_mode = OSC_MODE_FILLED;
int  osc_delay = 1;     /* in ticks */
bool osc_scroll = true;
long last_tick = 0;

/* implementation */

void animate(int cur_left, int cur_right)
{
    static int last_x = 0;
    static int last_left, last_right;

    int cur_x, x;
    int left, right, dl, dr;
    long cur_tick = *rb->current_tick;
    long d = (cur_tick - last_tick) / osc_delay;
    bool full_update = false;

    if (d == 0)  /* too early, bail out */
        return;

    last_tick = cur_tick;

    if (d > HZ) /* first call or too much delay, (re)start */
    {
        last_left = cur_left;
        last_right = cur_right;
        return;
    }
    cur_x = last_x + d;

    if (cur_x >= LCD_WIDTH)
    {
        if (osc_scroll)  /* scroll */
        {
            int shift = cur_x - (LCD_WIDTH-1);
            xlcd_scroll_left(shift);
            full_update = true;
            cur_x -= shift;
            last_x -= shift;
        }
        else             /* wrap */
        {
            cur_x -= LCD_WIDTH;
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    if (cur_x > last_x)
    {
        rb->lcd_fillrect(last_x + 1, 0, d + 2, LCD_HEIGHT);
    }
    else
    {
        rb->lcd_fillrect(last_x + 1, 0, (LCD_WIDTH-1) - last_x, LCD_HEIGHT);
        rb->lcd_fillrect(0, 0, cur_x + 2, LCD_HEIGHT);
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);

    switch (osc_mode)
    {
        case OSC_MODE_FILLED:
            left = last_left;
            right = last_right;
            dl = (cur_left  - left)  / d;
            dr = (cur_right - right) / d;

            for (x = last_x + 1; d > 0; x++, d--)
            {
                if (x == LCD_WIDTH)
                    x = 0;

                left  += dl;
                right += dr;

                rb->lcd_vline(x, LCD_HEIGHT/2+1,
                              LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * left) >> 16));
                rb->lcd_vline(x, LCD_HEIGHT/2-1,
                              LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * right) >> 16));
            }
            break;

        case OSC_MODE_OUTLINE:
            if (cur_x > last_x)
            {
                rb->lcd_drawline(
                    last_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * last_left) >> 16),
                    cur_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * cur_left) >> 16)
                );
                rb->lcd_drawline(
                    last_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * last_right) >> 16),
                    cur_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * cur_right) >> 16)
                );
            }
            else
            {
                left  = last_left 
                      + (LCD_WIDTH - last_x) * (last_left - cur_left) / d;
                right = last_right 
                      + (LCD_WIDTH - last_x) * (last_right - cur_right) / d;

                rb->lcd_drawline(
                    last_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * last_left) >> 16),
                    LCD_WIDTH, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * left) >> 16)
                );
                rb->lcd_drawline(
                    last_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * last_right) >> 16),
                    LCD_WIDTH, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * right) >> 16)
                );
                rb->lcd_drawline(
                    0, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * left) >> 16),
                    cur_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * cur_left) >> 16)
                );
                rb->lcd_drawline(
                    0, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * right) >> 16),
                    cur_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * cur_right) >> 16)
                );
            }
            break;
            
        case OSC_MODE_PIXEL:
            left = last_left;
            right = last_right;
            dl = (cur_left - left) / d;
            dr = (cur_right - right) / d;

            for (x = last_x + 1; d > 0; x++, d--)
            {
                if (x == LCD_WIDTH)
                    x = 0;

                left  += dl;
                right += dr;

                rb->lcd_drawpixel(x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * left) >> 16));
                rb->lcd_drawpixel(x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * right) >> 16));
            }
            break;

    }
    last_left  = cur_left;
    last_right = cur_right;

    if (full_update)
    {
        rb->lcd_update();
    }
    else
    {
        if (cur_x > last_x)
        {
            rb->lcd_update_rect(last_x, 0, cur_x - last_x + 2, LCD_HEIGHT);
        }
        else
        {
            rb->lcd_update_rect(last_x, 0, (LCD_WIDTH-1) - last_x, LCD_HEIGHT);
            rb->lcd_update_rect(0, 0, cur_x + 2, LCD_HEIGHT);
        }
    }
    last_x = cur_x;
}

void cleanup(void *parameter)
{
    (void)parameter;
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
    rb->lcd_set_background(LCD_DEFAULT_BG);
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#endif
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button, vol;
    int lastbutton = BUTTON_NONE;
    bool exit = false;
    bool paused = false;

    (void)parameter;
    rb = api;

    xlcd_init(rb);
    
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(128, 255, 0));
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_update();
    rb->backlight_set_timeout(1); /* keep the light on */
#endif
    
    while (!exit)
    {
        if (!paused)
        {
            int left, right;

            rb->sleep(MAX(last_tick + osc_delay - *rb->current_tick - 1, 0));

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
            left = rb->mas_codec_readreg(0xC);
            right = rb->mas_codec_readreg(0xD);
#elif (CONFIG_CODEC == SWCODEC)
            rb->pcm_calculate_peaks(&left, &right);
#endif
            animate(left, right);
        }

        button = rb->button_get(paused);
        switch (button)
        {
            case OSCILLOSCOPE_QUIT:
                exit = true;
                break;

            case OSCILLOSCOPE_SCROLL:
                osc_scroll = !osc_scroll;
                break;

            case OSCILLOSCOPE_MODE:
#ifdef OSCILLOSCOPE_MODE_PRE
                if (lastbutton != OSCILLOSCOPE_MODE_PRE)
                    break;
#endif
                if (++osc_mode >= OSC_MODE_COUNT)
                    osc_mode = 0;
                break;

            case OSCILLOSCOPE_PAUSE:
                paused = !paused;
                last_tick = 0;
                break;
                
            case OSCILLOSCOPE_SPEED_UP:
            case OSCILLOSCOPE_SPEED_UP | BUTTON_REPEAT:
                if (osc_delay > 1)
                    osc_delay--;
                break;
                
            case OSCILLOSCOPE_SPEED_DOWN:
            case OSCILLOSCOPE_SPEED_DOWN | BUTTON_REPEAT:
                osc_delay++;
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
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
    cleanup(NULL);
    return PLUGIN_OK;
}
#endif /* if HAVE_LCD_BITMAP */
