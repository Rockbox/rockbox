/*
 * Atrac 3 compatible decoder
 * Copyright (c) 2006-2008 Maxim Poliakovski
 * Copyright (c) 2006-2008 Benjamin Larsson
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/atrac3.c
 * Atrac 3 compatible decoder.
 * This decoder handles Sony's ATRAC3 data.
 *
 * Container formats used to store atrac 3 data:
 * RealMedia (.rm), RIFF WAV (.wav, .at3), Sony OpenMG (.oma, .aa3).
 *
 * To use this decoder, a calling application must supply the extradata
 * bytes provided in the containers above.
 */

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "avcodec.h"
#include "bitstream.h"
#include "bytestream.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "../librm/rm.h"
#include "atrac3data.h"
#include "atrac3data_fixed.h"
#include "fixp_math.h"
//#include "fixp_mdct.h"
#include "../lib/mdct2.h"

#define JOINT_STEREO    0x12
#define STEREO          0x2

#define AVERROR(...) -1

/* These structures are needed to store the parsed gain control data. */
typedef struct {
    int   num_gain_data;
    int   levcode[8];
    int   loccode[8];
} gain_info;

typedef struct {
    gain_info   gBlock[4];
} gain_block;

typedef struct {
    int     pos;
    int     numCoefs;
    int32_t coef[8];
} tonal_component;

typedef struct {
    int               bandsCoded;
    int               numComponents;
    tonal_component   components[64];
    int32_t           prevFrame[1024];
    int               gcBlkSwitch;
    gain_block        gainBlock[2];

    int32_t           spectrum[1024] __attribute__((aligned(16)));
    int32_t           IMDCT_buf[1024] __attribute__((aligned(16)));

    int32_t           delayBuf1[46]; ///<qmf delay buffers
    int32_t           delayBuf2[46];
    int32_t           delayBuf3[46];
} channel_unit;

typedef struct {
    GetBitContext       gb;
    //@{
    /** stream data */
    int                 channels;
    int                 codingMode;
    int                 bit_rate;
    int                 sample_rate;
    int                 samples_per_channel;
    int                 samples_per_frame;

    int                 bits_per_frame;
    int                 bytes_per_frame;
    int                 pBs;
    channel_unit*       pUnits;
    //@}
    //@{
    /** joint-stereo related variables */
    int                 matrix_coeff_index_prev[4];
    int                 matrix_coeff_index_now[4];
    int                 matrix_coeff_index_next[4];
    int                 weighting_delay[6];
    //@}
    //@{
    /** data buffers */
    int32_t             outSamples[2048];
    uint8_t             decoded_bytes_buffer[1024];
    int32_t             tempBuf[1070];
    //@}
    //@{
    /** extradata */
    int                 atrac3version;
    int                 delay;
    int                 scrambled_stream;
    int                 frame_factor;
    //@}
} ATRAC3Context;

static int32_t          qmf_window[48];
static VLC              spectral_coeff_tab[7];
static channel_unit     channel_units[2];
/**
 * Quadrature mirror synthesis filter.
 *
 * @param inlo      lower part of spectrum
 * @param inhi      higher part of spectrum
 * @param nIn       size of spectrum buffer
 * @param pOut      out buffer
 * @param delayBuf  delayBuf buffer
 * @param temp      temp buffer
 */
static void iqmf (int32_t *inlo, int32_t *inhi, unsigned int nIn, int32_t *pOut, int32_t *delayBuf, int32_t *temp)
{
    int   i, j;
    int32_t   *p1, *p3;

    memcpy(temp, delayBuf, 46*sizeof(int32_t));

    p3 = temp + 46;

    /* loop1 */
    for(i=0; i<nIn; i+=2){
        p3[2*i+0] = inlo[i  ] + inhi[i  ];
        p3[2*i+1] = inlo[i  ] - inhi[i  ];
        p3[2*i+2] = inlo[i+1] + inhi[i+1];
        p3[2*i+3] = inlo[i+1] - inhi[i+1];
    }

    /* loop2 */
    p1 = temp;
    for (j = nIn; j != 0; j--) {
        int32_t s1 = 0;
        int32_t s2 = 0;

        for (i = 0; i < 48; i += 2) {
            s1 += fixmul31(p1[i], qmf_window[i]);
            s2 += fixmul31(p1[i+1], qmf_window[i+1]);
        }

        pOut[0] = s2;
        pOut[1] = s1;

        p1 += 2;
        pOut += 2;
    }

    /* Update the delay buffer. */
    memcpy(delayBuf, temp + (nIn << 1), 46*sizeof(int32_t));
}

/**
 * Regular 512 points IMDCT without overlapping, with the exception of the swapping of odd bands
 * caused by the reverse spectra of the QMF.
 *
 * @param pInput    float input
 * @param pOutput   float output
 * @param odd_band  1 if the band is an odd band
 */

