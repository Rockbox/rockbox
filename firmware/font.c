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

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* compiled-in font */
extern struct font sysfont;

/* structure filled in by font_load */
static struct font font_ui;

/* system font table, in order of FONT_xxx definition */
static struct font* sysfonts[MAXFONTS] = { &sysfont, &font_ui };

/* static buffer allocation structures */
static unsigned char mbuf[MAX_FONT_SIZE];
static unsigned char *freeptr = mbuf;
static unsigned char *fileptr;
static unsigned char *eofptr;

static void rotate_font_bits(struct font* pf);
static void rotleft(unsigned char *dst, 
                    bitmap_t *src, 
                    unsigned int width,
                    unsigned int height);

void font_init(void)
{
    rotate_font_bits(&sysfont);
    memset(&font_ui, 0, sizeof(struct font));
}

static int readshort(unsigned short *sp)
{
    unsigned short s;

    s = *fileptr++ & 0xff;
    *sp = (*fileptr++ << 8) | s;
    return (fileptr <= eofptr);
}

static int readlong(unsigned long *lp)
{
    unsigned long l;

    l = *fileptr++ & 0xff;
    l |= *fileptr++ << 8;
    l |= *fileptr++ << 16;
    *lp = (*fileptr++ << 24) | l;
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

/* read totlen bytes, return NUL terminated string*/
/* may write 1 past buf[totlen]; removes blank pad*/
static int readstrpad(char *buf, int totlen)
{
    char *p = buf;
    int n = totlen;

    while (--n >= 0)
        *p++ = *fileptr++;
    if (fileptr > eofptr)
        return 0;

    p = &buf[totlen];
    *p-- = 0;
    while (*p == ' ' && p >= buf)
        *p-- = '\0';
    return totlen;
}

/* read and load font into incore font structure*/
struct font* font_load(char *path)
{
    int fd, filesize;
    unsigned short maxwidth, height, ascent, pad;
    unsigned long firstchar, defaultchar, size;
    unsigned long i, nbits, noffset, nwidth;
    char version[4+1];
    char copyright[256+1];
    struct font* pf = &font_ui;

    /* open and read entire font file*/
    fd = open(path, O_RDONLY|O_BINARY);
    if (fd < 0) {
        DEBUGF("Can't open font: %s\n", path);
        return NULL;
    }

    memset(pf, 0, sizeof(struct font));

    /* currently, font loading replaces earlier font allocation*/
    freeptr = (unsigned char *)(((int)mbuf + 3) & ~3);

    fileptr = freeptr;
    filesize = read(fd, fileptr, MAX_FONT_SIZE);
    eofptr = fileptr + filesize;

    /* no need for multiple font loads currently*/
    /*freeptr += filesize;*/
    /*freeptr = (unsigned char *)(freeptr + 3) & ~3;*/  /* pad freeptr*/

    close(fd);
    if (filesize == MAX_FONT_SIZE) {
        DEBUGF("Font %s too large: %d\n", path, filesize);
        return NULL;
    }

    /* read magic and version #*/
    memset(version, 0, sizeof(version));
    if (readstr(version, 4) != 4)
        return NULL;
    if (strcmp(version, VERSION) != 0)
        return NULL;

    /* internal font name*/
    pf->name = fileptr;
    if (readstrpad(pf->name, 64) != 64)
        return NULL;

    /* copyright, not currently stored*/
    if (readstrpad(copyright, 256) != 256)
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

    /* # longs of offset*/
    if (!readlong(&noffset))
        return NULL;

    /* # bytes of width*/
    if (!readlong(&nwidth))
        return NULL;

    /* variable font data*/
    pf->bits = (bitmap_t *)fileptr;
    for (i=0; i<nbits; ++i)
        if (!readshort(&pf->bits[i]))
            return NULL;
    /* pad to longword boundary*/
    fileptr = (unsigned char *)(((int)fileptr + 3) & ~3);

    if (noffset) {
        pf->offset = (unsigned long *)fileptr;
        for (i=0; i<noffset; ++i)
            if (!readlong(&pf->offset[i]))
                return NULL;
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

    /* one-time rotate font bits to rockbox format*/
    rotate_font_bits(pf);

    return pf;	/* success!*/
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

/* convert font bitmap data inplace to rockbox format*/
static void rotate_font_bits(struct font* pf)
{
    int i;
    unsigned long defaultchar = pf->defaultchar - pf->firstchar;
    bool did_defaultchar = false;
    unsigned char buf[256];

    for (i=0; i<pf->size; ++i) {
        bitmap_t *bits = pf->bits +
            (pf->offset ? pf->offset[i] : (pf->height * i));
        int width = pf->width? pf->width[i]: pf->maxwidth;
        int src_bytes = BITMAP_BYTES(width) * pf->height;

        /*
         * Due to the way the offset map works,
         * non-mapped characters are mapped to the default
         * character, and shouldn't be rotated twice.
         */

        if (pf->offset && pf->offset[i] == defaultchar) {
            if (did_defaultchar)
                continue;
            did_defaultchar = true;
        }

        /* rotate left for lcd_bitmap function input*/
        rotleft(buf, bits, width, pf->height);

        /* copy back into original location*/
        memcpy(bits, buf, src_bytes);
    }
}

/*
 * Take an bitmap_t bitmap and convert to Rockbox format.
 * Used for converting font glyphs for the time being.
 * Can use for standard X11 and Win32 images as well.
 *
 * Doing it this way keeps fonts in standard formats,
 * as well as keeping Rockbox hw bitmap format.
 */
static void rotleft(unsigned char *dst, bitmap_t *src, unsigned int width,
                    unsigned int height)
{
    unsigned int i,j;
    unsigned int dst_col = 0;		/* destination column*/
    unsigned int dst_shift = 0;		/* destination shift amount*/
    unsigned int dst_linelen;		/* # bytes per output row*/
    unsigned int src_words;		/* # words of input image*/
    
    /* calc bytes per output row*/
    dst_linelen = (height-1)/8+1;

    /* calc words of input image*/
    src_words = BITMAP_WORDS(width) * height;

    /* clear background*/
    memset(dst, 0, dst_linelen*width);

    for (i=0; i < src_words; i++) {
        bitmap_t srcmap;	/* current src input bit*/
        bitmap_t dstmap;	/* current dst output bit*/
        
        /* calc src input bit*/
        srcmap = 1 << (sizeof(bitmap_t)*8-1);

        /* calc dst output bit*/
        if (i>0 && (i%8==0)) {
            ++dst_col;
            dst_shift = 0;
        }
        dstmap = 1 << dst_shift++;

        /* for each input column...*/
        for(j=0; j < width; j++) {

            /* calc input bitmask*/
            bitmap_t bit = srcmap >> j;
            if (bit==0) {
                srcmap = 1 << (sizeof(bitmap_t)*8-1);
                bit = srcmap >> (j % 16);
            }

            /* if set in input, set in rotated output*/
            if (bit & src[i]) {
                /* input column j becomes output row*/
                dst[j*dst_linelen + dst_col] |= dstmap;
            }
            /*debugf((bit & src[i])? "*": ".");*/
        }
        /*debugf("\n");*/
    }
}
#endif /* HAVE_LCD_BITMAP */

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * vim: et sw=4 ts=8 sts=4 tw=78
 * end:
 */
