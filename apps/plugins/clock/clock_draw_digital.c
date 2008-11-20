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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "clock.h"
#include "clock_draw_digital.h"
#include "clock_bitmap_strings.h"
#include "clock_bitmaps.h"
#include "lib/picture.h"

const struct picture* digits_skin[]={digits,segments};
const struct picture* smalldigits_skin[]={smalldigits,smallsegments};

#define buffer_printf(buffer, buffer_pos, ... ) \
    buffer_pos+=rb->snprintf(&buffer[buffer_pos], sizeof(buffer)-buffer_pos, __VA_ARGS__);

void digital_clock_draw(struct screen* display,
                        struct time* time, 
                        struct clock_settings* settings, 
                        struct counter* counter, 
                        int skin){
    bool display_colon;
    const struct picture* digits_bitmaps = &(digits_skin[skin][display->screen_type]);
    const struct picture* smalldigits_bitmaps = &(smalldigits_skin[skin][display->screen_type]);
    int hour=time->hour;
    int str_w, str_h;
    char buffer[20];
    int buffer_pos=0;

    if(settings->digital.blinkcolon){
        display_colon=(time->second%2==0);
    }
    else
        display_colon=true;

    if(settings->general.hour_format==H12 && time->hour>12)/* AM/PM format */
        hour -= 12;

    buffer_printf(buffer, buffer_pos, "%02d", hour);
    buffer_printf(buffer, buffer_pos, "%c", display_colon?':':' ');
    buffer_printf(buffer, buffer_pos, "%02d", time->minute);
    if(settings->general.hour_format==H12){
        if(time->hour>=12){
            buffer_printf(buffer, buffer_pos, "P");/* PM */
        }else{
            buffer_printf(buffer, buffer_pos, "A");/* AM */
        }
    }
    getstringsize(digits_bitmaps, buffer, &str_w, &str_h);
    draw_string(display, digits_bitmaps, buffer,
                (display->getwidth()-str_w)/2, 0);
    if(settings->digital.show_seconds){
        buffer_pos=0;
        buffer_printf(buffer, buffer_pos, "%02d", time->second);
        getstringsize(digits_bitmaps, buffer, &str_w, &str_h);
        draw_string(display, digits_bitmaps, buffer,
                    (display->getwidth()-str_w)/2,
                    digits_bitmaps->height);
    }
    if(settings->general.date_format!=NONE){
        format_date(buffer, time, settings->general.date_format);
        getstringsize(smalldigits_bitmaps, buffer, &str_w, &str_h);
        draw_string(display, smalldigits_bitmaps, buffer,
                    (display->getwidth()-str_w)/2,
                    display->getheight()-smalldigits_bitmaps->height*2);
    }
    if(counter){
        struct time counter_time;
        counter_get_elapsed_time(counter, &counter_time);
        rb->snprintf(buffer, 20, "%02d:%02d:%02d",
                     counter_time.hour, counter_time.minute, counter_time.second);
        getstringsize(smalldigits_bitmaps, buffer, &str_w, &str_h);
        draw_string(display, smalldigits_bitmaps, buffer,
                    (display->getwidth()-str_w)/2, display->getheight()-str_h);
    }
}
