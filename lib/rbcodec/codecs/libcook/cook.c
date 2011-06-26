/*
 * COOK compatible decoder
 * Copyright (c) 2003 Sascha Sommer
 * Copyright (c) 2005 Benjamin Larsson
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
 * @file cook.c
 * Cook compatible decoder. Bastardization of the G.722.1 standard.
 * This decoder handles RealNetworks, RealAudio G2 data.
 * Cook is identified by the codec name cook in RM files.
 *
 * To use this decoder, a calling application must supply the extradata
 * bytes provided from the RM container; 8+ bytes for mono streams and
 * 16+ for stereo streams (maybe more).
 *
 * Codec technicalities (all this assume a buffer length of 1024):
 * Cook works with several different techniques to achieve its compression.
 * In the timedomain the buffer is divided into 8 pieces and quantized. If
 * two neighboring pieces have different quantization index a smooth
 * quantization curve is used to get a smooth overlap between the different
 * pieces.
 * To get to the transformdomain Cook uses a modulated lapped transform.
 * The transform domain has 50 subbands with 20 elements each. This
 * means only a maximum of 50*20=1000 coefficients are used out of the 1024
 * available.
 */

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "codeclib.h"

#include "cook.h"
#include "cookdata.h"

/* the different Cook versions */
#define MONO            0x1000001
#define STEREO          0x1000002
#define JOINT_STEREO    0x1000003
#define MC_COOK         0x2000000   //multichannel Cook, not supported

#define SUBBAND_SIZE    20
#define MAX_SUBPACKETS   5
//#define COOKDEBUG
#ifndef COOKDEBUG
#undef DEBUGF
#define DEBUGF(...)
#endif 

/**
 * Random bit stream generator.
 */
static inline int cook_random(COOKContext *q)
{
    q->random_state =
      q->random_state * 214013 + 2531011; /* typical RNG numbers */

    return (q->random_state/0x1000000)&1;  /*>>31*/
}
#include "cook_fixpoint.h"

/* debug functions */

#ifdef COOKDEBUG
static void dump_int_table(int* table, int size, int delimiter) {
    int i=0;
    DEBUGF("\n[%d]: ",i);
    for (i=0 ; i<size ; i++) {
        DEBUGF("%d, ", table[i]);
        if ((i+1)%delimiter == 0) DEBUGF("\n[%d]: ",i+1);
    }
}

static void dump_short_table(short* table, int size, int delimiter) {
    int i=0;
    DEBUGF("\n[%d]: ",i);
    for (i=0 ; i<size ; i++) {
        DEBUGF("%d, ", table[i]);
        if ((i+1)%delimiter == 0) DEBUGF("\n[%d]: ",i+1);
    }
}

#endif

