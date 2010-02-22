/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Mat Holton
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

#include "plugin.h"
#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/highscore.h"
#include "lib/playback_control.h"

PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define AST_PAUSE BUTTON_ON
#define AST_QUIT BUTTON_OFF
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_PLAY

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define AST_PAUSE BUTTON_ON
#define AST_QUIT BUTTON_OFF
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define AST_PAUSE (BUTTON_MENU | BUTTON_OFF)
#define AST_QUIT BUTTON_OFF
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_MENU

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define AST_PAUSE BUTTON_REC
#define AST_QUIT BUTTON_OFF
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#define AST_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define AST_PAUSE BUTTON_PLAY
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define AST_PAUSE (BUTTON_SELECT | BUTTON_PLAY)
#define AST_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define AST_THRUST BUTTON_MENU
#define AST_HYPERSPACE BUTTON_PLAY
#define AST_LEFT BUTTON_SCROLL_BACK
#define AST_RIGHT BUTTON_SCROLL_FWD
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define AST_PAUSE BUTTON_A
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define AST_PAUSE BUTTON_REC
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_SCROLL_BACK
#define AST_RIGHT BUTTON_SCROLL_FWD
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define AST_PAUSE (BUTTON_SELECT | BUTTON_UP)
#define AST_QUIT (BUTTON_HOME|BUTTON_REPEAT)
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_SCROLL_BACK
#define AST_RIGHT BUTTON_SCROLL_FWD
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define AST_PAUSE BUTTON_REC
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_CLIP_PAD)
#define AST_PAUSE BUTTON_HOME
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_M200_PAD)
#define AST_PAUSE (BUTTON_SELECT | BUTTON_UP)
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE (BUTTON_SELECT | BUTTON_REL)

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define AST_PAUSE BUTTON_PLAY
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_SCROLL_UP
#define AST_HYPERSPACE BUTTON_SCROLL_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_REW

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define AST_PAUSE BUTTON_PLAY
#define AST_QUIT BUTTON_BACK
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define AST_PAUSE BUTTON_DISPLAY
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define AST_PAUSE BUTTON_RC_PLAY
#define AST_QUIT BUTTON_RC_REC
#define AST_THRUST BUTTON_RC_VOL_UP
#define AST_HYPERSPACE BUTTON_RC_VOL_DOWN
#define AST_LEFT BUTTON_RC_REW
#define AST_RIGHT BUTTON_RC_FF
#define AST_FIRE BUTTON_RC_MODE

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
#define AST_QUIT BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define AST_PAUSE BUTTON_PLAY
#define AST_QUIT BUTTON_BACK
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_SELECT

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define AST_PAUSE BUTTON_VIEW
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_LEFT
#define AST_RIGHT BUTTON_RIGHT
#define AST_FIRE BUTTON_PLAYLIST

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define AST_PAUSE BUTTON_RIGHT
#define AST_QUIT BUTTON_POWER
#define AST_THRUST BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT BUTTON_PREV
#define AST_RIGHT BUTTON_NEXT
#define AST_FIRE BUTTON_LEFT

#elif (CONFIG_KEYPAD == ONDAVX747_PAD) || \
      (CONFIG_KEYPAD == ONDAVX777_PAD) || \
      (CONFIG_KEYPAD == MROBE500_PAD)
#define AST_QUIT BUTTON_POWER

#elif (CONFIG_KEYPAD == SAMSUNG_YH_PAD)
#define AST_PAUSE      BUTTON_FFWD
#define AST_QUIT       BUTTON_REC
#define AST_THRUST     BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT       BUTTON_LEFT
#define AST_RIGHT      BUTTON_RIGHT
#define AST_FIRE       BUTTON_PLAY

#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
#define AST_PAUSE      BUTTON_PLAY
#define AST_QUIT       BUTTON_REC
#define AST_THRUST     BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT       BUTTON_PREV
#define AST_RIGHT      BUTTON_NEXT
#define AST_FIRE       BUTTON_OK

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef AST_PAUSE
#define AST_PAUSE       BUTTON_CENTER
#endif
#ifndef AST_QUIT
#define AST_QUIT        BUTTON_TOPLEFT
#endif
#ifndef AST_THRUST
#define AST_THRUST      BUTTON_TOPMIDDLE
#endif
#ifndef AST_HYPERSPACE
#define AST_HYPERSPACE  BUTTON_TOPRIGHT
#endif
#ifndef AST_LEFT
#define AST_LEFT        BUTTON_MIDLEFT
#endif
#ifndef AST_RIGHT
#define AST_RIGHT       BUTTON_MIDRIGHT
#endif
#ifndef AST_FIRE
#define AST_FIRE        BUTTON_BOTTOMMIDDLE
#endif
#endif

#define RES                     MAX(LCD_WIDTH, LCD_HEIGHT)
#define LARGE_LCD               (RES >= 200)

#define CYCLETIME               30

#define SHOW_COL                0
#define SCALE                   5000
#define WRAP_GAP                (LARGE*SCALE*3)
#define POINT_SIZE              2
#define START_LEVEL             1
#define SHOW_LEVEL_TIME         50
#define EXPLOSION_LENGTH        20

#define MAX_NUM_ASTEROIDS       25
#define MAX_NUM_MISSILES        6
#define NUM_STARS               50
#define NUM_TRAIL_POINTS        70
#define MAX_LEVEL               MAX_NUM_ASTEROIDS

#define NUM_ASTEROID_VERTICES   10
#define NUM_SHIP_VERTICES       4
#define NUM_ENEMY_VERTICES      8

#define INVULNERABLE_TIME       30
#define BLINK_TIME              10
#define EXTRA_LIFE              250
#define START_LIVES             3
#define MISSILE_LIFE_LENGTH     40

#define ASTEROID_SPEED          (RES/20)
#define SPACE_CHECK_SIZE        30*SCALE

#if (LARGE_LCD)
#define SIZE_SHIP_COLLISION     8*SCALE
#else
#define SIZE_SHIP_COLLISION     6*SCALE
#endif

#define LITTLE_SHIP             1
#define BIG_SHIP                2
#define ENEMY_BIG_PROBABILITY_START     10
#define ENEMY_APPEAR_PROBABILITY_START  35
#define ENEMY_APPEAR_TIMING_START       600
#define ENEMY_SPEED             4
#define ENEMY_MISSILE_LIFE_LENGTH   (RES/2)
#if (LARGE_LCD)
#define SIZE_ENEMY_COLLISION    7*SCALE
#else
#define SIZE_ENEMY_COLLISION    5*SCALE
#endif

#define SIN_COS_SCALE           10000

#define FAST_ROT_CW_SIN         873
#define FAST_ROT_CW_COS         9963
#define FAST_ROT_ACW_SIN        -873
#define FAST_ROT_ACW_COS        9963