static void IMLT(int32_t *pInput, int32_t *pOutput, int odd_band)
{
    int     i;
    if (odd_band) {
        /**
        * Reverse the odd bands before IMDCT, this is an effect of the QMF transform
        * or it gives better compression to do it this way.
        * FIXME: It should be possible to handle this in ff_imdct_calc
        * for that to happen a modification of the prerotation step of
        * all SIMD code and C code is needed.
        * Or fix the functions before so they generate a pre reversed spectrum.
        */

        for (i=0; i<128; i++)
            FFSWAP(int32_t, pInput[i], pInput[255-i]);
    }
 
    /* Apply the imdct. */
    mdct_backward(512, pInput, pOutput);

    /* Windowing. */
    for(i = 0; i<512; i++)
        pOutput[i] = fixmul31(pOutput[i], window_lookup[i]);

}


/**
 * Atrac 3 indata descrambling, only used for data coming from the rm container
 *
 * @param in        pointer to 8 bit array of indata
 * @param bits      amount of bits
 * @param out       pointer to 8 bit array of outdata
 */

static int decode_bytes(const uint8_t* inbuffer, uint8_t* out, int bytes){
    int i, off;
    uint32_t c;
    const uint32_t* buf;
    uint32_t* obuf = (uint32_t*) out;

#ifdef TEST
    off = 0; //no check for memory alignment of inbuffer
#else
    off = (intptr_t)inbuffer & 3;
#endif /* TEST */
    buf = (const uint32_t*) (inbuffer - off);

    c = be2me_32((0x537F6103 >> (off*8)) | (0x537F6103 << (32-(off*8))));
    bytes += 3 + off;
    for (i = 0; i < bytes/4; i++)
        obuf[i] = c ^ buf[i];

    if (off)
        av_log(NULL,AV_LOG_DEBUG,"Offset of %d not handled, post sample on ffmpeg-dev.\n",off);

    return off;
}


static av_cold void init_atrac3_transforms(ATRAC3Context *q) {
    int32_t s;
    int i;

    /* Generate the mdct window, for details see
     * http://wiki.multimedia.cx/index.php?title=RealAudio_atrc#Windows */

    /* mdct window had been generated and saved as a lookup table in atrac3data_fixed.h */

    /* Generate the QMF window. */
    for (i=0 ; i<24; i++) {
        s = qmf_48tap_half_fix[i] << 1;
        qmf_window[i] = s;
        qmf_window[47 - i] = s;
    }
}

/**
 * Mantissa decoding
 *
 * @param gb            the GetBit context
 * @param selector      what table is the output values coded with
 * @param codingFlag    constant length coding or variable length coding
 * @param mantissas     mantissa output table
 * @param numCodes      amount of values to get
 */

static void readQuantSpectralCoeffs (GetBitContext *gb, int selector, int codingFlag, int* mantissas, int numCodes)
{
    int   numBits, cnt, code, huffSymb;

    if (selector == 1)
        numCodes /= 2;

    if (codingFlag != 0) {
        /* constant length coding (CLC) */
        numBits = CLCLengthTab[selector];

        if (selector > 1) {
            for (cnt = 0; cnt < numCodes; cnt++) {
                if (numBits)
                    code = get_sbits(gb, numBits);
                else
                    code = 0;
                mantissas[cnt] = code;
            }
        } else {
            for (cnt = 0; cnt < numCodes; cnt++) {
                if (numBits)
                    code = get_bits(gb, numBits); //numBits is always 4 in this case
                else
                    code = 0;
                mantissas[cnt*2] = seTab_0[code >> 2];
                mantissas[cnt*2+1] = seTab_0[code & 3];
            }
        }
    } else {
        /* variable length coding (VLC) */
        if (selector != 1) {
            for (cnt = 0; cnt < numCodes; cnt++) {
                huffSymb = get_vlc2(gb, spectral_coeff_tab[selector-1].table, spectral_coeff_tab[selector-1].bits, 3);
                huffSymb += 1;
                code = huffSymb >> 1;
                if (huffSymb & 1)
                    code = -code;
                mantissas[cnt] = code;
            }
        } else {
            for (cnt = 0; cnt < numCodes; cnt++) {
                huffSymb = get_vlc2(gb, spectral_coeff_tab[selector-1].table, spectral_coeff_tab[selector-1].bits, 3);
                mantissas[cnt*2] = decTable1[huffSymb*2];
                mantissas[cnt*2+1] = decTable1[huffSymb*2+1];
            }
        }
    }
}

/**
 * Restore the quantized band spectrum coefficients
 *
 * @param gb            the GetBit context
 * @param pOut          decoded band spectrum
 * @return outSubbands   subband counter, fix for broken specification/files
 */

