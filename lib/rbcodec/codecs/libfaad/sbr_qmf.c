/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id$
**/

#include "common.h"
#include "structs.h"

#ifdef SBR_DEC


#include <stdlib.h>
#include <string.h>
#include "sbr_dct.h"
#include "sbr_qmf.h"
#include "sbr_qmf_c.h"
#include "sbr_syntax.h"

#ifdef FIXED_POINT
    #define FAAD_SYNTHESIS_SCALE(X) ((X)>>1)
    #define FAAD_ANALYSIS_SCALE1(X) ((X)>>4)
    #define FAAD_ANALYSIS_SCALE2(X) ((X))
    #define FAAD_ANALYSIS_SCALE3(X) ((X))
#else
    #define FAAD_SYNTHESIS_SCALE(X) ((X)/64.0f)
    #define FAAD_ANALYSIS_SCALE1(X) ((X))
    #define FAAD_ANALYSIS_SCALE2(X) (2.0f*(X))
    #define FAAD_ANALYSIS_SCALE3(X) ((X)/32.0f)
#endif


void sbr_qmf_analysis_32(sbr_info *sbr, qmfa_info *qmfa, const real_t *input,
                         qmf_t X[MAX_NTSR][64], uint8_t offset, uint8_t kx)
{
    real_t u[64] MEM_ALIGN_ATTR;
#ifndef SBR_LOW_POWER
    real_t real[32] MEM_ALIGN_ATTR;
    real_t imag[32] MEM_ALIGN_ATTR;
#else
    real_t y[32] MEM_ALIGN_ATTR;
#endif
    qmf_t *pX;
    uint32_t in = 0;
    uint32_t l, idx0, idx1;

    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        int32_t n;

        /* shift input buffer x */
        /* input buffer is not shifted anymore, x is implemented as double ringbuffer */
        //memmove(qmfa->x + 32, qmfa->x, (320-32)*sizeof(real_t));

        /* add new samples to input buffer x */
        idx0 = qmfa->x_index + 31; idx1 = idx0 + 320;
        for (n = 0; n < 32; n+=4)
        {
            qmfa->x[idx0--] = qmfa->x[idx1--] = (input[in++]);
            qmfa->x[idx0--] = qmfa->x[idx1--] = (input[in++]);
            qmfa->x[idx0--] = qmfa->x[idx1--] = (input[in++]);
            qmfa->x[idx0--] = qmfa->x[idx1--] = (input[in++]);
        }

        /* window and summation to create array u */
        for (n = 0; n < 32; n++)
        {
            idx0 = qmfa->x_index + n; idx1 = n * 20;
            u[n] = FAAD_ANALYSIS_SCALE1(
                   MUL_F(qmfa->x[idx0      ], qmf_c[idx1    ]) +
                   MUL_F(qmfa->x[idx0 +  64], qmf_c[idx1 + 2]) +
                   MUL_F(qmfa->x[idx0 + 128], qmf_c[idx1 + 4]) +
                   MUL_F(qmfa->x[idx0 + 192], qmf_c[idx1 + 6]) +
                   MUL_F(qmfa->x[idx0 + 256], qmf_c[idx1 + 8]));
        }
        for (n = 32; n < 64; n++)
        {
            idx0 = qmfa->x_index + n; idx1 = n * 20 - 639;
            u[n] = FAAD_ANALYSIS_SCALE1(
                   MUL_F(qmfa->x[idx0      ], qmf_c[idx1    ]) +
                   MUL_F(qmfa->x[idx0 +  64], qmf_c[idx1 + 2]) +
                   MUL_F(qmfa->x[idx0 + 128], qmf_c[idx1 + 4]) +
                   MUL_F(qmfa->x[idx0 + 192], qmf_c[idx1 + 6]) +
                   MUL_F(qmfa->x[idx0 + 256], qmf_c[idx1 + 8]));
        }

        /* update ringbuffer index */
        qmfa->x_index -= 32;
        if (qmfa->x_index < 0)
            qmfa->x_index = (320-32);

        /* calculate 32 subband samples by introducing X */
#ifdef SBR_LOW_POWER
        y[0] = u[48];
        for (n = 1; n < 16; n++)
            y[n] = u[n+48] + u[48-n];
        for (n = 16; n < 32; n++)
            y[n] = -u[n-16] + u[48-n];

        DCT3_32_unscaled(u, y);

        for (n = 0; n < 32; n++)
        {
            if (n < kx)
            {
                QMF_RE(X[l + offset][n]) = FAAD_ANALYSIS_SCALE2(u[n]);
            } else {
                QMF_RE(X[l + offset][n]) = 0;
            }
        }
#else /* #ifdef SBR_LOW_POWER */

        // Reordering of data moved from DCT_IV to here
        idx0 = 30; idx1 = 63;
        imag[31] = u[ 1]; real[ 0] = u[ 0];
        for (n = 1; n < 31; n+=3)
        {
            imag[idx0--] = u[n+1]; real[n  ] = -u[idx1--];
            imag[idx0--] = u[n+2]; real[n+1] = -u[idx1--];
            imag[idx0--] = u[n+3]; real[n+2] = -u[idx1--];
        }
        imag[ 0] = u[32]; real[31] = -u[33];

        // dct4_kernel is DCT_IV without reordering which is done before and after FFT
        dct4_kernel(real, imag);

        // Reordering of data moved from DCT_IV to here
        /* Step 1: Calculate all non-zero pairs */
        pX = X[l + offset];
        for (n = 0; n < kx/2; n++) {
            idx0 = 2*n; idx1 = idx0 + 1;
            QMF_RE(pX[idx0]) = FAAD_ANALYSIS_SCALE2( real[n   ]);
            QMF_IM(pX[idx0]) = FAAD_ANALYSIS_SCALE2( imag[n   ]);
            QMF_RE(pX[idx1]) = FAAD_ANALYSIS_SCALE2(-imag[31-n]);
            QMF_IM(pX[idx1]) = FAAD_ANALYSIS_SCALE2(-real[31-n]);
        }
        /* Step 2: Calculate a single pair with half zero'ed */
        if (kx&1) {
            idx0 = 2*n; idx1 = idx0 + 1; 
            QMF_RE(pX[idx0]) = FAAD_ANALYSIS_SCALE2( real[n]);
            QMF_IM(pX[idx0]) = FAAD_ANALYSIS_SCALE2( imag[n]);
            QMF_RE(pX[idx1]) = QMF_IM(pX[idx1]) = 0;
            n++;
        }
        /* Step 3: All other are zero'ed */
        for (; n < 16; n++) {
            idx0 = 2*n; idx1 = idx0 + 1;
            QMF_RE(pX[idx0]) = QMF_IM(pX[idx0]) = 0;
            QMF_RE(pX[idx1]) = QMF_IM(pX[idx1]) = 0;
        }
#endif /* #ifdef SBR_LOW_POWER */
    }
}