#define MEDIUM_ROT_CW_SIN       350
#define MEDIUM_ROT_CW_COS       9994
#define MEDIUM_ROT_ACW_SIN      -350
#define MEDIUM_ROT_ACW_COS      9994

#define SLOW_ROT_CW_SIN         350
#define SLOW_ROT_CW_COS         9994
#define SLOW_ROT_ACW_SIN        -350
#define SLOW_ROT_ACW_COS        9994

#ifdef HAVE_LCD_COLOR
#define SHIP_ROT_CW_SIN         2419
#define SHIP_ROT_CW_COS         9702
#define SHIP_ROT_ACW_SIN        -2419
#define SHIP_ROT_ACW_COS        9702
#else
#define SHIP_ROT_CW_SIN         3827
#define SHIP_ROT_CW_COS         9239
#define SHIP_ROT_ACW_SIN        -3827
#define SHIP_ROT_ACW_COS        9239
#endif


#define SCALED_WIDTH            (LCD_WIDTH*SCALE)
#define SCALED_HEIGHT           (LCD_HEIGHT*SCALE)
#define CENTER_LCD_X            (LCD_WIDTH/2)
#define CENTER_LCD_Y            (LCD_HEIGHT/2)

#ifdef HAVE_LCD_COLOR
#define ASTEROID_R  230
#define ASTEROID_G  200
#define ASTEROID_B  100
#define SHIP_R      255
#define SHIP_G      255
#define SHIP_B      255
#define ENEMY_R     50
#define ENEMY_G     220
#define ENEMY_B     50
#define THRUST_R    200
#define THRUST_G    200
#define THRUST_B    0

#define COL_MISSILE  LCD_RGBPACK(200,0,0)
#define COL_PLAYER   LCD_RGBPACK(200,200,200)
#define COL_INVULN   LCD_RGBPACK(100,100,200)
#define COL_STARS    LCD_WHITE
#define COL_ASTEROID LCD_RGBPACK(ASTEROID_R,ASTEROID_G,ASTEROID_B)
#define COL_TEXT     LCD_RGBPACK(200,200,255)
#define COL_ENEMY    LCD_RGBPACK(ENEMY_R,ENEMY_G,ENEMY_B)
#define SET_FG       rb->lcd_set_foreground
#define SET_BG       rb->lcd_set_background
#else
#define SET_FG(x)
#define SET_BG(x)
#endif

#define HIGH_SCORE PLUGIN_GAMES_DIR "/spacerocks.score"
#define NUM_SCORES 5

static struct highscore highscores[NUM_SCORES];

/* The array of points that make up an asteroid */
static const short asteroid_one[NUM_ASTEROID_VERTICES*2] =
{
    -2, -12,
     4,  -8,
     8, -14,
    16,  -5,
    14,   0,
    20,   2,
    12,  14,
    -4,  14,
   -10,   6,
   -10,  -8,
};

/* The array of points that make up an asteroid */
static const short asteroid_two[NUM_ASTEROID_VERTICES*2] =
{
    -2, -12,
     4, -16,
     6, -14,
    16,  -8,
    14,   0,
    20,   2,
    12,  14,
    -4,  14,
   -10,   6,
   -10,  -8,
};

/* The array of points that make up an asteroid */
static const short asteroid_three[NUM_ASTEROID_VERTICES*2] =
{
    -2, -12,
     4, -16,
     6, -14,
     2,  -8,
    14,   0,
    20,   2,
    12,  14,
    -4,  14,
   -16,   6,
   -10,  -8,
};

/* The array of points the make up the ship */
static const short ship_vertices[NUM_SHIP_VERTICES*2] =
{
#if (LARGE_LCD)
    0, -6,
    4,  6,
    0,  2,
   -4,  6,
#else
    0, -4,
    3,  4,
    0,  1,
   -3,  4,
#endif
};

/* The array of points the make up the bad spaceship */
static const short enemy_vertices[NUM_ENEMY_VERTICES*2] =
{
#if (LARGE_LCD)
   -8,  0,
   -4,  4,
    4,  4,
    8,  0,
   -8,  0,
    8,  0,
    4, -4,
   -4, -4,
#else
   -5,  0,
   -2,  2,
    2,  2,
    5,  0,
   -5,  0,
    5,  0,
    2, -2,
   -2, -2,
#endif
};

enum asteroid_type
{
#if (LARGE_LCD)
    SMALL  = 2,
    MEDIUM = 4,
    LARGE  = 6,
#else
    SMALL  = 1,
    MEDIUM = 2,
    LARGE  = 3,
#endif
};

enum explosion_type
{
    EXPLOSION_SHIP,
    EXPLOSION_ASTEROID,
    EXPLOSION_ENEMY,
    EXPLOSION_THRUST,
};

enum game_state
{
    GAME_OVER,
    SHOW_LEVEL,
    PLAY_MODE,
    PAUSE_MODE,
};

struct Point
{
    int x;
    int y;
    int dx;
    int dy;
};

struct TrailPoint
{
    struct Point position;
    int alive;
#ifdef HAVE_LCD_COLOR
    short r;
    short g;
    short b;
    short dec;
#endif
};

/* Asteroid structure, contains an array of points */
struct Asteroid
{
    struct Point position;
    struct Point rotation;
    struct Point vertices[NUM_ASTEROID_VERTICES];
    bool exists;
    int explode_countdown;
    enum asteroid_type type;
    int radius;
    long speed_cos;
    long speed_sin;
};

struct Ship
{
    struct Point position;
    struct Point rotation;
    struct Point vertices[NUM_SHIP_VERTICES];
    bool exists;
    int explode_countdown;
    int invulnerable_time;
};

struct Enemy
{
    struct Point position;
    struct Point vertices[NUM_ENEMY_VERTICES];
    bool exists;
    int explode_countdown;
    int appear_countdown;
    short size_probability;
    short appear_probability;
    short appear_timing;
};

struct Missile
{
    struct Point position;
    struct Point oldpoint;
    int alive;
};

static enum game_state game_state;
static int asteroid_count;
static int next_missile_count;
static int next_thrust_count;
static int num_lives;
static int extra_life;
static int show_level_timeout;
static int current_level;
static int current_score;

static struct Ship ship;
static struct Point stars[NUM_STARS];
static struct Asteroid asteroids_array[MAX_NUM_ASTEROIDS];
static struct Missile missiles_array[MAX_NUM_MISSILES];
static struct Missile enemy_missile;
static struct Enemy enemy;
static struct Point lives_points[NUM_SHIP_VERTICES];
static struct TrailPoint trail_points[NUM_TRAIL_POINTS];

/*************************************************
** Handle polygon and point
*************************************************/