static int decodeSpectrum (GetBitContext *gb, int32_t *pOut)
{
    int   numSubbands, codingMode, cnt, first, last, subbWidth, *pIn;
    int   subband_vlc_index[32], SF_idxs[32];
    int   mantissas[128];
    int32_t SF;

    numSubbands = get_bits(gb, 5); // number of coded subbands
    codingMode = get_bits1(gb); // coding Mode: 0 - VLC/ 1-CLC

    /* Get the VLC selector table for the subbands, 0 means not coded. */
    for (cnt = 0; cnt <= numSubbands; cnt++)
        subband_vlc_index[cnt] = get_bits(gb, 3);

    /* Read the scale factor indexes from the stream. */
    for (cnt = 0; cnt <= numSubbands; cnt++) {
        if (subband_vlc_index[cnt] != 0)
            SF_idxs[cnt] = get_bits(gb, 6);
    }

    for (cnt = 0; cnt <= numSubbands; cnt++) {
        first = subbandTab[cnt];
        last = subbandTab[cnt+1];

        subbWidth = last - first;

        if (subband_vlc_index[cnt] != 0) {
            /* Decode spectral coefficients for this subband. */
            /* TODO: This can be done faster is several blocks share the
             * same VLC selector (subband_vlc_index) */
            readQuantSpectralCoeffs (gb, subband_vlc_index[cnt], codingMode, mantissas, subbWidth);

            /* Decode the scale factor for this subband. */
            SF = fixmul31(SFTable_fixed[SF_idxs[cnt]], iMaxQuant_fix[subband_vlc_index[cnt]]);

            /* Inverse quantize the coefficients. */
            for (pIn=mantissas ; first<last; first++, pIn++)
                pOut[first] = fixmul16(*pIn, SF);
        } else {
            /* This subband was not coded, so zero the entire subband. */
            memset(pOut+first, 0, subbWidth*sizeof(int32_t));
        }
    }

    /* Clear the subbands that were not coded. */
    first = subbandTab[cnt];
    memset(pOut+first, 0, (1024 - first) * sizeof(int32_t));
    return numSubbands;
}

/**
 * Restore the quantized tonal components
 *
 * @param gb            the GetBit context
 * @param pComponent    tone component
 * @param numBands      amount of coded bands
 */

static int decodeTonalComponents (GetBitContext *gb, tonal_component *pComponent, int numBands)
{
    int i,j,k,cnt;
    int   components, coding_mode_selector, coding_mode, coded_values_per_component;
    int   sfIndx, coded_values, max_coded_values, quant_step_index, coded_components;
    int   band_flags[4], mantissa[8];
    int32_t  *pCoef;
    int32_t  scalefactor;
    int   component_count = 0;

    components = get_bits(gb,5);

    /* no tonal components */
    if (components == 0)
        return 0;

    coding_mode_selector = get_bits(gb,2);
    if (coding_mode_selector == 2)
        return -1;

    coding_mode = coding_mode_selector & 1;

    for (i = 0; i < components; i++) {
        for (cnt = 0; cnt <= numBands; cnt++)
            band_flags[cnt] = get_bits1(gb);

        coded_values_per_component = get_bits(gb,3);

        quant_step_index = get_bits(gb,3);
        if (quant_step_index <= 1)
            return -1;

        if (coding_mode_selector == 3)
            coding_mode = get_bits1(gb);

        for (j = 0; j < (numBands + 1) * 4; j++) {
            if (band_flags[j >> 2] == 0)
                continue;

            coded_components = get_bits(gb,3);

            for (k=0; k<coded_components; k++) {
                sfIndx = get_bits(gb,6);
                pComponent[component_count].pos = j * 64 + (get_bits(gb,6));
                max_coded_values = 1024 - pComponent[component_count].pos;
                coded_values = coded_values_per_component + 1;
                coded_values = FFMIN(max_coded_values,coded_values);

                scalefactor = fixmul31(SFTable_fixed[sfIndx], iMaxQuant_fix[quant_step_index]);

                readQuantSpectralCoeffs(gb, quant_step_index, coding_mode, mantissa, coded_values);

                pComponent[component_count].numCoefs = coded_values;

                /* inverse quant */
                pCoef = pComponent[component_count].coef;
                for (cnt = 0; cnt < coded_values; cnt++)
                    pCoef[cnt] = fixmul16(mantissa[cnt], scalefactor);

                component_count++;
            }
        }
    }

    return component_count;
}

/**
 * Decode gain parameters for the coded bands
 *
 * @param gb            the GetBit context
 * @param pGb           the gainblock for the current band
 * @param numBands      amount of coded bands
 */

static int decodeGainControl (GetBitContext *gb, gain_block *pGb, int numBands)
{
    int   i, cf, numData;
    int   *pLevel, *pLoc;

    gain_info   *pGain = pGb->gBlock;

    for (i=0 ; i<=numBands; i++)
    {
        numData = get_bits(gb,3);
        pGain[i].num_gain_data = numData;
        pLevel = pGain[i].levcode;
        pLoc = pGain[i].loccode;

        for (cf = 0; cf < numData; cf++){
            pLevel[cf]= get_bits(gb,4);
            pLoc  [cf]= get_bits(gb,5);
            if(cf && pLoc[cf] <= pLoc[cf-1])
                return -1;
        }
    }

    /* Clear the unused blocks. */
    for (; i<4 ; i++)
        pGain[i].num_gain_data = 0;

    return 0;
}

/**
 * Apply gain parameters and perform the MDCT overlapping part
 *
 * @param pIn           input float buffer
 * @param pPrev         previous float buffer to perform overlap against
 * @param pOut          output float buffer
 * @param pGain1        current band gain info
 * @param pGain2        next band gain info
 */

