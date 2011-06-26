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

 /** CODECS - FIXED POINT MATH ROUTINES - USAGE
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
 */
#ifndef _FIXEDPOINT_H_CODECS
#define _FIXEDPOINT_H_CODECS

long fp_sincos(unsigned long phase, long *cos);
long fp_sqrt(long a, unsigned int fracbits);

#endif
