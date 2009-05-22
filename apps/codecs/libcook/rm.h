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
#include <stdint.h>

typedef struct rm_packet
{
    uint8_t  data[30000]; /* Reordered data. No malloc, hence the size */
    uint16_t version;
    uint16_t length;
    uint32_t timestamp;
    uint16_t stream_number;
    uint8_t  flags;
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
    uint32_t audiotimestamp; /* Audio packet timestamp*/
    uint16_t sub_packet_cnt; /* Subpacket counter, used while reading */
    uint16_t sub_packet_size, sub_packet_h, coded_framesize; /* Descrambling parameters from container */
    uint16_t audio_framesize; /* Audio frame size from container */
    uint16_t sub_packet_lengths[16]; /* Length of each subpacket */

    /* Codec Context */
    uint16_t block_align;
    uint32_t nb_packets;
    int frame_number;
    uint32_t extradata_size;
    uint16_t sample_rate;
    uint16_t nb_channels;
    uint32_t bit_rate;
    uint16_t flags;

    /*cook extradata*/
    uint32_t cook_version;
    uint16_t samples_pf_pc;    /* samples per frame per channel */
    uint16_t nb_subbands;      /* number of subbands in the frequency domain */
    /* extra 8 bytes for stereo data */
    uint32_t unused;
    uint16_t js_subband_start; /* joint stereo subband start */
    uint16_t js_vlc_bits;    

} RMContext;

int open_wav(char* filename);
void close_wav(int fd, RMContext *rmctx);
int real_parse_header(int fd, RMContext *rmctx);
void rm_get_packet(int fd,RMContext *rmctx, RMPacket *pkt);
#endif
