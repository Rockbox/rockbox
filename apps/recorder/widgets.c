/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: not checked in
 *
 * Copyright (C) 2002 Markus Braun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <lcd.h>

#include "widgets.h"

#ifdef HAVE_LCD_BITMAP
/*
 * Print a progress bar
 */
void progressbar(int x, int y, int width, int height, int percent, int direction)
{
    int pos;
    int i,j;

    /* draw horizontal lines */
    for(i=x+1;i<=x+width-2;i++) {
        DRAW_PIXEL(i,y);
        DRAW_PIXEL(i,(y+height-1));
    }

    /* draw vertical lines */
    for(i=1;i<height;i++) {
        DRAW_PIXEL(x,(y+i));
        DRAW_PIXEL((x+width-1),(y+i));
    }

    /* clear edge pixels */
    CLEAR_PIXEL(x,y);
    CLEAR_PIXEL((x+width-1),y);
    CLEAR_PIXEL(x,(y+height-1));
    CLEAR_PIXEL((x+width-1),(y+height-1));

    /* clear pixels in progress bar */
    for(i=1;i<=width-2;i++) {
        for(j=1;j<=height-2;j++) {
            CLEAR_PIXEL((x+i),(y+j));
            CLEAR_PIXEL((x+i),(y+j));
        }
    }

    /* draw bar */
    pos=percent;
    if(pos<0)
        pos=0;
    if(pos>100)
        pos=100;

    switch (direction)
    {
        case Grow_Right:
            pos=(width-2)*pos/100;
            for(i=1;i<=pos;i++)
                for(j=1;j<height-1;j++)
                    DRAW_PIXEL((x+i),(y+j));
            break;
        case Grow_Left:
            pos=(width-2)*(100-pos)/100;
            for(i=pos+1;i<=width-2;i++)
                for(j=1;j<height-1;j++)
                    DRAW_PIXEL((x+i),(y+j));
            break;
        case Grow_Down:
            pos=(height-2)*pos/100;
            for(i=1;i<=pos;i++)
                for(j=1;j<width-1;j++)
                    DRAW_PIXEL((x+j),(y+i));
            break;
        case Grow_Up:
            pos=(height-2)*(100-pos)/100;
            for(i=pos+1;i<=height-2;i++)
                for(j=1;j<width-1;j++)
                    DRAW_PIXEL((x+j),(y+i));
            break;
    }

}


/*
 * Print a slidebar bar
 */
void slidebar(int x, int y, int width, int height, int percent, int direction)
{
    int pos;
    int i,j;

    /* draw horizontal lines */
    for(i=x+1;i<=x+width-2;i++) {
        DRAW_PIXEL(i,y);
        DRAW_PIXEL(i,(y+height-1));
    }

    /* draw vertical lines */
    for(i=1;i<height;i++) {
        DRAW_PIXEL(x,(y+i));
        DRAW_PIXEL((x+width-1),(y+i));
    }

    /* clear edge pixels */
    CLEAR_PIXEL(x,y);
    CLEAR_PIXEL((x+width-1),y);
    CLEAR_PIXEL(x,(y+height-1));
    CLEAR_PIXEL((x+width-1),(y+height-1));

    /* clear pixels in progress bar */
    for(i=1;i<=width-2;i++)
        for(j=1;j<=height-2;j++) {
            CLEAR_PIXEL((x+i),(y+j));
            CLEAR_PIXEL((x+i),(y+j));
        }

    /* draw point */
    pos=percent;
    if(pos<0)
        pos=0;
    if(pos>100)
        pos=100;

    switch (direction)
    {
        case Grow_Right:
            pos=(width-height-1)*pos/100;
            break;
        case Grow_Left:
            pos=(width-height-1)*(100-pos)/100;
            break;
        case Grow_Down:
            pos=(height-width-1)*pos/100;
            break;
        case Grow_Up:
            pos=(height-width-1)*(100-pos)/100;
            break;
    }

    if(direction == Grow_Left || direction == Grow_Right)
        for(i=1;i<height;i++)
            for(j=1;j<height;j++)
                DRAW_PIXEL((x+pos+i),(y+j));
    else
        for(i=1;i<width;i++)
            for(j=1;j<width;j++)
                DRAW_PIXEL((x+i),(y+pos+j));
}
#endif
