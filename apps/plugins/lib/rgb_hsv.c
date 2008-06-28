/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Antoine Cellerier <dionoea -at- videolan -dot- org>
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

#include "rgb_hsv.h"

/***********************************************************************
 * Colorspace transformations
 ***********************************************************************
 * r, g and b range from 0 to 255
 * h ranges from 0 to 3599 (which in fact means 0.0 to 359.9).
 *   360 is the same as 0 (it loops)
 * s and v range from 0 to 255 (which in fact means 0.00 to 1.00)
 ***********************************************************************/

void rgb2hsv( int r, int g, int b, int *h, int *s, int *v )
{
    int max;
    int min;

    max = r > g ? r : g;
    if( b > max ) max = b;

    min = r < g ? r : g;
    if( b < min ) min = b;

    if( max == 0 )
    {
        *v = 0;
        *h = 0; *s = 0; /* Random since it's black */
        return;
    }
    else if( max == min )
    {
        *h = 0; /* Random since it's gray */
    }
    else if( max == r && g >= b )
    {
        *h = ( 10 * 60 * ( g - b )/( max - min ) );
    }
    else if( max == r && g < b )
    {
        *h = ( 10 * ( 60 * ( g - b )/( max - min ) + 360 ));
    }
    else if( max == g )
    {
        *h = ( 10 * ( 60 * ( b - r )/( max - min ) + 120 ));
    }
    else// if( max == b )
    {
        *h = ( 10 * ( 60 * ( r - g )/( max - min ) + 240 ));
    }

    /* Just in case ? */
    while( *h < 0 ) *h += 3600;
    while( *h >= 3600 ) *h-= 3600;

    *s = (( max - min )*255)/max;
    *v = max;
}

void hsv2rgb( int h, int s, int v, int *r, int *g, int *b )
{
    int f, p, q, t;
    while( h < 0 ) h += 3600;
    while( h >= 3600 ) h-= 3600;
    f = h%600;
    p = ( v * ( 255 - s ) ) / ( 255 );
    q = ( v * ( 600*255 - f*s ) ) / ( 255 * 600 );
    t = ( v * ( 600*255 - ( 600 - f ) * s ) ) / ( 255 * 600 );


    if( s == 0 ) /* gray */
    {
        *r = v;
        *g = *r;
        *b = *r;
        return;
    }

    switch( h/600 )
    {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        case 5:
            *r = v;
            *g = p;
            *b = q;
            break;
    }
}
