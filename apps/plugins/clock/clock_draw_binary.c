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
#include "clock_draw_binary.h"
#include "clock_bitmap_strings.h"
#include "clock_bitmaps.h"
#include "lib/picture.h"

const struct picture* binary_skin[]={binary,digits,segments};

void print_binary(char* buffer, int number, int nb_bits){
    int i;
    int mask=1;
    buffer[nb_bits]='\0';
    for(i=0; i<nb_bits; i++){
        if((number & mask) !=0)
            buffer[nb_bits-i-1]='1';
        else
            buffer[nb_bits-i-1]='0';
        mask=mask<<1;
    }
}

void binary_clock_draw(struct screen* display, struct time* time, int skin){
    int lines_values[]={
        time->hour,time->minute,time->second
    };
    char buffer[9];
    int i;
    const struct picture* binary_bitmaps = &(binary_skin[skin][display->screen_type]);
    int y_offset=(display->getheight()-(binary_bitmaps->height*3))/2;
    int x_offset=(display->getwidth()-(binary_bitmaps->width*6))/2;
    for(i=0;i<3;i++){
        print_binary(buffer, lines_values[i], 6);
        draw_string(display, binary_bitmaps, buffer, x_offset,
                    binary_bitmaps->height*i+y_offset);
    }
}
