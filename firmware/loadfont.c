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
 * Load an rbf font, store in incore format.
 */
#include "config.h"

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)

#include <stdio.h>
#include <string.h>
#include "font.h"
#include "file.h"

#ifndef DEBUGF
#include "debug.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* static buffer allocation structures*/
static unsigned char mbuf[MAX_FONT_SIZE];
static unsigned char *freeptr = mbuf;
typedef unsigned char CFILE;
static CFILE *fileptr;
static CFILE *eofptr;

static int
READSHORT(unsigned short *sp)
{
    unsigned short s;

    s = *fileptr++ & 0xff;
    *sp = (*fileptr++ << 8) | s;
    return (fileptr <= eofptr);
}

static int
READLONG(unsigned long *lp)
{
    unsigned long l;

    l = *fileptr++ & 0xff;
    l |= *fileptr++ << 8;
    l |= *fileptr++ << 16;
    *lp = (*fileptr++ << 24) | l;
    return (fileptr <= eofptr);
}

/* read count bytes*/
static int
READSTR(char *buf, int count)
{
    int n = count;

    while (--n >= 0)
        *buf++ = *fileptr++;
    return (fileptr <= eofptr)? count: 0;
}

/* read totlen bytes, return NUL terminated string*/
/* may write 1 past buf[totlen]; removes blank pad*/
static int
READSTRPAD(char *buf, int totlen)
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
PMWCFONT
rbf_load_font(char *path, PMWCFONT pf)
{
    int fd, filesize;
    unsigned short maxwidth, height, ascent;
    unsigned long firstchar, defaultchar, size;
    unsigned long nbits, noffset, nwidth;
    char version[4+1];
    char copyright[256+1];

    memset(pf, 0, sizeof(MWCFONT));

    /* open and read entire font file*/
    fd = open(path, O_RDONLY|O_BINARY);
    if (fd < 0) {
        DEBUGF("Can't open font: %s\n", path);
        return NULL;
    }
    fileptr = freeptr;
    filesize = read(fd, fileptr, MAX_FONT_SIZE);
    freeptr += filesize;
    eofptr = fileptr + filesize;
    close(fd);
    if (filesize == MAX_FONT_SIZE) {
        DEBUGF("Font %s too large: %d\n", path, filesize);
        return NULL;
    }

    /* read magic and version #*/
    memset(version, 0, sizeof(version));
    if (READSTR(version, 4) != 4)
        return NULL;
    if (strcmp(version, VERSION) != 0)
        return NULL;

    /* internal font name*/
    pf->name = fileptr;
    if (READSTRPAD(pf->name, 64) != 64)
        return NULL;

    /* copyright, not currently stored*/
    if (READSTRPAD(copyright, 256) != 256)
        return NULL;

    /* font info*/
    if (!READSHORT(&maxwidth))
        return NULL;
    pf->maxwidth = maxwidth;
    if (!READSHORT(&height))
        return NULL;
    pf->height = height;
    if (!READSHORT(&ascent))
        return NULL;
    pf->ascent = ascent;
    if (!READLONG(&firstchar))
        return NULL;
    pf->firstchar = firstchar;
    if (!READLONG(&defaultchar))
        return NULL;
    pf->defaultchar = defaultchar;
    if (!READLONG(&size))
        return NULL;
    pf->size = size;

    /* get variable font data sizes*/
    /* # words of MWIMAGEBITS*/
    if (!READLONG(&nbits))
        return NULL;
    pf->bits_size = nbits;

    /* # longs of offset*/
    if (!READLONG(&noffset))
        return NULL;

    /* # bytes of width*/
    if (!READLONG(&nwidth))
        return NULL;

    /* variable font data*/
    pf->bits = (MWIMAGEBITS *)fileptr;
    fileptr += nbits*sizeof(MWIMAGEBITS);

    if (noffset) {
        pf->offset = (unsigned long *)fileptr;
        fileptr += noffset*sizeof(unsigned long);
    } else pf->offset = NULL;

    if (nwidth) {
        pf->width = (unsigned char *)fileptr;
        fileptr += noffset*sizeof(unsigned char);
    } else pf->width = NULL;

    if (fileptr > eofptr)
        return NULL;
    return pf;	/* success!*/
}
#endif /* HAVE_LCD_BITMAP */

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */
