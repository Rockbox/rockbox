/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Fred Bauer
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

#include <stdbool.h>
#include <stdio.h>
#include "buttonmap.h"
#include "config.h"

int xy2button( int x, int y)
{
    int i;
    extern bool debug_buttons;
    
    for ( i = 0; bm[i].button; i++ )
        /* check distance from center of button < radius */
        if ( ( (x-bm[i].x)*(x-bm[i].x) ) + ( ( y-bm[i].y)*(y-bm[i].y) ) < bm[i].radius*bm[i].radius ) {
            if (debug_buttons) 
                printf("Button: %s\n", bm[i].description );
            return bm[i].button;
        }
    return 0;
}
