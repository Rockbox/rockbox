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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)

#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "font.h"
#include "file.h"
#include "debug.h"
#include "panic.h"
#include "rbunicode.h"
/* Font cache includes */
#include "font_cache.h"
#include "lru.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* compiled-in font */
extern struct font sysfont;

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
unsigned long file_width_offset;    /* offset to file width data    */
unsigned long file_offset_offset;   /* offset to file offset data   */
static void cache_create(int maxwidth, int height);
static int long_offset = 0;
static int glyph_file;
/* End Font cache structures */

void font_init(void)
{
    memset(&font_ui, 0, sizeof(struct font));
}

static int readshort(unsigned short *sp)
{
    unsigned short s;

    s = *fileptr++ & 0xff;
    *sp = (*fileptr++ << 8) | s;
    return (fileptr <= eofptr);
}

static long readlong(unsigned long *lp)
{
    unsigned long l;

    l = *fileptr++ & 0xff;
    l |= *fileptr++ << 8;
    l |= ((unsigned long)(*fileptr++)) << 16;
    l |= ((unsigned long)(*fileptr++)) << 24;
    *lp = l;
    return (fileptr <= eofptr);
}

/* read count bytes*/
static int readstr(char *buf, int count)
{
    int n = count;

    while (--n >= 0)
        *buf++ = *fileptr++;
    return (fileptr <= eofptr)? count: 0;
}

void font_reset(void)
{
    memset(&font_ui, 0, sizeof(struct font));
}

static struct font*  font_load_header(struct font *pf)
{
    char version[4+1];
    unsigned short maxwidth, height, ascent, pad;
    unsigned long firstchar, defaultchar, size;
    unsigned long nbits;

    /* read magic and version #*/
    memset(version, 0, sizeof(version));
    if (readstr(version, 4) != 4)
        return NULL;
    if (strcmp(version, VERSION) != 0)
        return NULL;

    /* font info*/
    if (!readshort(&maxwidth))
        return NULL;
    pf->maxwidth = maxwidth;
    if (!readshort(&height))
        return NULL;
    pf->height = height;
    if (!readshort(&ascent))
        return NULL;
    pf->ascent = ascent;
    if (!readshort(&pad))
        return NULL;
    if (!readlong(&firstchar))
        return NULL;
    pf->firstchar = firstchar;
    if (!readlong(&defaultchar))
        return NULL;
    pf->defaultchar = defaultchar;
    if (!readlong(&size))
        return NULL;
    pf->size = size;

    /* get variable font data sizes*/
    /* # words of bitmap_t*/
    if (!readlong(&nbits))
        return NULL;
    pf->bits_size = nbits;

    return pf;
}
/* Load memory font */
struct font* font_load_in_memory(struct font* pf)
{
    long i, noffset, nwidth;

    /* # longs of offset*/
    if (!readlong(&noffset))
        return NULL;

    /* # bytes of width*/
    if (!readlong(&nwidth))
        return NULL;

    /* variable font data*/
    pf->bits = (unsigned char *)fileptr;
    fileptr += pf->bits_size*sizeof(unsigned char);

    if ( pf->bits_size < 0xFFDB )
    {
        /* pad to 16-bit boundary */
        fileptr = (unsigned char *)(((long)fileptr + 1) & ~1);
    }
    else
    {
        /* pad to 32-bit boundary*/
        fileptr = (unsigned char *)(((long)fileptr + 3) & ~3);
    }

