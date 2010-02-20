/*
 * Fixed Point IMDCT 
 * Copyright (c) 2002 The FFmpeg Project.
 * Copyright (c) 2010 Dave Hooper, Mohamed Tarek, Michael Giacomelli
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "codeclib.h"
#include "mdct.h"
#include "asm_arm.h"
#include "asm_mcf5249.h"
#include "codeclib_misc.h"
#include "mdct_lookup.h"

#ifndef ICODE_ATTR_TREMOR_MDCT
#define ICODE_ATTR_TREMOR_MDCT ICODE_ATTR
#endif

/**
 * Compute the middle half of the inverse MDCT of size N = 2^nbits
 * thus excluding the parts that can be derived by symmetry
 * @param output N/2 samples
 * @param input N/2 samples
 *
 * NOTE - CANNOT CURRENTLY OPERATE IN PLACE (input and output must
 *                                          not overlap or intersect at all)
 */
void ff_imdct_half(unsigned int nbits, fixed32 *output, const fixed32 *input) ICODE_ATTR_TREMOR_MDCT;
void ff_imdct_half(unsigned int nbits, fixed32 *output, const fixed32 *input)
{
    int n8, n4, n2, n, j;
    const fixed32 *in1, *in2;
    
    n = 1 << nbits;

    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    FFTComplex *z = (FFTComplex *)output;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
    
    /* revtab comes from the fft; revtab table is sized for N=4096 size fft = 2^12.
       The fft is size N/4 so s->nbits-2, so our shift needs to be (12-(nbits-2)) */
    const int revtab_shift = (14- nbits);
    
    /* bitreverse reorder the input and rotate;   result here is in OUTPUT ... */
    /* (note that when using the current split radix, the bitreverse ordering is
        complex, meaning that this reordering cannot easily be done in-place) */
    /* Using the following pdf, you can see that it is possible to rearrange
       the 'classic' pre/post rotate with an alternative one that enables
       us to use fewer distinct twiddle factors.
       http://www.eurasip.org/Proceedings/Eusipco/Eusipco2006/papers/1568980508.pdf
       
       For prerotation, the factors are just sin,cos(2PI*i/N)
       For postrotation, the factors are sin,cos(2PI*(i+1/4)/N)
       
       Therefore, prerotation can immediately reuse the same twiddles as fft
       (for postrotation it's still a bit complex, so this is still using
        an mdct-local set of twiddles to do that part)
       */
    const int32_t *T = sincos_lookup0;
    const int step = 2<<(12-nbits);
    const uint16_t * p_revtab=revtab;
    {
        const uint16_t * const p_revtab_end = p_revtab + n8;
        while(LIKELY(p_revtab < p_revtab_end))
        {
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[1], T[0], &z[j].re, &z[j].im );
            T += step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[1], T[0], &z[j].re, &z[j].im );
            T += step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
        }
    }
    {
        const uint16_t * const p_revtab_end = p_revtab + n8;
        while(LIKELY(p_revtab < p_revtab_end))
        {
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[0], T[1], &z[j].re, &z[j].im);
            T -= step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[0], T[1], &z[j].re, &z[j].im);
            T -= step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
        }
    }


    /* ... and so fft runs in OUTPUT buffer */
    ff_fft_calc_c(nbits-2, z);

    /* post rotation + reordering.  now keeps the result within the OUTPUT buffer */
    switch( nbits )
    {
        default:
        {
            fixed32 * z1 = (fixed32 *)(&z[0]);
            fixed32 * z2 = (fixed32 *)(&z[n4-1]);
            int magic_step = step>>2;
            int newstep;
            if(n<=1024)
            {
                T = sincos_lookup0 + magic_step;
                newstep = step>>1;
            }
            else
            {   
                 T = sincos_lookup1;
                 newstep = 2;
            }
        
            while(z1<z2)
            {
                fixed32 r0,i0,r1,i1;
                XNPROD31_R(z1[1], z1[0], T[0], T[1], r0, i1 ); T+=newstep;
                XNPROD31_R(z2[1], z2[0], T[1], T[0], r1, i0 ); T+=newstep;
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
            }
            
            break;
        }

        case 12: /* n=4096 */
        {
            /* linear interpolation (50:50) between sincos_lookup0 and sincos_lookup1 */
            const int32_t * V = sincos_lookup1;
            T = sincos_lookup0;
            int32_t t0,t1,v0,v1;
            fixed32 * z1 = (fixed32 *)(&z[0]);
            fixed32 * z2 = (fixed32 *)(&z[n4-1]);

            t0 = T[0]>>1; t1=T[1]>>1;
        
            while(z1<z2)
            {
                fixed32 r0,i0,r1,i1;
                t0 += (v0 = (V[0]>>1));
                t1 += (v1 = (V[1]>>1));
                XNPROD31_R(z1[1], z1[0], t0, t1, r0, i1 );
                T+=2;
                v0 += (t0 = (T[0]>>1));
                v1 += (t1 = (T[1]>>1));
                XNPROD31_R(z2[1], z2[0], v1, v0, r1, i0 );
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
                V+=2;
            }
            
            break;
        }
        
        case 13: /* n = 8192 */
        {
            /* weight linear interpolation between sincos_lookup0 and sincos_lookup1
               specifically: 25:75 for first twiddle and 75:25 for second twiddle */
            const int32_t * V = sincos_lookup1;
            T = sincos_lookup0;
            int32_t t0,t1,v0,v1,q0,q1;
            fixed32 * z1 = (fixed32 *)(&z[0]);
            fixed32 * z2 = (fixed32 *)(&z[n4-1]);

            t0 = T[0]; t1=T[1];
        
            while(z1<z2)
            {
                fixed32 r0,i0,r1,i1;
                v0 = V[0]; v1 = V[1];
                t0 += (q0 = (v0-t0)>>1);
                t1 += (q1 = (v1-t1)>>1);
                XNPROD31_R(z1[1], z1[0], t0, t1, r0, i1 );
                t0 = v0-q0;
                t1 = v1-q1;
                XNPROD31_R(z2[1], z2[0], t1, t0, r1, i0 );
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
                T+=2;
                
                t0 = T[0]; t1 = T[1];
                v0 += (q0 = (t0-v0)>>1);
                v1 += (q1 = (t1-v1)>>1);
                XNPROD31_R(z1[1], z1[0], v0, v1, r0, i1 );
                v0 = t0-q0;
                v1 = t1-q1;
                XNPROD31_R(z2[1], z2[0], v1, v0, r1, i0 );
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
                V+=2;
            }
               
            break;
        }
    }
} 

