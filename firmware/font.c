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
static struct font* const sysfonts[MAXFONTS] = { &sysfont, &font_ui };

/* static buffer allocation structures */
static unsigned char mbuf[MAX_FONT_SIZE];
static unsigned char *freeptr = mbuf;
static unsigned char *fileptr;
static unsigned char *eofptr;

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

/* read and load font into incore font structure*/
struct font* font_load(const char *path)
{
    int fd, filesize;
    unsigned short maxwidth, height, ascent, pad;
    unsigned long firstchar, defaultchar, size;
    unsigned long i, nbits, noffset, nwidth;
    char version[4+1];
    struct font* pf = &font_ui;

    /* open and read entire font file*/
    fd = open(path, O_RDONLY|O_BINARY);
    if (fd < 0) {
        DEBUGF("Can't open font: %s\n", path);
        return NULL;
    }

    font_reset();

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

    /* # longs of offset*/
    if (!readlong(&noffset))
        return NULL;

    /* # bytes of width*/
    if (!readlong(&nwidth))
        return NULL;

    /* variable font data*/
    pf->bits = (unsigned char *)fileptr;
    fileptr += nbits*sizeof(unsigned char);

    /* pad to 16 bit boundary*/
    fileptr = (unsigned char *)(((long)fileptr + 1) & ~1);

    if (noffset) {
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
    int ch;
    int width = 0;

    while((ch = *str++)) {
        /* check input range*/
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width and glyph bits*/
        width += pf->width? pf->width[ch]: pf->maxwidth;
    }
    if ( w )
        *w = width;
    if ( h )
        *h = pf->height;
    return width;
}


#endif /* HAVE_LCD_BITMAP */

/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
