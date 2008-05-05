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
