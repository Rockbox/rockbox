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
#include "debug.h"
#include "panic.h"

/* available compiled-in fonts*/
extern MWCFONT font_X5x8;
/*extern MWCFONT font_X6x9; */
/*extern MWCFONT font_courB08; */
/*extern MWCFONT font_timR08; */

/* structure filled in by rbf_load_font*/
static MWCFONT font_UI;

/* system font table, in order of FONT_xxx definition*/
struct corefont sysfonts[MAXFONTS] = {
    { &font_X5x8, NULL 	        }, /* compiled-in FONT_SYSFIXED*/
    { &font_UI,	  "/system.fnt"	}, /* loaded FONT_UI*/
    { NULL,       NULL	        }, /* no FONT_MP3*/
};

void
font_init(void)
{
    struct corefont *cfp;

    for (cfp=sysfonts; cfp < &sysfonts[MAXFONTS]; ++cfp) {
        if (cfp->pf && cfp->diskname) {
            cfp->pf = rbf_load_font(cfp->diskname, cfp->pf);
#if defined(DEBUG) || defined(SIMULATOR)
            if (!cfp->pf)
                DEBUGF("Font load failed: %s\n", cfp->diskname);
#endif
        }
    }
}

/*
 * Return a pointer to an incore font structure.
 * If the requested font isn't loaded/compiled-in,
 * decrement the font number and try again.
 */
PMWCFONT
getfont(int font)
{
    PMWCFONT pf;

    while (1) {
        pf = sysfonts[font].pf;
        if (pf && pf->height)
            return pf;
        if (--font < 0)
            panicf("No font!");
    }
}

/*
 * Return width and height of a given font.
 */
void lcd_getfontsize(int font, int *width, int *height)
{
   PMWCFONT pf = getfont(font);

   *width =  pf->maxwidth;
   *height = pf->height;
}

/*
 * Return width and height of a given font.
 */
//FIXME rename to font_gettextsize, add baseline
int
lcd_getstringsize(unsigned char *str, int font, int *w, int *h)
{
    PMWCFONT pf = getfont(font);
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
    *w = width;
    *h = pf->height;

    return width;
}

/*
 * Take an MWIMAGEBITS bitmap and convert to Rockbox format.
 * Used for converting font glyphs for the time being.
 * Can use for standard X11 and Win32 images as well.
 *
 * Doing it this way keeps fonts in standard formats,
 * as well as keeping Rockbox hw bitmap format.
 */
static void
rotleft(unsigned char *dst, MWIMAGEBITS *src, unsigned int width,
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
    src_words = MWIMAGE_WORDS(width) * height;

    /* clear background*/
    memset(dst, 0, dst_linelen*height);

    for (i=0; i < src_words; i++) {
        MWIMAGEBITS srcmap;	/* current src input bit*/
        MWIMAGEBITS dstmap;	/* current dst output bit*/
        
	/* calc src input bit*/
	srcmap = 1 << (sizeof(MWIMAGEBITS)*8-1);

	/* calc dst output bit*/
        if (i>0 && (i%8==0)) {
            ++dst_col;
            dst_shift = 0;
        }
        dstmap = 1 << dst_shift++;

	/* for each input column...*/
        for(j=0; j < width; j++) {

	    /* calc input bitmask*/
            MWIMAGEBITS bit = srcmap >> j;
            if (bit==0) {
                srcmap = 1 << (sizeof(MWIMAGEBITS)*8-1);
                bit = srcmap >> (j % 16);
            }

	    /* if set in input, set in rotated output*/
	    if (bit & src[i]) {
		/* input column j becomes output row*/
                dst[j*dst_linelen + dst_col] |= dstmap;
            }
	    //printf((bit & src[i])? "*": ".");
        }
        //printf("\n");
    }
}

/*
 * Put a string at specified bit position
 */
//FIXME rename font_putsxy?
void
lcd_putsxy(int x, int y, unsigned char *str, int font)
{
    int ch;
    unsigned char *src;
    PMWCFONT pf = getfont(font);

    while (((ch = *str++) != '\0')) {
	MWIMAGEBITS *bits;
	int width;
	unsigned char outbuf[256];

	/* check input range*/
	if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
		ch = pf->defaultchar;
	ch -= pf->firstchar;

	/* get proportional width and glyph bits*/
	width = pf->width? pf->width[ch]: pf->maxwidth;
        if(x + width > LCD_WIDTH)
            break;
	bits = pf->bits + (pf->offset? pf->offset[ch]: (pf->height * ch));

	/* rotate left for lcd_bitmap function input*/
	rotleft(outbuf, bits, width, pf->height);
	src = outbuf;

        lcd_bitmap (src, x, y, width, pf->height, true);
        x += width;
    }
}
#endif /* HAVE_LCD_BITMAP */

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */
