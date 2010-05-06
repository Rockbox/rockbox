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
#include "inttypes.h"
#include "lcd.h"
#include "font.h"
#include "file.h"
#include "debug.h"
#include "panic.h"
#include "rbunicode.h"
#include "diacritic.h"

#define MAX_FONTSIZE_FOR_16_BIT_OFFSETS 0xFFDB

/* max static loadable font buffer size */
#ifndef MAX_FONT_SIZE
#if LCD_HEIGHT > 64
#if MEM > 2
#define MAX_FONT_SIZE   60000
#else
#define MAX_FONT_SIZE   10000
#endif
#else
#define MAX_FONT_SIZE   4000
#endif
#endif

#ifndef FONT_HEADER_SIZE
#define FONT_HEADER_SIZE 36
#endif

#define GLYPH_CACHE_FILE ROCKBOX_DIR"/.glyphcache"

#ifndef BOOTLOADER
/* Font cache includes */
#include "font_cache.h"
#include "lru.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* compiled-in font */
extern struct font sysfont;

#ifndef BOOTLOADER

/* structure filled in by font_load */
static struct font font_ui;
/* static buffer allocation structures */
static unsigned char main_buf[MAX_FONT_SIZE];
#ifdef HAVE_REMOTE_LCD
#define REMOTE_FONT_SIZE 10000
static struct font remote_font_ui;
static unsigned char remote_buf[REMOTE_FONT_SIZE];
#endif

/* system font table, in order of FONT_xxx definition */
static struct font* sysfonts[MAXFONTS] = { &sysfont, &font_ui, NULL};


/* Font cache structures */
static void cache_create(struct font* pf, int maxwidth, int height);
static void glyph_cache_load(struct font* pf);
/* End Font cache structures */