/**
 * Compute inverse MDCT of size N = 2^nbits
 * @param output N samples
 * @param input N/2 samples
 * "In-place" processing can be achieved provided that:
 *            [0  ..  N/2-1 | N/2  ..  N-1 ]
 *            <----input---->
 *            <-----------output----------->
 *
 */
void ff_imdct_calc(unsigned int nbits, fixed32 *output, const fixed32 *input) ICODE_ATTR_TREMOR_MDCT;
void ff_imdct_calc(unsigned int nbits, fixed32 *output, const fixed32 *input)
{
    const int n = (1<<nbits);
    const int n2 = (n>>1);
    const int n4 = (n>>2);
    
    ff_imdct_half(nbits,output+n2,input);

    /* reflect the half imdct into the full N samples */
    /* TODO: this could easily be optimised more! */
    fixed32 * in_r, * in_r2, * out_r, * out_r2;

    out_r = output;
    out_r2 = output+n2-8;
    in_r  = output+n2+n4-8;
    while(out_r<out_r2)
    {
        out_r[0]     = -(out_r2[7] = in_r[7]);
        out_r[1]     = -(out_r2[6] = in_r[6]);
        out_r[2]     = -(out_r2[5] = in_r[5]);
        out_r[3]     = -(out_r2[4] = in_r[4]);
        out_r[4]     = -(out_r2[3] = in_r[3]);
        out_r[5]     = -(out_r2[2] = in_r[2]);
        out_r[6]     = -(out_r2[1] = in_r[1]);
        out_r[7]     = -(out_r2[0] = in_r[0]);
        in_r -= 8;
        out_r += 8;
        out_r2 -= 8;
    }

    in_r = output + n2+n4;
    in_r2 = output + n-4;
    out_r = output + n2;
    out_r2 = output + n2 + n4 - 4;
    while(in_r<in_r2)
    {
        register fixed32 t0,t1,t2,t3;
        register fixed32 s0,s1,s2,s3;

        //simultaneously do the following things:
        // 1. copy range from [n2+n4 .. n-1] to range[n2 .. n2+n4-1]
        // 2. reflect range from [n2+n4 .. n-1] inplace
        //
        //  [                      |                        ]
        //   ^a ->            <- ^b ^c ->               <- ^d
        //
        //  #1: copy from ^c to ^a
        //  #2: copy from ^d to ^b
        //  #3: swap ^c and ^d in place
        //
        // #1 pt1 : load 4 words from ^c.
        t0=in_r[0]; t1=in_r[1]; t2=in_r[2]; t3=in_r[3];
        // #1 pt2 : write to ^a
        out_r[0]=t0;out_r[1]=t1;out_r[2]=t2;out_r[3]=t3;
        // #2 pt1 : load 4 words from ^d
        s0=in_r2[0];s1=in_r2[1];s2=in_r2[2];s3=in_r2[3];
        // #2 pt2 : write to ^b
        out_r2[0]=s0;out_r2[1]=s1;out_r2[2]=s2;out_r2[3]=s3;
        // #3 pt1 : write words from #2 to ^c
        in_r[0]=s3;in_r[1]=s2;in_r[2]=s1;in_r[3]=s0;
        // #3 pt2 : write words from #1 to ^d
        in_r2[0]=t3;in_r2[1]=t2;in_r2[2]=t1;in_r2[3]=t0;

        in_r += 4;
        in_r2 -= 4;
        out_r += 4;
        out_r2 -= 4;
    }
}

