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

#if defined(CPU_SH)
/* perform 32x32->40 unsigned multiply, round off and return top 8 bits */
static inline uint32_t sc_mul_u32_rnd(uint32_t m, uint32_t n)
{
    unsigned r, t1, t2, t3;
    unsigned h = 1 << 15;
    /* notation:
       m = ab, n = cd
       final result is (((a *c) << 32) + ((b * c + a * d) << 16) + b * d +
            (1 << 31)) >> 32
    */
    asm (
        "swap.w  %[m], %[t1]\n\t" /* t1 = ba */
        "mulu    %[m], %[n]\n\t" /* b * d */
        "swap.w  %[n], %[t3]\n\t" /* t3 = dc */
        "sts     macl, %[r]\n\t" /* r = b * d */
        "mulu    %[m], %[t3]\n\t" /* b * c */
        "shlr16  %[r]\n\t"
        "sts     macl, %[t2]\n\t" /* t2 = b * c */
        "mulu    %[t1], %[t3]\n\t" /* a * c */
        "add     %[t2], %[r]\n\t"
        "sts     macl, %[t3]\n\t" /* t3 = a * c */
        "mulu    %[t1], %[n]\n\t" /* a * d */
        "shll16  %[t3]\n\t"
        "sts     macl, %[t2]\n\t" /* t2 = a * d */
        "add     %[t2], %[r]\n\t"
        "add     %[t3], %[r]\n\t" /* r = ((b * d) >> 16) + (b * c + a * d) +
                                         ((a * c) << 16) */
        "add     %[h], %[r]\n\t" /* round result */
        "shlr16  %[r]\n\t" /* truncate result */
        : /* outputs */
        [r] "=&r"(r),
        [t1]"=&r"(t1),
        [t2]"=&r"(t2),
        [t3]"=&r"(t3)
        : /* inputs */
        [h] "r"  (h),
        [m] "r"  (m),
        [n] "r"  (n)
    );
    return r;
}
#elif defined(TEST_SH_MATH)
static inline uint32_t sc_mul_u32_rnd(uint32_t op1, uint32_t op2)
{
    uint64_t tmp = (uint64_t)op1 * op2;
    tmp += 1LU << 31;
    tmp >>= 32;
    return tmp;
}   
#else
#define SC_OUT(n, c) (((n) + (1 << 23)) >> 24)
#endif
#ifndef SC_OUT
#define SC_OUT(n, c) (sc_mul_u32_rnd(n, (c)->recip))
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
#if defined(CPU_SH) || defined(TEST_SH_MATH)
    uint32_t recip;
#else
    uint32_t h_i_val;
    uint32_t h_o_val;
    uint32_t v_i_val;
    uint32_t v_o_val;
#endif
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

#if defined(HAVE_LCD_COLOR)
#define IF_PIX_FMT(...) __VA_ARGS__
#else
#define IF_PIX_FMT(...)
#endif

struct custom_format {
    void (*output_row_8)(uint32_t,void*, struct scaler_context*);
#if defined(HAVE_LCD_COLOR)
    void (*output_row_32[2])(uint32_t,void*, struct scaler_context*);
#else
    void (*output_row_32)(uint32_t,void*, struct scaler_context*);
#endif
    unsigned int (*get_size)(struct bitmap *bm);
};

struct rowset;

extern const struct custom_format format_native;

int recalc_dimension(struct dim *dst, struct dim *src);

int resize_on_load(struct bitmap *bm, bool dither,
                   struct dim *src, struct rowset *tmp_row,
                   unsigned char *buf, unsigned int len,
                   const struct custom_format *cformat,
                   IF_PIX_FMT(int format_index,)
                   struct img_part* (*store_part)(void *args),
                   void *args);

#endif /* _RESIZE_H_ */
