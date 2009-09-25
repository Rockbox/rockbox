/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
 *
 * Fixed point library for plugins
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

/** APPS - FIXED POINT MATH ROUTINES - USAGE
 *
 *  - x and y arguments are fixed point integers
 *  - fracbits is the number of fractional bits in the argument(s)
 *  - functions return long fixed point integers with the specified number
 *    of fractional bits unless otherwise specified
 *
 *  Multiply two fixed point numbers:
 *      fp_mul(x, y, fracbits)
 *
 *  Divide two fixed point numbers:
 *      fp_div(x, y, fracbits)
 *
 *  Calculate decibel equivalent of a gain factor:
 *      fp_decibels(factor, fracbits)
 *          where fracbits is in the range 12 to 22 (higher is better),
 *          and factor is a positive fixed point integer.
 *
 *  Calculate factor equivalent of a decibel value:
 *      fp_factor(decibels, fracbits)
 *          where fracbits is in the range 12 to 22 (lower is better),
 *          and decibels is a fixed point integer.
 */

#ifndef _FIXEDPOINT_H_APPS
#define _FIXEDPOINT_H_APPS

#define fp_mul(x, y, z) (long)((((long long)(x)) * ((long long)(y))) >> (z))
#define fp_div(x, y, z) (long)((((long long)(x)) << (z)) / ((long long)(y)))


/** TAKEN FROM ORIGINAL fixedpoint.h */
long fp_sincos(unsigned long phase, long *cos);


/** MODIFIED FROM replaygain.c */
#define FP_INF          (0x7fffffff)
#define FP_NEGINF      -(0x7fffffff)

/* fracbits in range 12 - 22 work well. Higher is better for
 * calculating dB, lower is better for calculating factor.
 */
/* long fp_decibels(unsigned long factor, unsigned int fracbits); */
long fp_factor(long decibels, unsigned int fracbits);

#endif
