/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Itai Shaked 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

#define LARGE 55
#define HAUT  31


enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;
    int timer = 10;
    int x=0;
    int y=0;
    int sx = 3;
    int sy = 3;
    struct plugin_api* rb = api;
    TEST_PLUGIN_API(api);
    (void)parameter;

    rb->lcd_clear_display();
    while (1) {

        x+=sx;
        if (x>LARGE) 
        {
            x = 2*LARGE-x;
            sx=-sx;
        }
	
        if (x<0) 
        {
            x = -x;
            sx = -sx;
        }
	
        y+=sy;
        if (y>HAUT) 
        {
            y = 2*HAUT-y;
            sy=-sy;
        }
	
        if (y<0) 
        {
            y = -y;
            sy = -sy;
        }
	
        rb->lcd_invertrect(LARGE-x, HAUT-y, 2*x+1, 1);
        rb->lcd_invertrect(LARGE-x, HAUT+y, 2*x+1, 1);
        rb->lcd_invertrect(LARGE-x, HAUT-y+1, 1, 2*y-1);
        rb->lcd_invertrect(LARGE+x, HAUT-y+1, 1, 2*y-1);

        rb->lcd_update();
        rb->sleep(HZ/timer);
        
        button = rb->button_get(false);
        if ( button == BUTTON_OFF)
        {
            return false;
        }
		 
        if ( button == BUTTON_F1 )
        {
            timer = timer+5;
            if (timer>20)
                timer=5;
        }
		 
        if ( button == BUTTON_PLAY )
        {
            sx = rb->rand()%20+1;
            sy = rb->rand()%20+1;
            x=0;
            y=0;
            rb->lcd_clear_display();
        }
	
        if ( button == SYS_USB_CONNECTED) {
            rb->usb_screen();
            return 0;
        }	

    }
}

#endif
