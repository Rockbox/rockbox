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
 * Thanks to Jens Arnold and Joerg Hohensohn for the speed tips.
 * Boy, that was a hell of a code review ;-)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 *
 * further optimization ideas:
 *    - incremental recalculation when moving
 *    
 ****************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP // this is not fun on the player

static struct plugin_api* rb;
static char buff[32];
static int lcd_aspect_ratio;
static int x_min;
static int x_max;
static int y_min;
static int y_max;
static int delta;
static int max_iter;

void init_mandelbrot_set(void){
    x_min = -5<<27;  // -2.5<<28
    x_max =  1<<28;  //  1.0<<28
    y_min = -1<<28;  // -1.0<<28
    y_max =  1<<28;  //  1.0<<28
    delta = (x_max - x_min) >> 3;  // /8
    max_iter = 25;
}

void calc_mandelbrot_set(void){
    
    unsigned int start_tick;
    int n_iter;
    int x_pixel, y_pixel;
    int x, x2, y, y2, a, b;
    int x_fact, y_fact;
    
    start_tick = *rb->current_tick;  
    
    rb->lcd_clear_display();
    rb->lcd_update();

    x_fact = (x_max - x_min) / LCD_WIDTH;
    y_fact = (y_max - y_min) / LCD_HEIGHT;
    
    for(y_pixel = LCD_HEIGHT-1; y_pixel>=0; y_pixel--){    
        b = (y_pixel * y_fact) + y_min;
        for (x_pixel = LCD_WIDTH-1; x_pixel>=0; x_pixel--){
            a = (x_pixel * x_fact) + x_min;
            x = 0;
            y = 0;
            n_iter = 0;
            
            while (++n_iter<=max_iter) {
                x >>= 14;
                y >>= 14;
                x2 = x * x;
                y2 = y * y;
                
                if (x2 + y2 > (4<<28)) break;
                
                y = 2 * x * y + b;
                x = x2 - y2 + a;
            }
            
            // "coloring"
            if ( (n_iter > max_iter) ||
                (n_iter == (max_iter >> 1)) ||
                (n_iter == (max_iter >> 2)) ||
                (n_iter == (max_iter >> 3)) 
                ){
                rb->lcd_drawpixel(x_pixel,y_pixel);
            }
        }
        
        /* update block of 8 lines */
        if ((y_pixel & 0x7) == 0)
            rb->lcd_update_rect(0, y_pixel, LCD_WIDTH, 8);
    }
    
    /* we want to know how long we had to wait */
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->snprintf(buff, sizeof(buff), "%d", (*rb->current_tick - start_tick));
    rb->lcd_puts(0,0,buff);
    rb->snprintf(buff, sizeof(buff), "%d",  max_iter);
    rb->lcd_puts(0,1,buff);
    rb->lcd_update_rect(0,0,24,16);
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;

    init_mandelbrot_set();
    lcd_aspect_ratio = ((LCD_WIDTH<<14) / LCD_HEIGHT)<<14;

    /* main loop */
    while (true){

	    calc_mandelbrot_set();

	    /* FIXME: is this the right way to empty the key-queue? 
	       (Otherwise we have to process things twice :-/ ?! */
	    
	    rb->button_get(true);


	    switch (rb->button_get(true)) {
	    case BUTTON_OFF:
	        return PLUGIN_OK;

	    case BUTTON_ON:
	        x_min -= ((delta>>14)*(lcd_aspect_ratio>>14));
	        x_max += ((delta>>14)*(lcd_aspect_ratio>>14));
	        y_min -= delta;
	        y_max += delta;
	        delta = (x_max - x_min) >> 3;
	        break;
 	        

	    case BUTTON_PLAY:
	        x_min += ((delta>>14)*(lcd_aspect_ratio>>14));
	        x_max -= ((delta>>14)*(lcd_aspect_ratio>>14));
	        y_min += delta;
	        y_max -= delta;
	        delta = (x_max - x_min) >> 3;
	        break;
      
	    case BUTTON_UP:
	        y_min -= delta;
	        y_max -= delta;
	        break;

	    case BUTTON_DOWN:
	        y_min += delta;
	        y_max += delta;
	        break;

	    case BUTTON_LEFT:
	        x_min -= delta;
	        x_max -= delta;
	        break;

	    case BUTTON_RIGHT:
	        x_min += delta;
	        x_max += delta;
	        break;

	    case BUTTON_F1:
	        if (max_iter>5){
		    max_iter -= 5;
	        }
	        break;

	    case BUTTON_F2:
	        if (max_iter < 195){
		    max_iter += 5;
	        }
	        break;

	    case BUTTON_F3:
	        init_mandelbrot_set();
	        break;

	        
	    case SYS_USB_CONNECTED:
	        rb->usb_screen();
	        return PLUGIN_USB_CONNECTED;
	    }
    }
    return false;
}
#endif
