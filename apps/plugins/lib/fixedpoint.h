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

/** PLUGINS - FIXED POINT MATH ROUTINES - USAGE
 *
 *  - x and y arguments are fixed point integers
 *  - fracbits is the number of fractional bits in the argument(s)
 *  - functions return long fixed point integers with the specified number
 *    of fractional bits unless otherwise specified
 *
 *  Calculate sin and cos of an angle:
 *      fp_sincos(phase, *cos)
 *          where phase is a 32 bit unsigned integer with 0 representing 0
 *          and 0xFFFFFFFF representing 2*pi, and *cos is the address to
 *          a long signed integer.  Value returned is a long signed integer
 *          from -0x80000000 to 0x7fffffff, representing -1 to 1 respectively.
 *          That is, value is a fixed point integer with 31 fractional bits.
 *
 *  Take square root of a fixed point number:
 *      fp_sqrt(x, fracbits)
 *
 *  Calculate sin or cos of an angle (very fast, from a table):
 *      fp14_sin(angle)
 *      fp14_cos(angle)
 *          where angle is a non-fixed point integer in degrees.  Value
 *          returned is a fixed point integer with 14 fractional bits.
 *
 *  Calculate the natural log of a positive fixed point integer
 *      fp16_log(x)
 *          where x and the value returned are fixed point integers
 *          with 16 fractional bits.
 */

#ifndef _FIXEDPOINT_H_PLUGINS
#define _FIXEDPOINT_H_PLUGINS

long fp_sincos(unsigned long phase, long *cos);
long fp_sqrt(long a, unsigned int fracbits);
long fp14_cos(int val);
long fp14_sin(int val);
long fp16_log(int x);
long fp16_exp(int x);

unsigned long isqrt(unsigned long x);

/* fast unsigned multiplication (16x16bit->32bit or 32x32bit->32bit,
 * whichever is faster for the architecture) */
#ifdef CPU_ARM
#define FMULU(a, b) ((uint32_t) (((uint32_t) (a)) * ((uint32_t) (b))))
#else /* SH1, coldfire */
#define FMULU(a, b) ((uint32_t) (((uint16_t) (a)) * ((uint16_t) (b))))
#endif

#endif