static void gainCompensateAndOverlap (int32_t *pIn, int32_t *pPrev, int32_t *pOut, gain_info *pGain1, gain_info *pGain2)
{
    /* gain compensation function */
    int32_t  gain1, gain2, gain_inc;
    int   cnt, numdata, nsample, startLoc, endLoc;


    if (pGain2->num_gain_data == 0)
        gain1 = ONE_16;
    else
        gain1 = gain_tab1[pGain2->levcode[0]];

    if (pGain1->num_gain_data == 0) {
        for (cnt = 0; cnt < 256; cnt++)
            pOut[cnt] = fixmul16(pIn[cnt], gain1) + pPrev[cnt];
    } else {
        numdata = pGain1->num_gain_data;
        pGain1->loccode[numdata] = 32;
        pGain1->levcode[numdata] = 4;

        nsample = 0; // current sample = 0

        for (cnt = 0; cnt < numdata; cnt++) {
            startLoc = pGain1->loccode[cnt] * 8;
            endLoc = startLoc + 8;

            gain2 = gain_tab1[pGain1->levcode[cnt]];
            gain_inc = gain_tab2[(pGain1->levcode[cnt+1] - pGain1->levcode[cnt])+15];

            /* interpolate */
            for (; nsample < startLoc; nsample++)
                pOut[nsample] = fixmul16((fixmul16(pIn[nsample], gain1) + pPrev[nsample]), gain2);

            /* interpolation is done over eight samples */
            for (; nsample < endLoc; nsample++) {
                pOut[nsample] = fixmul16((fixmul16(pIn[nsample], gain1) + pPrev[nsample]),gain2);
                gain2 = fixmul16(gain2, gain_inc);
            }
        }

        for (; nsample < 256; nsample++)
            pOut[nsample] = fixmul16(pIn[nsample], gain1) + pPrev[nsample];
    }

    /* Delay for the overlapping part. */
    memcpy(pPrev, &pIn[256], 256*sizeof(int32_t));
}

/**
 * Combine the tonal band spectrum and regular band spectrum
 * Return position of the last tonal coefficient

 *
 * @param pSpectrum     output spectrum buffer
 * @param numComponents amount of tonal components
 * @param pComponent    tonal components for this band
 */

static int addTonalComponents (int32_t *pSpectrum, int numComponents, tonal_component *pComponent)
{
    int   cnt, i, lastPos = -1;
    int32_t *pOut;
    int32_t *pIn;

    for (cnt = 0; cnt < numComponents; cnt++){
        lastPos = FFMAX(pComponent[cnt].pos + pComponent[cnt].numCoefs, lastPos);
        pIn = pComponent[cnt].coef;
        pOut = &(pSpectrum[pComponent[cnt].pos]);

        for (i=0 ; i<pComponent[cnt].numCoefs ; i++)
            pOut[i] += pIn[i];
    }

    return lastPos;
}


#define INTERPOLATE(old,new,nsample)  ((old*ONE_16) + fixmul16(((nsample*ONE_16)>>3), (((new) - (old))*ONE_16)))

static void reverseMatrixing(int32_t *su1, int32_t *su2, int *pPrevCode, int *pCurrCode)
{
    int    i, band, nsample, s1, s2;
    int32_t    c1, c2;
    int32_t    mc1_l, mc1_r, mc2_l, mc2_r;

    for (i=0,band = 0; band < 4*256; band+=256,i++) {
        s1 = pPrevCode[i];
        s2 = pCurrCode[i];
        nsample = 0;

        if (s1 != s2) {
            /* Selector value changed, interpolation needed. */
            mc1_l = matrixCoeffs_fix[s1<<1];
            mc1_r = matrixCoeffs_fix[(s1<<1)+1];
            mc2_l = matrixCoeffs_fix[s2<<1];
            mc2_r = matrixCoeffs_fix[(s2<<1)+1];

            /* Interpolation is done over the first eight samples. */
            for(; nsample < 8; nsample++) {
                c1 = su1[band+nsample];
                c2 = su2[band+nsample];
                c2 = fixmul16(c1, INTERPOLATE(mc1_l, mc2_l, nsample)) + fixmul16(c2, INTERPOLATE(mc1_r, mc2_r, nsample));
                su1[band+nsample] = c2;
                su2[band+nsample] = (c1 << 1) - c2;
            }
        }

        /* Apply the matrix without interpolation. */
        switch (s2) {
            case 0:     /* M/S decoding */
                for (; nsample < 256; nsample++) {
                    c1 = su1[band+nsample];
                    c2 = su2[band+nsample];
                    su1[band+nsample] = c2 << 1;
                    su2[band+nsample] = (c1 - c2) << 1;
                }
                break;

            case 1:
                for (; nsample < 256; nsample++) {
                    c1 = su1[band+nsample];
                    c2 = su2[band+nsample];
                    su1[band+nsample] = (c1 + c2) << 1;
                    su2[band+nsample] = -1*(c2 << 1);
                }
                break;
            case 2:
            case 3:
                for (; nsample < 256; nsample++) {
                    c1 = su1[band+nsample];
                    c2 = su2[band+nsample];
                    su1[band+nsample] = c1 + c2;
                    su2[band+nsample] = c1 - c2;
                }
                break;
            default:
                assert(0);
        }
    }
}

static void getChannelWeights (int indx, int flag, int32_t ch[2]){
    if (indx == 7) {
        ch[0] = ONE_16;
        ch[1] = ONE_16;
    } else {
        ch[0] = fixdiv16(((indx & 7)*ONE_16), 7*ONE_16);
        ch[1] = fastSqrt((ONE_16 << 1) - fixmul16(ch[0], ch[0]));
        if(flag)
            FFSWAP(int32_t, ch[0], ch[1]);
    }
}

