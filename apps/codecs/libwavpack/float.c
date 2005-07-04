////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2004 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// float.c

#include "wavpack.h"

int read_float_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    char *byteptr = wpmd->data;

    if (bytecnt != 4)
        return FALSE;

    wps->float_flags = *byteptr++;
    wps->float_shift = *byteptr++;
    wps->float_max_exp = *byteptr++;
    wps->float_norm_exp = *byteptr;
    return TRUE;
}

void float_values (WavpackStream *wps, long *values, long num_values)
{
    while (num_values--) {
        int shift_count = 0, exp = wps->float_max_exp;
        f32 outval = { 0, 0, 0 };

        if (*values) {
            *values <<= wps->float_shift;

            if (*values < 0) {
                *values = -*values;
                outval.sign = 1;
            }

            if (*values == 0x1000000)
                outval.exponent = 255;
            else {
                if (exp)
                    while (!(*values & 0x800000) && --exp) {
                        shift_count++;
                        *values <<= 1;
                    }

                if (shift_count && (wps->float_flags & FLOAT_SHIFT_ONES))
                    *values |= ((1 << shift_count) - 1);

                outval.mantissa = *values;
                outval.exponent = exp;
            }
        }

        * (f32 *) values++ = outval;
    }
}

void float_normalize (long *values, long num_values, int delta_exp)
{
    f32 *fvalues = (f32 *) values, fzero = { 0, 0, 0 };
    int exp;

    if (!delta_exp)
        return;

    while (num_values--) {
        if ((exp = fvalues->exponent) == 0 || exp + delta_exp <= 0)
            *fvalues = fzero;
        else if (exp == 255 || (exp += delta_exp) >= 255) {
            fvalues->exponent = 255;
            fvalues->mantissa = 0;
        }
        else
            fvalues->exponent = exp;

        fvalues++;
    }
}