/*************** init functions ***************/
/* Codebook sizes (11586 * 4 bytes in total) */
/* Used for envelope_quant_index[]. */
static VLC_TYPE vlcbuf00[ 520][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf01[ 640][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf02[ 544][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf03[ 528][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf04[ 544][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf05[ 544][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf06[ 640][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf07[ 576][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf08[ 528][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf09[ 544][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf10[ 544][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf11[ 640][2] IBSS_ATTR_COOK_VLCBUF;
static VLC_TYPE vlcbuf12[ 544][2] IBSS_ATTR_COOK_LARGE_IRAM;
/* Used for sqvh[]. */
static VLC_TYPE vlcbuf13[ 622][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf14[ 308][2] IBSS_ATTR_COOK_LARGE_IRAM; 
static VLC_TYPE vlcbuf15[ 280][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf16[1456][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf17[ 694][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf18[ 698][2] IBSS_ATTR_COOK_LARGE_IRAM;
static VLC_TYPE vlcbuf19[ 104][2] IBSS_ATTR_COOK_LARGE_IRAM;
/* Used for ccpl. */
static VLC_TYPE vlcbuf20[  88][2] IBSS_ATTR_COOK_VLCBUF;

/* Code book sizes (11586 entries in total) */
static int env_size[13] = {520,640,544, 528,544,544,640,576,528,544,544,640,544};
static int sqvh_size[7] = {622,308,280,1456,694,698,104};
static int ccpl_size    = 88;


static int init_cook_vlc_tables(COOKContext *q) {
    int i, result = 0;
    
    /* Set pointers for codebooks. */
    q->envelope_quant_index[ 0].table = vlcbuf00;
    q->envelope_quant_index[ 1].table = vlcbuf01;
    q->envelope_quant_index[ 2].table = vlcbuf02;
    q->envelope_quant_index[ 3].table = vlcbuf03;
    q->envelope_quant_index[ 4].table = vlcbuf04;
    q->envelope_quant_index[ 5].table = vlcbuf05;
    q->envelope_quant_index[ 6].table = vlcbuf06;
    q->envelope_quant_index[ 7].table = vlcbuf07;
    q->envelope_quant_index[ 8].table = vlcbuf08;
    q->envelope_quant_index[ 9].table = vlcbuf09;
    q->envelope_quant_index[10].table = vlcbuf10;
    q->envelope_quant_index[11].table = vlcbuf11;
    q->envelope_quant_index[12].table = vlcbuf12;
    q->sqvh[0].table = vlcbuf13; 
    q->sqvh[1].table = vlcbuf14;
    q->sqvh[2].table = vlcbuf15; 
    q->sqvh[3].table = vlcbuf16;
    q->sqvh[4].table = vlcbuf17; 
    q->sqvh[5].table = vlcbuf18;
    q->sqvh[6].table = vlcbuf19;
    q->ccpl.table = vlcbuf20;

    /* Init envelope VLC (13 books) */
    for (i=0 ; i<13 ; i++) {
        q->envelope_quant_index[i].table_allocated = env_size[i];
        result |= init_vlc (&q->envelope_quant_index[i], 9, 24,
            envelope_quant_index_huffbits[i], 1, 1,
            envelope_quant_index_huffcodes[i], 2, 2, INIT_VLC_USE_NEW_STATIC);
    }

    /* Init subband VLC (7 books) */
    for (i=0 ; i<7 ; i++) {
        q->sqvh[i].table_allocated = sqvh_size[i];
        result |= init_vlc (&q->sqvh[i], vhvlcsize_tab[i], vhsize_tab[i],
            cvh_huffbits[i], 1, 1,
            cvh_huffcodes[i], 2, 2, INIT_VLC_USE_NEW_STATIC);
    }

    /* Init Joint-Stereo VLC (1 book) */
    if (q->nb_channels==2 && q->joint_stereo==1){
        q->ccpl.table_allocated = ccpl_size;
        result |= init_vlc (&q->ccpl, 6, (1<<q->js_vlc_bits)-1,
            ccpl_huffbits[q->js_vlc_bits-2], 1, 1,
            ccpl_huffcodes[q->js_vlc_bits-2], 2, 2, INIT_VLC_USE_NEW_STATIC);
        DEBUGF("Joint-stereo VLC used.\n");
    }

    DEBUGF("VLC tables initialized. Result = %d\n",result);
    return result;
}
/*************** init functions end ***********/

/**
 * Cook indata decoding, every 32 bits are XORed with 0x37c511f2.
 * Why? No idea, some checksum/error detection method maybe.
 *
 * Out buffer size: extra bytes are needed to cope with
 * padding/misalignment.
 * Subpackets passed to the decoder can contain two, consecutive
 * half-subpackets, of identical but arbitrary size.
 *          1234 1234 1234 1234  extraA extraB
 * Case 1:  AAAA BBBB              0      0
 * Case 2:  AAAA ABBB BB--         3      3
 * Case 3:  AAAA AABB BBBB         2      2
 * Case 4:  AAAA AAAB BBBB BB--    1      5
 *
 * Nice way to waste CPU cycles.
 *
 * @param inbuffer  pointer to byte array of indata
 * @param out       pointer to byte array of outdata
 * @param bytes     number of bytes
 */
#define DECODE_BYTES_PAD1(bytes) (3 - ((bytes)+3) % 4)
#define DECODE_BYTES_PAD2(bytes) ((bytes) % 4 + DECODE_BYTES_PAD1(2 * (bytes)))

static inline int decode_bytes(const uint8_t* inbuffer, uint8_t* out, int bytes){
    int i, off;
    uint32_t c;
    const uint32_t* buf;
    uint32_t* obuf = (uint32_t*) out;
    /* FIXME: 64 bit platforms would be able to do 64 bits at a time.
     * I'm too lazy though, should be something like
     * for(i=0 ; i<bitamount/64 ; i++)
     *     (int64_t)out[i] = 0x37c511f237c511f2^be2me_64(int64_t)in[i]);
     * Buffer alignment needs to be checked. */

    off = (intptr_t)inbuffer & 3;
    buf = (const uint32_t*) (inbuffer - off);
    c = be2me_32((0x37c511f2 >> (off*8)) | (0x37c511f2 << (32-(off*8))));
    bytes += 3 + off;
    for (i = 0; i < bytes/4; i++)
        obuf[i] = c ^ buf[i];

    return off;
}

/**
 * Fill the gain array for the timedomain quantization.
 *
 * @param q                 pointer to the COOKContext
 * @param gaininfo[9]       array of gain indexes
 */

static void decode_gain_info(GetBitContext *gb, int *gaininfo)
{
    int i, n;

    while (get_bits1(gb)) {}
    n = get_bits_count(gb) - 1;     //amount of elements*2 to update

    i = 0;
    while (n--) {
        int index = get_bits(gb, 3);
        int gain = get_bits1(gb) ? (int)get_bits(gb, 4) - 7 : -1;

        while (i <= index) gaininfo[i++] = gain;
    }
    while (i <= 8) gaininfo[i++] = 0;
}

/**
 * Create the quant index table needed for the envelope.
 *
 * @param q                 pointer to the COOKContext
 * @param quant_index_table pointer to the array
 */

static void decode_envelope(COOKContext *q, int* quant_index_table) {
    int i,j, vlc_index;

    quant_index_table[0]= get_bits(&q->gb,6) - 6;       //This is used later in categorize

    for (i=1 ; i < q->total_subbands ; i++){
        vlc_index=i;
        if (i >= q->js_subband_start * 2) {
            vlc_index-=q->js_subband_start;
        } else {
            vlc_index/=2;
            if(vlc_index < 1) vlc_index = 1;
        }
        if (vlc_index>13) vlc_index = 13;           //the VLC tables >13 are identical to No. 13

        j = get_vlc2(&q->gb, q->envelope_quant_index[vlc_index-1].table,
                     q->envelope_quant_index[vlc_index-1].bits,2);
        quant_index_table[i] = quant_index_table[i-1] + j - 12;    //differential encoding
    }
}

/**
 * Calculate the category and category_index vector.
 *
 * @param q                     pointer to the COOKContext
 * @param quant_index_table     pointer to the array
 * @param category              pointer to the category array
 * @param category_index        pointer to the category_index array
 */

static void categorize(COOKContext *q, int* quant_index_table,
                       int* category, int* category_index){
    int exp_idx, bias, tmpbias1, tmpbias2, bits_left, num_bits, index, v, i, j;
    int exp_index2[102];
    int exp_index1[102];

    int tmp_categorize_array[128*2];
    int tmp_categorize_array1_idx=q->numvector_size;
    int tmp_categorize_array2_idx=q->numvector_size;

    bits_left =  q->bits_per_subpacket - get_bits_count(&q->gb);

    if(bits_left > q->samples_per_channel) {
        bits_left = q->samples_per_channel +
                    ((bits_left - q->samples_per_channel)*5)/8;
        //av_log(q->avctx, AV_LOG_ERROR, "bits_left = %d\n",bits_left);
    }

    memset(&exp_index1,0,102*sizeof(int));
    memset(&exp_index2,0,102*sizeof(int));
    memset(&tmp_categorize_array,0,128*2*sizeof(int));

    bias=-32;

    /* Estimate bias. */
    for (i=32 ; i>0 ; i=i/2){
        num_bits = 0;
        index = 0;
        for (j=q->total_subbands ; j>0 ; j--){
            exp_idx = av_clip((i - quant_index_table[index] + bias) / 2, 0, 7);
            index++;
            num_bits+=expbits_tab[exp_idx];
        }
        if(num_bits >= bits_left - 32){
            bias+=i;
        }
    }

    /* Calculate total number of bits. */
    num_bits=0;
    for (i=0 ; i<q->total_subbands ; i++) {
        exp_idx = av_clip((bias - quant_index_table[i]) / 2, 0, 7);
        num_bits += expbits_tab[exp_idx];
        exp_index1[i] = exp_idx;
        exp_index2[i] = exp_idx;
    }
    tmpbias1 = tmpbias2 = num_bits;

    for (j = 1 ; j < q->numvector_size ; j++) {
        if (tmpbias1 + tmpbias2 > 2*bits_left) {  /* ---> */
            int max = -999999;
            index=-1;
            for (i=0 ; i<q->total_subbands ; i++){
                if (exp_index1[i] < 7) {
                    v = (-2*exp_index1[i]) - quant_index_table[i] + bias;
                    if ( v >= max) {
                        max = v;
                        index = i;
                    }
                }
            }
            if(index==-1)break;
            tmp_categorize_array[tmp_categorize_array1_idx++] = index;
            tmpbias1 -= expbits_tab[exp_index1[index]] -
                        expbits_tab[exp_index1[index]+1];
            ++exp_index1[index];
        } else {  /* <--- */
            int min = 999999;
            index=-1;
            for (i=0 ; i<q->total_subbands ; i++){
                if(exp_index2[i] > 0){
                    v = (-2*exp_index2[i])-quant_index_table[i]+bias;
                    if ( v < min) {
                        min = v;
                        index = i;
                    }
                }
            }
            if(index == -1)break;
            tmp_categorize_array[--tmp_categorize_array2_idx] = index;
            tmpbias2 -= expbits_tab[exp_index2[index]] -
                        expbits_tab[exp_index2[index]-1];
            --exp_index2[index];
        }
    }
    memcpy(category, exp_index2, sizeof(int) * q->total_subbands );
    memcpy(category_index, tmp_categorize_array+tmp_categorize_array2_idx, sizeof(int) * (q->numvector_size-1) );
}


/**
 * Expand the category vector.
 *
 * @param q                     pointer to the COOKContext
 * @param category              pointer to the category array
 * @param category_index        pointer to the category_index array
 */

static inline void expand_category(COOKContext *q, int* category,
                                   int* category_index){
    int i;
    for(i=0 ; i<q->num_vectors ; i++){
        ++category[category_index[i]];
    }
}

/**
 * Unpack the subband_coef_index and subband_coef_sign vectors.
 *
 * @param q                     pointer to the COOKContext
 * @param category              pointer to the category array
 * @param subband_coef_index    array of indexes to quant_centroid_tab
 * @param subband_coef_sign     signs of coefficients
 */

static int unpack_SQVH(COOKContext *q, int category, int* subband_coef_index,
                       int* subband_coef_sign) {
    int i,j;
    int vlc, vd ,tmp, result;

    vd = vd_tab[category];
    result = 0;
    for(i=0 ; i<vpr_tab[category] ; i++)
    {
        vlc = get_vlc2(&q->gb, q->sqvh[category].table, q->sqvh[category].bits, 3);
        if (q->bits_per_subpacket < get_bits_count(&q->gb))
        {
            vlc = 0;
            result = 1;
            memset(subband_coef_index, 0, sizeof(int)*vd);
            memset(subband_coef_sign, 0, sizeof(int)*vd);
            subband_coef_index+=vd;
            subband_coef_sign+=vd;
        }
        else
        {
            for(j=vd-1 ; j>=0 ; j--){
                tmp = (vlc * invradix_tab[category])/0x100000;
                subband_coef_index[j] = vlc - tmp * (kmax_tab[category]+1);
                vlc = tmp;
            }

            for(j=0 ; j<vd ; j++)
            {
                if (*subband_coef_index++) {
                    if(get_bits_count(&q->gb) < q->bits_per_subpacket) {
                        *subband_coef_sign++ = get_bits1(&q->gb);
                    } else {
                        result=1;
                        *subband_coef_sign++=0;
                    }
                } else {
                    *subband_coef_sign++=0;
                }
            }
        }
    }
    return result;
}


/**
 * Fill the mlt_buffer with mlt coefficients.
 *
 * @param q                 pointer to the COOKContext
 * @param category          pointer to the category array
 * @param quant_index_table pointer to the array
 * @param mlt_buffer        pointer to mlt coefficients
 */

static void decode_vectors(COOKContext* q, int* category,
                           int *quant_index_table, REAL_T* mlt_buffer)
                           ICODE_ATTR_COOK_DECODE;
static void decode_vectors(COOKContext* q, int* category,
                           int *quant_index_table, REAL_T* mlt_buffer){
    /* A zero in this table means that the subband coefficient is
       random noise coded. */
    int subband_coef_index[SUBBAND_SIZE];
    /* A zero in this table means that the subband coefficient is a
       positive multiplicator. */
    int subband_coef_sign[SUBBAND_SIZE];
    int band, j;
    int index=0;

    for(band=0 ; band<q->total_subbands ; band++){
        index = category[band];
        if(category[band] < 7){
            if(unpack_SQVH(q, category[band], subband_coef_index, subband_coef_sign)){
                index=7;
                for(j=0 ; j<q->total_subbands ; j++) category[band+j]=7;
            }
        }
        if(index>=7) {
            memset(subband_coef_index, 0, sizeof(subband_coef_index));
            memset(subband_coef_sign, 0, sizeof(subband_coef_sign));
        }
        scalar_dequant_math(q, index, quant_index_table[band],
                            subband_coef_index, subband_coef_sign,
                            &mlt_buffer[band * SUBBAND_SIZE]);
    }

    if(q->total_subbands*SUBBAND_SIZE >= q->samples_per_channel){
        return;
    } /* FIXME: should this be removed, or moved into loop above? */
}


/**
 * function for decoding mono data
 *
 * @param q                 pointer to the COOKContext
 * @param mlt_buffer        pointer to mlt coefficients
 */

static void mono_decode(COOKContext *q, REAL_T* mlt_buffer) ICODE_ATTR_COOK_DECODE;
static void mono_decode(COOKContext *q, REAL_T* mlt_buffer) {

    int category_index[128];
    int quant_index_table[102];
    int category[128];

    memset(&category, 0, 128*sizeof(int));
    memset(&category_index, 0, 128*sizeof(int));

    decode_envelope(q, quant_index_table);
    q->num_vectors = get_bits(&q->gb,q->log2_numvector_size);
    categorize(q, quant_index_table, category, category_index);
    expand_category(q, category, category_index);
    decode_vectors(q, category, quant_index_table, mlt_buffer);
}

/**
 * function for getting the jointstereo coupling information
 *
 * @param q                 pointer to the COOKContext
 * @param decouple_tab      decoupling array
 *
 */

static void decouple_info(COOKContext *q, int* decouple_tab){
    int length, i;

    if(get_bits1(&q->gb)) {
        if(cplband[q->js_subband_start] > cplband[q->subbands-1]) return;

        length = cplband[q->subbands-1] - cplband[q->js_subband_start] + 1;
        for (i=0 ; i<length ; i++) {
            decouple_tab[cplband[q->js_subband_start] + i] = get_vlc2(&q->gb, q->ccpl.table, q->ccpl.bits, 2);
        }
        return;
    }

    if(cplband[q->js_subband_start] > cplband[q->subbands-1]) return;

    length = cplband[q->subbands-1] - cplband[q->js_subband_start] + 1;
    for (i=0 ; i<length ; i++) {
       decouple_tab[cplband[q->js_subband_start] + i] = get_bits(&q->gb, q->js_vlc_bits);
    }
    return;
}

/**
 * function for decoding joint stereo data
 *
 * @param q                 pointer to the COOKContext
 * @param mlt_buffer1       pointer to left channel mlt coefficients
 * @param mlt_buffer2       pointer to right channel mlt coefficients
 */

static void joint_decode(COOKContext *q, REAL_T* mlt_buffer1,
                         REAL_T* mlt_buffer2) {
    int i;
    int decouple_tab[SUBBAND_SIZE];
    REAL_T *decode_buffer = q->decode_buffer_0;
    int idx;

    memset(decouple_tab, 0, sizeof(decouple_tab));
    memset(decode_buffer, 0, sizeof(q->decode_buffer_0));

    /* Make sure the buffers are zeroed out. */
    memset(mlt_buffer1,0, 1024*sizeof(REAL_T));
    memset(mlt_buffer2,0, 1024*sizeof(REAL_T));
    decouple_info(q, decouple_tab);
    mono_decode(q, decode_buffer);

    /* The two channels are stored interleaved in decode_buffer. */
    REAL_T * mlt_buffer1_end = mlt_buffer1 + (q->js_subband_start*SUBBAND_SIZE);
    while(mlt_buffer1 < mlt_buffer1_end)
    {
        memcpy(mlt_buffer1,decode_buffer,sizeof(REAL_T)*SUBBAND_SIZE);
        memcpy(mlt_buffer2,decode_buffer+20,sizeof(REAL_T)*SUBBAND_SIZE);
        mlt_buffer1 += 20;
        mlt_buffer2 += 20;
        decode_buffer += 40;
    }

    /* When we reach js_subband_start (the higher frequencies)
       the coefficients are stored in a coupling scheme. */
    idx = (1 << q->js_vlc_bits) - 1;
    for (i=q->js_subband_start ; i<q->subbands ; i++) {
        int i1 = decouple_tab[cplband[i]];
        int i2 = idx - i1 - 1;
        mlt_buffer1_end = mlt_buffer1 + SUBBAND_SIZE;
        while(mlt_buffer1 < mlt_buffer1_end)
        {
            *mlt_buffer1++ = cplscale_math(*decode_buffer, q->js_vlc_bits, i1);
            *mlt_buffer2++ = cplscale_math(*decode_buffer++, q->js_vlc_bits, i2);
        }
        mlt_buffer1 += (20-SUBBAND_SIZE);
        mlt_buffer2 += (20-SUBBAND_SIZE);
        decode_buffer += (20-SUBBAND_SIZE);
    }
}

/**
 * First part of subpacket decoding:
 *  decode raw stream bytes and read gain info.
 *
 * @param q                 pointer to the COOKContext
 * @param inbuffer          pointer to raw stream data
 * @param gain_ptr          array of current/prev gain pointers
 */

#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)

static inline void
decode_bytes_and_gain(COOKContext *q, const uint8_t *inbuffer,
                      cook_gains *gains_ptr)
{
    int offset;

    offset = decode_bytes(inbuffer, q->decoded_bytes_buffer,
                          q->bits_per_subpacket/8);
    init_get_bits(&q->gb, q->decoded_bytes_buffer + offset,
                  q->bits_per_subpacket);
    decode_gain_info(&q->gb, gains_ptr->now);

    /* Swap current and previous gains */
    FFSWAP(int *, gains_ptr->now, gains_ptr->previous);
}

/**
 * Final part of subpacket decoding:
 *  Apply modulated lapped transform, gain compensation,
 *  clip and convert to integer.
 *
 * @param q                 pointer to the COOKContext
 * @param decode_buffer     pointer to the mlt coefficients
 * @param gain_ptr          array of current/prev gain pointers
 * @param previous_buffer   pointer to the previous buffer to be used for overlapping
 * @param out               pointer to the output buffer
 * @param chan              0: left or single channel, 1: right channel
 */

static void
mlt_compensate_output(COOKContext *q, REAL_T *decode_buffer,
                      cook_gains *gains, REAL_T *previous_buffer,
                      int32_t *out, int chan)
{
    REAL_T *buffer = q->mono_mdct_output;
    int i;
    imlt_math(q, decode_buffer);

    /* Overlap with the previous block. */
    overlap_math(q, gains->previous[0], previous_buffer);

    /* Apply gain profile */
    for (i = 0; i < 8; i++) {
        if (gains->now[i] || gains->now[i + 1])
            interpolate_math(q, &buffer[q->samples_per_channel/8 * i],
                             gains->now[i], gains->now[i + 1]);
    }

    /* Save away the current to be previous block. */
    memcpy(previous_buffer, buffer+q->samples_per_channel,
           sizeof(REAL_T)*q->samples_per_channel);

    /* Copy output to non-interleaved sample buffer */
    memcpy(out + (chan * q->samples_per_channel), buffer,
           sizeof(REAL_T)*q->samples_per_channel);
}


/**
 * Cook subpacket decoding. This function returns one decoded subpacket,
 * usually 1024 samples per channel.
 *
 * @param q                 pointer to the COOKContext
 * @param inbuffer          pointer to the inbuffer
 * @param sub_packet_size   subpacket size
 * @param outbuffer         pointer to the outbuffer
 */


static int decode_subpacket(COOKContext *q, const uint8_t *inbuffer,
                            int sub_packet_size, int32_t *outbuffer) {
    /* packet dump */
//    for (i=0 ; i<sub_packet_size ; i++) {
//        DEBUGF("%02x", inbuffer[i]);
//    }
//    DEBUGF("\n");

    decode_bytes_and_gain(q, inbuffer, &q->gains1);

    if (q->joint_stereo) {
        joint_decode(q, q->decode_buffer_1, q->decode_buffer_2);
    } else {
        mono_decode(q, q->decode_buffer_1);

        if (q->nb_channels == 2) {
            decode_bytes_and_gain(q, inbuffer + sub_packet_size/2, &q->gains2);
            mono_decode(q, q->decode_buffer_2);
        }
    }

    mlt_compensate_output(q, q->decode_buffer_1, &q->gains1,
                          q->mono_previous_buffer1, outbuffer, 0);

    if (q->nb_channels == 2) {
        if (q->joint_stereo) {
            mlt_compensate_output(q, q->decode_buffer_2, &q->gains1,
                                  q->mono_previous_buffer2, outbuffer, 1);
        } else {
            mlt_compensate_output(q, q->decode_buffer_2, &q->gains2,
                                  q->mono_previous_buffer2, outbuffer, 1);
        }
    }
    return q->samples_per_frame * sizeof(int32_t);
}


/**
 * Cook frame decoding
 *
 * @param rmctx     pointer to the RMContext
 */

int cook_decode_frame(RMContext *rmctx,COOKContext *q,
            int32_t *outbuffer, int *data_size,
            const uint8_t *inbuffer, int buf_size) {
    //COOKContext *q = avctx->priv_data;
    //COOKContext *q;

    if (buf_size < rmctx->block_align)
        return buf_size;

    *data_size = decode_subpacket(q, inbuffer, rmctx->block_align, outbuffer);

    /* Discard the first two frames: no valid audio. */
    if (rmctx->frame_number < 2) *data_size = 0;

    return rmctx->block_align;
}

#ifdef COOKDEBUG
static void dump_cook_context(COOKContext *q)
{
    //int i=0;
#define PRINT(a,b) DEBUGF(" %s = %d\n", a, b);
    DEBUGF("COOKextradata\n");
    DEBUGF("cookversion=%x\n",q->cookversion);
    if (q->cookversion > STEREO) {
        PRINT("js_subband_start",q->js_subband_start);
        PRINT("js_vlc_bits",q->js_vlc_bits);
    }
    PRINT("nb_channels",q->nb_channels);
    PRINT("bit_rate",q->bit_rate);
    PRINT("sample_rate",q->sample_rate);
    PRINT("samples_per_channel",q->samples_per_channel);
    PRINT("samples_per_frame",q->samples_per_frame);
    PRINT("subbands",q->subbands);
    PRINT("random_state",q->random_state);
    PRINT("js_subband_start",q->js_subband_start);
    PRINT("log2_numvector_size",q->log2_numvector_size);
    PRINT("numvector_size",q->numvector_size);
    PRINT("total_subbands",q->total_subbands);
}
#endif

/**
 * Cook initialization
 */

int cook_decode_init(RMContext *rmctx, COOKContext *q)
{
#if defined(CPU_COLDFIRE)
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
#endif
    /* cook extradata */
    q->cookversion = rm_get_uint32be(rmctx->codec_extradata);
    q->samples_per_frame =  rm_get_uint16be(&rmctx->codec_extradata[4]);
    q->subbands = rm_get_uint16be(&rmctx->codec_extradata[6]);
    q->extradata_size = rmctx->extradata_size;
    if (q->extradata_size >= 16){
        q->js_subband_start = rm_get_uint16be(&rmctx->codec_extradata[12]);
        q->js_vlc_bits = rm_get_uint16be(&rmctx->codec_extradata[14]);
    }

    /* Take data from the RMContext (RM container). */
    q->sample_rate = rmctx->sample_rate;
    q->nb_channels = rmctx->nb_channels;
    q->bit_rate = rmctx->bit_rate;  

    /* Initialize RNG. */
    q->random_state = 0;

    /* Initialize extradata related variables. */
    q->samples_per_channel = q->samples_per_frame >> (q->nb_channels-1);
    q->bits_per_subpacket = rmctx->block_align * 8;

    /* Initialize default data states. */
    q->log2_numvector_size = 5;
    q->total_subbands = q->subbands;

    /* Initialize version-dependent variables */
    DEBUGF("q->cookversion=%x\n",q->cookversion);
    q->joint_stereo = 0;
    switch (q->cookversion) {
        case MONO:
            if (q->nb_channels != 1) {
                DEBUGF("Container channels != 1, report sample!\n");
                return -1;
            }
            DEBUGF("MONO\n");
            break;
        case STEREO:
            if (q->nb_channels != 1) {
                q->bits_per_subpacket = q->bits_per_subpacket/2;
            }
            DEBUGF("STEREO\n");
            break;
        case JOINT_STEREO:
            if (q->nb_channels != 2) {
                DEBUGF("Container channels != 2, report sample!\n");
                return -1;
            }
            DEBUGF("JOINT_STEREO\n");
            if (q->extradata_size >= 16){
                q->total_subbands = q->subbands + q->js_subband_start;
                q->joint_stereo = 1;
            }
            if (q->samples_per_channel > 256) {
                q->log2_numvector_size  = 6;
            }
            if (q->samples_per_channel > 512) {
                q->log2_numvector_size  = 7;
            }
            break;
        case MC_COOK:
            DEBUGF("MC_COOK not supported!\n");
            return -1;
            break;
        default:
            DEBUGF("Unknown Cook version, report sample!\n");
            return -1;
            break;
    }

    /* Initialize variable relations */
    q->numvector_size = (1 << q->log2_numvector_size);
    q->mdct_nbits = av_log2(q->samples_per_channel)+1;

    /* Generate tables */
    if (init_cook_vlc_tables(q) != 0)
        return -1;


    if(rmctx->block_align >= UINT16_MAX/2)
        return -1;

    q->gains1.now      = q->gain_1;
    q->gains1.previous = q->gain_2;
    q->gains2.now      = q->gain_3;
    q->gains2.previous = q->gain_4;


    /* Initialize COOK signal arithmetic handling */
    /*
    if (1) {
        q->scalar_dequant  = scalar_dequant_math;
        q->interpolate     = interpolate_math;
    }
    */

    /* Try to catch some obviously faulty streams, othervise it might be exploitable */
    if (q->total_subbands > 53) {
        DEBUGF("total_subbands > 53, report sample!\n");
        return -1;
    }
    if (q->subbands > 50) {
        DEBUGF("subbands > 50, report sample!\n");
        return -1;
    }
    if ((q->samples_per_channel == 256) || (q->samples_per_channel == 512) || (q->samples_per_channel == 1024)) {
    } else {
        DEBUGF("unknown amount of samples_per_channel = %d, report sample!\n",q->samples_per_channel);
        return -1;
    }
    if ((q->js_vlc_bits > 6) || (q->js_vlc_bits < 0)) {
        DEBUGF("q->js_vlc_bits = %d, only >= 0 and <= 6 allowed!\n",q->js_vlc_bits);
        return -1;
    }


#ifdef COOKDEBUG
    dump_cook_context(q);
#endif
    return 0;
}