    if (noffset)
    {
        if ( pf->bits_size < 0xFFDB )
        {
            long_offset = 0;
            pf->offset = (unsigned short *)fileptr;
            for (i=0; i<noffset; ++i)
            {
                unsigned short offset;
                if (!readshort(&offset))
                    return NULL;
                ((unsigned short*)(pf->offset))[i] = (unsigned short)offset;
            }
        }
        else
        {
            long_offset = 1;
            pf->offset = (unsigned short *)fileptr;
            for (i=0; i<noffset; ++i)
            {
                unsigned long offset;
                if (!readlong(&offset))
                    return NULL;
                ((unsigned long*)(pf->offset))[i] = (unsigned long)offset;
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
struct font* font_load_cached(struct font* pf)
{
    unsigned long noffset, nwidth;
    unsigned char* oldfileptr = fileptr;

    /* # longs of offset*/
    if (!readlong(&noffset))
        return NULL;

    /* # bytes of width*/
    if (!readlong(&nwidth))
        return NULL;

    /* We are now at the bitmap data, this is fixed at 36.. */
    pf->bits = NULL;

    /* Calculate offset to offset data */
    fileptr += pf->bits_size * sizeof(unsigned char);

    if ( pf->bits_size < 0xFFDB )
    {
        long_offset = 0;
        /* pad to 16-bit boundary */
        fileptr = (unsigned char *)(((long)fileptr + 1) & ~1);
    }
    else
    {
        long_offset = 1;
        /* pad to 32-bit boundary*/
        fileptr = (unsigned char *)(((long)fileptr + 3) & ~3);
    }

    if (noffset)
        file_offset_offset = (unsigned long)(fileptr - freeptr);
    else
        file_offset_offset = 0;

    /* Calculate offset to widths data */
    if ( pf->bits_size < 0xFFDB )
        fileptr += noffset * sizeof(unsigned short);
    else
        fileptr += noffset * sizeof(unsigned long);

    if (nwidth)
        file_width_offset = (unsigned long)(fileptr - freeptr);
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
    int filesize;
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
    filesize = lseek(fnt_file, 0, SEEK_END);
    lseek(fnt_file, 0, SEEK_SET);

    font_reset();

    /* currently, font loading replaces earlier font allocation*/
    freeptr = (unsigned char *)(((int)mbuf + 3) & ~3);
    fileptr = freeptr;


    if (filesize > MAX_FONT_SIZE)
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
        eofptr = fileptr + filesize;
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
 * Returns the stringsize of a given string. 
 */
int font_getstringsize(const unsigned char *str, int *w, int *h, int fontnumber)
{
    struct font* pf = font_get(fontnumber);
    unsigned short ch;
    int width = 0;

    for (str = utf8decode(str, &ch); ch != 0 ; str = utf8decode(str, &ch))
    {
        /* check input range*/
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width and glyph bits*/
        width += font_get_width(pf,ch);
    }
    if ( w )
        *w = width;
    if ( h )
        *h = pf->height;
    return width;
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
 
    long bitmap_offset = 0;

    if (file_offset_offset)
    {
        long offset = file_offset_offset + char_code * (long_offset ? sizeof(long) : sizeof(short));
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

    long file_offset = FONT_HEADER_SIZE + bitmap_offset;
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
    return (fnt_file >= 0 && pf != &sysfont)?
        font_cache_get(&font_cache_ui,char_code,load_cache_entry,pf)->width:
        pf->width? pf->width[char_code]: pf->maxwidth;
}
 
const unsigned char* font_get_bits(struct font* pf, unsigned short char_code)
{
    const unsigned char* bits;
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

void glyph_file_write(void* data)
{
    struct font_cache_entry* p = data;
    unsigned char tmp[2];

    if (p->_char_code != 0xffff && glyph_file >= 0) {
        tmp[0] = p->_char_code >> 8;
        tmp[1] = p->_char_code & 0xff;
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

        glyph_file = creat(GLYPH_CACHE_FILE, 0);

        if (glyph_file < 0) return;

        lru_traverse(&font_cache_ui._lru, glyph_file_write);

        if (glyph_file >= 0)
            close(glyph_file);
    }
    return;
}

void glyph_cache_load(void)
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
            ch = 255 - pf->firstchar;
            while (ch--)
                font_get_bits(pf, ch);
        }
    }
    return;
}

#endif /* HAVE_LCD_BITMAP */

/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
