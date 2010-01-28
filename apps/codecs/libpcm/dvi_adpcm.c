/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 * Copyright (C) 2009 Yoshihisa Uchida
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "codeclib.h"
#include "pcm_common.h"
#include "support_formats.h"

/*
 * Intel DVI ADPCM
 */

static const uint16_t dvi_adpcm_steptab[89] ICONST_ATTR = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767 };

static const int dvi_adpcm_indextab4[8] ICONST_ATTR = {
    -1, -1, -1, -1, 2, 4, 6, 8 };

static const int dvi_adpcm_indextab3[4] ICONST_ATTR = { -1, -1, 1, 2 };

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format, const unsigned char *fmtpos)
{
    fmt = format;

    (void)fmtpos;

    if (fmt->bitspersample != 4 && fmt->bitspersample != 3)
    {
        DEBUGF("CODEC_ERROR: dvi_adpcm must have 3 or 4 bitspersample\n");
        return false;
    }

    if (fmt->size < 2) {
        DEBUGF("CODEC_ERROR: dvi_adpcm is missing SamplesPerBlock value\n");
        return false;
    }

    /* chunksize is computed so that one chunk is about 1/50s.
     * this make 4096 for 44.1kHz 16bits stereo.
     * It also has to be a multiple of blockalign */
    fmt->chunksize = (1 + fmt->avgbytespersec / (50*fmt->blockalign))*fmt->blockalign;

    /* check that the output buffer is big enough (convert to samplespersec,
       then round to the blockalign multiple below) */
    if ((((uint64_t)fmt->chunksize * ci->id3->frequency * fmt->channels * fmt->bitspersample)>>3)
        /(uint64_t)fmt->avgbytespersec >= PCM_CHUNK_SIZE)
        fmt->chunksize = ((uint64_t)PCM_CHUNK_SIZE * fmt->avgbytespersec
                         /((uint64_t)ci->id3->frequency * fmt->channels * 2
                         * fmt->blockalign)) * fmt->blockalign;

    return true;
}

static uint32_t get_seek_pos(long seek_time)
{
    uint32_t newpos;

    /* use avgbytespersec to round to the closest blockalign multiple,
       add firstblockposn. 64-bit casts to avoid overflows. */
    newpos = (((uint64_t)fmt->avgbytespersec*(seek_time - 1))
             / (1000LL*fmt->blockalign))*fmt->blockalign;
    return newpos;
}

