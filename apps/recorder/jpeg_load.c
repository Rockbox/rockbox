/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* Copyright (C) 2009 Andrew Mahone fractional decode, split IDCT - 16-point
*   IDCT based on IJG jpeg-7 pre-release
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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

#include "plugin.h"
#include "debug.h"
#include "jpeg_load.h"
/*#define JPEG_BS_DEBUG*/
//#define ROCKBOX_DEBUG_JPEG
/* for portability of below JPEG code */
#define MEMSET(p,v,c) memset(p,v,c)
#define MEMCPY(d,s,c) memcpy(d,s,c)
#define INLINE static inline
#define ENDIAN_SWAP16(n) n /* only for poor little endian machines */
#ifdef ROCKBOX_DEBUG_JPEG
#define JDEBUGF DEBUGF
#else
#define JDEBUGF(...)
#endif

/**************** begin JPEG code ********************/

#ifdef HAVE_LCD_COLOR
typedef struct uint8_rgb jpeg_pix_t;
#else
typedef uint8_t jpeg_pix_t;
#endif
#define JPEG_IDCT_TRANSPOSE
#define JPEG_PIX_SZ (sizeof(jpeg_pix_t))
#ifdef HAVE_LCD_COLOR
#define COLOR_EXTRA_IDCT_WS 64
#else
#define COLOR_EXTRA_IDCT_WS 0
#endif
#ifdef JPEG_IDCT_TRANSPOSE
#define V_OUT(n) ws2[8*n]
#define V_IN_ST 1
#define TRANSPOSE_EXTRA_IDCT_WS 64
#else
#define V_OUT(n) ws[8*n]
#define V_IN_ST 8
#define TRANSPOSE_EXTRA_IDCT_WS 0
#endif
#define IDCT_WS_SIZE (64 + TRANSPOSE_EXTRA_IDCT_WS + COLOR_EXTRA_IDCT_WS)

/* This can't be in jpeg_load.h because plugin.h includes it, and it conflicts
 * with the definition in jpeg_decoder.h
 */
struct jpeg
{
#ifdef JPEG_FROM_MEM
    unsigned char *data;
#else
    int fd;
    int buf_left;
    int buf_index;
#endif
    unsigned long len;
    unsigned long int bitbuf;
    int bitbuf_bits;
    int marker_ind;
    int marker_val;
    unsigned char marker;
    int x_size, y_size; /* size of image (can be less than block boundary) */
    int x_phys, y_phys; /* physical size, block aligned */
    int x_mbl; /* x dimension of MBL */
    int y_mbl; /* y dimension of MBL */
    int blocks; /* blocks per MB */
    int restart_interval; /* number of MCUs between RSTm markers */
    int restart; /* blocks until next restart marker */
    int mcu_row; /* current row relative to first row of this row of MCUs */
    unsigned char *out_ptr; /* pointer to current row to output */
    int cur_row; /* current row relative to top of image */
    int set_rows;
    int store_pos[4]; /* for Y block ordering */
#ifdef HAVE_LCD_COLOR
    int last_dc_val[3];
    int h_scale[2]; /* horizontal scalefactor = (2**N) / 8 */
    int v_scale[2]; /* same as above, for vertical direction */
    int k_need[2]; /* per component zig-zag index of last needed coefficient */
    int zero_need[2]; /* per compenent number of coefficients to zero */
#else
    int last_dc_val;
    int h_scale[1]; /* horizontal scalefactor = (2**N) / 8 */
    int v_scale[1]; /* same as above, for vertical direction */
    int k_need[1]; /* per component zig-zag index of last needed coefficient */
    int zero_need[1]; /* per compenent number of coefficients to zero */
#endif
    jpeg_pix_t *img_buf;

    int16_t quanttable[4][QUANT_TABLE_LENGTH];/* raw quantization tables 0-3 */

    struct huffman_table hufftable[2]; /* Huffman tables  */
    struct derived_tbl dc_derived_tbls[2]; /* Huffman-LUTs */
    struct derived_tbl ac_derived_tbls[2];

    struct frame_component frameheader[3]; /* Component descriptor */
    struct scan_component scanheader[3]; /* currently not used */

    int mcu_membership[6]; /* info per block */
    int tab_membership[6];
    int subsample_x[3]; /* info per component */
    int subsample_y[3];
    bool resize;
    unsigned char buf[JPEG_READ_BUF_SIZE];
    struct img_part part;
};

#ifdef JPEG_FROM_MEM
static struct jpeg jpeg;
#endif

INLINE unsigned range_limit(int value)
{
#if defined(CPU_COLDFIRE)
    /* Note: Uses knowledge that only the low byte of the result is used */
    asm (
        "cmp.l   #255,%[v]   \n"  /* overflow? */
        "bls.b   1f          \n"  /* no: return value */
        /* yes: set low byte to appropriate boundary */
        "spl.b   %[v]        \n"
    "1:                      \n"
        : /* outputs */
        [v]"+d"(value)
    );
    return value;
#elif defined(CPU_ARM)
    /* Note: Uses knowledge that only the low byte of the result is used */
    asm (
        "cmp     %[v], #255          \n"  /* out of range 0..255? */
        "mvnhi   %[v], %[v], asr #31 \n"  /* yes: set all bits to ~(sign_bit) */
        : /* outputs */
        [v]"+r"(value)
    );
    return value;
#else
    if ((unsigned)value <= 255)
        return value;

    if (value < 0)
        return 0;

    return 255;
#endif
}

INLINE unsigned scale_output(int value)
{
#if defined(CPU_ARM) && ARM_ARCH >= 6
    asm (
        "usat %[v], #8, %[v], asr #18\n"
        : [v] "+r" (value)
    );
    return value;
#else
    return range_limit(value >> 18);
#endif
}

/* IDCT implementation */


#define CONST_BITS 13
#define PASS1_BITS 2


/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
* causing a lot of useless floating-point operations at run time.
* To get around this we use the following pre-calculated constants.
* If you change CONST_BITS you may want to add appropriate values.
* (With a reasonable C compiler, you can just rely on the FIX() macro...)
*/
#define FIX_0_298631336  2446 /* FIX(0.298631336) */
#define FIX_0_390180644  3196 /* FIX(0.390180644) */
#define FIX_0_541196100  4433 /* FIX(0.541196100) */
#define FIX_0_765366865  6270 /* FIX(0.765366865) */
#define FIX_0_899976223  7373 /* FIX(0.899976223) */
#define FIX_1_175875602  9633 /* FIX(1.175875602) */
#define FIX_1_501321110 12299 /* FIX(1.501321110) */
#define FIX_1_847759065 15137 /* FIX(1.847759065) */
#define FIX_1_961570560 16069 /* FIX(1.961570560) */
#define FIX_2_053119869 16819 /* FIX(2.053119869) */
#define FIX_2_562915447 20995 /* FIX(2.562915447) */
#define FIX_3_072711026 25172 /* FIX(3.072711026) */



/* Multiply an long variable by an long constant to yield an long result.
* For 8-bit samples with the recommended scaling, all the variable
* and constant values involved are no more than 16 bits wide, so a
* 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
* For 12-bit samples, a full 32-bit multiplication will be needed.
*/
#define MULTIPLY(var1, var2) ((var1) * (var2))

#if defined(CPU_COLDFIRE) || \
    (defined(CPU_ARM) && ARM_ARCH > 4)
#define MULTIPLY16(var,const)  (((short) (var)) * ((short) (const)))
#else
#define MULTIPLY16 MULTIPLY
#endif

/*
 * Macros for handling fixed-point arithmetic; these are used by many
 * but not all of the DCT/IDCT modules.
 *
 * All values are expected to be of type INT32.
 * Fractional constants are scaled left by CONST_BITS bits.
 * CONST_BITS is defined within each module using these macros,
 * and may differ from one module to the next.
 */
#define ONE ((long)1)
#define CONST_SCALE (ONE << CONST_BITS)

/* Convert a positive real constant to an integer scaled by CONST_SCALE.
 * Caution: some C compilers fail to reduce "FIX(constant)" at compile time,
 * thus causing a lot of useless floating-point operations at run time.
 */
#define FIX(x) ((long) ((x) * CONST_SCALE + 0.5))
#define RIGHT_SHIFT(x,shft)     ((x) >> (shft))

/* Descale and correctly round an int value that's scaled by N bits.
* We assume RIGHT_SHIFT rounds towards minus infinity, so adding
* the fudge factor is correct for either sign of X.
*/
#define DESCALE(x,n) (((x) + (1l << ((n)-1))) >> (n))

#define DS_OUT ((CONST_BITS)+(PASS1_BITS)+3)

/*
 * Conversion of full 0-255 range YCrCb to RGB:
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 * Scaled (yields s15-bit output):
 *   |R|   |128    0  179| |Y       |
 *   |G| = |128  -43  -91| |Cb - 128|
 *   |B|   |128  227    0| |Cr - 128|
 */
#define YFAC            128
#define RVFAC           179
#define GUFAC           (-43)
#define GVFAC           (-91)
#define BUFAC           227
#define COMPONENT_SHIFT  15

#ifndef CPU_ARM
/* horizontal-pass 1-point IDCT */
static void jpeg_idct1h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep)
{
    for (; ws < end; ws += 8)
    {
        *out = range_limit(128 + (int) DESCALE(*ws, 3 + PASS1_BITS));
        out += rowstep;
    }
}

/* vertical-pass 2-point IDCT */
static void jpeg_idct2v(int16_t *ws, int16_t *end)
{
    for (; ws < end; ws++)
    {
        int tmp1 = ws[0*8];
        int tmp2 = ws[1*8];
        ws[0*8] = tmp1 + tmp2;
        ws[1*8] = tmp1 - tmp2;
    }
}

/* horizontal-pass 2-point IDCT */
static void jpeg_idct2h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep)
{
    for (; ws < end; ws += 8, out += rowstep)
    {
        int tmp1 = ws[0] + (ONE << (PASS1_BITS + 2))
                   + (128 << (PASS1_BITS + 3));
        int tmp2 = ws[1];
        out[JPEG_PIX_SZ*0] = range_limit((int) RIGHT_SHIFT(tmp1 + tmp2,
            PASS1_BITS + 3));
        out[JPEG_PIX_SZ*1] = range_limit((int) RIGHT_SHIFT(tmp1 - tmp2,
            PASS1_BITS + 3));
    }
}

