/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Matthias Wientapper
 *
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SIMULATOR 
#include "plugin.h"

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH < 4)
#include "gray.h"

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_ZOOM_IN BUTTON_PLAY
#define MANDELBROT_ZOOM_OUT BUTTON_ON
#define MANDELBROT_MAXITER_INC BUTTON_F2
#define MANDELBROT_MAXITER_DEC BUTTON_F1
#define MANDELBROT_RESET BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_ZOOM_IN_PRE BUTTON_MENU
#define MANDELBROT_ZOOM_IN (BUTTON_MENU | BUTTON_REL)
#define MANDELBROT_ZOOM_IN2 (BUTTON_MENU | BUTTON_UP)
#define MANDELBROT_ZOOM_OUT (BUTTON_MENU | BUTTON_DOWN)
#define MANDELBROT_MAXITER_INC (BUTTON_MENU | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_MENU | BUTTON_LEFT)
#define MANDELBROT_RESET (BUTTON_MENU | BUTTON_OFF)

#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_ZOOM_IN BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT BUTTON_MODE
#define MANDELBROT_MAXITER_INC (BUTTON_ON | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_ON | BUTTON_LEFT)
#define MANDELBROT_RESET BUTTON_REC
#endif

static struct plugin_api* rb;
static char buff[32];
static int lcd_aspect_ratio;
static int x_min;
static int x_max;
static int y_min;
static int y_max;
static int delta;
static int max_iter;
static unsigned char *gbuf;
static unsigned int gbuf_size = 0;
static unsigned char graybuffer[LCD_HEIGHT];


void init_mandelbrot_set(void){
#if CONFIG_LCD == LCD_LCD_SSD1815 /* non-square aspect */
    x_min = -5<<25;  // -2.5<<26
    x_max =  1<<26;  //  1.0<<26
#else
    x_min = -2<<26;  // -2.0<<26
    x_max =  3<<24;  //  0.75<<26
#endif
    y_min = -1<<26;  // -1.0<<26
    y_max =  1<<26;  //  1.0<<26
    delta = (x_max - x_min) >> 3;  // /8
    max_iter = 25;
}

void calc_mandelbrot_set(void){
    
    long start_tick, last_yield;
    int n_iter;
    int x_pixel, y_pixel;
    int x, x2, y, y2, a, b;
    int x_fact, y_fact;
    int brightness;
    
    start_tick = last_yield = *rb->current_tick;
    
    gray_ub_clear_display();

    x_fact = (x_max - x_min) / LCD_WIDTH;
    y_fact = (y_max - y_min) / LCD_HEIGHT;
    
    for (x_pixel = 0; x_pixel<LCD_WIDTH; x_pixel++){
        a = (x_pixel * x_fact) + x_min;
        for(y_pixel = LCD_HEIGHT-1; y_pixel>=0; y_pixel--){
            b = (y_pixel * y_fact) + y_min;
            x = 0;
            y = 0;
            n_iter = 0;

            while (++n_iter<=max_iter) {
                x >>= 13;
                y >>= 13;
                x2 = x * x;
                y2 = y * y;
                
                if (x2 + y2 > (4<<26)) break;
                
                y = 2 * x * y + b;
                x = x2 - y2 + a;
            }

            // "coloring"
            brightness = 0;
            if  (n_iter > max_iter){
                brightness = 0; // black
            } else {
                brightness = 255 - (32 * (n_iter & 7));
            }
            graybuffer[y_pixel]=brightness;
            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (*rb->current_tick > last_yield){
                rb->yield();
                last_yield = *rb->current_tick;
            }
        }
        gray_ub_gray_bitmap(graybuffer, x_pixel, 0, 1, LCD_HEIGHT);
    }
}

void cleanup(void *parameter)
{
    (void)parameter;
    
    gray_release();
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;
    int lastbutton = BUTTON_NONE;
    int grayscales;
    bool redraw = true;

    TEST_PLUGIN_API(api);
    rb = api;
    (void)parameter;

    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the grayscale buffer:
     * 112 pixels wide, 8 rows (64 pixels) high, (try to) reserve
     * 16 bitplanes for 17 shades of gray.*/
    grayscales = gray_init(rb, gbuf, gbuf_size, false, LCD_WIDTH, 
                           (LCD_HEIGHT*LCD_DEPTH/8), 8, NULL) + 1;
    if (grayscales != 9){
        rb->snprintf(buff, sizeof(buff), "%d", grayscales);
        rb->lcd_puts(0, 1, buff);
        rb->lcd_update();
        rb->sleep(HZ*2);
        return(0);
    }

    gray_show(true); /* switch on greyscale overlay */

    init_mandelbrot_set();
    lcd_aspect_ratio = ((LCD_WIDTH<<13) / LCD_HEIGHT)<<13;

    /* main loop */
    while (true){
        if(redraw)
            calc_mandelbrot_set();

        redraw = false;
        
        button = rb->button_get(true);
        switch (button) {
        case MANDELBROT_QUIT:
            gray_release();
            return PLUGIN_OK;

        case MANDELBROT_ZOOM_OUT:
            x_min -= ((delta>>13)*(lcd_aspect_ratio>>13));
            x_max += ((delta>>13)*(lcd_aspect_ratio>>13));
            y_min -= delta;
            y_max += delta;
            delta = (x_max - x_min) >> 3;
            redraw = true;
            break;


        case MANDELBROT_ZOOM_IN:
#ifdef MANDELBROT_ZOOM_IN_PRE
            if (lastbutton != MANDELBROT_ZOOM_IN_PRE)
                break;
#endif
#ifdef MANDELBROT_ZOOM_IN2
        case MANDELBROT_ZOOM_IN2:
#endif
            x_min += ((delta>>13)*(lcd_aspect_ratio>>13));
            x_max -= ((delta>>13)*(lcd_aspect_ratio>>13));
            y_min += delta;
            y_max -= delta;
            delta = (x_max - x_min) >> 3;
            redraw = true;
            break;

        case BUTTON_UP:
            y_min -= delta;
            y_max -= delta;
            redraw = true;
            break;

        case BUTTON_DOWN:
            y_min += delta;
            y_max += delta;
            redraw = true;
            break;

        case BUTTON_LEFT:
            x_min -= delta;
            x_max -= delta;
            redraw = true;
            break;

        case BUTTON_RIGHT:
            x_min += delta;
            x_max += delta;
            redraw = true;
            break;

        case MANDELBROT_MAXITER_DEC:
            if (max_iter>5){
                max_iter -= 5;
                redraw = true;
            }
            break;

        case MANDELBROT_MAXITER_INC:
            if (max_iter < 195){
                max_iter += 5;
                redraw = true;
            }
            break;

        case MANDELBROT_RESET:
            init_mandelbrot_set();
            redraw = true;
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
    gray_release();
    return PLUGIN_OK;
}
#endif
#endif