static int decode_dvi_adpcm(const uint8_t *inbuf, size_t inbufsize,
                            int32_t *outbuf, size_t *outbufcount)
{
    size_t nsamples = 0;
    int sample[2];
    int samplecode[32][2];
    int i;
    int stepindex[2];
    int c;
    int diff;
    int step;
    int codem;
    int code;

    if (fmt->bitspersample != 4 && fmt->bitspersample != 3) {
        DEBUGF("decode_dvi_adpcm: wrong bitspersample\n");
        return CODEC_ERROR;
    }

    /* decode block header */
    for (c = 0; c < fmt->channels && inbufsize >= 4; c++) {
        /* decode + push first sample */
        sample[c] = (short)(inbuf[0]|(inbuf[1]<<8));/* need cast for sign-extend */
        outbuf[c] = sample[c] << 13;
        nsamples++;
        stepindex[c] = inbuf[2];
        /* check for step table index overflow */
        if (stepindex[c] > 88) {
            DEBUGF("decode_dvi_adpcm: stepindex[%d]=%d>88\n",c,stepindex[c]);
            return CODEC_ERROR;
        }

        inbuf += 4;
        inbufsize -= 4;
    }
    if (fmt->bitspersample == 4) {
        while (inbufsize >= (size_t)(fmt->channels*4) &&
               (nsamples + (fmt->channels*8) <= *outbufcount))
        {
            for (c = 0; c < fmt->channels; c++)
            {
                samplecode[0][c] = inbuf[0]&0xf;
                samplecode[1][c] = inbuf[0]>>4;
                samplecode[2][c] = inbuf[1]&0xf;
                samplecode[3][c] = inbuf[1]>>4;
                samplecode[4][c] = inbuf[2]&0xf;
                samplecode[5][c] = inbuf[2]>>4;
                samplecode[6][c] = inbuf[3]&0xf;
                samplecode[7][c] = inbuf[3]>>4;
                inbuf += 4;
                inbufsize -= 4;
            }
            for (i = 0; i < 8; i++)
            {
                for (c = 0; c < fmt->channels; c++)
                {
                    step = dvi_adpcm_steptab[stepindex[c]];
                    codem = samplecode[i][c];
                    code = codem & 0x07;

                    /* adjust the step table index */
                    stepindex[c] += dvi_adpcm_indextab4[code];
                    /* check for step table index overflow and underflow */
                    if (stepindex[c] > 88)
                        stepindex[c] = 88;
                    else if (stepindex[c] < 0)
                        stepindex[c] = 0;
                    /* calculate the difference */
#ifdef STRICT_IMA
                    diff = 0;
                    if (code & 4)
                        diff += step;
                    step = step >> 1;
                    if (code & 2)
                        diff += step;
                    step = step >> 1;
                    if (code & 1)
                        diff += step;
                    step = step >> 1;
                    diff += step;
#else
                    diff = ((code + code + 1) * step) >> 3; /* faster */
#endif
                    /* check the sign bit */
                    /* check for overflow and underflow errors */
                    if (code != codem)
                    {
                        sample[c] -= diff;
                        if (sample[c] < -32768)
                            sample[c] = -32768;
                    }
                    else
                    {
                        sample[c] += diff;
                        if (sample[c] > 32767)
                            sample[c] = 32767;
                    }
                    /* output the new sample */
                    outbuf[nsamples] = sample[c] << 13;
                    nsamples++;
                }
            }
        }
    } else {                  /* bitspersample == 3 */
        while (inbufsize >= (uint32_t)(fmt->channels*12) &&
               (nsamples + 32*fmt->channels) <= *outbufcount) {
            for (c = 0; c < fmt->channels; c++) {
                uint16_t bitstream = 0;
                int bitsread = 0;
                for (i = 0; i < 32 && inbufsize > 0; i++) {
                    if (bitsread < 3) {
                        /* read 8 more bits */
                        bitstream |= inbuf[0]<<bitsread;
                        bitsread += 8;
                        inbufsize--;
                        inbuf++;
                    }
                    samplecode[i][c] = bitstream & 7;
                    bitstream = bitstream>>3;
                    bitsread -= 3;
                }
                if (bitsread != 0) {
                    /* 32*3 = 3 words, so we should end with bitsread==0 */
                    DEBUGF("decode_dvi_adpcm: error in implementation\n");
                    return CODEC_ERROR;
                }
            }
            
            for (i = 0; i < 32; i++) {
                for (c = 0; c < fmt->channels; c++) {
                    step = dvi_adpcm_steptab[stepindex[c]];
                    codem = samplecode[i][c];
                    code = codem & 0x03;

                    /* adjust the step table index */
                    stepindex[c] += dvi_adpcm_indextab3[code];
                    /* check for step table index overflow and underflow */
                    if (stepindex[c] > 88)
                        stepindex[c] = 88;
                    else if (stepindex[c] < 0)
                        stepindex[c] = 0;
                    /* calculate the difference */
#ifdef STRICT_IMA
                    diff = 0;
                    if (code & 2)
                        diff += step;
                    step = step >> 1;
                    if (code & 1)
                        diff += step;
                    step = step >> 1;
                    diff += step;
#else
                    diff = ((code + code + 1) * step) >> 3; /* faster */
#endif
                    /* check the sign bit */
                    /* check for overflow and underflow errors */
                    if (code != codem) {
                        sample[c] -= diff;
                        if (sample[c] < -32768)
                            sample[c] = -32768;
                    }
                    else {
                        sample[c] += diff;
                        if (sample[c] > 32767)
                            sample[c] = 32767;
                    }
                    /* output the new sample */
                    outbuf[nsamples] = sample[c] << 13;
                    nsamples++;
                }
            }
        }
    }

    if (nsamples > *outbufcount) {
        DEBUGF("decode_dvi_adpcm: output buffer overflow!\n");
        return CODEC_ERROR;
    }
    *outbufcount = nsamples;
    if (inbufsize != 0) {
        DEBUGF("decode_dvi_adpcm: n=%d unprocessed bytes\n", (int)inbufsize);
    }
    return CODEC_OK;
}

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufsize)
{
    unsigned int i;
    unsigned int nblocks = fmt->chunksize / fmt->blockalign;

    (void)inbufsize;

    for (i = 0; i < nblocks; i++)
    {
        size_t decodedsize = fmt->samplesperblock * fmt->channels;
        if (decode_dvi_adpcm(inbuf + i * fmt->blockalign, fmt->blockalign,
                             outbuf + i * fmt->samplesperblock * fmt->channels,
                             &decodedsize) != CODEC_OK) {
           return CODEC_ERROR;
        }
    }
    *outbufsize = nblocks * fmt->samplesperblock;
    return CODEC_OK;
}

static const struct pcm_codec codec = {
                                          set_format,
                                          get_seek_pos,
                                          decode,
                                      };

const struct pcm_codec *get_dvi_adpcm_codec(void)
{
    return &codec;
}
