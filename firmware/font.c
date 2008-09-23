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

/* system font table, in order of FONT_xxx definition */
static struct font* const sysfonts[MAXFONTS] = { &sysfont, &font_ui };

/* static buffer allocation structures */
static unsigned char mbuf[MAX_FONT_SIZE];
static unsigned char *freeptr = mbuf;
static unsigned char *fileptr;
static unsigned char *eofptr;

/* Font cache structures */
static struct font_cache font_cache_ui;
static int fnt_file = -1;           /* >=0 if font is cached   */
static uint32_t file_width_offset;    /* offset to file width data    */
static uint32_t file_offset_offset;   /* offset to file offset data   */
static void cache_create(int maxwidth, int height);
static int long_offset = 0;
static int glyph_file;
/* End Font cache structures */

static void glyph_cache_load(void);

void font_init(void)
{
    memset(&font_ui, 0, sizeof(struct font));
}

/* Check if we have x bytes left in the file buffer */
#define HAVEBYTES(x) (fileptr + (x) <= eofptr)

/* Helper functions to read big-endian unaligned short or long from
   the file buffer.  Bounds-checking must be done in the calling
   function.
 */

static short readshort(void)
{
    unsigned short s;

    s = *fileptr++ & 0xff;
    s |= (*fileptr++ << 8);
    return s;
}

static int32_t readlong(void)
{
    uint32_t l;

    l = *fileptr++ & 0xff;
    l |= *fileptr++ << 8;
    l |= ((uint32_t)(*fileptr++)) << 16;
    l |= ((uint32_t)(*fileptr++)) << 24;
    return l;
}

/* read count bytes*/
static void readstr(char *buf, int count)
{
    while (count--)
        *buf++ = *fileptr++;
}

void font_reset(void)
{
    memset(&font_ui, 0, sizeof(struct font));
}

static struct font*  font_load_header(struct font *pf)
{
    char version[4+1];

    /* Check we have enough data */
    if (!HAVEBYTES(28))
        return NULL;

    /* read magic and version #*/
    memset(version, 0, sizeof(version));
    readstr(version, 4);

    if (strcmp(version, VERSION) != 0)
        return NULL;

    /* font info*/
    pf->maxwidth = readshort();
    pf->height = readshort();
    pf->ascent = readshort();
    fileptr += 2; /* Skip padding */
    pf->firstchar = readlong();
    pf->defaultchar = readlong();
    pf->size = readlong();

    /* get variable font data sizes*/
    /* # words of bitmap_t*/
    pf->bits_size = readlong();

    return pf;
}
/* Load memory font */
static struct font* font_load_in_memory(struct font* pf)
{
    int32_t i, noffset, nwidth;

    if (!HAVEBYTES(4))
        return NULL;

    /* # longs of offset*/
    noffset = readlong();

    /* # bytes of width*/
    nwidth = readlong();

    /* variable font data*/
    pf->bits = (unsigned char *)fileptr;
    fileptr += pf->bits_size*sizeof(unsigned char);

    if ( pf->bits_size < 0xFFDB )
    {
        /* pad to 16-bit boundary */
        fileptr = (unsigned char *)(((intptr_t)fileptr + 1) & ~1);
    }
    else
    {
        /* pad to 32-bit boundary*/
        fileptr = (unsigned char *)(((intptr_t)fileptr + 3) & ~3);
    }

    if (noffset)
    {
        if ( pf->bits_size < 0xFFDB )
        {
            long_offset = 0;
            pf->offset = (unsigned short *)fileptr;

            /* Check we have sufficient buffer */
            if (!HAVEBYTES(noffset * sizeof(short)))
                return NULL;

            for (i=0; i<noffset; ++i)
            {
                ((unsigned short*)(pf->offset))[i] = (unsigned short)readshort();
            }
        }
        else
        {
            long_offset = 1;
            pf->offset = (unsigned short *)fileptr;

            /* Check we have sufficient buffer */
            if (!HAVEBYTES(noffset * sizeof(int32_t)))
                return NULL;

            for (i=0; i<noffset; ++i)
            {
                ((uint32_t*)(pf->offset))[i] = (uint32_t)readlong();
            }
        }
    }
    else
        pf->offset = NULL;