/* vertical-pass 4-point IDCT */
static void jpeg_idct4v(int16_t *ws, int16_t *end)
{
    for (; ws < end; ws++)
    {
        int tmp0, tmp2, tmp10, tmp12;
        int z1, z2, z3;
        /* Even part */

        tmp0 = ws[8*0];
        tmp2 = ws[8*2];

        tmp10 = (tmp0 + tmp2) << PASS1_BITS;
        tmp12 = (tmp0 - tmp2) << PASS1_BITS;

        /* Odd part */
        /* Same rotation as in the even part of the 8x8 LL&M IDCT */

        z2 = ws[8*1];
        z3 = ws[8*3];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100) +
            (ONE << (CONST_BITS - PASS1_BITS - 1));
        tmp0 = RIGHT_SHIFT(z1 + MULTIPLY16(z3, - FIX_1_847759065),
            CONST_BITS-PASS1_BITS);
        tmp2 = RIGHT_SHIFT(z1 + MULTIPLY16(z2, FIX_0_765366865),
            CONST_BITS-PASS1_BITS);

        /* Final output stage */
        ws[8*0] = (int) (tmp10 + tmp2);
        ws[8*3] = (int) (tmp10 - tmp2);
        ws[8*1] = (int) (tmp12 + tmp0);
        ws[8*2] = (int) (tmp12 - tmp0);
    }
}

/* horizontal-pass 4-point IDCT */
static void jpeg_idct4h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep)
{
    for (; ws < end; out += rowstep, ws += 8)
    {
        int tmp0, tmp2, tmp10, tmp12;
        int z1, z2, z3;
        /* Even part */

        tmp0 = (int) ws[0] + (ONE << (PASS1_BITS + 2))
               + (128 << (PASS1_BITS + 3));
        tmp2 = (int) ws[2];

        tmp10 = (tmp0 + tmp2) << CONST_BITS;
        tmp12 = (tmp0 - tmp2) << CONST_BITS;

        /* Odd part */
        /* Same rotation as in the even part of the 8x8 LL&M IDCT */

        z2 = (int) ws[1];
        z3 = (int) ws[3];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp0 = z1 - MULTIPLY16(z3, FIX_1_847759065);
        tmp2 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        /* Final output stage */

        out[JPEG_PIX_SZ*0] = range_limit((int) RIGHT_SHIFT(tmp10 + tmp2,
            DS_OUT));
        out[JPEG_PIX_SZ*3] = range_limit((int) RIGHT_SHIFT(tmp10 - tmp2,
            DS_OUT));
        out[JPEG_PIX_SZ*1] = range_limit((int) RIGHT_SHIFT(tmp12 + tmp0,
            DS_OUT));
        out[JPEG_PIX_SZ*2] = range_limit((int) RIGHT_SHIFT(tmp12 - tmp0,
            DS_OUT));
    }
}