static void channelWeighting (int32_t *su1, int32_t *su2, int *p3)
{
    int   band, nsample;
    /* w[x][y] y=0 is left y=1 is right */
    int32_t w[2][2];

    if (p3[1] != 7 || p3[3] != 7){
        getChannelWeights(p3[1], p3[0], w[0]);
        getChannelWeights(p3[3], p3[2], w[1]);

        for(band = 1; band < 4; band++) {
            /* scale the channels by the weights */
            for(nsample = 0; nsample < 8; nsample++) {
                su1[band*256+nsample] = fixmul16(su1[band*256+nsample], INTERPOLATE(w[0][0], w[0][1], nsample));
                su2[band*256+nsample] = fixmul16(su2[band*256+nsample], INTERPOLATE(w[1][0], w[1][1], nsample));
            }

            for(; nsample < 256; nsample++) {
                su1[band*256+nsample] = fixmul16(su1[band*256+nsample], w[1][0]);
                su2[band*256+nsample] = fixmul16(su2[band*256+nsample], w[1][1]);
            }
        }
    }
}


/**
 * Decode a Sound Unit
 *
 * @param gb            the GetBit context
 * @param pSnd          the channel unit to be used
 * @param pOut          the decoded samples before IQMF in float representation
 * @param channelNum    channel number
 * @param codingMode    the coding mode (JOINT_STEREO or regular stereo/mono)
 */


static int decodeChannelSoundUnit (ATRAC3Context *q, GetBitContext *gb, channel_unit *pSnd, int32_t *pOut, int channelNum, int codingMode)
{
    int   band, result=0, numSubbands, lastTonal, numBands;
    if (codingMode == JOINT_STEREO && channelNum == 1) {
        if (get_bits(gb,2) != 3) {
            av_log(NULL,AV_LOG_ERROR,"JS mono Sound Unit id != 3.\n");
            return -1;
        }
    } else {
        if (get_bits(gb,6) != 0x28) {
            av_log(NULL,AV_LOG_ERROR,"Sound Unit id != 0x28.\n");
            return -1;
        }
    }

    /* number of coded QMF bands */
    pSnd->bandsCoded = get_bits(gb,2);

    result = decodeGainControl (gb, &(pSnd->gainBlock[pSnd->gcBlkSwitch]), pSnd->bandsCoded);
    if (result) return result;

    pSnd->numComponents = decodeTonalComponents (gb, pSnd->components, pSnd->bandsCoded);
    if (pSnd->numComponents == -1) return -1;

    numSubbands = decodeSpectrum (gb, pSnd->spectrum);

    /* Merge the decoded spectrum and tonal components. */
    lastTonal = addTonalComponents (pSnd->spectrum, pSnd->numComponents, pSnd->components);


    /* calculate number of used MLT/QMF bands according to the amount of coded spectral lines */
    numBands = (subbandTab[numSubbands] - 1) >> 8;
    if (lastTonal >= 0)
        numBands = FFMAX((lastTonal + 256) >> 8, numBands);


    /* Reconstruct time domain samples. */
    for (band=0; band<4; band++) {
        /* Perform the IMDCT step without overlapping. */
        if (band <= numBands) {
            IMLT(&(pSnd->spectrum[band*256]), pSnd->IMDCT_buf, band&1);
        } else
            memset(pSnd->IMDCT_buf, 0, 512 * sizeof(int32_t));

        /* gain compensation and overlapping */
        gainCompensateAndOverlap (pSnd->IMDCT_buf, &(pSnd->prevFrame[band*256]), &(pOut[band*256]),
                                    &((pSnd->gainBlock[1 - (pSnd->gcBlkSwitch)]).gBlock[band]),
                                    &((pSnd->gainBlock[pSnd->gcBlkSwitch]).gBlock[band]));
    }

    /* Swap the gain control buffers for the next frame. */
    pSnd->gcBlkSwitch ^= 1;

    return 0;
}

/**
 * Frame handling
 *
 * @param q             Atrac3 private context
 * @param databuf       the input data
 */

