/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2023 Christian Soffke
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
#include "plugin.h"
#include "mul_id3.h"

struct multiple_tracks_id3 {
    unsigned long length;
    unsigned long filesize;
    unsigned long frequency;
    unsigned int artist_hash;
    unsigned int composer_hash;
    unsigned int albumartist_hash;
    unsigned int grouping_hash;
    unsigned int comment_hash;
    unsigned int album_hash;
    unsigned int genre_hash;
    unsigned int codectype;
    unsigned int bitrate;
    int year;
    bool filesize_ovf;
    bool length_ovf;
    bool vbr;
};

static struct multiple_tracks_id3 mul_id3;


/* Calculate modified FNV hash of string
 * has good avalanche behaviour and uniform distribution
 * see http://home.comcast.net/~bretm/hash/ */
static unsigned int mfnv(char *str)
{
    const unsigned int p = 16777619;
    unsigned int hash = 0x811C9DC5; // 2166136261;

    if (!str)
        return 0;

    while(*str)
        hash = (hash ^ *str++) * p;
    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    return hash;
}

void init_mul_id3(void)
{
    mul_id3.artist_hash = 0;
    mul_id3.album_hash = 0;
    mul_id3.genre_hash = 0;
    mul_id3.composer_hash = 0;
    mul_id3.albumartist_hash = 0;
    mul_id3.grouping_hash = 0;
    mul_id3.comment_hash = 0;
    mul_id3.codectype = 0;
    mul_id3.vbr = false;
    mul_id3.bitrate = 0;
    mul_id3.frequency = 0;
    mul_id3.length = 0;
    mul_id3.filesize = 0;
    mul_id3.year = 0;
    mul_id3.length_ovf = false;
    mul_id3.filesize_ovf = false;
}

void collect_id3(struct mp3entry *id3, bool is_first_track)
{
    if (is_first_track)
    {
        mul_id3.artist_hash = mfnv(id3->artist);
        mul_id3.album_hash = mfnv(id3->album);
        mul_id3.genre_hash = mfnv(id3->genre_string);
        mul_id3.composer_hash = mfnv(id3->composer);
        mul_id3.albumartist_hash = mfnv(id3->albumartist);
        mul_id3.grouping_hash = mfnv(id3->grouping);
        mul_id3.comment_hash = mfnv(id3->comment);
        mul_id3.codectype = id3->codectype;
        mul_id3.vbr = id3->vbr;
        mul_id3.bitrate = id3->bitrate;
        mul_id3.frequency = id3->frequency;
        mul_id3.year = id3->year;
    }
    else
    {
        if (mul_id3.artist_hash && (mfnv(id3->artist) != mul_id3.artist_hash))
            mul_id3.artist_hash = 0;
        if (mul_id3.album_hash && (mfnv(id3->album) != mul_id3.album_hash))
            mul_id3.album_hash = 0;
        if (mul_id3.genre_hash && (mfnv(id3->genre_string) != mul_id3.genre_hash))
            mul_id3.genre_hash = 0;
        if (mul_id3.composer_hash && (mfnv(id3->composer) != mul_id3.composer_hash))
            mul_id3.composer_hash = 0;
        if (mul_id3.albumartist_hash && (mfnv(id3->albumartist) !=
                                          mul_id3.albumartist_hash))
            mul_id3.albumartist_hash = 0;
        if (mul_id3.grouping_hash && (mfnv(id3->grouping) != mul_id3.grouping_hash))
            mul_id3.grouping_hash = 0;
        if (mul_id3.comment_hash && (mfnv(id3->comment) != mul_id3.comment_hash))
            mul_id3.comment_hash = 0;

        if (mul_id3.codectype && (id3->codectype != mul_id3.codectype))
            mul_id3.codectype = AFMT_UNKNOWN;
        if (mul_id3.bitrate && (id3->bitrate != mul_id3.bitrate ||
                                 id3->vbr != mul_id3.vbr))
            mul_id3.bitrate = 0;
        if (mul_id3.frequency && (id3->frequency != mul_id3.frequency))
            mul_id3.frequency = 0;
        if (mul_id3.year && (id3->year != mul_id3.year))
            mul_id3.year = 0;
    }

    if (ULONG_MAX - mul_id3.length < id3->length)
    {
        mul_id3.length_ovf = true;
        mul_id3.length = 0;
    }
    else if (!mul_id3.length_ovf)
        mul_id3.length += id3->length;

    if (INT_MAX - mul_id3.filesize < id3->filesize) /* output_dyn_value expects int */
    {
        mul_id3.filesize_ovf = true;
        mul_id3.filesize = 0;
    }
    else if (!mul_id3.filesize_ovf)
        mul_id3.filesize += id3->filesize;
}

void write_id3_mul_tracks(struct mp3entry *id3)
{
    id3->path[0] = '\0';
    id3->title = NULL;
    if (!mul_id3.artist_hash)
        id3->artist = NULL;
    if (!mul_id3.album_hash)
        id3->album = NULL;
    if (!mul_id3.genre_hash)
        id3->genre_string = NULL;
    if (!mul_id3.composer_hash)
        id3->composer = NULL;
    if (!mul_id3.albumartist_hash)
        id3->albumartist = NULL;
    if (!mul_id3.grouping_hash)
        id3->grouping = NULL;
    if (!mul_id3.comment_hash)
        id3->comment = NULL;
    id3->disc_string = NULL;
    id3->track_string = NULL;
    id3->year_string = NULL;
    id3->year = mul_id3.year;
    id3->length = mul_id3.length;
    id3->filesize = mul_id3.filesize;
    id3->frequency = mul_id3.frequency;
    id3->bitrate = mul_id3.bitrate;
    id3->codectype = mul_id3.codectype;
    id3->vbr = mul_id3.vbr;
    id3->discnum = 0;
    id3->tracknum = 0;
    id3->track_level = 0;
    id3->album_level = 0;
}