    if (nwidth) {
        pf->width = (unsigned char *)fileptr;
        fileptr += nwidth*sizeof(unsigned char);
    }
    else
        pf->width = NULL;

    if (fileptr > eofptr)
        return NULL;

    return pf;    /* success!*/
}

/* Load cached font */
static struct font* font_load_cached(struct font* pf)
{
    uint32_t noffset, nwidth;
    unsigned char* oldfileptr = fileptr;

    if (!HAVEBYTES(2 * sizeof(int32_t)))
        return NULL;

    /* # longs of offset*/
    noffset = readlong();

    /* # bytes of width*/
    nwidth = readlong();

    /* We are now at the bitmap data, this is fixed at 36.. */
    pf->bits = NULL;

    /* Calculate offset to offset data */
    fileptr += pf->bits_size * sizeof(unsigned char);

    if ( pf->bits_size < 0xFFDB )
    {
        long_offset = 0;
        /* pad to 16-bit boundary */
        fileptr = (unsigned char *)(((intptr_t)fileptr + 1) & ~1);
    }
    else
    {
        long_offset = 1;
        /* pad to 32-bit boundary*/
        fileptr = (unsigned char *)(((intptr_t)fileptr + 3) & ~3);
    }

    if (noffset)
        file_offset_offset = (uint32_t)(fileptr - freeptr);
    else
        file_offset_offset = 0;

    /* Calculate offset to widths data */
    if ( pf->bits_size < 0xFFDB )
        fileptr += noffset * sizeof(unsigned short);
    else
        fileptr += noffset * sizeof(uint32_t);

    if (nwidth)
        file_width_offset = (uint32_t)(fileptr - freeptr);
    else
        file_width_offset = 0;

    fileptr = oldfileptr;

    /* Create the cache */
    cache_create(pf->maxwidth, pf->height);

    return pf;
}

/* read and load font into incore font structure*/
struct font* font_load(const char *path)
{
    int size;
    struct font* pf = &font_ui;

    /* save loaded glyphs */
    glyph_cache_save();

    /* Close font file handle */
    if (fnt_file >= 0)
        close(fnt_file);

    /* open and read entire font file*/
    fnt_file = open(path, O_RDONLY|O_BINARY);

    if (fnt_file < 0) {
        DEBUGF("Can't open font: %s\n", path);
        return NULL;
    }

    /* Check file size */
    size = filesize(fnt_file);

    font_reset();

    /* currently, font loading replaces earlier font allocation*/
    freeptr = (unsigned char *)(((intptr_t)mbuf + 3) & ~3);
    fileptr = freeptr;


    if (size > MAX_FONT_SIZE)
    {
        read(fnt_file, fileptr, FONT_HEADER_SIZE);
        eofptr = fileptr + FONT_HEADER_SIZE;

        if (!font_load_header(pf))
        {
            DEBUGF("Failed font header load");
            return NULL;
        }

        if (!font_load_cached(pf))
        {
            DEBUGF("Failed font cache load");
            return NULL;
        }

        glyph_cache_load();
    }
    else
    {
        read(fnt_file, fileptr, MAX_FONT_SIZE);
        eofptr = fileptr + size;
        close(fnt_file);
        fnt_file = -1;

        if (!font_load_header(pf))
        {
            DEBUGF("Failed font header load");
            return NULL;
        }

        if (!font_load_in_memory(pf))
        {
            DEBUGF("Failed mem load");
            return NULL;
        }
    }

    /* no need for multiple font loads currently*/
    /*freeptr += filesize;*/
    /*freeptr = (unsigned char *)(freeptr + 3) & ~3;*/  /* pad freeptr*/

    return pf; /* success!*/
}

/*
 * Return a pointer to an incore font structure.
 * If the requested font isn't loaded/compiled-in,
 * decrement the font number and try again.
 */
