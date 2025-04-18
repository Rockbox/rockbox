/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif
     
# include "global.h"

# include <string.h>

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# ifdef HAVE_LIMITS_H
#  include <limits.h>
# else
#  undef  CHAR_BIT
#  define CHAR_BIT  8
# endif

# include "fixed.h"
# include "bit.h"
# include "stream.h"
# include "frame.h"
# include "huffman.h"
# include "layer3.h"

/* depending on the cpu "leftshift32" may be supported or not */
# if defined(CPU_COLDFIRE)
#define MAXLSHIFT 32
#else
#define MAXLSHIFT 31
#endif

/* --- Layer III ----------------------------------------------------------- */

enum {
  count1table_select = 0x01,
  scalefac_scale     = 0x02,
  preflag            = 0x04,
  mixed_block_flag   = 0x08
};

enum {
  I_STEREO  = 0x1,
  MS_STEREO = 0x2
};

struct sideinfo {
  unsigned int main_data_begin;
  unsigned int private_bits;

  unsigned char scfsi[2];

  struct granule {
    struct channel {
      /* from side info */
      unsigned short part2_3_length;
      unsigned short big_values;
      unsigned short global_gain;
      unsigned short scalefac_compress;

      unsigned char flags;
      unsigned char block_type;
      unsigned char table_select[3];
      unsigned char subblock_gain[3];
      unsigned char region0_count;
      unsigned char region1_count;

      /* from main_data */
      unsigned char scalefac[39];       /* scalefac_l and/or scalefac_s */
    } ch[2];
  } gr[2];
};

/*
 * scalefactor bit lengths
 * derived from section 2.4.2.7 of ISO/IEC 11172-3
 */
static
struct {
  unsigned char slen1;
  unsigned char slen2;
} const sflen_table[16] = {
  { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 },
  { 3, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 },
  { 2, 1 }, { 2, 2 }, { 2, 3 }, { 3, 1 },
  { 3, 2 }, { 3, 3 }, { 4, 2 }, { 4, 3 }
};

/*
 * number of LSF scalefactor band values
 * derived from section 2.4.3.2 of ISO/IEC 13818-3
 */
static
unsigned char const nsfb_table[6][3][4] = {
  { {  6,  5,  5, 5 },
    {  9,  9,  9, 9 },
    {  6,  9,  9, 9 } },

  { {  6,  5,  7, 3 },
    {  9,  9, 12, 6 },
    {  6,  9, 12, 6 } },

  { { 11, 10,  0, 0 },
    { 18, 18,  0, 0 },
    { 15, 18,  0, 0 } },

  { {  7,  7,  7, 0 },
    { 12, 12, 12, 0 },
    {  6, 15, 12, 0 } },

  { {  6,  6,  6, 3 },
    { 12,  9,  9, 6 },
    {  6, 12,  9, 6 } },

  { {  8,  8,  5, 0 },
    { 15, 12,  9, 0 },
    {  6, 18,  9, 0 } }
};

/*
 * MPEG-1 scalefactor band widths
 * derived from Table B.8 of ISO/IEC 11172-3
 */
static
unsigned char const sfb_48000_long[] = {
   4,  4,  4,  4,  4,  4,  6,  6,  6,   8,  10,
  12, 16, 18, 22, 28, 34, 40, 46, 54,  54, 192
};

static
unsigned char const sfb_44100_long[] = {
   4,  4,  4,  4,  4,  4,  6,  6,  8,   8,  10,
  12, 16, 20, 24, 28, 34, 42, 50, 54,  76, 158
};

static
unsigned char const sfb_32000_long[] = {
   4,  4,  4,  4,  4,  4,  6,  6,  8,  10,  12,
  16, 20, 24, 30, 38, 46, 56, 68, 84, 102,  26
};

static
unsigned char const sfb_48000_short[] = {
   4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  6,
   6,  6,  6,  6,  6, 10, 10, 10, 12, 12, 12, 14, 14,
  14, 16, 16, 16, 20, 20, 20, 26, 26, 26, 66, 66, 66
};

static
unsigned char const sfb_44100_short[] = {
   4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  6,
   6,  6,  8,  8,  8, 10, 10, 10, 12, 12, 12, 14, 14,
  14, 18, 18, 18, 22, 22, 22, 30, 30, 30, 56, 56, 56
};

static
unsigned char const sfb_32000_short[] = {
   4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  6,
   6,  6,  8,  8,  8, 12, 12, 12, 16, 16, 16, 20, 20,
  20, 26, 26, 26, 34, 34, 34, 42, 42, 42, 12, 12, 12
};

static
unsigned char const sfb_48000_mixed[] = {
  /* long */   4,  4,  4,  4,  4,  4,  6,  6,
  /* short */  4,  4,  4,  6,  6,  6,  6,  6,  6, 10,
              10, 10, 12, 12, 12, 14, 14, 14, 16, 16,
              16, 20, 20, 20, 26, 26, 26, 66, 66, 66
};

static
unsigned char const sfb_44100_mixed[] = {
  /* long */   4,  4,  4,  4,  4,  4,  6,  6,
  /* short */  4,  4,  4,  6,  6,  6,  8,  8,  8, 10,
              10, 10, 12, 12, 12, 14, 14, 14, 18, 18,
              18, 22, 22, 22, 30, 30, 30, 56, 56, 56
};

static
unsigned char const sfb_32000_mixed[] = {
  /* long */   4,  4,  4,  4,  4,  4,  6,  6,
  /* short */  4,  4,  4,  6,  6,  6,  8,  8,  8, 12,
              12, 12, 16, 16, 16, 20, 20, 20, 26, 26,
              26, 34, 34, 34, 42, 42, 42, 12, 12, 12
};

/*
 * MPEG-2 scalefactor band widths
 * derived from Table B.2 of ISO/IEC 13818-3
 */
static
unsigned char const sfb_24000_long[] = {
   6,  6,  6,  6,  6,  6,  8, 10, 12,  14,  16,
  18, 22, 26, 32, 38, 46, 54, 62, 70,  76,  36
};

static
unsigned char const sfb_22050_long[] = {
   6,  6,  6,  6,  6,  6,  8, 10, 12,  14,  16,
  20, 24, 28, 32, 38, 46, 52, 60, 68,  58,  54
};

# define sfb_16000_long  sfb_22050_long

static
unsigned char const sfb_24000_short[] = {
   4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6,  8,
   8,  8, 10, 10, 10, 12, 12, 12, 14, 14, 14, 18, 18,
  18, 24, 24, 24, 32, 32, 32, 44, 44, 44, 12, 12, 12
};

static
unsigned char const sfb_22050_short[] = {
   4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6,  6,
   6,  6,  8,  8,  8, 10, 10, 10, 14, 14, 14, 18, 18,
  18, 26, 26, 26, 32, 32, 32, 42, 42, 42, 18, 18, 18
};

static
unsigned char const sfb_16000_short[] = {
   4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6,  8,
   8,  8, 10, 10, 10, 12, 12, 12, 14, 14, 14, 18, 18,
  18, 24, 24, 24, 30, 30, 30, 40, 40, 40, 18, 18, 18
};

static
unsigned char const sfb_24000_mixed[] = {
  /* long */   6,  6,  6,  6,  6,  6,
  /* short */  6,  6,  6,  8,  8,  8, 10, 10, 10, 12,
              12, 12, 14, 14, 14, 18, 18, 18, 24, 24,
              24, 32, 32, 32, 44, 44, 44, 12, 12, 12
};

static
unsigned char const sfb_22050_mixed[] = {
  /* long */   6,  6,  6,  6,  6,  6,
  /* short */  6,  6,  6,  6,  6,  6,  8,  8,  8, 10,
              10, 10, 14, 14, 14, 18, 18, 18, 26, 26,
              26, 32, 32, 32, 42, 42, 42, 18, 18, 18
};

static
unsigned char const sfb_16000_mixed[] = {
  /* long */   6,  6,  6,  6,  6,  6,
  /* short */  6,  6,  6,  8,  8,  8, 10, 10, 10, 12,
              12, 12, 14, 14, 14, 18, 18, 18, 24, 24,
              24, 30, 30, 30, 40, 40, 40, 18, 18, 18
};

/*
 * MPEG 2.5 scalefactor band widths
 * derived from public sources
 */
# define sfb_12000_long  sfb_16000_long
# define sfb_11025_long  sfb_12000_long

static
unsigned char const sfb_8000_long[] = {
  12, 12, 12, 12, 12, 12, 16, 20, 24,  28,  32,
  40, 48, 56, 64, 76, 90,  2,  2,  2,   2,   2
};

# define sfb_12000_short  sfb_16000_short
# define sfb_11025_short  sfb_12000_short

static
unsigned char const sfb_8000_short[] = {
   8,  8,  8,  8,  8,  8,  8,  8,  8, 12, 12, 12, 16,
  16, 16, 20, 20, 20, 24, 24, 24, 28, 28, 28, 36, 36,
  36,  2,  2,  2,  2,  2,  2,  2,  2,  2, 26, 26, 26
};

# define sfb_12000_mixed  sfb_16000_mixed
# define sfb_11025_mixed  sfb_12000_mixed

/* the 8000 Hz short block scalefactor bands do not break after
   the first 36 frequency lines, so this is probably wrong */
static
unsigned char const sfb_8000_mixed[] = {
  /* long */  12, 12, 12,
  /* short */  4,  4,  4,  8,  8,  8, 12, 12, 12, 16, 16, 16,
              20, 20, 20, 24, 24, 24, 28, 28, 28, 36, 36, 36,
               2,  2,  2,  2,  2,  2,  2,  2,  2, 26, 26, 26
};

static
struct {
  unsigned char const *l;
  unsigned char const *s;
  unsigned char const *m;
} const sfbwidth_table[9] = {
  { sfb_48000_long, sfb_48000_short, sfb_48000_mixed },
  { sfb_44100_long, sfb_44100_short, sfb_44100_mixed },
  { sfb_32000_long, sfb_32000_short, sfb_32000_mixed },
  { sfb_24000_long, sfb_24000_short, sfb_24000_mixed },
  { sfb_22050_long, sfb_22050_short, sfb_22050_mixed },
  { sfb_16000_long, sfb_16000_short, sfb_16000_mixed },
  { sfb_12000_long, sfb_12000_short, sfb_12000_mixed },
  { sfb_11025_long, sfb_11025_short, sfb_11025_mixed },
  {  sfb_8000_long,  sfb_8000_short,  sfb_8000_mixed }
};

/*
 * scalefactor band preemphasis (used only when preflag is set)
 * derived from Table B.6 of ISO/IEC 11172-3
 */
static
unsigned char const pretab[22] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0
};

/*
 * table for requantization
 *
 * rq_table[x].mantissa * 2^(rq_table[x].exponent) = x^(4/3)
 *
 * format rq_table: bit31-27=exponent bit26-0=mantissa
 */
static
uint32_t const rq_table[8207] = {
# include "rq_table.dat"
};

/*
 * fractional powers of two
 * used for requantization and joint stereo decoding
 *
 * root_table[3 + x] = 2^(x/4)
 */
mad_fixed_t const root_table[7] ICONST_ATTR MEM_ALIGN_ATTR = {
  MAD_F(0x09837f05) /* 2^(-3/4) == 0.59460355750136 */,
  MAD_F(0x0b504f33) /* 2^(-2/4) == 0.70710678118655 */,
  MAD_F(0x0d744fcd) /* 2^(-1/4) == 0.84089641525371 */,
  MAD_F(0x10000000) /* 2^( 0/4) == 1.00000000000000 */,
  MAD_F(0x1306fe0a) /* 2^(+1/4) == 1.18920711500272 */,
  MAD_F(0x16a09e66) /* 2^(+2/4) == 1.41421356237310 */,
  MAD_F(0x1ae89f99) /* 2^(+3/4) == 1.68179283050743 */
};

/*
 * coefficients for aliasing reduction
 * derived from Table B.9 of ISO/IEC 11172-3
 *
 *  c[]  = { -0.6, -0.535, -0.33, -0.185, -0.095, -0.041, -0.0142, -0.0037 }
 * cs[i] =    1 / sqrt(1 + c[i]^2)
 * ca[i] = c[i] / sqrt(1 + c[i]^2)
 */
mad_fixed_t const cs[8] ICONST_ATTR MEM_ALIGN_ATTR = {
  +MAD_F(0x0db84a81) /* +0.857492926 */, +MAD_F(0x0e1b9d7f) /* +0.881741997 */,
  +MAD_F(0x0f31adcf) /* +0.949628649 */, +MAD_F(0x0fbba815) /* +0.983314592 */,
  +MAD_F(0x0feda417) /* +0.995517816 */, +MAD_F(0x0ffc8fc8) /* +0.999160558 */,
  +MAD_F(0x0fff964c) /* +0.999899195 */, +MAD_F(0x0ffff8d3) /* +0.999993155 */
};

mad_fixed_t const ca[8] ICONST_ATTR MEM_ALIGN_ATTR = {
  -MAD_F(0x083b5fe7) /* -0.514495755 */, -MAD_F(0x078c36d2) /* -0.471731969 */,
  -MAD_F(0x05039814) /* -0.313377454 */, -MAD_F(0x02e91dd1) /* -0.181913200 */,
  -MAD_F(0x0183603a) /* -0.094574193 */, -MAD_F(0x00a7cb87) /* -0.040965583 */,
  -MAD_F(0x003a2847) /* -0.014198569 */, -MAD_F(0x000f27b4) /* -0.003699975 */
};

