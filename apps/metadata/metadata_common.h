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

#ifdef ROCKBOX_BIG_ENDIAN
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif

#define TAG_NAME_LENGTH             32
#define TAG_VALUE_LENGTH            128

enum tagtype { TAGTYPE_APE = 1, TAGTYPE_VORBIS };

bool read_ape_tags(int fd, struct mp3entry* id3);
bool read_vorbis_tags(int fd, struct mp3entry *id3,
    long tag_remaining);

bool skip_id3v2(int fd, struct mp3entry *id3);
long read_string(int fd, char* buf, long buf_size, int eos, long size);
#ifdef ROCKBOX_BIG_ENDIAN
#define read_uint32be(fd,buf) read((fd), (buf), 4)
#else
int read_uint32be(int fd, unsigned int* buf);
#endif
unsigned long get_long_le(void* buf);
unsigned short get_short_le(void* buf);
unsigned long get_long_be(void* buf);
long get_slong(void* buf);
unsigned long get_itunes_int32(char* value, int count);
long parse_tag(const char* name, char* value, struct mp3entry* id3,
    char* buf, long buf_remaining, enum tagtype type);
