/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef ID3_H
#define ID3_H

#include "file.h"

struct mp3entry {
    char path[MAX_PATH];
    char *title;
    char *artist;
    char *album;
    int tracknum;
    int version;
    int layer;
    int year;
    unsigned int bitrate;
    unsigned int frequency;
    unsigned int id3v2len;
    unsigned int id3v1len;
    unsigned int first_frame_offset; /* Byte offset to first real MP3 frame.
                                        Used for skipping leading garbage to
                                        avoid gaps between tracks. */
    unsigned int filesize; /* in bytes */
    unsigned int length;   /* song length */
    unsigned int elapsed;  /* ms played */
    long bpf;              /* bytes per frame */
    long tpf;              /* time per frame */

    /* Xing VBR fields */
    bool vbr;
    unsigned char vbrflags;
    unsigned char toc[100];/* table of contents */

    /* these following two fields are used for local buffering */
    char id3v2buf[300];
    char id3v1buf[3][32];

    /* resume related */
    int offset;            /* bytes played */
    int index;             /* playlist index */
};

#define VBR_FRAMES_FLAG 0x01
#define VBR_BYTES_FLAG  0x02
#define VBR_TOC_FLAG    0x04

bool mp3info(struct mp3entry *entry, char *filename);

#endif