/* Check if point is in a polygon */
static bool is_point_in_polygon(struct Point* vertices, int num_vertices,
                                int x, int y)
{
    struct Point* pi;
    struct Point* pj;
    int n;
    bool c = false;

    if (x < -SCALED_WIDTH/2) x += SCALED_WIDTH;
    else if (x > SCALED_WIDTH/2) x -= SCALED_WIDTH;
    if (y < -SCALED_HEIGHT/2) y += SCALED_HEIGHT;
    else if (y > SCALED_HEIGHT/2) y -= SCALED_HEIGHT;

    pi = vertices;
    pj = vertices + num_vertices-1;

    n = num_vertices;
    while (n--)
    {
        if ((((pi->y <= y) && (y < pj->y)) || ((pj->y <= y) && (y < pi->y))) &&
            (x < (pj->x - pi->x) * (y - pi->y) / (pj->y - pi->y) + pi->x))
            c = !c;

        pj = pi;
        pi++;
    }

    return c;
}

/* Check if point is within a rectangle */
static bool is_point_within_rectangle(struct Point* rect, struct Point* p,
                                      int size)
{
    int dx = p->x - rect->x;
    int dy = p->y - rect->y;
#if SHOW_COL
    rb->lcd_drawrect((rect->x - size)/SCALE, (rect->y - size)/SCALE,
                     (size*2+1)/SCALE, (size*2+1)/SCALE);
#endif
    if (dx < -SCALED_WIDTH/2) dx += SCALED_WIDTH;
    else if (dx > SCALED_WIDTH/2) dx -= SCALED_WIDTH;
    if (dy < -SCALED_HEIGHT/2) dy += SCALED_HEIGHT;
    else if (dy > SCALED_HEIGHT/2) dy -= SCALED_HEIGHT;
    return (dx > -size && dx < size && dy > -size && dy < size);
}

/* Rotate polygon */
static void rotate_polygon(struct Point* vertices, int num_vertices,
                           struct Point* rotation, int cos, int sin)
{
    struct Point* point;
    int n;
    long temp_x, temp_y;

    temp_x = rotation->x;
    temp_y = rotation->y;
    rotation->x = (temp_x*cos - temp_y*sin)/SIN_COS_SCALE;
    rotation->y = (temp_y*cos + temp_x*sin)/SIN_COS_SCALE;
#define MIN_SCALE   (SIN_COS_SCALE-10)
#define MAX_SCALE   (SIN_COS_SCALE+10)
    /* normalize vector. this is not accurate but would be enough. */
    temp_x = rotation->x*rotation->x + rotation->y*rotation->y;
    if (temp_x <= MIN_SCALE*MIN_SCALE)
    {
        rotation->x = rotation->x*SIN_COS_SCALE/MIN_SCALE;
        rotation->y = rotation->y*SIN_COS_SCALE/MIN_SCALE;
    }
    else if (temp_x >= MAX_SCALE*MAX_SCALE)
    {
        rotation->x = rotation->x*SIN_COS_SCALE/MAX_SCALE;
        rotation->y = rotation->y*SIN_COS_SCALE/MAX_SCALE;
    }
#undef  MIN_SCALE
#undef  MAX_SCALE

    point = vertices;
    n = num_vertices;
    while (n--)
    {
        point->x = (point->dx*rotation->x - point->dy*rotation->y)/SIN_COS_SCALE;
        point->y = (point->dy*rotation->x + point->dx*rotation->y)/SIN_COS_SCALE;
        point++;
    }
}

/* Draw polygon */
static void draw_polygon(struct Point* vertices, int num_vertices,
                         int px, int py)
{
    int n, new_x, new_y, old_x, old_y;
    struct Point *p;
    bool draw_wrap;

    if (px > SCALED_WIDTH - WRAP_GAP)
        px -= SCALED_WIDTH;
    if (py > SCALED_HEIGHT - WRAP_GAP)
        py -= SCALED_HEIGHT;

    draw_wrap = (px < WRAP_GAP || py < WRAP_GAP);

    p = vertices + num_vertices - 1;
    old_x = (p->x + px)/SCALE;
    old_y = (p->y + py)/SCALE;
    p = vertices;
    n = num_vertices;
    while (n--)
    {
        new_x = (p->x + px)/SCALE;
        new_y = (p->y + py)/SCALE;

        rb->lcd_drawline(old_x, old_y, new_x, new_y);
        if (draw_wrap)
        {
            rb->lcd_drawline(old_x + LCD_WIDTH, old_y, new_x + LCD_WIDTH, new_y);
            rb->lcd_drawline(old_x, old_y + LCD_HEIGHT, new_x, new_y + LCD_HEIGHT);
            rb->lcd_drawline(old_x + LCD_WIDTH, old_y + LCD_HEIGHT,
                             new_x + LCD_WIDTH, new_y + LCD_HEIGHT);
        }
        old_x = new_x;
        old_y = new_y;
        p++;
    }
}

static void move_point(struct Point* point)
{
    point->x += point->dx;
    point->y += point->dy;

    /* Check bounds on the x-axis: */
    point->x %= SCALED_WIDTH;
    if (point->x < 0)
        point->x += SCALED_WIDTH;

    /* Check bounds on the y-axis: */
    point->y %= SCALED_HEIGHT;
    if (point->y < 0)
        point->y += SCALED_HEIGHT;
}

/*************************************************
** Handle trail blaiz.
*************************************************/

static void create_ship_trail(struct TrailPoint* tpoint)
{
    tpoint->position.x += ship.vertices[2].x;
    tpoint->position.y += ship.vertices[2].y;
    tpoint->position.dx = -( ship.vertices[0].x - ship.vertices[2].x )/10;
    tpoint->position.dy = -( ship.vertices[0].y - ship.vertices[2].y )/10;
}

static void create_explosion_trail(struct TrailPoint* tpoint)
{
    tpoint->position.dx = (rb->rand()%5001)-2500;
    tpoint->position.dy = (rb->rand()%5001)-2500;
}

