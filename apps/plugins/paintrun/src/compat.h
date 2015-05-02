/****************************************************************************
 *     _____        _____ _   _ _______   _____  _    _ _   _
 *    |  __ \ /\   |_   _| \ | |__   __| |  __ \| |  | | \ | |
 *    | |__) /  \    | | |  \| |  | |    | |__) | |  | |  \| |
 *    |  ___/ /\ \   | | | . ` |  | |    |  _  /| |  | | . ` |
 *    | |  / ____ \ _| |_| |\  |  | |    | | \ \| |__| | |\  |
 *    |_| /_/    \_\_____|_| \_|  |_|    |_|  \_\\____/|_| \_|
 *
 * Copyright (C) 2015 Franklin Wei
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

/* defines a platform-neutral abstraction layer */

/*
  What a platform MUST define:
   LCD_WIDTH: fixed width of the screen, in pixels
   LCD_HEIGHT: height of screen
   RAND_MAX: maximum value returned by plat_rand
   color_t: RGB type
   LCD_RGBPACK(r,g,b): creates a color_t object from RGB components
   fixed_t: a fixed-point type with FRACBITS fractional bits
   FRACBITS: number of fractional bits in a fixed_t type
   FP_MUL(x,y): fixed-point multiply
   FP_DIV(x,y): fixed-point divide
   LOGF(str, ...): logging
*/

/*
  What a platform CAN define:
   PLAT_WANTS_YIELD - if defined, the code will call plat_yield() when possible
*/

#ifdef ROCKBOX
#include "../platforms/rockbox/rockbox.h"
#else
#include "../platforms/sdl/sdl.h"
#endif

#include <stdbool.h>

#undef ARRAYLEN
#define ARRAYLEN(x) (sizeof(x)/sizeof(x[0]))

#undef FIXED
#define FIXED(x) ((x)<<FRACBITS)

#undef RANDRANGE
#define RANDRANGE(a, b) (plat_rand()%(b-a)+a)

/* rounds x to nearest integer */
#undef FP_ROUND
#define FP_ROUND(x) (((x)+(1<<(FRACBITS-1)))>>FRACBITS)

#undef FLOAT_TO_FIXED
#define FLOAT_TO_FIXED(x) (x*(1<<FRACBITS))

#undef RAND_RANGE
#define RAND_RANGE(x,y) (plat_rand()%(y-x+1)+x)

#undef MIN
#define MIN(x,y) (x<y?x:y)

/* fixed_t is a fixed-point type with FRACBITS fractional bits */
/* FRACBITS is platform-dependent */
typedef long fixed_t;

void plat_set_foreground(color_t);
void plat_set_background(color_t);

void plat_clear(void);
void plat_vline(int x, int y1, int y2);
void plat_fillrect(int x, int y, int w, int h);
void plat_update(void);

unsigned plat_rand(void);

enum keyaction_t { NONE = 0, ACTION_JUMP, ACTION_PAUSE, ACTION_OTHER };

enum keyaction_t plat_pollaction(void);

#ifdef PLAT_WANTS_YIELD
void plat_yield(void);
#endif

void plat_sleep(long ms);

/* time is in ms */
long plat_time(void);

void plat_logf(const char*, ...);

struct game_ctx_t;

void plat_gameover(struct game_ctx_t*);

enum menuaction_t { MENU_DOGAME, MENU_ABOUT, MENU_QUIT };

enum menuaction_t plat_domenu(void);

void plat_paused(struct game_ctx_t*);

void plat_drawscore(int score);
