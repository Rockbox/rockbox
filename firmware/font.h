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
 * Incore font and image definitions
 */
#include "config.h"

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)

/* max static loadable fonts buffer*/
#ifndef MAX_FONT_SIZE
#define MAX_FONT_SIZE	9000	/* max total fontsize allocation*/
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
#define FONT_SYSFIXED	0	/* system fixed pitch font*/
#define FONT_UI		1	/* system porportional font*/
#define FONT_MP3	2	/* font used for mp3 info*/
#define MAXFONTS	3	/* max # fonts*/

/*
 * .fnt (.rbf) loadable font file format definition
 *
 * format                     len	description
 * -------------------------  ----	------------------------------
 * UCHAR version[4]		4	magic number and version bytes
 * UCHAR name[64]	       64	font name, space padded
 * UCHAR copyright[256]	      256	copyright info, space padded
 * USHORT maxwidth		2	font max width in pixels
 * USHORT height		2	font height in pixels
 * USHORT ascent		2	font ascent (baseline) in pixels
 * USHORT pad                   2       unused, pad to 32-bit boundary
 * ULONG firstchar		4	first character code in font
 * ULONG defaultchar		4	default character code in font
 * ULONG size			4	# characters in font
 * ULONG nbits			4	# words imagebits data in file
 * ULONG noffset		4	# longs offset data in file
 * ULONG nwidth			4	# bytes width data in file
 * MWIMAGEBITS bits	  nbits*2	image bits variable data
 * [MWIMAGEBITS padded to 32-bit boundary]
 * ULONG offset         noffset*4	offset variable data
 * UCHAR width		 nwidth*1	width variable data
 */

/* loadable font magic and version #*/
#define VERSION		"RB11"

/* MWIMAGEBITS helper macros*/
#define MWIMAGE_WORDS(x)	(((x)+15)/16)	/* image size in words*/
#define MWIMAGE_BYTES(x)	(MWIMAGE_WORDS(x)*sizeof(MWIMAGEBITS))
#define	MWIMAGE_BITSPERIMAGE	(sizeof(MWIMAGEBITS) * 8)
#define	MWIMAGE_BITVALUE(n)	((MWIMAGEBITS) (((MWIMAGEBITS) 1) << (n)))
#define	MWIMAGE_FIRSTBIT	(MWIMAGE_BITVALUE(MWIMAGE_BITSPERIMAGE - 1))
#define	MWIMAGE_TESTBIT(m)	((m) & MWIMAGE_FIRSTBIT)
#define	MWIMAGE_SHIFTBIT(m)	((MWIMAGEBITS) ((m) << 1))

typedef unsigned short	MWIMAGEBITS;	/* bitmap image unit size*/

/* builtin C-based proportional/fixed font structure */
/* based on The Microwindows Project http://microwindows.org */
typedef struct {
    char *	name;		/* font name*/
    int		maxwidth;	/* max width in pixels*/
    unsigned int height;	/* height in pixels*/
    int		ascent;		/* ascent (baseline) height*/
    int		firstchar;	/* first character in bitmap*/
    int		size;		/* font size in glyphs*/
    MWIMAGEBITS *bits;		/* 16-bit right-padded bitmap data*/
    unsigned long *offset;	/* offsets into bitmap data*/
    unsigned char *width;	/* character widths or NULL if fixed*/
    int		defaultchar;	/* default char (not glyph index)*/
    long	bits_size;	/* # words of MWIMAGEBITS bits*/
#if 0
    char *	facename;	/* facename of font*/
    char *	copyright;	/* copyright info for loadable fonts*/
#endif
} MWCFONT, *PMWCFONT;

/* structure for rockbox startup font selection*/
struct corefont {
    PMWCFONT pf;		/* compiled-in or loaded font*/
    char *diskname;		/* diskname if not compiled-in*/
};

extern struct corefont sysfonts[MAXFONTS];

/* font routines*/
PMWCFONT getfont(int font);
PMWCFONT rbf_load_font(char *path, PMWCFONT pf);

void font_init(void);

#else /* HAVE_LCD_BITMAP */

#define font_init()

#endif

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * vim: et sw=4 ts=8 sts=4 tw=78
 * end:
 */
