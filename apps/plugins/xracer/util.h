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

#include <stdint.h>

#include "xracer.h"
#include "fixedpoint.h"

/* finds the address of a segment corresponding to a z coordinate */
#define FIND_SEGMENT(z, r, l) (&r[(int)((float)z/SEGMENT_LENGTH)%l])

/* linearly interpolates a and b with percent having FRACBITS fractional bits */
#define INTERPOLATE(a, b, percent) (((a << FRACBITS) + fp_mul((b<<FRACBITS)-(a<<FRACBITS), percent, FRACBITS)) >> FRACBITS)

/* rounds a fixed-point number with FRACBITS fractional bits */
/* returns an integer */
/* adds 1/2 and truncates the fractional bits */
#define ROUND(x) ((x+(1<<(FRACBITS-1)))>>FRACBITS)

#ifdef ROCKBOX_LITTLE_ENDIAN
#define READ_BE_UINT16(p) ((((const uint8_t*)p)[0] << 8) | ((const uint8_t*)p)[1])
#define READ_BE_UINT32(p) ((((const uint8_t*)p)[0] << 24) | (((const uint8_t*)p)[1] << 16) | (((const uint8_t*)p)[2] << 8) | ((const uint8_t*)p)[3])
#else
#define READ_BE_UINT16(p) (*(const uint16_t*)p)
#define READ_BE_UINT32(p) (*(const uint32_t*)p)
#endif

/* call this before using any alloc functions or the plugin buffer */
void init_alloc(void);

void *util_alloc(size_t);

size_t util_alloc_remaining(void);

/* camera_calc_depth(): calculates the camera depth given its FOV by evaluating */
/* LCD_WIDTH / tan(fov/2) */
int camera_calc_depth(int fov);

void warning_real(const char *msg, ...);

void error_real(const char *msg, ...);

unsigned short crc16_ccitt(unsigned char *data, size_t length, unsigned short seed, unsigned short final);

#define warning warning_real
#define error error_real
/*#define warning(msg, ...) warning_real("in function %s at %s:%d: %s", __func__, __FILE__, __LINE__, msg, ##__VA_ARGS__)
  #define error(msg, ...) error_real("in function %s at %s:%d: %s", __func__, __FILE__, __LINE__, msg, ##__VA_ARGS__)*/