static void create_trail_blaze(int colour, struct Point* position)
{
    int numtoadd;
    struct TrailPoint* tpoint;
    int n;

    if (colour != EXPLOSION_SHIP)
    {
        numtoadd = NUM_TRAIL_POINTS/5;
    }
    else
    {
        numtoadd = NUM_TRAIL_POINTS/8;
    }

    /* give the point a random countdown timer, so they dissapears at different
       times */
    tpoint = trail_points;
    n = NUM_TRAIL_POINTS;
    while (n--)
    {
        /* find a space in the array of trail_points that is NULL or DEAD or
           whatever and place this one here. */
        if (tpoint->alive <= 0)
        {
            /* take a random point near the position. */
            tpoint->position.x = (rb->rand()%18000)-9000 + position->x;
            tpoint->position.y = (rb->rand()%18000)-9000 + position->y;

            switch(colour)
            {
                case EXPLOSION_SHIP:
                    create_explosion_trail(tpoint);
                    tpoint->alive = 51;
#ifdef HAVE_LCD_COLOR
                    tpoint->r = SHIP_R;
                    tpoint->g = SHIP_G;
                    tpoint->b = SHIP_B;
                    tpoint->dec = 2;
#endif
                break;
                case EXPLOSION_ASTEROID:
                    create_explosion_trail(tpoint);
                    tpoint->alive = 51;
#ifdef HAVE_LCD_COLOR
                    tpoint->r = ASTEROID_R;
                    tpoint->g = ASTEROID_G;
                    tpoint->b = ASTEROID_B;
                    tpoint->dec = 2;
#endif
                break;
                case EXPLOSION_ENEMY:
                    create_explosion_trail(tpoint);
                    tpoint->alive = 51;
#ifdef HAVE_LCD_COLOR
                    tpoint->r = ENEMY_R;
                    tpoint->g = ENEMY_G;
                    tpoint->b = ENEMY_B;
                    tpoint->dec = 2;
#endif
                break;
                case EXPLOSION_THRUST:
                    create_ship_trail(tpoint);
                    tpoint->alive = 17;
#ifdef HAVE_LCD_COLOR
                    tpoint->r = THRUST_R;
                    tpoint->g = THRUST_G;
                    tpoint->b = THRUST_B;
                    tpoint->dec = 4;
#endif
                break;
            }

            /* give the points a speed based on direction of travel
               - i.e. opposite */
            tpoint->position.dx += position->dx;
            tpoint->position.dy += position->dy;

            numtoadd--;
            if (numtoadd <= 0)
                break;
        }
        tpoint++;
    }
}

static void draw_and_move_trail_blaze(void)
{
    struct TrailPoint* tpoint;
    int n;

    /* loop through, if alive then move and draw.
       when drawn, countdown it's timer.
       if zero kill it! */

    tpoint = trail_points;
    n = NUM_TRAIL_POINTS;
    while (n--)
    {
        if (tpoint->alive > 0)
        {
            if (game_state != PAUSE_MODE)
            {
                tpoint->alive--;
                move_point(&(tpoint->position));
#ifdef HAVE_LCD_COLOR
                /* intensity = tpoint->alive/2; */
                if (tpoint->r >= tpoint->dec) tpoint->r -= tpoint->dec;
                if (tpoint->g >= tpoint->dec) tpoint->g -= tpoint->dec;
                if (tpoint->b >= tpoint->dec) tpoint->b -= tpoint->dec;
#endif
            }
            SET_FG(LCD_RGBPACK(tpoint->r, tpoint->g, tpoint->b));
            rb->lcd_drawpixel(tpoint->position.x/SCALE, tpoint->position.y/SCALE);
        }
        tpoint++;
    }
}

/*************************************************
** Handle asteroid.
*************************************************/

static void rotate_asteroid(struct Asteroid* asteroid)
{
    rotate_polygon(asteroid->vertices, NUM_ASTEROID_VERTICES,
                   &asteroid->rotation,
                   asteroid->speed_cos, asteroid->speed_sin);
}

/* Initialise the passed Asteroid.
 * if position is NULL, place it at the random loacation
 * where ship doesn't exist
 */
static void initialise_asteroid(struct Asteroid* asteroid,
                                enum asteroid_type type, struct Point *position)
{
    const short *asteroid_vertices;
    struct Point* point;
    int n;

    asteroid->exists = true;
    asteroid->explode_countdown = 0;
    asteroid->type = type;

    /* Set the radius of the asteroid: */
    asteroid->radius = (int)type*SCALE*3;

    /* shall we move Clockwise and Fast */
    n = rb->rand()%100;
    if (n < 25)
    {
        asteroid->speed_cos = FAST_ROT_CW_COS;
        asteroid->speed_sin = FAST_ROT_CW_SIN;
    }
    else if (n < 50)
    {
        asteroid->speed_cos = FAST_ROT_ACW_COS;
        asteroid->speed_sin = FAST_ROT_ACW_SIN;
    }
    else if (n < 75)
    {
        asteroid->speed_cos = SLOW_ROT_ACW_COS;
        asteroid->speed_sin = SLOW_ROT_ACW_SIN;
    }
    else
    {
        asteroid->speed_cos = SLOW_ROT_CW_COS;
        asteroid->speed_sin = SLOW_ROT_CW_SIN;
    }

    n = rb->rand()%99;
    if (n < 33)
        asteroid_vertices = asteroid_one;
    else if (n < 66)
        asteroid_vertices = asteroid_two;
    else
        asteroid_vertices = asteroid_three;

    point = asteroid->vertices;
    for(n = 0; n < NUM_ASTEROID_VERTICES*2; n += 2)
    {
        point->x = asteroid_vertices[n];
        point->y = asteroid_vertices[n+1];
        point->x *= asteroid->radius/20;
        point->y *= asteroid->radius/20;
        /* dx and dy are used when rotate polygon */
        point->dx = point->x;
        point->dy = point->y;
        point++;
    }

    if (!position)
    {
        do {
            /* Set the position randomly: */
            asteroid->position.x = (rb->rand()%SCALED_WIDTH);
            asteroid->position.y = (rb->rand()%SCALED_HEIGHT);
        } while (is_point_within_rectangle(&ship.position, &asteroid->position,
                                           SPACE_CHECK_SIZE));
    }
    else
    {
        asteroid->position.x = position->x;
        asteroid->position.y = position->y;
    }

    do {
        asteroid->position.dx = (rb->rand()%ASTEROID_SPEED)-ASTEROID_SPEED/2;
    } while (asteroid->position.dx == 0);

    do {
        asteroid->position.dy = (rb->rand()%ASTEROID_SPEED)-ASTEROID_SPEED/2;
    } while (asteroid->position.dy == 0);

    asteroid->position.dx *= SCALE/10;
    asteroid->position.dy *= SCALE/10;

    asteroid->rotation.x = SIN_COS_SCALE;
    asteroid->rotation.y = 0;

    /* Now rotate the asteroid a bit, so they all look a bit different */
    for(n = (rb->rand()%30)+2; n--; )
        rotate_asteroid(asteroid);

    /* great, we've created an asteroid, don't forget to increment the total: */
    asteroid_count++;
}

/*
 * Creates a new asteroid of the given 4type (size) and at the given location.
 */
static void create_asteroid(enum asteroid_type type, struct Point *position)
{
    struct Asteroid* asteroid;
    int n;

    asteroid = asteroids_array;
    n = MAX_NUM_ASTEROIDS;
    while (n--)
    {
        if (!asteroid->exists && asteroid->explode_countdown <= 0)
        {
            initialise_asteroid(asteroid, type, position);
            break;
        }
        asteroid++;
    }
}