#ifdef SBR_LOW_POWER

void sbr_qmf_synthesis_32(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSR][64],
                          real_t *output)
{
    real_t x[16] MEM_ALIGN_ATTR;
    real_t y[16] MEM_ALIGN_ATTR;
    int16_t n, k, out = 0;
    uint8_t l;

    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffers */
        /* we are not shifting v, it is a double ringbuffer */
        //memmove(qmfs->v + 64, qmfs->v, (640-64)*sizeof(real_t));

        /* calculate 64 samples */
        for (k = 0; k < 16; k++)
        {
            y[k] = FAAD_ANALYSIS_SCALE3((QMF_RE(X[l][k]) - QMF_RE(X[l][31-k])));
            x[k] = FAAD_ANALYSIS_SCALE3((QMF_RE(X[l][k]) + QMF_RE(X[l][31-k])));
        }

        /* even n samples */
        DCT2_16_unscaled(x, x);
        /* odd n samples */
        DCT4_16(y, y);

        for (n = 8; n < 24; n++)
        {
            qmfs->v[qmfs->v_index + n*2  ] = qmfs->v[qmfs->v_index + 640 + n*2  ] = x[n-8];
            qmfs->v[qmfs->v_index + n*2+1] = qmfs->v[qmfs->v_index + 640 + n*2+1] = y[n-8];
        }
        for (n = 0; n < 16; n++)
        {
            qmfs->v[qmfs->v_index + n] = qmfs->v[qmfs->v_index + 640 + n] = qmfs->v[qmfs->v_index + 32-n];
        }
        qmfs->v[qmfs->v_index + 48] = qmfs->v[qmfs->v_index + 640 + 48] = 0;
        for (n = 1; n < 16; n++)
        {
            qmfs->v[qmfs->v_index + 48+n] = qmfs->v[qmfs->v_index + 640 + 48+n] = -qmfs->v[qmfs->v_index + 48-n];
        }

        /* calculate 32 output samples and window */
        for (k = 0; k < 32; k++)
        {
            output[out++] = MUL_F(qmfs->v[qmfs->v_index       + k], qmf_c[    2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  96 + k], qmf_c[1 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 128 + k], qmf_c[2 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 224 + k], qmf_c[3 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 256 + k], qmf_c[4 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 352 + k], qmf_c[5 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 384 + k], qmf_c[6 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 480 + k], qmf_c[7 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 512 + k], qmf_c[8 + 2*k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 608 + k], qmf_c[9 + 2*k*10]);
        }

        /* update the ringbuffer index */
        qmfs->v_index -= 64;
        if (qmfs->v_index < 0)
            qmfs->v_index = (640-64);
    }
}

