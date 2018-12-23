/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dave Chapman
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

#include "rm.h"
#ifdef ROCKBOX
#include "codeclib.h"
#endif

#ifdef TEST
#include <fcntl.h>
#include <unistd.h>
#include "inttypes.h"
#include <sys/stat.h>

int filesize(int fd)
{
  struct stat buf;

  if (fstat(fd,&buf) == -1) {
    return -1;
  } else {
    return (int)buf.st_size;
  }
}

static int read_uint8(int fd, uint8_t* buf)
{
    unsigned char tmp[1];
    int res;

    res=read(fd, tmp, 1);
    *buf = tmp[0];
    return res;
}

static int read_uint16be(int fd, uint16_t* buf)
{
    unsigned char tmp[2];
    int res;

    res=read(fd, tmp, 2);
    *buf = (tmp[0] << 8) | tmp[1];
    return res;
}

static int read_uint32be(int fd, uint32_t* buf)
{
    unsigned char tmp[4];
    int res;

    res=read(fd, tmp, 4);
    *buf = (tmp[0] << 24) | (tmp[1] << 16) | (tmp[2] << 8) | tmp[3];
    return res;
}


static void print_cook_extradata(RMContext *rmctx) {

    DEBUGF("            cook_version = 0x%08x\n", rm_get_uint32be(rmctx->codec_extradata));
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

#define FOURCC(a,b,c,d) (((a)<<24) | ((b) << 16) | ((c) << 8) | (d))

static int real_read_object_header(int fd, struct real_object_t* obj)
{
    int n;

    if ((n = read_uint32be(fd, &obj->fourcc)) <= 0) return n;
    if ((n = read_uint32be(fd, &obj->size)) <= 0) return n;
    if ((n = read_uint16be(fd, &obj->version)) <= 0) return n;

    return 1;
}

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

static int read_str(int fd, char* buf)
{
    uint8_t len;
    int res;

    res = read(fd, &len, 1);
    res = read(fd, buf, len);
    buf[len]=0;

    return len+1;
}

static int real_read_audio_stream_info(int fd, RMContext *rmctx)
{
    int skipped = 0;
    uint32_t version;
    struct real_object_t obj;
    memset(&obj,0,sizeof(obj));
    uint32_t header_size;
    uint16_t flavor;
    uint32_t coded_framesize;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
    uint16_t unknown4;
    uint16_t unknown5;
    uint16_t unknown6;
    uint16_t unknown7;
    uint32_t unknown8;
    uint8_t  interleaver_id_length;
    uint32_t interleaver_id;
    uint8_t  fourcc_length;
    uint32_t fourcc = 0;
    uint8_t  unknown9;
    uint16_t unknown10;
    uint8_t  unknown11;

    read_uint32be(fd, &version);
    skipped += 4;

    DEBUGF("    version=0x%04x\n",((version >> 16) & 0xff));
    if (((version >> 16) & 0xff) == 3) {
        /* Very old version */
    } else {
       real_read_object_header(fd, &obj);
       skipped += 10;
       read_uint32be(fd, &header_size);
       skipped += 4;
       /* obj.size will be filled with an unknown value, replaced with header_size */
       DEBUGF("    Object: %s, size: %d bytes, version: 0x%04x\n",fourcc2str(obj.fourcc),header_size,obj.version);

       read_uint16be(fd, &flavor);
       read_uint32be(fd, &coded_framesize);
       read_uint32be(fd, &unknown1);
       read_uint32be(fd, &unknown2);
       read_uint32be(fd, &unknown3);
       read_uint16be(fd, &rmctx->sub_packet_h);
       read_uint16be(fd, &rmctx->block_align);
       read_uint16be(fd, &rmctx->sub_packet_size);
       read_uint16be(fd, &unknown4);
       skipped += 26;
       if (((version >> 16) & 0xff) == 5)
       {
           read_uint16be(fd, &unknown5);
           read_uint16be(fd, &unknown6);
           read_uint16be(fd, &unknown7);
           skipped += 6;
       }
       read_uint16be(fd, &rmctx->sample_rate);
       read_uint32be(fd, &unknown8);
       read_uint16be(fd, &rmctx->nb_channels);
       skipped += 8;
       if (((version >> 16) & 0xff) == 4)
       {
           read_uint8(fd, &interleaver_id_length);
           read_uint32be(fd, &interleaver_id);
           read_uint8(fd, &fourcc_length);
           read_uint32be(fd, &fourcc);
           skipped += 10;
       }
       if (((version >> 16) & 0xff) == 5)
       {
           read_uint32be(fd, &interleaver_id);
           read_uint32be(fd, &fourcc);
           skipped += 8;
       }
       read_uint8(fd,&unknown9);
       read_uint16be(fd,&unknown10);
       skipped += 3;
       if (((version >> 16) & 0xff) == 5)
       {
           read_uint8(fd, &unknown11);
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

           case FOURCC('a','t','r','c'):  
               rmctx->codec_type = CODEC_ATRAC;
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

           default: /* Not a supported codec */
               return -1;
       }
       
       
       DEBUGF("        flavor = %d\n",flavor);
       DEBUGF("        coded_frame_size = %d\n",coded_framesize);
       DEBUGF("        sub_packet_h = %d\n",rmctx->sub_packet_h);
       DEBUGF("        frame_size = %d\n",rmctx->block_align);
       DEBUGF("        sub_packet_size = %d\n",rmctx->sub_packet_size);
       DEBUGF("        sample_rate= %d\n",rmctx->sample_rate);
       DEBUGF("        channels= %d\n",rmctx->nb_channels);
       DEBUGF("        fourcc = %s\n",fourcc2str(fourcc));
       DEBUGF("        codec_extra_data_length = %d\n",rmctx->extradata_size);
       if(rmctx->codec_type == CODEC_COOK) {
           DEBUGF("        cook_extradata :\n");
           print_cook_extradata(rmctx);
       }

    }

    return skipped;
}

int real_parse_header(int fd, RMContext *rmctx)
{
    struct real_object_t obj;
    memset(&obj,0,sizeof(obj));
    int res;
    int skipped;
    off_t curpos;

    uint32_t unknown1;
    uint32_t unknown2;

    uint32_t max_bitrate;
    uint32_t avg_bitrate = 0;
    uint32_t max_packet_size;
    uint32_t avg_packet_size;
    uint32_t packet_count;
    uint32_t duration;
    uint32_t preroll;
    uint32_t index_offset;
    uint16_t num_streams;
    uint16_t flags = 0;

    uint16_t stream_id;
    uint32_t start_time;
    char desc[256];
    char mimetype[256];
    uint32_t codec_data_size;
    uint32_t v;

    char title[256];
    char author[256];
    char copyright[256];
    char comment[256];

    uint32_t next_data_off;
    uint8_t  header_end;

    curpos = lseek(fd, 0, SEEK_SET);    
    res = real_read_object_header(fd, &obj);

    if (obj.fourcc == FOURCC('.','r','a',0xfd))
    {
        /* Very old .ra format - not yet supported */
        return -1;
    } 
    else if (obj.fourcc != FOURCC('.','R','M','F'))
    {
        return -1;
    }

    read_uint32be(fd, &unknown1);
    read_uint32be(fd, &unknown2);

    DEBUGF("Object: %s, size: %d bytes, version: 0x%04x, pos: %d\n",fourcc2str(obj.fourcc),(int)obj.size,obj.version,(int)curpos);
    DEBUGF("    unknown1=%d (0x%08x)\n",unknown1,unknown1);
    DEBUGF("    unknown2=%d (0x%08x)\n",unknown2,unknown2);

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
                read_uint32be(fd, &max_packet_size);
                read_uint32be(fd, &avg_packet_size);
                read_uint32be(fd, &packet_count);
                read_uint32be(fd, &rmctx->duration);
                read_uint32be(fd, &preroll);
                read_uint32be(fd, &index_offset);
                read_uint32be(fd, &rmctx->data_offset);
                read_uint16be(fd, &num_streams);
                read_uint16be(fd, &rmctx->flags);
                skipped += 40;

                DEBUGF("    max_bitrate = %d\n",max_bitrate);
                DEBUGF("    avg_bitrate = %d\n",avg_bitrate);
                DEBUGF("    max_packet_size = %d\n",max_packet_size);
                DEBUGF("    avg_packet_size = %d\n",avg_packet_size);
                DEBUGF("    packet_count = %d\n",packet_count);
                DEBUGF("    duration = %d\n",rmctx->duration);
                DEBUGF("    preroll = %d\n",preroll);
                DEBUGF("    index_offset = %d\n",index_offset);
                DEBUGF("    data_offset = %d\n",rmctx->data_offset);
                DEBUGF("    num_streams = %d\n",num_streams);
                DEBUGF("    flags=0x%04x\n",flags);
                break;

            case FOURCC('C','O','N','T'):
                /* Four strings - Title, Author, Copyright, Comment */
                skipped += read_str(fd,title);
                skipped += read_str(fd,author);
                skipped += read_str(fd,copyright);
                skipped += read_str(fd,comment);

                DEBUGF("    title=\"%s\"\n",title);
                DEBUGF("    author=\"%s\"\n",author);
                DEBUGF("    copyright=\"%s\"\n",copyright);
                DEBUGF("    comment=\"%s\"\n",comment);
                break;

            case FOURCC('M','D','P','R'): /* Media properties */
                read_uint16be(fd,&stream_id);
                skipped += 2;
                read_uint32be(fd,&max_bitrate);
                skipped += 4;
                read_uint32be(fd,&avg_bitrate);
                skipped += 4;
                read_uint32be(fd,&max_packet_size);
                skipped += 4;
                read_uint32be(fd,&avg_packet_size);
                skipped += 4;
                read_uint32be(fd,&start_time);
                skipped += 4;
                read_uint32be(fd,&preroll);
                skipped += 4;
                read_uint32be(fd,&duration);
                skipped += 4;
                skipped += read_str(fd,desc);
                skipped += read_str(fd,mimetype);
                read_uint32be(fd,&codec_data_size);
                skipped += 4;
                //From ffmpeg: codec_pos = url_ftell(pb);
                read_uint32be(fd,&v);
                skipped += 4;

                DEBUGF("    stream_id = 0x%04x\n",stream_id);
                DEBUGF("    max_bitrate = %d\n",max_bitrate);
                DEBUGF("    avg_bitrate = %d\n",avg_bitrate);
                DEBUGF("    max_packet_size = %d\n",max_packet_size);
                DEBUGF("    avg_packet_size = %d\n",avg_packet_size);
                DEBUGF("    start_time = %d\n",start_time);
                DEBUGF("    preroll = %d\n",preroll);
                DEBUGF("    duration = %d\n",duration);
                DEBUGF("    desc=\"%s\"\n",desc);
                DEBUGF("    mimetype=\"%s\"\n",mimetype);
                DEBUGF("    codec_data_size = %d\n",codec_data_size);
                DEBUGF("    v=\"%s\"\n", fourcc2str(v));

                if (v == FOURCC('.','r','a',0xfd))
                {
                    skipped += real_read_audio_stream_info(fd, rmctx);
                }

                break;

            case FOURCC('D','A','T','A'):    
                
              read_uint32be(fd,&rmctx->nb_packets);
              skipped += 4;
              read_uint32be(fd,&next_data_off);
              skipped += 4;
              if (!rmctx->nb_packets && (rmctx->flags & 4))
                  rmctx->nb_packets = 3600 * 25;        

              /***
               * nb_packets correction :
               *   in some samples, number of packets may not exactly form
               *   an integer number of scrambling units. This is corrected
               *   by constructing a partially filled unit out of the few 
               *   remaining samples at the end of decoding.
               ***/
              if(rmctx->nb_packets % rmctx->sub_packet_h)
                  rmctx->nb_packets += rmctx->sub_packet_h - (rmctx->nb_packets % rmctx->sub_packet_h);

              DEBUGF("    data_nb_packets = %d\n",rmctx->nb_packets);
              DEBUGF("    next DATA offset = %d\n",next_data_off);
              header_end = 1;           
                break; 
        }
        if(header_end) break;
        curpos = lseek(fd, obj.size - skipped, SEEK_CUR);
        res = real_read_object_header(fd, &obj);
    }


    return 0;
}

