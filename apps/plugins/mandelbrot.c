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

#ifdef HAVE_LCD_BITMAP // this is not fun on the player
# include "gray.h"

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
    x_min = -5<<25;  // -2.0<<26
    x_max =  1<<26;  //  1.0<<26
    y_min = -1<<26;  // -1.0<<26
    y_max =  1<<26;  //  1.0<<26
    delta = (x_max - x_min) >> 3;  // /8
    max_iter = 25;
}

void calc_mandelbrot_set(void){
    
    unsigned int start_tick, last_yield;
    int n_iter;
    int x_pixel, y_pixel;
    int x, x2, y, y2, a, b;
    int x_fact, y_fact;
    int brightness;
    
    start_tick = last_yield = *rb->current_tick;
    
    gray_clear_display();

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
                brightness = 255 - (31 * (n_iter & 7));
            }
            graybuffer[y_pixel]=brightness;
            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (*rb->current_tick > last_yield){
                rb->yield();
                last_yield = *rb->current_tick;
            }
        }
        gray_drawgraymap(graybuffer, x_pixel, 0, 1, LCD_HEIGHT, 1);
    }
}


enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int grayscales;
    bool redraw = true;

    TEST_PLUGIN_API(api);
    rb = api;
    (void)parameter;

    /* This plugin uses the grayscale framework, so initialize */
    gray_init(api);

    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the grayscale buffer:
     * 112 pixels wide, 8 rows (64 pixels) high, (try to) reserve
     * 16 bitplanes for 17 shades of gray.*/
    grayscales = gray_init_buffer(gbuf, gbuf_size, 112, 8, 16, NULL) + 1;
    if (grayscales != 17){
        rb->snprintf(buff, sizeof(buff), "%d", grayscales);
        rb->lcd_puts(0, 1, buff);
        rb->lcd_update();
        rb->sleep(HZ*2);
        return(0);
    }

    gray_show_display(true); /* switch on grayscale overlay */

    init_mandelbrot_set();
    lcd_aspect_ratio = ((LCD_WIDTH<<13) / LCD_HEIGHT)<<13;

    /* main loop */
    while (true){
        if(redraw)
            calc_mandelbrot_set();

        redraw = false;
        
        switch (rb->button_get(true)) {
        case BUTTON_OFF:
            gray_release_buffer();
            return PLUGIN_OK;

        case BUTTON_ON:
            x_min -= ((delta>>13)*(lcd_aspect_ratio>>13));
            x_max += ((delta>>13)*(lcd_aspect_ratio>>13));
            y_min -= delta;
            y_max += delta;
            delta = (x_max - x_min) >> 3;
            redraw = true;
            break;


        case BUTTON_PLAY:
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

        case BUTTON_F1:
            if (max_iter>5){
                max_iter -= 5;
                redraw = true;
            }
            break;

        case BUTTON_F2:
            if (max_iter < 195){
                max_iter += 5;
                redraw = true;
            }
            break;

        case BUTTON_F3:
            init_mandelbrot_set();
            redraw = true;
            break;

        case SYS_USB_CONNECTED:
            gray_release_buffer();
            rb->usb_screen();
            return PLUGIN_USB_CONNECTED;
        }
    }
    gray_release_buffer();
    return false;
}
#endif
#endif
