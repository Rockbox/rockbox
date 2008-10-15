/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
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

char* id3_get_num_genre(unsigned int genre_num);
bool id3_is_genre_string(const char *string);
int getid3v2len(int fd);
bool get_mp3_metadata(int fd, struct mp3entry* id3, const char *filename);

bool get_adx_metadata(int fd, struct mp3entry* id3);
bool get_aiff_metadata(int fd, struct mp3entry* id3);
bool get_flac_metadata(int fd, struct mp3entry* id3);
bool get_mp4_metadata(int fd, struct mp3entry* id3);
bool get_monkeys_metadata(int fd, struct mp3entry* id3);
bool get_musepack_metadata(int fd, struct mp3entry *id3);
bool get_sid_metadata(int fd, struct mp3entry* id3);
bool get_mod_metadata(int fd, struct mp3entry* id3);
bool get_spc_metadata(int fd, struct mp3entry* id3);
bool get_ogg_metadata(int fd, struct mp3entry* id3);
bool get_wave_metadata(int fd, struct mp3entry* id3);
bool get_wavpack_metadata(int fd, struct mp3entry* id3);
bool get_a52_metadata(int fd, struct mp3entry* id3);
bool get_asf_metadata(int fd, struct mp3entry* id3);
bool get_asap_metadata(int fd, struct mp3entry* id3);
