/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "xracer.h"
#include "fixedpoint.h"

/* finds the address of a segment corresponding to a z coordinate */
#define FIND_SEGMENT(z, r, l) (&r[(int)((float)z/SEGMENT_LENGTH)%l])

/* linearly interpolates a and b with percent having FRACBITS fractional bits */
#define INTERPOLATE(a, b, percent) (((a << FRACBITS) + fp_mul((b<<FRACBITS)-(a<<FRACBITS), percent, FRACBITS)) >> FRACBITS)

/* rounds a fixed-point number with FRACBITS fractional bits */
#define ROUND(x) ((x+(1<<(FRACBITS-1)))>>FRACBITS)

void *util_alloc(size_t);

/* camera_calc_depth(): calculates the camera depth given its FOV by evaluating */
/* LCD_WIDTH / tan(fov/2) */
int camera_calc_depth(int fov);
