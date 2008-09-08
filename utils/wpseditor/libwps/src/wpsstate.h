/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Rostilav Checkan
 *   $Id$
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

#ifndef STATES_H
#define STATES_H
//
struct trackstate
{
    char* title;
    char* artist;
    char* album;
    char* genre_string;
    char* disc_string;
    char* track_string;
    char* year_string;
    char* composer;
    char* comment;
    char* albumartist;
    char* grouping;
    int discnum;
    int tracknum;
    int version;
    int layer;
    int year;

    int length;
    int elapsed;
};

struct wpsstate{
    int volume;
    int fontheight;
    int fontwidth;
    int battery_level;
    int audio_status;
};
#endif
