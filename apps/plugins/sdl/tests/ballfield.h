/*
 * "Ballfield"
 *
 *   (C) David Olofson <david@olofson.net>, 2002
 *
 * This software is released under the terms of the GPL.
 *
 * Contact author for permission if you want to use this
 * software, or work derived from it, under other terms.
 */

#ifndef	_BALLFIELD_H_
#define	_BALLFIELD_H_

#include "SDL.h"

/*----------------------------------------------------------
	Definitions...
----------------------------------------------------------*/

#define	BALLS	200

#define	COLORS	2

typedef struct
{
	Sint32	x, y, z;	/* Position */
	Uint32	c;		/* Color */
} point_t;


/*
 * Ballfield
 */
typedef struct
{
	point_t		points[BALLS];
	SDL_Rect	*frames;
	SDL_Surface	*gfx[COLORS];
	int		use_alpha;
} ballfield_t;


/*
 * Size of the screen in pixels
 */
#define	SCREEN_W	320
#define	SCREEN_H	240

/*
 * Size of the biggest ball image in pixels
 *
 * Balls are scaled down and *packed*, one pixel
 * smaller for each frame down to 1x1. The actual
 * image width is (obviously...) the same as the
 * width of the first frame.
 */
#define	BALL_W	32
#define	BALL_H	32

#endif	/* _BALLFIELD_H_ */
