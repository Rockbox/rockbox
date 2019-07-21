/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_blit.h"
#include "SDL_sysvideo.h"
#include "SDL_endian.h"

/* Functions to blit from 8-bit surfaces to other surfaces */

static void Blit1to1(SDL_BlitInfo *info)
{
#ifndef USE_DUFFS_LOOP
	int c;
#endif
	int width, height;
	Uint8 *src, *map, *dst;
	int srcskip, dstskip;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = info->d_pixels;
	dstskip = info->d_skip;
	map = info->table;

	while ( height-- ) {
#ifdef USE_DUFFS_LOOP
		DUFFS_LOOP(
			{
			  *dst = map[*src];
			}
			dst++;
			src++;
		, width);
#else
		for ( c=width; c; --c ) {
		        *dst = map[*src];
			dst++;
			src++;
		}
#endif
		src += srcskip;
		dst += dstskip;
	}
}
/* This is now endian dependent */
#if ( SDL_BYTEORDER == SDL_LIL_ENDIAN )
#define HI	1
#define LO	0
#else /* ( SDL_BYTEORDER == SDL_BIG_ENDIAN ) */
#define HI	0
#define LO	1
#endif
static void Blit1to2(SDL_BlitInfo *info)
{
//#ifndef USE_DUFFS_LOOP
	int c;
//#endif
	int width, height;
	Uint8 *src, *dst;
	Uint16 *map;
	int srcskip, dstskip;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = info->d_pixels;
	dstskip = info->d_skip;
	map = (Uint16 *)info->table;

//#ifdef USE_DUFFS_LOOP
	while ( height-- ) {
		DUFFS_LOOP(
		{
			*(Uint16 *)dst = map[*src++];
			dst += 2;
		},
		width);
		src += srcskip;
		dst += dstskip;
	}
//#else
	/* Memory align at 4-byte boundary, if necessary */
	if ( (long)dst & 0x03 ) {
		/* Don't do anything if width is 0 */
		if ( width == 0 ) {
			return;
		}
		--width;

		while ( height-- ) {
			/* Perform copy alignment */
			*(Uint16 *)dst = map[*src++];
			dst += 2;

			/* Copy in 4 pixel chunks */
			for ( c=width/4; c; --c ) {
				*(Uint32 *)dst =
					(map[src[HI]]<<16)|(map[src[LO]]);
				src += 2;
				dst += 4;
				*(Uint32 *)dst =
					(map[src[HI]]<<16)|(map[src[LO]]);
				src += 2;
				dst += 4;
			}
			/* Get any leftovers */
			switch (width & 3) {
				case 3:
					*(Uint16 *)dst = map[*src++];
					dst += 2;
				case 2:
					*(Uint32 *)dst =
					  (map[src[HI]]<<16)|(map[src[LO]]);
					src += 2;
					dst += 4;
					break;
				case 1:
					*(Uint16 *)dst = map[*src++];
					dst += 2;
					break;
			}
			src += srcskip;
			dst += dstskip;
		}
	} else {
#if defined(__ARM_ARCH_5TEJ__)
            if(!((long)dst & 0x07) && !(width & 0x07) && !((long)src & 0x07)) // 8-aligned and multiple of 8 width
            {
                const uint32_t mask = 0x1fe; /* 111111110 */
                while(height--) {
                    asm volatile(
                        "sub r6, %[width], #8                         \n" /* x = width - 8 */
                        "mov r8, r6, lsl #1                           \n" /* y = 2 * x */
                        "1:                                           \n"
                        "ldrd r0, r1, [%[src], r6]                    \n" /* load 8 bytes; r1=src[7,6,5,4], r0=src[3,2,1,0] */

                        /* process first 4 bytes */
                        "and r2, %[mask], r0, lsl #1                  \n" /* extract byte 0 from r0, shifted left 1 */
                        "ldrh r3, [%[map], r2]                        \n" /* r3 = map[src[0]] */
                        "and r2, %[mask], lsr #7                      \n" /* byte 1 */
                        "ldrh r4, [%[map], r2]                        \n" /* r4 = map[src[1]] */
                        "orr r4, r3, r4, lsl #16                      \n" /* r3 = [r4:r3], TRASH r2, r3 */

                        "and r2, r0, lsr #15                          \n" /* byte 2 */
                        "ldrh r3, [%[map], r2]                        \n" /* r3 = map[src[2]] */
                        "and r2, r0, lsr #23                          \n" /* byte 3, TRASH r0 */
                        "ldrh r5, [%[map], r0]                        \n" /* r5 = map[src[3]] */
                        "orr r5, r3, r5, lsl #16                      \n" /* r5 = [r5, r3] */

                        "strd r4, r5, [%[dst], r8]                    \n" /* dst[0..7] = r4, r5 (write 4 pixels) */
                        "sub r6, r6, #4                               \n" /* advance to next 4 */
                        "sub r8, r8, #8                               \n"

                        /* process next 4 bytes */
                        "and r2, %[mask], r1, lsl #1                  \n" /* extract byte 0 from r1, shifted left 1 */
                        "ldrh r3, [%[map], r2]                        \n" /* r3 = map[src[0]] */
                        "and r2, %[mask], lsr #7                      \n" /* byte 1 */
                        "ldrh r4, [%[map], r2]                        \n" /* r4 = map[src[1]] */
                        "orr r4, r3, r4, lsl #16                      \n" /* r3 = [r4:r3], trash r2, r3 */

                        "and r2, r1, lsr #15                          \n" /* byte 2 */
                        "ldrh r3, [%[map], r2]                        \n" /* r3 = map[src[2]] */
                        "and r2, r1, lsr #23                          \n" /* byte 3, TRASH r1 */
                        "ldrh r5, [%[map], r1]                        \n" /* r5 = map[src[3]] */
                        "orr r5, r3, r5, lsl #16                      \n" /* r5 = [r5, r3] */

                        "strd r4, r5, [%[dst], r8]                    \n" /* dst[0..3] = r3, r4 */

                        "sub  r8, r8, #8                              \n"
                        "subs r6, r6, #4                              \n"
                        //"bgt 1b                                       \n" /* loop if > 0 */
                        : /* output */
                          [dst] "&=r" (dst)
                        : /* input */
                          [src] "r" (src),
                          [map] "r" (map),
                          [width] "r" (width),
                          [mask] "r" (mask)
                        : /* clobber */
                          "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r8");
                    src += srcskip;
                    dst += dstskip;
                }
            }
#else
            else
#error a
		while ( height-- ) {
			/* Copy in 4 pixel chunks */
			for ( c=width/4; c; --c ) {
				*(Uint32 *)dst =
					(map[src[HI]]<<16)|(map[src[LO]]);
				src += 2;
				dst += 4;
				*(Uint32 *)dst =
					(map[src[HI]]<<16)|(map[src[LO]]);
				src += 2;
				dst += 4;
			}
			/* Get any leftovers */
			switch (width & 3) {
				case 3:
					*(Uint16 *)dst = map[*src++];
					dst += 2;
				case 2:
					*(Uint32 *)dst =
					  (map[src[HI]]<<16)|(map[src[LO]]);
					src += 2;
					dst += 4;
					break;
				case 1:
					*(Uint16 *)dst = map[*src++];
					dst += 2;
					break;
			}
			src += srcskip;
			dst += dstskip;
		}
#endif
	}
//#endif /* USE_DUFFS_LOOP */
}
static void Blit1to3(SDL_BlitInfo *info)
{
#ifndef USE_DUFFS_LOOP
	int c;
#endif
	int o;
	int width, height;
	Uint8 *src, *map, *dst;
	int srcskip, dstskip;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = info->d_pixels;
	dstskip = info->d_skip;
	map = info->table;

	while ( height-- ) {
#ifdef USE_DUFFS_LOOP
		DUFFS_LOOP(
			{
				o = *src * 4;
				dst[0] = map[o++];
				dst[1] = map[o++];
				dst[2] = map[o++];
			}
			src++;
			dst += 3;
		, width);
#else
		for ( c=width; c; --c ) {
			o = *src * 4;
			dst[0] = map[o++];
			dst[1] = map[o++];
			dst[2] = map[o++];
			src++;
			dst += 3;
		}
#endif /* USE_DUFFS_LOOP */
		src += srcskip;
		dst += dstskip;
	}
}
static void Blit1to4(SDL_BlitInfo *info)
{
#ifndef USE_DUFFS_LOOP
	int c;
#endif
	int width, height;
	Uint8 *src;
	Uint32 *map, *dst;
	int srcskip, dstskip;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = (Uint32 *)info->d_pixels;
	dstskip = info->d_skip/4;
	map = (Uint32 *)info->table;

	while ( height-- ) {
#ifdef USE_DUFFS_LOOP
		DUFFS_LOOP(
			*dst++ = map[*src++];
		, width);
#else
		for ( c=width/4; c; --c ) {
			*dst++ = map[*src++];
			*dst++ = map[*src++];
			*dst++ = map[*src++];
			*dst++ = map[*src++];
		}
		switch ( width & 3 ) {
			case 3:
				*dst++ = map[*src++];
			case 2:
				*dst++ = map[*src++];
			case 1:
				*dst++ = map[*src++];
		}
#endif /* USE_DUFFS_LOOP */
		src += srcskip;
		dst += dstskip;
	}
}