static int decodeFrame(ATRAC3Context *q, const uint8_t* databuf)
{
    int   result, i;
    int32_t   *p1, *p2, *p3, *p4;
    uint8_t *ptr1;

    if (q->codingMode == JOINT_STEREO) {

        /* channel coupling mode */
        /* decode Sound Unit 1 */
        init_get_bits(&q->gb,databuf,q->bits_per_frame);

        result = decodeChannelSoundUnit(q,&q->gb, q->pUnits, q->outSamples, 0, JOINT_STEREO);
        if (result != 0)
            return (result);

        /* Framedata of the su2 in the joint-stereo mode is encoded in
         * reverse byte order so we need to swap it first. */
        if (databuf == q->decoded_bytes_buffer) {
            uint8_t *ptr2 = q->decoded_bytes_buffer+q->bytes_per_frame-1;
            ptr1 = q->decoded_bytes_buffer;
            for (i = 0; i < (q->bytes_per_frame/2); i++, ptr1++, ptr2--) {
                FFSWAP(uint8_t,*ptr1,*ptr2);
            }
        } else {
            const uint8_t *ptr2 = databuf+q->bytes_per_frame-1;
            for (i = 0; i < q->bytes_per_frame; i++)
                q->decoded_bytes_buffer[i] = *ptr2--;
        }

        /* Skip the sync codes (0xF8). */
        ptr1 = q->decoded_bytes_buffer;
        for (i = 4; *ptr1 == 0xF8; i++, ptr1++) {
            if (i >= q->bytes_per_frame)
                return -1;
        }


        /* set the bitstream reader at the start of the second Sound Unit*/
        init_get_bits(&q->gb,ptr1,q->bits_per_frame);

        /* Fill the Weighting coeffs delay buffer */
        memmove(q->weighting_delay,&(q->weighting_delay[2]),4*sizeof(int));
        q->weighting_delay[4] = get_bits1(&q->gb);
        q->weighting_delay[5] = get_bits(&q->gb,3);

        for (i = 0; i < 4; i++) {
            q->matrix_coeff_index_prev[i] = q->matrix_coeff_index_now[i];
            q->matrix_coeff_index_now[i] = q->matrix_coeff_index_next[i];
            q->matrix_coeff_index_next[i] = get_bits(&q->gb,2);
        }

        /* Decode Sound Unit 2. */
        result = decodeChannelSoundUnit(q,&q->gb, &q->pUnits[1], &q->outSamples[1024], 1, JOINT_STEREO);
        if (result != 0)
            return (result);

        /* Reconstruct the channel coefficients. */
        reverseMatrixing(q->outSamples, &q->outSamples[1024], q->matrix_coeff_index_prev, q->matrix_coeff_index_now);

        channelWeighting(q->outSamples, &q->outSamples[1024], q->weighting_delay);

    } else {
        /* normal stereo mode or mono */
        /* Decode the channel sound units. */
        for (i=0 ; i<q->channels ; i++) {

            /* Set the bitstream reader at the start of a channel sound unit. */
            init_get_bits(&q->gb, databuf+((i*q->bytes_per_frame)/q->channels), (q->bits_per_frame)/q->channels);

            result = decodeChannelSoundUnit(q,&q->gb, &q->pUnits[i], &q->outSamples[i*1024], i, q->codingMode);
            if (result != 0)
                return (result);
        }
    }

    /* Apply the iQMF synthesis filter. */
    p1= q->outSamples;
    for (i=0 ; i<q->channels ; i++) {
        p2= p1+256;
        p3= p2+256;
        p4= p3+256;
        iqmf (p1, p2, 256, p1, q->pUnits[i].delayBuf1, q->tempBuf);
        iqmf (p4, p3, 256, p3, q->pUnits[i].delayBuf2, q->tempBuf);
        iqmf (p1, p3, 512, p1, q->pUnits[i].delayBuf3, q->tempBuf);
        p1 +=1024;
    }

    return 0;
}


/**
 * Atrac frame decoding
 *
 * @param rmctx     pointer to the AVCodecContext
 */

static int atrac3_decode_frame(RMContext *rmctx, ATRAC3Context *q,
            void *data, int *data_size,
            const uint8_t *buf, int buf_size) {
    int result = 0, i;
    const uint8_t* databuf;
    int16_t* samples = data;

    if (buf_size < rmctx->block_align)
        return buf_size;

    /* Check if we need to descramble and what buffer to pass on. */
    if (q->scrambled_stream) {
        decode_bytes(buf, q->decoded_bytes_buffer, rmctx->block_align);
        databuf = q->decoded_bytes_buffer;
    } else {
        databuf = buf;
    }

    result = decodeFrame(q, databuf);

    if (result != 0) {
        av_log(NULL,AV_LOG_ERROR,"Frame decoding error!\n");
        return -1;
    }

    if (q->channels == 1) {
        /* mono */
        for (i = 0; i<1024; i++)
            samples[i] = av_clip_int16(q->outSamples[i]);
        *data_size = 1024 * sizeof(int16_t);
    } else {
        /* stereo */
        for (i = 0; i < 1024; i++) {
            samples[i*2] = av_clip_int16(q->outSamples[i]);
            samples[i*2+1] = av_clip_int16(q->outSamples[1024+i]);
        }
        *data_size = 2048 * sizeof(int16_t);
    }

    return rmctx->block_align;
}


/**
 * Atrac3 initialization
 *
 * @param rmctx     pointer to the RMContext
 */

