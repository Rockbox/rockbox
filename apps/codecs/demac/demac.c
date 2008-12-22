/*

demac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

/* 

This example is intended to demonstrate how the decoder can be used in
embedded devices - there is no usage of dynamic memory (i.e. no
malloc/free) and small buffer sizes are chosen to minimise both the
memory usage and decoding latency.

This implementation requires the following memory and supports decoding of all APE files up to 24-bit Stereo.

32768 - data from the input stream to be presented to the decoder in one contiguous chunk.
18432 - decoding buffer (left channel)
18432 - decoding buffer (right channel)

17408+5120+2240 - buffers used for filter histories (compression levels 2000-5000)

In addition, this example uses a static 27648 byte buffer as temporary
storage for outputting the data to a WAV file but that could be
avoided by writing the decoded data one sample at a time.

*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "demac.h"
#include "wavwrite.h"

#ifndef __WIN32__
#define O_BINARY 0
#endif

#define CALC_CRC 1

#define BLOCKS_PER_LOOP     4608
#define MAX_CHANNELS        2
#define MAX_BYTESPERSAMPLE  3

#define INPUT_CHUNKSIZE     (32*1024)

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


/* 4608*2*3 = 27648 bytes */
static unsigned char wavbuffer[BLOCKS_PER_LOOP*MAX_CHANNELS*MAX_BYTESPERSAMPLE];

/* 4608*4 = 18432 bytes per channel */
static int32_t decoded0[BLOCKS_PER_LOOP];
static int32_t decoded1[BLOCKS_PER_LOOP];

/* We assume that 32KB of compressed data is enough to extract up to
   27648 bytes of decompressed data. */

static unsigned char inbuffer[INPUT_CHUNKSIZE];