/*
 * IMDCT coefficients for short blocks
 * derived from section 2.4.3.4.10.2 of ISO/IEC 11172-3
 *
 * imdct_s[i/even][k] = cos((PI / 24) * (2 *       (i / 2) + 7) * (2 * k + 1))
 * imdct_s[i /odd][k] = cos((PI / 24) * (2 * (6 + (i-1)/2) + 7) * (2 * k + 1))
 */
mad_fixed_t const imdct_s[6][6] ICONST_ATTR MEM_ALIGN_ATTR = {
# include "imdct_s.dat"
};

# if !defined(FPM_ARM)
/*
 * windowing coefficients for long blocks
 * derived from section 2.4.3.4.10.3 of ISO/IEC 11172-3
 *
 * window_l[i] = sin((PI / 36) * (i + 1/2))
 */
static
mad_fixed_t const window_l[36] ICONST_ATTR MEM_ALIGN_ATTR = {
  MAD_F(0x00b2aa3e) /* 0.043619387 */, MAD_F(0x0216a2a2) /* 0.130526192 */,
  MAD_F(0x03768962) /* 0.216439614 */, MAD_F(0x04cfb0e2) /* 0.300705800 */,
  MAD_F(0x061f78aa) /* 0.382683432 */, MAD_F(0x07635284) /* 0.461748613 */,
  MAD_F(0x0898c779) /* 0.537299608 */, MAD_F(0x09bd7ca0) /* 0.608761429 */,
  MAD_F(0x0acf37ad) /* 0.675590208 */, MAD_F(0x0bcbe352) /* 0.737277337 */,
  MAD_F(0x0cb19346) /* 0.793353340 */, MAD_F(0x0d7e8807) /* 0.843391446 */,

  MAD_F(0x0e313245) /* 0.887010833 */, MAD_F(0x0ec835e8) /* 0.923879533 */,
  MAD_F(0x0f426cb5) /* 0.953716951 */, MAD_F(0x0f9ee890) /* 0.976296007 */,
  MAD_F(0x0fdcf549) /* 0.991444861 */, MAD_F(0x0ffc19fd) /* 0.999048222 */,
  MAD_F(0x0ffc19fd) /* 0.999048222 */, MAD_F(0x0fdcf549) /* 0.991444861 */,
  MAD_F(0x0f9ee890) /* 0.976296007 */, MAD_F(0x0f426cb5) /* 0.953716951 */,
  MAD_F(0x0ec835e8) /* 0.923879533 */, MAD_F(0x0e313245) /* 0.887010833 */,

  MAD_F(0x0d7e8807) /* 0.843391446 */, MAD_F(0x0cb19346) /* 0.793353340 */,
  MAD_F(0x0bcbe352) /* 0.737277337 */, MAD_F(0x0acf37ad) /* 0.675590208 */,
  MAD_F(0x09bd7ca0) /* 0.608761429 */, MAD_F(0x0898c779) /* 0.537299608 */,
  MAD_F(0x07635284) /* 0.461748613 */, MAD_F(0x061f78aa) /* 0.382683432 */,
  MAD_F(0x04cfb0e2) /* 0.300705800 */, MAD_F(0x03768962) /* 0.216439614 */,
  MAD_F(0x0216a2a2) /* 0.130526192 */, MAD_F(0x00b2aa3e) /* 0.043619387 */,
};
# endif  /* FPM_ARM */

/*
 * windowing coefficients for short blocks
 * derived from section 2.4.3.4.10.3 of ISO/IEC 11172-3
 *
 * window_s[i] = sin((PI / 12) * (i + 1/2))
 */
mad_fixed_t const window_s[12] ICONST_ATTR MEM_ALIGN_ATTR = {
  MAD_F(0x0216a2a2) /* 0.130526192 */, MAD_F(0x061f78aa) /* 0.382683432 */,
  MAD_F(0x09bd7ca0) /* 0.608761429 */, MAD_F(0x0cb19346) /* 0.793353340 */,
  MAD_F(0x0ec835e8) /* 0.923879533 */, MAD_F(0x0fdcf549) /* 0.991444861 */,
  MAD_F(0x0fdcf549) /* 0.991444861 */, MAD_F(0x0ec835e8) /* 0.923879533 */,
  MAD_F(0x0cb19346) /* 0.793353340 */, MAD_F(0x09bd7ca0) /* 0.608761429 */,
  MAD_F(0x061f78aa) /* 0.382683432 */, MAD_F(0x0216a2a2) /* 0.130526192 */,
};

/*
 * coefficients for intensity stereo processing
 * derived from section 2.4.3.4.9.3 of ISO/IEC 11172-3
 *
 * is_ratio[i] = tan(i * (PI / 12))
 * is_table[i] = is_ratio[i] / (1 + is_ratio[i])
 */
static
mad_fixed_t const is_table[7] = {
  MAD_F(0x00000000) /* 0.000000000 */,
  MAD_F(0x0361962f) /* 0.211324865 */,
  MAD_F(0x05db3d74) /* 0.366025404 */,
  MAD_F(0x08000000) /* 0.500000000 */,
  MAD_F(0x0a24c28c) /* 0.633974596 */,
  MAD_F(0x0c9e69d1) /* 0.788675135 */,
  MAD_F(0x10000000) /* 1.000000000 */
};

/*
 * coefficients for LSF intensity stereo processing
 * derived from section 2.4.3.2 of ISO/IEC 13818-3
 *
 * is_lsf_table[0][i] = (1 / sqrt(sqrt(2)))^(i + 1)
 * is_lsf_table[1][i] = (1 /      sqrt(2)) ^(i + 1)
 */
static
mad_fixed_t const is_lsf_table[2][15] = {
  {
    MAD_F(0x0d744fcd) /* 0.840896415 */,
    MAD_F(0x0b504f33) /* 0.707106781 */,
    MAD_F(0x09837f05) /* 0.594603558 */,
    MAD_F(0x08000000) /* 0.500000000 */,
    MAD_F(0x06ba27e6) /* 0.420448208 */,
    MAD_F(0x05a8279a) /* 0.353553391 */,
    MAD_F(0x04c1bf83) /* 0.297301779 */,
    MAD_F(0x04000000) /* 0.250000000 */,
    MAD_F(0x035d13f3) /* 0.210224104 */,
    MAD_F(0x02d413cd) /* 0.176776695 */,
    MAD_F(0x0260dfc1) /* 0.148650889 */,
    MAD_F(0x02000000) /* 0.125000000 */,
    MAD_F(0x01ae89fa) /* 0.105112052 */,
    MAD_F(0x016a09e6) /* 0.088388348 */,
    MAD_F(0x01306fe1) /* 0.074325445 */
  }, {
    MAD_F(0x0b504f33) /* 0.707106781 */,
    MAD_F(0x08000000) /* 0.500000000 */,
    MAD_F(0x05a8279a) /* 0.353553391 */,
    MAD_F(0x04000000) /* 0.250000000 */,
    MAD_F(0x02d413cd) /* 0.176776695 */,
    MAD_F(0x02000000) /* 0.125000000 */,
    MAD_F(0x016a09e6) /* 0.088388348 */,
    MAD_F(0x01000000) /* 0.062500000 */,
    MAD_F(0x00b504f3) /* 0.044194174 */,
    MAD_F(0x00800000) /* 0.031250000 */,
    MAD_F(0x005a827a) /* 0.022097087 */,
    MAD_F(0x00400000) /* 0.015625000 */,
    MAD_F(0x002d413d) /* 0.011048543 */,
    MAD_F(0x00200000) /* 0.007812500 */,
    MAD_F(0x0016a09e) /* 0.005524272 */
  }
};

/*
 * NAME:        III_sideinfo()
 * DESCRIPTION: decode frame side information from a bitstream
 */
static
enum mad_error III_sideinfo(struct mad_bitptr *ptr, unsigned int nch,
                            int lsf, struct sideinfo *si,
                            unsigned int *data_bitlen,
                            unsigned int *priv_bitlen)
{
  unsigned int ngr, gr, ch, i;
  enum mad_error result = MAD_ERROR_NONE;

  *data_bitlen = 0;
  *priv_bitlen = lsf ? ((nch == 1) ? 1 : 2) : ((nch == 1) ? 5 : 3);

  si->main_data_begin = mad_bit_read(ptr, lsf ? 8 : 9);
  si->private_bits    = mad_bit_read(ptr, *priv_bitlen);

  ngr = 1;
  if (!lsf) {
    ngr = 2;

    for (ch = 0; ch < nch; ++ch)
      si->scfsi[ch] = mad_bit_read(ptr, 4);
  }

  for (gr = 0; gr < ngr; ++gr) {
    struct granule *granule = &si->gr[gr];

    for (ch = 0; ch < nch; ++ch) {
      struct channel *channel = &granule->ch[ch];

      channel->part2_3_length    = mad_bit_read(ptr, 12);
      channel->big_values        = mad_bit_read(ptr, 9);
      channel->global_gain       = mad_bit_read(ptr, 8);
      channel->scalefac_compress = mad_bit_read(ptr, lsf ? 9 : 4);

      *data_bitlen += channel->part2_3_length;

      if (channel->big_values > 288 && result == 0)
        result = MAD_ERROR_BADBIGVALUES;

      channel->flags = 0;

      /* window_switching_flag */
      if (mad_bit_read(ptr, 1)) {
        channel->block_type = mad_bit_read(ptr, 2);

        if (channel->block_type == 0 && result == 0)
          result = MAD_ERROR_BADBLOCKTYPE;

        if (!lsf && channel->block_type == 2 && si->scfsi[ch] && result == 0)
          result = MAD_ERROR_BADSCFSI;

        channel->region0_count = 7;
        channel->region1_count = 36;

        if (mad_bit_read(ptr, 1))
          channel->flags |= mixed_block_flag;
        else if (channel->block_type == 2)
          channel->region0_count = 8;

        for (i = 0; i < 2; ++i)
          channel->table_select[i] = mad_bit_read(ptr, 5);

# if defined(DEBUG)
        channel->table_select[2] = 4;  /* not used */
# endif

        for (i = 0; i < 3; ++i)
          channel->subblock_gain[i] = mad_bit_read(ptr, 3);
      }
      else {
        channel->block_type = 0;

        for (i = 0; i < 3; ++i)
          channel->table_select[i] = mad_bit_read(ptr, 5);

        channel->region0_count = mad_bit_read(ptr, 4);
        channel->region1_count = mad_bit_read(ptr, 3);
      }

      /* [preflag,] scalefac_scale, count1table_select */
      channel->flags |= mad_bit_read(ptr, lsf ? 2 : 3);
    }
  }

  return result;
}

/*
 * NAME:        III_scalefactors_lsf()
 * DESCRIPTION: decode channel scalefactors for LSF from a bitstream
 */
static
unsigned int III_scalefactors_lsf(struct mad_bitptr *ptr,
                                  struct channel *channel,
                                  struct channel *gr1ch, int mode_extension)
{
  struct mad_bitptr start;
  unsigned int scalefac_compress, index, slen[4], part, n, i;
  unsigned char const *nsfb;

  start = *ptr;

  scalefac_compress = channel->scalefac_compress;
  index = (channel->block_type == 2) ?
    ((channel->flags & mixed_block_flag) ? 2 : 1) : 0;

  if (!((mode_extension & I_STEREO) && gr1ch)) {
    if (scalefac_compress < 400) {
      slen[0] = (scalefac_compress >> 4) / 5;
      slen[1] = (scalefac_compress >> 4) % 5;
      slen[2] = (scalefac_compress % 16) >> 2;
      slen[3] =  scalefac_compress %  4;

      nsfb = nsfb_table[0][index];
    }
    else if (scalefac_compress < 500) {
      scalefac_compress -= 400;

      slen[0] = (scalefac_compress >> 2) / 5;
      slen[1] = (scalefac_compress >> 2) % 5;
      slen[2] =  scalefac_compress %  4;
      slen[3] = 0;

      nsfb = nsfb_table[1][index];
    }
    else {
      scalefac_compress -= 500;

      slen[0] = scalefac_compress / 3;
      slen[1] = scalefac_compress % 3;
      slen[2] = 0;
      slen[3] = 0;

      channel->flags |= preflag;

      nsfb = nsfb_table[2][index];
    }

    n = 0;
    for (part = 0; part < 4; ++part) {
      for (i = 0; i < nsfb[part]; ++i)
        channel->scalefac[n++] = mad_bit_read(ptr, slen[part]);
    }

    while (n < 39)
      channel->scalefac[n++] = 0;
  }
  else {  /* (mode_extension & I_STEREO) && gr1ch (i.e. ch == 1) */
    scalefac_compress >>= 1;

    if (scalefac_compress < 180) {
      slen[0] =  scalefac_compress / 36;
      slen[1] = (scalefac_compress % 36) / 6;
      slen[2] = (scalefac_compress % 36) % 6;
      slen[3] = 0;

      nsfb = nsfb_table[3][index];
    }
    else if (scalefac_compress < 244) {
      scalefac_compress -= 180;

      slen[0] = (scalefac_compress % 64) >> 4;
      slen[1] = (scalefac_compress % 16) >> 2;
      slen[2] =  scalefac_compress %  4;
      slen[3] = 0;

      nsfb = nsfb_table[4][index];
    }
    else {
      scalefac_compress -= 244;

      slen[0] = scalefac_compress / 3;
      slen[1] = scalefac_compress % 3;
      slen[2] = 0;
      slen[3] = 0;

      nsfb = nsfb_table[5][index];
    }

    n = 0;
    for (part = 0; part < 4; ++part) {
      unsigned int max, is_pos;

      max = (1 << slen[part]) - 1;

      for (i = 0; i < nsfb[part]; ++i) {
        is_pos = mad_bit_read(ptr, slen[part]);

        channel->scalefac[n] = is_pos;
        gr1ch->scalefac[n++] = (is_pos == max);
      }
    }

    while (n < 39) {
      channel->scalefac[n] = 0;
      gr1ch->scalefac[n++] = 0;  /* apparently not illegal */
    }
  }

  return mad_bit_length(&start, ptr);
}

/*
 * NAME:        III_scalefactors()
 * DESCRIPTION: decode channel scalefactors of one granule from a bitstream
 */
static
unsigned int III_scalefactors(struct mad_bitptr *ptr, struct channel *channel,
                              struct channel const *gr0ch, unsigned int scfsi)
{
  struct mad_bitptr start;
  unsigned int slen1, slen2, sfbi;

  start = *ptr;

  slen1 = sflen_table[channel->scalefac_compress].slen1;
  slen2 = sflen_table[channel->scalefac_compress].slen2;

  if (channel->block_type == 2) {
    unsigned int nsfb;

    sfbi = 0;

    nsfb = (channel->flags & mixed_block_flag) ? 8 + 3 * 3 : 6 * 3;
    while (nsfb--)
      channel->scalefac[sfbi++] = mad_bit_read(ptr, slen1);

    nsfb = 6 * 3;
    while (nsfb--)
      channel->scalefac[sfbi++] = mad_bit_read(ptr, slen2);

    nsfb = 1 * 3;
    while (nsfb--)
      channel->scalefac[sfbi++] = 0;
  }
  else {  /* channel->block_type != 2 */
    if (scfsi & 0x8) {
      for (sfbi = 0; sfbi < 6; ++sfbi)
        channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
    }
    else {
      for (sfbi = 0; sfbi < 6; ++sfbi)
        channel->scalefac[sfbi] = mad_bit_read(ptr, slen1);
    }

    if (scfsi & 0x4) {
      for (sfbi = 6; sfbi < 11; ++sfbi)
        channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
    }
    else {
      for (sfbi = 6; sfbi < 11; ++sfbi)
        channel->scalefac[sfbi] = mad_bit_read(ptr, slen1);
    }

    if (scfsi & 0x2) {
      for (sfbi = 11; sfbi < 16; ++sfbi)
        channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
    }
    else {
      for (sfbi = 11; sfbi < 16; ++sfbi)
        channel->scalefac[sfbi] = mad_bit_read(ptr, slen2);
    }

    if (scfsi & 0x1) {
      for (sfbi = 16; sfbi < 21; ++sfbi)
        channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
    }
    else {
      for (sfbi = 16; sfbi < 21; ++sfbi)
        channel->scalefac[sfbi] = mad_bit_read(ptr, slen2);
    }

    channel->scalefac[21] = 0;
  }

  return mad_bit_length(&start, ptr);
}

/*
 * The Layer III formula for requantization and scaling is defined by
 * section 2.4.3.4.7.1 of ISO/IEC 11172-3, as follows:
 *
 *   long blocks:
 *   xr[i] = sign(is[i]) * abs(is[i])^(4/3) *
 *           2^((1/4) * (global_gain - 210)) *
 *           2^-(scalefac_multiplier *
 *               (scalefac_l[sfb] + preflag * pretab[sfb]))
 *
 *   short blocks:
 *   xr[i] = sign(is[i]) * abs(is[i])^(4/3) *
 *           2^((1/4) * (global_gain - 210 - 8 * subblock_gain[w])) *
 *           2^-(scalefac_multiplier * scalefac_s[sfb][w])
 *
 *   where:
 *   scalefac_multiplier = (scalefac_scale + 1) / 2
 *
 * The routines III_exponents() and III_requantize() facilitate this
 * calculation.
 */

/*
 * NAME:        III_exponents()
 * DESCRIPTION: calculate scalefactor exponents
 */
static
void III_exponents(struct channel const *channel,
                   unsigned char const *sfbwidth, signed int exponents[39])
{
  signed int gain;
  unsigned int scalefac_multiplier, sfbi;

  gain = (signed int) channel->global_gain - 210;
  scalefac_multiplier = (channel->flags & scalefac_scale) ? 2 : 1;

  if (channel->block_type == 2) {
    unsigned int l;
    signed int gain0, gain1, gain2;

    sfbi = l = 0;

    if (channel->flags & mixed_block_flag) {
      unsigned int premask;

      premask = (channel->flags & preflag) ? ~0 : 0;

      /* long block subbands 0-1 */

      while (l < 36) {
        exponents[sfbi] = gain -
          (signed int) ((channel->scalefac[sfbi] + (pretab[sfbi] & premask)) <<
                        scalefac_multiplier);

        l += sfbwidth[sfbi++];
      }
    }

    /* this is probably wrong for 8000 Hz short/mixed blocks */

    gain0 = gain - 8 * (signed int) channel->subblock_gain[0];
    gain1 = gain - 8 * (signed int) channel->subblock_gain[1];
    gain2 = gain - 8 * (signed int) channel->subblock_gain[2];

    while (l < 576) {
      exponents[sfbi + 0] = gain0 -
        (signed int) (channel->scalefac[sfbi + 0] << scalefac_multiplier);
      exponents[sfbi + 1] = gain1 -
        (signed int) (channel->scalefac[sfbi + 1] << scalefac_multiplier);
      exponents[sfbi + 2] = gain2 -
        (signed int) (channel->scalefac[sfbi + 2] << scalefac_multiplier);
      unsigned int w = sfbwidth[sfbi];
      l    += 3 * w;
      sfbi += 3;
    }
  }
  else {  /* channel->block_type != 2 */
    if (channel->flags & preflag) {
      for (sfbi = 0; sfbi < 22; ++sfbi) {
        exponents[sfbi] = gain -
          (signed int) ((channel->scalefac[sfbi] + pretab[sfbi]) <<
                        scalefac_multiplier);
      }
    }
    else {
      for (sfbi = 0; sfbi < 22; ++sfbi) {
        exponents[sfbi] = gain -
          (signed int) (channel->scalefac[sfbi] << scalefac_multiplier);
      }
    }
  }
}

/*
 * NAME:        III_requantize()
 * DESCRIPTION: requantize one (positive) value
 */
static
mad_fixed_t III_requantize(unsigned int value, signed int exp)
{
  mad_fixed_t requantized;
  signed int frac;
  uint32_t power;

  frac = exp % 4;  /* assumes sign(frac) == sign(exp) */
  exp /= 4;

  /* format rq_table: bit31-27=exponent bit26-0=mantissa */
  power = rq_table[value];
  requantized = power & 0x07ffffff;
  exp += power >> 27;

  if (exp < 0) {
    if (-exp >= (signed int) (sizeof(mad_fixed_t) * CHAR_BIT)) {
      /* underflow */
      requantized = 0;
    }
    else {
      requantized += 1L << (-exp - 1);
      requantized >>= -exp;
    }
  }
  else {
    if (exp >= 5) {
      /* overflow */
# if defined(DEBUG)
      fprintf(stderr, "requantize overflow (%f * 2^%d)\n",
              mad_f_todouble(requantized), exp);
# endif
      requantized = MAD_F_MAX;
    }
    else
      requantized <<= exp;
  }

  return frac ? mad_f_mul(requantized, root_table[3 + frac]) : requantized;
}

/* we must take care that sz >= bits and sz < sizeof(long) lest bits == 0 */
# if defined(CPU_ARM_CLASSIC)
# define MASK(cache, sz, bits) \
    ({ unsigned long res; \
       asm ("mov  %0, #1\n\t" \
            "rsb  %0, %0, %0, lsl %3\n\t" \
            "and  %0, %0, %1, lsr %2" \
           : "=&r" (res) : "r" (cache), "r" ((sz) - (bits)), "r" (bits)); \
       res; \
    })
#else
# define MASK(cache, sz, bits)  \
    (((cache) >> ((sz) - (bits))) & ((1 << (bits)) - 1))
#endif
# define MASK1BIT(cache, sz)  \
    ((cache) & (1 << ((sz) - 1)))

/*
 * NAME:        III_huffdecode()
 * DESCRIPTION: decode Huffman code words of one channel of one granule
 */
static
enum mad_error III_huffdecode(struct mad_bitptr *ptr, mad_fixed_t xrarr[576],
                              struct channel *channel,
                              unsigned char const *sfbwidth,
                              unsigned int part2_length)
{
  unsigned int bits;
  signed int exponents[39], exp;
  signed int const *expptr;
  struct mad_bitptr peek;
  signed int bits_left, cachesz;
  register mad_fixed_t *xr;
  mad_fixed_t const *sfbound;
  register unsigned long bitcache;

  bits_left = (signed) channel->part2_3_length - (signed) part2_length;
  if (bits_left < 0)
    return MAD_ERROR_BADPART3LEN;

  III_exponents(channel, sfbwidth, exponents);

  peek    = *ptr;
  cachesz = 0;
  sfbound = xr = xrarr;
  mad_bit_skip(ptr, bits_left);

  /* big_values */
  {
    int                    region;
    struct hufftable const *entry;
    union huffpair const   *table;
    unsigned int           linbits, startbits, rcount;
    mad_fixed_t            reqcache[16];
    mad_fixed_t const      *xr_end, *xr_big_val;

    rcount     = 1;
    expptr     = &exponents[0];
    region     = -1;
    exp        = 0x3210; /* start value */
    bitcache   = 0;
    linbits    = startbits = 0; 
    table      = NULL;
    xr_big_val = xr + 2 * channel->big_values;

    while(xr < xr_big_val)
    {
      sfbound += *sfbwidth++;
      xr_end = sfbound > xr_big_val ? xr_big_val : sfbound;

      /* change table if region boundary */
      if(--rcount == 0)
      {
        if(exp == 0x3210)
            rcount = channel->region0_count + 1;
        else
          if(region == 0)
            rcount = channel->region1_count + 1;
          else
            rcount = 0;  /* all remaining */

        entry     = &mad_huff_pair_table[channel->table_select[++region]];
        table     = entry->table;
        linbits   = entry->linbits;
        startbits = entry->startbits;

        if(table == 0)
          return MAD_ERROR_BADHUFFTABLE;
      }

      if(exp != *expptr)
      {
        exp = *expptr;
        /* clear cache */
        memset(reqcache, 0, sizeof(reqcache));
      }

      ++expptr;

      if(linbits)
      {
        for( ; xr<xr_end; xr+=2)
        {
          union huffpair const *pair;
          register mad_fixed_t requantized;
          unsigned int clumpsz, value;

          /* maxhuffcode(hufftab16,hufftab24)=17bit + sign(x,y)=2bit */
          if(cachesz < 19)
          {
            if(cachesz < 0)
              return MAD_ERROR_BADHUFFDATA;  /* cache underrun */

            bits     = MAXLSHIFT - cachesz;
            bitcache = (bitcache << bits) | mad_bit_read(&peek, bits);
            cachesz += bits;
          }

          /* hcod (0..19) */
          clumpsz = startbits;
          pair    = &table[MASK(bitcache, cachesz, clumpsz)];

          while(!pair->final)
          {
            cachesz -= clumpsz;
            clumpsz = pair->ptr.bits;
            pair    = &table[pair->ptr.offset + MASK(bitcache, cachesz, clumpsz)];
          }

          cachesz -= pair->value.hlen;

          /* x (0..14) */
          value = pair->value.x;
          if(value == 0)
            xr[0] = 0;
          else
          {
            if(value == 15)
            {
              /* maxlinbits=13bit + sign(x,y)=2bit */
              if(cachesz < 15)
              {
                if(cachesz < 0)
                  return MAD_ERROR_BADHUFFDATA;  /* cache underrun */

                bits     = MAXLSHIFT - cachesz;
                bitcache = (bitcache << bits) | mad_bit_read(&peek, bits);
                cachesz += bits;
              }

              requantized = III_requantize(15+MASK(bitcache, cachesz, linbits), exp);
              cachesz    -= linbits;
            }
            else
            {
              if(reqcache[value])
                requantized = reqcache[value];
              else
                requantized = reqcache[value] = III_requantize(value, exp);
            }

            xr[0] = MASK1BIT(bitcache, cachesz--) ? -requantized : requantized;
          }

          /* y (0..14) */
          value = pair->value.y;
          if(value == 0)
            xr[1] = 0;
          else
          {
            if(value == 15)
            {
              /* maxlinbits=13bit + sign(y)=1bit */
              if(cachesz < 14)
              {
                if(cachesz < 0)
                  return MAD_ERROR_BADHUFFDATA;  /* cache underrun */

                bits     = MAXLSHIFT - cachesz;
                bitcache = (bitcache << bits) | mad_bit_read(&peek, bits);
                cachesz += bits;
              }

              requantized = III_requantize(15+MASK(bitcache, cachesz, linbits), exp);
              cachesz -= linbits;
            }
            else
            {
              if(reqcache[value])
                requantized = reqcache[value];
              else
                requantized = reqcache[value] = III_requantize(value, exp);
            }
            xr[1] = MASK1BIT(bitcache, cachesz--) ? -requantized : requantized;
          }
        }
      }
      else
      {
        for( ; xr<xr_end; xr+=2)
        {
          union huffpair const *pair;
          register mad_fixed_t requantized;
          unsigned int clumpsz, value;

          /* maxlookup=4bit + sign(x,y)=2bit */
          if(cachesz < 6)
          {
            if(cachesz < 0)
              return MAD_ERROR_BADHUFFDATA;  /* cache underrun */

            bits     = MAXLSHIFT - cachesz;
            bitcache = (bitcache << bits) | mad_bit_read(&peek, bits);
            cachesz += bits;
          }

          /* hcod (0..19) */
          clumpsz = startbits;
          pair    = &table[MASK(bitcache, cachesz, clumpsz)];

          while(!pair->final)
          {
            cachesz -= clumpsz;

            /* maxlookup=4bit + sign(x,y)=2bit */
            if(cachesz < 6)
            {
              if(cachesz < 0)
                return MAD_ERROR_BADHUFFDATA;  /* cache underrun */

              bits     = MAXLSHIFT - cachesz;
              bitcache = (bitcache << bits) | mad_bit_read(&peek, bits);
              cachesz += bits;
            }

            clumpsz = pair->ptr.bits;
            pair    = &table[pair->ptr.offset + MASK(bitcache, cachesz, clumpsz)];
          }

          cachesz -= pair->value.hlen;

          /* x (0..1) */
          value = pair->value.x;
          if(value == 0)
            xr[0] = 0;
          else
          {
            if(reqcache[value])
              requantized = reqcache[value];
            else
              requantized = reqcache[value] = III_requantize(value, exp);

            xr[0] = MASK1BIT(bitcache, cachesz--) ? -requantized : requantized;
          }

          /* y (0..1) */
          value = pair->value.y;
          if(value == 0)
            xr[1] = 0;
          else
          {
            if(reqcache[value])
              requantized = reqcache[value];
            else
              requantized = reqcache[value] = III_requantize(value, exp);

            xr[1] = MASK1BIT(bitcache, cachesz--) ? -requantized : requantized;
          }
        }
      }
    }
  }

  bits_left = ptr->readbit - peek.readbit;

  if(bits_left + cachesz < 0)
    return MAD_ERROR_BADHUFFDATA;  /* big_values overrun */

  /* count1 */
  {
    union huffquad const *table;
    register mad_fixed_t requantized;

    table = mad_huff_quad_table[channel->flags & count1table_select];

    requantized = III_requantize(1, exp);

    while(xr <= &xrarr[572] && bits_left + cachesz > 0)
    {
      union huffquad const *quad;

      /* hcod (1..6) */
      if(cachesz < 10)
      {
        if(cachesz < 0)
          return MAD_ERROR_BADHUFFDATA;  /* cache underrun */

        bits       = MAXLSHIFT - cachesz;
        bitcache   = (bitcache << bits) | mad_bit_read(&peek, bits);
        cachesz   += bits;
        bits_left -= bits;
      }

      quad = &table[MASK(bitcache, cachesz, 4)];

      /* quad tables guaranteed to have at most one extra lookup */
      if (!quad->final) {
        cachesz -= 4;

        quad = &table[quad->ptr.offset +
                      MASK(bitcache, cachesz, quad->ptr.bits)];
      }

      cachesz -= quad->value.hlen;

      if (xr == sfbound) {
        sfbound += *sfbwidth++;

        if (exp != *expptr) {
          exp = *expptr;
          requantized = III_requantize(1, exp);
        }

        ++expptr;
      }

      /* v (0..1) */
      xr[0] = quad->value.v ?
        (MASK1BIT(bitcache, cachesz--) ? -requantized : requantized) : 0;

      /* w (0..1) */
      xr[1] = quad->value.w ?
        (MASK1BIT(bitcache, cachesz--) ? -requantized : requantized) : 0;

      xr += 2;

      if (xr == sfbound) {
        sfbound += *sfbwidth++;

        if (exp != *expptr) {
          exp = *expptr;
          requantized = III_requantize(1, exp);
        }

        ++expptr;
      }

      /* x (0..1) */
      xr[0] = quad->value.x ?
        (MASK1BIT(bitcache, cachesz--) ? -requantized : requantized) : 0;

      /* y (0..1) */
      xr[1] = quad->value.y ?
        (MASK1BIT(bitcache, cachesz--) ? -requantized : requantized) : 0;

      xr += 2;
    }

    if(bits_left + cachesz < 0)
    {
      /* technically the bitstream is misformatted, but apparently
         some encoders are just a bit sloppy with stuffing bits */
      xr -= 4;
    }
  }

  /* rzero */
  memset(xr, 0, (char*)&xrarr[576] - (char*)xr);

  return MAD_ERROR_NONE;
}

# undef MASK
# undef MASK1BIT

/*
 * NAME:        III_reorder()
 * DESCRIPTION: reorder frequency lines of a short block into subband order
 */
static
void III_reorder(mad_fixed_t xr[576], struct channel const *channel,
                 unsigned char const sfbwidth[39])
{
  mad_fixed_t tmp[32][3][6];
  unsigned int sb, l, f, w, sbw[3], sw[3];

  /* this is probably wrong for 8000 Hz mixed blocks */

  sb = 0;
  if (channel->flags & mixed_block_flag) {
    sb = 2;

    l = 0;
    while (l < 36)
      l += *sfbwidth++;
  }

  for (w = 0; w < 3; ++w) {
    sbw[w] = sb;
    sw[w]  = 0;
  }

  f = *sfbwidth++;
  w = 0;

  for (l = 18 * sb; l < 576; ++l) {
    if (f-- == 0) {
      f = *sfbwidth++ - 1;
      w = (w + 1) % 3;
    }

    unsigned int sbww = sbw[w];
    tmp[sbww][w][sw[w]++] = xr[l];

    if (sw[w] == 6) {
      sw[w] = 0;
      ++sbw[w];
    }
  }

  memcpy(&xr[18 * sb], &tmp[sb], (576 - 18 * sb) * sizeof(mad_fixed_t));
}

/*
 * NAME:        III_stereo()
 * DESCRIPTION: perform joint stereo processing on a granule
 */
static
enum mad_error III_stereo(mad_fixed_t xr[2][576],
                          struct granule const *granule,
                          struct mad_header *header,
                          unsigned char const *sfbwidth)
{
  short modes[39];
  unsigned int sfbi, l, n, i;

  if (granule->ch[0].block_type !=
      granule->ch[1].block_type ||
      (granule->ch[0].flags & mixed_block_flag) !=
      (granule->ch[1].flags & mixed_block_flag))
    return MAD_ERROR_BADSTEREO;

  for (i = 0; i < 39; ++i)
    modes[i] = header->mode_extension;

  /* intensity stereo */

  if (header->mode_extension & I_STEREO) {
    struct channel const *right_ch = &granule->ch[1];
    mad_fixed_t const *right_xr = xr[1];
    unsigned int is_pos;

    header->flags |= MAD_FLAG_I_STEREO;

    /* first determine which scalefactor bands are to be processed */

    if (right_ch->block_type == 2) {
      unsigned int lower, start, max, bound[3], w;

      lower = start = max = bound[0] = bound[1] = bound[2] = 0;

      sfbi = l = 0;

      if (right_ch->flags & mixed_block_flag) {
        while (l < 36) {
          n = sfbwidth[sfbi++];

          for (i = 0; i < n; ++i) {
            if (right_xr[i]) {
              lower = sfbi;
              break;
            }
          }

          right_xr += n;
          l += n;
        }

        start = sfbi;
      }

      w = 0;
      while (l < 576) {
        n = sfbwidth[sfbi++];

        for (i = 0; i < n; ++i) {
          if (right_xr[i]) {
            max = bound[w] = sfbi;
            break;
          }
        }

        right_xr += n;
        l += n;
        w = (w + 1) % 3;
      }

      if (max)
        lower = start;

      /* long blocks */

      for (i = 0; i < lower; ++i)
        modes[i] = header->mode_extension & ~I_STEREO;

      /* short blocks */

      w = 0;
      for (i = start; i < max; ++i) {
        if (i < bound[w])
          modes[i] = header->mode_extension & ~I_STEREO;

        w = (w + 1) % 3;
      }
    }
    else {  /* right_ch->block_type != 2 */
      unsigned int bound;

      bound = 0;
      for (sfbi = l = 0; l < 576; l += n) {
        n = sfbwidth[sfbi++];

        for (i = 0; i < n; ++i) {
          if (right_xr[i]) {
            bound = sfbi;
            break;
          }
        }

        right_xr += n;
      }

      for (i = 0; i < bound; ++i)
        modes[i] = header->mode_extension & ~I_STEREO;
    }

    /* now do the actual processing */

    if (header->flags & MAD_FLAG_LSF_EXT) {
      unsigned char const *illegal_pos = granule[1].ch[1].scalefac;
      mad_fixed_t const *lsf_scale;

      /* intensity_scale */
      lsf_scale = is_lsf_table[right_ch->scalefac_compress & 0x1];

      for (sfbi = l = 0; l < 576; ++sfbi, l += n) {
        n = sfbwidth[sfbi];

        if (!(modes[sfbi] & I_STEREO))
          continue;

        if (illegal_pos[sfbi]) {
          modes[sfbi] &= ~I_STEREO;
          continue;
        }

        is_pos = right_ch->scalefac[sfbi];

        for (i = 0; i < n; ++i) {
          register mad_fixed_t left;

          left = xr[0][l + i];

          if (is_pos == 0)
            xr[1][l + i] = left;
          else {
            register mad_fixed_t opposite;

            opposite = mad_f_mul(left, lsf_scale[(is_pos - 1) / 2]);

            if (is_pos & 1) {
              xr[0][l + i] = opposite;
              xr[1][l + i] = left;
            }
            else
              xr[1][l + i] = opposite;
          }
        }
      }
    }
    else {  /* !(header->flags & MAD_FLAG_LSF_EXT) */
      for (sfbi = l = 0; l < 576; ++sfbi, l += n) {
        n = sfbwidth[sfbi];

        if (!(modes[sfbi] & I_STEREO))
          continue;

        is_pos = right_ch->scalefac[sfbi];

        if (is_pos >= 7) {  /* illegal intensity position */
          modes[sfbi] &= ~I_STEREO;
          continue;
        }

        for (i = 0; i < n; ++i) {
          register mad_fixed_t left;

          left = xr[0][l + i];

          xr[0][l + i] = mad_f_mul(left, is_table[    is_pos]);
          xr[1][l + i] = mad_f_mul(left, is_table[6 - is_pos]);
        }
      }
    }
  }

  /* middle/side stereo */

  if (header->mode_extension & MS_STEREO) {
    register mad_fixed_t invsqrt2;

    header->flags |= MAD_FLAG_MS_STEREO;

    invsqrt2 = root_table[3 + -2];

    for (sfbi = l = 0; l < 576; ++sfbi, l += n) {
      n = sfbwidth[sfbi];

      if (modes[sfbi] != MS_STEREO)
        continue;

      for (i = 0; i < n; ++i) {
        register mad_fixed_t m, s;

        m = xr[0][l + i];
        s = xr[1][l + i];

        xr[0][l + i] = mad_f_mul(m + s, invsqrt2);  /* l = (m + s) / sqrt(2) */
        xr[1][l + i] = mad_f_mul(m - s, invsqrt2);  /* r = (m - s) / sqrt(2) */
      }
    }
  }

  return MAD_ERROR_NONE;
}

#if defined(CPU_ARM_CLASSIC)
void III_aliasreduce(mad_fixed_t xr[576], int lines);
#else
/*
 * NAME:        III_aliasreduce()
 * DESCRIPTION: perform frequency line alias reduction
 */
static
void III_aliasreduce(mad_fixed_t xr[576], int lines)
{
  mad_fixed_t const *bound;
  int i;

  bound = &xr[lines];
  for (xr += 18; xr < bound; xr += 18) {
    for (i = 0; i < 8; ++i) {
      register mad_fixed_t a, b;
      register mad_fixed64hi_t hi;
      register mad_fixed64lo_t lo;

      a = xr[-1 - i];
      b = xr[     i];

# if defined(CPU_COLDFIRE)
      (void)hi, (void)lo;
      asm volatile ("mac.l %[a], %[csi], %%acc0\n\t"
                    "msac.l %[b], %[cai], %%acc0\n\t"
                    "mac.l %[b], %[csi], %%acc1\n\t"
                    "mac.l %[a], %[cai], %%acc1\n\t"
                    "movclr.l %%acc0, %[a]\n\t"
                    "asl.l #3, %[a]\n\t"
                    "movclr.l %%acc1, %[b]\n\t"
                    "asl.l #3, %[b]\n\t"
                    : [a] "+d" (a), [b] "+d" (b)
                    : [csi] "r" (cs[i]), [cai] "r" (ca[i]));
      xr[-1 - i] = a;
      xr[     i] = b;
# else
        MAD_F_ML0(hi, lo,  a, cs[i]);
        MAD_F_MLA(hi, lo, -b, ca[i]);

        xr[-1 - i] = MAD_F_MLZ(hi, lo);

        MAD_F_ML0(hi, lo,  b, cs[i]);
        MAD_F_MLA(hi, lo,  a, ca[i]);

        xr[     i] = MAD_F_MLZ(hi, lo);
# endif
    }
  }
}
#endif

# if defined(FPM_ARM)
void III_imdct_l(mad_fixed_t const [18], mad_fixed_t [36], unsigned int);
# else
/*
 * NAME:        imdct36
 * DESCRIPTION: perform X[18]->x[36] IMDCT
 */
#  if defined(CPU_COLDFIRE)
/* emac optimized imdct36, it is very ugly and i hope to replace it soon.
 * for now it is actually somewhat faster than the stock implementation. 
 */
static inline
void imdct36(mad_fixed_t const X[18], mad_fixed_t x[36])
{
  mad_fixed_t t[16];
  /* assumes FRACBITS = 28 */
  asm volatile (
  /* MAD_F_ML0(hi, lo, X[4],  MAD_F(0x0ec835e8)); */
  /* MAD_F_MLA(hi, lo, X[13], MAD_F(0x061f78aa)); */
  /* t6 = MAD_F_MLZ(hi, lo);                      */
    "move.l (4*4, %[X]), %%d0\n"
    "move.l #0x0ec835e8, %%d1\n"
    "move.l #0x061f78aa, %%d2\n"
    "mac.l %%d1, %%d0, (13*4, %[X]), %%d0, %%acc0\n"
    "mac.l %%d2, %%d0, ( 1*4, %[X]), %%d0, %%acc0\n"
    "move.l %%acc0, %%d7\n"
    "asl.l #3, %%d7\n"
    "move.l %%d7, (6*4, %[t])\n"

  /* MAD_F_ML0(hi, lo, (t14 = X[1] - X[10]), -MAD_F(0x0ec835e8)); */
  /* MAD_F_MLA(hi, lo, (t15 = X[7] + X[16]),  MAD_F(0x061f78aa)); */
  /* t4 = MAD_F_MLZ(hi, lo);                                      */
  /* MAD_F_MLA(hi, lo, t14, -MAD_F(0x061f78aa));                  */
  /* MAD_F_MLA(hi, lo, t15, -MAD_F(0x0ec835e8));                  */ 
  /* t0 = MAD_F_MLZ(hi, lo);                                      */
    "sub.l (10*4, %[X]), %%d0\n" /* t14 */
    "msac.l %%d0, %%d1,                    %%acc1\n"
    "msac.l %%d0, %%d2, (7*4, %[X]), %%d5, %%acc0\n"
    "add.l (16*4, %[X]), %%d5\n" /* t15 */
    "mac.l  %%d5, %%d2,                    %%acc1\n"
    "msac.l %%d5, %%d1, (     %[X]), %%d5, %%acc0\n"
    "movclr.l %%acc1, %%d6\n"
    "asl.l #3, %%d6\n" /* t4 */
    "move.l %%d6, (4*4, %[t])\n"
    "move.l %%acc0, %%d0\n"
    "asl.l #3, %%d0\n" /* t0 */
    "move.l %%d0, (0*4, %[t])\n"
    
  /* MAD_F_MLA(hi, lo, (t8 =X[0]-X[11]-X[12]),  MAD_F(0x0216a2a2)); */
  /* MAD_F_MLA(hi, lo, (t9 =X[2]-X[ 9]-X[14]),  MAD_F(0x09bd7ca0)); */
  /* MAD_F_MLA(hi, lo, (t10=X[3]-X[ 8]-X[15]), -MAD_F(0x0cb19346)); */
  /* MAD_F_MLA(hi, lo, (t11=X[5]-X[ 6]-X[17]), -MAD_F(0x0fdcf549)); */
  /* x[10] = -(x[7] = MAD_F_MLZ(hi, lo)); */

  /* MAD_F_ML0(hi, lo, t8,  -MAD_F(0x0cb19346)); */
  /* MAD_F_MLA(hi, lo, t9,   MAD_F(0x0fdcf549)); */
  /* MAD_F_MLA(hi, lo, t10,  MAD_F(0x0216a2a2)); */
  /* MAD_F_MLA(hi, lo, t11, -MAD_F(0x09bd7ca0)); */
  /* x[19] = x[34] = MAD_F_MLZ(hi, lo) - t0; */

  /* MAD_F_ML0(hi, lo, t8,   MAD_F(0x09bd7ca0)); */
  /* MAD_F_MLA(hi, lo, t9,  -MAD_F(0x0216a2a2)); */
  /* MAD_F_MLA(hi, lo, t10,  MAD_F(0x0fdcf549)); */
  /* MAD_F_MLA(hi, lo, t11, -MAD_F(0x0cb19346)); */
  /* x[ 1] = MAD_F_MLZ(hi, lo); */

  /* MAD_F_ML0(hi, lo, t8,  -MAD_F(0x0fdcf549)); */
  /* MAD_F_MLA(hi, lo, t9,  -MAD_F(0x0cb19346)); */
  /* MAD_F_MLA(hi, lo, t10, -MAD_F(0x09bd7ca0)); */
  /* MAD_F_MLA(hi, lo, t11, -MAD_F(0x0216a2a2)); */
  /* x[25] = MAD_F_MLZ(hi, lo); */

  /* t12 = t8 - t10; */
  /* t13 = t9 + t11; */
    "move.l #0x0216a2a2, %%d1\n"
    "move.l #0x0cb19346, %%d2\n"
    "move.l #0x09bd7ca0, %%d3\n"
    "move.l #0x0fdcf549, %%d4\n"
    "sub.l (11*4, %[X]), %%d5\n"
    "sub.l (12*4, %[X]), %%d5\n"
    "mac.l  %%d1, %%d5,                    %%acc0\n"
    "msac.l %%d2, %%d5,                    %%acc1\n"
    "mac.l  %%d3, %%d5,                    %%acc2\n"
    "msac.l %%d4, %%d5, (2*4, %[X]), %%d6, %%acc3\n"
    "sub.l ( 9*4, %[X]), %%d6\n"
    "sub.l (14*4, %[X]), %%d6\n"
    "mac.l  %%d3, %%d6,                    %%acc0\n"
    "mac.l  %%d4, %%d6,                    %%acc1\n"
    "msac.l %%d1, %%d6,                    %%acc2\n"
    "msac.l %%d2, %%d6, (3*4, %[X]), %%d7, %%acc3\n"
    "sub.l ( 8*4, %[X]), %%d7\n"
    "sub.l (15*4, %[X]), %%d7\n"
    "sub.l %%d7, %%d5\n" /* d5: t12 */
    "move.l %%d5, (12*4, %[t])\n"
    "msac.l %%d2, %%d7,                    %%acc0\n"
    "mac.l  %%d1, %%d7,                    %%acc1\n"
    "mac.l  %%d4, %%d7,                    %%acc2\n"
    "msac.l %%d3, %%d7, (5*4, %[X]), %%d7, %%acc3\n"
    "sub.l ( 6*4, %[X]), %%d7\n"
    "sub.l (17*4, %[X]), %%d7\n"
    "add.l %%d7, %%d6\n" /* d6: t13 */
    "move.l %%d6, (13*4, %[t])\n"
    "msac.l %%d4, %%d7,                    %%acc0\n"
    "msac.l %%d3, %%d7,                    %%acc1\n"
    "msac.l %%d2, %%d7,                    %%acc2\n"
    "msac.l %%d1, %%d7, (1*4, %[X]), %%d5, %%acc3\n"
    
    "movclr.l %%acc0, %%d7\n"
    "asl.l #3, %%d7\n"
    "move.l %%d7, (7*4, %[x])\n"
    "neg.l %%d7\n"
    "move.l %%d7, (10*4, %[x])\n"

    "movclr.l %%acc1, %%d7\n"
    "asl.l #3, %%d7\n"
    "sub.l %%d0, %%d7\n"
    "move.l %%d7, (19*4, %[x])\n"
    "move.l %%d7, (34*4, %[x])\n"
    
    "movclr.l %%acc2, %%d7\n"
    "asl.l #3, %%d7\n"
    "move.l %%d7, ( 1*4, %[x])\n"
    
    "movclr.l %%acc3, %%d7\n"
    "asl.l #3, %%d7\n"
    "move.l %%d7, (25*4, %[x])\n"
    
  /* MAD_F_ML0(hi, lo, X[1],  -MAD_F(0x09bd7ca0)); */
  /* MAD_F_MLA(hi, lo, X[7],   MAD_F(0x0216a2a2)); */
  /* MAD_F_MLA(hi, lo, X[10], -MAD_F(0x0fdcf549)); */
  /* MAD_F_MLA(hi, lo, X[16],  MAD_F(0x0cb19346)); */
  /* t1 = MAD_F_MLZ(hi, lo) + t6; */
  
  /* MAD_F_ML0(hi, lo, X[1],  -MAD_F(0x0216a2a2)); */
  /* MAD_F_MLA(hi, lo, X[7],  -MAD_F(0x09bd7ca0)); */
  /* MAD_F_MLA(hi, lo, X[10],  MAD_F(0x0cb19346)); */
  /* MAD_F_MLA(hi, lo, X[16],  MAD_F(0x0fdcf549)); */
  /* t3 = MAD_F_MLZ(hi, lo); */

  /* MAD_F_ML0(hi, lo, X[1],  -MAD_F(0x0fdcf549)); */
  /* MAD_F_MLA(hi, lo, X[7],  -MAD_F(0x0cb19346)); */
  /* MAD_F_MLA(hi, lo, X[10], -MAD_F(0x09bd7ca0)); */
  /* MAD_F_MLA(hi, lo, X[16], -MAD_F(0x0216a2a2)); */
  /* t5 = MAD_F_MLZ(hi, lo) - t6; */
    "msac.l %%d3, %%d5,                     %%acc0\n"
    "msac.l %%d1, %%d5,                     %%acc1\n"
    "msac.l %%d4, %%d5, ( 7*4, %[X]), %%d5, %%acc2\n"
    "mac.l  %%d1, %%d5,                     %%acc0\n"
    "msac.l %%d3, %%d5,                     %%acc1\n"
    "msac.l %%d2, %%d5, (10*4, %[X]), %%d5, %%acc2\n"
    "msac.l %%d4, %%d5,                     %%acc0\n"
    "mac.l  %%d2, %%d5,                     %%acc1\n"
    "msac.l %%d3, %%d5, (16*4, %[X]), %%d5, %%acc2\n"
    "mac.l  %%d2, %%d5,                     %%acc0\n"
    "mac.l  %%d4, %%d5,                     %%acc1\n"
    "msac.l %%d1, %%d5, ( 0*4, %[X]), %%d0, %%acc2\n"

    "movclr.l %%acc0, %%d7\n"
    "asl.l #3, %%d7\n"
    "add.l (6*4, %[t]), %%d7\n" /* t1 */
    
    "movclr.l %%acc1, %%d5\n"
    "asl.l #3, %%d5\n"          /* t3 */
    
    "movclr.l %%acc2, %%d6\n"
    "asl.l #3, %%d6\n"
    "sub.l (6*4, %[t]), %%d6\n" /* t5 */
    "move.l %%d6, (5*4, %[t])\n"

  /* MAD_F_ML0(hi, lo, X[0],   MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[6],   MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[11],  MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[15], -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0f9ee890)); */
  /* x[11] = -(x[6] = MAD_F_MLZ(hi, lo) + t1); */
  /* MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[5],   MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[17],  MAD_F(0x04cfb0e2)); */
  /* x[23] = x[30] = MAD_F_MLZ(hi, lo) + t1; */
  /* MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[5],   MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[11],  MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0acf37ad)); */
  /* x[18] = x[35] = MAD_F_MLZ(hi, lo) - t1; */
    "move.l #0x03768962, %%d1\n"
    "move.l #0x0f426cb5, %%d2\n"
    "move.l #0x0bcbe352, %%d3\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, (11*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d3, %%d0,                     %%acc1\n"
    "mac.l  %%d1, %%d0, (12*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "msac.l %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, ( 2*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0e313245, %%d1\n"
    "move.l #0x00b2aa3e, %%d2\n"
    "move.l #0x0d7e8807, %%d3\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "mac.l  %%d3, %%d0, ( 9*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d3, %%d0,                     %%acc0\n"
    "msac.l %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, (14*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "mac.l  %%d1, %%d0, ( 3*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0ffc19fd, %%d1\n"
    "move.l #0x0898c779, %%d2\n"
    "move.l #0x07635284, %%d3\n"
    "msac.l %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, ( 8*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, (15*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "mac.l  %%d2, %%d0, ( 5*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0acf37ad, %%d1\n"
    "move.l #0x0f9ee890, %%d2\n"
    "move.l #0x04cfb0e2, %%d3\n"
    "msac.l %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "mac.l  %%d3, %%d0, ( 6*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "mac.l  %%d2, %%d0, (17*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d2, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, ( 4*4, %[X]), %%d0, %%acc2\n"

    "movclr.l %%acc0, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l %%d7, %%d6\n"
    "move.l %%d6, (6*4, %[x])\n"
    "neg.l %%d6\n"
    "move.l %%d6, (11*4, %[x])\n"
    
    "movclr.l %%acc1, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l %%d7, %%d6\n"
    "move.l %%d6, (23*4, %[x])\n"
    "move.l %%d6, (30*4, %[x])\n"
    
    "movclr.l %%acc2, %%d6\n"
    "asl.l #3, %%d6\n"
    "sub.l %%d7, %%d6\n"
    "move.l %%d6, (18*4, %[x])\n"
    "move.l %%d6, (35*4, %[x])\n"
    
  /* MAD_F_ML0(hi, lo, X[4],   MAD_F(0x061f78aa)); */
  /* MAD_F_MLA(hi, lo, X[13], -MAD_F(0x0ec835e8)); */
  /* t3 += (t7 = MAD_F_MLZ(hi, lo)); */
  /* t4 -= t7; */
    "move.l #0x061f78aa, %%d1\n"
    "mac.l %%d1, %%d0, (13*4, %[X]), %%d0, %%acc0\n"
    "move.l #0x0ec835e8, %%d1\n"
    "msac.l %%d1, %%d0, (1*4, %[X]), %%d0, %%acc0\n"
    "move.l %%acc0, %%d6\n"
    "asl.l #3, %%d6\n"   /* t7 */
    "add.l %%d6, %%d5\n" /* t3 */
    "move.l (4*4, %[t]), %%d1\n"
    "sub.l %%d6, %%d1\n" /* t4 */
    "move.l %%d1, (4*4, %[t])\n"
    
  /* MAD_F_MLA(hi, lo, X[1],  -MAD_F(0x0cb19346)); */
  /* MAD_F_MLA(hi, lo, X[7],   MAD_F(0x0fdcf549)); */
  /* MAD_F_MLA(hi, lo, X[10],  MAD_F(0x0216a2a2)); */
  /* MAD_F_MLA(hi, lo, X[16], -MAD_F(0x09bd7ca0)); */
  /* t2 = MAD_F_MLZ(hi, lo); */
    "move.l #0x0cb19346, %%d1\n"
    "msac.l %%d1, %%d0, ( 7*4, %[X]), %%d0, %%acc0\n"
    "move.l #0x0fdcf549, %%d1\n"
    "mac.l  %%d1, %%d0, (10*4, %[X]), %%d0, %%acc0\n"
    "move.l #0x0216a2a2, %%d1\n"
    "mac.l  %%d1, %%d0, (16*4, %[X]), %%d0, %%acc0\n"
    "move.l #0x09bd7ca0, %%d1\n"
    "msac.l %%d1, %%d0, (      %[X]), %%d0, %%acc0\n"
    "move.l %%acc0, %%d7\n"
    "asl.l #3, %%d7\n" /* t2 */
    
  /* MAD_F_MLA(hi, lo, X[0],   MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[5],   MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[9],   MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[12],  MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[15],  MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[17],  MAD_F(0x0f426cb5)); */
  /* x[12] = -(x[5] = MAD_F_MLZ(hi, lo)); */
  /* MAD_F_ML0(hi, lo, X[0],   MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[8],   MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[11],  MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0bcbe352)); */
  /* x[17] = -(x[0] = MAD_F_MLZ(hi, lo) + t2); */
  /* MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[5],   MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[14], -MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[15], -MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x03768962)); */
  /* x[24] = x[29] = MAD_F_MLZ(hi, lo) + t2; */
    "move.l #0x0acf37ad, %%d1\n"
    "move.l #0x0f9ee890, %%d2\n"
    "move.l #0x04cfb0e2, %%d3\n"
    "mac.l  %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, (11*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, (12*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, ( 2*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0ffc19fd, %%d1\n"
    "move.l #0x0898c779, %%d2\n"
    "move.l #0x07635284, %%d3\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, ( 9*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d3, %%d0,                     %%acc0\n"
    "msac.l %%d1, %%d0,                     %%acc1\n"
    "mac.l  %%d2, %%d0, (14*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, ( 3*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0e313245, %%d1\n"
    "move.l #0x00b2aa3e, %%d2\n"
    "move.l #0x0d7e8807, %%d3\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, ( 8*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "mac.l  %%d3, %%d0, (15*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, ( 5*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x03768962, %%d1\n"
    "move.l #0x0f426cb5, %%d2\n"
    "move.l #0x0bcbe352, %%d3\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "mac.l  %%d3, %%d0, ( 6*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "msac.l %%d1, %%d0,                     %%acc1\n"
    "mac.l  %%d2, %%d0, (17*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, (      %[X]), %%d0, %%acc2\n"

    "movclr.l %%acc0, %%d6\n"
    "asl.l #3, %%d6\n"
    "move.l %%d6, ( 5*4, %[x])\n"
    "neg.l %%d6\n"
    "move.l %%d6, (12*4, %[x])\n"
    
    "movclr.l %%acc1, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l %%d7, %%d6\n"
    "move.l %%d6, (      %[x])\n"
    "neg.l %%d6\n"
    "move.l %%d6, (17*4, %[x])\n"
    
    "movclr.l %%acc2, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l %%d7, %%d6\n"
    "move.l %%d6, (24*4, %[x])\n"
    "move.l %%d6, (29*4, %[x])\n"

  /* MAD_F_ML0(hi, lo, X[0],   MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[12],  MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[15], -MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0ffc19fd)); */
  /* x[9] = -(x[8] = MAD_F_MLZ(hi, lo) + t3); */

  /* MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[8],   MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[14], -MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[17],  MAD_F(0x07635284)); */
  /* x[21] = x[32] = MAD_F_MLZ(hi, lo) + t3; */

  /* MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[9],   MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[12],  MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[15], -MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0898c779)); */
  /* x[20] = x[33] = MAD_F_MLZ(hi, lo) - t3; */
    "move.l #0x0e313245, %%d1\n"
    "move.l #0x00b2aa3e, %%d2\n"
    "move.l #0x0d7e8807, %%d3\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, (11*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, (12*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "msac.l %%d3, %%d0,                     %%acc1\n"
    "mac.l  %%d2, %%d0, ( 2*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x03768962, %%d1\n"
    "move.l #0x0f426cb5, %%d2\n"
    "move.l #0x0bcbe352, %%d3\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "mac.l  %%d2, %%d0, ( 9*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "mac.l  %%d1, %%d0, (14*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d1, %%d0,                     %%acc1\n"
    "mac.l  %%d3, %%d0, ( 3*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0acf37ad, %%d1\n"
    "move.l #0x0f9ee890, %%d2\n"
    "move.l #0x04cfb0e2, %%d3\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "mac.l  %%d1, %%d0, ( 8*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "mac.l  %%d2, %%d0, (15*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d2, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, ( 5*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0ffc19fd, %%d1\n"
    "move.l #0x0898c779, %%d2\n"
    "move.l #0x07635284, %%d3\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, ( 6*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, (17*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, (12*4, %[t]), %%d0, %%acc2\n"

    "movclr.l %%acc0, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l %%d5, %%d6\n"
    "move.l %%d6, (8*4, %[x])\n"
    "neg.l %%d6\n"
    "move.l %%d6, (9*4, %[x])\n"
    
    "movclr.l %%acc1, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l %%d5, %%d6\n"
    "move.l %%d6, (21*4, %[x])\n"
    "move.l %%d6, (32*4, %[x])\n"
    
    "movclr.l %%acc2, %%d6\n"
    "asl.l #3, %%d6\n"
    "sub.l %%d5, %%d6\n"
    "move.l %%d6, (20*4, %[x])\n"
    "move.l %%d6, (33*4, %[x])\n"
    
  /* MAD_F_ML0(hi, lo, t12, -MAD_F(0x0ec835e8)); */
  /* MAD_F_MLA(hi, lo, t13,  MAD_F(0x061f78aa)); */
  /* x[22] = x[31] = MAD_F_MLZ(hi, lo) + t0; */
    "move.l #0x0ec835e8, %%d1\n"
    "move.l #0x061f78aa, %%d2\n"
    "msac.l %%d1, %%d0, (13*4, %[t]), %%d3, %%acc0\n"
    "mac.l  %%d2, %%d3, ( 1*4, %[x]), %%d4, %%acc0\n"
    "movclr.l %%acc0, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l (0*4, %[t]), %%d6\n"
    "move.l %%d6, (22*4, %[x])\n"
    "move.l %%d6, (31*4, %[x])\n"
    
  /* MAD_F_ML0(hi, lo, t12, MAD_F(0x061f78aa)); */
  /* MAD_F_MLA(hi, lo, t13, MAD_F(0x0ec835e8)); */
  /* x[13] = -(x[4] = MAD_F_MLZ(hi, lo) + t4); */
  /* x[16] = -(x[1] = x[1]              + t4); */
  /* x[25] =  x[28] = x[25]             + t4;  */
    "mac.l  %%d2, %%d0, (4*4, %[t]), %%d2, %%acc0\n"
    "mac.l  %%d1, %%d3, (     %[X]), %%d0, %%acc0\n"
    "movclr.l %%acc0, %%d6\n"
    "asl.l #3, %%d6\n"
    "add.l %%d2, %%d6\n"
    "move.l %%d6, ( 4*4, %[x])\n"
    "neg.l %%d6\n"
    "move.l %%d6, (13*4, %[x])\n"

    "add.l %%d2, %%d4\n"
    "move.l %%d4, ( 1*4, %[x])\n"
    "neg.l %%d4\n"
    "move.l %%d4, (16*4, %[x])\n"

    "move.l (25*4, %[x]), %%d4\n"
    "add.l %%d2, %%d4\n"
    "move.l %%d4, (25*4, %[x])\n"
    "move.l %%d4, (28*4, %[x])\n"

  /* MAD_F_ML0(hi, lo, X[0],   MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[5],   MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[12],  MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[14], -MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0d7e8807)); */
  /* x[15] = -(x[2] = MAD_F_MLZ(hi, lo) + t5); */
  /* MAD_F_ML0(hi, lo, X[0],   MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[3],   MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[5],   MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x00b2aa3e)); */
  /* MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[11],  MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[15], -MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[17],  MAD_F(0x0e313245)); */
  /* x[14] = -(x[3] = MAD_F_MLZ(hi, lo) + t5); */
  /* MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0ffc19fd)); */
  /* MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x0f9ee890)); */
  /* MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x0f426cb5)); */
  /* MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0e313245)); */
  /* MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x0d7e8807)); */
  /* MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0bcbe352)); */
  /* MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0acf37ad)); */
  /* MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0898c779)); */
  /* MAD_F_MLA(hi, lo, X[12], -MAD_F(0x07635284)); */
  /* MAD_F_MLA(hi, lo, X[14], -MAD_F(0x04cfb0e2)); */
  /* MAD_F_MLA(hi, lo, X[15], -MAD_F(0x03768962)); */
  /* MAD_F_MLA(hi, lo, X[17], -MAD_F(0x00b2aa3e)); */
  /* x[26] = x[27] = MAD_F_MLZ(hi, lo) + t5; */
    "move.l #0x0ffc19fd, %%d1\n"
    "move.l #0x0898c779, %%d2\n"
    "move.l #0x07635284, %%d3\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, (11*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, (12*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, ( 2*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0acf37ad, %%d1\n"
    "move.l #0x0f9ee890, %%d2\n"
    "move.l #0x04cfb0e2, %%d3\n"
    "mac.l  %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, ( 9*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, (14*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, ( 3*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x03768962, %%d1\n"
    "move.l #0x0f426cb5, %%d2\n"
    "move.l #0x0bcbe352, %%d3\n"
    "mac.l  %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, ( 8*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d1, %%d0,                     %%acc0\n"
    "mac.l  %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, (15*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "msac.l %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, ( 5*4, %[X]), %%d0, %%acc2\n"
    "move.l #0x0e313245, %%d1\n"
    "move.l #0x00b2aa3e, %%d2\n"
    "move.l #0x0d7e8807, %%d3\n"
    "mac.l  %%d2, %%d0,                     %%acc0\n"
    "mac.l  %%d3, %%d0,                     %%acc1\n"
    "msac.l %%d1, %%d0, ( 6*4, %[X]), %%d0, %%acc2\n"
    "mac.l  %%d1, %%d0,                     %%acc0\n"
    "msac.l %%d2, %%d0,                     %%acc1\n"
    "msac.l %%d3, %%d0, (17*4, %[X]), %%d0, %%acc2\n"
    "msac.l %%d3, %%d0,                     %%acc0\n"
    "mac.l  %%d1, %%d0,                     %%acc1\n"
    "msac.l %%d2, %%d0, ( 5*4, %[t]), %%d6, %%acc2\n"
    "movclr.l %%acc0, %%d7\n"
    "asl.l #3, %%d7\n"
    "add.l %%d6, %%d7\n"
    "move.l %%d7, (2*4, %[x])\n"
    "neg.l %%d7\n"
    "move.l %%d7, (15*4, %[x])\n"
    
    "movclr.l %%acc1, %%d7\n"
    "asl.l #3, %%d7\n"
    "add.l %%d6, %%d7\n"
    "move.l %%d7, (3*4, %[x])\n"
    "neg.l %%d7\n"
    "move.l %%d7, (14*4, %[x])\n"
    
    "movclr.l %%acc2, %%d7\n"
    "asl.l #3, %%d7\n"
    "add.l %%d6, %%d7\n"
    "move.l %%d7, (26*4, %[x])\n"
    "move.l %%d7, (27*4, %[x])\n"

    : : [X] "a" (X), [x] "a" (x), [t] "a" (t) 
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7");
  /* pfew */
}

#  else /* if defined(CPU_COLDFIRE) */

static inline
void imdct36(mad_fixed_t const X[18], mad_fixed_t x[36])
{
  mad_fixed_t t0, t1, t2,  t3,  t4,  t5,  t6,  t7;
  mad_fixed_t t8, t9, t10, t11, t12, t13, t14, t15;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;

  MAD_F_ML0(hi, lo, (t14 = X[1] - X[10]), -MAD_F(0x0ec835e8));
  MAD_F_MLA(hi, lo, (t15 = X[7] + X[16]),  MAD_F(0x061f78aa));
  t4 = MAD_F_MLZ(hi, lo);

  MAD_F_ML0(hi, lo, X[4],  MAD_F(0x0ec835e8));
  MAD_F_MLA(hi, lo, X[13], MAD_F(0x061f78aa));
  t6 = MAD_F_MLZ(hi, lo);

  MAD_F_MLA(hi, lo, t14, -MAD_F(0x061f78aa));
  MAD_F_MLA(hi, lo, t15, -MAD_F(0x0ec835e8));
  t0 = MAD_F_MLZ(hi, lo);

  MAD_F_MLA(hi, lo, (t8 =X[0]-X[11]-X[12]),  MAD_F(0x0216a2a2));
  MAD_F_MLA(hi, lo, (t9 =X[2]-X[ 9]-X[14]),  MAD_F(0x09bd7ca0));
  MAD_F_MLA(hi, lo, (t10=X[3]-X[ 8]-X[15]), -MAD_F(0x0cb19346));
  MAD_F_MLA(hi, lo, (t11=X[5]-X[ 6]-X[17]), -MAD_F(0x0fdcf549));
  x[10] = -(x[7] = MAD_F_MLZ(hi, lo));

  MAD_F_ML0(hi, lo, t8,  -MAD_F(0x0cb19346));
  MAD_F_MLA(hi, lo, t9,   MAD_F(0x0fdcf549));
  MAD_F_MLA(hi, lo, t10,  MAD_F(0x0216a2a2));
  MAD_F_MLA(hi, lo, t11, -MAD_F(0x09bd7ca0));
  x[19] = x[34] = MAD_F_MLZ(hi, lo) - t0;

  MAD_F_ML0(hi, lo, t8,   MAD_F(0x09bd7ca0));
  MAD_F_MLA(hi, lo, t9,  -MAD_F(0x0216a2a2));
  MAD_F_MLA(hi, lo, t10,  MAD_F(0x0fdcf549));
  MAD_F_MLA(hi, lo, t11, -MAD_F(0x0cb19346));
  x[ 1] = MAD_F_MLZ(hi, lo);

  MAD_F_ML0(hi, lo, t8,  -MAD_F(0x0fdcf549));
  MAD_F_MLA(hi, lo, t9,  -MAD_F(0x0cb19346));
  MAD_F_MLA(hi, lo, t10, -MAD_F(0x09bd7ca0));
  MAD_F_MLA(hi, lo, t11, -MAD_F(0x0216a2a2));
  x[25] = MAD_F_MLZ(hi, lo);

  t12 = t8 - t10;
  t13 = t9 + t11;

  MAD_F_ML0(hi, lo, X[1],  -MAD_F(0x09bd7ca0));
  MAD_F_MLA(hi, lo, X[7],   MAD_F(0x0216a2a2));
  MAD_F_MLA(hi, lo, X[10], -MAD_F(0x0fdcf549));
  MAD_F_MLA(hi, lo, X[16],  MAD_F(0x0cb19346));
  t1 = MAD_F_MLZ(hi, lo) + t6;

  MAD_F_ML0(hi, lo, X[1],  -MAD_F(0x0216a2a2));
  MAD_F_MLA(hi, lo, X[7],  -MAD_F(0x09bd7ca0));
  MAD_F_MLA(hi, lo, X[10],  MAD_F(0x0cb19346));
  MAD_F_MLA(hi, lo, X[16],  MAD_F(0x0fdcf549));
  t3 = MAD_F_MLZ(hi, lo);

  MAD_F_ML0(hi, lo, X[1],  -MAD_F(0x0fdcf549));
  MAD_F_MLA(hi, lo, X[7],  -MAD_F(0x0cb19346));
  MAD_F_MLA(hi, lo, X[10], -MAD_F(0x09bd7ca0));
  MAD_F_MLA(hi, lo, X[16], -MAD_F(0x0216a2a2));
  t5 = MAD_F_MLZ(hi, lo) - t6;

  MAD_F_ML0(hi, lo, X[0],   MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[6],   MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[11],  MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[15], -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0f9ee890));
  x[11] = -(x[6] = MAD_F_MLZ(hi, lo) + t1);

  MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[5],   MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[17],  MAD_F(0x04cfb0e2));
  x[23] = x[30] = MAD_F_MLZ(hi, lo) + t1;

  MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[5],   MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[11],  MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0acf37ad));
  x[18] = x[35] = MAD_F_MLZ(hi, lo) - t1;

  MAD_F_ML0(hi, lo, X[4],   MAD_F(0x061f78aa));
  MAD_F_MLA(hi, lo, X[13], -MAD_F(0x0ec835e8));
  t3+= (t7 = MAD_F_MLZ(hi, lo));
  t4-= t7;

  MAD_F_MLA(hi, lo, X[1],  -MAD_F(0x0cb19346));
  MAD_F_MLA(hi, lo, X[7],   MAD_F(0x0fdcf549));
  MAD_F_MLA(hi, lo, X[10],  MAD_F(0x0216a2a2));
  MAD_F_MLA(hi, lo, X[16], -MAD_F(0x09bd7ca0));
  t2 = MAD_F_MLZ(hi, lo);

  MAD_F_MLA(hi, lo, X[0],   MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[5],   MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[9],   MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[12],  MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[15],  MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[17],  MAD_F(0x0f426cb5));
  x[12] = -(x[5] = MAD_F_MLZ(hi, lo));

  MAD_F_ML0(hi, lo, X[0],   MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[8],   MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[11],  MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0bcbe352));
  x[17] = -(x[0] = MAD_F_MLZ(hi, lo) + t2);

  MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[5],   MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[14], -MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[15], -MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x03768962));
  x[24] = x[29] = MAD_F_MLZ(hi, lo) + t2;

  MAD_F_ML0(hi, lo, X[0],   MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[12],  MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[15], -MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0ffc19fd));
  x[9] = -(x[8] = MAD_F_MLZ(hi, lo) + t3);

  MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[8],   MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[14], -MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[17],  MAD_F(0x07635284));
  x[21] = x[32] = MAD_F_MLZ(hi, lo) + t3;

  MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[9],   MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[12],  MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[15], -MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0898c779));
  x[20] = x[33] = MAD_F_MLZ(hi, lo) - t3;

  MAD_F_ML0(hi, lo, t12, -MAD_F(0x0ec835e8));
  MAD_F_MLA(hi, lo, t13,  MAD_F(0x061f78aa));
  x[22] = x[31] = MAD_F_MLZ(hi, lo) + t0;

  MAD_F_ML0(hi, lo, t12, MAD_F(0x061f78aa));
  MAD_F_MLA(hi, lo, t13, MAD_F(0x0ec835e8));
  x[13] = -(x[4] = MAD_F_MLZ(hi, lo) + t4);
  x[16] = -(x[1] = x[1]              + t4);
  x[25] =  x[28] = x[25]             + t4;

  MAD_F_ML0(hi, lo, X[0],   MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[3],   MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[5],   MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[6],   MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[9],   MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[12],  MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[14], -MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[15],  MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x0d7e8807));
  x[15] = -(x[2] = MAD_F_MLZ(hi, lo) + t5);

  MAD_F_ML0(hi, lo, X[0],   MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[2],   MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[3],   MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[5],   MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x00b2aa3e));
  MAD_F_MLA(hi, lo, X[8],   MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[11],  MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[14],  MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[15], -MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[17],  MAD_F(0x0e313245));
  x[14] = -(x[3] = MAD_F_MLZ(hi, lo) + t5);

  MAD_F_ML0(hi, lo, X[0],  -MAD_F(0x0ffc19fd));
  MAD_F_MLA(hi, lo, X[2],  -MAD_F(0x0f9ee890));
  MAD_F_MLA(hi, lo, X[3],  -MAD_F(0x0f426cb5));
  MAD_F_MLA(hi, lo, X[5],  -MAD_F(0x0e313245));
  MAD_F_MLA(hi, lo, X[6],  -MAD_F(0x0d7e8807));
  MAD_F_MLA(hi, lo, X[8],  -MAD_F(0x0bcbe352));
  MAD_F_MLA(hi, lo, X[9],  -MAD_F(0x0acf37ad));
  MAD_F_MLA(hi, lo, X[11], -MAD_F(0x0898c779));
  MAD_F_MLA(hi, lo, X[12], -MAD_F(0x07635284));
  MAD_F_MLA(hi, lo, X[14], -MAD_F(0x04cfb0e2));
  MAD_F_MLA(hi, lo, X[15], -MAD_F(0x03768962));
  MAD_F_MLA(hi, lo, X[17], -MAD_F(0x00b2aa3e));
  x[26] = x[27] = MAD_F_MLZ(hi, lo) + t5;
}
#  endif /* CPU_COLDFIRE */

/*
 * NAME:        III_imdct_l()
 * DESCRIPTION: perform IMDCT and windowing for long blocks
 */
static
void III_imdct_l(mad_fixed_t const X[18], mad_fixed_t z[36],
                 unsigned int block_type)
{
  unsigned int i;

  /* IMDCT */

  imdct36(X, z);

  /* windowing */

  switch (block_type) {
  case 0:  /* normal window */
# if 1
    /* loop unrolled implementation */
    for (i = 0; i < 36; i += 4) {
      z[i + 0] = mad_f_mul(z[i + 0], window_l[i + 0]);
      z[i + 1] = mad_f_mul(z[i + 1], window_l[i + 1]);
      z[i + 2] = mad_f_mul(z[i + 2], window_l[i + 2]);
      z[i + 3] = mad_f_mul(z[i + 3], window_l[i + 3]);
    }
# else
    /* reference implementation */
    for (i =  0; i < 36; ++i) z[i] = mad_f_mul(z[i], window_l[i]);
# endif
    break;

  case 1:  /* start block */
    for (i =  0; i < 18; i += 3) {
      z[i + 0] = mad_f_mul(z[i + 0], window_l[i + 0]);
      z[i + 1] = mad_f_mul(z[i + 1], window_l[i + 1]);
      z[i + 2] = mad_f_mul(z[i + 2], window_l[i + 2]);
    }
    /*  (i = 18; i < 24; ++i) z[i] unchanged */
    for (i = 24; i < 30; ++i) z[i] = mad_f_mul(z[i], window_s[i - 18]);
    for (i = 30; i < 36; ++i) z[i] = 0;
    break;

  case 3:  /* stop block */
    for (i =  0; i <  6; ++i) z[i] = 0;
    for (i =  6; i < 12; ++i) z[i] = mad_f_mul(z[i], window_s[i - 6]);
    /*  (i = 12; i < 18; ++i) z[i] unchanged */
    for (i = 18; i < 36; i += 3) {
      z[i + 0] = mad_f_mul(z[i + 0], window_l[i + 0]);
      z[i + 1] = mad_f_mul(z[i + 1], window_l[i + 1]);
      z[i + 2] = mad_f_mul(z[i + 2], window_l[i + 2]);
    }
    break;
  }
}
# endif  /* FPM_ARM */

/*
 * NAME:        III_imdct_s()
 * DESCRIPTION: perform IMDCT and windowing for short blocks
 */

# if defined(CPU_COLDFIRE)
void III_imdct_s(mad_fixed_t const X[18], mad_fixed_t z[36]);
#else

static
void III_imdct_s(mad_fixed_t const X[18], mad_fixed_t z[36])
{
  mad_fixed_t y[36], *yptr;
  mad_fixed_t const *wptr;
  int w, i;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;

  /* IMDCT */

  yptr = &y[0];

  for (w = 0; w < 3; ++w) {
    register mad_fixed_t const (*s)[6];

    s = imdct_s;

    for (i = 0; i < 3; ++i) {
      MAD_F_ML0(hi, lo, X[0], (*s)[0]);
      MAD_F_MLA(hi, lo, X[1], (*s)[1]);
      MAD_F_MLA(hi, lo, X[2], (*s)[2]);
      MAD_F_MLA(hi, lo, X[3], (*s)[3]);
      MAD_F_MLA(hi, lo, X[4], (*s)[4]);
      MAD_F_MLA(hi, lo, X[5], (*s)[5]);

      yptr[i + 0] = MAD_F_MLZ(hi, lo);
      yptr[5 - i] = -yptr[i + 0];

      ++s;

      MAD_F_ML0(hi, lo, X[0], (*s)[0]);
      MAD_F_MLA(hi, lo, X[1], (*s)[1]);
      MAD_F_MLA(hi, lo, X[2], (*s)[2]);
      MAD_F_MLA(hi, lo, X[3], (*s)[3]);
      MAD_F_MLA(hi, lo, X[4], (*s)[4]);
      MAD_F_MLA(hi, lo, X[5], (*s)[5]);

      yptr[ i + 6] = MAD_F_MLZ(hi, lo);
      yptr[11 - i] = yptr[i + 6];

      ++s;
    }

    yptr += 12;
    X    += 6;
  }

  /* windowing, overlapping and concatenation */

  yptr = &y[0];
  wptr = &window_s[0];

  for (i = 0; i < 6; ++i) {
    z[i +  0] = 0;
    z[i +  6] = mad_f_mul(yptr[ 0 + 0], wptr[0]);

    MAD_F_ML0(hi, lo, yptr[ 0 + 6], wptr[6]);
    MAD_F_MLA(hi, lo, yptr[12 + 0], wptr[0]);

    z[i + 12] = MAD_F_MLZ(hi, lo);

    MAD_F_ML0(hi, lo, yptr[12 + 6], wptr[6]);
    MAD_F_MLA(hi, lo, yptr[24 + 0], wptr[0]);

    z[i + 18] = MAD_F_MLZ(hi, lo);

    z[i + 24] = mad_f_mul(yptr[24 + 6], wptr[6]);
    z[i + 30] = 0;

    ++yptr;
    ++wptr;
  }
}

#endif

#if defined(CPU_ARM_CLASSIC)
void III_overlap(mad_fixed_t const output[36], mad_fixed_t overlap[18],
                 mad_fixed_t sample[18][32], unsigned int sb);
#else

/*
 * NAME:        III_overlap()
 * DESCRIPTION: perform overlap-add of windowed IMDCT outputs
 */
static
void III_overlap(mad_fixed_t const output[36], mad_fixed_t overlap[18],
                 mad_fixed_t sample[18][32], unsigned int sb)
{
  unsigned int i;
  for (i = 0; i < 18; ++i) {
    sample[i][sb] = output[i +  0] + overlap[i];
    overlap[i]    = output[i + 18];
  }
}
#endif

/*
 * NAME:        III_overlap_z()
 * DESCRIPTION: perform "overlap-add" of zero IMDCT outputs
 */
static inline
void III_overlap_z(mad_fixed_t overlap[18],
                   mad_fixed_t sample[18][32], unsigned int sb)
{
  unsigned int i;
  for (i = 0; i < 18; ++i) {
    sample[i][sb] = overlap[i];
    overlap[i]    = 0;
  }
}

/*
 * NAME:        III_freqinver()
 * DESCRIPTION: perform subband frequency inversion for odd sample lines
 */
static
void III_freqinver(mad_fixed_t sample[18][32], unsigned int sb)
{
  unsigned int i;
   for (i = 1; i < 18; i += 2)
     sample[i][sb] = -sample[i][sb];
}

/*
 * NAME:        III_decode()
 * DESCRIPTION: decode frame main_data
 */
static
enum mad_error III_decode(struct mad_bitptr *ptr, struct mad_frame *frame,
                          struct sideinfo *si, unsigned int nch)
{
  struct mad_header *header = &frame->header;
  unsigned int sfreqi, ngr, gr;

  {
    unsigned int sfreq;

    sfreq = header->samplerate;
    if (header->flags & MAD_FLAG_MPEG_2_5_EXT)
      sfreq *= 2;

    /* 48000 => 0, 44100 => 1, 32000 => 2,
       24000 => 3, 22050 => 4, 16000 => 5 */
    sfreqi = ((sfreq >>  7) & 0x000f) +
             ((sfreq >> 15) & 0x0001) - 8;

    if (header->flags & MAD_FLAG_MPEG_2_5_EXT)
      sfreqi += 3;
  }

  /* scalefactors, Huffman decoding, requantization */

  ngr = (header->flags & MAD_FLAG_LSF_EXT) ? 1 : 2;

  for (gr = 0; gr < ngr; ++gr) {
    struct granule *granule = &si->gr[gr];
    unsigned char const *sfbwidth[2];
    mad_fixed_t xr[2][576];
    unsigned int ch;
    enum mad_error error;

    for (ch = 0; ch < nch; ++ch) {
      struct channel *channel = &granule->ch[ch];
      unsigned int part2_length;

      sfbwidth[ch] = sfbwidth_table[sfreqi].l;
      if (channel->block_type == 2) {
        sfbwidth[ch] = (channel->flags & mixed_block_flag) ?
          sfbwidth_table[sfreqi].m : sfbwidth_table[sfreqi].s;
      }

      if (header->flags & MAD_FLAG_LSF_EXT) {
        part2_length = III_scalefactors_lsf(ptr, channel,
                                            ch == 0 ? 0 : &si->gr[1].ch[1],
                                            header->mode_extension);
      }
      else {
        part2_length = III_scalefactors(ptr, channel, &si->gr[0].ch[ch],
                                        gr == 0 ? 0 : si->scfsi[ch]);
      }

      error = III_huffdecode(ptr, xr[ch], channel, sfbwidth[ch], part2_length);
      if (error)
        return error;
    }

    /* joint stereo processing */

    if (header->mode == MAD_MODE_JOINT_STEREO && header->mode_extension) {
      error = III_stereo(xr, granule, header, sfbwidth[0]);
      if (error)
        return error;
    }

    /* reordering, alias reduction, IMDCT, overlap-add, frequency inversion */

    for (ch = 0; ch < nch; ++ch) {
      struct channel const *channel = &granule->ch[ch];
      mad_fixed_t (*sample)[32] = &((*frame->sbsample)[ch][18 * gr]);
      unsigned int sb, l, i, sblimit;
      mad_fixed_t output[36];

      if (channel->block_type == 2) {
        III_reorder(xr[ch], channel, sfbwidth[ch]);

# if !defined(OPT_STRICT)
        /*
         * According to ISO/IEC 11172-3, "Alias reduction is not applied for
         * granules with block_type == 2 (short block)." However, other
         * sources suggest alias reduction should indeed be performed on the
         * lower two subbands of mixed blocks. Most other implementations do
         * this, so by default we will too.
         */
        if (channel->flags & mixed_block_flag)
          III_aliasreduce(xr[ch], 36);
# endif
      }
      else
        III_aliasreduce(xr[ch], 576);

      l = 0;

      /* subbands 0-1 */

      if (channel->block_type != 2 || (channel->flags & mixed_block_flag)) {
        unsigned int block_type;

        block_type = channel->block_type;
        if (channel->flags & mixed_block_flag)
          block_type = 0;

        /* long blocks */
        for (sb = 0; sb < 2; ++sb, l += 18) {
          III_imdct_l(&xr[ch][l], output, block_type);
          III_overlap(output, (*frame->overlap)[ch][sb], sample, sb);
        }
      }
      else {
        /* short blocks */
        for (sb = 0; sb < 2; ++sb, l += 18) {
          III_imdct_s(&xr[ch][l], output);
          III_overlap(output, (*frame->overlap)[ch][sb], sample, sb);
        }
      }

      III_freqinver(sample, 1);

      /* (nonzero) subbands 2-31 */

/*
      i = 576;
      while (i > 36 && xr[ch][i - 1] == 0)
        --i;
*/

      {
          /* saves ~600k cycles */
          mad_fixed_t *p = &xr[ch][576];
          mad_fixed_t tmp = xr[ch][35];
          xr[ch][35] = 1;
          while (!*--p);
          xr[ch][35] = tmp;
          i = p - &xr[ch][0] + 1;
      }
      sblimit = 32 - (576 - i) / 18;

      if (channel->block_type != 2) {
        /* long blocks */
        for (sb = 2; sb < sblimit; ++sb, l += 18) {
          III_imdct_l(&xr[ch][l], output, channel->block_type);
          III_overlap(output, (*frame->overlap)[ch][sb], sample, sb);

          if (sb & 1)
            III_freqinver(sample, sb);
        }
      }
      else {
        /* short blocks */
        for (sb = 2; sb < sblimit; ++sb, l += 18) {
          III_imdct_s(&xr[ch][l], output);
          III_overlap(output, (*frame->overlap)[ch][sb], sample, sb);

          if (sb & 1)
            III_freqinver(sample, sb);
        }
      }

      /* remaining (zero) subbands */

      for (sb = sblimit; sb < 32; ++sb) {
        III_overlap_z((*frame->overlap)[ch][sb], sample, sb);

        if (sb & 1)
          III_freqinver(sample, sb);
      }
    }
  }

  return MAD_ERROR_NONE;
}

/*
 * NAME:        layer->III()
 * DESCRIPTION: decode a single Layer III frame
 */
int mad_layer_III(struct mad_stream *stream, struct mad_frame *frame)
{
  struct mad_header *header = &frame->header;
  unsigned int nch, priv_bitlen, next_md_begin = 0;
  unsigned int si_len, data_bitlen, md_len;
  unsigned int frame_space, frame_used, frame_free;
  struct mad_bitptr ptr;
  struct sideinfo si;
  enum mad_error error;
  int result = 0;

#if 0 /* rockbox: do not allocate */
  /* allocate Layer III dynamic structures */
  /* rockbox: not used. Both pointers are linked to an array in
   * ../mpa.c before calling this. */
  if (stream->main_data == 0) {
    stream->main_data = malloc(MAD_BUFFER_MDLEN);
    if (stream->main_data == 0) {
      stream->error = MAD_ERROR_NOMEM;
      return -1;
    }
  }

  if (frame->overlap == 0) {
    frame->overlap = calloc(2 * 32 * 18, sizeof(mad_fixed_t));
    if (frame->overlap == 0) {
      stream->error = MAD_ERROR_NOMEM;
      return -1;
    }
  }
#else
  /* rockbox: This should not happen. Return with error. */
  if ((stream->main_data == 0) || (frame->overlap == 0)) {
      stream->error = MAD_ERROR_NOMEM;
      return -1;
  }
#endif /* rockbox */

  nch = MAD_NCHANNELS(header);
  si_len = (header->flags & MAD_FLAG_LSF_EXT) ?
    (nch == 1 ? 9 : 17) : (nch == 1 ? 17 : 32);

  /* check frame sanity */

  if (stream->next_frame - mad_bit_nextbyte(&stream->ptr) <
      (signed int) si_len) {
    stream->error = MAD_ERROR_BADFRAMELEN;
    stream->md_len = 0;
    return -1;
  }

  /* check CRC word */

  if (header->flags & MAD_FLAG_PROTECTION) {
    header->crc_check =
      mad_bit_crc(stream->ptr, si_len * CHAR_BIT, header->crc_check);

    if (header->crc_check != header->crc_target &&
        !(frame->options & MAD_OPTION_IGNORECRC)) {
      stream->error = MAD_ERROR_BADCRC;
      result = -1;
    }
  }

  /* decode frame side information */

  error = III_sideinfo(&stream->ptr, nch, header->flags & MAD_FLAG_LSF_EXT,
                       &si, &data_bitlen, &priv_bitlen);
  if (error && result == 0) {
    stream->error = error;
    result = -1;
  }

  header->flags        |= priv_bitlen;
  header->private_bits |= si.private_bits;

  /* find main_data of next frame */

  {
    struct mad_bitptr peek;
    unsigned long header;

    mad_bit_init(&peek, stream->next_frame);

    header = mad_bit_read(&peek, 32);
    if ((header & 0xffe60000L) /* syncword | layer */ == 0xffe20000L) {
      if (!(header & 0x00010000L))  /* protection_bit */
        mad_bit_skip(&peek, 16);  /* crc_check */

      next_md_begin =
        mad_bit_read(&peek, (header & 0x00080000L) /* ID */ ? 9 : 8);
    }

    mad_bit_finish(&peek);
  }

  /* find main_data of this frame */

  frame_space = stream->next_frame - mad_bit_nextbyte(&stream->ptr);

  if (next_md_begin > si.main_data_begin + frame_space)
    next_md_begin = 0;

  md_len = si.main_data_begin + frame_space - next_md_begin;

  frame_used = 0;

  if (si.main_data_begin == 0) {
    ptr = stream->ptr;
    stream->md_len = 0;

    frame_used = md_len;
  }
  else {
    if (si.main_data_begin > stream->md_len) {
      if (result == 0) {
        stream->error = MAD_ERROR_BADDATAPTR;
        result = -1;
      }
    }
    else {
      mad_bit_init(&ptr,
                   *stream->main_data + stream->md_len - si.main_data_begin);

      if (md_len > si.main_data_begin) {
        assert(stream->md_len + md_len -
               si.main_data_begin <= MAD_BUFFER_MDLEN);

        memcpy(*stream->main_data + stream->md_len,
               mad_bit_nextbyte(&stream->ptr),
               frame_used = md_len - si.main_data_begin);
        stream->md_len += frame_used;
      }
    }
  }

  frame_free = frame_space - frame_used;

  /* decode main_data */

  if (result == 0) {
    error = III_decode(&ptr, frame, &si, nch);
    if (error) {
      stream->error = error;
      result = -1;
    }

    /* designate ancillary bits */

    stream->anc_ptr    = ptr;
    stream->anc_bitlen = md_len * CHAR_BIT - data_bitlen;
  }

# if 0 && defined(DEBUG)
  fprintf(stderr,
          "main_data_begin:%u, md_len:%u, frame_free:%u, "
          "data_bitlen:%u, anc_bitlen: %u\n",
          si.main_data_begin, md_len, frame_free,
          data_bitlen, stream->anc_bitlen);
# endif

  /* preload main_data buffer with up to 511 bytes for next frame(s) */

  if (frame_free >= next_md_begin) {
    memcpy(*stream->main_data,
           stream->next_frame - next_md_begin, next_md_begin);
    stream->md_len = next_md_begin;
  }
  else {
    if (md_len < si.main_data_begin) {
      unsigned int extra;

      extra = si.main_data_begin - md_len;
      if (extra + frame_free > next_md_begin)
        extra = next_md_begin - frame_free;

      if (extra < stream->md_len) {
        memmove(*stream->main_data,
                *stream->main_data + stream->md_len - extra, extra);
        stream->md_len = extra;
      }
    }
    else
      stream->md_len = 0;

    memcpy(*stream->main_data + stream->md_len,
           stream->next_frame - frame_free, frame_free);
    stream->md_len += frame_free;
  }

  return result;
}