/* Draw and move all asteroids */
static void draw_and_move_asteroids(void)
{
    struct Asteroid* asteroid;
    int n;

    SET_FG(COL_ASTEROID);

    asteroid = asteroids_array;
    n = MAX_NUM_ASTEROIDS;
    while (n--)
    {
        if (asteroid->exists)
        {
            draw_polygon(asteroid->vertices, NUM_ASTEROID_VERTICES,
                         asteroid->position.x, asteroid->position.y);
        }
        if (game_state != PAUSE_MODE)
        {
            if (asteroid->exists)
            {
                move_point(&asteroid->position);
                rotate_asteroid(asteroid);
            }
            else if (asteroid->explode_countdown > 0)
            {
                asteroid->explode_countdown--;
            }
        }
        asteroid++;
    }
}

static void explode_asteroid(struct Asteroid* asteroid)
{
    struct Point p;
    p.dx = asteroid->position.dx;
    p.dy = asteroid->position.dy;
    p.x  = asteroid->position.x;
    p.y  = asteroid->position.y;

    asteroid_count--;
    asteroid->exists = false;

    switch(asteroid->type)
    {
        case SMALL:
            asteroid->explode_countdown = EXPLOSION_LENGTH;
            create_trail_blaze(EXPLOSION_ASTEROID, &p);
            break;

        case MEDIUM:
            create_asteroid(SMALL, &p);
            create_asteroid(SMALL, &p);
            break;

        case LARGE:
            create_asteroid(MEDIUM, &p);
            create_asteroid(MEDIUM, &p);
            break;
    }
}

/*************************************************
** Handle ship.
*************************************************/

/* Initialise the ship */
static void initialise_ship(void)
{
    struct Point* point;
    struct Point* lives_point;
    int n;

    ship.position.x = CENTER_LCD_X * SCALE;
    ship.position.y = CENTER_LCD_Y * SCALE;
    ship.position.dx = 0;
    ship.position.dy = 0;
    ship.rotation.x = SIN_COS_SCALE;
    ship.rotation.y = 0;
    ship.exists = true;
    ship.explode_countdown = 0;
    ship.invulnerable_time = INVULNERABLE_TIME;

    point = ship.vertices;
    lives_point = lives_points;
    for(n = 0; n < NUM_SHIP_VERTICES*2; n += 2)
    {
        point->x = ship_vertices[n];
        point->y = ship_vertices[n+1];
        point->x *= SCALE;
        point->y *= SCALE;
        /* dx and dy are used when rotate polygon */
        point->dx = point->x;
        point->dy = point->y;
        /* grab a copy of the ships points for the lives display: */
        lives_point->x = point->x;
        lives_point->y = point->y;

        point++;
        lives_point++;
    }
}

/*
 * Draws the ship, moves the ship and creates a new
 * one if it's finished exploding.
 */
static void draw_and_move_ship(void)
{
    if (ship.invulnerable_time > BLINK_TIME || ship.invulnerable_time % 2 != 0)
    {
        SET_FG(COL_INVULN);
    }
    else
    {
        SET_FG(COL_PLAYER);
    }

    if (ship.exists)
    {
        draw_polygon(ship.vertices, NUM_SHIP_VERTICES,
                     ship.position.x, ship.position.y);
    }

    if (game_state != PAUSE_MODE)
    {
        if (ship.exists)
        {
            if (ship.invulnerable_time > 0)
                ship.invulnerable_time--;
            move_point(&ship.position);
        }
        else if (ship.explode_countdown > 0)
        {
            ship.explode_countdown--;
            if (ship.explode_countdown <= 0)
            {
                num_lives--;
                if (num_lives <= 0)
                {
                    game_state = GAME_OVER;
                }
                else
                {
                    initialise_ship();
                }
            }
        }
    }
}

static void explode_ship(void)
{
    if (!ship.invulnerable_time)
    {
        /* if not invulnerable, blow up ship */
        ship.explode_countdown = EXPLOSION_LENGTH;
        ship.exists = false;
        create_trail_blaze(EXPLOSION_SHIP, &ship.position);
    }
}

/* Rotate the ship using the passed sin & cos values */
static void rotate_ship(int cos, int sin)
{
    if (ship.exists)
    {
        rotate_polygon(ship.vertices, NUM_SHIP_VERTICES,
                       &ship.rotation, cos, sin);
    }
}

static void thrust_ship(void)
{
    if (ship.exists)
    {
        ship.position.dx += ( ship.vertices[0].x - ship.vertices[2].x )/20;
        ship.position.dy += ( ship.vertices[0].y - ship.vertices[2].y )/20;

        /* if dx and dy are below a certain threshold, then set 'em to 0
           but to do this we need to ascertain if the spacehip as moved on
           screen for more than a certain amount. */

        create_trail_blaze(EXPLOSION_THRUST, &ship.position);
    }
}

/* stop movement of ship, 'cos that's what happens when you go into hyperspace. */
static void hyperspace(void)
{
    if (ship.exists)
    {
        ship.position.dx = ship.position.dy = 0;
        ship.position.x = (rb->rand()%SCALED_WIDTH);
        ship.position.y = (rb->rand()%SCALED_HEIGHT);
    }
}

static void draw_lives(void)
{
    int n;
#if (LARGE_LCD)
    int px = (LCD_WIDTH-1 - 4)*SCALE;
    int py = (LCD_HEIGHT-1 - 6)*SCALE;
#else
    int px = (LCD_WIDTH-1 - 3)*SCALE;
    int py = (LCD_HEIGHT-1 - 4)*SCALE;
#endif

    SET_FG(COL_PLAYER);

    n = num_lives-1;
    while (n--)
    {
        draw_polygon(lives_points, NUM_SHIP_VERTICES, px, py);
#if (LARGE_LCD)
        px -= 8*SCALE;
#else
        px -= 6*SCALE;
#endif
    }
}

/*
 * missile
 */

/* Initialise a missile */
static void initialise_missile(struct Missile* missile)
{
    missile->position.x = ship.position.x + ship.vertices[0].x;
    missile->position.y = ship.position.y + ship.vertices[0].y;
    missile->position.dx = (ship.vertices[0].x - ship.vertices[2].x)/2;
    missile->position.dy = (ship.vertices[0].y - ship.vertices[2].y)/2;
    missile->alive = MISSILE_LIFE_LENGTH;
    missile->oldpoint.x = missile->position.x;
    missile->oldpoint.y = missile->position.y;
}

/* Fire the next missile */
static void fire_missile(void)
{
    struct Missile* missile;
    int n;

    if (ship.exists)
    {
        missile = missiles_array;
        n = MAX_NUM_MISSILES;
        while (n--)
        {
            if (missile->alive <= 0)
            {
                initialise_missile(missile);
                break;
            }
            missile++;
        }
    }
}

