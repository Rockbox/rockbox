/*
 * Load an rbf font, store in incore format and display - or -
 * Read an incore font and display it.
 *
 * If FONT defined, just link in FONT and display it
 * otherwise, load av[1] and display it
 *
 * Copyright (c) 2002 by Greg Haerr <greg@censoft.com>
 */
#include <stdio.h>

/* this should go in a library...*/
#define DEBUGF printf
#include "../firmware/loadfont.c"

#ifdef FONT
extern MWCFONT FONT;
PMWCFONT pf = &FONT;
#endif

/* printf display an incore font*/
void
dispfont(PMWCFONT pf)
{
	int i;

	printf("Font: '%s' %dx%d ", pf->name, pf->maxwidth, pf->height);
	printf("0x%02x-0x%02x (size %d)\n", pf->firstchar,
		pf->firstchar+pf->size, pf->size);
	printf("\tDefault char 0x%02x ", pf->defaultchar);
	printf("(%s width)\n", pf->width? "proportional": "fixed");
	
	for (i=0; i<pf->size; ++i) {
 		int width = pf->width ? pf->width[i] : pf->maxwidth;
		int height = pf->height;
		int x;
		int bitcount = 0;
		MWIMAGEBITS *bits = pf->bits + (pf->offset? pf->offset[i]: (height * i));
		MWIMAGEBITS bitvalue;

		printf("\nCharacter 0x%02x (width %d)\n", i+pf->firstchar, width);
		printf("+");
		for (x=0; x<width; ++x) printf("-");
		printf("+\n");

		x = 0;
		while (height > 0) {
			if (x == 0) printf("|");

			if (bitcount <= 0) {
				bitcount = MWIMAGE_BITSPERIMAGE;
				bitvalue = *bits++;
			}

			printf( MWIMAGE_TESTBIT(bitvalue)? "*": " ");

			bitvalue = MWIMAGE_SHIFTBIT(bitvalue);
			--bitcount;
			if (++x == width) {
				printf("|\n");
				--height;
				x = 0;
				bitcount = 0;
			}
		}
		printf("+");
		for (x=0; x<width; ++x) printf("-");
		printf("+\n");
	}
}

int
main(int ac, char **av)
{
	PMWCFONT pf;
	MWCFONT font;
#ifdef FONT
	/* if FONT defined, just display linked-in font*/
	extern MWCFONT FONT;
	pf = &FONT;
#else
	if (ac != 2) {
		printf("usage: loadrbf font.rbf\n");
		exit(1);
	}

	pf = rbf_load_font(av[1], &font);
	if (!pf) {
		printf("loadrbf: read error: %s\n", av[1]);
		exit(1);
	}
#endif
	dispfont(pf);
	return 0;
}
