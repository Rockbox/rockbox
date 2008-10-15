/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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

#ifndef _MP3DATA_H_
#define _MP3DATA_H_

#define MPEG_VERSION1   0
#define MPEG_VERSION2   1
#define MPEG_VERSION2_5 2

struct mp3info {
    /* Standard MP3 frame header fields */
    int version;
    int layer;
    bool protection;
    int bitrate;
    long frequency;
    int padding;
    int channel_mode;
    int mode_extension;
    int emphasis;
    int frame_size;   /* Frame size in bytes */
    int frame_samples; /* Samples per frame */
    int ft_num;       /* Numerator of frametime in milliseconds */
    int ft_den;       /* Denominator of frametime in milliseconds */

    bool is_vbr;      /* True if the file is VBR */
    bool has_toc;     /* True if there is a VBR header in the file */
    bool is_xing_vbr; /* True if the VBR header is of Xing type */
    bool is_vbri_vbr; /* True if the VBR header is of VBRI type */
    unsigned char toc[100];
    unsigned long frame_count; /* Number of frames in the file (if VBR) */
    unsigned long byte_count;  /* File size in bytes */
    unsigned long file_time;   /* Length of the whole file in milliseconds */
    unsigned long vbr_header_pos;
    int enc_delay;    /* Encoder delay, fetched from LAME header */
    int enc_padding;  /* Padded samples added to last frame. LAME header */
};

/* Xing header information */
#define VBR_FRAMES_FLAG  0x01
#define VBR_BYTES_FLAG   0x02
#define VBR_TOC_FLAG     0x04
#define VBR_QUALITY_FLAG 0x08

#define MAX_XING_HEADER_SIZE 576

unsigned long find_next_frame(int fd, long *offset, long max_offset,
                              unsigned long last_header);
unsigned long mem_find_next_frame(int startpos, long *offset, long max_offset,
                                  unsigned long last_header);
int get_mp3file_info(int fd, struct mp3info *info);
int count_mp3_frames(int fd, int startpos, int filesize,
                     void (*progressfunc)(int));
int create_xing_header(int fd, long startpos, long filesize,
                       unsigned char *buf, unsigned long num_frames,
                       unsigned long rec_time, unsigned long header_template,
                       void (*progressfunc)(int), bool generate_toc);

extern unsigned long bytes2int(unsigned long b0,
                               unsigned long b1,
                               unsigned long b2,
                               unsigned long b3);

#endif
