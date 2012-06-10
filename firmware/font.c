/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2002 by Greg Haerr <greg@censoft.com>
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
/*
 * Rockbox startup font initialization
 * This file specifies which fonts get compiled-in and
 * loaded at startup, as well as their mapping into
 * the FONT_SYSFIXED, FONT_UI and FONT_MP3 ids.
 */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "inttypes.h"
#include "lcd.h"
#include "system.h"
#include "font.h"
#include "file.h"
#include "core_alloc.h"
#include "debug.h"
#include "panic.h"
#include "rbunicode.h"
#include "diacritic.h"
#include "rbpaths.h"

#define MAX_FONTSIZE_FOR_16_BIT_OFFSETS 0xFFDB

/* max static loadable font buffer size */
#ifndef MAX_FONT_SIZE
#if LCD_HEIGHT > 64
#if MEMORYSIZE > 2
#define MAX_FONT_SIZE   60000
#else
#define MAX_FONT_SIZE   10000
#endif
#else
#define MAX_FONT_SIZE   4000
#endif
#endif
#define GLYPHS_TO_CACHE 256

#if MEMORYSIZE < 4
#define FONT_HARD_LIMIT
#endif

#ifndef FONT_HEADER_SIZE
#define FONT_HEADER_SIZE 36
#endif

#ifndef BOOTLOADER
/* Font cache includes */
#include "font_cache.h"
#include "lru.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Define this to try loading /.rockbox/.glyphcache    *
 * when a font specific file fails. This requires the  *
 * user to copy and rename a font glyph cache file     */
//#define TRY_DEFAULT_GLYPHCACHE

/* compiled-in font */
extern struct font sysfont;

#ifndef BOOTLOADER

struct buflib_alloc_data {
    struct font font;
    int handle_locks; /* is the buflib handle currently locked? */
    int refcount;       /* how many times has this font been loaded? */
    unsigned char buffer[];
};
static int buflib_allocations[MAXFONTS];

static int cache_fd;
static struct font* cache_pf;

static void glyph_cache_save(int font_id);

static int buflibmove_callback(int handle, void* current, void* new)
{
    (void)handle;
    struct buflib_alloc_data *alloc = (struct buflib_alloc_data*)current;
    ptrdiff_t diff = new - current;

    if (alloc->handle_locks > 0)
        return BUFLIB_CB_CANNOT_MOVE;

#define UPDATE(x) if (x) { x = PTR_ADD(x, diff); }

    UPDATE(alloc->font.bits);
    UPDATE(alloc->font.offset);
    UPDATE(alloc->font.width);

    UPDATE(alloc->font.buffer_start);
    UPDATE(alloc->font.buffer_end);
    UPDATE(alloc->font.buffer_position);

    UPDATE(alloc->font.cache._index);
    UPDATE(alloc->font.cache._lru._base);

    return BUFLIB_CB_OK;
}
static void lock_font_handle(int handle, bool lock)
{
    if ( handle < 0 )
        return;
    struct buflib_alloc_data *alloc = core_get_data(handle);
    if ( lock )
        alloc->handle_locks++;
    else
        alloc->handle_locks--;
}

void font_lock(int font_id, bool lock)
{
    if( font_id < 0 || font_id >= MAXFONTS )
        return;
    if( buflib_allocations[font_id] >= 0 )
        lock_font_handle(buflib_allocations[font_id], lock);
}

static struct buflib_callbacks buflibops = {buflibmove_callback, NULL, NULL };

static inline struct font *pf_from_handle(int handle)
{
    struct buflib_alloc_data *alloc = core_get_data(handle);
    struct font *pf = &alloc->font;
    return pf;
}

static inline unsigned char *buffer_from_handle(int handle)
{
    struct buflib_alloc_data *alloc = core_get_data(handle);
    unsigned char* buffer = alloc->buffer;
    return buffer;
}

/* Font cache structures */
static void cache_create(struct font* pf);
static void glyph_cache_load(const char *font_path, struct font *pf);
/* End Font cache structures */