struct font* font_get(int font)
{
    struct font* pf;

    if (font >= MAXFONTS)
        font = 0;

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
 
    if (file_width_offset)
    {
        int width_offset = file_width_offset + char_code;
        lseek(fnt_file, width_offset, SEEK_SET);
        read(fnt_file, &(p->width), 1);
    }
    else
    {
        p->width = pf->maxwidth;
    }
 
    int32_t bitmap_offset = 0;

    if (file_offset_offset)
    {
        int32_t offset = file_offset_offset + char_code * (long_offset ? sizeof(int32_t) : sizeof(short));
        lseek(fnt_file, offset, SEEK_SET);
        read (fnt_file, tmp, 2);
        bitmap_offset = tmp[0] | (tmp[1] << 8);
        if (long_offset) {
            read (fnt_file, tmp, 2);
            bitmap_offset |= (tmp[0] << 16) | (tmp[1] << 24);
        }
    }
    else
    {
        bitmap_offset = ((pf->height + 7) / 8) * p->width * char_code;
    }

    int32_t file_offset = FONT_HEADER_SIZE + bitmap_offset;
    lseek(fnt_file, file_offset, SEEK_SET);

    int src_bytes = p->width * ((pf->height + 7) / 8);
    read(fnt_file, p->bitmap, src_bytes);
}

/*
 * Converts cbuf into a font cache
 */
static void cache_create(int maxwidth, int height)
{
    /* maximum size of rotated bitmap */
    int bitmap_size = maxwidth * ((height + 7) / 8);
    
    /* Initialise cache */
    font_cache_create(&font_cache_ui, mbuf, MAX_FONT_SIZE, bitmap_size);
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

    return (fnt_file >= 0 && pf != &sysfont)?
        font_cache_get(&font_cache_ui,char_code,load_cache_entry,pf)->width:
        pf->width? pf->width[char_code]: pf->maxwidth;
}
 
const unsigned char* font_get_bits(struct font* pf, unsigned short char_code)
{
    const unsigned char* bits;

    /* check input range*/
    if (char_code < pf->firstchar || char_code >= pf->firstchar+pf->size)
        char_code = pf->defaultchar;
    char_code -= pf->firstchar;

    if (fnt_file >= 0 && pf != &sysfont)
    {
        bits =
            (unsigned char*)font_cache_get(&font_cache_ui,char_code,load_cache_entry,pf)->bitmap;
    }
    else
    {
        bits = pf->bits + (pf->offset?
            pf->offset[char_code]:
            (((pf->height + 7) / 8) * pf->maxwidth * char_code));
    }
        
    return bits;
}

static void glyph_file_write(void* data)
{
    struct font_cache_entry* p = data;
    struct font* pf = &font_ui;
    unsigned short ch;
    unsigned char tmp[2];

    ch = p->_char_code + pf->firstchar;

    if (ch != 0xffff && glyph_file >= 0) {
        tmp[0] = ch >> 8;
        tmp[1] = ch & 0xff;
        if (write(glyph_file, tmp, 2) != 2) {
            close(glyph_file);
            glyph_file = -1;
        }
    }
    return;
}

/* save the char codes of the loaded glyphs to a file */
void glyph_cache_save(void)
{

    if (fnt_file >= 0) {
#ifdef WPSEDITOR
        glyph_file = open(GLYPH_CACHE_FILE, O_WRONLY|O_CREAT|O_TRUNC);
#else
        glyph_file = creat(GLYPH_CACHE_FILE);
#endif
        if (glyph_file < 0) return;

        lru_traverse(&font_cache_ui._lru, glyph_file_write);

        if (glyph_file >= 0)
            close(glyph_file);
    }
    return;
}

static void glyph_cache_load(void)
{
    if (fnt_file >= 0) {

        int fd;
        unsigned char tmp[2];
        unsigned short ch;
        struct font* pf = &font_ui;

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

    bits = pf->bits + (pf->offset?
            pf->offset[char_code]:
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
