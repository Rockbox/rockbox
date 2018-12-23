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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "platform.h"

#include <codecs/librm/rm.h>
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "logf.h"

/* Uncomment the following line for debugging */
//#define DEBUG_RM
#ifndef DEBUG_RM
#undef DEBUGF
#define DEBUGF(...)
#endif

#define ID3V1_OFFSET            -128
#define METADATA_FOOTER_OFFSET  -140

static inline void print_cook_extradata(RMContext *rmctx) {

    DEBUGF("            cook_version = 0x%08lx\n", rm_get_uint32be(rmctx->codec_extradata));
    DEBUGF("            samples_per_frame_per_channel = %d\n", rm_get_uint16be(&rmctx->codec_extradata[4]));
    DEBUGF("            number_of_subbands_in_freq_domain = %d\n", rm_get_uint16be(&rmctx->codec_extradata[6]));
     if(rmctx->extradata_size == 16) {
        DEBUGF("            joint_stereo_subband_start = %d\n",rm_get_uint16be(&rmctx->codec_extradata[12]));
        DEBUGF("            joint_stereo_vlc_bits = %d\n", rm_get_uint16be(&rmctx->codec_extradata[14]));
    }
} 


struct real_object_t
{
    uint32_t fourcc;
    uint32_t size;
    uint16_t version;
};

static int real_read_object_header(int fd, struct real_object_t* obj)
{
    int n;

    if ((n = read_uint32be(fd, &obj->fourcc)) <= 0)
        return n;
    if ((n = read_uint32be(fd, &obj->size)) <= 0)
        return n;
    if ((n = read_uint16be(fd, &obj->version)) <= 0)
        return n;

    return 1;
}

#if (defined(SIMULATOR) && defined(DEBUG_RM))
static char* fourcc2str(uint32_t f)
{
    static char res[5];

    res[0] = (f & 0xff000000) >> 24;
    res[1] = (f & 0xff0000) >> 16;
    res[2] = (f & 0xff00) >> 8;
    res[3] = (f & 0xff);
    res[4] = 0;

    return res;
}
#endif

static int real_read_audio_stream_info(int fd, RMContext *rmctx)
{
    int skipped = 0;
    uint32_t version;
    struct real_object_t obj;
#ifdef SIMULATOR
    uint32_t header_size;
    uint16_t flavor;
    uint32_t coded_framesize;
    uint8_t  interleaver_id_length;
    uint8_t  fourcc_length;
#endif
    uint32_t interleaver_id;
    uint32_t fourcc = 0;

    memset(&obj,0,sizeof(obj));
    read_uint32be(fd, &version);
    skipped += 4;

    DEBUGF("    version=0x%04lx\n",((version >> 16) & 0xff));
    if (((version >> 16) & 0xff) == 3) {
        /* Very old version */
        return -1;
    } else {
#ifdef SIMULATOR
       real_read_object_header(fd, &obj);
       read_uint32be(fd, &header_size);
       /* obj.size will be filled with an unknown value, replaced with header_size */
       DEBUGF("    Object: %s, size: %ld bytes, version: 0x%04x\n",fourcc2str(obj.fourcc),header_size,obj.version);

       read_uint16be(fd, &flavor);
       read_uint32be(fd, &coded_framesize);
#else
       lseek(fd, 20, SEEK_CUR);
#endif
       lseek(fd, 12, SEEK_CUR); /* unknown */
       read_uint16be(fd, &rmctx->sub_packet_h);
       read_uint16be(fd, &rmctx->block_align);
       read_uint16be(fd, &rmctx->sub_packet_size);
       lseek(fd, 2, SEEK_CUR); /* unknown */
       skipped += 40;
       if (((version >> 16) & 0xff) == 5)
       {
           lseek(fd, 6, SEEK_CUR); /* unknown */
           skipped += 6;
       }
       read_uint16be(fd, &rmctx->sample_rate);
       lseek(fd, 4, SEEK_CUR); /* unknown */
       read_uint16be(fd, &rmctx->nb_channels);
       skipped += 8;
       if (((version >> 16) & 0xff) == 4)
       {
#ifdef SIMULATOR
           read_uint8(fd, &interleaver_id_length);
           read_uint32be(fd, &interleaver_id);
           read_uint8(fd, &fourcc_length);
#else
           lseek(fd, 6, SEEK_CUR);
#endif
           read_uint32be(fd, &fourcc);
           skipped += 10;
       }
       if (((version >> 16) & 0xff) == 5)
       {
           read_uint32be(fd, &interleaver_id);
           read_uint32be(fd, &fourcc);
           skipped += 8;
       }
       lseek(fd, 3, SEEK_CUR); /* unknown */
       skipped += 3;
       if (((version >> 16) & 0xff) == 5)
       {
           lseek(fd, 1, SEEK_CUR); /* unknown */
           skipped += 1;
       }
  
       switch(fourcc) {
           case FOURCC('c','o','o','k'):               
               rmctx->codec_type = CODEC_COOK;
               read_uint32be(fd, &rmctx->extradata_size);
               skipped += 4;
               read(fd, rmctx->codec_extradata, rmctx->extradata_size);
               skipped += rmctx->extradata_size;
               break;

           case FOURCC('r','a','a','c'):
           case FOURCC('r','a','c','p'):
               rmctx->codec_type = CODEC_AAC;
               read_uint32be(fd, &rmctx->extradata_size);
               skipped += 4;
               read(fd, rmctx->codec_extradata, rmctx->extradata_size);
               skipped += rmctx->extradata_size;
               break;

           case FOURCC('d','n','e','t'):
               rmctx->codec_type = CODEC_AC3;
               break;

           case FOURCC('a','t','r','c'):  
               rmctx->codec_type = CODEC_ATRAC;
               read_uint32be(fd, &rmctx->extradata_size);
               skipped += 4;
               read(fd, rmctx->codec_extradata, rmctx->extradata_size);
               skipped += rmctx->extradata_size;
               break;

           default: /* Not a supported codec */
               return -1;
       }
       
       DEBUGF("        flavor = %d\n",flavor);
       DEBUGF("        coded_frame_size = %ld\n",coded_framesize);
       DEBUGF("        sub_packet_h = %d\n",rmctx->sub_packet_h);
       DEBUGF("        frame_size = %d\n",rmctx->block_align);
       DEBUGF("        sub_packet_size = %d\n",rmctx->sub_packet_size);
       DEBUGF("        sample_rate= %d\n",rmctx->sample_rate);
       DEBUGF("        channels= %d\n",rmctx->nb_channels);
       DEBUGF("        fourcc = %s\n",fourcc2str(fourcc));
       DEBUGF("        codec_extra_data_length = %ld\n",rmctx->extradata_size);
       DEBUGF("        codec_extradata :\n");
       if(rmctx->codec_type == CODEC_COOK) {
           DEBUGF("        cook_extradata :\n");
           print_cook_extradata(rmctx);
       }

    }

    return skipped;
}

