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
#ifndef _RM_H
#define _RM_H

#include <stdio.h>
#include <inttypes.h>
#include "bytestream.h"

#define RM_RAW_DATASTREAM 0x0100
#define RM_PKT_V1 0x0200

#define MAX_EXTRADATA_SIZE 16
#define DATA_HEADER_SIZE 18
#define PACKET_HEADER_SIZE 12

enum codecs {
    CODEC_COOK, 
    CODEC_AAC,
    CODEC_AC3,
    CODEC_ATRAC
};

typedef struct rm_packet
{
    uint8_t *frames[100]; /* Pointers to ordered audio frames in buffer */
    uint16_t version;
    uint16_t length;
    uint32_t timestamp;
    uint16_t stream_number;
    uint8_t  flags;

#ifdef TEST
    uint8_t  data[30000]; /* Reordered data. No malloc, hence the size */
#endif
}RMPacket;
    
typedef struct rm_context 
{
    /* Demux Context */
    int old_format;
    int current_stream;
    int remaining_len;
    int audio_stream_num; /*< Stream number for audio packets*/
    int audio_pkt_cnt; /* Output packet counter*/

    /* Stream Variables */
    uint32_t data_offset;
    uint32_t duration;
    uint32_t audiotimestamp; /* Audio packet timestamp*/
    uint16_t sub_packet_cnt; /* Subpacket counter, used while reading */
    uint16_t sub_packet_size, sub_packet_h, coded_framesize; /* Descrambling parameters from container */
    uint16_t audio_framesize; /* Audio frame size from container */
    uint16_t sub_packet_lengths[16]; /* Length of each subpacket */

    /* Codec Context */
    enum codecs codec_type; 
    uint16_t block_align;
    uint32_t nb_packets;
    int frame_number;
    uint16_t sample_rate;
    uint16_t nb_channels;
    uint32_t bit_rate;
    uint16_t flags;

    /*codec extradata*/
    uint32_t extradata_size;
    uint8_t  codec_extradata[MAX_EXTRADATA_SIZE];    

} RMContext;

int real_parse_header(int fd, RMContext *rmctx);

void rm_ac3_swap_bytes(uint8_t *buf, int bufsize);

/* Get a (sub_packet_h*frames_per_packet) number of audio frames from a memory buffer */
int rm_get_packet(uint8_t **src,RMContext *rmctx, RMPacket *pkt);

#ifdef TEST

int filesize(int fd);
void advance_buffer(uint8_t **buf, int val);

/* Get a (sub_packet_h*frames_per_packet) number of audio frames from a file descriptor */
void rm_get_packet_fd(int fd,RMContext *rmctx, RMPacket *pkt);

#endif /* TEST */

#endif /* _RM_H */