void font_init(void)
{
    int i = SYSTEMFONTCOUNT;
    while (i<MAXFONTS)
        sysfonts[i++] = NULL;
    font_reset(NULL);
#ifdef HAVE_REMOTE_LCD
    font_reset(&remote_font_ui);
#endif
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

void font_reset(struct font *pf)
{
    unsigned char* buffer = NULL;
    size_t buf_size = 0;
    if (pf == NULL)
        pf = &font_ui;
    else
    {
        buffer = pf->buffer_start;
        buf_size = pf->buffer_size;
    }
    memset(pf, 0, sizeof(struct font));
    pf->fd = -1;
    if (buffer)
    {
        pf->buffer_start = buffer;
        pf->buffer_size = buf_size;
    }
}

static struct font* font_load_header(struct font *pf)
{
    /* Check we have enough data */
    if (!HAVEBYTES(28))
        return NULL;

    /* read magic and version #*/
    if (memcmp(pf->buffer_position, VERSION, 4) != 0)
        return NULL;

    pf->buffer_position += 4;

    /* font info*/
    pf->maxwidth = readshort(pf);
    pf->height = readshort(pf);
    pf->ascent = readshort(pf);
    pf->buffer_position += 2; /* Skip padding */
    pf->firstchar = readlong(pf);
    pf->defaultchar = readlong(pf);
    pf->size = readlong(pf);

    /* get variable font data sizes*/
    /* # words of bitmap_t*/
    pf->bits_size = readlong(pf);

    return pf;
}
/* Load memory font */
static struct font* font_load_in_memory(struct font* pf)
{
    int32_t i, noffset, nwidth;

    if (!HAVEBYTES(4))
        return NULL;

    /* # longs of offset*/
    noffset = readlong(pf);

    /* # bytes of width*/
    nwidth = readlong(pf);

    /* variable font data*/
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
static struct font* font_load_cached(struct font* pf)
{
    uint32_t noffset, nwidth;
    unsigned char* oldfileptr = pf->buffer_position;

    if (!HAVEBYTES(2 * sizeof(int32_t)))
        return NULL;

    /* # longs of offset*/
    noffset = readlong(pf);

    /* # bytes of width*/
    nwidth = readlong(pf);

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

    pf->buffer_position = oldfileptr;

    /* Create the cache */
    cache_create(pf, pf->maxwidth, pf->height);

    return pf;
}

static bool internal_load_font(struct font* pf, const char *path,
                               char *buf, size_t buf_size)
{
    int size;
    
    /* save loaded glyphs */
    glyph_cache_save(pf);
    /* Close font file handle */
    if (pf->fd >= 0)
        close(pf->fd);

    font_reset(pf);

    /* open and read entire font file*/
    pf->fd = open(path, O_RDONLY|O_BINARY);

    if (pf->fd < 0) {
        DEBUGF("Can't open font: %s\n", path);
        return false;
    }

    /* Check file size */
    size = filesize(pf->fd);
    pf->buffer_start = buf;
    pf->buffer_size = buf_size;
    
    pf->buffer_position = buf;
    
    if (size > pf->buffer_size)
    {
        read(pf->fd, pf->buffer_position, FONT_HEADER_SIZE);
        pf->buffer_end = pf->buffer_position + FONT_HEADER_SIZE;

        if (!font_load_header(pf))
        {
            DEBUGF("Failed font header load");
            return false;
        }

        if (!font_load_cached(pf))
        {
            DEBUGF("Failed font cache load");
            return false;
        }

        glyph_cache_load(pf);
    }
    else
    {
        read(pf->fd, pf->buffer_position, pf->buffer_size);
        pf->buffer_end = pf->buffer_position + size;
        close(pf->fd);
        pf->fd = -1;

        if (!font_load_header(pf))
        {
            DEBUGF("Failed font header load");
            return false;
        }

        if (!font_load_in_memory(pf))
        {
            DEBUGF("Failed mem load");
            return false;
        }
    }
    return true;
}

#ifdef HAVE_REMOTE_LCD
/* Load a font into the special remote ui font slot */
int font_load_remoteui(const char* path)
{
    struct font* pf = &remote_font_ui;
    if (!path)
    {
        if (sysfonts[FONT_UI_REMOTE] && sysfonts[FONT_UI_REMOTE] != sysfonts[FONT_UI])
            font_unload(FONT_UI_REMOTE);
        sysfonts[FONT_UI_REMOTE] = NULL;
        return FONT_UI;
    }
    if (!internal_load_font(pf, path, remote_buf, REMOTE_FONT_SIZE))
    {
        sysfonts[FONT_UI_REMOTE] = NULL;
        return -1;
    }
    
    sysfonts[FONT_UI_REMOTE] = pf;
    return FONT_UI_REMOTE;
}
#endif

/* read and load font into incore font structure,
 * returns the font number on success, -1 on failure */
int font_load(struct font* pf, const char *path)
{
    int font_id = -1;
    char *buffer;
    size_t buffer_size;
    if (pf == NULL)
    {
        pf = &font_ui;
        font_id = FONT_UI;
    }
    else
    {
        for (font_id = SYSTEMFONTCOUNT; font_id < MAXFONTS; font_id++)
        {
            if (sysfonts[font_id] == NULL)
                break;
        }
        if (font_id == MAXFONTS)
            return -1; /* too many fonts */
    }
    
    if (font_id == FONT_UI)
    {
        /* currently, font loading replaces earlier font allocation*/
        buffer = (unsigned char *)(((intptr_t)main_buf + 3) & ~3);
        buffer_size = MAX_FONT_SIZE;
    }
    else
    {
        buffer = pf->buffer_start;
        buffer_size = pf->buffer_size;
    }
    
    if (!internal_load_font(pf, path, buffer, buffer_size))
        return -1;
        
    sysfonts[font_id] = pf;
    return font_id; /* success!*/
}

void font_unload(int font_id)
{
    struct font* pf = sysfonts[font_id];
    if (font_id >= SYSTEMFONTCOUNT && pf)
    {
        if (pf->fd >= 0)
            close(pf->fd);
        sysfonts[font_id] = NULL;
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

    while (1) {
        pf = sysfonts[font];
        if (pf && pf->height)
            return pf;
        if (--font < 0)
            panicf("No font!");
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

    if (pf->file_width_offset)
    {
        int width_offset = pf->file_width_offset + char_code;
        lseek(pf->fd, width_offset, SEEK_SET);
        read(pf->fd, &(p->width), 1);
    }
    else
    {
        p->width = pf->maxwidth;
    }

    int32_t bitmap_offset = 0;

    if (pf->file_offset_offset)
    {
        int32_t offset = pf->file_offset_offset + char_code * (pf->long_offset ? sizeof(int32_t) : sizeof(int16_t));
        lseek(pf->fd, offset, SEEK_SET);
        read (pf->fd, tmp, 2);
        bitmap_offset = tmp[0] | (tmp[1] << 8);
        if (pf->long_offset) {
            read (pf->fd, tmp, 2);
            bitmap_offset |= (tmp[0] << 16) | (tmp[1] << 24);
        }
    }
    else
    {
        bitmap_offset = ((pf->height + 7) / 8) * p->width * char_code;
    }

    int32_t file_offset = FONT_HEADER_SIZE + bitmap_offset;
    lseek(pf->fd, file_offset, SEEK_SET);

    int src_bytes = p->width * ((pf->height + 7) / 8);
    read(pf->fd, p->bitmap, src_bytes);
}

/*
 * Converts cbuf into a font cache
 */
static void cache_create(struct font* pf, int maxwidth, int height)
{
    /* maximum size of rotated bitmap */
    int bitmap_size = maxwidth * ((height + 7) / 8);
    
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
            (unsigned char*)font_cache_get(&pf->cache,char_code,load_cache_entry,pf)->bitmap;
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
            bits += ((pf->height + 7) / 8) * pf->maxwidth * char_code;
    }

    return bits;
}
static int cache_fd;
static void glyph_file_write(void* data)
{
    struct font_cache_entry* p = data;
    struct font* pf = &font_ui;
    unsigned short ch;
    unsigned char tmp[2];

    ch = p->_char_code + pf->firstchar;

    if (ch != 0xffff && cache_fd >= 0) {
        tmp[0] = ch >> 8;
        tmp[1] = ch & 0xff;
        if (write(cache_fd, tmp, 2) != 2) {
            close(cache_fd);
            cache_fd = -1;
        }
    }
    return;
}

/* save the char codes of the loaded glyphs to a file */
void glyph_cache_save(struct font* pf)
{
    if (!pf)
        pf = &font_ui;
    if (pf->fd >= 0 && pf == &font_ui) 
    {
#ifdef WPSEDITOR
        cache_fd = open(GLYPH_CACHE_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
#else
        cache_fd = creat(GLYPH_CACHE_FILE, 0666);
#endif
        if (cache_fd < 0) return;

        lru_traverse(&pf->cache._lru, glyph_file_write);

        if (cache_fd >= 0)
        {
            close(cache_fd);
            cache_fd = -1;
        }
    }
    return;
}

static void glyph_cache_load(struct font* pf)
{
    if (pf->fd >= 0) {
        int fd;
        unsigned char tmp[2];
        unsigned short ch;

        fd = open(GLYPH_CACHE_FILE, O_RDONLY|O_BINARY);

        if (fd >= 0) {

            while (read(fd, tmp, 2) == 2) {
                ch = (tmp[0] << 8) | tmp[1];
                font_get_bits(pf, ch);
            }

            close(fd);
        } else {
            /* load latin1 chars into cache */
            ch = 256;
            while (ch-- > 32)
                font_get_bits(pf, ch);
        }
    }
    return;
}
#else /* BOOTLOADER */

void font_init(void)
{
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
    return width;
}

/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