static void Blit1to1Key(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	Uint8 *palmap = info->table;
	Uint32 ckey = info->src->colorkey;
        
	if ( palmap ) {
		while ( height-- ) {
			DUFFS_LOOP(
			{
				if ( *src != ckey ) {
				  *dst = palmap[*src];
				}
				dst++;
				src++;
			},
			width);
			src += srcskip;
			dst += dstskip;
		}
	} else {
		while ( height-- ) {
			DUFFS_LOOP(
			{
				if ( *src != ckey ) {
				  *dst = *src;
				}
				dst++;
				src++;
			},
			width);
			src += srcskip;
			dst += dstskip;
		}
	}
}

static void Blit1to2Key(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint16 *dstp = (Uint16 *)info->d_pixels;
	int dstskip = info->d_skip;
	Uint16 *palmap = (Uint16 *)info->table;
	Uint32 ckey = info->src->colorkey;

	/* Set up some basic variables */
	dstskip /= 2;

	while ( height-- ) {
		DUFFS_LOOP(
		{
			if ( *src != ckey ) {
				*dstp=palmap[*src];
			}
			src++;
			dstp++;
		},
		width);
		src += srcskip;
		dstp += dstskip;
	}
}

static void Blit1to3Key(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	Uint8 *palmap = info->table;
	Uint32 ckey = info->src->colorkey;
	int o;

	while ( height-- ) {
		DUFFS_LOOP(
		{
			if ( *src != ckey ) {
				o = *src * 4;
				dst[0] = palmap[o++];
				dst[1] = palmap[o++];
				dst[2] = palmap[o++];
			}
			src++;
			dst += 3;
		},
		width);
		src += srcskip;
		dst += dstskip;
	}
}

static void Blit1to4Key(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint32 *dstp = (Uint32 *)info->d_pixels;
	int dstskip = info->d_skip;
	Uint32 *palmap = (Uint32 *)info->table;
	Uint32 ckey = info->src->colorkey;

	/* Set up some basic variables */
	dstskip /= 4;

	while ( height-- ) {
		DUFFS_LOOP(
		{
			if ( *src != ckey ) {
				*dstp = palmap[*src];
			}
			src++;
			dstp++;
		},
		width);
		src += srcskip;
		dstp += dstskip;
	}
}

static void Blit1toNAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	SDL_PixelFormat *dstfmt = info->dst;
	const SDL_Color *srcpal	= info->src->palette->colors;
	int dstbpp;
	const int A = info->src->alpha;

	/* Set up some basic variables */
	dstbpp = dstfmt->BytesPerPixel;

	while ( height-- ) {
	        int sR, sG, sB;
		int dR, dG, dB;
	    	DUFFS_LOOP4(
			{
			        Uint32 pixel;
				sR = srcpal[*src].r;
				sG = srcpal[*src].g;
				sB = srcpal[*src].b;
				DISEMBLE_RGB(dst, dstbpp, dstfmt,
					     pixel, dR, dG, dB);
				ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
			  	ASSEMBLE_RGB(dst, dstbpp, dstfmt, dR, dG, dB);
				src++;
				dst += dstbpp;
			},
			width);
		src += srcskip;
		dst += dstskip;
	}
}

static void Blit1toNAlphaKey(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;
	const SDL_Color *srcpal	= info->src->palette->colors;
	Uint32 ckey = srcfmt->colorkey;
	int dstbpp;
	const int A = srcfmt->alpha;

	/* Set up some basic variables */
	dstbpp = dstfmt->BytesPerPixel;

	while ( height-- ) {
	        int sR, sG, sB;
		int dR, dG, dB;
		DUFFS_LOOP(
		{
			if ( *src != ckey ) {
			        Uint32 pixel;
				sR = srcpal[*src].r;
				sG = srcpal[*src].g;
				sB = srcpal[*src].b;
				DISEMBLE_RGB(dst, dstbpp, dstfmt,
							pixel, dR, dG, dB);
				ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
			  	ASSEMBLE_RGB(dst, dstbpp, dstfmt, dR, dG, dB);
			}
			src++;
			dst += dstbpp;
		},
		width);
		src += srcskip;
		dst += dstskip;
	}
}

static SDL_loblit one_blit[] = {
	NULL, Blit1to1, Blit1to2, Blit1to3, Blit1to4
};

static SDL_loblit one_blitkey[] = {
        NULL, Blit1to1Key, Blit1to2Key, Blit1to3Key, Blit1to4Key
};

SDL_loblit SDL_CalculateBlit1(SDL_Surface *surface, int blit_index)
{
	int which;
	SDL_PixelFormat *dstfmt;

	dstfmt = surface->map->dst->format;
	if ( dstfmt->BitsPerPixel < 8 ) {
		which = 0;
	} else {
		which = dstfmt->BytesPerPixel;
	}
	switch(blit_index) {
	case 0:			/* copy */
	    return one_blit[which];

	case 1:			/* colorkey */
	    return one_blitkey[which];

	case 2:			/* alpha */
	    /* Supporting 8bpp->8bpp alpha is doable but requires lots of
	       tables which consume space and takes time to precompute,
	       so is better left to the user */
	    return which >= 2 ? Blit1toNAlpha : NULL;

	case 3:			/* alpha + colorkey */
	    return which >= 2 ? Blit1toNAlphaKey : NULL;

	}
	return NULL;
}
