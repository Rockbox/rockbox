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

#include "picture.h"

void picture_draw(struct screen* display, const struct picture* picture,
                  int x, int y){
    display->bitmap(
        picture->data, 
        x, y,
        picture->width, picture->height
    );
}

/**
 * Draws a part of the given picture on the given screen
 * Use it when the data contains multiple pictures from top to bottom.
 * In that case, picture.height represents the height of one picture,
 * not the whole set.
 * @param display the screen where to display the picture
 * @param picture the picture's data, only a part will be displayed
 * @param yoffset display the data in the picture from yoffset to
 *                yoffset+picture.height
 * @param x abscissa where to put the picture
 * @param y ordinate where to put the picture
 */
void vertical_picture_draw_part(struct screen* display, const struct picture* picture,
                       int yoffset,
                       int x, int y){
    display->bitmap_part(
        picture->data, 
        /*slice into picture->data */
        0, yoffset,
        picture->width,
        /* Position on the screen */
        x, y, picture->width, picture->height
    );
}

/**
 * Draws a part of the given picture on the given screen
 * Use it when the data contains multiple pictures from top to bottom.
 *
 * @param display the screen where to display the picture
 * @param picture the picture's data, only a part will be displayed
 * @param sprite_no display that sprite in the picture
 * @param x abscissa where to put the picture
 * @param y ordinate where to put the picture
 */
void vertical_picture_draw_sprite(struct screen* display, const struct picture* picture,
                       int sprite_no,
                       int x, int y){
    vertical_picture_draw_part(display, picture, sprite_no*picture->height, x, y);
}