/* Draw and Move all the missiles */
static void draw_and_move_missiles(void)
{
    struct Missile* missile;
    struct Point vertices[2];
    int n;

    SET_FG(COL_MISSILE);

    missile = missiles_array;
    n = MAX_NUM_MISSILES;
    while (n--)
    {
        if (missile->alive > 0)
        {
            vertices[0].x = 0;
            vertices[0].y = 0;
            vertices[1].x = -missile->position.dx;
            vertices[1].y = -missile->position.dy;
            draw_polygon(vertices, 2, missile->position.x, missile->position.y);

            if (game_state != PAUSE_MODE)
            {
                missile->oldpoint.x = missile->position.x;
                missile->oldpoint.y = missile->position.y;
                move_point(&missile->position);
                missile->alive--;
            }
        }
        missile++;
    }
}

/*************************************************
** Handle enemy.
*************************************************/

static void initialise_enemy(void)
{
    struct Point* point;
    int n;
    int size;

    if (rb->rand()%100 > enemy.size_probability)
    {
        size = BIG_SHIP;
        enemy.size_probability++;
        if (enemy.size_probability > 90)
        {
            enemy.size_probability = ENEMY_BIG_PROBABILITY_START;
        }
    }
    else
    {
        size = LITTLE_SHIP;
        enemy.size_probability = ENEMY_BIG_PROBABILITY_START;
    }

    enemy.exists = true;
    enemy.explode_countdown = 0;
    enemy.appear_countdown = enemy.appear_timing;

    point = enemy.vertices;
    for(n = 0; n < NUM_ENEMY_VERTICES*2; n += 2)
    {
        point->x  = enemy_vertices[n];
        point->y  = enemy_vertices[n+1];
        point->x *= size*SCALE/2;
        point->y *= size*SCALE/2;
        point++;
    }

    if (ship.position.x >= SCALED_WIDTH/2)
    {
        enemy.position.dx = ENEMY_SPEED;
        enemy.position.x  = 0;
    }
    else
    {
        enemy.position.dx = -ENEMY_SPEED;
        enemy.position.x  = SCALED_WIDTH;
    }

    if (ship.position.y >= SCALED_HEIGHT/2)
    {
        enemy.position.dy = ENEMY_SPEED;
        enemy.position.y  = 0;
    }
    else
    {
        enemy.position.dy = -ENEMY_SPEED;
        enemy.position.y  = SCALED_HEIGHT;
    }

    enemy.position.dx *= SCALE/10;
    enemy.position.dy *= SCALE/10;
}

static void draw_and_move_enemy(void)
{
    SET_FG(COL_ENEMY);

    if (enemy.exists)
    {
        draw_polygon(enemy.vertices, NUM_ENEMY_VERTICES,
                     enemy.position.x, enemy.position.y);
    }

    if (game_state != PAUSE_MODE)
    {
        if (enemy.exists)
        {
            enemy.position.x += enemy.position.dx;
            enemy.position.y += enemy.position.dy;

            if (enemy.position.x > SCALED_WIDTH || enemy.position.x < 0)
                enemy.exists = false;

            enemy.position.y %= SCALED_HEIGHT;
            if (enemy.position.y < 0)
                enemy.position.y += SCALED_HEIGHT;

            if ((rb->rand()%1000) < 10)
                enemy.position.dy = -enemy.position.dy;
        }
        else if (enemy.explode_countdown > 0)
        {
            enemy.explode_countdown--;
        }
        else
        {
            if (enemy.appear_countdown > 0)
                enemy.appear_countdown--;
            else if (rb->rand()%100 >= enemy.appear_probability)
                initialise_enemy();
        }
    }

    if (enemy_missile.alive <= 0)
    {
        /* if no missile and the enemy is here and not exploding..
           then shoot baby! */
        if (enemy.exists && ship.exists &&
            game_state == PLAY_MODE && (rb->rand()%10) >= 5 )
        {
            int dx = ship.position.x - enemy.position.x;
            int dy = ship.position.y - enemy.position.y;

            if (dx < -SCALED_WIDTH/2) dx += SCALED_WIDTH;
            else if (dx > SCALED_WIDTH/2) dx -= SCALED_WIDTH;
            if (dy < -SCALED_HEIGHT/2) dy += SCALED_HEIGHT;
            else if (dy > SCALED_HEIGHT/2) dy -= SCALED_HEIGHT;

            enemy_missile.position.x = enemy.position.x;
            enemy_missile.position.y = enemy.position.y;

            /* lame, needs to be sorted - it's trying to shoot at the ship */
            if (dx < -5*SCALE)
                enemy_missile.position.dx = -1;
            else if (dx > 5*SCALE)
                enemy_missile.position.dx = 1;
            else
                enemy_missile.position.dx = 0;

            if (dy < -5*SCALE)
                enemy_missile.position.dy = -1;
            else if (dy > 5*SCALE)
                enemy_missile.position.dy = 1;
            else
                enemy_missile.position.dy = 0;

            while (enemy_missile.position.dx == 0 &&
                   enemy_missile.position.dy == 0)
            {
                enemy_missile.position.dx = rb->rand()%2-1;
                enemy_missile.position.dy = rb->rand()%2-1;
            }

            enemy_missile.position.dx *= SCALE;
            enemy_missile.position.dy *= SCALE;
            enemy_missile.alive = ENEMY_MISSILE_LIFE_LENGTH;
        }
    }
    else
    {
        rb->lcd_fillrect( enemy_missile.position.x/SCALE,
                          enemy_missile.position.y/SCALE,
                          POINT_SIZE, POINT_SIZE );
        if (game_state != PAUSE_MODE)
        {
            move_point(&enemy_missile.position);
            enemy_missile.alive--;
        }
    }
}

/*************************************************
** Check collisions.
*************************************************/

/* Add score if missile hit asteroid or enemy */
static void add_score(int val)
{
    current_score += val;
    if (current_score >= extra_life)
    {
        num_lives++;
        extra_life += EXTRA_LIFE;
    }
}

static bool is_point_within_asteroid(struct Asteroid* asteroid,
                                     struct Point* point)
{
    if (is_point_within_rectangle(&asteroid->position, point, asteroid->radius)
     && is_point_in_polygon(asteroid->vertices, NUM_ASTEROID_VERTICES,
                            point->x - asteroid->position.x,
                            point->y - asteroid->position.y))
    {
        explode_asteroid(asteroid);
        return true;
    }
    else
        return false;
}

static bool is_point_within_ship(struct Point* point)
{
    if (is_point_within_rectangle(&ship.position, point, SIZE_SHIP_COLLISION)
     && is_point_in_polygon(ship.vertices, NUM_SHIP_VERTICES,
                            point->x - ship.position.x,
                            point->y - ship.position.y))
    {
        return true;
    }
    else
        return false;
}

static bool is_point_within_enemy(struct Point* point)
{
    if (is_point_within_rectangle(&enemy.position, point, SIZE_ENEMY_COLLISION))
    {
        add_score(5);
        enemy.explode_countdown = EXPLOSION_LENGTH;
        enemy.exists = false;
        create_trail_blaze(EXPLOSION_ENEMY, &enemy.position);
        return true;
    }
    else
        return false;
}