int ape_decode(char* infile, char* outfile)
{
    int fd;
    int fdwav;
    int currentframe;
    int nblocks;
    int bytesconsumed;
    struct ape_ctx_t ape_ctx;
    int i, n;
    unsigned char* p;
    int bytesinbuffer;
    int blockstodecode;
    int res;
    int firstbyte;
    int16_t  sample16;
    int32_t  sample32;
    uint32_t frame_crc;
    int crc_errors = 0;

    fd = open(infile,O_RDONLY|O_BINARY);
    if (fd < 0) return -1;

    /* Read the file headers to populate the ape_ctx struct */
    if (ape_parseheader(fd,&ape_ctx) < 0) {
        printf("Cannot read header\n");
        close(fd);
        return -1;
    }

    if ((ape_ctx.fileversion < APE_MIN_VERSION) || (ape_ctx.fileversion > APE_MAX_VERSION)) {
        printf("Unsupported file version - %.2f\n", ape_ctx.fileversion/1000.0);
        close(fd);
        return -2;
    }

    //ape_dumpinfo(&ape_ctx);

    printf("Decoding file - v%.2f, compression level %d\n",ape_ctx.fileversion/1000.0,ape_ctx.compressiontype);

    /* Open the WAV file and write a canonical 44-byte WAV header
       based on the audio format information in the ape_ctx struct.

       NOTE: This example doesn't write the original WAV header and
             tail data which are (optionally) stored in the APE file.
     */
    fdwav = open_wav(&ape_ctx,outfile);

    currentframe = 0;

    /* Initialise the buffer */
    lseek(fd, ape_ctx.firstframe, SEEK_SET);
    bytesinbuffer = read(fd, inbuffer, INPUT_CHUNKSIZE);
    firstbyte = 3;  /* Take account of the little-endian 32-bit byte ordering */

    /* The main decoding loop - we decode the frames a small chunk at a time */
    while (currentframe < ape_ctx.totalframes)
    {
        /* Calculate how many blocks there are in this frame */
        if (currentframe == (ape_ctx.totalframes - 1))
            nblocks = ape_ctx.finalframeblocks;
        else
            nblocks = ape_ctx.blocksperframe;

        ape_ctx.currentframeblocks = nblocks;

        /* Initialise the frame decoder */
        init_frame_decoder(&ape_ctx, inbuffer, &firstbyte, &bytesconsumed);

        /* Update buffer */
        memmove(inbuffer,inbuffer + bytesconsumed, bytesinbuffer - bytesconsumed);
        bytesinbuffer -= bytesconsumed;

        n = read(fd, inbuffer + bytesinbuffer, INPUT_CHUNKSIZE - bytesinbuffer);
        bytesinbuffer += n;

#if CALC_CRC
        frame_crc = ape_initcrc();
#endif

        /* Decode the frame a chunk at a time */
        while (nblocks > 0)
        {
            blockstodecode = MIN(BLOCKS_PER_LOOP, nblocks);

            if ((res = decode_chunk(&ape_ctx, inbuffer, &firstbyte,
                                    &bytesconsumed,
                                    decoded0, decoded1,
                                    blockstodecode)) < 0)
            {
                /* Frame decoding error, abort */
                close(fd);
                return res;
            }

            /* Convert the output samples to WAV format and write to output file */
            p = wavbuffer;
            if (ape_ctx.bps == 8) {
                for (i = 0 ; i < blockstodecode ; i++)
                {
                    /* 8 bit WAV uses unsigned samples */
                    *(p++) = (decoded0[i] + 0x80) & 0xff;

                    if (ape_ctx.channels == 2) {
                        *(p++) = (decoded1[i] + 0x80) & 0xff;
                    }
                }
            } else if (ape_ctx.bps == 16) {
                for (i = 0 ; i < blockstodecode ; i++)
                {
                    sample16 = decoded0[i];
                    *(p++) = sample16 & 0xff;
                    *(p++) = (sample16 >> 8) & 0xff;

                    if (ape_ctx.channels == 2) {
                        sample16 = decoded1[i];
                        *(p++) = sample16 & 0xff;
                        *(p++) = (sample16 >> 8) & 0xff;
                    }
                }
            } else if (ape_ctx.bps == 24) {
                for (i = 0 ; i < blockstodecode ; i++)
                {
                    sample32 = decoded0[i];
                    *(p++) = sample32 & 0xff;
                    *(p++) = (sample32 >> 8) & 0xff;
                    *(p++) = (sample32 >> 16) & 0xff;

                    if (ape_ctx.channels == 2) {
                        sample32 = decoded1[i];
                        *(p++) = sample32 & 0xff;
                        *(p++) = (sample32 >> 8) & 0xff;
                        *(p++) = (sample32 >> 16) & 0xff;
                    }
                }
            }

#if CALC_CRC
            frame_crc = ape_updatecrc(wavbuffer, p - wavbuffer, frame_crc);
#endif
            write(fdwav,wavbuffer,p - wavbuffer);

            /* Update the buffer */
            memmove(inbuffer,inbuffer + bytesconsumed, bytesinbuffer - bytesconsumed);
            bytesinbuffer -= bytesconsumed;

            n = read(fd, inbuffer + bytesinbuffer, INPUT_CHUNKSIZE - bytesinbuffer);
            bytesinbuffer += n;

            /* Decrement the block count */
            nblocks -= blockstodecode;
        }

#if CALC_CRC
        frame_crc = ape_finishcrc(frame_crc);

        if (ape_ctx.CRC != frame_crc)
        {
            fprintf(stderr,"CRC error in frame %d\n",currentframe);
            crc_errors++;
        }
#endif

        currentframe++;
    }

    close(fd);
    close(fdwav);

    if (crc_errors > 0)
        return -1;
    else
        return 0;
}

int main(int argc, char* argv[])
{
    int res;

    if (argc != 3) {
        fprintf(stderr,"Usage: demac infile.ape outfile.wav\n");
        return 0;
    }        

    res = ape_decode(argv[1], argv[2]);

    if (res < 0)
    {
        fprintf(stderr,"DECODING ERROR %d, ABORTING\n", res);
    }
    else 
    {
        fprintf(stderr,"DECODED OK - NO CRC ERRORS.\n");
    }

    return 0;
}