void font_init(void)
{
    int i = 0;
    cache_fd = -1;
    while (i<MAXFONTS)
        buflib_allocations[i++] = -1;
}

/* Check if we have x bytes left in the file buffer */
#define HAVEBYTES(x) (pf->buffer_position + (x) <= pf->buffer_end)

/* Helper functions to read big-endian unaligned short or long from
   the file buffer.  Bounds-checking must be done in the calling
   function.
 */

static short readshort(struct font *pf)
{
    unsigned short s;

    s = *pf->buffer_position++ & 0xff;
    s |= (*pf->buffer_position++ << 8);
    return s;
}

static int32_t readlong(struct font *pf)
{
    uint32_t l;

    l = *pf->buffer_position++ & 0xff;
    l |= *pf->buffer_position++ << 8;
    l |= ((uint32_t)(*pf->buffer_position++)) << 16;
    l |= ((uint32_t)(*pf->buffer_position++)) << 24;
    return l;
}

static int glyph_bytes( struct font *pf, int width )
{
    int ret;
    if (pf->depth)
        ret = ( pf->height * width + 1 ) / 2;
    else
        ret = width * ((pf->height + 7) / 8);
    return (ret + 1) & ~1;
}

/* Load memory font */
static struct font* font_load_in_memory(struct font* pf,
                                        int32_t nwidth,
                                        int32_t noffset )
{
    int i;
    /* variable font data*/
    pf->buffer_position = pf->buffer_start + 36;
    pf->bits = (unsigned char *)pf->buffer_position;
    pf->buffer_position += pf->bits_size*sizeof(unsigned char);

    if (pf->bits_size < MAX_FONTSIZE_FOR_16_BIT_OFFSETS)
    {
        /* pad to 16-bit boundary */
        pf->buffer_position = (unsigned char *)(((intptr_t)pf->buffer_position + 1) & ~1);
    }
    else
    {
        /* pad to 32-bit boundary*/
        pf->buffer_position = (unsigned char *)(((intptr_t)pf->buffer_position + 3) & ~3);
    }

    if (noffset)
    {
        if (pf->bits_size < MAX_FONTSIZE_FOR_16_BIT_OFFSETS)
        {
            pf->long_offset = 0;
            pf->offset = (uint16_t*)pf->buffer_position;

            /* Check we have sufficient buffer */
            if (!HAVEBYTES(noffset * sizeof(uint16_t)))
                return NULL;

            for (i=0; i<noffset; ++i)
            {
                ((uint16_t*)(pf->offset))[i] = (uint16_t)readshort(pf);
            }
        }
        else
        {
            pf->long_offset = 1;
            pf->offset = (uint16_t*)pf->buffer_position;

            /* Check we have sufficient buffer */
            if (!HAVEBYTES(noffset * sizeof(int32_t)))
                return NULL;

            for (i=0; i<noffset; ++i)
            {
                ((uint32_t*)(pf->offset))[i] = (uint32_t)readlong(pf);
            }
        }
    }
    else
        pf->offset = NULL;

    if (nwidth) {
        pf->width = (unsigned char *)pf->buffer_position;
        pf->buffer_position += nwidth*sizeof(unsigned char);
    }
    else
        pf->width = NULL;

    if (pf->buffer_position > pf->buffer_end)
        return NULL;

    return pf;    /* success!*/
}