void sbr_qmf_synthesis_64(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSR][64],
                          real_t *output)
{
    real_t x[64] MEM_ALIGN_ATTR;
    real_t y[64] MEM_ALIGN_ATTR;
    int16_t n, k, out = 0;
    uint8_t l;


    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffers */
        /* we are not shifting v, it is a double ringbuffer */
        //memmove(qmfs->v + 128, qmfs->v, (1280-128)*sizeof(real_t));

        /* calculate 128 samples */
        for (k = 0; k < 32; k++)
        {
            y[k] = FAAD_ANALYSIS_SCALE3((QMF_RE(X[l][k]) - QMF_RE(X[l][63-k])));
            x[k] = FAAD_ANALYSIS_SCALE3((QMF_RE(X[l][k]) + QMF_RE(X[l][63-k])));
        }

        /* even n samples */
        DCT2_32_unscaled(x, x);
        /* odd n samples */
        DCT4_32(y, y);

        for (n = 16; n < 48; n++)
        {
            qmfs->v[qmfs->v_index + n*2]   = qmfs->v[qmfs->v_index + 1280 + n*2  ] = x[n-16];
            qmfs->v[qmfs->v_index + n*2+1] = qmfs->v[qmfs->v_index + 1280 + n*2+1] = y[n-16];
        }
        for (n = 0; n < 32; n++)
        {
            qmfs->v[qmfs->v_index + n] = qmfs->v[qmfs->v_index + 1280 + n] = qmfs->v[qmfs->v_index + 64-n];
        }
        qmfs->v[qmfs->v_index + 96] = qmfs->v[qmfs->v_index + 1280 + 96] = 0;
        for (n = 1; n < 32; n++)
        {
            qmfs->v[qmfs->v_index + 96+n] = qmfs->v[qmfs->v_index + 1280 + 96+n] = -qmfs->v[qmfs->v_index + 96-n];
        }

        /* calculate 64 output samples and window */
        for (k = 0; k < 64; k++)
        {
            output[out++] = MUL_F(qmfs->v[qmfs->v_index              + k], qmf_c[    k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  192       + k], qmf_c[1 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  256       + k], qmf_c[2 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  256 + 192 + k], qmf_c[3 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  512       + k], qmf_c[4 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  512 + 192 + k], qmf_c[5 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  768       + k], qmf_c[6 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index +  768 + 192 + k], qmf_c[7 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 1024       + k], qmf_c[8 + k*10]) +
                            MUL_F(qmfs->v[qmfs->v_index + 1024 + 192 + k], qmf_c[9 + k*10]);
        }

        /* update the ringbuffer index */
        qmfs->v_index -= 128;
        if (qmfs->v_index < 0)
            qmfs->v_index = (1280-128);
    }
}
#else /* #ifdef SBR_LOW_POWER */

