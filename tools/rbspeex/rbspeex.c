/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 Thom Johansen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include <speex/speex.h>
#include <speex/speex_resampler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "rbspeex.h" 

/* Read an unaligned 32-bit little endian long from buffer. */
unsigned int get_long_le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void put_ushort_le(unsigned short x, unsigned char *out)
{
    out[0] = x & 0xff;
    out[1] = x >> 8;
}

void put_uint_le(unsigned int x, unsigned char *out)
{
    out[0] = x & 0xff;
    out[1] = (x >> 8) & 0xff;
    out[2] = (x >> 16) & 0xff;
    out[3] = x >> 24;
}



bool get_wave_metadata(FILE *fd, int *numchan, int *bps, int *sr, int *numsamples)
{
    unsigned char buf[1024];
    unsigned long totalsamples = 0;
    unsigned long channels = 0;
    unsigned long bitspersample = 0;
    unsigned long numbytes = 0;
    size_t read_bytes;
    int i;

    if ((read_bytes = fread(buf, 1, 12, fd)) < 12)
        return false;

    if ((memcmp(buf, "RIFF",4) != 0) || (memcmp(&buf[8], "WAVE", 4) != 0))
        return false;

    /* iterate over WAVE chunks until 'data' chunk */
    while (1) {
        /* get chunk header */
        if ((read_bytes = fread(buf, 1, 8, fd)) < 8)
            return false;

        /* chunkSize */
        i = get_long_le(&buf[4]);

        if (memcmp(buf, "fmt ", 4) == 0) {
            /* get rest of chunk */
            if ((read_bytes = fread(buf, 1, 16, fd)) < 16)
                return false;

            i -= 16;

            channels = *numchan = buf[2] | (buf[3] << 8);
            *sr = get_long_le(&buf[4]);
            /* wBitsPerSample */
            bitspersample = *bps = buf[14] | (buf[15] << 8);
        } else if (memcmp(buf, "data", 4) == 0) {
            numbytes = i;
            break;
        } else if (memcmp(buf, "fact", 4) == 0) {
            /* dwSampleLength */
            if (i >= 4) {
                /* get rest of chunk */
                if ((read_bytes = fread(buf, 1, 4, fd)) < 4)
                    return false;

                i -= 4;
                totalsamples = get_long_le(buf);
            }
        }

        /* seek to next chunk (even chunk sizes must be padded) */
        if (i & 0x01)
            i++;

        if (fseek(fd, i, SEEK_CUR) < 0)
            return false;
    }

    if ((numbytes == 0) || (channels == 0))
        return false;

    if (totalsamples == 0) {
        /* for PCM only */
        totalsamples = numbytes/((((bitspersample - 1) / 8) + 1)*channels);
    }
    *numsamples = totalsamples;
    return true;
}

/* We'll eat an entire WAV file here, and encode it with Speex, packing the
 * bits as tightly as we can. Output is completely raw, with absolutely
 * nothing to identify the contents. Files are left open, so remember to close
 * them.
 */
bool encode_file(FILE *fin, FILE *fout, float quality, int complexity,
                 bool narrowband, float volume, char *errstr, size_t errlen)
{
    spx_int16_t *in = NULL, *inpos;
    spx_int16_t enc_buf[640]; /* Max frame size */
    char cbits[200];
    void *st = NULL;
    SpeexResamplerState *resampler = NULL;
    SpeexBits bits;
    int i, tmp, target_sr, numchan, bps, sr, numsamples, frame_size, lookahead;
    int nbytes;
    bool ret = true;

    if (!get_wave_metadata(fin, &numchan, &bps, &sr, &numsamples)) {
        snprintf(errstr, errlen, "invalid WAV file");
        return false;
    }
    if (numchan != 1) {
        snprintf(errstr, errlen, "input file must be mono");
        return false;
    }
    if (bps != 16) {
        snprintf(errstr, errlen, "samples must be 16 bit");
        return false;
    }

    /* Allocate an encoder of specified type, defaults to wideband */
    st = speex_encoder_init(narrowband ? &speex_nb_mode : &speex_wb_mode);
    if (narrowband)
        target_sr = 8000;
    else
        target_sr = 16000;
    speex_bits_init(&bits);

    /* VBR */
    tmp = 1;
    speex_encoder_ctl(st, SPEEX_SET_VBR, &tmp);
    /* Quality, 0-10 */
    speex_encoder_ctl(st, SPEEX_SET_VBR_QUALITY, &quality);
    /* Complexity, 0-10 */
    speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &complexity);
    speex_encoder_ctl(st, SPEEX_GET_FRAME_SIZE, &frame_size);
    speex_encoder_ctl(st, SPEEX_GET_LOOKAHEAD, &lookahead);

    /* Read input samples into a buffer */
    in = calloc(numsamples + lookahead, sizeof(spx_int16_t));
    if (in == NULL) {
        snprintf(errstr, errlen, "could not allocate clip memory");
        ret = false;
        goto finish;
    }
    if (fread(in, 2, numsamples, fin) != numsamples) {
        snprintf(errstr, errlen, "could not read input file data");
        ret = false;
        goto finish;
   }

    if (volume != 1.0f) {
        for (i = 0; i < numsamples; ++i)
            in[i] *= volume;
    }

    if (sr != target_sr) {
        resampler = speex_resampler_init(1, sr, target_sr, 10, NULL);
        speex_resampler_skip_zeros(resampler);
    }

    /* There will be 'lookahead' samples of zero at the end of the array, to
     * make sure the Speex encoder is allowed to spit out all its data at clip
     * end */
    numsamples += lookahead;
   
    inpos = in;
    while (numsamples > 0) {
        int samples = frame_size;

        /* Check if we need to resample */
        if (sr != target_sr) {
            spx_uint32_t in_len = numsamples, out_len = frame_size;
            double resample_factor = (double)sr/(double)target_sr;
            /* Calculate how many input samples are needed for one full frame
             * out, and add some, just in case. */
            spx_uint32_t samples_in = frame_size*resample_factor + 50;

            /* Limit this or resampler will try to allocate it all on stack */
            if (in_len > samples_in)
                in_len = samples_in;
            speex_resampler_process_int(resampler, 0, inpos, &in_len,
                                        enc_buf, &out_len);
            inpos += in_len;
            samples = out_len;
            numsamples -= in_len;
        } else {
            if (samples > numsamples)
                samples = numsamples;
            memcpy(enc_buf, inpos, samples*2);
            inpos += frame_size;
            numsamples -= frame_size;
        }
        /* Pad out with zeros if we didn't fill all input */
        memset(enc_buf + samples, 0, (frame_size - samples)*2);

        if (speex_encode_int(st, enc_buf, &bits) < 0) {
            snprintf(errstr, errlen, "encoder error");
            ret = false;
            goto finish;
        }

        /* Copy the bits to an array of char that can be written */
        nbytes = speex_bits_write_whole_bytes(&bits, cbits, 200);

        /* Write the compressed data */
        if (fwrite(cbits, 1, nbytes, fout) != nbytes) {
            snprintf(errstr, errlen, "could not write output data");
            ret = false;
            goto finish;
        }
    }
    /* Squeeze out the last bits */
    nbytes = speex_bits_write(&bits, cbits, 200);
    if (fwrite(cbits, 1, nbytes, fout) != nbytes) {
        snprintf(errstr, errlen, "could not write output data");
        ret = false;
    }

finish:
    if (st != NULL)
        speex_encoder_destroy(st);
    speex_bits_destroy(&bits);
    if (resampler != NULL)
        speex_resampler_destroy(resampler);
    if (in != NULL)
        free(in);
    return ret;
}


