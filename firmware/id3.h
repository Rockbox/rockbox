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
    char *path;
    char *title;
    char *artist;
    char *album;
    int version;
    int layer;
    int bitrate;
    int frequency;
    int id3v2len;
    int id3v1len;
    int filesize; /* in bytes */
    int length;   /* song length */

    /* these following two fields are used for local buffering */
    char id3v2buf[300];
    char id3v1buf[3][32];
};

typedef struct mp3entry mp3entry;

bool mp3info(mp3entry *entry, char *filename);

#endif