/* vertical-pass 8-point IDCT */
static void jpeg_idct8v(int16_t *ws, int16_t *end)
{
    long tmp0, tmp1, tmp2, tmp3;
    long tmp10, tmp11, tmp12, tmp13;
    long z1, z2, z3, z4, z5;
#ifdef JPEG_IDCT_TRANSPOSE
    int16_t *ws2 = ws + 64;
    for (; ws < end; ws += 8, ws2++)
    {
#else
    for (; ws < end; ws++)
    {
#endif
    /* Due to quantization, we will usually find that many of the input
    * coefficients are zero, especially the AC terms.  We can exploit this
    * by short-circuiting the IDCT calculation for any column in which all
    * the AC terms are zero.  In that case each output is equal to the
    * DC coefficient (with scale factor as needed).
    * With typical images and quantization tables, half or more of the
    * column DCT calculations can be simplified this way.
    */
        if ((ws[V_IN_ST*1] | ws[V_IN_ST*2] | ws[V_IN_ST*3]
           | ws[V_IN_ST*4] | ws[V_IN_ST*5] | ws[V_IN_ST*6] | ws[V_IN_ST*7]) == 0)
        {
            /* AC terms all zero */
            int dcval = ws[V_IN_ST*0] << PASS1_BITS;

            V_OUT(0) = V_OUT(1) = V_OUT(2) = V_OUT(3) = V_OUT(4) = V_OUT(5) =
                       V_OUT(6) = V_OUT(7) = dcval;
            continue;
        }

        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = ws[V_IN_ST*2];
        z3 = ws[V_IN_ST*6];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp3 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        z2 = ws[V_IN_ST*0] << CONST_BITS;
        z2 += ONE << (CONST_BITS - PASS1_BITS - 1);
        z3 = ws[V_IN_ST*4] << CONST_BITS;

        tmp0 = (z2 + z3);
        tmp1 = (z2 - z3);

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
           transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively. */

        tmp0 = ws[V_IN_ST*7];
        tmp1 = ws[V_IN_ST*5];
        tmp2 = ws[V_IN_ST*3];
        tmp3 = ws[V_IN_ST*1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY16(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

        tmp0 = MULTIPLY16(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp1 = MULTIPLY16(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp2 = MULTIPLY16(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp3 = MULTIPLY16(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY16(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY16(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY16(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY16(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        V_OUT(0) = (int) RIGHT_SHIFT(tmp10 + tmp3, CONST_BITS-PASS1_BITS);
        V_OUT(7) = (int) RIGHT_SHIFT(tmp10 - tmp3, CONST_BITS-PASS1_BITS);
        V_OUT(1) = (int) RIGHT_SHIFT(tmp11 + tmp2, CONST_BITS-PASS1_BITS);
        V_OUT(6) = (int) RIGHT_SHIFT(tmp11 - tmp2, CONST_BITS-PASS1_BITS);
        V_OUT(2) = (int) RIGHT_SHIFT(tmp12 + tmp1, CONST_BITS-PASS1_BITS);
        V_OUT(5) = (int) RIGHT_SHIFT(tmp12 - tmp1, CONST_BITS-PASS1_BITS);
        V_OUT(3) = (int) RIGHT_SHIFT(tmp13 + tmp0, CONST_BITS-PASS1_BITS);
        V_OUT(4) = (int) RIGHT_SHIFT(tmp13 - tmp0, CONST_BITS-PASS1_BITS);
    }
}

/* horizontal-pass 8-point IDCT */
static void jpeg_idct8h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep)
{
    long tmp0, tmp1, tmp2, tmp3;
    long tmp10, tmp11, tmp12, tmp13;
    long z1, z2, z3, z4, z5;
    for (; ws < end; out += rowstep, ws += 8)
    {
        /* Rows of zeroes can be exploited in the same way as we did with
         * columns. However, the column calculation has created many nonzero AC
         * terms, so the simplification applies less often (typically 5% to 10%
         * of the time). On machines with very fast multiplication, it's
         * possible that the test takes more time than it's worth.  In that
         * case this section may be commented out.
        */

#ifndef NO_ZERO_ROW_TEST
        if ((ws[1] | ws[2] | ws[3]
           | ws[4] | ws[5] | ws[6] | ws[7]) == 0)
        {
            /* AC terms all zero */
            unsigned char dcval = range_limit(128 + (int) DESCALE((long) ws[0],
                PASS1_BITS+3));

            out[JPEG_PIX_SZ*0] = dcval;
            out[JPEG_PIX_SZ*1] = dcval;
            out[JPEG_PIX_SZ*2] = dcval;
            out[JPEG_PIX_SZ*3] = dcval;
            out[JPEG_PIX_SZ*4] = dcval;
            out[JPEG_PIX_SZ*5] = dcval;
            out[JPEG_PIX_SZ*6] = dcval;
            out[JPEG_PIX_SZ*7] = dcval;
            continue;
        }
#endif

        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = (long) ws[2];
        z3 = (long) ws[6];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp3 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        z4 = (long) ws[0] + (ONE << (PASS1_BITS + 2))
             + (128 << (PASS1_BITS + 3));
        z4 <<= CONST_BITS;
        z5 = (long) ws[4] << CONST_BITS;
        tmp0 = z4 + z5;
        tmp1 = z4 - z5;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
        * transpose is its inverse. i0..i3 are y7,y5,y3,y1 respectively. */

        tmp0 = (long) ws[7];
        tmp1 = (long) ws[5];
        tmp2 = (long) ws[3];
        tmp3 = (long) ws[1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY16(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

        tmp0 = MULTIPLY16(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp1 = MULTIPLY16(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp2 = MULTIPLY16(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp3 = MULTIPLY16(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY16(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY16(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY16(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY16(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        out[JPEG_PIX_SZ*0] = range_limit((int) RIGHT_SHIFT(tmp10 + tmp3,
            DS_OUT));
        out[JPEG_PIX_SZ*7] = range_limit((int) RIGHT_SHIFT(tmp10 - tmp3,
            DS_OUT));
        out[JPEG_PIX_SZ*1] = range_limit((int) RIGHT_SHIFT(tmp11 + tmp2,
            DS_OUT));
        out[JPEG_PIX_SZ*6] = range_limit((int) RIGHT_SHIFT(tmp11 - tmp2,
            DS_OUT));
        out[JPEG_PIX_SZ*2] = range_limit((int) RIGHT_SHIFT(tmp12 + tmp1,
            DS_OUT));
        out[JPEG_PIX_SZ*5] = range_limit((int) RIGHT_SHIFT(tmp12 - tmp1,
            DS_OUT));
        out[JPEG_PIX_SZ*3] = range_limit((int) RIGHT_SHIFT(tmp13 + tmp0,
            DS_OUT));
        out[JPEG_PIX_SZ*4] = range_limit((int) RIGHT_SHIFT(tmp13 - tmp0,
            DS_OUT));
    }
}

#else
extern void jpeg_idct1h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep);
extern void jpeg_idct2v(int16_t *ws, int16_t *end);
extern void jpeg_idct2h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep);
extern void jpeg_idct4v(int16_t *ws, int16_t *end);
extern void jpeg_idct4h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep);
extern void jpeg_idct8v(int16_t *ws, int16_t *end);
extern void jpeg_idct8h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep);
#endif

#ifdef HAVE_LCD_COLOR
/* vertical-pass 16-point IDCT */
static void jpeg_idct16v(int16_t *ws, int16_t *end)
{
    long tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
    long tmp20, tmp21, tmp22, tmp23, tmp24, tmp25, tmp26, tmp27;
    long z1, z2, z3, z4;
#ifdef JPEG_IDCT_TRANSPOSE
    int16_t *ws2 = ws + 64;
    for (; ws < end; ws += 8, ws2++)
    {
#else
    for (; ws < end; ws++)
    {
#endif
        /* Even part */

        tmp0 = ws[V_IN_ST*0] << CONST_BITS;
        /* Add fudge factor here for final descale. */
        tmp0 += 1 << (CONST_BITS-PASS1_BITS-1);

        z1 = ws[V_IN_ST*4];
        tmp1 = MULTIPLY(z1, FIX(1.306562965));      /* c4[16] = c2[8] */
        tmp2 = MULTIPLY(z1, FIX_0_541196100);       /* c12[16] = c6[8] */

        tmp10 = tmp0 + tmp1;
        tmp11 = tmp0 - tmp1;
        tmp12 = tmp0 + tmp2;
        tmp13 = tmp0 - tmp2;

        z1 = ws[V_IN_ST*2];
        z2 = ws[V_IN_ST*6];
        z3 = z1 - z2;
        z4 = MULTIPLY(z3, FIX(0.275899379));        /* c14[16] = c7[8] */
        z3 = MULTIPLY(z3, FIX(1.387039845));        /* c2[16] = c1[8] */

        /* (c6+c2)[16] = (c3+c1)[8] */
        tmp0 = z3 + MULTIPLY(z2, FIX_2_562915447);
        /* (c6-c14)[16] = (c3-c7)[8] */
        tmp1 = z4 + MULTIPLY(z1, FIX_0_899976223);
        /* (c2-c10)[16] = (c1-c5)[8] */
        tmp2 = z3 - MULTIPLY(z1, FIX(0.601344887));
        /* (c10-c14)[16] = (c5-c7)[8] */
        tmp3 = z4 - MULTIPLY(z2, FIX(0.509795579));

        tmp20 = tmp10 + tmp0;
        tmp27 = tmp10 - tmp0;
        tmp21 = tmp12 + tmp1;
        tmp26 = tmp12 - tmp1;
        tmp22 = tmp13 + tmp2;
        tmp25 = tmp13 - tmp2;
        tmp23 = tmp11 + tmp3;
        tmp24 = tmp11 - tmp3;

        /* Odd part */

        z1 = ws[V_IN_ST*1];
        z2 = ws[V_IN_ST*3];
        z3 = ws[V_IN_ST*5];
        z4 = ws[V_IN_ST*7];

        tmp11 = z1 + z3;

        tmp1  = MULTIPLY(z1 + z2, FIX(1.353318001));   /* c3 */
        tmp2  = MULTIPLY(tmp11,   FIX(1.247225013));   /* c5 */
        tmp3  = MULTIPLY(z1 + z4, FIX(1.093201867));   /* c7 */
        tmp10 = MULTIPLY(z1 - z4, FIX(0.897167586));   /* c9 */
        tmp11 = MULTIPLY(tmp11,   FIX(0.666655658));   /* c11 */
        tmp12 = MULTIPLY(z1 - z2, FIX(0.410524528));   /* c13 */
        tmp0  = tmp1 + tmp2 + tmp3 -
            MULTIPLY(z1, FIX(2.286341144));        /* c7+c5+c3-c1 */
        tmp13 = tmp10 + tmp11 + tmp12 -
            MULTIPLY(z1, FIX(1.835730603));        /* c9+c11+c13-c15 */
        z1    = MULTIPLY(z2 + z3, FIX(0.138617169));   /* c15 */
        tmp1  += z1 + MULTIPLY(z2, FIX(0.071888074));  /* c9+c11-c3-c15 */
        tmp2  += z1 - MULTIPLY(z3, FIX(1.125726048));  /* c5+c7+c15-c3 */
        z1    = MULTIPLY(z3 - z2, FIX(1.407403738));   /* c1 */
        tmp11 += z1 - MULTIPLY(z3, FIX(0.766367282));  /* c1+c11-c9-c13 */
        tmp12 += z1 + MULTIPLY(z2, FIX(1.971951411));  /* c1+c5+c13-c7 */
        z2    += z4;
        z1    = MULTIPLY(z2, - FIX(0.666655658));      /* -c11 */
        tmp1  += z1;
        tmp3  += z1 + MULTIPLY(z4, FIX(1.065388962));  /* c3+c11+c15-c7 */
        z2    = MULTIPLY(z2, - FIX(1.247225013));      /* -c5 */
        tmp10 += z2 + MULTIPLY(z4, FIX(3.141271809));  /* c1+c5+c9-c13 */
        tmp12 += z2;
        z2    = MULTIPLY(z3 + z4, - FIX(1.353318001)); /* -c3 */
        tmp2  += z2;
        tmp3  += z2;
        z2    = MULTIPLY(z4 - z3, FIX(0.410524528));   /* c13 */
        tmp10 += z2;
        tmp11 += z2;

        /* Final output stage */
        V_OUT(0)  = (int) RIGHT_SHIFT(tmp20 + tmp0,  CONST_BITS-PASS1_BITS);
        V_OUT(15) = (int) RIGHT_SHIFT(tmp20 - tmp0,  CONST_BITS-PASS1_BITS);
        V_OUT(1)  = (int) RIGHT_SHIFT(tmp21 + tmp1,  CONST_BITS-PASS1_BITS);
        V_OUT(14) = (int) RIGHT_SHIFT(tmp21 - tmp1,  CONST_BITS-PASS1_BITS);
        V_OUT(2)  = (int) RIGHT_SHIFT(tmp22 + tmp2,  CONST_BITS-PASS1_BITS);
        V_OUT(13) = (int) RIGHT_SHIFT(tmp22 - tmp2,  CONST_BITS-PASS1_BITS);
        V_OUT(3)  = (int) RIGHT_SHIFT(tmp23 + tmp3,  CONST_BITS-PASS1_BITS);
        V_OUT(12) = (int) RIGHT_SHIFT(tmp23 - tmp3,  CONST_BITS-PASS1_BITS);
        V_OUT(4)  = (int) RIGHT_SHIFT(tmp24 + tmp10, CONST_BITS-PASS1_BITS);
        V_OUT(11) = (int) RIGHT_SHIFT(tmp24 - tmp10, CONST_BITS-PASS1_BITS);
        V_OUT(5)  = (int) RIGHT_SHIFT(tmp25 + tmp11, CONST_BITS-PASS1_BITS);
        V_OUT(10) = (int) RIGHT_SHIFT(tmp25 - tmp11, CONST_BITS-PASS1_BITS);
        V_OUT(6)  = (int) RIGHT_SHIFT(tmp26 + tmp12, CONST_BITS-PASS1_BITS);
        V_OUT(9)  = (int) RIGHT_SHIFT(tmp26 - tmp12, CONST_BITS-PASS1_BITS);
        V_OUT(7)  = (int) RIGHT_SHIFT(tmp27 + tmp13, CONST_BITS-PASS1_BITS);
        V_OUT(8)  = (int) RIGHT_SHIFT(tmp27 - tmp13, CONST_BITS-PASS1_BITS);
    }
}

/* horizontal-pass 16-point IDCT */
static void jpeg_idct16h(int16_t *ws, unsigned char *out, int16_t *end, int rowstep)
{
    long tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
    long tmp20, tmp21, tmp22, tmp23, tmp24, tmp25, tmp26, tmp27;
    long z1, z2, z3, z4;
    for (; ws < end; out += rowstep, ws += 8)
    {
        /* Even part */

        /* Add fudge factor here for final descale. */
        tmp0 = (long) ws[0] + (ONE << (PASS1_BITS+2))
               + (128 << (PASS1_BITS + 3));
        tmp0 <<= CONST_BITS;

        z1 = (long) ws[4];
        tmp1 = MULTIPLY(z1, FIX(1.306562965));      /* c4[16] = c2[8] */
        tmp2 = MULTIPLY(z1, FIX_0_541196100);       /* c12[16] = c6[8] */

        tmp10 = tmp0 + tmp1;
        tmp11 = tmp0 - tmp1;
        tmp12 = tmp0 + tmp2;
        tmp13 = tmp0 - tmp2;

        z1 = (long) ws[2];
        z2 = (long) ws[6];
        z3 = z1 - z2;
        z4 = MULTIPLY(z3, FIX(0.275899379));        /* c14[16] = c7[8] */
        z3 = MULTIPLY(z3, FIX(1.387039845));        /* c2[16] = c1[8] */

        /* (c6+c2)[16] = (c3+c1)[8] */
        tmp0 = z3 + MULTIPLY(z2, FIX_2_562915447);
        /* (c6-c14)[16] = (c3-c7)[8] */
        tmp1 = z4 + MULTIPLY(z1, FIX_0_899976223);
        /* (c2-c10)[16] = (c1-c5)[8] */
        tmp2 = z3 - MULTIPLY(z1, FIX(0.601344887));
        /* (c10-c14)[16] = (c5-c7)[8] */
        tmp3 = z4 - MULTIPLY(z2, FIX(0.509795579));

        tmp20 = tmp10 + tmp0;
        tmp27 = tmp10 - tmp0;
        tmp21 = tmp12 + tmp1;
        tmp26 = tmp12 - tmp1;
        tmp22 = tmp13 + tmp2;
        tmp25 = tmp13 - tmp2;
        tmp23 = tmp11 + tmp3;
        tmp24 = tmp11 - tmp3;

        /* Odd part */

        z1 = (long) ws[1];
        z2 = (long) ws[3];
        z3 = (long) ws[5];
        z4 = (long) ws[7];

        tmp11 = z1 + z3;

        tmp1  = MULTIPLY(z1 + z2, FIX(1.353318001));   /* c3 */
        tmp2  = MULTIPLY(tmp11,   FIX(1.247225013));   /* c5 */
        tmp3  = MULTIPLY(z1 + z4, FIX(1.093201867));   /* c7 */
        tmp10 = MULTIPLY(z1 - z4, FIX(0.897167586));   /* c9 */
        tmp11 = MULTIPLY(tmp11,   FIX(0.666655658));   /* c11 */
        tmp12 = MULTIPLY(z1 - z2, FIX(0.410524528));   /* c13 */
        tmp0  = tmp1 + tmp2 + tmp3 -
            MULTIPLY(z1, FIX(2.286341144));        /* c7+c5+c3-c1 */
        tmp13 = tmp10 + tmp11 + tmp12 -
            MULTIPLY(z1, FIX(1.835730603));        /* c9+c11+c13-c15 */
        z1    = MULTIPLY(z2 + z3, FIX(0.138617169));   /* c15 */
        tmp1  += z1 + MULTIPLY(z2, FIX(0.071888074));  /* c9+c11-c3-c15 */
        tmp2  += z1 - MULTIPLY(z3, FIX(1.125726048));  /* c5+c7+c15-c3 */
        z1    = MULTIPLY(z3 - z2, FIX(1.407403738));   /* c1 */
        tmp11 += z1 - MULTIPLY(z3, FIX(0.766367282));  /* c1+c11-c9-c13 */
        tmp12 += z1 + MULTIPLY(z2, FIX(1.971951411));  /* c1+c5+c13-c7 */
        z2    += z4;
        z1    = MULTIPLY(z2, - FIX(0.666655658));      /* -c11 */
        tmp1  += z1;
        tmp3  += z1 + MULTIPLY(z4, FIX(1.065388962));  /* c3+c11+c15-c7 */
        z2    = MULTIPLY(z2, - FIX(1.247225013));      /* -c5 */
        tmp10 += z2 + MULTIPLY(z4, FIX(3.141271809));  /* c1+c5+c9-c13 */
        tmp12 += z2;
        z2    = MULTIPLY(z3 + z4, - FIX(1.353318001)); /* -c3 */
        tmp2  += z2;
        tmp3  += z2;
        z2    = MULTIPLY(z4 - z3, FIX(0.410524528));   /* c13 */
        tmp10 += z2;
        tmp11 += z2;

        /* Final output stage */

        out[JPEG_PIX_SZ*0]  = scale_output(tmp20 + tmp0);
        out[JPEG_PIX_SZ*15] = scale_output(tmp20 - tmp0);
        out[JPEG_PIX_SZ*1]  = scale_output(tmp21 + tmp1);
        out[JPEG_PIX_SZ*14] = scale_output(tmp21 - tmp1);
        out[JPEG_PIX_SZ*2]  = scale_output(tmp22 + tmp2);
        out[JPEG_PIX_SZ*13] = scale_output(tmp22 - tmp2);
        out[JPEG_PIX_SZ*3]  = scale_output(tmp23 + tmp3);
        out[JPEG_PIX_SZ*12] = scale_output(tmp23 - tmp3);
        out[JPEG_PIX_SZ*4]  = scale_output(tmp24 + tmp10);
        out[JPEG_PIX_SZ*11] = scale_output(tmp24 - tmp10);
        out[JPEG_PIX_SZ*5]  = scale_output(tmp25 + tmp11);
        out[JPEG_PIX_SZ*10] = scale_output(tmp25 - tmp11);
        out[JPEG_PIX_SZ*6]  = scale_output(tmp26 + tmp12);
        out[JPEG_PIX_SZ*9]  = scale_output(tmp26 - tmp12);
        out[JPEG_PIX_SZ*7]  = scale_output(tmp27 + tmp13);
        out[JPEG_PIX_SZ*8]  = scale_output(tmp27 - tmp13);
    }
}
#endif

struct idct_entry {
    int scale;
    void (*v_idct)(int16_t *ws, int16_t *end);
    void (*h_idct)(int16_t *ws, unsigned char *out, int16_t *end, int rowstep);
};

static const struct idct_entry idct_tbl[] = {
    { PASS1_BITS, NULL, jpeg_idct1h },
    { PASS1_BITS, jpeg_idct2v, jpeg_idct2h },
    { 0, jpeg_idct4v, jpeg_idct4h },
    { 0, jpeg_idct8v, jpeg_idct8h },
#ifdef HAVE_LCD_COLOR
    { 0, jpeg_idct16v, jpeg_idct16h },
#endif
};

/* JPEG decoder implementation */

#ifdef JPEG_FROM_MEM
INLINE unsigned char *jpeg_getc(struct jpeg* p_jpeg)
{
    if (LIKELY(p_jpeg->len))
    {
        p_jpeg->len--;
        return p_jpeg->data++;
    } else
        return NULL;
}

INLINE bool skip_bytes(struct jpeg* p_jpeg, int count)
{
    if (p_jpeg->len >= (unsigned)count)
    {
        p_jpeg->len -= count;
        p_jpeg->data += count;
        return true;
    } else {
        p_jpeg->data += p_jpeg->len;
        p_jpeg->len = 0;
        return false;
    }
}

INLINE void jpeg_putc(struct jpeg* p_jpeg)
{
    p_jpeg->len++;
    p_jpeg->data--;
}
#else
INLINE void fill_buf(struct jpeg* p_jpeg)
{
        p_jpeg->buf_left = read(p_jpeg->fd, p_jpeg->buf,
                                (p_jpeg->len >= JPEG_READ_BUF_SIZE)?
                                     JPEG_READ_BUF_SIZE : p_jpeg->len);
        p_jpeg->buf_index = 0;
        if (p_jpeg->buf_left > 0)
            p_jpeg->len -= p_jpeg->buf_left;
}

static unsigned char *jpeg_getc(struct jpeg* p_jpeg)
{
    if (UNLIKELY(p_jpeg->buf_left < 1))
        fill_buf(p_jpeg);
    if (UNLIKELY(p_jpeg->buf_left < 1))
        return NULL;
    p_jpeg->buf_left--;
    return (p_jpeg->buf_index++) + p_jpeg->buf;
}

INLINE bool skip_bytes_seek(struct jpeg* p_jpeg)
{
    if (UNLIKELY(lseek(p_jpeg->fd, -p_jpeg->buf_left, SEEK_CUR) < 0))
        return false;
    p_jpeg->buf_left = 0;
    return true;
}

static bool skip_bytes(struct jpeg* p_jpeg, int count)
{
    p_jpeg->buf_left -= count;
    p_jpeg->buf_index += count;
    return p_jpeg->buf_left >= 0 || skip_bytes_seek(p_jpeg);
}

static void jpeg_putc(struct jpeg* p_jpeg)
{
    p_jpeg->buf_left++;
    p_jpeg->buf_index--;
}
#endif

#define e_skip_bytes(jpeg, count) \
do {\
    if (UNLIKELY(!skip_bytes((jpeg),(count)))) \
        return -1; \
} while (0)

#define e_getc(jpeg, code) \
({ \
    unsigned char *c; \
    if (UNLIKELY(!(c = jpeg_getc(jpeg)))) \
        return (code); \
    *c; \
})

#define d_getc(jpeg, def) \
({ \
    unsigned char *cp = jpeg_getc(jpeg); \
    unsigned char c = LIKELY(cp) ? *cp : (def); \
    c; \
})

/* Preprocess the JPEG JFIF file */
static int process_markers(struct jpeg* p_jpeg)
{
    unsigned char c;
    int marker_size; /* variable length of marker segment */
    int i, j, n;
    int ret = 0; /* returned flags */
    bool done = false;

    while (!done)
    {
        c = e_getc(p_jpeg, -1);
        if (c != 0xFF) /* no marker? */
        {
            JDEBUGF("Non-marker data\n");
            continue; /* discard */
        }

        c = e_getc(p_jpeg, -1);
        JDEBUGF("marker value %X\n",c);
        switch (c)
        {
        case 0xFF: /* Previous FF was fill byte */
            jpeg_putc(p_jpeg); /* This FF could be start of a marker */
            continue;
        case 0x00: /* Zero stuffed byte */
            break; /* discard */

        case 0xC0: /* SOF Huff  - Baseline DCT */
            {
                JDEBUGF("SOF marker ");
                ret |= SOF0;
                marker_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                marker_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                JDEBUGF("len: %d\n", marker_size);
                n = e_getc(p_jpeg, -1); /* sample precision (= 8 or 12) */
                if (n != 8)
                {
                    return(-1); /* Unsupported sample precision */
                }
                p_jpeg->y_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                p_jpeg->y_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                p_jpeg->x_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                p_jpeg->x_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                JDEBUGF("  dimensions: %dx%d\n", p_jpeg->x_size,
                    p_jpeg->y_size);

                n = (marker_size-2-6)/3;
                if (e_getc(p_jpeg, -1) != n || (n != 1 && n != 3))
                {
                    return(-2); /* Unsupported SOF0 component specification */
                }
                for (i=0; i<n; i++)
                {
                    /* Component info */
                    p_jpeg->frameheader[i].ID = e_getc(p_jpeg, -1);
                    p_jpeg->frameheader[i].horizontal_sampling =
                        (c = e_getc(p_jpeg, -1)) >> 4;
                    p_jpeg->frameheader[i].vertical_sampling = c & 0x0F;
                    p_jpeg->frameheader[i].quanttable_select =
                        e_getc(p_jpeg, -1);
                    if (p_jpeg->frameheader[i].horizontal_sampling > 2
                     || p_jpeg->frameheader[i].vertical_sampling > 2)
                    return -3; /* Unsupported SOF0 subsampling */
                }
                p_jpeg->blocks = n;
            }
            break;

        case 0xC1: /* SOF Huff  - Extended sequential DCT*/
        case 0xC2: /* SOF Huff  - Progressive DCT*/
        case 0xC3: /* SOF Huff  - Spatial (sequential) lossless*/
        case 0xC5: /* SOF Huff  - Differential sequential DCT*/
        case 0xC6: /* SOF Huff  - Differential progressive DCT*/
        case 0xC7: /* SOF Huff  - Differential spatial*/
        case 0xC8: /* SOF Arith - Reserved for JPEG extensions*/
        case 0xC9: /* SOF Arith - Extended sequential DCT*/
        case 0xCA: /* SOF Arith - Progressive DCT*/
        case 0xCB: /* SOF Arith - Spatial (sequential) lossless*/
        case 0xCD: /* SOF Arith - Differential sequential DCT*/
        case 0xCE: /* SOF Arith - Differential progressive DCT*/
        case 0xCF: /* SOF Arith - Differential spatial*/
            {
                return (-4); /* other DCT model than baseline not implemented */
            }

        case 0xC4: /* Define Huffman Table(s) */
            {
                ret |= DHT;
                marker_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                marker_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                marker_size -= 2;

                while (marker_size > 17) /* another table */
                {
                    c = e_getc(p_jpeg, -1);
                    marker_size--;
                    int sum = 0;
                    i = c & 0x0F; /* table index */
                    if (i > 1)
                    {
                        return (-5); /* Huffman table index out of range */
                    } else {
                        if (c & 0xF0) /* AC table */
                        {
                            for (j=0; j<16; j++)
                            {
                                p_jpeg->hufftable[i].huffmancodes_ac[j] =
                                    (c = e_getc(p_jpeg, -1));
                                sum += c;
                                marker_size -= 1;
                            }
                            if(16 + sum > AC_LEN)
                                return -10; /* longer than allowed */

                            for (; j < 16 + sum; j++)
                            {
                                p_jpeg->hufftable[i].huffmancodes_ac[j] =
                                    e_getc(p_jpeg, -1);
                                marker_size--;
                            }
                        }
                        else /* DC table */
                        {
                            for (j=0; j<16; j++)
                            {
                                p_jpeg->hufftable[i].huffmancodes_dc[j] =
                                    (c = e_getc(p_jpeg, -1));
                                sum += c;
                                marker_size--;
                            }
                            if(16 + sum > DC_LEN)
                                return -11; /* longer than allowed */

                            for (; j < 16 + sum; j++)
                            {
                                p_jpeg->hufftable[i].huffmancodes_dc[j] =
                                    e_getc(p_jpeg, -1);
                                marker_size--;
                            }
                        }
                    }
                } /* while */
                e_skip_bytes(p_jpeg, marker_size);
            }
            break;

        case 0xCC: /* Define Arithmetic coding conditioning(s) */
            return(-6); /* Arithmetic coding not supported */

        case 0xD8: /* Start of Image */
            JDEBUGF("SOI\n");
            break;
        case 0xD9: /* End of Image */
            JDEBUGF("EOI\n");
            break;
        case 0x01: /* for temp private use arith code */
            JDEBUGF("private\n");
            break; /* skip parameterless marker */


        case 0xDA: /* Start of Scan */
            {
                ret |= SOS;
                marker_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                marker_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                marker_size -= 2;

                n = (marker_size-1-3)/2;
                if (e_getc(p_jpeg, -1) != n || (n != 1 && n != 3))
                {
                    return (-7); /* Unsupported SOS component specification */
                }
                marker_size--;
                for (i=0; i<n; i++)
                {
                    p_jpeg->scanheader[i].ID = e_getc(p_jpeg, -1);
                    p_jpeg->scanheader[i].DC_select = (c = e_getc(p_jpeg, -1))
                        >> 4;
                    p_jpeg->scanheader[i].AC_select = c & 0x0F;
                    marker_size -= 2;
                }
                /* skip spectral information */
                e_skip_bytes(p_jpeg, marker_size);
                done = true;
            }
            break;

        case 0xDB: /* Define quantization Table(s) */
            {
                ret |= DQT;
                marker_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                marker_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                marker_size -= 2;

                n = (marker_size)/(QUANT_TABLE_LENGTH+1); /* # of tables */
                for (i=0; i<n; i++)
                {
                    int id = e_getc(p_jpeg, -1); /* ID */
                    marker_size--;
                    if (id >= 4)
                    {
                        return (-8); /* Unsupported quantization table */
                    }
                    /* Read Quantisation table: */
                    for (j=0; j<QUANT_TABLE_LENGTH; j++)
                    {
                        p_jpeg->quanttable[id][j] = e_getc(p_jpeg, -1);
                        marker_size--;
                    }
                }
                e_skip_bytes(p_jpeg, marker_size);
            }
            break;

        case 0xDD: /* Define Restart Interval */
            {
                marker_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                marker_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                marker_size -= 4;
                /* Highbyte */
                p_jpeg->restart_interval = e_getc(p_jpeg, -1) << 8;
                p_jpeg->restart_interval |= e_getc(p_jpeg, -1); /* Lowbyte */
                e_skip_bytes(p_jpeg, marker_size); /* skip segment */
            }
            break;

        case 0xDC: /* Define Number of Lines */
        case 0xDE: /* Define Hierarchical progression */
        case 0xDF: /* Expand Reference Component(s) */
        case 0xE0: /* Application Field 0*/
        case 0xE1: /* Application Field 1*/
        case 0xE2: /* Application Field 2*/
        case 0xE3: /* Application Field 3*/
        case 0xE4: /* Application Field 4*/
        case 0xE5: /* Application Field 5*/
        case 0xE6: /* Application Field 6*/
        case 0xE7: /* Application Field 7*/
        case 0xE8: /* Application Field 8*/
        case 0xE9: /* Application Field 9*/
        case 0xEA: /* Application Field 10*/
        case 0xEB: /* Application Field 11*/
        case 0xEC: /* Application Field 12*/
        case 0xED: /* Application Field 13*/
        case 0xEE: /* Application Field 14*/
        case 0xEF: /* Application Field 15*/
        case 0xFE: /* Comment */
            {
                marker_size = e_getc(p_jpeg, -1) << 8; /* Highbyte */
                marker_size |= e_getc(p_jpeg, -1); /* Lowbyte */
                marker_size -= 2;
                JDEBUGF("unhandled marker len %d\n", marker_size);
                e_skip_bytes(p_jpeg, marker_size); /* skip segment */
            }
            break;

        case 0xF0: /* Reserved for JPEG extensions */
        case 0xF1: /* Reserved for JPEG extensions */
        case 0xF2: /* Reserved for JPEG extensions */
        case 0xF3: /* Reserved for JPEG extensions */
        case 0xF4: /* Reserved for JPEG extensions */
        case 0xF5: /* Reserved for JPEG extensions */
        case 0xF6: /* Reserved for JPEG extensions */
        case 0xF7: /* Reserved for JPEG extensions */
        case 0xF8: /* Reserved for JPEG extensions */
        case 0xF9: /* Reserved for JPEG extensions */
        case 0xFA: /* Reserved for JPEG extensions */
        case 0xFB: /* Reserved for JPEG extensions */
        case 0xFC: /* Reserved for JPEG extensions */
        case 0xFD: /* Reserved for JPEG extensions */
        case 0x02: /* Reserved */
        default:
            return (-9); /* Unknown marker */
        } /* switch */
    } /* while */

    return (ret); /* return flags with seen markers */
}

static const struct huffman_table luma_table =
{
    {
        0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B
    },
    {
        0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,
        0x01,0x7D,0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,
        0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,
        0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,
        0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,0x2A,0x34,0x35,0x36,0x37,
        0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,
        0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,
        0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
        0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,
        0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,
        0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,
        0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,
        0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA
    }
};

static const struct huffman_table chroma_table =
{
    {
        0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,
        0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B
    },
    {
        0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,
        0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,
        0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,
        0xC1,0x09,0x23,0x33,0x52,0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,
        0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,0x27,0x28,0x29,0x2A,0x35,0x36,
        0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,
        0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
        0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,
        0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,
        0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,
        0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,
        0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,
        0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA
    }
};

static void default_huff_tbl(struct jpeg* p_jpeg)
{

    MEMCPY(&p_jpeg->hufftable[0], &luma_table, sizeof(luma_table));
    MEMCPY(&p_jpeg->hufftable[1], &chroma_table, sizeof(chroma_table));

    return;
}

/* Compute the derived values for a Huffman table */
static void fix_huff_tbl(int* htbl, struct derived_tbl* dtbl)
{
    int p, i, l, si;
    int lookbits, ctr;
    char huffsize[257];
    unsigned int huffcode[257];
    unsigned int code;

    dtbl->pub = htbl; /* fill in back link */

    /* Figure C.1: make table of Huffman code length for each symbol */
    /* Note that this is in code-length order. */

    p = 0;
    for (l = 1; l <= 16; l++)
    {    /* all possible code length */
        for (i = 1; i <= (int) htbl[l-1]; i++)  /* all codes per length */
            huffsize[p++] = (char) l;
    }
    huffsize[p] = 0;

    /* Figure C.2: generate the codes themselves */
    /* Note that this is in code-length order. */

    code = 0;
    si = huffsize[0];
    p = 0;
    while (huffsize[p])
    {
        while (((int) huffsize[p]) == si)
        {
            huffcode[p++] = code;
            code++;
        }
        code <<= 1;
        si++;
    }

    /* Figure F.15: generate decoding tables for bit-sequential decoding */

    p = 0;
    for (l = 1; l <= 16; l++)
    {
        if (htbl[l-1])
        {
            /* huffval[] index of 1st symbol of code length l */
            dtbl->valptr[l] = p;
            dtbl->mincode[l] = huffcode[p]; /* minimum code of length l */
            p += htbl[l-1];
            dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
        }
        else
        {
            dtbl->maxcode[l] = -1;  /* -1 if no codes of this length */
        }
    }
    dtbl->maxcode[17] = 0xFFFFFL; /* ensures huff_DECODE terminates */

    /* Compute lookahead tables to speed up decoding.
    * First we set all the table entries to 0, indicating "too long";
    * then we iterate through the Huffman codes that are short enough and
    * fill in all the entries that correspond to bit sequences starting
    * with that code.
    */

    MEMSET(dtbl->look_nbits, 0, sizeof(dtbl->look_nbits));

    p = 0;
    for (l = 1; l <= HUFF_LOOKAHEAD; l++)
    {
        for (i = 1; i <= (int) htbl[l-1]; i++, p++)
        {
            /* l = current code's length, p = its index in huffcode[] &
             * huffval[]. Generate left-justified code followed by all possible
             * bit sequences
             */
            lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
            for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--)
            {
                dtbl->look_nbits[lookbits] = l;
                dtbl->look_sym[lookbits] = htbl[16+p];
                lookbits++;
            }
        }
    }
}


/* zag[i] is the natural-order position of the i'th element of zigzag order. */
static const unsigned char zag[] =
{
#ifdef JPEG_IDCT_TRANSPOSE
      0,   8,   1,   2,   9,  16,  24,  17,
     10,   3,   4,  11,  18,  25,  32,  40,
     33,  26,  19,  12,   5,   6,  13,  20,
     27,  34,  41,  48,  56,  49,  42,  35,
     28,  21,  14,   7,  15,  22,  29,  36,
     43,  50,  57,  58,  51,  44,  37,  30,
     23,  31,  38,  45,  52,  59,  60,  53,
     46,  39,  47,  54,  61,  62,  55,  63,
#endif
      0,   1,   8,  16,   9,   2,   3,  10,
     17,  24,  32,  25,  18,  11,   4,   5,
     12,  19,  26,  33,  40,  48,  41,  34,
     27,  20,  13,   6,   7,  14,  21,  28,
     35,  42,  49,  56,  57,  50,  43,  36,
     29,  22,  15,  23,  30,  37,  44,  51,
     58,  59,  52,  45,  38,  31,  39,  46,
     53,  60,  61,  54,  47,  55,  62,  63,
};

/* zig[i] is the the zig-zag order position of the i'th element of natural
 * order, reading left-to-right then top-to-bottom.
 */
static const unsigned char zig[] =
{
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

/* Reformat some image header data so that the decoder can use it properly. */
INLINE void fix_headers(struct jpeg* p_jpeg)
{
    int i;

    for (i=0; i<4; i++)
        p_jpeg->store_pos[i] = i; /* default ordering */

    /* assignments for the decoding of blocks */
    if (p_jpeg->frameheader[0].horizontal_sampling == 2
        && p_jpeg->frameheader[0].vertical_sampling == 1)
    {   /* 4:2:2 */
        p_jpeg->blocks = 4;
        p_jpeg->x_mbl = (p_jpeg->x_size+15) / 16;
        p_jpeg->x_phys = p_jpeg->x_mbl * 16;
        p_jpeg->y_mbl = (p_jpeg->y_size+7) / 8;
        p_jpeg->y_phys = p_jpeg->y_mbl * 8;
        p_jpeg->mcu_membership[0] = 0; /* Y1=Y2=0, U=1, V=2 */
        p_jpeg->mcu_membership[1] = 0;
        p_jpeg->mcu_membership[2] = 1;
        p_jpeg->mcu_membership[3] = 2;
        p_jpeg->tab_membership[0] = 0; /* DC, DC, AC, AC */
        p_jpeg->tab_membership[1] = 0;
        p_jpeg->tab_membership[2] = 1;
        p_jpeg->tab_membership[3] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 2;
        p_jpeg->subsample_x[2] = 2;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 1;
        p_jpeg->subsample_y[2] = 1;
    }
    if (p_jpeg->frameheader[0].horizontal_sampling == 1
        && p_jpeg->frameheader[0].vertical_sampling == 2)
    {   /* 4:2:2 vertically subsampled */
        p_jpeg->store_pos[1] = 2; /* block positions are mirrored */
        p_jpeg->store_pos[2] = 1;
        p_jpeg->blocks = 4;
        p_jpeg->x_mbl = (p_jpeg->x_size+7) / 8;
        p_jpeg->x_phys = p_jpeg->x_mbl * 8;
        p_jpeg->y_mbl = (p_jpeg->y_size+15) / 16;
        p_jpeg->y_phys = p_jpeg->y_mbl * 16;
        p_jpeg->mcu_membership[0] = 0; /* Y1=Y2=0, U=1, V=2 */
        p_jpeg->mcu_membership[1] = 0;
        p_jpeg->mcu_membership[2] = 1;
        p_jpeg->mcu_membership[3] = 2;
        p_jpeg->tab_membership[0] = 0; /* DC, DC, AC, AC */
        p_jpeg->tab_membership[1] = 0;
        p_jpeg->tab_membership[2] = 1;
        p_jpeg->tab_membership[3] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 1;
        p_jpeg->subsample_x[2] = 1;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 2;
        p_jpeg->subsample_y[2] = 2;
    }
    else if (p_jpeg->frameheader[0].horizontal_sampling == 2
        && p_jpeg->frameheader[0].vertical_sampling == 2)
    {   /* 4:2:0 */
        p_jpeg->blocks = 6;
        p_jpeg->x_mbl = (p_jpeg->x_size+15) / 16;
        p_jpeg->x_phys = p_jpeg->x_mbl * 16;
        p_jpeg->y_mbl = (p_jpeg->y_size+15) / 16;
        p_jpeg->y_phys = p_jpeg->y_mbl * 16;
        p_jpeg->mcu_membership[0] = 0;
        p_jpeg->mcu_membership[1] = 0;
        p_jpeg->mcu_membership[2] = 0;
        p_jpeg->mcu_membership[3] = 0;
        p_jpeg->mcu_membership[4] = 1;
        p_jpeg->mcu_membership[5] = 2;
        p_jpeg->tab_membership[0] = 0;
        p_jpeg->tab_membership[1] = 0;
        p_jpeg->tab_membership[2] = 0;
        p_jpeg->tab_membership[3] = 0;
        p_jpeg->tab_membership[4] = 1;
        p_jpeg->tab_membership[5] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 2;
        p_jpeg->subsample_x[2] = 2;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 2;
        p_jpeg->subsample_y[2] = 2;
    }
    else if (p_jpeg->frameheader[0].horizontal_sampling == 1
        && p_jpeg->frameheader[0].vertical_sampling == 1)
    {   /* 4:4:4 */
        /* don't overwrite p_jpeg->blocks */
        p_jpeg->x_mbl = (p_jpeg->x_size+7) / 8;
        p_jpeg->x_phys = p_jpeg->x_mbl * 8;
        p_jpeg->y_mbl = (p_jpeg->y_size+7) / 8;
        p_jpeg->y_phys = p_jpeg->y_mbl * 8;
        p_jpeg->mcu_membership[0] = 0;
        p_jpeg->mcu_membership[1] = 1;
        p_jpeg->mcu_membership[2] = 2;
        p_jpeg->tab_membership[0] = 0;
        p_jpeg->tab_membership[1] = 1;
        p_jpeg->tab_membership[2] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 1;
        p_jpeg->subsample_x[2] = 1;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 1;
        p_jpeg->subsample_y[2] = 1;
    }
    else
    {
        /* error */
    }

}

INLINE void fix_huff_tables(struct jpeg *p_jpeg)
{
    fix_huff_tbl(p_jpeg->hufftable[0].huffmancodes_dc,
        &p_jpeg->dc_derived_tbls[0]);
    fix_huff_tbl(p_jpeg->hufftable[0].huffmancodes_ac,
        &p_jpeg->ac_derived_tbls[0]);
    fix_huff_tbl(p_jpeg->hufftable[1].huffmancodes_dc,
        &p_jpeg->dc_derived_tbls[1]);
    fix_huff_tbl(p_jpeg->hufftable[1].huffmancodes_ac,
        &p_jpeg->ac_derived_tbls[1]);
}

/* Because some of the IDCT routines never multiply by any constants, and
 * therefore do not produce shifted output, we add the shift into the
 * quantization table when one of these IDCT routines is used, rather than
 * have the IDCT shift each value it processes.
 */
INLINE void fix_quant_tables(struct jpeg *p_jpeg)
{
    int shift, i, j;

#ifdef HAVE_LCD_COLOR
    const int k = 2;
#else
    const int k = 1;
#endif

    for (i = 0; i < k; i++)
    {
        shift = idct_tbl[p_jpeg->v_scale[i]].scale;
        if (shift)
        {
            for (j = 0; j < 64; j++)
                p_jpeg->quanttable[i][j] <<= shift;
        }
    }
}

/*
* These functions/macros provide the in-line portion of bit fetching.
* Use check_bit_buffer to ensure there are N bits in get_buffer
* before using get_bits, peek_bits, or drop_bits.
*  check_bit_buffer(state,n,action);
*    Ensure there are N bits in get_buffer; if suspend, take action.
*  val = get_bits(n);
*    Fetch next N bits.
*  val = peek_bits(n);
*    Fetch next N bits without removing them from the buffer.
*  drop_bits(n);
*    Discard next N bits.
* The value N should be a simple variable, not an expression, because it
* is evaluated multiple times.
*/

static void fill_bit_buffer(struct jpeg* p_jpeg)
{
    unsigned char byte, marker;

    if (p_jpeg->marker_val)
        p_jpeg->marker_ind += 16;
    byte = d_getc(p_jpeg, 0);
    if (UNLIKELY(byte == 0xFF)) /* legal marker can be byte stuffing or RSTm */
    {   /* simplification: just skip the (one-byte) marker code */
        marker = d_getc(p_jpeg, 0);
        if ((marker & ~7) == 0xD0)
        {
            p_jpeg->marker_val = marker;
            p_jpeg->marker_ind = 8;
        }
    }
    p_jpeg->bitbuf = (p_jpeg->bitbuf << 8) | byte;

    byte = d_getc(p_jpeg, 0);
    if (UNLIKELY(byte == 0xFF)) /* legal marker can be byte stuffing or RSTm */
    {   /* simplification: just skip the (one-byte) marker code */
        marker = d_getc(p_jpeg, 0);
        if ((marker & ~7) == 0xD0)
        {
            p_jpeg->marker_val = marker;
            p_jpeg->marker_ind = 0;
        }
    }
    p_jpeg->bitbuf = (p_jpeg->bitbuf << 8) | byte;
    p_jpeg->bitbuf_bits += 16;
#ifdef JPEG_BS_DEBUG
    DEBUGF("read in: %04X\n", p_jpeg->bitbuf & 0xFFFF);
#endif
}

INLINE void check_bit_buffer(struct jpeg *p_jpeg, int nbits)
{
    if (nbits > p_jpeg->bitbuf_bits)
        fill_bit_buffer(p_jpeg);
}

INLINE int get_bits(struct jpeg *p_jpeg, int nbits)
{
#ifdef JPEG_BS_DEBUG
    if (nbits > p_jpeg->bitbuf_bits)
        DEBUGF("bitbuffer underrun\n");
    int mask = BIT_N(p_jpeg->bitbuf_bits - 1);
    int i;
    DEBUGF("get %d bits: ", nbits);
    for (i = 0; i < nbits; i++)
        DEBUGF("%d",!!(p_jpeg->bitbuf & (mask >>= 1)));
    DEBUGF("\n");
#endif
    return ((int) (p_jpeg->bitbuf >> (p_jpeg->bitbuf_bits -= nbits))) &
        (BIT_N(nbits)-1);
}

INLINE int peek_bits(struct jpeg *p_jpeg, int nbits)
{
#ifdef JPEG_BS_DEBUG
    int mask = BIT_N(p_jpeg->bitbuf_bits - 1);
    int i;
    DEBUGF("peek %d bits: ", nbits);
    for (i = 0; i < nbits; i++)
        DEBUGF("%d",!!(p_jpeg->bitbuf & (mask >>= 1)));
    DEBUGF("\n");
#endif
    return ((int) (p_jpeg->bitbuf >> (p_jpeg->bitbuf_bits - nbits))) &
        (BIT_N(nbits)-1);
}

INLINE void drop_bits(struct jpeg *p_jpeg, int nbits)
{
#ifdef JPEG_BS_DEBUG
    int mask = BIT_N(p_jpeg->bitbuf_bits - 1);
    int i;
    DEBUGF("drop %d bits: ", nbits);
    for (i = 0; i < nbits; i++)
        DEBUGF("%d",!!(p_jpeg->bitbuf & (mask >>= 1)));
    DEBUGF("\n");
#endif
    p_jpeg->bitbuf_bits -= nbits;
}

/* re-synchronize to entropy data (skip restart marker) */
static void search_restart(struct jpeg *p_jpeg)
{
    if (p_jpeg->marker_val)
    {
        p_jpeg->marker_val = 0;
        p_jpeg->bitbuf_bits = p_jpeg->marker_ind;
        p_jpeg->marker_ind = 0;
        return;
    }
    unsigned char byte;
    p_jpeg->bitbuf_bits = 0;
    while ((byte = d_getc(p_jpeg, 0xFF)))
    {
        if (byte == 0xff)
        {
            byte = d_getc(p_jpeg, 0xD0);
            if ((byte & ~7) == 0xD0)
            {
                return;
            }
            else
                jpeg_putc(p_jpeg);
        }
    }
}

/* Figure F.12: extend sign bit. */
/* This saves some code and data size, benchmarks about the same on RAM */
#define HUFF_EXTEND(x,s) \
({ \
    int x__ = x; \
    int s__ = s; \
    x__ & BIT_N(s__- 1) ? x__ : x__ + (-1 << s__) + 1; \
})

/* Decode a single value */
#define huff_decode_dc(p_jpeg, tbl, s, r) \
{ \
    int nb, look; \
\
    check_bit_buffer((p_jpeg), HUFF_LOOKAHEAD); \
    look = peek_bits((p_jpeg), HUFF_LOOKAHEAD); \
    if ((nb = (tbl)->look_nbits[look]) != 0) \
    { \
        drop_bits((p_jpeg), nb); \
        s = (tbl)->look_sym[look]; \
        check_bit_buffer((p_jpeg), s); \
        r = get_bits((p_jpeg), s); \
    } else { \
        /*  slow_DECODE(s, HUFF_LOOKAHEAD+1)) < 0); */ \
        long code; \
        nb=HUFF_LOOKAHEAD+1; \
        check_bit_buffer((p_jpeg), nb); \
        code = get_bits((p_jpeg), nb); \
        while (code > (tbl)->maxcode[nb]) \
        { \
            code <<= 1; \
            check_bit_buffer((p_jpeg), 1); \
            code |= get_bits((p_jpeg), 1); \
            nb++; \
        } \
        if (nb > 16) /* error in Huffman */ \
        { \
            r = 0; s = 0; /* fake a zero, this is most safe */ \
        } else { \
            s = (tbl)->pub[16 + (tbl)->valptr[nb] + \
                ((int) (code - (tbl)->mincode[nb]))]; \
            check_bit_buffer((p_jpeg), s); \
            r = get_bits((p_jpeg), s); \
        } \
    } /* end slow decode */ \
}

#define huff_decode_ac(p_jpeg, tbl, s) \
{ \
    int nb, look; \
\
    check_bit_buffer((p_jpeg), HUFF_LOOKAHEAD); \
    look = peek_bits((p_jpeg), HUFF_LOOKAHEAD); \
    if ((nb = (tbl)->look_nbits[look]) != 0) \
    { \
        drop_bits((p_jpeg), nb); \
        s = (tbl)->look_sym[look]; \
    } else { \
        /*  slow_DECODE(s, HUFF_LOOKAHEAD+1)) < 0); */ \
        long code; \
        nb=HUFF_LOOKAHEAD+1; \
        check_bit_buffer((p_jpeg), nb); \
        code = get_bits((p_jpeg), nb); \
        while (code > (tbl)->maxcode[nb]) \
        { \
            code <<= 1; \
            check_bit_buffer((p_jpeg), 1); \
            code |= get_bits((p_jpeg), 1); \
            nb++; \
        } \
        if (nb > 16) /* error in Huffman */ \
        { \
            s = 0; /* fake a zero, this is most safe */ \
        } else { \
            s = (tbl)->pub[16 + (tbl)->valptr[nb] + \
                ((int) (code - (tbl)->mincode[nb]))]; \
        } \
    } /* end slow decode */ \
}

static struct img_part *store_row_jpeg(void *jpeg_args)
{
    struct jpeg *p_jpeg = (struct jpeg*) jpeg_args;
#ifdef HAVE_LCD_COLOR
    int mcu_hscale = p_jpeg->h_scale[1];
    int mcu_vscale = p_jpeg->v_scale[1];
#else
    int mcu_hscale = (p_jpeg->h_scale[0] +
        p_jpeg->frameheader[0].horizontal_sampling - 1);
    int mcu_vscale = (p_jpeg->v_scale[0] +
        p_jpeg->frameheader[0].vertical_sampling - 1);
#endif
    unsigned int width = p_jpeg->x_mbl << mcu_hscale;
    unsigned int b_width = width * JPEG_PIX_SZ;
    int height = BIT_N(mcu_vscale);
    int x;
    if (!p_jpeg->mcu_row) /* Need to decode a new row of MCUs */
    {
        p_jpeg->out_ptr = (unsigned char *)p_jpeg->img_buf;
        int store_offs[4];
#ifdef HAVE_LCD_COLOR
        unsigned mcu_width = BIT_N(mcu_hscale);
#endif
        int mcu_offset = JPEG_PIX_SZ << mcu_hscale;
        unsigned char *out = p_jpeg->out_ptr;
        store_offs[p_jpeg->store_pos[0]] = 0;
        store_offs[p_jpeg->store_pos[1]] = JPEG_PIX_SZ << p_jpeg->h_scale[0];
        store_offs[p_jpeg->store_pos[2]] = b_width << p_jpeg->v_scale[0];
        store_offs[p_jpeg->store_pos[3]] = store_offs[1] + store_offs[2];
        /* decoded DCT coefficients */
        int16_t block[IDCT_WS_SIZE] __attribute__((aligned(8)));
        for (x = 0; x < p_jpeg->x_mbl; x++)
        {
            int blkn;
            for (blkn = 0; blkn < p_jpeg->blocks; blkn++)
            {
                int ci = p_jpeg->mcu_membership[blkn]; /* component index */
                int ti = p_jpeg->tab_membership[blkn]; /* table index */
#ifdef JPEG_IDCT_TRANSPOSE
                bool transpose = p_jpeg->v_scale[!!ci] > 2;
#endif
                int k = 1; /* coefficient index */
                int s, r; /* huffman values */
                struct derived_tbl* dctbl = &p_jpeg->dc_derived_tbls[ti];
                struct derived_tbl* actbl = &p_jpeg->ac_derived_tbls[ti];

                /* Section F.2.2.1: decode the DC coefficient difference */
                huff_decode_dc(p_jpeg, dctbl, s, r);

#ifndef HAVE_LCD_COLOR
                if (!ci)
#endif
                {
                    s = HUFF_EXTEND(r, s);
#ifdef HAVE_LCD_COLOR
                    p_jpeg->last_dc_val[ci] += s;
                    /* output it (assumes zag[0] = 0) */
                    block[0] = MULTIPLY16(p_jpeg->last_dc_val[ci],
                        p_jpeg->quanttable[!!ci][0]);
#else
                    p_jpeg->last_dc_val += s;
                    /* output it (assumes zag[0] = 0) */
                    block[0] = MULTIPLY16(p_jpeg->last_dc_val,
                        p_jpeg->quanttable[0][0]);
#endif
                    /* coefficient buffer must be cleared */
                    MEMSET(block+1, 0, p_jpeg->zero_need[!!ci] * sizeof(int));
                    /* Section F.2.2.2: decode the AC coefficients */
                    while(true)
                    {
                        huff_decode_ac(p_jpeg, actbl, s);
                        r = s >> 4;
                        s &= 15;
                        k += r;
                        if (s)
                        {
                            check_bit_buffer(p_jpeg, s);
                            if (k >= p_jpeg->k_need[!!ci])
                                goto skip_rest;
                            r = get_bits(p_jpeg, s);
                            r = HUFF_EXTEND(r, s);
                            r = MULTIPLY16(r, p_jpeg->quanttable[!!ci][k]);
#ifdef JPEG_IDCT_TRANSPOSE
                            block[zag[transpose ? k : k + 64]] = r ;
#else
                            block[zag[k]] = r ;
#endif
                        }
                        else
                        {
                            if (r != 15)
                                goto block_end;
                        }
                        if ((++k) & 64)
                            goto block_end;
                    }  /* for k */
                }
                for (; k < 64; k++)
                {
                    huff_decode_ac(p_jpeg, actbl, s);
                    r = s >> 4;
                    s &= 15;

                    if (s)
                    {
                        k += r;
                        check_bit_buffer(p_jpeg, s);
skip_rest:
                        drop_bits(p_jpeg, s);
                    }
                    else
                    {
                        if (r != 15)
                            break;
                        k += r;
                    }
                }  /* for k */
block_end:
#ifndef HAVE_LCD_COLOR
                if (!ci)
#endif
                {
                    int idct_cols = BIT_N(MIN(p_jpeg->h_scale[!!ci], 3));
                    int idct_rows = BIT_N(p_jpeg->v_scale[!!ci]);
                    unsigned char *b_out = out + (ci ? ci : store_offs[blkn]);
                    if (idct_tbl[p_jpeg->v_scale[!!ci]].v_idct)
#ifdef JPEG_IDCT_TRANSPOSE
                        idct_tbl[p_jpeg->v_scale[!!ci]].v_idct(block,
                            transpose ? block + 8 * idct_cols
                                      : block + idct_cols);
                    uint16_t * h_block = transpose ? block + 64 : block;
                    idct_tbl[p_jpeg->h_scale[!!ci]].h_idct(h_block, b_out,
                        h_block + idct_rows * 8, b_width);
#else
                        idct_tbl[p_jpeg->v_scale[!!ci]].v_idct(block,
                            block + idct_cols);
                    idct_tbl[p_jpeg->h_scale[!!ci]].h_idct(block, b_out,
                        block + idct_rows * 8, b_width);
#endif
                }
            } /* for blkn */
            /* don't starve other threads while an MCU row decodes */
            yield();
#ifdef HAVE_LCD_COLOR
            unsigned int xp;
            int yp;
            unsigned char *row = out;
            if (p_jpeg->blocks == 1)
            {
                for (yp = 0; yp < height; yp++, row += b_width)
                {
                    unsigned char *px = row;
                    for (xp = 0; xp < mcu_width; xp++, px += JPEG_PIX_SZ)
                    {
                        px[1] = px[2] = px[0];
                    }
                }
            }
#endif
            out += mcu_offset;
            if (p_jpeg->restart_interval && --p_jpeg->restart == 0)
            {   /* if a restart marker is due: */
                p_jpeg->restart = p_jpeg->restart_interval; /* count again */
                search_restart(p_jpeg); /* align the bitstream */
#ifdef HAVE_LCD_COLOR
                p_jpeg->last_dc_val[0] = p_jpeg->last_dc_val[1] =
                                 p_jpeg->last_dc_val[2] = 0; /* reset decoder */
#else
                p_jpeg->last_dc_val = 0;
#endif
            }
        }
    } /* if !p_jpeg->mcu_row */
    p_jpeg->mcu_row = (p_jpeg->mcu_row + 1) & (height - 1);
    p_jpeg->part.len = width;
    p_jpeg->part.buf = (jpeg_pix_t *)p_jpeg->out_ptr;
    p_jpeg->out_ptr += b_width;
    return &(p_jpeg->part);
}

/******************************************************************************
 * read_jpeg_file()
 *
 * Reads a JPEG file and puts the data in rockbox format in *bitmap.
 *
 *****************************************************************************/
#ifndef JPEG_FROM_MEM
int clip_jpeg_file(const char* filename,
                   int offset,
                   unsigned long jpeg_size,
                   struct bitmap *bm,
                   int maxsize,
                   int format,
                   const struct custom_format *cformat)
{
    int fd, ret;
    fd = open(filename, O_RDONLY);
    JDEBUGF("read_jpeg_file: filename: %s buffer len: %d cformat: %p\n",
        filename, maxsize, cformat);
    /* Exit if file opening failed */
    if (fd < 0) {
        DEBUGF("read_jpeg_file: can't open '%s', rc: %d\n", filename, fd);
        return fd * 10 - 1;
    }
    lseek(fd, offset, SEEK_SET);
    ret = clip_jpeg_fd(fd, jpeg_size, bm, maxsize, format, cformat);
    close(fd);
    return ret;
}

int read_jpeg_file(const char* filename,
                   struct bitmap *bm,
                   int maxsize,
                   int format,
                   const struct custom_format *cformat)
{
    return clip_jpeg_file(filename, 0, 0, bm, maxsize, format, cformat);
}
#endif

static int calc_scale(int in_size, int out_size)
{
    int scale = 0;
    out_size <<= 3;
    for (scale = 0; scale < 3; scale++)
    {
        if (out_size <= in_size)
            break;
        else
            in_size <<= 1;
    }
    return scale;
}

#ifdef JPEG_FROM_MEM
int get_jpeg_dim_mem(unsigned char *data, unsigned long len,
                     struct dim *size)
{
    struct jpeg *p_jpeg = &jpeg;
    memset(p_jpeg, 0, sizeof(struct jpeg));
    p_jpeg->data = data;
    p_jpeg->len = len;
    int status = process_markers(p_jpeg);
    if (status < 0)
        return status;
    if ((status & (DQT | SOF0)) != (DQT | SOF0))
        return -(status * 16);
    size->width = p_jpeg->x_size;
    size->height = p_jpeg->y_size;
    return 0;
}

int decode_jpeg_mem(unsigned char *data,
#else
int clip_jpeg_fd(int fd,
#endif
                 unsigned long len,
                 struct bitmap *bm,
                 int maxsize,
                 int format,
                 const struct custom_format *cformat)
{
    bool resize = false, dither = false;
    struct rowset rset;
    struct dim src_dim;
    int status;
    int bm_size;
#ifdef JPEG_FROM_MEM
    struct jpeg *p_jpeg = &jpeg;
#else
    struct jpeg *p_jpeg = (struct jpeg*)bm->data;
    int tmp_size = maxsize;
    ALIGN_BUFFER(p_jpeg, tmp_size, sizeof(long));
    /* not enough memory for our struct jpeg */
    if ((size_t)tmp_size < sizeof(struct jpeg))
        return -1;
#endif
    memset(p_jpeg, 0, sizeof(struct jpeg));
    p_jpeg->len = len;
#ifdef JPEG_FROM_MEM
    p_jpeg->data = data;
#else
    p_jpeg->fd = fd;
    if (p_jpeg->len == 0)
        p_jpeg->len = filesize(p_jpeg->fd);
#endif
    status = process_markers(p_jpeg);
#ifndef JPEG_FROM_MEM
    JDEBUGF("position in file: %d buffer fill: %d\n",
        (int)lseek(p_jpeg->fd, 0, SEEK_CUR), p_jpeg->buf_left);
#endif
    if (status < 0)
        return status;
    if ((status & (DQT | SOF0)) != (DQT | SOF0))
        return -(status * 16);
    if (!(status & DHT)) /* if no Huffman table present: */
        default_huff_tbl(p_jpeg); /* use default */
    fix_headers(p_jpeg); /* derive Huffman and other lookup-tables */

    /*the dim array in rockbox is limited to 2^15-1 pixels, so we cannot resize
      images larger than this without overflowing */
    if(p_jpeg->x_size > 32767 || p_jpeg->y_size > 32767)
    {
        JDEBUGF("Aborting resize of image > 32767 pixels\n");
        return -1;
    }

    src_dim.width = p_jpeg->x_size;
    src_dim.height = p_jpeg->y_size;
    if (format & FORMAT_RESIZE)
        resize = true;
    if (format & FORMAT_DITHER)
        dither = true;
#ifdef HAVE_LCD_COLOR
    bm->alpha_offset = 0; /* no alpha channel */
#endif
    if (resize) {
        struct dim resize_dim = {
            .width = bm->width,
            .height = bm->height,
        };
        if (format & FORMAT_KEEP_ASPECT)
            recalc_dimension(&resize_dim, &src_dim);
        bm->width = resize_dim.width;
        bm->height = resize_dim.height;
    } else {
        bm->width = p_jpeg->x_size;
        bm->height = p_jpeg->y_size;
    }
    p_jpeg->h_scale[0] = calc_scale(p_jpeg->x_size, bm->width);
    p_jpeg->v_scale[0] = calc_scale(p_jpeg->y_size, bm->height);
    JDEBUGF("luma IDCT size: %dx%d\n", BIT_N(p_jpeg->h_scale[0]),
        BIT_N(p_jpeg->v_scale[0]));
    if ((p_jpeg->x_size << p_jpeg->h_scale[0]) >> 3 == bm->width &&
        (p_jpeg->y_size << p_jpeg->v_scale[0]) >> 3 == bm->height)
        resize = false;
#ifdef HAVE_LCD_COLOR
    p_jpeg->h_scale[1] = p_jpeg->h_scale[0] +
        p_jpeg->frameheader[0].horizontal_sampling - 1;
    p_jpeg->v_scale[1] = p_jpeg->v_scale[0] +
        p_jpeg->frameheader[0].vertical_sampling - 1;
    JDEBUGF("chroma IDCT size: %dx%d\n", BIT_N(p_jpeg->h_scale[1]),
        BIT_N(p_jpeg->v_scale[1]));
#endif
    JDEBUGF("scaling from %dx%d -> %dx%d\n",
        (p_jpeg->x_size << p_jpeg->h_scale[0]) >> 3,
        (p_jpeg->y_size << p_jpeg->v_scale[0]) >> 3,
        bm->width, bm->height);
    fix_quant_tables(p_jpeg);
    int decode_w = BIT_N(p_jpeg->h_scale[0]) - 1;
    int decode_h = BIT_N(p_jpeg->v_scale[0]) - 1;
    src_dim.width = (p_jpeg->x_size << p_jpeg->h_scale[0]) >> 3;
    src_dim.height = (p_jpeg->y_size << p_jpeg->v_scale[0]) >> 3;
#ifdef JPEG_IDCT_TRANSPOSE
    if (p_jpeg->v_scale[0] > 2)
        p_jpeg->zero_need[0] = (decode_w << 3) + decode_h;
    else
#endif
        p_jpeg->zero_need[0] = (decode_h << 3) + decode_w;
    p_jpeg->k_need[0] = zig[(decode_h << 3) + decode_w];
    JDEBUGF("need luma components to %d\n", p_jpeg->k_need[0]);
#ifdef HAVE_LCD_COLOR
    decode_w = BIT_N(MIN(p_jpeg->h_scale[1],3)) - 1;
    decode_h = BIT_N(MIN(p_jpeg->v_scale[1],3)) - 1;
    if (p_jpeg->v_scale[1] > 2)
        p_jpeg->zero_need[1] = (decode_w << 3) + decode_h;
    else
        p_jpeg->zero_need[1] = (decode_h << 3) + decode_w;
    p_jpeg->k_need[1] = zig[(decode_h << 3) + decode_w];
    JDEBUGF("need chroma components to %d\n", p_jpeg->k_need[1]);
#endif
    if (cformat)
        bm_size = cformat->get_size(bm);
    else
        bm_size = BM_SIZE(bm->width,bm->height,FORMAT_NATIVE,false);
    if (bm_size > maxsize)
        return -1;
    char *buf_start = (char *)bm->data + bm_size;
    char *buf_end = (char *)bm->data + maxsize;
    maxsize = buf_end - buf_start;
#ifndef JPEG_FROM_MEM
    ALIGN_BUFFER(buf_start, maxsize, sizeof(long));
    if (maxsize < (int)sizeof(struct jpeg))
        return -1;
    memmove(buf_start, p_jpeg, sizeof(struct jpeg));
    p_jpeg = (struct jpeg *)buf_start;
    buf_start += sizeof(struct jpeg);
    maxsize = buf_end - buf_start;
#endif
    fix_huff_tables(p_jpeg);
#ifdef HAVE_LCD_COLOR
    int decode_buf_size = (p_jpeg->x_mbl << p_jpeg->h_scale[1])
        << p_jpeg->v_scale[1];
#else
    int decode_buf_size = (p_jpeg->x_mbl << p_jpeg->h_scale[0])
        << p_jpeg->v_scale[0];
    decode_buf_size <<= p_jpeg->frameheader[0].horizontal_sampling +
        p_jpeg->frameheader[0].vertical_sampling - 2;
#endif
    decode_buf_size *= JPEG_PIX_SZ;
    JDEBUGF("decode buffer size: %d\n", decode_buf_size);
    p_jpeg->img_buf = (jpeg_pix_t *)buf_start;
    if (buf_end - buf_start < decode_buf_size)
        return -1;
    buf_start += decode_buf_size;
    maxsize = buf_end - buf_start;
    memset(p_jpeg->img_buf, 0, decode_buf_size);
    p_jpeg->mcu_row = 0;
    p_jpeg->restart = p_jpeg->restart_interval;
    rset.rowstart = 0;
    rset.rowstop = bm->height;
    rset.rowstep = 1;
    p_jpeg->resize = resize;
    if (resize)
    {
        if (resize_on_load(bm, dither, &src_dim, &rset, buf_start, maxsize,
            cformat, IF_PIX_FMT(p_jpeg->blocks == 1 ? 0 : 1,) store_row_jpeg,
            p_jpeg))
            return bm_size;
    } else {
        int row;
        struct scaler_context ctx = {
            .bm = bm,
            .dither = dither,
        };
#if LCD_DEPTH > 1
        void (*output_row_8)(uint32_t, void*, struct scaler_context*) =
            output_row_8_native;
#elif defined(PLUGIN)
        void (*output_row_8)(uint32_t, void*, struct scaler_context*) = NULL;
#endif
#if LCD_DEPTH > 1 || defined(PLUGIN)
        if (cformat)
            output_row_8 = cformat->output_row_8;
#endif
        struct img_part *part;
        for (row = 0; row < bm->height; row++)
        {
            part = store_row_jpeg(p_jpeg);
#ifdef HAVE_LCD_COLOR
            if (p_jpeg->blocks > 1)
            {
                struct uint8_rgb *qp = part->buf;
                struct uint8_rgb *end = qp + bm->width;
                uint8_t y, u, v;
                unsigned r, g, b;
                for (; qp < end; qp++)
                {
                    y = qp->blue;
                    u = qp->green;
                    v = qp->red;
                    yuv_to_rgb(y, u, v, &r, &g, &b);
                    qp->red = r;
                    qp->blue = b;
                    qp->green = g;
                }
            }
#endif
            output_row_8(row, part->buf, &ctx);
        }
        return bm_size;
    }
    return 0;
}

#ifndef JPEG_FROM_MEM
int read_jpeg_fd(int fd,
                 struct bitmap *bm,
                 int maxsize,
                 int format,
                 const struct custom_format *cformat)
{
    return clip_jpeg_fd(fd, 0, bm, maxsize, format, cformat);
}
#endif

const size_t JPEG_DECODE_OVERHEAD =
    /* Reserve an arbitrary amount for the decode buffer
     * FIXME: Somebody who knows what they're doing should look at this */
    (38 * 1024)
#ifndef JPEG_FROM_MEM
    /* Unless the struct jpeg is defined statically, we need to allocate
     * it in the bitmap buffer as well */
    + sizeof(struct jpeg)
#endif
    ;

/**************** end JPEG code ********************/
