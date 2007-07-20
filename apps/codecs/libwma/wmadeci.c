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

/**
 * @file wmadec.c
 * WMA compatible decoder.
 */

#include <codecs.h>
#include <codecs/lib/codeclib.h>
#include "asf.h"
#include "wmadec.h"
#include "wmafixed.c"
#include "bitstream.h"


#define VLCBITS 9
#define VLCMAX ((22+VLCBITS-1)/VLCBITS)

#define EXPVLCBITS 9
#define EXPMAX ((19+EXPVLCBITS-1)/EXPVLCBITS)

#define HGAINVLCBITS 9
#define HGAINMAX ((13+HGAINVLCBITS-1)/HGAINVLCBITS)


#ifdef CPU_ARM
static inline
void CMUL(fixed32 *x, fixed32 *y,
          fixed32  a, fixed32  b,
          fixed32  t, fixed32  v)
{
    /* This version loses one bit of precision. Could be solved at the cost
     * of 2 extra cycles if it becomes an issue. */
    int x1, y1, l;
    asm(
        "smull    %[l], %[y1], %[b], %[t] \n"
        "smlal    %[l], %[y1], %[a], %[v] \n"
        "rsb      %[b], %[b], #0          \n"
        "smull    %[l], %[x1], %[a], %[t] \n"
        "smlal    %[l], %[x1], %[b], %[v] \n"
        : [l] "=&r" (l), [x1]"=&r" (x1), [y1]"=&r" (y1), [b] "+r" (b)
        : [a] "r" (a),   [t] "r" (t),    [v] "r" (v)
        : "cc"
    );
    *x = x1 << 1;
    *y = y1 << 1;
}
#elif defined CPU_COLDFIRE
static inline
void CMUL(fixed32 *x, fixed32 *y,
          fixed32  a, fixed32  b,
          fixed32  t, fixed32  v)
{
  asm volatile ("mac.l %[a], %[t], %%acc0;"
                "msac.l %[b], %[v], %%acc0;"
                "mac.l %[b], %[t], %%acc1;"
                "mac.l %[a], %[v], %%acc1;"
                "movclr.l %%acc0, %[a];"
                "move.l %[a], (%[x]);"
                "movclr.l %%acc1, %[a];"
                "move.l %[a], (%[y]);"
                : [a] "+&r" (a)
                : [x] "a" (x), [y] "a" (y),
                  [b] "r" (b), [t] "r" (t), [v] "r" (v)
                : "cc", "memory");
}
#else
// PJJ : reinstate macro
void CMUL(fixed32 *pre,
          fixed32 *pim,
          fixed32 are,
          fixed32 aim,
          fixed32 bre,
          fixed32 bim)
{
    //int64_t x,y;
    fixed32 _aref = are;
    fixed32 _aimf = aim;
    fixed32 _bref = bre;
    fixed32 _bimf = bim;
    fixed32 _r1 = fixmul32b(_bref, _aref);
    fixed32 _r2 = fixmul32b(_bimf, _aimf);
    fixed32 _r3 = fixmul32b(_bref, _aimf);
    fixed32 _r4 = fixmul32b(_bimf, _aref);
    *pre = _r1 - _r2;
    *pim = _r3 + _r4;

}
#endif

typedef struct CoefVLCTable
{
    int n; /* total number of codes */
    const uint32_t *huffcodes; /* VLC bit values */
    const uint8_t *huffbits;   /* VLC bit size */
    const uint16_t *levels; /* table to build run/level tables */
}
CoefVLCTable;

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len);
int fft_calc(FFTContext *s, FFTComplex *z);


fixed32 coefsarray[MAX_CHANNELS][BLOCK_MAX_SIZE] IBSS_ATTR;

//static variables that replace malloced stuff
fixed32 stat0[2048], stat1[1024], stat2[512], stat3[256], stat4[128];    //these are the MDCT reconstruction windows

fixed32 *tcosarray[5], *tsinarray[5];
fixed32 tcos0[1024], tcos1[512], tcos2[256], tcos3[128], tcos4[64];        //these are the sin and cos rotations used by the MDCT
fixed32 tsin0[1024], tsin1[512], tsin2[256], tsin3[128], tsin4[64];

FFTComplex *exparray[5];                                    //these are the fft lookup tables

uint16_t *revarray[5];

FFTComplex  exptab0[512] IBSS_ATTR;
uint16_t revtab0[1024];

uint16_t *runtabarray[2], *levtabarray[2];                                        //these are VLC lookup tables

uint16_t runtab0[1336], runtab1[1336], levtab0[1336], levtab1[1336];                //these could be made smaller since only one can be 1336

FFTComplex mdct_tmp[1] ;              /* dummy var */

//may also be too large by ~ 1KB each?
static VLC_TYPE vlcbuf1[6144][2];
static VLC_TYPE vlcbuf2[3584][2];
static VLC_TYPE vlcbuf3[1536][2] IBSS_ATTR;    //small so lets try iram




#include "wmadata.h" // PJJ


/* butter fly op */
#define BF(pre, pim, qre, qim, pre1, pim1, qre1, qim1) \
{\
  fixed32 ax, ay, bx, by;\
  bx=pre1;\
  by=pim1;\
  ax=qre1;\
  ay=qim1;\
  pre = (bx + ax);\
  pim = (by + ay);\
  qre = (bx - ax);\
  qim = (by - ay);\
}