void rm_get_packet_fd(int fd,RMContext *rmctx, RMPacket *pkt)
{   
    uint8_t unknown,packet_group;
    uint16_t x;
    uint16_t sps = rmctx->sub_packet_size;
    uint16_t h = rmctx->sub_packet_h;
    uint16_t y = rmctx->sub_packet_cnt;
    uint16_t w = rmctx->audio_framesize;
    int res;
    do
    {
        y = rmctx->sub_packet_cnt;
        read_uint16be(fd,&pkt->version);
        read_uint16be(fd,&pkt->length);
        read_uint16be(fd,&pkt->stream_number);
        read_uint32be(fd,&pkt->timestamp);
        DEBUGF("    version = %d\n"
               "    length  = %d\n"
               "    stream  = %d\n"
               "    timestmp= %d\n",pkt->version,pkt->length,pkt->stream_number,pkt->timestamp);

        if(pkt->version == 0)
        {
            read_uint8(fd,&packet_group);
            read_uint8(fd,&pkt->flags);
        }
        if(pkt->version == 1)
            read_uint8(fd,&unknown);

        if (pkt->flags & 2) /* keyframe */
            y = rmctx->sub_packet_cnt = 0;
        if (!y) /* if keyframe update playback elapsed time */
            rmctx->audiotimestamp = pkt->timestamp;

        for(x = 0 ; x < w/sps; x++)
        {
            res = read(fd,pkt->data+(sps*(h*x+((h+1)/2)*(y&1)+(y>>1))), sps);
        }
        rmctx->audio_pkt_cnt++;
    }while(++(rmctx->sub_packet_cnt) < h);

}
#endif /*TEST*/