static inline int rm_parse_header(int fd, RMContext *rmctx, struct mp3entry *id3)
{
    struct real_object_t obj;
    int res;
    int skipped;
    off_t curpos __attribute__((unused));
    uint8_t len; /* Holds a string_length, which is then passed to read_string() */

#ifdef SIMULATOR
    uint32_t avg_bitrate = 0;
    uint32_t max_packet_size;
    uint32_t avg_packet_size;
    uint32_t packet_count;
    uint32_t duration;
    uint32_t preroll;
    uint32_t index_offset;    
    uint16_t stream_id;
    uint32_t start_time;
    uint32_t codec_data_size;
#endif
    uint32_t v;
    uint32_t max_bitrate;
    uint16_t num_streams;
    uint32_t next_data_off;
    uint16_t pkt_version;
    uint8_t  header_end;

    memset(&obj,0,sizeof(obj));
    curpos = lseek(fd, 0, SEEK_SET);    
    res = real_read_object_header(fd, &obj);

    if (obj.fourcc == FOURCC('.','r','a',0xfd))
    {
        lseek(fd, 4, SEEK_SET);
        skipped = real_read_audio_stream_info(fd, rmctx);
        if (skipped > 0 && rmctx->codec_type == CODEC_AC3)
        {
            read_uint8(fd,&len);
            skipped += (int)read_string(fd, id3->id3v1buf[0], sizeof(id3->id3v1buf[0]), '\0', len);
            read_uint8(fd,&len);
            skipped += (int)read_string(fd, id3->id3v1buf[1], sizeof(id3->id3v1buf[1]), '\0', len);
            read_uint8(fd,&len);
            skipped += (int)read_string(fd, id3->id3v1buf[2], sizeof(id3->id3v1buf[2]), '\0', len);
            read_uint8(fd,&len);
            skipped += (int)read_string(fd, id3->id3v1buf[3], sizeof(id3->id3v1buf[3]), '\0', len);
            rmctx->data_offset = skipped + 8;
            rmctx->bit_rate = rmctx->block_align * rmctx->sample_rate / 192;
            if (rmctx->block_align)
                rmctx->nb_packets = (filesize(fd) - rmctx->data_offset) / rmctx->block_align;
            if (rmctx->sample_rate)
                rmctx->duration = (uint32_t)(256LL * 6 * 1000 * rmctx->nb_packets / rmctx->sample_rate);
            rmctx->flags |= RM_RAW_DATASTREAM;

            DEBUGF("    data_offset = %ld\n",rmctx->data_offset);
            DEBUGF("    avg_bitrate = %ld\n",rmctx->bit_rate);
            DEBUGF("    duration = %ld\n",rmctx->duration);
            DEBUGF("    title=\"%s\"\n",id3->id3v1buf[0]);
            DEBUGF("    author=\"%s\"\n",id3->id3v1buf[1]);
            DEBUGF("    copyright=\"%s\"\n",id3->id3v1buf[2]);
            DEBUGF("    comment=\"%s\"\n",id3->id3v1buf[3]);
            return 0;
        }
        /* Very old .ra format - not yet supported */
        return -1;
    } 
    else if (obj.fourcc != FOURCC('.','R','M','F'))
    {
        return -1;
    }

    lseek(fd, 8, SEEK_CUR); /* unknown */

    DEBUGF("Object: %s, size: %d bytes, version: 0x%04x, pos: %d\n",fourcc2str(obj.fourcc),(int)obj.size,obj.version,(int)curpos);

    res = real_read_object_header(fd, &obj);
    header_end = 0;
    while(res)
    {
        DEBUGF("Object: %s, size: %d bytes, version: 0x%04x, pos: %d\n",fourcc2str(obj.fourcc),(int)obj.size,obj.version,(int)curpos);
        skipped = 10;
        if(obj.fourcc == FOURCC('I','N','D','X'))
                break;
        switch (obj.fourcc)
        {
            case FOURCC('P','R','O','P'): /* File properties */
                read_uint32be(fd, &max_bitrate);
                read_uint32be(fd, &rmctx->bit_rate); /*avg bitrate*/
#ifdef SIMULATOR
                read_uint32be(fd, &max_packet_size);
                read_uint32be(fd, &avg_packet_size);
                read_uint32be(fd, &packet_count);
#else
                lseek(fd, 3*sizeof(uint32_t), SEEK_CUR);
#endif
                read_uint32be(fd, &rmctx->duration);
#ifdef SIMULATOR
                read_uint32be(fd, &preroll);
                read_uint32be(fd, &index_offset);
#else
                lseek(fd, 2*sizeof(uint32_t), SEEK_CUR);
#endif
                read_uint32be(fd, &rmctx->data_offset);
                read_uint16be(fd, &num_streams);
                read_uint16be(fd, &rmctx->flags);
                skipped += 40;

                DEBUGF("    max_bitrate = %ld\n",max_bitrate);
                DEBUGF("    avg_bitrate = %ld\n",rmctx->bit_rate);
                DEBUGF("    max_packet_size = %ld\n",max_packet_size);
                DEBUGF("    avg_packet_size = %ld\n",avg_packet_size);
                DEBUGF("    packet_count = %ld\n",packet_count);
                DEBUGF("    duration = %ld\n",rmctx->duration);
                DEBUGF("    preroll = %ld\n",preroll);
                DEBUGF("    index_offset = %ld\n",index_offset);
                DEBUGF("    data_offset = %ld\n",rmctx->data_offset);
                DEBUGF("    num_streams = %d\n",num_streams);
                DEBUGF("    flags=0x%04x\n",rmctx->flags);

                rmctx->flags &= 0x00FF;
                break;

            case FOURCC('C','O','N','T'):
                /* Four strings - Title, Author, Copyright, Comment */
                read_uint8(fd,&len);
                skipped += (int)read_string(fd, id3->id3v1buf[0], sizeof(id3->id3v1buf[0]), '\0', len);
                read_uint8(fd,&len);
                skipped += (int)read_string(fd, id3->id3v1buf[1], sizeof(id3->id3v1buf[1]), '\0', len);
                read_uint8(fd,&len);
                skipped += (int)read_string(fd, id3->id3v1buf[2], sizeof(id3->id3v1buf[2]), '\0', len);
                read_uint8(fd,&len);
                skipped += (int)read_string(fd, id3->id3v1buf[3], sizeof(id3->id3v1buf[3]), '\0', len);
                skipped += 4;

                DEBUGF("    title=\"%s\"\n",id3->id3v1buf[0]);
                DEBUGF("    author=\"%s\"\n",id3->id3v1buf[1]);
                DEBUGF("    copyright=\"%s\"\n",id3->id3v1buf[2]);
                DEBUGF("    comment=\"%s\"\n",id3->id3v1buf[3]);
                break;

            case FOURCC('M','D','P','R'): /* Media properties */
#ifdef SIMULATOR
                read_uint16be(fd,&stream_id);
                read_uint32be(fd,&max_bitrate);
                read_uint32be(fd,&avg_bitrate);
                read_uint32be(fd,&max_packet_size);
                read_uint32be(fd,&avg_packet_size);
                read_uint32be(fd,&start_time);
                read_uint32be(fd,&preroll);
                read_uint32be(fd,&duration);
#else
                lseek(fd, 30, SEEK_CUR);
#endif
                skipped += 30;
                read_uint8(fd,&len);
                skipped += 1;
                lseek(fd, len, SEEK_CUR); /* desc */
                skipped += len;
                read_uint8(fd,&len);
                skipped += 1;
#ifdef SIMULATOR
                lseek(fd, len, SEEK_CUR); /* mimetype */
                read_uint32be(fd,&codec_data_size);
#else
                lseek(fd, len + 4, SEEK_CUR);
#endif
                skipped += len + 4;
                read_uint32be(fd,&v);
                skipped += 4;

                DEBUGF("    stream_id = 0x%04x\n",stream_id);
                DEBUGF("    max_bitrate = %ld\n",max_bitrate);
                DEBUGF("    avg_bitrate = %ld\n",avg_bitrate);
                DEBUGF("    max_packet_size = %ld\n",max_packet_size);
                DEBUGF("    avg_packet_size = %ld\n",avg_packet_size);
                DEBUGF("    start_time = %ld\n",start_time);
                DEBUGF("    preroll = %ld\n",preroll);
                DEBUGF("    duration = %ld\n",duration);
                DEBUGF("    codec_data_size = %ld\n",codec_data_size);
                DEBUGF("    v=\"%s\"\n", fourcc2str(v));

                if (v == FOURCC('.','r','a',0xfd))
                {
                    int temp;
                    temp= real_read_audio_stream_info(fd, rmctx);
                    if(temp < 0)
                        return -1;
                    else
                        skipped += temp;
                } 
                else if (v == FOURCC('L','S','D',':'))
                {
                    DEBUGF("Real audio lossless is not supported.");
                    return -1;
                }
                else
                {
                    /* We shall not abort with -1 here. *.rm file often seem
                     * to have a second media properties header that contains
                     * other metadata. */
                    DEBUGF("Unknown header signature :\"%s\"\n", fourcc2str(v));
                }
                    

                break;

            case FOURCC('D','A','T','A'):             
                read_uint32be(fd,&rmctx->nb_packets);
                skipped += 4;
                read_uint32be(fd,&next_data_off);
                skipped += 4;

              /***
               * nb_packets correction :
               *   in some samples, number of packets may not exactly form
               *   an integer number of scrambling units. This is corrected
               *   by constructing a partially filled unit out of the few 
               *   remaining samples at the end of decoding.
               ***/
                if(rmctx->nb_packets % rmctx->sub_packet_h)
                    rmctx->nb_packets += rmctx->sub_packet_h - (rmctx->nb_packets % rmctx->sub_packet_h);

                DEBUGF("    data_nb_packets = %ld\n",rmctx->nb_packets);
                DEBUGF("    next DATA offset = %ld\n",next_data_off);

                if (!next_data_off)
                {
                    if (rmctx->duration == 0 && rmctx->bit_rate != 0)
                    {
                        rmctx->duration = (uint32_t)(8000LL * rmctx->nb_packets * rmctx->block_align / rmctx->bit_rate);
                        DEBUGF("    estimated duration = %ld\n",rmctx->duration);
                    }
                    read_uint16be(fd, &pkt_version);
                    skipped += 2;
                    DEBUGF("    pkt_version=0x%04x\n", pkt_version);
                    if (pkt_version)
                        rmctx->flags |= RM_PKT_V1;
                    rmctx->data_offset = curpos;
                    header_end = 1;
                }
                break; 
        }
        if(header_end) break;
        curpos = lseek(fd, obj.size - skipped, SEEK_CUR);
        res = real_read_object_header(fd, &obj);
    }


    return 0;
}