int fft_calc_unscaled(FFTContext *s, FFTComplex *z)
{
    int ln = s->nbits;
    int j, np, np2;
    int nblocks, nloops;
    register FFTComplex *p, *q;
   // FFTComplex *exptab = s->exptab;
    int l;
    fixed32 tmp_re, tmp_im;
    int tabshift = 10-ln;

    np = 1 << ln;


    /* pass 0 */

    p=&z[0];
    j=(np >> 1);
    do
    {
        BF(p[0].re, p[0].im, p[1].re, p[1].im,
           p[0].re, p[0].im, p[1].re, p[1].im);
        p+=2;
    }
    while (--j != 0);

    /* pass 1 */


    p=&z[0];
    j=np >> 2;
    if (s->inverse)
    {
        do
        {
            BF(p[0].re, p[0].im, p[2].re, p[2].im,
               p[0].re, p[0].im, p[2].re, p[2].im);
            BF(p[1].re, p[1].im, p[3].re, p[3].im,
               p[1].re, p[1].im, -p[3].im, p[3].re);
            p+=4;
        }
        while (--j != 0);
    }
    else
    {
        do
        {
            BF(p[0].re, p[0].im, p[2].re, p[2].im,
               p[0].re, p[0].im, p[2].re, p[2].im);
            BF(p[1].re, p[1].im, p[3].re, p[3].im,
               p[1].re, p[1].im, p[3].im, -p[3].re);
            p+=4;
        }
        while (--j != 0);
    }
    /* pass 2 .. ln-1 */

    nblocks = np >> 3;
    nloops = 1 << 2;
    np2 = np >> 1;
    do
    {
        p = z;
        q = z + nloops;
        for (j = 0; j < nblocks; ++j)
        {
            BF(p->re, p->im, q->re, q->im,
               p->re, p->im, q->re, q->im);

            p++;
            q++;
            for(l = nblocks; l < np2; l += nblocks)
            {
                CMUL(&tmp_re, &tmp_im, exptab0[(l<<tabshift)].re, exptab0[(l<<tabshift)].im, q->re, q->im);
                //CMUL(&tmp_re, &tmp_im, exptab[l].re, exptab[l].im, q->re, q->im);
                BF(p->re, p->im, q->re, q->im,
                   p->re, p->im, tmp_re, tmp_im);
                p++;
                q++;
            }

            p += nloops;
            q += nloops;
        }
        nblocks = nblocks >> 1;
        nloops = nloops << 1;
    }
    while (nblocks != 0);
    return 0;
}

/**
 * init MDCT or IMDCT computation.
 */
int ff_mdct_init(MDCTContext *s, int nbits, int inverse)
{
    int n, n4, i;
   // fixed32 alpha;


    memset(s, 0, sizeof(*s));
    n = 1 << nbits;            //nbits ranges from 12 to 8 inclusive
    s->nbits = nbits;
    s->n = n;
    n4 = n >> 2;
    s->tcos = tcosarray[12-nbits];
    s->tsin = tsinarray[12-nbits];
    for(i=0;i<n4;i++)
    {
        //fixed32 pi2 = fixmul32(0x20000, M_PI_F);
        fixed32 ip = itofix32(i) + 0x2000;
        ip = ip >> nbits;
        //ip = fixdiv32(ip,itofix32(n)); // PJJ optimize
        //alpha = fixmul32(TWO_M_PI_F, ip);
        //s->tcos[i] = -fixcos32(alpha);        //alpha between 0 and pi/2
        //s->tsin[i] = -fixsin32(alpha);

    s->tsin[i] = - fsincos(ip<<16, &(s->tcos[i]));            //I can't remember why this works, but it seems to agree for ~24 bits, maybe more!
    s->tcos[i] *=-1;
  }
      s->fft.nbits = s->nbits - 2;


    s->fft.inverse = inverse;

    return 0;

}

/**
 * Compute inverse MDCT of size N = 2^nbits
 * @param output N samples
 * @param input N/2 samples
 * @param tmp N/2 samples
 */
