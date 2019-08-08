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

#if CONFIG_CODEC == SWCODEC
char* id3_get_num_genre(unsigned int genre_num);
#endif
int getid3v1len(int fd);
int getid3v2len(int fd);
bool setid3v1title(int fd, struct mp3entry *entry);
void setid3v2title(int fd, struct mp3entry *entry);
bool get_mp3_metadata(int fd, struct mp3entry* id3);
#if CONFIG_CODEC == SWCODEC
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
bool get_rm_metadata(int fd, struct mp3entry* id3);
bool get_nsf_metadata(int fd, struct mp3entry* id3);
bool get_oma_metadata(int fd, struct mp3entry* id3);
bool get_smaf_metadata(int fd, struct mp3entry* id3);
bool get_au_metadata(int fd, struct mp3entry* id3);
bool get_vox_metadata(int fd, struct mp3entry* id3);
bool get_wave64_metadata(int fd, struct mp3entry* id3);
bool get_tta_metadata(int fd, struct mp3entry* id3);
bool get_ay_metadata(int fd, struct mp3entry* id3);
bool get_gbs_metadata(int fd, struct mp3entry* id3);
bool get_hes_metadata(int fd, struct mp3entry* id3);
bool get_sgc_metadata(int fd, struct mp3entry* id3);
bool get_vgm_metadata(int fd, struct mp3entry* id3);
bool get_kss_metadata(int fd, struct mp3entry* id3);
bool get_aac_metadata(int fd, struct mp3entry* id3);
#endif /* CONFIG_CODEC == SWCODEC */
