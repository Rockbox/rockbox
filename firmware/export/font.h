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
#ifndef _FONT_H
#define _FONT_H

#include "inttypes.h"

/*
 * Incore font and image definitions
 */
#include "config.h"

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
#ifndef __PCTOOL__
#include "font_cache.h"
#include "sysfont.h"
#endif


/*
 * Fonts are specified by number, and used for display
 * of menu information as well as mp3 filename data.
 * At system startup, up to MAXFONTS fonts are initialized,
 * either by being compiled-in, or loaded from disk.
 * If the font asked for does not exist, then the
 * system uses the next lower font number.  Font 0
 * must be available at system startup.
 * Fonts are specified in firmware/font.c.
 */
enum {
    FONT_SYSFIXED, /* system fixed pitch font*/
    FONT_UI,       /* system porportional font*/
#ifdef HAVE_REMOTE_LCD
    FONT_UI_REMOTE, /* UI font for remote LCD */
#endif
    SYSTEMFONTCOUNT, /* Number of fonts reserved for the system and ui */
    FONT_FIRSTUSERFONT = 2
};
#define MAXUSERFONTS 8

/* SYSFONT, FONT_UI, FONT_UI_REMOTE + MAXUSERFONTS fonts in skins */
#define MAXFONTS (SYSTEMFONTCOUNT + MAXUSERFONTS)

/*
 * .fnt loadable font file format definition
 *
 * format                     len  description
 * -------------------------  ---- ------------------------------
 * UCHAR version[4]              4   magic number and version bytes
 * USHORT maxwidth               2   font max width in pixels
 * USHORT height                 2   font height in pixels
 * USHORT ascent                 2   font ascent (baseline) in pixels
 * USHORT pad                    2   unused, pad to 32-bit boundary
 * ULONG firstchar               4   first character code in font
 * ULONG defaultchar             4   default character code in font
 * ULONG size                    4   # characters in font
 * ULONG nbits                   4   # bytes imagebits data in file
 * ULONG noffset                 4   # longs offset data in file
 * ULONG nwidth                  4   # bytes width data in file
 * MWIMAGEBITS bits          nbits   image bits variable data
 * [MWIMAGEBITS padded to 16-bit boundary]
 * USHORT offset         noffset*2   offset variable data
 * UCHAR width            nwidth*1   width variable data
 */

/* loadable font magic and version #*/
#define VERSION "RB12"

/* builtin C-based proportional/fixed font structure */
/* based on The Microwindows Project http://microwindows.org */
struct font {
    int          maxwidth;        /* max width in pixels*/
    unsigned int height;          /* height in pixels*/
    int          ascent;          /* ascent (baseline) height*/
    int          firstchar;       /* first character in bitmap*/
    int          size;            /* font size in glyphs*/
    const unsigned char *bits;    /* 8-bit column bitmap data*/
    const void *offset;           /* offsets into bitmap data,
                                     uint16_t if bits_size < 0xFFDB else uint32_t*/
    const unsigned char *width;   /* character widths or NULL if fixed*/
    int          defaultchar;     /* default char (not glyph index)*/
    int32_t      bits_size;       /* # bytes of glyph bits*/
    
    /* file, buffer and cache management */
    int          fd;              /* fd for the font file. >= 0 if cached */
    unsigned char *buffer_start;    /* buffer to store the font in */       
    unsigned char *buffer_position; /* position in the buffer */    
    unsigned char *buffer_end;      /* end of the buffer */
    int          buffer_size;       /* size of the buffer in bytes */
#ifndef __PCTOOL__    
    struct font_cache cache;
    uint32_t file_width_offset;    /* offset to file width data    */
    uint32_t file_offset_offset;   /* offset to file offset data   */
    int long_offset;
#endif    
    
};

/* font routines*/
void font_init(void) INIT_ATTR;
#ifdef HAVE_REMOTE_LCD
/* Load a font into the special remote ui font slot */
int font_load_remoteui(const char* path);
#endif
int font_load(struct font* pf, const char *path);
void font_unload(int font_id);

struct font* font_get(int font);

void font_reset(struct font *pf);
int font_getstringsize(const unsigned char *str, int *w, int *h, int fontnumber);
int font_get_width(struct font* ft, unsigned short ch);
const unsigned char * font_get_bits(struct font* ft, unsigned short ch);
void glyph_cache_save(struct font* pf);

#else /* HAVE_LCD_BITMAP */

#define font_init()
#define font_load(x)

#endif

#endif