/* Load cached font */
static struct font* font_load_cached(struct font* pf,
                                     int32_t nwidth,
                                     int32_t noffset)
{
    /* We are now at the bitmap data, this is fixed at 36.. */
    pf->bits = NULL;

    /* Calculate offset to offset data */
    pf->buffer_position += pf->bits_size * sizeof(unsigned char);

    if (pf->bits_size < MAX_FONTSIZE_FOR_16_BIT_OFFSETS)
    {
        pf->long_offset = 0;
        /* pad to 16-bit boundary */
        pf->buffer_position = (unsigned char *)(((intptr_t)pf->buffer_position + 1) & ~1);
    }
    else
    {
        pf->long_offset = 1;
        /* pad to 32-bit boundary*/
        pf->buffer_position = (unsigned char *)(((intptr_t)pf->buffer_position + 3) & ~3);
    }

    if (noffset)
        pf->file_offset_offset = (uint32_t)(pf->buffer_position - pf->buffer_start);
    else
        pf->file_offset_offset = 0;

    /* Calculate offset to widths data */
    if (pf->bits_size < MAX_FONTSIZE_FOR_16_BIT_OFFSETS)
        pf->buffer_position += noffset * sizeof(uint16_t);
    else
        pf->buffer_position += noffset * sizeof(uint32_t);

    if (nwidth)
        pf->file_width_offset = (uint32_t)(pf->buffer_position - pf->buffer_start);
    else
        pf->file_width_offset = 0;

    /* Create the cache */
    cache_create(pf);

    return pf;
}


static int find_font_index(const char* path)
{
    int index = 0, handle;

    while (index < MAXFONTS)
    {
        handle = buflib_allocations[index];
        if (handle > 0 && !strcmp(core_get_name(handle), path))
            return index;
        index++;
    }
    return FONT_SYSFIXED;
}

const char* font_filename(int font_id)
{
    if ( font_id < 0 || font_id >= MAXFONTS )
        return NULL;
    int handle = buflib_allocations[font_id];
    if (handle > 0)
        return core_get_name(handle);
    return NULL;
}

static size_t font_glyphs_to_bufsize(struct font *pf, int glyphs)
{
    size_t bufsize;

    /* LRU bytes per glyph */
    bufsize = LRU_SLOT_OVERHEAD + sizeof(struct font_cache_entry) + 
        sizeof( unsigned short);
    /* Image bytes per glyph */
    bufsize += glyph_bytes(pf, pf->maxwidth);
    bufsize *= glyphs;

    return bufsize;
}

static struct font* font_load_header(int fd, struct font *pheader,
                                     struct font *pf, 
                                     uint32_t *nwidth, uint32_t *noffset)
{
    /* Load the header. Readshort() and readlong()              *
     * update buffer_position address as they read              */
    pheader->buffer_start = pheader->buffer_position = (char *)pheader;
    pheader->buffer_size = FONT_HEADER_SIZE;
    pheader->buffer_end = pheader->buffer_start + pheader->buffer_size;

    if (read(fd, pheader, FONT_HEADER_SIZE) != FONT_HEADER_SIZE)
        return NULL;

    /* read magic and version #*/
    if (memcmp(pheader->buffer_position, VERSION, 4) != 0)
        return NULL;

    pheader->buffer_position += 4;

    /* font info*/
    pf->maxwidth      = readshort(pheader);
    pf->height        = readshort(pheader);
    pf->ascent        = readshort(pheader);
    pf->depth         = readshort(pheader);
    pf->firstchar     = readlong(pheader);
    pf->defaultchar   = readlong(pheader);
    pf->size          = readlong(pheader);

    /* get variable font data sizes*/
    /* # words of bitmap_t*/
    pf->bits_size     = readlong(pheader);
    *noffset          = readlong(pheader);
    *nwidth           = readlong(pheader);

    return pf;
}

/* load a font with room for glyphs, limited to bufsize if not zero */
int font_load_ex( const char *path, size_t buf_size, int glyphs )
{
    //printf("\nfont_load_ex(%s, %d, %d)\n", path, buf_size, glyphs);
    int fd = open(path, O_RDONLY|O_BINARY);
    if ( fd < 0 )
        return -1;

    /* load font struct f with file header */
    int file_size = filesize( fd );
    struct font header;
    struct font *pheader = &header;
    struct font f;

    uint32_t nwidth, noffset;     
    if ( !font_load_header( fd, pheader, &f, &nwidth, &noffset )
#if LCD_DEPTH < 16
        || f.depth
#endif  
    )
    {
        close(fd);
        return -1;
    }

    /* examine f and calc buffer size */
    bool cached = false;
    size_t bufsize = buf_size;
    size_t glyph_buf_size = font_glyphs_to_bufsize( &f, glyphs );

    if ( bufsize && glyphs && bufsize > glyph_buf_size)
         bufsize = glyph_buf_size;
    else
    {
        if ( glyphs )
            bufsize = glyph_buf_size;
        else
            bufsize = MAX_FONT_SIZE;
    }
#ifdef FONT_HARD_LIMIT
    if ( bufsize > MAX_FONT_SIZE )
        bufsize = MAX_FONT_SIZE;
#endif
    if ( bufsize < (size_t) file_size )
        cached = true;
    else
        bufsize = file_size;
    
    /* check already loaded */
    int font_id = find_font_index(path);

    if (font_id > FONT_SYSFIXED)
    {
        /* already loaded, no need to reload */
        struct buflib_alloc_data *pd = core_get_data(buflib_allocations[font_id]);
        if (pd->font.buffer_size < bufsize)
        {
            int old_refcount, old_id;
            size_t old_bufsize = pd->font.buffer_size;
            bool failed = false;
            /* reload the font:
             * 1) save of refcont and id
             * 2) force unload (set refcount to 1 to make sure it get unloaded)
             * 3) reload with the larger buffer
             * 4) restore the id and refcount
             */
            old_id = font_id;
            old_refcount = pd->refcount;
            pd->refcount = 1;
            font_unload(font_id);
            font_id = font_load_ex(path, bufsize, glyphs);
            if (font_id < 0)
            {
                failed = true;
                font_id = font_load_ex(path, old_bufsize, 0);
                /* we couldn't even get the old size, this shouldn't happen */
                if ( font_id < 0 )
                    return -1;
            }
            if (old_id != font_id)
            {
                buflib_allocations[old_id] = buflib_allocations[font_id];
                buflib_allocations[font_id] = -1;
                font_id = old_id;
            }
            pd = core_get_data(buflib_allocations[font_id]);
            pd->refcount = old_refcount;
            if(failed)
                /* return error because we didn't satisfy the new buffer size */
                return -1;
        }
        pd->refcount++;
        //printf("reusing handle %d for %s (count: %d)\n", font_id, path, pd->refcount); 
        close(fd);
        return font_id;
    }

    int open_slot = -1;

    for (font_id = FONT_FIRSTUSERFONT; font_id < MAXFONTS; font_id++)
    {
        if (buflib_allocations[ font_id ] < 0)
        {
            open_slot = font_id;
            break;
        }
    }
    if ( open_slot == -1 )
        return -1;
    font_id = open_slot;

    /* allocate mem */    
    int handle = core_alloc_ex( path, 
                     bufsize + sizeof( struct buflib_alloc_data ), 
                     &buflibops );
    if ( handle <= 0 )
    {
        return -1;
    }
    struct buflib_alloc_data *pdata;
    pdata = core_get_data(handle);
    pdata->handle_locks = 1;
    pdata->refcount     = 1;

    /* load and init */
    struct font *pf = pf_from_handle( handle );
    memcpy(pf, &f, sizeof( struct font) );

    pf->fd = fd;
    pf->fd_width = pf->fd_offset = -1;
    pf->handle = handle;

    pf->buffer_start = buffer_from_handle( pf->handle );
    pf->buffer_position = pf->buffer_start + FONT_HEADER_SIZE;
    pf->buffer_size = bufsize;
    pf->buffer_end = pf->buffer_start + bufsize;

    if ( cached )
    {
        if ( ! font_load_cached( pf, nwidth, noffset ) )
        {
            core_free( handle );
            return -1;
        }

        /* trick to get a small cache for each file section   *
         * during glyph_cache_load()                          */
        pf->fd_width  = open( path, O_RDONLY|O_BINARY );
        pf->fd_offset = open( path, O_RDONLY|O_BINARY );

        glyph_cache_load( path, pf );

        /* cached font: pf->fd stays open until the font is unloaded */
        close( pf->fd_width );
        pf->fd_width = -1;
        close( pf->fd_offset );
        pf->fd_offset = -1;
    }
    else
    {           
        lseek( fd, 0, SEEK_SET);
        read(fd, pf->buffer_start, pf->buffer_size);

        close( fd );
        pf->fd = -1;

        if ( ! font_load_in_memory( pf, nwidth, noffset ) )
        {
            core_free( handle );
            return -1;
        }
    }
    buflib_allocations[font_id] = handle;
    //printf("%s -> [%d] -> %d\n", path, font_id, *handle);
    lock_font_handle( handle, false );
    return font_id; /* success!*/
}

int font_load(const char *path)
{
    return font_load_ex(path, MAX_FONT_SIZE, GLYPHS_TO_CACHE);
}

void font_unload(int font_id)
{
    if ( font_id < 0 || font_id >= MAXFONTS )
        return;
    int handle = buflib_allocations[font_id];
    if ( handle < 0 )
        return;
    struct buflib_alloc_data *pdata = core_get_data(handle);
    struct font* pf = &pdata->font;
    pdata->refcount--;
    if (pdata->refcount < 1)
    {
        //printf("freeing id: %d %s\n", font_id, core_get_name(*handle));
        if (pf && pf->fd >= 0)
        {
            glyph_cache_save(font_id);
            close(pf->fd);
        }
        if (handle > 0)
            core_free(handle);
        buflib_allocations[font_id] = -1;

    }
}

void font_unload_all(void)
{
    int i;
    for (i=0; i<MAXFONTS; i++)
    {
        if (buflib_allocations[i] > 0)
        {
            struct buflib_alloc_data *alloc = core_get_data(buflib_allocations[i]);
            alloc->refcount = 1; /* force unload */
            font_unload(i);
        }
    }
}

/*
 * Return a pointer to an incore font structure.
 * If the requested font isn't loaded/compiled-in,
 * decrement the font number and try again.
 */
struct font* font_get(int font)
{
    struct font* pf;
    if (font == FONT_UI)
        font = MAXFONTS-1;
    if (font <= FONT_SYSFIXED)
        return &sysfont;

    while (1) {
        if (buflib_allocations[font] > 0)
        {
            struct buflib_alloc_data *alloc = core_get_data(buflib_allocations[font]);
            pf = &alloc->font;
            if (pf && pf->height)
                return pf;
        }
        if (--font < 0)
            return &sysfont;
    }
}

/*
 * Reads an entry into cache entry
 */
static void
load_cache_entry(struct font_cache_entry* p, void* callback_data)
{
    struct font* pf = callback_data;

    unsigned short char_code = p->_char_code;
    unsigned char tmp[2];
    int fd;

    lock_font_handle(pf->handle, true);
    if (pf->file_width_offset)
    {
        int width_offset = pf->file_width_offset + char_code;
        /* load via different fd to get this file section cached */
        if(pf->fd_width >=0 )
            fd = pf->fd_width;
        else
            fd = pf->fd;
        lseek(fd, width_offset, SEEK_SET);
        read(fd, &(p->width), 1);
    }
    else
    {
        p->width = pf->maxwidth;
    }

    int32_t bitmap_offset = 0;

    if (pf->file_offset_offset)
    {
        int32_t offset = pf->file_offset_offset + char_code * (pf->long_offset ? sizeof(int32_t) : sizeof(int16_t));
        /* load via different fd to get this file section cached */
        if(pf->fd_offset >=0 )
            fd = pf->fd_offset;
        else
            fd = pf->fd;
        lseek(fd, offset, SEEK_SET);
        read (fd, tmp, 2);
        bitmap_offset = tmp[0] | (tmp[1] << 8);
        if (pf->long_offset) {
            read (fd, tmp, 2);
            bitmap_offset |= (tmp[0] << 16) | (tmp[1] << 24);
        }
    }
    else
    {
        bitmap_offset = char_code * glyph_bytes(pf, p->width);
    }

    int32_t file_offset = FONT_HEADER_SIZE + bitmap_offset;
    lseek(pf->fd, file_offset, SEEK_SET);
    int src_bytes = glyph_bytes(pf, p->width);
    read(pf->fd, p->bitmap, src_bytes);

    lock_font_handle(pf->handle, false);
}

/*
 * Converts cbuf into a font cache
 */
static void cache_create(struct font* pf)
{
    /* maximum size of rotated bitmap */
    int bitmap_size = glyph_bytes( pf, pf->maxwidth);
  
    /* Initialise cache */
    font_cache_create(&pf->cache, pf->buffer_start, pf->buffer_size, bitmap_size);
}

/*
 * Returns width of character
 */
int font_get_width(struct font* pf, unsigned short char_code)
{
    /* check input range*/
    if (char_code < pf->firstchar || char_code >= pf->firstchar+pf->size)
        char_code = pf->defaultchar;
    char_code -= pf->firstchar;

    return (pf->fd >= 0 && pf != &sysfont)?
        font_cache_get(&pf->cache,char_code,load_cache_entry,pf)->width:
        pf->width? pf->width[char_code]: pf->maxwidth;
}

const unsigned char* font_get_bits(struct font* pf, unsigned short char_code)
{
    const unsigned char* bits;

    /* check input range*/
    if (char_code < pf->firstchar || char_code >= pf->firstchar+pf->size)
        char_code = pf->defaultchar;
    char_code -= pf->firstchar;

    if (pf->fd >= 0 && pf != &sysfont)
    {
        bits = 
            (unsigned char*)font_cache_get(&pf->cache,char_code,load_cache_entry, pf)->bitmap;
    }
    else
    {
        bits = pf->bits;
        if (pf->offset)
        {
            if (pf->bits_size < MAX_FONTSIZE_FOR_16_BIT_OFFSETS)
                bits += ((uint16_t*)(pf->offset))[char_code];
            else
                bits += ((uint32_t*)(pf->offset))[char_code];
        }
        else
            bits += char_code * glyph_bytes(pf, pf->maxwidth);
    }

    return bits;
}

static void font_path_to_glyph_path( const char *font_path, char *glyph_path)
{
    /* take full file name, cut extension, and add .glyphcache */
    strlcpy(glyph_path, font_path, MAX_PATH);
    glyph_path[strlen(glyph_path)-4] = '\0';
    strcat(glyph_path, ".gc");
}

/* call with NULL to flush */
static void glyph_file_write(void* data)
{
    struct font_cache_entry* p = data;
    struct font* pf = cache_pf;
    unsigned short ch;
    static int buffer_pos = 0;
#define WRITE_BUFFER 256
    static unsigned char buffer[WRITE_BUFFER];

    /* flush buffer & reset */
    if ( data == NULL || buffer_pos >= WRITE_BUFFER)
    {
        write(cache_fd, buffer, buffer_pos);
        buffer_pos = 0;
        if ( data == NULL )
            return;
    }
    if ( p->_char_code == 0xffff )
        return;
    
    ch = p->_char_code + pf->firstchar;
    buffer[buffer_pos] = ch >> 8;
    buffer[buffer_pos+1] = ch & 0xff;
    buffer_pos += 2;
    return;
}

/* save the char codes of the loaded glyphs to a file */
static void glyph_cache_save(int font_id)
{
    int fd;

    if( font_id < 0 )
        return;
    int handle = buflib_allocations[font_id];
    if ( handle < 0 )
        return;

    struct font *pf = pf_from_handle(handle);
    if(pf && pf->fd >= 0)
    {
        char filename[MAX_PATH];
        font_path_to_glyph_path(font_filename(font_id), filename);
        fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        if (fd < 0)
            return;

        cache_pf = pf;
        cache_fd = fd;
        lru_traverse(&cache_pf->cache._lru, glyph_file_write);
        glyph_file_write(NULL);
        if (cache_fd >= 0) 
        {
            close(cache_fd);
            cache_fd = -1;
        }
    }
    return;
}


