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

#ifndef LIB_HSV_RGB_H
#define LIB_HSV_RGB_H

/***********************************************************************
 * Colorspace transformations
 ***********************************************************************
 * r, g and b range from 0 to 255
 * h ranges from 0 to 3599 (which in fact means 0.0 to 359.9).
 *   360 is the same as 0 (it loops)
 * s and v range from 0 to 255 (which in fact means 0.00 to 1.00)
 ***********************************************************************/

void rgb2hsv( int r, int g, int b, int *h, int *s, int *v );
void hsv2rgb( int h, int s, int v, int *r, int *g, int *b );

#endif