static bool is_ship_within_asteroid(struct Asteroid* asteroid)
{
    struct Point p;

    if (!is_point_within_rectangle(&asteroid->position, &ship.position,
                                   asteroid->radius+SIZE_SHIP_COLLISION))
        return false;

    p.x = ship.position.x + ship.vertices[0].x;
    p.y = ship.position.y + ship.vertices[0].y;
    if (is_point_within_asteroid(asteroid, &p))
        return true;

    p.x = ship.position.x + ship.vertices[1].x;
    p.y = ship.position.y + ship.vertices[1].y;
    if (is_point_within_asteroid(asteroid, &p))
        return true;

    p.x = ship.position.x + ship.vertices[3].x;
    p.y = ship.position.y + ship.vertices[3].y;
    if (is_point_within_asteroid(asteroid, &p))
        return true;

    return false;
}

/* Check for collsions between the missiles and the asteroids and the ship */
static void check_collisions(void)
{
    struct Missile* missile;
    struct Asteroid* asteroid;
    int m, n;
    bool asteroids_onscreen = false;

    asteroid = asteroids_array;
    m = MAX_NUM_ASTEROIDS;
    while (m--)
    {
        /* if the asteroids exists then test missile collision: */
        if (asteroid->exists)
        {
            missile = missiles_array;
            n = MAX_NUM_MISSILES;
            while (n--)
            {
                /* if the missiles exists: */
                if (missile->alive > 0)
                {
                    /* has the missile hit the asteroid? */
                    if (is_point_within_asteroid(asteroid, &missile->position) ||
                        is_point_within_asteroid(asteroid, &missile->oldpoint))
                    {
                        add_score(1);
                        missile->alive = 0;
                        break;
                    }
                }
                missile++;
            }

            /* now check collision with ship: */
            if (asteroid->exists && ship.exists)
            {
                if (is_ship_within_asteroid(asteroid))
                {
                    add_score(1);
                    explode_ship();
                }
            }

            /* has the enemy missile blown something up? */
            if (asteroid->exists && enemy_missile.alive > 0)
            {
                if (is_point_within_asteroid(asteroid, &enemy_missile.position))
                {
                    enemy_missile.alive = 0;
                }
            }
        }

        /* is an asteroid still exploding? */
        if (asteroid->explode_countdown > 0)
            asteroids_onscreen = true;

        asteroid++;
    }

    /* now check collision between ship and enemy */
    if (enemy.exists && ship.exists)
    {
        /* has the enemy collided with the ship? */
        if (is_point_within_enemy(&ship.position))
        {
            explode_ship();
            create_trail_blaze(EXPLOSION_ENEMY, &enemy.position);
        }

        if (enemy.exists)
        {
            /* Now see if the enemy has been shot at by the ships missiles: */
            missile = missiles_array;
            n = MAX_NUM_MISSILES;
            while (n--)
            {
                if (missile->alive > 0 &&
                    is_point_within_enemy(&missile->position))
                {
                    missile->alive = 0;
                    break;
                }
                missile++;
            }
        }
    }

    /* test collision with enemy missile and ship: */
    if (enemy_missile.alive > 0 && is_point_within_ship(&enemy_missile.position))
    {
        explode_ship();
        enemy_missile.alive = 0;
        enemy_missile.position.x = enemy_missile.position.y = 0;
    }

    /* if all asteroids cleared then start again: */
    if (asteroid_count == 0 && !asteroids_onscreen
        && !enemy.exists && enemy.explode_countdown <= 0)
    {
        current_level++;
        if (current_level > MAX_LEVEL)
            current_level = START_LEVEL;
        enemy.appear_probability += 5;
        if (enemy.appear_probability >= 100)
            enemy.appear_probability = ENEMY_APPEAR_PROBABILITY_START;
        enemy.appear_timing -= 30;
        if (enemy.appear_timing < 30)
            enemy.appear_timing = 30;
        game_state = SHOW_LEVEL;
        show_level_timeout = SHOW_LEVEL_TIME;
    }
}

/*
 * stars
 */

static void create_stars(void)
{
    struct Point* p;
    int n;

    p = stars;
    n = NUM_STARS;
    while (n--)
    {
        p->x = (rb->rand()%LCD_WIDTH);
        p->y = (rb->rand()%LCD_HEIGHT);
        p++;
    }
}

static void drawstars(void)
{
    struct Point* p;
    int n;

    SET_FG(COL_STARS);

    p = stars;
    n = NUM_STARS;
    while (n--)
    {
        rb->lcd_drawpixel(p->x , p->y);
        p++;
    }
}

/*************************************************
** Creates start_num number of new asteroids of
** full size.
**************************************************/
static void initialise_level(int start_num)
{
    struct Asteroid* asteroid;
    struct Missile* missile;
    struct TrailPoint* tpoint;
    int n;
    asteroid_count = next_missile_count = next_thrust_count = 0;

    /* no enemy */
    enemy.exists = 0;
    enemy.explode_countdown = 0;
    enemy_missile.alive = 0;

    /* clear asteroids */
    asteroid = asteroids_array;
    n = MAX_NUM_ASTEROIDS;
    while (n--)
    {
        asteroid->exists = false;
        asteroid++;
    }

    /* make some LARGE asteroids */
    for(n = 0; n < start_num; n++)
        initialise_asteroid(&asteroids_array[n], LARGE, NULL);

    /* ensure all missiles are out of action: */
    missile = missiles_array;
    n = MAX_NUM_MISSILES;
    while (n--)
    {
        missile->alive = 0;
        missile++;
    }

    tpoint = trail_points;
    n = NUM_TRAIL_POINTS;
    while (n--)
    {
        tpoint->alive = 0;
        tpoint++;
    }
}

static void initialise_game(void)
{
    enemy.appear_probability = ENEMY_APPEAR_PROBABILITY_START;
    enemy.appear_timing = ENEMY_APPEAR_TIMING_START;
    enemy.appear_countdown = enemy.appear_timing;
    enemy.size_probability = ENEMY_BIG_PROBABILITY_START;
    current_level = START_LEVEL;
    num_lives = START_LIVES;
    extra_life = EXTRA_LIFE;
    current_score = 0;
    initialise_ship();
    initialise_level(0);
    game_state = SHOW_LEVEL;
    show_level_timeout = SHOW_LEVEL_TIME;
}

/* menu stuff */
static bool spacerocks_help(void)
{
    static char *help_text[] = {
        "Spacerocks", "", "Aim", "",
        "The", "goal", "of", "the", "game", "is", "to", "blow", "up",
        "the", "asteroids", "and", "avoid", "being", "hit", "by", "them.",
        "Also", "you'd", "better", "watch", "out", "for", "the", "UFOs!"
    };
    static struct style_text formation[]={
        { 0, TEXT_CENTER|TEXT_UNDERLINE },
        { 2, C_RED },
        LAST_STYLE_ITEM
    };

    rb->lcd_setfont(FONT_UI);
    SET_BG(LCD_BLACK);
    SET_FG(LCD_WHITE);
    if (display_text(ARRAYLEN(help_text), help_text, formation, NULL, true))
        return true;
    rb->lcd_setfont(FONT_SYSFIXED);

    return false;
}

