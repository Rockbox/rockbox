/*
 * writerbf - write an incore font in .rbf format.
 * Must be compiled with -DFONT=font_name and linked
 * with compiled in font.
 *
 * Copyright (c) 2002 by Greg Haerr <greg@censoft.com>
 */
#include <stdio.h>
#include "../firmware/font.h"

extern MWCFONT FONT;
PMWCFONT pf = &FONT;

static int
WRITEBYTE(FILE *fp, unsigned char c)
{
	return putc(c, fp) != EOF;
}

static int
WRITESHORT(FILE *fp, unsigned short s)
{
	putc(s, fp);
	return putc(s>>8, fp) != EOF;
}

static int
WRITELONG(FILE *fp, unsigned long l)
{
	putc(l, fp);
	putc(l>>8, fp);
	putc(l>>16, fp);
	return putc(l>>24, fp) != EOF;
}

static int
WRITESTR(FILE *fp, char *str, int count)
{
	return fwrite(str, 1, count, fp) == count;
}

static int
WRITESTRPAD(FILE *fp, char *str, int totlen)
{
	int ret;
	
	while (*str && totlen > 0)
		if (*str) {
			ret = putc(*str++, fp);
			--totlen;
		}
	while (--totlen >= 0)
		ret = putc(' ', fp);
	return ret;
}

/* write font, < 0 return is error*/
int
rbf_write_font(PMWCFONT pf)
{
	FILE *ofp;
	int i;
	char name[256];

	sprintf(name, "%s.fnt", pf->name);
	ofp = fopen(name, "wb");
	if (!ofp)
		return -1;
	
	/* write magic and version #*/
	WRITESTR(ofp, VERSION, 4);

	/* internal font name*/
	WRITESTRPAD(ofp, pf->name, 64);

	/* copyright - FIXME not converted with bdf2c*/
	WRITESTRPAD(ofp, " ", 256);

	/* font info*/
	WRITESHORT(ofp, pf->maxwidth);
	WRITESHORT(ofp, pf->height);
	WRITESHORT(ofp, pf->ascent);
	WRITELONG(ofp, pf->firstchar);
	WRITELONG(ofp, pf->defaultchar);
	WRITELONG(ofp, pf->size);

	/* variable font data sizes*/
	WRITELONG(ofp, pf->bits_size);		  /* # words of MWIMAGEBITS*/
	WRITELONG(ofp, pf->offset? pf->size: 0);  /* # longs of offset*/
	WRITELONG(ofp, pf->width? pf->size: 0);	  /* # bytes of width*/

	/* variable font data*/
	for (i=0; i<pf->bits_size; ++i)
		WRITESHORT(ofp, pf->bits[i]);
	if (pf->offset)
		for (i=0; i<pf->size; ++i)
			WRITELONG(ofp, pf->offset[i]);
	if (pf->width)
		for (i=0; i<pf->size; ++i)
			WRITEBYTE(ofp, pf->width[i]);
}

int
main(int ac, char **av)
{
	rbf_write_font(pf);
}
