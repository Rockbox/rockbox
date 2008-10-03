/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#include <string.h>
#include <inttypes.h>

#include "demac.h"
#include "filter.h"

#ifdef CPU_COLDFIRE
#include "vector_math16_cf.h"
#elif ARM_ARCH >= 6
#include "vector_math16_armv6.h"
#elif defined CPU_ARM7TDMI
#include "vector_math16_arm7.h"
#else
#include "vector_math16.h"
#endif

struct filter_t {
    int16_t* coeffs; /* ORDER entries */

    /* We store all the filter delays in a single buffer */
    int16_t* historybuffer;  /* ORDER*2 + HISTORY_SIZE entries */

    int16_t* delay;
    int16_t* adaptcoeffs;

    int avg;
};

/* We name the functions according to the ORDER and FRACBITS
   pre-processor symbols and build multiple .o files from this .c file
   - this increases code-size but gives the compiler more scope for
   optimising the individual functions, as well as replacing a lot of
   variables with constants.
*/

#if FRACBITS == 11
  #if ORDER == 16
     #define INIT_FILTER   init_filter_16_11
     #define APPLY_FILTER apply_filter_16_11
  #elif ORDER == 64
     #define INIT_FILTER  init_filter_64_11
     #define APPLY_FILTER apply_filter_64_11
  #endif
#elif FRACBITS == 13
  #define INIT_FILTER  init_filter_256_13
  #define APPLY_FILTER apply_filter_256_13
#elif FRACBITS == 10
  #define INIT_FILTER  init_filter_32_10
  #define APPLY_FILTER apply_filter_32_10
#elif FRACBITS == 15
  #define INIT_FILTER  init_filter_1280_15
  #define APPLY_FILTER apply_filter_1280_15
#endif

/* Some macros to handle the fixed-point stuff */

/* Convert from (32-FRACBITS).FRACBITS fixed-point format to an
   integer (rounding to nearest). */
#define FP_HALF  (1 << (FRACBITS - 1))   /* 0.5 in fixed-point format. */
#define FP_TO_INT(x) ((x + FP_HALF) >> FRACBITS);  /* round(x) */

#define SATURATE(x) (int16_t)(((x) == (int16_t)(x)) ? (x) : ((x) >> 31) ^ 0x7FFF);

/* Apply the filter with state f to count entries in data[] */

static void do_apply_filter_3980(struct filter_t* f, int32_t* data, int count)
{
    int res;
    int absres;

#ifdef PREPARE_SCALARPRODUCT
    PREPARE_SCALARPRODUCT
#endif

    while(count--)
    {
        res = FP_TO_INT(scalarproduct(f->coeffs, f->delay - ORDER));

        if (*data < 0)
            vector_add(f->coeffs, f->adaptcoeffs - ORDER);
        else if (*data > 0)
            vector_sub(f->coeffs, f->adaptcoeffs - ORDER);

        res += *data;

        *data++ = res;

        /* Update the output history */
        *f->delay++ = SATURATE(res);

        /* Version 3.98 and later files */

        /* Update the adaption coefficients */
        absres = (res < 0 ? -res : res);

        if (absres > (f->avg * 3))
            *f->adaptcoeffs = ((res >> 25) & 64) - 32;
        else if (absres > (f->avg * 4) / 3)
            *f->adaptcoeffs = ((res >> 26) & 32) - 16;
        else if (absres > 0)
            *f->adaptcoeffs = ((res >> 27) & 16) - 8;
        else
            *f->adaptcoeffs = 0;

        f->avg += (absres - f->avg) / 16;

        f->adaptcoeffs[-1] >>= 1;
        f->adaptcoeffs[-2] >>= 1;
        f->adaptcoeffs[-8] >>= 1;

        f->adaptcoeffs++;

        /* Have we filled the history buffer? */
        if (f->delay == f->historybuffer + HISTORY_SIZE + (ORDER*2)) {
            memmove(f->historybuffer, f->delay - (ORDER*2), 
                    (ORDER*2) * sizeof(int16_t));
            f->delay = f->historybuffer + ORDER*2;
            f->adaptcoeffs = f->historybuffer + ORDER;
        }
    }
}

static void do_apply_filter_3970(struct filter_t* f, int32_t* data, int count)
{
    int res;
    
#ifdef PREPARE_SCALARPRODUCT
    PREPARE_SCALARPRODUCT
#endif

    while(count--)
    {
        res = FP_TO_INT(scalarproduct(f->coeffs, f->delay - ORDER));

        if (*data < 0)
            vector_add(f->coeffs, f->adaptcoeffs - ORDER);
        else if (*data > 0)
            vector_sub(f->coeffs, f->adaptcoeffs - ORDER);

        /* Convert res from (32-FRACBITS).FRACBITS fixed-point format to an
           integer (rounding to nearest) and add the input value to
           it */
        res += *data;

        *data++ = res;

        /* Update the output history */
        *f->delay++ = SATURATE(res);

        /* Version ??? to < 3.98 files (untested) */
        f->adaptcoeffs[0] = (res == 0) ? 0 : ((res >> 28) & 8) - 4;
        f->adaptcoeffs[-4] >>= 1;
        f->adaptcoeffs[-8] >>= 1;

        f->adaptcoeffs++;

        /* Have we filled the history buffer? */
        if (f->delay == f->historybuffer + HISTORY_SIZE + (ORDER*2)) {
            memmove(f->historybuffer, f->delay - (ORDER*2), 
                    (ORDER*2) * sizeof(int16_t));
            f->delay = f->historybuffer + ORDER*2;
            f->adaptcoeffs = f->historybuffer + ORDER;
        }
    }
}

static struct filter_t filter0 IBSS_ATTR;
static struct filter_t filter1 IBSS_ATTR;

static void do_init_filter(struct filter_t* f, int16_t* buf)
{
    f->coeffs = buf;
    f->historybuffer = buf + ORDER;

    /* Zero the output history buffer */
    memset(f->historybuffer, 0 , (ORDER*2) * sizeof(int16_t));
    f->delay = f->historybuffer + ORDER*2;
    f->adaptcoeffs = f->historybuffer + ORDER;

    /* Zero the co-efficients  */
    memset(f->coeffs, 0, ORDER * sizeof(int16_t));

    /* Zero the running average */
    f->avg = 0;
}

void INIT_FILTER(int16_t* buf)
{
    do_init_filter(&filter0, buf);
    do_init_filter(&filter1, buf + ORDER * 3 + HISTORY_SIZE);
}

int APPLY_FILTER(int fileversion, int32_t* data0, int32_t* data1, int count)
{
    if (fileversion >= 3980) {
        do_apply_filter_3980(&filter0, data0, count);
        if (data1 != NULL)
            do_apply_filter_3980(&filter1, data1, count);
    } else {
        do_apply_filter_3970(&filter0, data0, count);
        if (data1 != NULL)
            do_apply_filter_3970(&filter1, data1, count);
    }

    return 0;
}