void ff_imdct_calc(MDCTContext *s,
                   fixed32 *output,
                   fixed32 *input)
{
    int k, n8, n4, n2, n, j,scale;
    const fixed32 *tcos = s->tcos;
    const fixed32 *tsin = s->tsin;
    const fixed32 *in1, *in2;
    FFTComplex *z1 = (FFTComplex *)output;
    FFTComplex *z2 = (FFTComplex *)input;
    int revtabshift = 12 - s->nbits;

    n = 1 << s->nbits;

    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;


    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;

    for(k = 0; k < n4; k++)
    {
        j=revtab0[k<<revtabshift];
        CMUL(&z1[j].re, &z1[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
    }

        scale = fft_calc_unscaled(&s->fft, z1);

    /* post rotation + reordering */

    for(k = 0; k < n4; k++)
    {
        CMUL(&z2[k].re, &z2[k].im, (z1[k].re), (z1[k].im), tcos[k], tsin[k]);
    }

    for(k = 0; k < n8; k++)
    {
        fixed32 r1,r2,r3,r4,r1n,r2n,r3n;

        r1 = z2[n8 + k].im;
        r1n = r1 * -1;
        r2 = z2[n8-1-k].re;
        r2n = r2 * -1;
        r3 = z2[k+n8].re;
        r3n = r3 * -1;
        r4 = z2[n8-k-1].im;

        output[2*k] = r1n;
        output[n2-1-2*k] = r1;

        output[2*k+1] = r2;
        output[n2-1-2*k-1] = r2n;

        output[n2 + 2*k]= r3n;
        output[n-1- 2*k]= r3n;

        output[n2 + 2*k+1]= r4;
        output[n-2 - 2 * k] = r4;
    }




}


/*
 * Helper functions for wma_window.
 *
 *
 */

static inline void vector_fmul_add_add(fixed32 *dst, const fixed32 *src0, const fixed32 *src1, int len){
    int i;
    for(i=0; i<len; i++)
        dst[i] = fixmul32b(src0[i], src1[i]) + dst[i];
}

static inline void vector_fmul_reverse(fixed32 *dst, const fixed32 *src0, const fixed32 *src1, int len){
    int i;
    src1 += len-1;
    for(i=0; i<len; i++)
        dst[i] = fixmul32b(src0[i], src1[-i]);
}

/**
  * Apply MDCT window and add into output.
  *
  * We ensure that when the windows overlap their squared sum
  * is always 1 (MDCT reconstruction rule).
  */
 static void wma_window(WMADecodeContext *s, fixed32 *in, fixed32 *out)
 {
     //float *in = s->output;
     int block_len, bsize, n;

     /* left part */
     if (s->block_len_bits <= s->prev_block_len_bits) {
         block_len = s->block_len;
         bsize = s->frame_len_bits - s->block_len_bits;

         vector_fmul_add_add(out, in, s->windows[bsize], block_len);

     } else {
         block_len = 1 << s->prev_block_len_bits;
         n = (s->block_len - block_len) / 2;
         bsize = s->frame_len_bits - s->prev_block_len_bits;

         vector_fmul_add_add(out+n, in+n, s->windows[bsize],  block_len);

         memcpy(out+n+block_len, in+n+block_len, n*sizeof(fixed32));
     }

     out += s->block_len;
     in += s->block_len;

     /* right part */
     if (s->block_len_bits <= s->next_block_len_bits) {
         block_len = s->block_len;
         bsize = s->frame_len_bits - s->block_len_bits;

         vector_fmul_reverse(out, in, s->windows[bsize], block_len);

     } else {
         block_len = 1 << s->next_block_len_bits;
         n = (s->block_len - block_len) / 2;
         bsize = s->frame_len_bits - s->next_block_len_bits;

         memcpy(out, in, n*sizeof(fixed32));

         vector_fmul_reverse(out+n, in+n, s->windows[bsize], block_len);

         memset(out+n+block_len, 0, n*sizeof(fixed32));
     }
 }




/* XXX: use same run/length optimization as mpeg decoders */
static void init_coef_vlc(VLC *vlc,
                          uint16_t **prun_table, uint16_t **plevel_table,
                          const CoefVLCTable *vlc_table, int tab)
{
    int n = vlc_table->n;
    const uint8_t *table_bits = vlc_table->huffbits;
    const uint32_t *table_codes = vlc_table->huffcodes;
    const uint16_t *levels_table = vlc_table->levels;
    uint16_t *run_table, *level_table;
    const uint16_t *p;
    int i, l, j, level;


    init_vlc(vlc, VLCBITS, n, table_bits, 1, 1, table_codes, 4, 4, 0);

    run_table = runtabarray[tab];
    level_table= levtabarray[tab];

    p = levels_table;
    i = 2;
    level = 1;
    while (i < n)
    {
        l = *p++;
        for(j=0;j<l;++j)
        {
            run_table[i] = j;
            level_table[i] = level;
            ++i;
        }
        ++level;
    }
    *prun_table = run_table;
    *plevel_table = level_table;
}

int wma_decode_init(WMADecodeContext* s, asf_waveformatex_t *wfx)
{
    //WMADecodeContext *s = avctx->priv_data;
    int i, m, j, flags1, flags2;
    fixed32 *window;
    uint8_t *extradata;
    fixed64 bps1;
    fixed32 high_freq;
    fixed64 bps;
    int sample_rate1;
    int coef_vlc_table;
    //    int filehandle;
    #ifdef CPU_COLDFIRE
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
    #endif

    s->sample_rate = wfx->rate;
    s->nb_channels = wfx->channels;
    s->bit_rate = wfx->bitrate;
    s->block_align = wfx->blockalign;

    s->coefs = &coefsarray;

    if (wfx->codec_id == ASF_CODEC_ID_WMAV1) {
        s->version = 1;
    } else if (wfx->codec_id == ASF_CODEC_ID_WMAV2 ) {
        s->version = 2;
    } else {
        /*one of those other wma flavors that don't have GPLed decoders */
        return -1;
    }

    /* extract flag infos */
    flags1 = 0;
    flags2 = 0;
    extradata = wfx->data;
    if (s->version == 1 && wfx->datalen >= 4) {
        flags1 = extradata[0] | (extradata[1] << 8);
        flags2 = extradata[2] | (extradata[3] << 8);
    }else if (s->version == 2 && wfx->datalen >= 6){
        flags1 = extradata[0] | (extradata[1] << 8) |
                 (extradata[2] << 16) | (extradata[3] << 24);
        flags2 = extradata[4] | (extradata[5] << 8);
    }
    s->use_exp_vlc = flags2 & 0x0001;
    s->use_bit_reservoir = flags2 & 0x0002;
    s->use_variable_block_len = flags2 & 0x0004;

    /* compute MDCT block size */
    if (s->sample_rate <= 16000){
        s->frame_len_bits = 9;
    }else if (s->sample_rate <= 22050 ||
             (s->sample_rate <= 32000 && s->version == 1)){
        s->frame_len_bits = 10;
    }else{
        s->frame_len_bits = 11;
    }
    s->frame_len = 1 << s->frame_len_bits;
    if (s-> use_variable_block_len)
    {
        int nb_max, nb;
        nb = ((flags2 >> 3) & 3) + 1;
        if ((s->bit_rate / s->nb_channels) >= 32000)
        {
            nb += 2;
        }
        nb_max = s->frame_len_bits - BLOCK_MIN_BITS;        //max is 11-7
        if (nb > nb_max)
            nb = nb_max;
        s->nb_block_sizes = nb + 1;
    }
    else
    {
        s->nb_block_sizes = 1;
    }

    /* init rate dependant parameters */
    s->use_noise_coding = 1;
    high_freq = fixmul64byfixed(itofix64(s->sample_rate), 0x8000);


    /* if version 2, then the rates are normalized */
    sample_rate1 = s->sample_rate;
    if (s->version == 2)
    {
        if (sample_rate1 >= 44100)
            sample_rate1 = 44100;
        else if (sample_rate1 >= 22050)
            sample_rate1 = 22050;
        else if (sample_rate1 >= 16000)
            sample_rate1 = 16000;
        else if (sample_rate1 >= 11025)
            sample_rate1 = 11025;
        else if (sample_rate1 >= 8000)
            sample_rate1 = 8000;
    }

    fixed64 tmp = itofix64(s->bit_rate);
    fixed64 tmp2 = itofix64(s->nb_channels * s->sample_rate);
    bps = fixdiv64(tmp, tmp2);
    fixed64 tim = fixmul64byfixed(bps, s->frame_len);
    fixed64 tmpi = fixdiv64(tim,itofix64(8));
    s->byte_offset_bits = av_log2(fixtoi64(tmpi+0x8000)) + 2;

    /* compute high frequency value and choose if noise coding should
       be activated */
    bps1 = bps;
    if (s->nb_channels == 2)
        bps1 = fixmul32(bps,0x1999a);
    if (sample_rate1 == 44100)
    {
        if (bps1 >= 0x9c29)
            s->use_noise_coding = 0;
        else
            high_freq = fixmul64byfixed(high_freq,0x6666);
    }
    else if (sample_rate1 == 22050)
    {
        if (bps1 >= 0x128f6)
            s->use_noise_coding = 0;
        else if (bps1 >= 0xb852)
            high_freq = fixmul64byfixed(high_freq,0xb333);
        else
            high_freq = fixmul64byfixed(high_freq,0x999a);
    }
    else if (sample_rate1 == 16000)
    {
        if (bps > 0x8000)
            high_freq = fixmul64byfixed(high_freq,0x8000);
        else
            high_freq = fixmul64byfixed(high_freq,0x4ccd);
    }
    else if (sample_rate1 == 11025)
    {
        high_freq = fixmul64byfixed(high_freq,0xb3333);
    }
    else if (sample_rate1 == 8000)
    {
        if (bps <= 0xa000)
        {
            high_freq = fixmul64byfixed(high_freq,0x8000);
        }
        else if (bps > 0xc000)
        {
            s->use_noise_coding = 0;
        }
        else
        {
            high_freq = fixmul64byfixed(high_freq,0xa666);
        }
    }
    else
    {
        if (bps >= 0xcccd)
        {
            high_freq = fixmul64byfixed(high_freq,0xc000);
        }
        else if (bps >= 0x999a)
        {
            high_freq = fixmul64byfixed(high_freq,0x999a);
        }
        else
        {
            high_freq = fixmul64byfixed(high_freq,0x8000);
        }
    }

    /* compute the scale factor band sizes for each MDCT block size */
    {
        int a, b, pos, lpos, k, block_len, i, j, n;
        const uint8_t *table;

        if (s->version == 1)
        {
            s->coefs_start = 3;
        }
        else
        {
            s->coefs_start = 0;
        }
        for(k = 0; k < s->nb_block_sizes; ++k)
        {
            block_len = s->frame_len >> k;

            if (s->version == 1)
            {
                lpos = 0;
                for(i=0;i<25;++i)
                {
                    a = wma_critical_freqs[i];
                    b = s->sample_rate;
                    pos = ((block_len * 2 * a)  + (b >> 1)) / b;
                    if (pos > block_len)
                        pos = block_len;
                    s->exponent_bands[0][i] = pos - lpos;
                    if (pos >= block_len)
                    {
                        ++i;
                        break;
                    }
                    lpos = pos;
                }
                s->exponent_sizes[0] = i;
            }
            else
            {
                /* hardcoded tables */
                table = NULL;
                a = s->frame_len_bits - BLOCK_MIN_BITS - k;
                if (a < 3)
                {
                    if (s->sample_rate >= 44100)
                        table = exponent_band_44100[a];
                    else if (s->sample_rate >= 32000)
                        table = exponent_band_32000[a];
                    else if (s->sample_rate >= 22050)
                        table = exponent_band_22050[a];
                }
                if (table)
                {
                    n = *table++;
                    for(i=0;i<n;++i)
                        s->exponent_bands[k][i] = table[i];
                    s->exponent_sizes[k] = n;
                }
                else
                {
                    j = 0;
                    lpos = 0;
                    for(i=0;i<25;++i)
                    {
                        a = wma_critical_freqs[i];
                        b = s->sample_rate;
                        pos = ((block_len * 2 * a)  + (b << 1)) / (4 * b);
                        pos <<= 2;
                        if (pos > block_len)
                            pos = block_len;
                        if (pos > lpos)
                            s->exponent_bands[k][j++] = pos - lpos;
                        if (pos >= block_len)
                            break;
                        lpos = pos;
                    }
                    s->exponent_sizes[k] = j;
                }
            }

            /* max number of coefs */
            s->coefs_end[k] = (s->frame_len - ((s->frame_len * 9) / 100)) >> k;
            /* high freq computation */
            fixed64 tmp = itofix64(block_len<<2);
            tmp = fixmul64byfixed(tmp,high_freq);
            fixed64 tmp2 = itofix64(s->sample_rate);
            tmp2 += 0x8000;
            s->high_band_start[k] = fixtoi64(fixdiv64(tmp,tmp2));

            /*
            s->high_band_start[k] = (int)((block_len * 2 * high_freq) /
                                          s->sample_rate + 0.5);*/

            n = s->exponent_sizes[k];
            j = 0;
            pos = 0;
            for(i=0;i<n;++i)
            {
                int start, end;
                start = pos;
                pos += s->exponent_bands[k][i];
                end = pos;
                if (start < s->high_band_start[k])
                    start = s->high_band_start[k];
                if (end > s->coefs_end[k])
                    end = s->coefs_end[k];
                if (end > start)
                    s->exponent_high_bands[k][j++] = end - start;
            }
            s->exponent_high_sizes[k] = j;
        }
    }

    /* init MDCT */
    /*TODO:  figure out how to fold this up into one array*/
    tcosarray[0] = tcos0; tcosarray[1] = tcos1; tcosarray[2] = tcos2; tcosarray[3] = tcos3;tcosarray[4] = tcos4;
    tsinarray[0] = tsin0; tsinarray[1] = tsin1; tsinarray[2] = tsin2; tsinarray[3] = tsin3;tsinarray[4] = tsin4;

      /*these are folded up now*/
    exparray[0] = exptab0; //exparray[1] = exptab1; exparray[2] = exptab2; exparray[3] = exptab3; exparray[4] = exptab4;
    revarray[0]=revtab0; //revarray[1]=revtab1; revarray[2]=revtab2; revarray[3]=revtab3; revarray[4]=revtab4;

    s->mdct_tmp = mdct_tmp; /* temporary storage for imdct */
    for(i = 0; i < s->nb_block_sizes; ++i)
    {
        ff_mdct_init(&s->mdct_ctx[i], s->frame_len_bits - i + 1, 1);
    }

	{
		int i, n;
		fixed32 c1, s1, s2;

		n=1<<10;
		s2 = 1 ? 1 : -1;
		  for(i=0;i<(n/2);++i)
		  {
		    fixed32 ifix = itofix32(i);
		    fixed32 nfix = itofix32(n);
		    fixed32 res = fixdiv32(ifix,nfix);

		    s1 = fsincos(res<<16, &c1);

		    exptab0[i].re = c1;
		    exptab0[i].im = s1*s2;
		}
	}

    /* init the MDCT bit reverse table here rather then in fft_init */

      for(i=0;i<1024;i++)           /*hard coded to a 2048 bit rotation*/
      {                             /*smaller sizes can reuse the largest*/
             m=0;
             for(j=0;j<10;j++)
             {
                 m |= ((i >> j) & 1) << (10-j-1);
             }

       revtab0[i]=m;
       }

    /*ffmpeg uses malloc to only allocate as many window sizes as needed.  However, we're really only interested in the worst case memory usage.
    * In the worst case you can have 5 window sizes, 128 doubling up 2048
    * Smaller windows are handled differently.
    * Since we don't have malloc, just statically allocate this
    */
        fixed32 *temp[5];
    temp[0] = stat0;
    temp[1] = stat1;
    temp[2] = stat2;
    temp[3] = stat3;
    temp[4] = stat4;

    /* init MDCT windows : simple sinus window */
    for(i = 0; i < s->nb_block_sizes; i++)
    {
        int n, j;
        fixed32 alpha;
        n = 1 << (s->frame_len_bits - i);
        //window = av_malloc(sizeof(fixed32) * n);
        window = temp[i];

        //fixed32 n2 = itofix32(n<<1);        //2x the window length
        //alpha = fixdiv32(M_PI_F, n2);        //PI / (2x Window length) == PI<<(s->frame_len_bits - i+1)

        //alpha = M_PI_F>>(s->frame_len_bits - i+1);
        alpha = (1<<15)>>(s->frame_len_bits - i+1);   /* this calculates 0.5/(2*n) */
        for(j=0;j<n;++j)
        {
            fixed32 j2 = itofix32(j) + 0x8000;
             window[j] = fsincos(fixmul32(j2,alpha)<<16, 0);        //alpha between 0 and pi/2

        }
        //printf("created window\n");
        s->windows[i] = window;
        //printf("assigned window\n");
    }

    s->reset_block_lengths = 1;

    if (s->use_noise_coding)
    {
        /* init the noise generator */
        if (s->use_exp_vlc)
        {
            s->noise_mult = 0x51f;
        }
        else
        {
            s->noise_mult = 0xa3d;
        }


        {
            unsigned int seed;
            fixed32 norm;
            seed = 1;
            norm = 0;   // PJJ: near as makes any diff to 0!
            for (i=0;i<NOISE_TAB_SIZE;++i)
            {
                seed = seed * 314159 + 1;
                s->noise_table[i] = itofix32((int)seed) * norm;
            }
        }


		 init_vlc(&s->hgain_vlc, 9, sizeof(hgain_huffbits),
                 hgain_huffbits, 1, 1,
                 hgain_huffcodes, 2, 2, 0);
    }

    if (s->use_exp_vlc)
    {

        s->exp_vlc.table = vlcbuf3;
        s->exp_vlc.table_allocated = 1536;

         init_vlc(&s->exp_vlc, 9, sizeof(scale_huffbits),
        		scale_huffbits, 1, 1,
                 scale_huffcodes, 4, 4, 0);
    }
    else
    {
        wma_lsp_to_curve_init(s, s->frame_len);
    }

    /* choose the VLC tables for the coefficients */
    coef_vlc_table = 2;
    if (s->sample_rate >= 32000)
    {
        if (bps1 < 0xb852)
            coef_vlc_table = 0;
        else if (bps1 < 0x128f6)
            coef_vlc_table = 1;
    }

    runtabarray[0] = runtab0; runtabarray[1] = runtab1;
    levtabarray[0] = levtab0; levtabarray[1] = levtab1;

    s->coef_vlc[0].table = vlcbuf1;
    s->coef_vlc[0].table_allocated = 24576/4;
    s->coef_vlc[1].table = vlcbuf2;
    s->coef_vlc[1].table_allocated = 14336/4;


    init_coef_vlc(&s->coef_vlc[0], &s->run_table[0], &s->level_table[0],
                  &coef_vlcs[coef_vlc_table * 2], 0);
    init_coef_vlc(&s->coef_vlc[1], &s->run_table[1], &s->level_table[1],
                  &coef_vlcs[coef_vlc_table * 2 + 1], 1);

    s->last_superframe_len = 0;
    s->last_bitoffset = 0;

    return 0;
}


/* compute x^-0.25 with an exponent and mantissa table. We use linear
   interpolation to reduce the mantissa table size at a small speed
   expense (linear interpolation approximately doubles the number of
   bits of precision). */
static inline fixed32 pow_m1_4(WMADecodeContext *s, fixed32 x)
{
    union {
        fixed64 f;
        unsigned int v;
    } u, t;
    unsigned int e, m;
    fixed64 a, b;

    u.f = x;
    e = u.v >> 23;
    m = (u.v >> (23 - LSP_POW_BITS)) & ((1 << LSP_POW_BITS) - 1);
    /* build interpolation scale: 1 <= t < 2. */
    t.v = ((u.v << LSP_POW_BITS) & ((1 << 23) - 1)) | (127 << 23);
    a = s->lsp_pow_m_table1[m];
    b = s->lsp_pow_m_table2[m];
    return lsp_pow_e_table[e] * (a + b * t.f);
}

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len)
{
    fixed32 wdel, a, b;
    int i, m;

    wdel = fixdiv32(M_PI_F, itofix32(frame_len));
    for (i=0; i<frame_len; ++i)
    {
        s->lsp_cos_table[i] = 0x20000 * fixcos32(wdel * i);    //wdel*i between 0 and pi

    }


    /* NOTE: these two tables are needed to avoid two operations in
       pow_m1_4 */
    b = itofix32(1);
    int ix = 0;
    for(i=(1 << LSP_POW_BITS) - 1;i>=0;i--)
    {
        m = (1 << LSP_POW_BITS) + i;
        a = m * (0x8000 / (1 << LSP_POW_BITS)); //PJJ
        a = pow_a_table[ix++];  // PJJ : further refinement
        s->lsp_pow_m_table1[i] = 2 * a - b;
        s->lsp_pow_m_table2[i] = b - a;
        b = a;
    }
}

/* NOTE: We use the same code as Vorbis here */
/* XXX: optimize it further with SSE/3Dnow */
static void wma_lsp_to_curve(WMADecodeContext *s,
                             fixed32 *out,
                             fixed32 *val_max_ptr,
                             int n,
                             fixed32 *lsp)
{
    int i, j;
    fixed32 p, q, w, v, val_max;

    val_max = 0;
    for(i=0;i<n;++i)
    {
        p = 0x8000;
        q = 0x8000;
        w = s->lsp_cos_table[i];
        for (j=1;j<NB_LSP_COEFS;j+=2)
        {
            q *= w - lsp[j - 1];
            p *= w - lsp[j];
        }
        p *= p * (0x20000 - w);
        q *= q * (0x20000 + w);
        v = p + q;
        v = pow_m1_4(s, v); // PJJ
        if (v > val_max)
            val_max = v;
        out[i] = v;
    }
    *val_max_ptr = val_max;
}

/* decode exponents coded with LSP coefficients (same idea as Vorbis) */
static void decode_exp_lsp(WMADecodeContext *s, int ch)
{
    fixed32 lsp_coefs[NB_LSP_COEFS];
    int val, i;

    for (i = 0; i < NB_LSP_COEFS; ++i)
    {
        if (i == 0 || i >= 8)
            val = get_bits(&s->gb, 3);
        else
            val = get_bits(&s->gb, 4);
        lsp_coefs[i] = lsp_codebook[i][val];
    }

    wma_lsp_to_curve(s,
                     s->exponents[ch],
                     &s->max_exponent[ch],
                     s->block_len,
                     lsp_coefs);
}

/* decode exponents coded with VLC codes */
static int decode_exp_vlc(WMADecodeContext *s, int ch)
{
    int last_exp, n, code;
    const uint16_t *ptr, *band_ptr;
    fixed32 v, max_scale;
    fixed32 *q,*q_end;

    band_ptr = s->exponent_bands[s->frame_len_bits - s->block_len_bits];
    ptr = band_ptr;
    q = s->exponents[ch];
    q_end = q + s->block_len;
    max_scale = 0;


    if (s->version == 1)        //wmav1 only
    {
        last_exp = get_bits(&s->gb, 5) + 10;
        /* XXX: use a table */
        v = pow_10_to_yover16[last_exp];
        max_scale = v;
        n = *ptr++;
        do
        {
            *q++ = v;
        }
        while (--n);
    }
    last_exp = 36;

    while (q < q_end)
    {
        code = get_vlc2(&s->gb, s->exp_vlc.table, EXPVLCBITS, EXPMAX);
        if (code < 0)
        {
            return -1;
        }
        /* NOTE: this offset is the same as MPEG4 AAC ! */
        last_exp += code - 60;
        /* XXX: use a table */
        v = pow_10_to_yover16[last_exp];
        if (v > max_scale)
        {
            max_scale = v;
        }
        n = *ptr++;
        do
        {
            *q++ = v;

        }
        while (--n);
    }

    s->max_exponent[ch] = max_scale;
    return 0;
}

/* return 0 if OK. return 1 if last block of frame. return -1 if
   unrecorrable error. */
static int wma_decode_block(WMADecodeContext *s)
{
    int n, v, a, ch, code, bsize;
    int coef_nb_bits, total_gain;
    int nb_coefs[MAX_CHANNELS];
    fixed32 mdct_norm;

//    printf("***decode_block: %d:%d (%d)\n", s->frame_count - 1, s->block_num, s->block_len);

   /* compute current block length */
    if (s->use_variable_block_len)
    {
        n = av_log2(s->nb_block_sizes - 1) + 1;

        if (s->reset_block_lengths)
        {
            s->reset_block_lengths = 0;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
            {
                return -2;
            }
            s->prev_block_len_bits = s->frame_len_bits - v;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
            {
                return -3;
            }
            s->block_len_bits = s->frame_len_bits - v;
        }
        else
        {
            /* update block lengths */
            s->prev_block_len_bits = s->block_len_bits;
            s->block_len_bits = s->next_block_len_bits;
        }
        v = get_bits(&s->gb, n);




        if (v >= s->nb_block_sizes)
        {
         // rb->splash(HZ*4, "v was %d", v);        //5, 7
            return -4;        //this is it
        }
        else{
              //rb->splash(HZ, "passed v block (%d)!", v);
      }
        s->next_block_len_bits = s->frame_len_bits - v;
    }
    else
    {
        /* fixed block len */
        s->next_block_len_bits = s->frame_len_bits;
        s->prev_block_len_bits = s->frame_len_bits;
        s->block_len_bits = s->frame_len_bits;
    }
    /* now check if the block length is coherent with the frame length */
    s->block_len = 1 << s->block_len_bits;

    if ((s->block_pos + s->block_len) > s->frame_len)
    {
        return -5;
    }

    if (s->nb_channels == 2)
    {
        s->ms_stereo = get_bits(&s->gb, 1);
    }
    v = 0;
    for (ch = 0; ch < s->nb_channels; ++ch)
    {
        a = get_bits(&s->gb, 1);
        s->channel_coded[ch] = a;
        v |= a;
    }
    /* if no channel coded, no need to go further */
    /* XXX: fix potential framing problems */
    if (!v)
    {
        goto next;
    }

    bsize = s->frame_len_bits - s->block_len_bits;

    /* read total gain and extract corresponding number of bits for
       coef escape coding */
    total_gain = 1;
    for(;;)
    {
        a = get_bits(&s->gb, 7);
        total_gain += a;
        if (a != 127)
        {
            break;
        }
    }

    if (total_gain < 15)
        coef_nb_bits = 13;
    else if (total_gain < 32)
        coef_nb_bits = 12;
    else if (total_gain < 40)
        coef_nb_bits = 11;
    else if (total_gain < 45)
        coef_nb_bits = 10;
    else
        coef_nb_bits = 9;
    /* compute number of coefficients */
    n = s->coefs_end[bsize] - s->coefs_start;

    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        nb_coefs[ch] = n;
    }
    /* complex coding */
    if (s->use_noise_coding)
    {

        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                int i, n, a;
                n = s->exponent_high_sizes[bsize];
                for(i=0;i<n;++i)
                {
                    a = get_bits(&s->gb, 1);
                    s->high_band_coded[ch][i] = a;
                    /* if noise coding, the coefficients are not transmitted */
                    if (a)
                        nb_coefs[ch] -= s->exponent_high_bands[bsize][i];
                }
            }
        }
        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                int i, n, val, code;

                n = s->exponent_high_sizes[bsize];
                val = (int)0x80000000;
                for(i=0;i<n;++i)
                {
                    if (s->high_band_coded[ch][i])
                    {
                        if (val == (int)0x80000000)
                        {
                            val = get_bits(&s->gb, 7) - 19;
                        }
                        else
                        {
                            //code = get_vlc(&s->gb, &s->hgain_vlc);
                            code = get_vlc2(&s->gb, s->hgain_vlc.table, HGAINVLCBITS, HGAINMAX);
                            if (code < 0)
                            {
                                return -6;
                            }
                            val += code - 18;
                        }
                        s->high_band_values[ch][i] = val;
                    }
                }
            }
        }
    }

 /* exponents can be reused in short blocks. */
    if ((s->block_len_bits == s->frame_len_bits) || get_bits(&s->gb, 1)) {

        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                if (s->use_exp_vlc)
                {
                    if (decode_exp_vlc(s, ch) < 0)
                    {
                        return -7;
                    }
                }
                else
                {
                    decode_exp_lsp(s, ch);
                }
                s->exponents_bsize[ch] = bsize;
            }
        }
    }

    /* parse spectral coefficients : just RLE encoding */
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            VLC *coef_vlc;
            int level, run, sign, tindex;
            int16_t *ptr, *eptr;
            const int16_t *level_table, *run_table;

            /* special VLC tables are used for ms stereo because
               there is potentially less energy there */
            tindex = (ch == 1 && s->ms_stereo);
            coef_vlc = &s->coef_vlc[tindex];
            run_table = s->run_table[tindex];
            level_table = s->level_table[tindex];
            /* XXX: optimize */
            ptr = &s->coefs1[ch][0];
            eptr = ptr + nb_coefs[ch];
            memset(ptr, 0, s->block_len * sizeof(int16_t));



            for(;;)
            {
                code = get_vlc2(&s->gb, coef_vlc->table, VLCBITS, VLCMAX);
                //code = get_vlc(&s->gb, coef_vlc);
                if (code < 0)
                {
                    return -8;
                }
                if (code == 1)
                {
                    /* EOB */
                    break;
                }
                else if (code == 0)
                {
                    /* escape */
                    level = get_bits(&s->gb, coef_nb_bits);
                    /* NOTE: this is rather suboptimal. reading
                       block_len_bits would be better */
                    run = get_bits(&s->gb, s->frame_len_bits);
                }
                else
                {
                    /* normal code */
                    run = run_table[code];
                    level = level_table[code];
                }
                sign = get_bits(&s->gb, 1);
                if (!sign)
                    level = -level;
                ptr += run;
                if (ptr >= eptr)
                {
                    return -9;
                }
                *ptr++ = level;


                /* NOTE: EOB can be omitted */
                if (ptr >= eptr)
                    break;
            }
        }
        if (s->version == 1 && s->nb_channels >= 2)
        {
            align_get_bits(&s->gb);
        }
    }

    {
        int n4 = s->block_len >> 1;
        //mdct_norm = 0x10000;
        //mdct_norm = fixdiv32(mdct_norm,itofix32(n4));

        mdct_norm = 0x10000>>(s->block_len_bits-1);        //theres no reason to do a divide by two in fixed precision ...

        if (s->version == 1)
        {
            fixed32 tmp = fixsqrt32(itofix32(n4));
            mdct_norm *= tmp; // PJJ : exercise this path
        }
    }


    /* finally compute the MDCT coefficients */
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            int16_t *coefs1;
            fixed32 *exponents, *exp_ptr;
            fixed32 *coefs, atemp;
            fixed64 mult;
            fixed64 mult1;
            fixed32 noise;
            int i, j, n, n1, last_high_band, esize;
            fixed32 exp_power[HIGH_BAND_MAX_SIZE];

            //total_gain, coefs1, mdctnorm are lossless

            coefs1 = s->coefs1[ch];
            exponents = s->exponents[ch];
            esize = s->exponents_bsize[ch];
            mult = fixdiv64(pow_table[total_gain],Fixed32To64(s->max_exponent[ch]));
         //   mul = fixtof64(pow_table[total_gain])/(s->block_len/2)/fixtof64(s->max_exponent[ch]);

            mult = fixmul64byfixed(mult, mdct_norm);        //what the hell?  This is actually fixed64*2^16!
            coefs = (*(s->coefs))[ch];                                            //VLC exponenents are used to get MDCT coef here!

        n=0;

            if (s->use_noise_coding)
            {
                mult1 = mult;

                /* very low freqs : noise */
                for(i = 0;i < s->coefs_start; ++i)
                {
                    *coefs++ = fixmul32(fixmul32(s->noise_table[s->noise_index],(*exponents++)),Fixed32From64(mult1));
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }

                n1 = s->exponent_high_sizes[bsize];

                /* compute power of high bands */
                exp_ptr = exponents +
                          s->high_band_start[bsize] -
                          s->coefs_start;
                last_high_band = 0; /* avoid warning */
                for (j=0;j<n1;++j)
                {
                    n = s->exponent_high_bands[s->frame_len_bits -
                                               s->block_len_bits][j];
                    if (s->high_band_coded[ch][j])
                    {
                        fixed32 e2, v;
                        e2 = 0;
                        for(i = 0;i < n; ++i)
                        {
                            v = exp_ptr[i];
                            e2 += v * v;
                        }
                        exp_power[j] = fixdiv32(e2,n);
                        last_high_band = j;
                    }
                    exp_ptr += n;
                }

                /* main freqs and high freqs */
                for(j=-1;j<n1;++j)
                {
                    if (j < 0)
                    {
                        n = s->high_band_start[bsize] -
                            s->coefs_start;
                    }
                    else
                    {
                        n = s->exponent_high_bands[s->frame_len_bits -
                                                   s->block_len_bits][j];
                    }
                    if (j >= 0 && s->high_band_coded[ch][j])
                    {
                        /* use noise with specified power */
                        fixed32 tmp = fixdiv32(exp_power[j],exp_power[last_high_band]);
                        mult1 = (fixed64)fixsqrt32(tmp);
                        /* XXX: use a table */
                        mult1 = mult1 * pow_table[s->high_band_values[ch][j]];
                        mult1 = fixdiv64(mult1,fixmul32(s->max_exponent[ch],s->noise_mult));
                        mult1 = fixmul64byfixed(mult1,mdct_norm);
                        for(i = 0;i < n; ++i)
                        {
                            noise = s->noise_table[s->noise_index];
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                            *coefs++ = fixmul32(fixmul32(*exponents,noise),Fixed32From64(mult1));
                            ++exponents;
                        }
                    }
                    else
                    {
                        /* coded values + small noise */
                        for(i = 0;i < n; ++i)
                        {
                            // PJJ: check code path
                            noise = s->noise_table[s->noise_index];
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                            *coefs++ = fixmul32(fixmul32(((*coefs1++) + noise),*exponents),mult);
                            ++exponents;
                        }
                    }
                }

                /* very high freqs : noise */
                n = s->block_len - s->coefs_end[bsize];
                mult1 = fixmul32(mult,exponents[-1]);
                for (i = 0; i < n; ++i)
                {
                    *coefs++ = fixmul32(s->noise_table[s->noise_index],Fixed32From64(mult1));
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }
            }
            else
            {

                /* XXX: optimize more */

                n = nb_coefs[ch];




                for(i = 0;i < n; ++i)
                {
      /*
      *  Previously the IMDCT was run in 17.15 precision to avoid overflow. However rare files could
      *  overflow here as well, so switch to 17.15 now.  As a bonus, this saves us a shift later on.
      */


                  atemp = (fixed32)(coefs1[i]*mult>>17);
                //this "works" in the sense that the mdcts converge
                 //atemp= ftofix32(coefs1[i] * fixtof64(exponents[i]) * fixtof64(mult>>16));

                  *coefs++=fixmul32(atemp,exponents[i<<bsize>>esize]);

               }
                n = s->block_len - s->coefs_end[bsize];
                for(i = 0;i < n; ++i)
                    *coefs++ = 0;
            }
        }
    }



    if (s->ms_stereo && s->channel_coded[1])
    {
        fixed32 a, b;
        int i;
        fixed32 (*coefs)[MAX_CHANNELS][BLOCK_MAX_SIZE]  = (s->coefs);

        /* nominal case for ms stereo: we do it before mdct */
        /* no need to optimize this case because it should almost
           never happen */
        if (!s->channel_coded[0])
        {
            memset((*(s->coefs))[0], 0, sizeof(fixed32) * s->block_len);
            s->channel_coded[0] = 1;
        }

        for(i = 0; i < s->block_len; ++i)
        {
            a = (*coefs)[0][i];
            b = (*coefs)[1][i];
            (*coefs)[0][i] = a + b;
            (*coefs)[1][i] = a - b;
        }
    }

    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            static fixed32  output[BLOCK_MAX_SIZE * 2] IBSS_ATTR;

            int n4, index, n;

            n = s->block_len;
            n4 = s->block_len >>1;

            ff_imdct_calc(&s->mdct_ctx[bsize],
                          output,
                          (*(s->coefs))[ch]);


            /* add in the frame */
            index = (s->frame_len / 2) + s->block_pos - n4;

            wma_window(s, output, &s->frame_out[ch][index]);



            /* specific fast case for ms-stereo : add to second
               channel if it is not coded */
            if (s->ms_stereo && !s->channel_coded[1])
            {
                wma_window(s, output, &s->frame_out[1][index]);
            }
        }
    }
next:
    /* update block number */
    ++s->block_num;
    s->block_pos += s->block_len;
    if (s->block_pos >= s->frame_len)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* decode a frame of frame_len samples */
static int wma_decode_frame(WMADecodeContext *s, int16_t *samples)
{
    int ret, i, n, a, ch, incr;
    int16_t *ptr;
    fixed32 *iptr;
   // rb->splash(HZ, "in wma_decode_frame");

    /* read each block */
    s->block_num = 0;
    s->block_pos = 0;


    for(;;)
    {
        ret = wma_decode_block(s);
        if (ret < 0)
        {

            //rb->splash(HZ*4, "wma_decode_block failed with ret %d", ret);
            return -1;
        }
        if (ret)
        {
            break;
        }
    }

    /* convert frame to integer */
    n = s->frame_len;
    incr = s->nb_channels;
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        ptr = samples + ch;
        iptr = s->frame_out[ch];

        for (i=0;i<n;++i)
        {
            a = fixtoi32(*iptr++)<<1;        //ugly but good enough for now





            if (a > 32767)
            {
                a = 32767;
            }
            else if (a < -32768)
            {
                a = -32768;
            }
            *ptr = a;
            ptr += incr;
        }
        /* prepare for next block */
        memmove(&s->frame_out[ch][0], &s->frame_out[ch][s->frame_len],
                s->frame_len * sizeof(fixed32));

    }

    return 0;
}

/* Initialise the superframe decoding */

int wma_decode_superframe_init(WMADecodeContext* s,
                                 uint8_t *buf,  /*input*/
                                 int buf_size)
{
    if (buf_size==0)
    {
        s->last_superframe_len = 0;
        return 0;
    }

    s->current_frame = 0;

    init_get_bits(&s->gb, buf, buf_size*8);

    if (s->use_bit_reservoir)
    {
        /* read super frame header */
        get_bits(&s->gb, 4); /* super frame index */
        s->nb_frames = get_bits(&s->gb, 4);

        if (s->last_superframe_len == 0)
            s->nb_frames --;
        else if (s->nb_frames == 0)
            s->nb_frames++;

        s->bit_offset = get_bits(&s->gb, s->byte_offset_bits + 3);
    } else {
        s->nb_frames = 1;
    }

    return 1;
}


/* Decode a single frame in the current superframe - return -1 if
   there was a decoding error, or the number of samples decoded.
*/

int wma_decode_superframe_frame(WMADecodeContext* s,
                                int16_t* samples, /*output*/
                                uint8_t *buf,  /*input*/
                                int buf_size)
{
    int pos, len;
    uint8_t *q;
    int done = 0;

    if ((s->use_bit_reservoir) && (s->current_frame == 0))
    {
        if (s->last_superframe_len > 0)
        {
            /* add s->bit_offset bits to last frame */
            if ((s->last_superframe_len + ((s->bit_offset + 7) >> 3)) >
                    MAX_CODED_SUPERFRAME_SIZE)
            {
                goto fail;
            }
            q = s->last_superframe + s->last_superframe_len;
            len = s->bit_offset;
            while (len > 0)
            {
                *q++ = (get_bits)(&s->gb, 8);
                len -= 8;
            }
            if (len > 0)
            {
                *q++ = (get_bits)(&s->gb, len) << (8 - len);
            }

            /* XXX: s->bit_offset bits into last frame */
            init_get_bits(&s->gb, s->last_superframe, MAX_CODED_SUPERFRAME_SIZE*8);
            /* skip unused bits */
            if (s->last_bitoffset > 0)
                skip_bits(&s->gb, s->last_bitoffset);

            /* this frame is stored in the last superframe and in the
               current one */
            if (wma_decode_frame(s, samples) < 0)
            {
                goto fail;
            }
            done = 1;
        }

        /* read each frame starting from s->bit_offset */
        pos = s->bit_offset + 4 + 4 + s->byte_offset_bits + 3;
        init_get_bits(&s->gb, buf + (pos >> 3), (MAX_CODED_SUPERFRAME_SIZE - (pos >> 3))*8);
        len = pos & 7;
        if (len > 0)
            skip_bits(&s->gb, len);

        s->reset_block_lengths = 1;
    }

    /* If we haven't decoded a frame yet, do it now */
    if (!done)
        {
            if (wma_decode_frame(s, samples) < 0)
            {
                goto fail;
            }
        }

    s->current_frame++;

    if ((s->use_bit_reservoir) && (s->current_frame == s->nb_frames))
    {
        /* we copy the end of the frame in the last frame buffer */
        pos = get_bits_count(&s->gb) + ((s->bit_offset + 4 + 4 + s->byte_offset_bits + 3) & ~7);
        s->last_bitoffset = pos & 7;
        pos >>= 3;
        len = buf_size - pos;
        if (len > MAX_CODED_SUPERFRAME_SIZE || len < 0)
        {
            goto fail;
        }
        s->last_superframe_len = len;
        memcpy(s->last_superframe, buf + pos, len);
    }

    return s->frame_len;

fail:
    /* when error, we reset the bit reservoir */
    s->last_superframe_len = 0;
    return -1;
}