static const complex_t qmf32_pre_twiddle[] =
{
    { FRAC_CONST(0.999924701839145), FRAC_CONST(-0.012271538285720) },
    { FRAC_CONST(0.999322384588350), FRAC_CONST(-0.036807222941359) },
    { FRAC_CONST(0.998118112900149), FRAC_CONST(-0.061320736302209) },
    { FRAC_CONST(0.996312612182778), FRAC_CONST(-0.085797312344440) },
    { FRAC_CONST(0.993906970002356), FRAC_CONST(-0.110222207293883) },
    { FRAC_CONST(0.990902635427780), FRAC_CONST(-0.134580708507126) },
    { FRAC_CONST(0.987301418157858), FRAC_CONST(-0.158858143333861) },
    { FRAC_CONST(0.983105487431216), FRAC_CONST(-0.183039887955141) },
    { FRAC_CONST(0.978317370719628), FRAC_CONST(-0.207111376192219) },
    { FRAC_CONST(0.972939952205560), FRAC_CONST(-0.231058108280671) },
    { FRAC_CONST(0.966976471044852), FRAC_CONST(-0.254865659604515) },
    { FRAC_CONST(0.960430519415566), FRAC_CONST(-0.278519689385053) },
    { FRAC_CONST(0.953306040354194), FRAC_CONST(-0.302005949319228) },
    { FRAC_CONST(0.945607325380521), FRAC_CONST(-0.325310292162263) },
    { FRAC_CONST(0.937339011912575), FRAC_CONST(-0.348418680249435) },
    { FRAC_CONST(0.928506080473216), FRAC_CONST(-0.371317193951838) },
    { FRAC_CONST(0.919113851690058), FRAC_CONST(-0.393992040061048) },
    { FRAC_CONST(0.909167983090522), FRAC_CONST(-0.416429560097637) },
    { FRAC_CONST(0.898674465693954), FRAC_CONST(-0.438616238538528) },
    { FRAC_CONST(0.887639620402854), FRAC_CONST(-0.460538710958240) },
    { FRAC_CONST(0.876070094195407), FRAC_CONST(-0.482183772079123) },
    { FRAC_CONST(0.863972856121587), FRAC_CONST(-0.503538383725718) },
    { FRAC_CONST(0.851355193105265), FRAC_CONST(-0.524589682678469) },
    { FRAC_CONST(0.838224705554838), FRAC_CONST(-0.545324988422046) },
    { FRAC_CONST(0.824589302785025), FRAC_CONST(-0.565731810783613) },
    { FRAC_CONST(0.810457198252595), FRAC_CONST(-0.585797857456439) },
    { FRAC_CONST(0.795836904608884), FRAC_CONST(-0.605511041404326) },
    { FRAC_CONST(0.780737228572094), FRAC_CONST(-0.624859488142386) },
    { FRAC_CONST(0.765167265622459), FRAC_CONST(-0.643831542889791) },
    { FRAC_CONST(0.749136394523459), FRAC_CONST(-0.662415777590172) },
    { FRAC_CONST(0.732654271672413), FRAC_CONST(-0.680600997795453) },
    { FRAC_CONST(0.715730825283819), FRAC_CONST(-0.698376249408973) }
};