static av_cold int atrac3_decode_init(ATRAC3Context *q, RMContext *rmctx)
{
    int i;
    const uint8_t *edata_ptr = rmctx->codec_extradata;
    static VLC_TYPE atrac3_vlc_table[4096][2];
    static int vlcs_initialized = 0;

    /* Take data from the AVCodecContext (RM container). */
    q->sample_rate = rmctx->sample_rate;
    q->channels = rmctx->nb_channels;
    q->bit_rate = rmctx->bit_rate;
    q->bits_per_frame = rmctx->block_align * 8;
    q->bytes_per_frame = rmctx->block_align;

    /* Take care of the codec-specific extradata. */
    if (rmctx->extradata_size == 14) {
        /* Parse the extradata, WAV format */
        av_log(rmctx,AV_LOG_DEBUG,"[0-1] %d\n",bytestream_get_le16(&edata_ptr));  //Unknown value always 1
        q->samples_per_channel = bytestream_get_le32(&edata_ptr);
        q->codingMode = bytestream_get_le16(&edata_ptr);
        av_log(rmctx,AV_LOG_DEBUG,"[8-9] %d\n",bytestream_get_le16(&edata_ptr));  //Dupe of coding mode
        q->frame_factor = bytestream_get_le16(&edata_ptr);  //Unknown always 1
        av_log(rmctx,AV_LOG_DEBUG,"[12-13] %d\n",bytestream_get_le16(&edata_ptr));  //Unknown always 0

        /* setup */
        q->samples_per_frame = 1024 * q->channels;
        q->atrac3version = 4;
        q->delay = 0x88E;
        if (q->codingMode)
            q->codingMode = JOINT_STEREO;
        else
            q->codingMode = STEREO;
        q->scrambled_stream = 0;

        if ((q->bytes_per_frame == 96*q->channels*q->frame_factor) || (q->bytes_per_frame == 152*q->channels*q->frame_factor) || (q->bytes_per_frame == 192*q->channels*q->frame_factor)) {
        } else {
            av_log(rmctx,AV_LOG_ERROR,"Unknown frame/channel/frame_factor configuration %d/%d/%d\n", q->bytes_per_frame, q->channels, q->frame_factor);
            return -1;
        }

    } else if (rmctx->extradata_size == 10) {
        /* Parse the extradata, RM format. */
        q->atrac3version = bytestream_get_be32(&edata_ptr);
        q->samples_per_frame = bytestream_get_be16(&edata_ptr);
        q->delay = bytestream_get_be16(&edata_ptr);
        q->codingMode = bytestream_get_be16(&edata_ptr);

        q->samples_per_channel = q->samples_per_frame / q->channels;
        q->scrambled_stream = 1;

    } else {
        av_log(NULL,AV_LOG_ERROR,"Unknown extradata size %d.\n",rmctx->extradata_size);
    }
    /* Check the extradata. */

    if (q->atrac3version != 4) {
        av_log(rmctx,AV_LOG_ERROR,"Version %d != 4.\n",q->atrac3version);
        return -1;
    }

    if (q->samples_per_frame != 1024 && q->samples_per_frame != 2048) {
        av_log(rmctx,AV_LOG_ERROR,"Unknown amount of samples per frame %d.\n",q->samples_per_frame);
        return -1;
    }

    if (q->delay != 0x88E) {
        av_log(rmctx,AV_LOG_ERROR,"Unknown amount of delay %x != 0x88E.\n",q->delay);
        return -1;
    }

    if (q->codingMode == STEREO) {
        av_log(rmctx,AV_LOG_DEBUG,"Normal stereo detected.\n");
    } else if (q->codingMode == JOINT_STEREO) {
        av_log(rmctx,AV_LOG_DEBUG,"Joint stereo detected.\n");
    } else {
        av_log(rmctx,AV_LOG_ERROR,"Unknown channel coding mode %x!\n",q->codingMode);
        return -1;
    }

    if (rmctx->nb_channels <= 0 || rmctx->nb_channels > 2 /*|| ((rmctx->channels * 1024) != q->samples_per_frame)*/) {
        av_log(rmctx,AV_LOG_ERROR,"Channel configuration error!\n");
        return -1;
    }


    if(rmctx->block_align >= UINT16_MAX/2)
        return -1;


    /* Initialize the VLC tables. */
    if (!vlcs_initialized) {
        for (i=0 ; i<7 ; i++) {
            spectral_coeff_tab[i].table = &atrac3_vlc_table[atrac3_vlc_offs[i]];
            spectral_coeff_tab[i].table_allocated = atrac3_vlc_offs[i + 1] - atrac3_vlc_offs[i];
            init_vlc (&spectral_coeff_tab[i], 9, huff_tab_sizes[i],
                huff_bits[i], 1, 1,
                huff_codes[i], 1, 1, INIT_VLC_USE_NEW_STATIC);
        }

        vlcs_initialized = 1;

    }

    init_atrac3_transforms(q);

    /* init the joint-stereo decoding data */
    q->weighting_delay[0] = 0;
    q->weighting_delay[1] = 7;
    q->weighting_delay[2] = 0;
    q->weighting_delay[3] = 7;
    q->weighting_delay[4] = 0;
    q->weighting_delay[5] = 7;

    for (i=0; i<4; i++) {
        q->matrix_coeff_index_prev[i] = 3;
        q->matrix_coeff_index_now[i] = 3;
        q->matrix_coeff_index_next[i] = 3;
    }
   
    q->pUnits = channel_units;
    
    return 0;
}

/***************************************************************
 * Following is a test program to convert from atrac/rm to wav *
 ***************************************************************/
static unsigned char wav_header[44]={
    'R','I','F','F',//  0 - ChunkID
    0,0,0,0,        //  4 - ChunkSize (filesize-8)
    'W','A','V','E',//  8 - Format
    'f','m','t',' ',// 12 - SubChunkID
    16,0,0,0,       // 16 - SubChunk1ID  // 16 for PCM
    1,0,            // 20 - AudioFormat (1=Uncompressed)
    2,0,            // 22 - NumChannels
    0,0,0,0,        // 24 - SampleRate in Hz
    0,0,0,0,        // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
    4,0,            // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
    16,0,           // 34 - BitsPerSample
    'd','a','t','a',// 36 - Subchunk2ID
    0,0,0,0         // 40 - Subchunk2Size
};

