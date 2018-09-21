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
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "rbspeex.h"
  
 #define USAGE_TEXT \
"Usage: rbspeexdec infile outfile\n"\
"rbspeexdec outputs mono 16 bit 16 kHz WAV files.\n"\
"WARNING: This tool will only decode files made with rbspeexenc!\n"


int main(int argc, char **argv)
{
    FILE *fin, *fout;
    char *indata;
    short out[640]; /* max frame size (UWB) */
    unsigned char wavhdr[44];
    int numbytes;
    void *st; /* decoder state */
    SpeexBits bits;
    int i, tmp, lookahead, frame_size;
    unsigned int samples = 0;
    long insize;

    if (argc < 3) {
        printf(USAGE_TEXT);
        return 1;
    }

    /* Rockbox speex streams are always assumed to be WB */
    st = speex_decoder_init(&speex_wb_mode);
 
    /* Set the perceptual enhancement on (is default, but doesn't hurt) */
    tmp = 1;
    speex_decoder_ctl(st, SPEEX_SET_ENH, &tmp);
    speex_decoder_ctl(st, SPEEX_GET_LOOKAHEAD, &lookahead);
    speex_decoder_ctl(st, SPEEX_GET_FRAME_SIZE, &frame_size);
 
    if ((fin = fopen(argv[1], "rb")) == NULL) {
        printf("Error: could not open input file\n");
        return 1;
    }
    if ((fout = fopen(argv[2], "wb")) == NULL) {
        printf("Error: could not open output file\n");
        return 1;
    }
    /* slurp infile */
    fseek(fin, 0, SEEK_END);
    insize = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    indata = malloc(insize);
    insize = fread(indata, 1, insize, fin);
    fclose(fin);

    /* fill in wav header */
    strcpy(wavhdr, "RIFF");
    strcpy(wavhdr + 8, "WAVEfmt ");
    put_uint_le(16, wavhdr + 16);
    put_ushort_le(1, wavhdr + 20);      /* PCM data */
    put_ushort_le(1, wavhdr + 22);      /* mono */
    put_uint_le(16000, wavhdr + 24);    /* 16000 Hz */
    put_uint_le(16000*2, wavhdr + 28);  /* chan*sr*bbs/8 */
    put_ushort_le(2, wavhdr + 32);      /* chan*bps/8 */
    put_ushort_le(16, wavhdr + 34);     /* bits per sample */
    strcpy(wavhdr + 36, "data");
    fwrite(wavhdr, 1, 44, fout);        /* write header */
    /* make bit buffer use our own buffer */
    speex_bits_set_bit_buffer(&bits, indata, insize);
    while (speex_decode_int(st, &bits, out) == 0) {
        /* if no error, write decoded audio */
#if defined(__BIG_ENDIAN__)
        /* byteswap samples from host (big) endianess to file (little) before
         * writing. */
        unsigned int a = frame_size - lookahead;
        while(a--) {
            out[lookahead + a] = ((unsigned short)out[lookahead+a]<<8)&0xff00
                               | ((unsigned short)out[lookahead+a]>>8)&0x00ff;
        }
#endif
        fwrite(out + lookahead, sizeof(short), frame_size - lookahead, fout);
        samples += frame_size - lookahead;
        lookahead = 0; /* only skip samples at the start */
    }
    speex_decoder_destroy(st);
    /* now fill in the values in the wav header we didn't have at the start */
    fseek(fout, 4, SEEK_SET);
    put_uint_le(36 + samples*2, wavhdr); /* header size + data size */
    fwrite(wavhdr, 1, 4, fout);
    fseek(fout, 40, SEEK_SET);
    put_uint_le(samples*2, wavhdr);      /* data size */
    fwrite(wavhdr, 1, 4, fout);
    fclose(fout);
    free(indata);
    return 0;
}

