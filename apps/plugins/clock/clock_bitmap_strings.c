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
#include "clock_bitmap_strings.h"

void draw_string(struct screen* display, const struct picture* bitmaps,
                 char* str, int x, int y){
    int i, bitmap_pos;
    char c;
    for(i=0;(c=str[i]);i++){
        bitmap_pos=-1;
        if(c>='0'&&c<='9')
            bitmap_pos=c-'0';
        else if(c==':')
            bitmap_pos=10;
        else if(c=='A' || c=='/')/* 'AM' in digits, '/' in smalldigits */
            bitmap_pos=11;
        else if(c=='P' || c=='.')/* 'PM' in digits, '.' in smalldigits */
            bitmap_pos=12;
        if(bitmap_pos>=0)
            vertical_picture_draw_sprite(display, bitmaps, bitmap_pos,
                x+i*bitmaps->width, y);
    }
}

void getstringsize(const struct picture* bitmaps, char* str, int *w, int *h ){
    *h=bitmaps->height;
    *w=rb->strlen(str)*bitmaps->width;
}