#define FAAD_CMPLX_PRETWIDDLE_SUB(k) \
        (MUL_F(QMF_RE(X[l][k]), RE(qmf32_pre_twiddle[k])) - \
         MUL_F(QMF_IM(X[l][k]), IM(qmf32_pre_twiddle[k])))
        
#define FAAD_CMPLX_PRETWIDDLE_ADD(k) \
        (MUL_F(QMF_IM(X[l][k]), RE(qmf32_pre_twiddle[k])) + \
         MUL_F(QMF_RE(X[l][k]), IM(qmf32_pre_twiddle[k])))

void sbr_qmf_synthesis_32(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSR][64],
                          real_t *output)
{
    real_t x1[32] MEM_ALIGN_ATTR;
    real_t x2[32] MEM_ALIGN_ATTR;
    int32_t n, k, idx0, idx1, out = 0;
    uint32_t l;

    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffer v */
        /* buffer is not shifted, we are using a ringbuffer */
        //memmove(qmfs->v + 64, qmfs->v, (640-64)*sizeof(real_t));

        /* calculate 64 samples */
        /* complex pre-twiddle */
        for (k = 0; k < 32;)
        {
            x1[k] = FAAD_CMPLX_PRETWIDDLE_SUB(k); x2[k] = FAAD_CMPLX_PRETWIDDLE_ADD(k); k++;
            x1[k] = FAAD_CMPLX_PRETWIDDLE_SUB(k); x2[k] = FAAD_CMPLX_PRETWIDDLE_ADD(k); k++;
            x1[k] = FAAD_CMPLX_PRETWIDDLE_SUB(k); x2[k] = FAAD_CMPLX_PRETWIDDLE_ADD(k); k++;
            x1[k] = FAAD_CMPLX_PRETWIDDLE_SUB(k); x2[k] = FAAD_CMPLX_PRETWIDDLE_ADD(k); k++;
        }

        /* transform */
        DCT4_32(x1, x1);
        DST4_32(x2, x2);

        idx0 = qmfs->v_index; 
        idx1 = qmfs->v_index + 63;
        for (n = 0; n < 32; n+=2)
        {
            qmfs->v[idx0] = qmfs->v[idx0 + 640] = -x1[n  ] + x2[n  ]; idx0++;
            qmfs->v[idx1] = qmfs->v[idx1 + 640] =  x1[n  ] + x2[n  ]; idx1--;
            qmfs->v[idx0] = qmfs->v[idx0 + 640] = -x1[n+1] + x2[n+1]; idx0++;
            qmfs->v[idx1] = qmfs->v[idx1 + 640] =  x1[n+1] + x2[n+1]; idx1--;
        }

        /* calculate 32 output samples and window */
        for (k = 0; k < 32; k++)
        {
            idx0 = qmfs->v_index + k; idx1 = 2*k*10;
            output[out++] = FAAD_SYNTHESIS_SCALE(
                            MUL_F(qmfs->v[idx0      ], qmf_c[idx1  ]) +
                            MUL_F(qmfs->v[idx0 +  96], qmf_c[idx1+1]) +
                            MUL_F(qmfs->v[idx0 + 128], qmf_c[idx1+2]) +
                            MUL_F(qmfs->v[idx0 + 224], qmf_c[idx1+3]) +
                            MUL_F(qmfs->v[idx0 + 256], qmf_c[idx1+4]) +
                            MUL_F(qmfs->v[idx0 + 352], qmf_c[idx1+5]) +
                            MUL_F(qmfs->v[idx0 + 384], qmf_c[idx1+6]) +
                            MUL_F(qmfs->v[idx0 + 480], qmf_c[idx1+7]) +
                            MUL_F(qmfs->v[idx0 + 512], qmf_c[idx1+8]) +
                            MUL_F(qmfs->v[idx0 + 608], qmf_c[idx1+9]));
        }

        /* update ringbuffer index */
        qmfs->v_index -= 64;
        if (qmfs->v_index < 0)
            qmfs->v_index = (640 - 64);
    }
}

