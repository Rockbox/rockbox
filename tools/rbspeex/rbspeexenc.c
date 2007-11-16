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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

/* Read an unaligned 32-bit little endian long from buffer. */
unsigned int get_long_le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
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
                if ((read_bytes = read(buf, 1, 4, fd)) < 4)
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

int main(int argc, char **argv)
{
    FILE *fin, *fout;
    spx_int16_t *in, *inpos;
    spx_int16_t enc_buf[640]; /* Max frame size */
    char cbits[200];
    int nbytes;
    void *st;
    SpeexResamplerState *resampler = NULL;
    SpeexBits bits;
    int i, tmp;
    float ftmp;
    int numchan, bps, sr, numsamples;
    int frame_size;

    if (argc < 3) {
        printf("Usage: rbspeexenc [options] infile outfile\n"
               "Options:\n"
               "  -q x   Quality, floating point number in the range [0-10]\n"
               "  -c x   Complexity, affects quality and encoding time, where\n"
               "         both increase with increasing values, range [0-10]\n"
               "  Defaults are as in speexenc.\n"
               "\nWARNING: This tool will create files that are only usable by Rockbox!\n"
        );
        return 1;
    }

    /* We'll eat an entire WAV file here, and encode it with Speex, packing the
     * bits as tightly as we can. Output is completely raw, with absolutely
     * nothing to identify the contents.
	 */

    /* Wideband encoding */
    st = speex_encoder_init(&speex_wb_mode);

    /* VBR */
    tmp = 1;
    speex_encoder_ctl(st, SPEEX_SET_VBR, &tmp);
    /* Quality, 0-10 */
    ftmp = 8.f;
    for (i = 1; i < argc - 2; ++i) {
        if (strncmp(argv[i], "-q", 2) == 0) {
            ftmp = atof(argv[i + 1]);
            break;
        }
    }
    speex_encoder_ctl(st, SPEEX_SET_VBR_QUALITY, &ftmp);
    /* Complexity, 0-10 */
    tmp = 3;
    for (i = 1; i < argc - 2; ++i) {
        if (strncmp(argv[i], "-c", 2) == 0) {
            tmp = atoi(argv[i + 1]);
            break;
        }
    }
    speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &tmp);
    speex_encoder_ctl(st, SPEEX_GET_FRAME_SIZE, &frame_size);

    fin = fopen(argv[argc - 2], "rb");
    if (!get_wave_metadata(fin, &numchan, &bps, &sr, &numsamples)) {
        printf("invalid wave file!\n");
        return 1;
    }
    if (sr != 16000) {
        resampler = speex_resampler_init(1, sr, 16000, 10, NULL);
        speex_resampler_skip_zeros(resampler);
        printf("Resampling from %i Hz to 16000 Hz\n", sr);
    }
    if (numchan != 1) {
        printf("Error: input file must be mono\n");
        return 1;
    }
    if (bps != 16) {
        printf("samples must be 16 bit!\n");
        return 1;
    }

    /* Read input samples into a buffer */
    in = malloc(numsamples*2);
    if (malloc == NULL) {
        printf("error on malloc\n");
        return 1;
    }
    fread(in, 2, numsamples, fin);
    fclose(fin);
    
    speex_bits_init(&bits);
    inpos = in;
    fout = fopen(argv[argc - 1], "wb");

    while (numsamples > 0) {
        int samples = frame_size;

        /* Check if we need to resample */
        if (sr != 16000) {
            spx_uint32_t in_len = numsamples, out_len = frame_size;

            /* Limit this or resampler will try to allocate it all on stack */
            if (in_len > 2000)
                in_len = 2000;
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

        speex_encode_int(st, enc_buf, &bits);

        /* Copy the bits to an array of char that can be written */
        nbytes = speex_bits_write_whole_bytes(&bits, cbits, 200);

        /* Write the compressed data */
        fwrite(cbits, 1, nbytes, fout);
    }
 
     /* Squeeze out the last bits */
    nbytes = speex_bits_write(&bits, cbits, 200);
    fwrite(cbits, 1, nbytes, fout);

    /*Destroy the encoder state*/
    speex_encoder_destroy(st);
    /*Destroy the bit-packing struct*/
    speex_bits_destroy(&bits);
    if (resampler != NULL)
        speex_resampler_destroy(resampler);
    fclose(fout);
    free(in);
    return 0;
}