static int ushortcmp(const void *a, const void *b)
{
    return ((int)(*(unsigned short*)a - *(unsigned short*)b));
}
static void glyph_cache_load(const char *font_path, struct font *pf)
{
#define MAX_SORT 256
    if (pf->fd >= 0) {
        int i, size, fd;
        unsigned char tmp[2];
        unsigned short ch;
        unsigned short glyphs[MAX_SORT];
        unsigned short glyphs_lru_order[MAX_SORT];
        int glyph_file_skip=0, glyph_file_size=0;
        
        int sort_size = pf->cache._capacity;        
        if ( sort_size > MAX_SORT )
             sort_size = MAX_SORT;

        char filename[MAX_PATH];
        font_path_to_glyph_path(font_path, filename);

        fd = open(filename, O_RDONLY|O_BINARY);
#ifdef TRY_DEFAULT_GLYPHCACHE
        /* if font specific file fails, try default */
        if (fd < 0)
            fd = open(GLYPH_CACHE_FILE, O_RDONLY|O_BINARY);
#endif
        if (fd >= 0) {
            /* only read what fits */
            glyph_file_size = filesize( fd );
            if ( glyph_file_size > 2*pf->cache._capacity ) {
                glyph_file_skip = glyph_file_size - 2*pf->cache._capacity;
                lseek( fd, glyph_file_skip, SEEK_SET );
            }

            while(1) {

                for ( size = 0;
                      read( fd, tmp, 2 ) == 2 && size < sort_size;
                      size++ ) 
                {
                    glyphs[size] = (tmp[0] << 8) | tmp[1];
                    glyphs_lru_order[size] = glyphs[size];
                }
                
                /* sort glyphs array to make sector cache happy */
                qsort((void *)glyphs, size, sizeof(unsigned short), 
                      ushortcmp );

                /* load font bitmaps */
                for( i = 0; i < size ; i++ )
                         font_get_bits(pf, glyphs[i]);
                
                /* redo to fix lru order */
                for ( i = 0; i < size ; i++)
                    font_get_bits(pf, glyphs_lru_order[i]);

                if ( size < sort_size )
                    break;
            }

            close(fd);
        } else {
            /* load latin1 chars into cache */
            for ( ch = 32 ; ch < 256  && ch < pf->cache._capacity + 32; ch++ )
                font_get_bits(pf, ch);
        }
    }
    return;
}
#else /* BOOTLOADER */

void font_init(void)
{
}

void font_lock(int font_id, bool lock)
{
    (void)font_id;
    (void)lock;
}

/*
 * Bootloader only supports the built-in sysfont.
 */
struct font* font_get(int font)
{
    (void)font;
    return &sysfont;
}

/*
 * Returns width of character
 */
int font_get_width(struct font* pf, unsigned short char_code)
{
    /* check input range*/
    if (char_code < pf->firstchar || char_code >= pf->firstchar+pf->size)
        char_code = pf->defaultchar;
    char_code -= pf->firstchar;

    return pf->width? pf->width[char_code]: pf->maxwidth;
}

const unsigned char* font_get_bits(struct font* pf, unsigned short char_code)
{
    const unsigned char* bits;

    /* check input range*/
    if (char_code < pf->firstchar || char_code >= pf->firstchar+pf->size)
        char_code = pf->defaultchar;
    char_code -= pf->firstchar;

    /* assume small font with uint16_t offsets*/
    bits = pf->bits + (pf->offset?
            ((uint16_t*)(pf->offset))[char_code]:
            (((pf->height + 7) / 8) * pf->maxwidth * char_code));

    return bits;
}

#endif /* BOOTLOADER */

/*
 * Returns the stringsize of a given string. 
 */
int font_getstringsize(const unsigned char *str, int *w, int *h, int fontnumber)
{
    struct font* pf = font_get(fontnumber);
    unsigned short ch;
    int width = 0;

    font_lock( fontnumber, true );
    for (str = utf8decode(str, &ch); ch != 0 ; str = utf8decode(str, &ch))
    {
        if (is_diacritic(ch, NULL))
            continue;

        /* get proportional width and glyph bits*/
        width += font_get_width(pf,ch);
    }
    if ( w )
        *w = width;
    if ( h )
        *h = pf->height;
    font_lock( fontnumber, false );
    return width;
}

/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