static const long cordic_circular_gain = 0xb2458939; /* 0.607252929 */

/* Table of values of atan(2^-i) in 0.32 format fractions of pi where pi = 0xffffffff / 2 */
static const unsigned long atan_table[] = {
    0x1fffffff, /* +0.785398163 (or pi/4) */
    0x12e4051d, /* +0.463647609 */
    0x09fb385b, /* +0.244978663 */
    0x051111d4, /* +0.124354995 */
    0x028b0d43, /* +0.062418810 */
    0x0145d7e1, /* +0.031239833 */
    0x00a2f61e, /* +0.015623729 */
    0x00517c55, /* +0.007812341 */
    0x0028be53, /* +0.003906230 */
    0x00145f2e, /* +0.001953123 */
    0x000a2f98, /* +0.000976562 */
    0x000517cc, /* +0.000488281 */
    0x00028be6, /* +0.000244141 */
    0x000145f3, /* +0.000122070 */
    0x0000a2f9, /* +0.000061035 */
    0x0000517c, /* +0.000030518 */
    0x000028be, /* +0.000015259 */
    0x0000145f, /* +0.000007629 */
    0x00000a2f, /* +0.000003815 */
    0x00000517, /* +0.000001907 */
    0x0000028b, /* +0.000000954 */
    0x00000145, /* +0.000000477 */
    0x000000a2, /* +0.000000238 */
    0x00000051, /* +0.000000119 */
    0x00000028, /* +0.000000060 */
    0x00000014, /* +0.000000030 */
    0x0000000a, /* +0.000000015 */
    0x00000005, /* +0.000000007 */
    0x00000002, /* +0.000000004 */
    0x00000001, /* +0.000000002 */
    0x00000000, /* +0.000000001 */
    0x00000000, /* +0.000000000 */
};

/**
 * Implements sin and cos using CORDIC rotation.
 *
 * @param phase has range from 0 to 0xffffffff, representing 0 and
 *        2*pi respectively.
 * @param cos return address for cos
 * @return sin of phase, value is a signed value from LONG_MIN to LONG_MAX,
 *         representing -1 and 1 respectively.
 *
 *        Gives at least 24 bits precision (last 2-8 bits or so are probably off)
 */

long fsincos(unsigned long phase, fixed32 *cos)
{
    int32_t x, x1, y, y1;
    unsigned long z, z1;
    int i;

    /* Setup initial vector */
    x = cordic_circular_gain;
    y = 0;
    z = phase;

    /* The phase has to be somewhere between 0..pi for this to work right */
    if (z < 0xffffffff / 4) {
        /* z in first quadrant, z += pi/2 to correct */
        x = -x;
        z += 0xffffffff / 4;
    } else if (z < 3 * (0xffffffff / 4)) {
        /* z in third quadrant, z -= pi/2 to correct */
        z -= 0xffffffff / 4;
    } else {
        /* z in fourth quadrant, z -= 3pi/2 to correct */
        x = -x;
        z -= 3 * (0xffffffff / 4);
    }

    /* Each iteration adds roughly 1-bit of extra precision */
    for (i = 0; i < 31; i++) {
        x1 = x >> i;
        y1 = y >> i;
        z1 = atan_table[i];

        /* Decided which direction to rotate vector. Pivot point is pi/2 */
        if (z >= 0xffffffff / 4) {
            x -= y1;
            y += x1;
            z -= z1;
        } else {
            x += y1;
            y -= x1;
            z += z1;
        }
    }

    if (cos)
        *cos = x;

    return y;
}