void rm_ac3_swap_bytes(uint8_t *buf, int bufsize)
{
    uint8_t *bufptr;
    for (bufptr = buf; bufptr < buf + bufsize - 1; bufptr += 2)
    {
        bufptr[0] ^= bufptr[1];
        bufptr[1] ^= bufptr[0];
        bufptr[0] ^= bufptr[1];
    }
}

int rm_get_packet(uint8_t **src,RMContext *rmctx, RMPacket *pkt)
{   
    int consumed = 0;
    int headerlen;
    /* rockbox: comment 'set but unused' variables
    uint8_t unknown;
    */
    uint16_t x;
    uint16_t sps = rmctx->sub_packet_size;
    uint16_t h = rmctx->sub_packet_h;
    uint16_t y = 0;
    uint16_t w = rmctx->audio_framesize;

    rmctx->sub_packet_cnt = 0;
    rmctx->audio_pkt_cnt = 0;

    do
    {
        y = rmctx->sub_packet_cnt;
        pkt->version       = rm_get_uint16be(*src);

        /* Simple error checking */
        if(pkt->version != 0 && pkt->version != 1) 
        {
             DEBUGF("parsing packets failed\n");
             return -1;
        }
        
        headerlen = PACKET_HEADER_SIZE + (pkt->version ? 1 : 0);
        pkt->length        = rm_get_uint16be(*src+2);
        pkt->stream_number = rm_get_uint16be(*src+4);
        pkt->timestamp     = rm_get_uint32be(*src+6);
        /*DEBUGF("    version = %d\n"
               "    length  = %d\n"
               "    stream  = %d\n"
               "    timestamp= %d\n\n",pkt->version,pkt->length,pkt->stream_number,pkt->timestamp);*/
        /* unknown =*/ rm_get_uint8(*src+10);
        pkt->flags   = rm_get_uint8(*src+11);

        if(pkt->version == 1)
            /* unknown = */ rm_get_uint8(*src+12);

        if (pkt->flags & 2) { /* keyframe */
            if (y)
                return consumed;
            y = 0;
        }
        if (!y)
            rmctx->audiotimestamp = pkt->timestamp;

        /* Skip packet header */
        advance_buffer(src, headerlen);
        consumed += headerlen;
        if (rmctx->codec_type == CODEC_COOK || rmctx->codec_type == CODEC_ATRAC) {
            for(x = 0 ; x < w/sps; x++)
            {
                pkt->frames[h*x+((h+1)/2)*(y&1)+(y>>1)] = *src;
                advance_buffer(src,sps);
                consumed += sps;
            }
        }
        else if (rmctx->codec_type == CODEC_AAC) {
            rmctx->sub_packet_cnt = (rm_get_uint16be(*src) & 0xf0) >> 4;
            advance_buffer(src, 2);
            consumed += 2;
            if (rmctx->sub_packet_cnt) {
                for(x = 0; x < rmctx->sub_packet_cnt; x++) {
                    rmctx->sub_packet_lengths[x] = rm_get_uint16be(*src);
                    advance_buffer(src, 2);
                    consumed += 2;
                }                
                rmctx->audio_pkt_cnt = rmctx->sub_packet_cnt;
            }
            break;
        } 

        else if (rmctx->codec_type == CODEC_AC3) {
        /* The byte order of the data is reversed from standard AC3 */
            rm_ac3_swap_bytes(*src, pkt->length - headerlen);
            break;
        }
        else return -1; /* invalid codec type */

        rmctx->audio_pkt_cnt++;
    }while(++(rmctx->sub_packet_cnt) < h);

    return consumed;
}

#ifdef DEBUG
void dump_rm_context(RMContext *rmctx)
{
    DEBUGF("block_align = %d\n", rmctx->block_align);
    DEBUGF("nb_channels = %d\n", rmctx->nb_channels);
    DEBUGF("sample_rate = %d\n", rmctx->sample_rate);
    DEBUGF("bit_rate    = %ld\n", rmctx->bit_rate   );
}
#endif
