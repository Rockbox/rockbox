/*
 * WMA compatible decoder
 * Copyright (c) 2002 The FFmpeg Project.
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

/* these are the sin and cos rotations used by the MDCT
   Accessed too infrequently to give much speedup in IRAM */
/* Fixme - unify all this with the twiddle arrays used by the fft.
   It should be straightforward to flatten everything into a single
   table of sin/cos factors of maximum size = maximum block size
   (Reason: sin and cos factors are symmetric so don't need to store both
            cos itself has 8-way symmetry so only need to store 1/8th of them
            and the mdct rotations require 1/8th fractional steps so 8
            times as many points (so you're at N*8/8 = N) */
            
/* NOTE: data format of mdct trig tables is s.31 */
            
fixed32 *tcosarray[5], *tsinarray[5];
fixed32 tcos0[1024], tcos1[512], tcos2[256], tcos3[128], tcos4[64];
fixed32 tsin0[1024], tsin1[512], tsin2[256], tsin3[128], tsin4[64];

/**
 * init MDCT or IMDCT computation.
 */
int ff_mdct_init(MDCTContext *s, int nbits, int inverse)
{
    int n, n4, i;

    memset(s, 0, sizeof(*s));
    n = 1 << nbits;            //nbits ranges from 12 to 8 inclusive

    s->nbits = nbits;
    s->n = n;
    n4 = n>>2;
    s->tcos = tcosarray[12-nbits];
    s->tsin = tsinarray[12-nbits];
    
    unsigned long phase;
    
    /* NOTE THAT THESE TWIDDLE FACTORS HAVE NOW CHANGED !! */
    for(i=0;i<n4;i++)
    {
      /* phase = (i+(1/4))/N in 32bit range (e.g. 0x0=0deg, 0x80000000=180deg)
         Shifting by <<16 in two stages works so long as largest nbits < 16
         (which it is).
         Note - this has NOTHING to do with fixmul32/PRECISION/etc
         (see definition of fsincos function below)               */
      phase = ( ((i<<16) + 0x4000) >> nbits ) << 16;
      /* tsin and tcos are just reflections of each other with a phase shift
         so should be able to optimise to reduce storage */
      /* NOTE the required tables are actually -cos and -sin , so we need to 
         flip sign of the result of the fsincos calculation.
         Alternatively could flip at the calculation step instead */
      s->tsin[i] = -fsincos(phase, &(s->tcos[i]));
      s->tcos[i] *= (-1);
    }
    (&s->fft)->nbits = nbits-2;
    (&s->fft)->inverse = inverse;

    ff_fft_init((void *)(&s->fft), s->nbits - 2, 1);

    return 0;
}

/**
 * Compute the middle half of the inverse MDCT of size N = 2^nbits
 * thus excluding the parts that can be derived by symmetry
 * @param output N/2 samples
 * @param input N/2 samples
 */
