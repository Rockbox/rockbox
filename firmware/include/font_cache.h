/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2003 Tat Tang
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lru.h"

/*******************************************************************************
 * 
 ******************************************************************************/
struct font_cache
{
    struct lru _lru;
    int _size;
    int _capacity;
    short *_index; /* index of lru handles in char_code order */
};

struct font_cache_entry
{
    unsigned short _char_code;
    unsigned char width;
    unsigned char bitmap[1]; /* place holder */
};

/* void (*f) (void*, struct font_cache_entry*); */
/* Create an auto sized font cache from buf */
void font_cache_create(
    struct font_cache* fcache, void* buf, int buf_size, int bitmap_bytes_size);
/* Get font cache entry */
struct font_cache_entry* font_cache_get(
    struct font_cache* fcache,
    unsigned short char_code,
    void (*callback) (struct font_cache_entry* p, void *callback_data),
    void *callback_data);