#define PLUGIN_OTHER 10
static bool ingame;
static int spacerocks_menu_cb(int action, const struct menu_item_ex *this_item)
{
    if (action == ACTION_REQUEST_MENUITEM
        && !ingame && ((intptr_t)this_item)==0)
        return ACTION_EXIT_MENUITEM;
    return action;
}

static int spacerocks_menu(void)
{
    int selection = 0;
    MENUITEM_STRINGLIST(main_menu, "Spacerocks Menu", spacerocks_menu_cb,
                        "Resume Game", "Start New Game",
                        "Help", "High Scores",
                        "Playback Control", "Quit");
    rb->button_clear_queue();

    while (1)
    {
        switch (rb->do_menu(&main_menu, &selection, NULL, false))
        {
            case 0:
                return PLUGIN_OTHER;
            case 1:
                initialise_game();
                return PLUGIN_OTHER;
            case 2:
                if (spacerocks_help())
                    return PLUGIN_USB_CONNECTED;
                break;
            case 3:
                highscore_show(NUM_SCORES, highscores, NUM_SCORES, true);
                break;
            case 4:
                playback_control(NULL);
                break;
            case 5:
                return PLUGIN_OK;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            default:
                break;
        }
    }
}

static int spacerocks_game_loop(void)
{
    char str[20];
    int button;
    int end;
    int position;
    int ret;

    if ((ret = spacerocks_menu()) != PLUGIN_OTHER)
        return ret;

    SET_BG(LCD_BLACK);

    ingame = true;
    while (true)
    {
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;
        rb->lcd_clear_display();
        SET_FG(COL_TEXT);
        switch(game_state)
        {
            case GAME_OVER:
                ingame = false;
                rb->splash (HZ * 2, "Game Over");
                rb->lcd_clear_display();
                position = highscore_update(current_score, current_level, "",
                                            highscores, NUM_SCORES);
                if (position != -1)
                {
                    if (position == 0)
                        rb->splash(HZ*2, "New High Score");
                    highscore_show(position, highscores, NUM_SCORES, true);
                }
                return PLUGIN_OTHER;
                break;

            case PAUSE_MODE:
                rb->snprintf(str, sizeof(str), "score %d ", current_score);
                rb->lcd_putsxy(1,LCD_HEIGHT-8, str);
                rb->lcd_putsxy(CENTER_LCD_X - 15,
                               CENTER_LCD_Y + CENTER_LCD_Y/2 - 4, "pause");
                draw_and_move_missiles();
                draw_lives();
                draw_and_move_ship();
                break;

            case PLAY_MODE:
                rb->snprintf(str, sizeof(str), "score %d ", current_score);
                rb->lcd_putsxy(1, LCD_HEIGHT-8, str);
                draw_and_move_missiles();
                draw_lives();
                check_collisions();
                draw_and_move_ship();
                break;

            case SHOW_LEVEL:
                rb->snprintf(str, sizeof(str), "score %d ", current_score);
                rb->lcd_putsxy(1, LCD_HEIGHT-8, str);
                rb->snprintf(str, sizeof(str), "stage %d ", current_level);
                rb->lcd_putsxy(CENTER_LCD_X - 20,
                               CENTER_LCD_Y + CENTER_LCD_Y/2 - 4, str);
                draw_lives();
                draw_and_move_ship();
                show_level_timeout--;
                if (show_level_timeout <= 0)
                {
                    initialise_level(current_level);
                    game_state = PLAY_MODE;
                }
                break;
        }
        draw_and_move_trail_blaze();
        drawstars();
        draw_and_move_asteroids();
        draw_and_move_enemy();

        rb->lcd_update();

#ifdef HAS_BUTTON_HOLD
        if (rb->button_hold() && game_state == PLAY_MODE)
            game_state = PAUSE_MODE;
#endif
        button = rb->button_get(false);
        switch(button)
        {
            case(AST_QUIT):
                return PLUGIN_OTHER;
                break;
#ifdef AST_PAUSE
            case(AST_PAUSE):
                if (game_state == PAUSE_MODE)
                    game_state = PLAY_MODE;
                else if (game_state == PLAY_MODE)
                    game_state = PAUSE_MODE;
                break;
#endif
            case (AST_LEFT):
            case (AST_LEFT | BUTTON_REPEAT):
                if (game_state == PLAY_MODE || game_state == SHOW_LEVEL)
                    rotate_ship(SHIP_ROT_ACW_COS, SHIP_ROT_ACW_SIN);
                break;

            case (AST_RIGHT):
            case (AST_RIGHT | BUTTON_REPEAT):
                if (game_state == PLAY_MODE || game_state == SHOW_LEVEL)
                    rotate_ship(SHIP_ROT_CW_COS, SHIP_ROT_CW_SIN);
                break;

            case (AST_THRUST):
            case (AST_THRUST | BUTTON_REPEAT):
                if (game_state == PLAY_MODE || game_state == SHOW_LEVEL)
                {
                    if (next_thrust_count <= 0)
                    {
                        next_thrust_count = 5;
                        thrust_ship();
                    }
                }
                break;

            case (AST_HYPERSPACE):
                if (game_state == PLAY_MODE)
                    hyperspace();
                /* maybe shield if it gets too hard */
                break;

            case (AST_FIRE):
            case (AST_FIRE | BUTTON_REPEAT):
                if (game_state == PLAY_MODE)
                {
                    if (next_missile_count <= 0)
                    {
                        fire_missile();
                        next_missile_count = 10;
                    }
                }
                else if(game_state == PAUSE_MODE)
                    game_state = PLAY_MODE;
                break;

            default:
                if (rb->default_event_handler(button)==SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }

        if (next_missile_count > 0)
            next_missile_count--;

        if (next_thrust_count > 0)
            next_thrust_count--;

        if (TIME_BEFORE(*rb->current_tick, end))
            rb->sleep(end-*rb->current_tick);
        else
            rb->yield();
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    int ret = PLUGIN_OTHER;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* universal font */
    rb->lcd_setfont(FONT_SYSFIXED);
    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */
    highscore_load(HIGH_SCORE, highscores, NUM_SCORES);
    rb->srand(*rb->current_tick);

    /* create stars once, and once only: */
    create_stars();

    while (ret == PLUGIN_OTHER)
        ret = spacerocks_game_loop();

    rb->lcd_setfont(FONT_UI);
    highscore_save(HIGH_SCORE, highscores, NUM_SCORES);
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

    return ret;
}
