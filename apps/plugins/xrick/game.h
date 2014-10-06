/*
 * xrick/game.h
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

#ifndef _GAME_H
#define _GAME_H

#include "xrick/config.h"
#include "xrick/rects.h"
#ifdef ENABLE_SOUND
#include "xrick/data/sounds.h"
#endif

#include <stddef.h> /* NULL */

#define LEFT 1
#define RIGHT 0

#define GAME_PERIOD 40

#define GAME_BOMBS_INIT 6
#define GAME_BULLETS_INIT 6

extern U8 game_lives;      /* lives counter */
extern U8 game_bombs;      /* bombs counter */
extern U8 game_bullets;    /* bullets counter */

extern U32 game_score;     /* score */

extern U16 game_map;       /* current map */
extern U16 game_submap;    /* current submap */

extern U8 game_dir;        /* direction (LEFT, RIGHT) */
extern bool game_chsm;       /* change submap request (true, false) */

extern bool game_waitevt;    /* wait for events (true, false) */
extern U8 game_period;     /* time between each frame, in millisecond */

extern const rect_t *game_rects; /* rectangles to redraw at each frame */

extern void game_run(void);
#ifdef ENABLE_SOUND
extern void game_setmusic(sound_t * sound, S8 loop);
extern void game_stopmusic(void);
#endif /* ENABLE_SOUND */

#ifdef ENABLE_CHEATS
typedef enum
{
    Cheat_UNLIMITED_ALL,
    Cheat_NEVER_DIE,
    Cheat_EXPOSE
} cheat_t;
extern bool game_cheat1;     /* infinite lives, bombs and bullets */
extern bool game_cheat2;     /* never die */
extern bool game_cheat3;     /* highlight sprites */
extern void game_toggleCheat(cheat_t);
#endif /* ENABLE_CHEATS */

#endif /* ndef _GAME_H */

/* eof */


