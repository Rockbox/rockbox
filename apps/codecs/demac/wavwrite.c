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

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "parser.h"


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

int open_wav(struct ape_ctx_t* ape_ctx, char* filename)
{
    int fd;
    int x;
    int filesize;
    int bytespersample;

    fd=open(filename, O_CREAT|O_WRONLY|O_TRUNC|O_BINARY, 0644);
    if (fd < 0)
        return fd;

    bytespersample=ape_ctx->bps/8;

    filesize=ape_ctx->totalsamples*bytespersample*ape_ctx->channels+44;

    // ChunkSize
    x=filesize-8;
    wav_header[4]=(x&0xff);
    wav_header[5]=(x&0xff00)>>8;
    wav_header[6]=(x&0xff0000)>>16;
    wav_header[7]=(x&0xff000000)>>24;

    // Number of channels
    wav_header[22]=ape_ctx->channels;

    // Samplerate
    wav_header[24]=ape_ctx->samplerate&0xff;
    wav_header[25]=(ape_ctx->samplerate&0xff00)>>8;
    wav_header[26]=(ape_ctx->samplerate&0xff0000)>>16;
    wav_header[27]=(ape_ctx->samplerate&0xff000000)>>24;

    // ByteRate
    x=ape_ctx->samplerate*(ape_ctx->bps/8)*ape_ctx->channels;
    wav_header[28]=(x&0xff);
    wav_header[29]=(x&0xff00)>>8;
    wav_header[30]=(x&0xff0000)>>16;
    wav_header[31]=(x&0xff000000)>>24;

    // BlockAlign
    wav_header[32]=(ape_ctx->bps/8)*ape_ctx->channels;

    // Bits per sample
    wav_header[34]=ape_ctx->bps;
    
    // Subchunk2Size
    x=filesize-44;
    wav_header[40]=(x&0xff);
    wav_header[41]=(x&0xff00)>>8;
    wav_header[42]=(x&0xff0000)>>16;
    wav_header[43]=(x&0xff000000)>>24;

    write(fd,wav_header,sizeof(wav_header));

    return fd;
}
