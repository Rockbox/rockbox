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

#include "rm2wav.h"
#include "cook.h"

//#define DUMP_RAW_FRAMES
#define DEBUGF(message,args ...) av_log(NULL,AV_LOG_ERROR,message,## args)
int main(int argc, char *argv[])
{
    int fd, fd_dec;
    int res, datasize,x,i;
    int nb_frames = 0;
#ifdef DUMP_RAW_FRAMES 
    char filename[15];
    int fd_out;
#endif
    int16_t outbuf[2048];
    uint8_t inbuf[1024];
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
    av_log(NULL,AV_LOG_ERROR,"nb_frames = %d\n",nb_frames);
    x = 0;
    if(packet_count % h)
    {
        packet_count += h - (packet_count % h);
        rmctx.nb_packets = packet_count;
    }
    while(packet_count)
    {  
        
        memset(pkt.data,0,sizeof(pkt.data));
        rm_get_packet(fd, &rmctx, &pkt);
        DEBUGF("total frames = %d packet count = %d output counter = %d \n",rmctx.audio_pkt_cnt*(fs/sps), packet_count,rmctx.audio_pkt_cnt);
        for(i = 0; i < rmctx.audio_pkt_cnt*(fs/sps) ; i++)
        { 
            /* output raw audio frames that are sent to the decoder into separate files */
            #ifdef DUMP_RAW_FRAMES 
              snprintf(filename,sizeof(filename),"dump%d.raw",++x);
              fd_out = open(filename,O_WRONLY|O_CREAT|O_APPEND);           
              write(fd_out,pkt.data+i*sps,sps);  
              close(fd_out);
            #endif

            memcpy(inbuf,pkt.data+i*sps,sps);
            nb_frames = cook_decode_frame(&rmctx,&q, outbuf, &datasize, inbuf , rmctx.block_align);
            rmctx.frame_number++;
            write(fd_dec,outbuf,datasize);        
        }
        packet_count -= rmctx.audio_pkt_cnt;
        rmctx.audio_pkt_cnt = 0;
    } 
    cook_decode_close(&q);
    close_wav(fd_dec,&rmctx);
    close(fd);


  return 0;
}