int open_wav(char* filename) {
    int fd,res;

    fd=open(filename,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if (fd >= 0) {
        res = write(fd,wav_header,sizeof(wav_header));
    }

    return(fd);
}

void close_wav(int fd, RMContext *rmctx, ATRAC3Context *q) {
    int x,res;
    int filesize;
    int bytes_per_sample = 2;
    int samples_per_frame = q->samples_per_frame;
    int nb_channels = rmctx->nb_channels;
    int sample_rate = rmctx->sample_rate;
    int nb_frames = rmctx->audio_framesize/rmctx->block_align * rmctx->nb_packets - 2; // first 2 frames have no valid audio; skipped in output

    filesize= samples_per_frame*bytes_per_sample*nb_frames +44;
    printf("Filesize = %d\n",filesize);

    // ChunkSize
    x=filesize-8;
    wav_header[4]=(x&0xff);
    wav_header[5]=(x&0xff00)>>8;
    wav_header[6]=(x&0xff0000)>>16;
    wav_header[7]=(x&0xff000000)>>24;

    // Number of channels
    wav_header[22]=nb_channels;

    // Samplerate
    wav_header[24]=sample_rate&0xff;
    wav_header[25]=(sample_rate&0xff00)>>8;
    wav_header[26]=(sample_rate&0xff0000)>>16;
    wav_header[27]=(sample_rate&0xff000000)>>24;

    // ByteRate
    x=sample_rate*bytes_per_sample*nb_channels;
    wav_header[28]=(x&0xff);
    wav_header[29]=(x&0xff00)>>8;
    wav_header[30]=(x&0xff0000)>>16;
    wav_header[31]=(x&0xff000000)>>24;

    // BlockAlign
    wav_header[32]=rmctx->block_align;//2*rmctx->nb_channels;

    // Bits per sample
    wav_header[34]=16;
    
    // Subchunk2Size
    x=filesize-44;
    wav_header[40]=(x&0xff);
    wav_header[41]=(x&0xff00)>>8;
    wav_header[42]=(x&0xff0000)>>16;
    wav_header[43]=(x&0xff000000)>>24;

    lseek(fd,0,SEEK_SET);
    res = write(fd,wav_header,sizeof(wav_header));
    close(fd);
}

int main(int argc, char *argv[])
{
    int fd, fd_dec;
    int res, i, datasize = 0;

#ifdef DUMP_RAW_FRAMES 
    char filename[15];
    int fd_out;
#endif
    int16_t outbuf[2048];
    uint16_t fs,sps,h;
    uint32_t packet_count;
    ATRAC3Context q;
    RMContext rmctx;
    RMPacket pkt;

    memset(&q,0,sizeof(ATRAC3Context));
    memset(&rmctx,0,sizeof(RMContext));
    memset(&pkt,0,sizeof(RMPacket));

    if (argc != 2) {
        DEBUGF("Incorrect number of arguments\n");
        return -1;
    }

    fd = open(argv[1],O_RDONLY);
    if (fd < 0) {
        DEBUGF("Error opening file %s\n", argv[1]);
        return -1;
    }
    
    /* copy the input rm file to a memory buffer */
    uint8_t * filebuf = (uint8_t *)calloc((int)filesize(fd),sizeof(uint8_t));
    res = read(fd,filebuf,filesize(fd)); 
 
    fd_dec = open_wav("output.wav");
    if (fd_dec < 0) {
        DEBUGF("Error creating output file\n");
        return -1;
    }
    res = real_parse_header(fd, &rmctx);
    packet_count = rmctx.nb_packets;
    rmctx.audio_framesize = rmctx.block_align;
    rmctx.block_align = rmctx.sub_packet_size;
    fs = rmctx.audio_framesize;
    sps= rmctx.block_align;
    h = rmctx.sub_packet_h;
    atrac3_decode_init(&q,&rmctx);
    
    /* change the buffer pointer to point at the first audio frame */
    advance_buffer(&filebuf, rmctx.data_offset + DATA_HEADER_SIZE);
    while(packet_count)
    {  
        rm_get_packet(&filebuf, &rmctx, &pkt);
        for(i = 0; i < rmctx.audio_pkt_cnt*(fs/sps) ; i++)
        { 
            /* output raw audio frames that are sent to the decoder into separate files */
#ifdef DUMP_RAW_FRAMES 
              snprintf(filename,sizeof(filename),"dump%d.raw",++x);
              fd_out = open(filename,O_WRONLY|O_CREAT|O_APPEND);           
              write(fd_out,pkt.frames[i],sps);  
              close(fd_out);
#endif
            if(pkt.length > 0)
                res = atrac3_decode_frame(&rmctx,&q, outbuf, &datasize, pkt.frames[i] , rmctx.block_align);
            rmctx.frame_number++;
            res = write(fd_dec,outbuf,datasize);                  
        }
        packet_count -= rmctx.audio_pkt_cnt;
        rmctx.audio_pkt_cnt = 0;
    }
    close_wav(fd_dec, &rmctx, &q);
    close(fd);

  return 0;
}
