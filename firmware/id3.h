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

struct mp3entry {
    char path[256];
    char *title;
    char *artist;
    char *album;
    int tracknum;
    int version;
    int layer;
    bool vbr;
    unsigned int bitrate;
    unsigned int frequency;
    unsigned int id3v2len;
    unsigned int id3v1len;
    unsigned int filesize; /* in bytes */
    unsigned int length;   /* song length */
    unsigned int elapsed;  /* ms played */

    /* these following two fields are used for local buffering */
    char id3v2buf[300];
    char id3v1buf[3][32];
};

bool mp3info(struct mp3entry *entry, char *filename);

#endif
