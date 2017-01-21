/*
 * "Parallax Scrolling III - Overdraw Elimination"
 *
 *   Nghia             <nho@optushome.com.au>
 *   Randi J. Relander <rjrelander@users.sourceforge.net>
 *   David Olofson     <david@gardena.net>
 *
 * This software is released under the terms of the GPL.
 *
 * Contact authors for permission if you want to use this
 * software, or work derived from it, under other terms.
 */

#ifndef _PARALLAX3_H_
#define _PARALLAX3_H_

#include "SDL.h"

/*----------------------------------------------------------
        Definitions...
----------------------------------------------------------*/

/* foreground and background velocities in pixels/sec */
#define FOREGROUND_VEL_X        100.0
#define FOREGROUND_VEL_Y        50.0

#define BACKGROUND_VEL          100.0

/*
 * Size of the screen in pixels
 */
#define SCREEN_W        320
#define SCREEN_H        240

/*
 * Size of one background tile in pixels
 */
#define TILE_W          48
#define TILE_H          48

/*
 * The maps are 16 by 16 squares, and hold one
 * character per square. The characters determine
 * which tiles are to be drawn in the corresponding
 * squares on the screen. Space (" ") means that
 * no tile will be drawn.
 */
#define MAP_W           16
#define MAP_H           16

typedef char map_data_t[MAP_H][MAP_W];

typedef struct layer_t
{
        /* Next layer in recursion order */
        struct layer_t  *next;

        /* Position and speed */
        float           pos_x, pos_y;
        float           vel_x, vel_y;

        /* Various flags */
        int             flags;

        /* Map and tile data */
        map_data_t      *map;
        SDL_Surface     *tiles;
        SDL_Surface     *opaque_tiles;

        /* Position link */
        struct layer_t  *link;
        float           ratio;

        /* Statistics */
        int             calls;
        int             blits;
        int             recursions;
        int             pixels;
} layer_t;

/* Flag definitions */
#define TL_LIMIT_BOUNCE 0x00000001
#define TL_LINKED       0x00000002

static void layer_init(layer_t *lr, map_data_t *map,
                SDL_Surface *tiles, SDL_Surface *opaque_tiles);
static void layer_next(layer_t *lr, layer_t *next_l);
static void layer_pos(layer_t *lr, float x, float y);
static void layer_vel(layer_t *lr, float x, float y);
static void layer_animate(layer_t *lr, float dt);
static void layer_limit_bounce(layer_t *lr);
static void layer_link(layer_t *lr, layer_t *to_l, float ratio);
static void layer_render(layer_t *lr, SDL_Surface *screen, SDL_Rect *rect);
static void layer_reset_stats(layer_t *lr);

#endif  /* _PARALLAX3_H_ */