void sbr_qmf_synthesis_64(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSR][64],
                          real_t *output)
{
    real_t real1[32] MEM_ALIGN_ATTR; 
    real_t imag1[32] MEM_ALIGN_ATTR;
    real_t real2[32] MEM_ALIGN_ATTR; 
    real_t imag2[32] MEM_ALIGN_ATTR;
    qmf_t *pX;
    real_t *p_buf_1, *p_buf_3;
    int32_t n, k, idx0, idx1, out = 0;
    uint32_t l;
    
    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffer v */
        /* buffer is not shifted, we use double ringbuffer */
        //memmove(qmfs->v + 128, qmfs->v, (1280-128)*sizeof(real_t));

        /* calculate 128 samples */
        pX = X[l];
        for (k = 0; k < 32; k++)
        {
            idx0 = 2*k; idx1 = idx0+1;
            real1[   k] = QMF_RE(pX[idx0]); imag2[   k] = QMF_IM(pX[idx0]);
            imag1[31-k] = QMF_RE(pX[idx1]); real2[31-k] = QMF_IM(pX[idx1]);    
        }
        
        // dct4_kernel is DCT_IV without reordering which is done before and after FFT
        dct4_kernel(real1, imag1);
        dct4_kernel(real2, imag2);

        p_buf_1 = qmfs->v + qmfs->v_index;
        p_buf_3 = p_buf_1 + 1280;

        idx0 = 0; idx1 = 127;
        for (n = 0; n < 32; n++)
        {
            p_buf_1[idx0] = p_buf_3[idx0] = real2[   n] - real1[   n]; idx0++;
            p_buf_1[idx1] = p_buf_3[idx1] = real2[   n] + real1[   n]; idx1--;
            p_buf_1[idx0] = p_buf_3[idx0] = imag2[31-n] + imag1[31-n]; idx0++;
            p_buf_1[idx1] = p_buf_3[idx1] = imag2[31-n] - imag1[31-n]; idx1--;
        }

        p_buf_1 = qmfs->v + qmfs->v_index;

        /* calculate 64 output samples and window */
#ifdef CPU_ARM
        const real_t *qtab = qmf_c;
        real_t *pbuf = p_buf_1;
        for (k = 0; k < 64; k++, pbuf++)
        {
            real_t *pout = &output[out++];
            asm volatile (
               "ldmia %[qtab]!, { r0-r3 } \n\t"
               "ldr r4, [%[pbuf]]         \n\t"
               "ldr r7, [%[pbuf], #192*4] \n\t"
               "smull r5, r6, r4, r0      \n\t"
               "ldr r4, [%[pbuf], #256*4] \n\t"
               "smlal r5, r6, r7, r1      \n\t"
               "ldr r7, [%[pbuf], #448*4] \n\t"
               "smlal r5, r6, r4, r2      \n\t"
               "ldr r4, [%[pbuf], #512*4] \n\t"
               "smlal r5, r6, r7, r3      \n\t"

               "ldmia %[qtab]!, { r0-r3 } \n\t"
               "ldr r7, [%[pbuf], #704*4] \n\t"
               "smlal r5, r6, r4, r0      \n\t"
               "ldr r4, [%[pbuf], #768*4] \n\t"
               "smlal r5, r6, r7, r1      \n\t"
               "ldr r7, [%[pbuf], #960*4] \n\t"
               "smlal r5, r6, r4, r2      \n\t"
               "mov r2, #1024*4           \n\t"

               "ldmia %[qtab]!, { r0-r1 } \n\t"
               "ldr r4, [%[pbuf], r2]     \n\t"
               "smlal r5, r6, r7, r3      \n\t"
               "mov r2, #1216*4           \n\t"
               "ldr r7, [%[pbuf], r2]     \n\t"
               "smlal r5, r6, r4, r0      \n\t"
               "smlal r5, r6, r7, r1      \n\t"

               "str r6, [%[pout]]         \n"
               : [qtab] "+r" (qtab)
               : [pbuf] "r" (pbuf), [pout] "r" (pout)
               : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "memory");
        }
#elif defined CPU_COLDFIRE
        const real_t *qtab = qmf_c;
        real_t *pbuf = p_buf_1;
        for (k = 0; k < 64; k++, pbuf++)
        {
            real_t *pout = &output[out++];
            asm volatile (
               "move.l  (%[pbuf]), %%d5                              \n"

               "movem.l (%[qtab]), %%d0-%%d4                         \n"
               "mac.l   %%d0, %%d5, (192*4, %[pbuf]), %%d5, %%acc0   \n"
               "mac.l   %%d1, %%d5, (256*4, %[pbuf]), %%d5, %%acc0   \n"
               "mac.l   %%d2, %%d5, (448*4, %[pbuf]), %%d5, %%acc0   \n"
               "mac.l   %%d3, %%d5, (512*4, %[pbuf]), %%d5, %%acc0   \n"
               "mac.l   %%d4, %%d5, (704*4, %[pbuf]), %%d5, %%acc0   \n"
               "lea.l   (20, %[qtab]), %[qtab]                       \n"

               "movem.l (%[qtab]), %%d0-%%d4                         \n"
               "mac.l   %%d0, %%d5, (768*4, %[pbuf]), %%d5, %%acc0   \n"
               "mac.l   %%d1, %%d5, (960*4, %[pbuf]), %%d5, %%acc0   \n"
               "mac.l   %%d2, %%d5, (1024*4, %[pbuf]), %%d5, %%acc0  \n"
               "mac.l   %%d3, %%d5, (1216*4, %[pbuf]), %%d5, %%acc0  \n"
               "mac.l   %%d4, %%d5, %%acc0                           \n"
               "lea.l   (20, %[qtab]), %[qtab]                       \n"

               "movclr.l %%acc0, %%d0                                \n"
               "move.l  %%d0, (%[pout])                              \n"
               : [qtab] "+a" (qtab)
               : [pbuf] "a" (pbuf),
                 [pout] "a" (pout)
               : "d0", "d1", "d2", "d3", "d4", "d5", "memory");
        }
#else
        for (k = 0; k < 64; k++)
        {
            idx0 = k*10;
            output[out++] = FAAD_SYNTHESIS_SCALE(
                            MUL_F(p_buf_1[k         ], qmf_c[idx0  ]) +
                            MUL_F(p_buf_1[k+ 192    ], qmf_c[idx0+1]) +
                            MUL_F(p_buf_1[k+ 256    ], qmf_c[idx0+2]) +
                            MUL_F(p_buf_1[k+ 256+192], qmf_c[idx0+3]) +
                            MUL_F(p_buf_1[k+ 512    ], qmf_c[idx0+4]) +
                            MUL_F(p_buf_1[k+ 512+192], qmf_c[idx0+5]) +
                            MUL_F(p_buf_1[k+ 768    ], qmf_c[idx0+6]) +
                            MUL_F(p_buf_1[k+ 768+192], qmf_c[idx0+7]) +
                            MUL_F(p_buf_1[k+1024    ], qmf_c[idx0+8]) +
                            MUL_F(p_buf_1[k+1024+192], qmf_c[idx0+9]));
        }
#endif

        /* update ringbuffer index */
        qmfs->v_index -= 128;
        if (qmfs->v_index < 0)
            qmfs->v_index = (1280 - 128);
    }
}
#endif /* #ifdef SBR_LOW_POWER */

#endif /* #ifdef SBR_DEC */
