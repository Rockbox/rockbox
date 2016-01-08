/*
 * xrick/data/tiles.h
 *
 * Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net).
 * Copyright (C) 2008-2014 Pierluigi Vicinanza.
 * All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

/*
 * NOTES
 *
 * A tile consists in one column and 8 rows of 8 U16 (cga encoding, two
 * bits per pixel). The tl_tiles array contains all tiles, with the
 * following structure:
 *
 *  0x0000 - 0x00FF  tiles for main intro
 *  0x0100 - 0x01FF  tiles for map intro
 *  0x0200 - 0x0327  unused
 *  0x0328 - 0x0427  game tiles, page 0
 *  0x0428 - 0x0527  game tiles, page 1
 *  0x0527 - 0x05FF  unused
 */

#ifndef _TILES_H
#define _TILES_H

#include "xrick/system/basic_types.h"

#include "xrick/config.h"

#include <stddef.h> /* size_t */

/*
 * three special tile numbers
 */
enum {
    TILES_BULLET = 0x01,
    TILES_BOMB = 0x02,
    TILES_RICK = 0x03
};

/*
 * one single tile
 */
enum { TILES_NBR_LINES = 0x08 };

#ifdef GFXPC
typedef U16 tile_t[TILES_NBR_LINES];
#endif
#ifdef GFXST
typedef U32 tile_t[TILES_NBR_LINES];
#endif

/*
 * tiles banks (each bank is 0x100 tiles)
 */
enum { TILES_NBR_TILES = 0x100 };
extern size_t tiles_nbr_banks;
extern tile_t *tiles_data;

#endif /* ndef _TILES_H */

/* eof */
