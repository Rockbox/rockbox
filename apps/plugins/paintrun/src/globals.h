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

#include "compat.h"

#define WIDTH_INCREMENT (LCD_WIDTH / 6)
#define MAX_HEIGHT (int)(3*LCD_HEIGHT/5)
#define MIN_HEIGHT (int)(LCD_HEIGHT * .1)

#define GAP_SMALL (WIDTH_INCREMENT * 2)
#define GAP_MED (WIDTH_INCREMENT * 3)
#define GAP_LARGE (WIDTH_INCREMENT * 4)

#define SCORE_INCREMENT FP_DIV(FIXED(1), FIXED(GAP_SMALL))

#define PLAYER_SIZE (LCD_HEIGHT / 12)
#define MAX_SPEED FIXED(3)

#define PLAYER_INITX FIXED(LCD_WIDTH/4)
#define PLAYER_INITY FIXED(0)

#define PLAYER_COLOR LCD_RGBPACK(200,20,20)
#define LAND_COLOR LCD_RGBPACK(200,200,200)
#define PAINT_COLOR LCD_RGBPACK(50,50,50)
#define BACKGROUND_COLOR LCD_RGBPACK(0,0,0)
#define OBSTACLE_COLOR LCD_RGBPACK(40,240,40)

#define MAX_OBSTACLES 16
#define OBSTACLE_SIZE PLAYER_SIZE
#define OBSTACLE_PATH_LENGTH (OBSTACLE_SIZE * 4)
#define OBSTACLE_ADDL_HEIGHT (int)(OBSTACLE_SIZE * 1.5)

struct section_t {
    int height; /* in pixels, 0 = void */
    color_t color; /* color */
};

struct coords_t {
    fixed_t x, y;
};

struct obstacle_t {
    struct coords_t position;
    struct coords_t bounds;
    struct coords_t vel;
    int left_to_travel; /* decremented until = 0, then y velocity flips */
    int total_travel;
    color_t color;

    int visible;
};

struct player_t {
    struct coords_t position;
    struct coords_t bounds;
    struct coords_t vel;
    color_t color;
};

struct game_ctx_t {
    struct section_t screen[LCD_WIDTH];
    /* no linked-list here! */
    struct obstacle_t obstacles[MAX_OBSTACLES];

    fixed_t score;

    struct player_t player;

    int draw_position;
    enum { LAND, VOID } current_type;
    int current_height;
    int left_of_current;
    int total_current;

    enum { RUNNING, PAUSED, OVER } status;
};