bool get_rm_metadata(int fd, struct mp3entry* id3)
{
    RMContext *rmctx = (RMContext*) (( (intptr_t)id3->id3v2buf + 3 ) &~ 3);
    memset(rmctx,0,sizeof(RMContext));
    if(rm_parse_header(fd, rmctx, id3) < 0)
        return false;

    if (!setid3v1title(fd, id3)) {
    /* file has no id3v1 tags, use the tags from CONT chunk */
        id3->title  = id3->id3v1buf[0];
        id3->artist = id3->id3v1buf[1];
        id3->comment= id3->id3v1buf[3];
    }

    switch(rmctx->codec_type)
    {
        case CODEC_COOK:
        /* Already set, do nothing */
            break;
        case CODEC_AAC:
            id3->codectype = AFMT_RM_AAC;
            break;

        case CODEC_AC3:
            id3->codectype = AFMT_RM_AC3;
            break;
        
        case CODEC_ATRAC:
            id3->codectype = AFMT_RM_ATRAC3;
            break;
    }

    id3->channels = rmctx->nb_channels;
    id3->extradata_size = rmctx->extradata_size;
    id3->bitrate = (rmctx->bit_rate + 500) / 1000;
    id3->frequency = rmctx->sample_rate;
    id3->length = rmctx->duration;
    id3->filesize = filesize(fd);
    return true;
}
