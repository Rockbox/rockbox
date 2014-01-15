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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>
#include "font_cache.h"
#include "debug.h"

/*******************************************************************************
 * font_cache_lru_init
 ******************************************************************************/
static void font_cache_lru_init(void* data)
{
    struct font_cache_entry* p = data;
    p->_char_code = 0xffff;   /* assume invalid char */
}

/*******************************************************************************
 * font_cache_create
 ******************************************************************************/
void font_cache_create(
    struct font_cache* fcache,
    void *buf,
    int buf_size,
    int bitmap_bytes_size)
{
    int font_cache_entry_size =
        sizeof(struct font_cache_entry) + bitmap_bytes_size;

    /* make sure font cache entries are a multiple of 16 bits */
    if (font_cache_entry_size % 2 != 0)
        font_cache_entry_size++;

    int cache_size = buf_size /
        (font_cache_entry_size + LRU_SLOT_OVERHEAD + sizeof(short));

    fcache->_size = 1;
    fcache->_capacity = cache_size;
    fcache->_prev_result = 0;
    fcache->_prev_char_code = 0;

    /* set up index */
    fcache->_index = buf;

    /* set up lru list */
    unsigned char* lru_buf = buf;
    lru_buf += sizeof(short) * cache_size;
    lru_create(&fcache->_lru, lru_buf, cache_size, font_cache_entry_size);

    /* initialise cache */
    lru_traverse(&fcache->_lru, font_cache_lru_init);
    short i;
    for (i = 0; i < cache_size; i++)
        fcache->_index[i] = i; /* small cheat here */
}

/*************************************************************************
 * Binary search that attempts a primary lucky guess that succeeds
 * when there are consecutive codes in the cache between previous 
 * search and new search. Returns a negative of insertion point if 
 * not found.
 ************************************************************************/
static int search(struct font_cache* fcache,
                  unsigned short char_code,
                  int size,
                  int *p_insertion_point )
{
    struct font_cache_entry *p;
    int left, right, mid=-1, c;
    left = 0;
    right = size;
    
    /* go for a lucky guess */
    mid = char_code + 
        fcache->_prev_result - fcache->_prev_char_code;
            
    /* check bounds */        
    if ( mid < 0 || mid > right )
            mid = ( left + right ) / 2;

    do
    {
        p = lru_data(&fcache->_lru, fcache->_index[mid]);
        c = p->_char_code - char_code;

        if (c == 0)
        {
            fcache->_prev_result = mid;
            fcache->_prev_char_code = char_code;
            *p_insertion_point = mid;
            return 1;
        }
        if (c < 0)
            left = mid + 1;
        else
            right = mid - 1;

        mid = (left + right) / 2;
    }
    while (left <= right);
    
    /* not found */
    *p_insertion_point = mid;
    return 0;
}
/*******************************************************************************
 * font_cache_get
 ******************************************************************************/
struct font_cache_entry* font_cache_get(
    struct font_cache* fcache,
    unsigned short char_code,
    bool cache_only,
    void (*callback) (struct font_cache_entry* p, void *callback_data),
    void *callback_data)
{
    struct font_cache_entry* p;
    int insertion_point;
    int index_to_replace;
    
    /* check bounds */
    p = lru_data(&fcache->_lru, fcache->_index[0]);
    if( char_code < p->_char_code )
        insertion_point = -1;
    else
    {
        p = lru_data(&fcache->_lru, fcache->_index[fcache->_capacity - 1]);
        if( char_code > p->_char_code )
        {
            insertion_point = fcache->_capacity - 1;
        }
        else
        {
            if( search(fcache, char_code, fcache->_size - 1, &insertion_point))
            {
                short lru_handle = fcache->_index[insertion_point];
                p = lru_data(&fcache->_lru, lru_handle);
                if (p->_char_code == char_code)
                {
                    lru_touch(&fcache->_lru, lru_handle);
                    return lru_data(&fcache->_lru, lru_handle);
                }
            }
            else
            {
                p = lru_data(&fcache->_lru, 
                fcache->_index[insertion_point+1]);
                if ( char_code > p->_char_code )
                     insertion_point++;
            }
        }
    }
    
    /* not found */
    if (cache_only)
        return NULL;

    /* find index to replace */
    short lru_handle_to_replace = fcache->_lru._head;
    p = lru_data(&fcache->_lru, lru_handle_to_replace);
    search(fcache, p->_char_code, fcache->_size - 1, &index_to_replace);
    if (insertion_point < index_to_replace)
    {
        /* shift memory up */
        memmove(fcache->_index + insertion_point + 2,
                fcache->_index + insertion_point + 1,
                (index_to_replace - insertion_point - 1) * sizeof(short));

        /* add to index */
        fcache->_index[insertion_point + 1] = lru_handle_to_replace;
    }
    else if (insertion_point > index_to_replace)
    {
        /* shift memory down */
        memmove(fcache->_index + index_to_replace,
                fcache->_index + index_to_replace + 1,
                (insertion_point - index_to_replace) * sizeof(short));

        /* add to index */
        fcache->_index[insertion_point] = lru_handle_to_replace;
    }

    /* load new entry into cache */
    lru_touch(&fcache->_lru, lru_handle_to_replace);

    if (fcache->_size < fcache->_capacity)
        fcache->_size++;

    p->_char_code = char_code;
    /* fill bitmap */
    callback(p, callback_data);
    return p;
}
