/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 by Pedro Vasconcelos
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
/* asm routines for wide math on the MCF5249 */

#include "os_types.h"

#if defined(CPU_COLDFIRE)

#ifndef _V_WIDE_MATH
#define _V_WIDE_MATH

#define MB()

#ifndef _TREMOR_VECT_OPS
#define _TREMOR_VECT_OPS
static inline 
void vect_add_left_right(ogg_int32_t *x, const ogg_int32_t *y, int n)
{
    /* coldfire asm has symmetrical versions of vect_add_right_left
       and vect_add_left_right  (since symmetrical versions of
       vect_mult_fw and vect_mult_bw  i.e.  both use MULT31) */
    vect_add(x, y, n );
}

static inline 
void vect_add_right_left(ogg_int32_t *x, const ogg_int32_t *y, int n)
{
    /* coldfire asm has symmetrical versions of vect_add_right_left
       and vect_add_left_right  (since symmetrical versions of
       vect_mult_fw and vect_mult_bw  i.e.  both use MULT31) */
    vect_add(x, y, n );
}

static inline
void ogg_vect_mult_fw(int32_t *data, const int32_t *window, int n)
{
    vect_mult_fw(data, window, n);
}

static inline
void ogg_vect_mult_bw(int32_t *data, const int32_t *window, int n)
{
    vect_mult_bw(data, window, n);
}
#endif
#endif
#endif
