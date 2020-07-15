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

#define SC_OUT(n, c) (((n) + (1 << 23)) >> 24)
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
struct uint32_argb {
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t a;
};
#endif

/* struct which contains various parameters shared between vertical scaler,
   horizontal scaler, and row output
*/
struct scaler_context {
    uint32_t h_i_val;
    uint32_t h_o_val;
    uint32_t v_i_val;
    uint32_t v_o_val;
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
