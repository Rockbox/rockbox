/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Mohamed Tarek
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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "../librm/rm.h"
#include "cook.h"

//#define DUMP_RAW_FRAMES

#define DATA_HEADER_SIZE 18 /* size of DATA chunk header in a rm file */
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

void close_wav(int fd, RMContext *rmctx, COOKContext *q) {
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
    int res, datasize,i;
    int nb_frames = 0;
#ifdef DUMP_RAW_FRAMES 
    char filename[15];
    int fd_out;
#endif
    int32_t outbuf[2048];
    uint16_t fs,sps,h;
    uint32_t packet_count;
    COOKContext q;
    RMContext rmctx;
    RMPacket pkt;

    memset(&q,0,sizeof(COOKContext));
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
    cook_decode_init(&rmctx,&q);
    
    /* change the buffer pointer to point at the first audio frame */
    advance_buffer(&filebuf, rmctx.data_offset+ DATA_HEADER_SIZE);
    while(packet_count)
    {  
        rm_get_packet(&filebuf, &rmctx, &pkt);
        //DEBUGF("total frames = %d packet count = %d output counter = %d \n",rmctx.audio_pkt_cnt*(fs/sps), packet_count,rmctx.audio_pkt_cnt);
        for(i = 0; i < rmctx.audio_pkt_cnt*(fs/sps) ; i++)
        { 
            /* output raw audio frames that are sent to the decoder into separate files */
#ifdef DUMP_RAW_FRAMES 
              snprintf(filename,sizeof(filename),"dump%d.raw",++x);
              fd_out = open(filename,O_WRONLY|O_CREAT|O_APPEND, 0666);
              write(fd_out,pkt.frames[i],sps);  
              close(fd_out);
#endif
            nb_frames = cook_decode_frame(&rmctx,&q, outbuf, &datasize, pkt.frames[i] , rmctx.block_align);
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
