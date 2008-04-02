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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "id3.h"

bool get_adx_metadata(int fd, struct mp3entry* id3);
bool get_aiff_metadata(int fd, struct mp3entry* id3);
bool get_flac_metadata(int fd, struct mp3entry* id3);
bool get_mp4_metadata(int fd, struct mp3entry* id3);
bool get_monkeys_metadata(int fd, struct mp3entry* id3);
bool get_musepack_metadata(int fd, struct mp3entry *id3);
bool get_sid_metadata(int fd, struct mp3entry* id3);
bool get_spc_metadata(int fd, struct mp3entry* id3);
bool get_ogg_metadata(int fd, struct mp3entry* id3);
bool get_wave_metadata(int fd, struct mp3entry* id3);
bool get_wavpack_metadata(int fd, struct mp3entry* id3);
bool get_a52_metadata(int fd, struct mp3entry* id3);
bool get_asf_metadata(int fd, struct mp3entry* id3);
