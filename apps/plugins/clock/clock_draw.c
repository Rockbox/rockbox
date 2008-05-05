/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: jackpot.c 14034 2007-07-28 05:42:55Z kevin $
 *
 * Copyright (C) 2007 Copyright Kévin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "clock.h"
#include "clock_draw.h"
#include "clock_draw_digital.h"
#include "clock_draw_analog.h"
#include "clock_draw_binary.h"
#include "clock_settings.h"

void black_background(struct screen* display){
#if (LCD_DEPTH > 1) || (defined(LCD_REMOTE_DEPTH) && (LCD_REMOTE_DEPTH > 1))
    if(display->depth>1){
        display->set_background(LCD_BLACK);
        display->clear_display();
    }else
#endif
    {
        display->clear_display();
        display->fillrect(0,0,display->width,display->height);
    }
}

void white_background(struct screen* display){
#if (LCD_DEPTH > 1) || (defined(LCD_REMOTE_DEPTH) && (LCD_REMOTE_DEPTH > 1))
    if(display->depth>1){
#if defined(HAVE_LCD_COLOR)
        if(display->is_color)/* restore to the bitmap's background */
            display->set_background(LCD_RGBPACK(180,200,230));
        else
#endif
            display->set_background(LCD_WHITE);
    }
#endif
    display->clear_display();
}

bool skin_require_black_background(int mode, int skin){
    return((mode==BINARY && skin==2) || (mode==DIGITAL && skin==1 ));
}

void skin_set_background(struct screen* display, int mode, int skin){
    if(skin_require_black_background(mode, skin) )
        black_background(display);
    else
        white_background(display);
}

void skin_restore_background(struct screen* display, int mode, int skin){
    if(skin_require_black_background(mode, skin) )
        white_background(display);
}

void clock_draw_set_colors(void){
    int i;
    FOR_NB_SCREENS(i)
        skin_set_background(rb->screens[i],
                            clock_settings.mode,
                            clock_settings.skin[clock_settings.mode]);
}

void clock_draw_restore_colors(void){
    int i;
    FOR_NB_SCREENS(i){
            skin_restore_background(rb->screens[i],
                                    clock_settings.mode,
                                    clock_settings.skin[clock_settings.mode]);
            rb->screens[i]->update();
    }
}

void clock_draw(struct screen* display, struct time* time,
                struct counter* counter){
    if(!clock_settings.general.show_counter)
        counter=0;
    int skin=clock_settings.skin[clock_settings.mode];
    skin_set_background(display, clock_settings.mode, skin);
    if(clock_settings.mode == ANALOG)
        analog_clock_draw(display, time, &clock_settings, counter, skin);

    else if(clock_settings.mode == DIGITAL)
        digital_clock_draw(display, time, &clock_settings, counter, skin);

    else if(clock_settings.mode == BINARY)
        binary_clock_draw(display, time, skin);
    display->update();
}
