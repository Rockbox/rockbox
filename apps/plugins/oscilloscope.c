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
#if CONFIG_HWCODEC != MASNONE /* only for MAS-targets */

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

void lcd_scroll_left(int count, bool black_border)
{
    int by;
    unsigned filler;
    unsigned char *ptr;

    if ((unsigned) count >= LCD_WIDTH)
        return;
        
    filler = black_border ? 0xFF : 0;

    for (by = 0; by < (LCD_HEIGHT/8); by++)
    {
        ptr = rb->lcd_framebuffer + MULU16(LCD_WIDTH, by);
        asm volatile (
            "mov     %0,r1            \n" /* check if both source... */
            "or      %2,r1            \n" /* ...and offset are even */
            "shlr    r1               \n" /* -> lsb = 0 */
            "bf      .sl_start2       \n" /* -> copy word-wise */

            "add     #-1,%2           \n" /* copy byte-wise */
        ".sl_loop1:                   \n"
            "mov.b   @%0+,r1          \n"
            "mov.b   r1,@(%2,%0)      \n"
            "cmp/hi  %0,%1            \n"
            "bt      .sl_loop1        \n"

            "bra     .sl_end          \n"
            "nop                      \n"

        ".sl_start2:                  \n" /* copy word-wise */
            "add     #-2,%2           \n"
        ".sl_loop2:                   \n"
            "mov.w   @%0+,r1          \n"
            "mov.w   r1,@(%2,%0)      \n"
            "cmp/hi  %0,%1            \n"
            "bt      .sl_loop2        \n"

        ".sl_end:                     \n"
            : /* outputs */
            : /* inputs */
            /* %0 */ "r"(ptr + count),
            /* %1 */ "r"(ptr + LCD_WIDTH),
            /* %2 */ "z"(-count)
            : /* clobbers */
            "r1"
        );

        rb->memset(ptr + LCD_WIDTH - count, filler, count);
    }
}

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
                lcd_scroll_left(1, false);
                x = LCD_WIDTH-1;
                full_update = true;
            }
            else
                x = 0;
        }
        
        rb->lcd_clearline(x, 0, x, LCD_HEIGHT-1);

        switch (draw_mode)
        {
            case DRAW_MODE_FILLED:
                rb->lcd_drawline(x, LCD_HEIGHT/2+1,
                                 x, LCD_HEIGHT/2+1 + left_val);
                rb->lcd_drawline(x, LCD_HEIGHT/2-1,
                                 x, LCD_HEIGHT/2-1 - right_val);
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
    
    rb->plugin_unregister_timer();
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button, vol;
    bool exit = false;
    bool paused = false;

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;

    rb->plugin_register_timer(FREQ / 67, 1, timer_isr);

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
                if (vol < 100)
                {
                    vol++;
                    rb->mpeg_sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }
                break;

            case OSCILLOSCOPE_VOL_DOWN:
            case OSCILLOSCOPE_VOL_DOWN | BUTTON_REPEAT:
                vol = rb->global_settings->volume;
                if (vol > 0)
                {
                    vol--;
                    rb->mpeg_sound_set(SOUND_VOLUME, vol);
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