void ff_imdct_half(MDCTContext *s, fixed32 *output, const fixed32 *input)
{
    int k, n8, n4, n2, n, j;
    const fixed32 *tcos = s->tcos;
    const fixed32 *tsin = s->tsin;
    
    const fixed32 *in1, *in2;
    
    n = 1 << s->nbits;

    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    /* function can work in one of two ways, either return the whole N-point
       imdct, or just the unique N/2-point imdct part (without reflections)
       If the former, the output buffer needs to be size N/2.
       If the latter, the output buffer needs to be size N, but all the imdct
       processing except the final reflection happens in the N/2 samples beginning
       at N/4  (so range N/4 to 3N/4 in the output buffer) */
    FFTComplex *z = (FFTComplex *)output;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
    /* careful: fft is initialised for a different number of bits here
       so have to use s->fft->nbits not s->nbits .. */
    const int revtab_shift = (12- s->fft.nbits);
    
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
    const int step = (12-s->nbits)<<1;

    const uint16_t * p_revtab=s->fft.revtab;
    {
        const uint16_t * const p_revtab_end = p_revtab + n8;
        while(p_revtab < p_revtab_end)
        {
            j = (*p_revtab)>>revtab_shift;
            CMUL(&z[j].re, &z[j].im, *in2, *in1, -T[1], -T[0] );
            T += step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
            j = (*p_revtab)>>revtab_shift;
            CMUL(&z[j].re, &z[j].im, *in2, *in1, -T[1], -T[0] );
            T += step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
        }
    }
    {
        const uint16_t * const p_revtab_end = p_revtab + n8;
        while(p_revtab < p_revtab_end)
        {
            j = (*p_revtab)>>revtab_shift;
            CMUL(&z[j].re, &z[j].im, *in2, *in1, -T[0], -T[1] );
            T -= step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
            j = (*p_revtab)>>revtab_shift;
            CMUL(&z[j].re, &z[j].im, *in2, *in1, -T[0], -T[1] );
            T -= step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
        }
    }


    /* ... and so fft runs in OUTPUT buffer */
    ff_fft_calc_c(&s->fft, z);

    /* post rotation + reordering.  now keeps the result within the OUTPUT buffer */
    for(k = 0; k < n8; k++)
    {
        /* remember, tcos and tsin are really -cos and -sin 
           so what we're actually calculating is {re,im) x {-cos,-sin} */
        fixed32 r0,i0,r1,i1;
        XNPROD31_R(z[n8-k-1].im, z[n8-k-1].re, tsin[n8-k-1], tcos[n8-k-1], r0, i1 );
        XNPROD31_R(z[n8+k].im,   z[n8+k].re,   tsin[n8+k],   tcos[n8+k],   r1, i0 );
        z[n8-k-1].re = r0;
        z[n8-k-1].im = i0;
        z[n8+k].re   = r1;
        z[n8+k].im   = i1;
    }
} 

/**
 * Compute inverse MDCT of size N = 2^nbits
 * @param output N samples
 * @param input N/2 samples
 */
void ff_imdct_calc(MDCTContext *s, fixed32 *output, const fixed32 *input)
{
    const int n = (1<<s->nbits);
    const int n2 = (n>>1);
    const int n4 = (n>>2);
    
    ff_imdct_half(s,output+n4,input);

    /* reflect the half imdct into the full N samples */
    /* TODO: this could easily be optimised more! */
    fixed32 * in_r = output + n2;
    output += n-4;

    while(output>in_r)
    {
        output[3] = in_r[0];
        output[2] = in_r[1];
        output[1] = in_r[2];
        output[0] = in_r[3];
        in_r +=4;
        output -= 4;
    }
    output-=n2-n4+4;
    in_r=output+n2-4;
    while(in_r<output)
    {
        output[0]     = -in_r[3];
        output[1]     = -in_r[2];
        output[2]     = -in_r[1];
        output[3]     = -in_r[0];
        in_r -= 4;
        output += 4;
    }
}

/* init MDCT */
int mdct_init_global(void)
{
    /* although seemingly degenerate, these cannot actually be merged
       together without a substantial increase in error which is unjustified
       by the tiny memory savings */
    /* FIXME: Except, that's not true, and these can be merged/flattened */
    tcosarray[0] = tcos0;
    tcosarray[1] = tcos1;
    tcosarray[2] = tcos2;
    tcosarray[3] = tcos3;
    tcosarray[4] = tcos4;
    tsinarray[0] = tsin0;
    tsinarray[1] = tsin1;
    tsinarray[2] = tsin2;
    tsinarray[3] = tsin3;
    tsinarray[4] = tsin4;

    return 0;
}

const long cordic_circular_gain = 0xb2458939; /* 0.607252929 */

/* Table of values of atan(2^-i) in 0.32 format fractions of pi where pi = 0xffffffff / 2 */
const unsigned long atan_table[] = {
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

/* FIXME: yeah, all of this, should just be a fixed table... */
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
