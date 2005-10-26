/*
 * FLAC (Free Lossless Audio Codec) decoder
 * Copyright (c) 2003 Alex Beregszaszi
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
 * @file flac.c
 * FLAC (Free Lossless Audio Codec) decoder
 * @author Alex Beregszaszi
 *
 * For more information on the FLAC format, visit:
 *  http://flac.sourceforge.net/
 *
 * This decoder can be used in 1 of 2 ways: Either raw FLAC data can be fed
 * through, starting from the initial 'fLaC' signature; or by passing the
 * 34-byte streaminfo structure through avctx->extradata[_size] followed
 * by data starting with the 0xFFF8 marker.
 */

#include <inttypes.h>
#include <stdbool.h>
#ifndef BUILD_STANDALONE
#include "../codec.h"
#endif
 
#include "bitstream.h"
#include "golomb.h"

#include "decoder.h"

#define INT_MAX       2147483647

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

/* Needed by golomb.h */
const uint8_t ff_log2_tab[256] ICONST_ATTR = {
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static const int sample_rate_table[] ICONST_ATTR =
{ 0, 0, 0, 0,
  8000, 16000, 22050, 24000, 32000, 44100, 48000, 96000,
  0, 0, 0, 0 };

static const int sample_size_table[] ICONST_ATTR = 
{ 0, 8, 12, 0, 16, 20, 24, 0 };

static const int blocksize_table[] ICONST_ATTR = {
     0,    192, 576<<0, 576<<1, 576<<2, 576<<3,      0,      0, 
256<<0, 256<<1, 256<<2, 256<<3, 256<<4, 256<<5, 256<<6, 256<<7 
};

static const uint8_t table_crc8[256] ICONST_ATTR = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
    0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
    0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
    0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
    0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
    0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
    0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
    0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
    0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
    0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
    0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
    0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
    0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
    0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
    0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
    0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
    0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

static int64_t get_utf8(GetBitContext *gb)
{
    uint64_t val;
    int ones=0, bytes;
    
    while(get_bits1(gb))
        ones++;

    if     (ones==0) bytes=0;
    else if(ones==1) return -1;
    else             bytes= ones - 1;
    
    val= get_bits(gb, 7-ones);
    while(bytes--){
        const int tmp = get_bits(gb, 8);
        
        if((tmp>>6) != 2)
            return -1;
        val<<=6;
        val|= tmp&0x3F;
    }
    return val;
}

static int get_crc8(const uint8_t *buf, int count){
    int crc=0;
    int i;
    
    for(i=0; i<count; i++){
        crc = table_crc8[crc ^ buf[i]];
    }

    return crc;
}

static int decode_residuals(FLACContext *s, int32_t* decoded, int pred_order)
{
    int i, tmp, partition, method_type, rice_order;
    int sample = 0, samples;

    method_type = get_bits(&s->gb, 2);
    if (method_type != 0){
        //fprintf(stderr,"illegal residual coding method %d\n", method_type);
        return -1;
    }
    
    rice_order = get_bits(&s->gb, 4);

    samples= s->blocksize >> rice_order;

    sample= 
    i= pred_order;
    for (partition = 0; partition < (1 << rice_order); partition++)
    {
        tmp = get_bits(&s->gb, 4);
        if (tmp == 15)
        {
            //fprintf(stderr,"fixed len partition\n");
            tmp = get_bits(&s->gb, 5);
            for (; i < samples; i++, sample++)
                decoded[sample] = get_sbits(&s->gb, tmp);
        }
        else
        {
            for (; i < samples; i++, sample++){
                decoded[sample] = get_sr_golomb_flac(&s->gb, tmp, INT_MAX, 0);
            }
        }
        i= 0;
    }

    return 0;
}    

static int decode_subframe_fixed(FLACContext *s, int32_t* decoded, int pred_order)
{
    int i;
        
    /* warm up samples */
    for (i = 0; i < pred_order; i++)
    {
        decoded[i] = get_sbits(&s->gb, s->curr_bps);
    }
    
    if (decode_residuals(s, decoded, pred_order) < 0)
        return -1;

    switch(pred_order)
    {
        case 0:
            break;
        case 1:
            for (i = pred_order; i < s->blocksize; i++)
                decoded[i] +=   decoded[i-1];
            break;
        case 2:
            for (i = pred_order; i < s->blocksize; i++)
                decoded[i] += 2*decoded[i-1]
                              - decoded[i-2];
            break;
        case 3:
            for (i = pred_order; i < s->blocksize; i++)
                decoded[i] += 3*decoded[i-1] 
                            - 3*decoded[i-2]
                            +   decoded[i-3];
            break;
        case 4:
            for (i = pred_order; i < s->blocksize; i++)
                decoded[i] += 4*decoded[i-1] 
                            - 6*decoded[i-2]
                            + 4*decoded[i-3]
                            -   decoded[i-4];
            break;
        default:
            return -1;
    }

    return 0;
}

static int decode_subframe_lpc(FLACContext *s, int32_t* decoded, int pred_order)
{
    int sum, i, j;
    int64_t wsum;
    int coeff_prec, qlevel;
    int coeffs[pred_order];

    /* warm up samples */
    for (i = 0; i < pred_order; i++)
    {
        decoded[i] = get_sbits(&s->gb, s->curr_bps);
    }
    
    coeff_prec = get_bits(&s->gb, 4) + 1;
    if (coeff_prec == 16)
    {
        //fprintf(stderr,"invalid coeff precision\n");
        return -1;
    }
    qlevel = get_sbits(&s->gb, 5);
    if (qlevel < 0) 
    {
        //fprintf(stderr,"qlevel %d not supported, maybe buggy stream\n", qlevel);
        return -1;
    }

    for (i = 0; i < pred_order; i++)
    {
        coeffs[i] = get_sbits(&s->gb, coeff_prec);
    }
    
    if (decode_residuals(s, decoded, pred_order) < 0)
        return -1;

    if ((s->bps + coeff_prec + av_log2(pred_order)) <= 32) {
        for (i = pred_order; i < s->blocksize; i++)
        {
            sum = 0;
            for (j = 0; j < pred_order; j++)
                sum += coeffs[j] * decoded[i-j-1];
            decoded[i] += sum >> qlevel;
        }
    } else {
        for (i = pred_order; i < s->blocksize; i++)
        {
            wsum = 0;
            for (j = 0; j < pred_order; j++)
                wsum += (int64_t)coeffs[j] * (int64_t)decoded[i-j-1];
            decoded[i] += wsum >> qlevel;
        }
    }
    
    return 0;
}

static inline int decode_subframe(FLACContext *s, int channel, int32_t* decoded)
{
    int type, wasted = 0;
    int i, tmp;
    
    s->curr_bps = s->bps;
    if(channel == 0){
        if(s->decorrelation == RIGHT_SIDE)
            s->curr_bps++;
    }else{
        if(s->decorrelation == LEFT_SIDE || s->decorrelation == MID_SIDE)
            s->curr_bps++;
    }

    if (get_bits1(&s->gb))
    {
        //av_log(s->avctx, AV_LOG_ERROR, "invalid subframe padding\n");
        return -1;
    }
    type = get_bits(&s->gb, 6);
//    wasted = get_bits1(&s->gb);
    
//    if (wasted)
//    {
//        while (!get_bits1(&s->gb))
//            wasted++;
//        if (wasted)
//            wasted++;
//        s->curr_bps -= wasted;
//    }
#if 0
    wasted= 16 - av_log2(show_bits(&s->gb, 17));
    skip_bits(&s->gb, wasted+1);
    s->curr_bps -= wasted;
#else
    if (get_bits1(&s->gb))
    {
        wasted = 1;
        while (!get_bits1(&s->gb))
            wasted++;
        s->curr_bps -= wasted;
        //fprintf(stderr,"%d wasted bits\n", wasted);
    }
#endif
//FIXME use av_log2 for types
    if (type == 0)
    {
        //fprintf(stderr,"coding type: constant\n");
        tmp = get_sbits(&s->gb, s->curr_bps);
        for (i = 0; i < s->blocksize; i++)
            decoded[i] = tmp;
    }
    else if (type == 1)
    {
        //fprintf(stderr,"coding type: verbatim\n");
        for (i = 0; i < s->blocksize; i++)
            decoded[i] = get_sbits(&s->gb, s->curr_bps);
    }
    else if ((type >= 8) && (type <= 12))
    {
        //fprintf(stderr,"coding type: fixed\n");
        if (decode_subframe_fixed(s, decoded, type & ~0x8) < 0)
            return -1;
    }
    else if (type >= 32)
    {
        //fprintf(stderr,"coding type: lpc\n");
        if (decode_subframe_lpc(s, decoded, (type & ~0x20)+1) < 0)
            return -1;
    }
    else
    {
        //fprintf(stderr,"Unknown coding type: %d\n",type);
        return -1;
    }
        
    if (wasted)
    {
        int i;
        for (i = 0; i < s->blocksize; i++)
            decoded[i] <<= wasted;
    }

    return 0;
}

static int decode_frame(FLACContext *s,
                        int32_t* decoded0,
                        int32_t* decoded1,
                        void (*yield)(void))
{
    int blocksize_code, sample_rate_code, sample_size_code, assignment, crc8;
    int decorrelation, bps, blocksize, samplerate;
    
    blocksize_code = get_bits(&s->gb, 4);

    sample_rate_code = get_bits(&s->gb, 4);
    
    assignment = get_bits(&s->gb, 4); /* channel assignment */
    if (assignment < 8 && s->channels == assignment+1)
        decorrelation = INDEPENDENT;
    else if (assignment >=8 && assignment < 11 && s->channels == 2)
        decorrelation = LEFT_SIDE + assignment - 8;
    else
    {
        return -1;
    }
        
    sample_size_code = get_bits(&s->gb, 3);
    if(sample_size_code == 0)
        bps= s->bps;
    else if((sample_size_code != 3) && (sample_size_code != 7))
        bps = sample_size_table[sample_size_code];
    else 
    {
        return -1;
    }

    if (get_bits1(&s->gb))
    {
        return -1;
    }

    /* Get the samplenumber of the first sample in this block */
    s->samplenumber=get_utf8(&s->gb);

    /* samplenumber actually contains the frame number for streams
       with a constant block size - so we multiply by blocksize to
       get the actual sample number */
    if (s->min_blocksize == s->max_blocksize) {
        s->samplenumber*=s->min_blocksize;
    }

#if 0    
    if (/*((blocksize_code == 6) || (blocksize_code == 7)) &&*/
        (s->min_blocksize != s->max_blocksize)){
    }else{
    }
#endif
    
    if (blocksize_code == 0)
        blocksize = s->min_blocksize;
    else if (blocksize_code == 6)
        blocksize = get_bits(&s->gb, 8)+1;
    else if (blocksize_code == 7)
        blocksize = get_bits(&s->gb, 16)+1;
    else 
        blocksize = blocksize_table[blocksize_code];

    if(blocksize > s->max_blocksize){
        return -1;
    }

    if (sample_rate_code == 0){
        samplerate= s->samplerate;
    }else if ((sample_rate_code > 3) && (sample_rate_code < 12))
        samplerate = sample_rate_table[sample_rate_code];
    else if (sample_rate_code == 12)
        samplerate = get_bits(&s->gb, 8) * 1000;
    else if (sample_rate_code == 13)
        samplerate = get_bits(&s->gb, 16);
    else if (sample_rate_code == 14)
        samplerate = get_bits(&s->gb, 16) * 10;
    else{
        return -1;
    }

    skip_bits(&s->gb, 8);
    crc8= get_crc8(s->gb.buffer, get_bits_count(&s->gb)/8);
    if(crc8){
        return -1;
    }
    
    s->blocksize    = blocksize;
    s->samplerate   = samplerate;
    s->bps          = bps;
    s->decorrelation= decorrelation;

    yield();
    /* subframes */
    if (decode_subframe(s, 0, decoded0) < 0)
        return -1;

    yield();

    if (s->channels==2) {
      if (decode_subframe(s, 1, decoded1) < 0)
          return -1;
    }
    
    yield();
    align_get_bits(&s->gb);

    /* frame footer */
    skip_bits(&s->gb, 16); /* data crc */

    return 0;
}

int flac_decode_frame(FLACContext *s,
                             int32_t* decoded0,
                             int32_t* decoded1,
                             uint8_t *buf, int buf_size,
                             void (*yield)(void))
{
    int tmp = 0, i, input_buf_size = 0;
    int framesize;
    int scale;

    init_get_bits(&s->gb, buf, buf_size*8);

    tmp = show_bits(&s->gb, 16);
    if(tmp != 0xFFF8){
        //fprintf(stderr,"FRAME HEADER not here\n");
        while(get_bits_count(&s->gb)/8+2 < buf_size && show_bits(&s->gb, 16) != 0xFFF8)
           skip_bits(&s->gb, 8);
        goto end; // we may not have enough bits left to decode a frame, so try next time
    }
    skip_bits(&s->gb, 16);

    if ((framesize=decode_frame(s,decoded0,decoded1,yield)) < 0){
       s->bitstream_size=0;
       s->bitstream_index=0;
       return -1;
    }

    yield();

    scale=FLAC_OUTPUT_DEPTH-s->bps;
    switch(s->decorrelation)
    {
        case INDEPENDENT:
            if (s->channels==1) {;
                for (i = 0; i < s->blocksize; i++)
                {
                    decoded0[i] = decoded0[i] << scale;
                }
            } else {
                for (i = 0; i < s->blocksize; i++)
                {
                    decoded0[i] = decoded0[i] << scale;
                    decoded1[i] = decoded1[i] << scale;
                }
            }
            break;
        case LEFT_SIDE:
            //assert(s->channels == 2);
            for (i = 0; i < s->blocksize; i++)
            {
                decoded1[i] = (decoded0[i] - decoded1[i]) << scale;
                decoded0[i] = decoded0[i] << scale;
            }
            break;
        case RIGHT_SIDE:
            //assert(s->channels == 2);
            for (i = 0; i < s->blocksize; i++)
            {
                decoded0[i] = (decoded0[i] + decoded1[i]) << scale;
                decoded1[i] = decoded1[i] << scale;
            }
            break;
        case MID_SIDE:
            //assert(s->channels == 2);
            for (i = 0; i < s->blocksize; i++)
            {
                int mid, side;
                mid = decoded0[i];
                side = decoded1[i];

#if 1 //needs to be checked but IMHO it should be binary identical
                mid -= side>>1;
                decoded0[i] = (mid + side) << scale;
                decoded1[i] = mid << scale;
#else
                
                mid <<= 1;
                if (side & 1)
                    mid++;
                decoded0[i] = ((mid + side) >> 1) << scale;
                decoded1[i] = ((mid - side) >> 1) << scale;
#endif
            }
            break;
    }

end:
    i= (get_bits_count(&s->gb)+7)/8;;
    if(i > buf_size){
        s->bitstream_size=0;
        s->bitstream_index=0;
        return -1;
    }

    if(s->bitstream_size){
        s->bitstream_index += i;
        s->bitstream_size  -= i;
        return input_buf_size;
    }else 
        return i;
}
