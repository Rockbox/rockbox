/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Akio Idehara
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
#ifndef _RESIZE_H_
#define _RESIZE_H_
#include "config.h"
#include "lcd.h"
#include "inttypes.h"

/****************************************************************
 * resize_on_load()
 *
 * resize bitmap on load with scaling
 *
 * If HAVE_LCD_COLOR then this func use smooth scaling algorithm
 * - downscaling both way use "Area Sampling"
 *   if IMG_RESIZE_BILINER or IMG_RESIZE_NEAREST is NOT set
 * - otherwise "Bilinear" or "Nearest Neighbour"
 *
 * If !(HAVE_LCD_COLOR) then use simple scaling algorithm "Nearest Neighbour"
 * 
 * return -1 for error
 ****************************************************************/

/* nothing needs the on-stack buffer right now */
#define MAX_SC_STACK_ALLOC 0
#define HAVE_UPSCALER 1

#if defined(CPU_COLDFIRE)
#define SC_NUM 0x80000000U
#define SC_MUL_INIT \
    unsigned long macsr_st = coldfire_get_macsr(); \
    coldfire_set_macsr(EMAC_UNSIGNED);
#define SC_MUL_END coldfire_set_macsr(macsr_st);
#define SC_MUL(x, y) \
({ \
    unsigned long t; \
    asm ("mac.l    %[a], %[b], %%acc0\n\t" \
         "move.l %%accext01, %[t]\n\t" \
         "move.l #0, %%acc0\n\t" \
         : [t] "=r" (t) : [a] "r" (x), [b] "r" (y)); \
    t; \
})
#elif (CONFIG_CPU == SH7034)
/* multiply two unsigned 32 bit values and return the top 32 bit
 * of the 64 bit result */
static inline unsigned sc_mul32(unsigned a, unsigned b)
{
    unsigned r, t1, t2, t3;

    asm (
        "swap.w  %[a], %[t1]     \n" /* t1 = ba */
        "mulu    %[t1], %[b]     \n" /* a * d */
        "swap.w  %[b], %[t3]     \n" /* t3 = dc */
        "sts     macl, %[t2]     \n" /* t2 = a * d */
        "mulu    %[t1], %[t3]    \n" /* a * c */
        "sts     macl, %[r]      \n" /* hi = a * c */
        "mulu    %[a], %[t3]     \n" /* b * c */
        "clrt                    \n"
        "sts     macl, %[t3]     \n" /* t3 = b * c */
        "addc    %[t2], %[t3]    \n" /* t3 += t2, carry -> t2 */
        "movt    %[t2]           \n"
        "mulu    %[a], %[b]      \n" /* b * d */
        "mov     %[t3], %[t1]    \n" /* t2t3 <<= 16 */
        "xtrct   %[t2], %[t1]    \n"
        "mov     %[t1], %[t2]    \n"
        "shll16  %[t3]           \n"
        "sts     macl, %[t1]     \n" /* lo = b * d */
        "clrt                    \n" /* hi.lo += t2t3 */
        "addc    %[t3], %[t1]    \n"
        "addc    %[t2], %[r]     \n"
        : /* outputs */
        [r] "=&r"(r),
        [t1]"=&r"(t1),
        [t2]"=&r"(t2),
        [t3]"=&r"(t3)
        : /* inputs */
        [a] "r"  (a),
        [b] "r"  (b)
    );
    return r;
}
#define SC_MUL(x, y) sc_mul32(x, y)
#define SC_MUL_INIT
#define SC_MUL_END
#endif

#ifndef SC_SHIFT
#define SC_SHIFT 32
#endif

#if SC_SHIFT == 24
#define SC_NUM 0x1000000U
#define SC_FIX 0

#ifndef SC_MUL
#define SC_MUL(x, y) ((x) * (y) >> 24)
#define SC_MUL_INIT
#define SC_MUL_END
#endif

#else /* SC_SHIFT == 32 */
#define SC_NUM 0x80000000U
#define SC_FIX 1

#ifndef SC_MUL
#define SC_MUL(x, y) ((x) * (uint64_t)(y) >> 32)
#define SC_MUL_INIT
#define SC_MUL_END
#endif

#endif

struct img_part {
    int len;
#if !defined(HAVE_LCD_COLOR)    
    uint8_t *buf;
#else
    struct uint8_rgb* buf;
#endif
};

#ifdef HAVE_LCD_COLOR
/* intermediate type used by the scaler for color output. greyscale version
   uses uint32_t
*/
struct uint32_rgb {
    uint32_t r;
    uint32_t g;
    uint32_t b;
};
#endif

/* struct which contains various parameters shared between vertical scaler,
   horizontal scaler, and row output
*/
struct scaler_context {
    uint32_t divisor;
    uint32_t round;
    struct bitmap *bm;
    struct dim *src;
    unsigned char *buf;
    bool dither;
    int len;
    void *args;
    struct img_part* (*store_part)(void *);
    void (*output_row)(uint32_t,void*,struct scaler_context*);
    bool (*h_scaler)(void*,struct scaler_context*, bool);
};

struct custom_format {
    void (*output_row)(uint32_t,void*,struct scaler_context*);
    unsigned int (*get_size)(struct bitmap *bm);
};

struct rowset;
int recalc_dimension(struct dim *dst, struct dim *src);

int resize_on_load(struct bitmap *bm, bool dither,
                   struct dim *src, struct rowset *tmp_row,
                   unsigned char *buf, unsigned int len,
                   const struct custom_format *cformat,
                   struct img_part* (*store_part)(void *args),
                   void *args);

#endif /* _RESIZE_H_ */
