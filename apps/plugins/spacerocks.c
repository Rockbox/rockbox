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

#elif (CONFIG_KEYPAD == COWOND2_PAD)
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
CONFIG_KEYPAD == MROBE500_PAD
#define AST_QUIT BUTTON_POWER

#elif (CONFIG_KEYPAD == SAMSUNG_YH_PAD)
#define AST_PAUSE      BUTTON_FFWD
#define AST_QUIT       BUTTON_REC
#define AST_THRUST_REP (BUTTON_UP | BUTTON_REW)
#define AST_THRUST     BUTTON_UP
#define AST_HYPERSPACE BUTTON_DOWN
#define AST_LEFT       BUTTON_LEFT
#define AST_LEFT_REP   (BUTTON_LEFT | BUTTON_REW)
#define AST_RIGHT      BUTTON_RIGHT
#define AST_RIGHT_REP  (BUTTON_RIGHT | BUTTON_REW)
#define AST_FIRE       BUTTON_PLAY
#define AST_FIRE_REP   (BUTTON_PLAY | BUTTON_REW)

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
#ifndef AST_THRUST_REP
#define AST_THRUST_REP (BUTTON_TOPMIDDLE | BUTTON_REPEAT)
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
#ifndef AST_LEFT_REP
#define AST_LEFT_REP   (BUTTON_MIDLEFT | BUTTON_REPEAT)
#endif
#ifndef AST_RIGHT
#define AST_RIGHT       BUTTON_MIDRIGHT
#endif
#ifndef AST_RIGHT_REP
#define AST_RIGHT_REP  (BUTTON_MIDRIGHT | BUTTON_REPEAT)
#endif
#ifndef AST_FIRE
#define AST_FIRE        BUTTON_BOTTOMMIDDLE
#endif
#ifndef AST_FIRE_REP

#ifdef BUTTON_MENU
#define AST_FIRE_REP   (BUTTON_BOTTOMMIDDLE | BUTTON_MENU)
#else
#define AST_FIRE_REP   BUTTON_BOTTOMMIDDLE | BUTTON_REPEAT
#endif

#endif
#endif

#define RES                     MAX(LCD_WIDTH, LCD_HEIGHT)
#define LARGE_LCD               (RES >= 200)

#define CYCLETIME               30

#define SHOW_COL                0
#define SCALE                   5000
#define MISSILE_SCALE           5000
#define WRAP_GAP                12
#define POINT_SIZE              2
#define SHOW_GAME_OVER_TIME     100
#define SHOW_LEVEL_TIME         50
#define EXPLOSION_LENGTH        20

#define MAX_NUM_ASTEROIDS       25
#define MAX_NUM_MISSILES        6
#define MAX_LEVEL               MAX_NUM_ASTEROIDS
#define NUM_STARS               50
#define NUM_TRAIL_POINTS        70
#define NUM_ROTATIONS           16

#define NUM_ASTEROID_VERTICES   10
#define NUM_SHIP_VERTICES       4
#define NUM_ENEMY_VERTICES      6

#define SPAWN_TIME              30
#define BLINK_TIME              10
#define EXTRA_LIFE              250
#define START_LIVES             3
#define START_LEVEL             1
#define MISSILE_SURVIVAL_LENGTH 40

#define ASTEROID_SPEED          (RES/20)
#define SPACE_CHECK_SIZE        30*SCALE

#define LITTLE_SHIP             2
#define BIG_SHIP                1
#define ENEMY_BIG_PROBABILITY_START     10
#define ENEMY_APPEAR_PROBABILITY_START  35
#define ENEMY_APPEAR_TIMING_START       1800
#define ENEMY_SPEED             4
#define ENEMY_MISSILE_SURVIVAL_LENGTH (RES/2)
#define SIZE_ENEMY_COLLISION    5*SCALE

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


#define SCALED_WIDTH (LCD_WIDTH*SCALE)
#define SCALED_HEIGHT (LCD_HEIGHT*SCALE)
#define CENTER_LCD_X (LCD_WIDTH/2)
#define CENTER_LCD_Y (LCD_HEIGHT/2)

#define SHIP_EXPLOSION_COLOUR     1
#define ASTEROID_EXPLOSION_COLOUR 2
#define ENEMY_EXPLOSION_COLOUR    3
#define THRUST_COLOUR             4

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

struct highscore highscores[NUM_SCORES];

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

/* The array od points the make up the ship */
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
    4, -4,
   -4, -4,
#else
   -5,  0,
   -2,  2,
    2,  2,
    5,  0,
    2, -2,
   -2, -2,
#endif
};

enum asteroid_type
{
#if (LARGE_LCD)
    SMALL =  2,
    MEDIUM = 4,
    LARGE =  6,
#else
    SMALL =  1,
    MEDIUM = 2,
    LARGE =  3,
#endif
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
    int alive;
    struct Point position;
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
    struct Point vertices[NUM_SHIP_VERTICES];
    struct Point position;
    bool waiting_for_space;
    int explode_countdown;
    bool invulnerable;
    int spawn_time;
};

struct Enemy
{
    struct Point vertices[NUM_ENEMY_VERTICES];
    struct Point position;
    bool exists;
    int explode_countdown;
    long last_time_appeared;
    short size_probability;
    short appear_probability;
    short appear_timing;
};

struct Missile
{
    struct Point position;
    struct Point oldpoint;
    int survived;
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

void draw_and_move_asteroids(void);
void initialise_game(int nStartNum);

bool is_asteroid_near_ship(struct Asteroid* asteroid);
bool is_point_within_asteroid(struct Asteroid* asteroid, struct Point* point);

void initialise_asteroid(struct Asteroid* asteroid, enum asteroid_type eType);
void draw_polygon(struct Point* vertices, int px, int py, int num_vertices);
void rotate_asteroid(struct Asteroid* asteroid);
void create_asteroid(enum asteroid_type type, int x, int y);
void create_stars(void);

void initialise_ship(void);
void draw_and_move_ship(void);
void rotate_ship(int c, int s);
void thrust_ship(void);

void initialise_missile(struct Missile* missile);
void draw_and_move_missiles(void);
void fire_missile(void);

void animate_and_draw_explosion(struct Point* point, int num_points, int xoffset, int yoffset);
void initialise_explosion(struct Point* point, int num_points);

void move_point(struct Point* point);
void hyperspace(void);
void check_collisions(void);
void initialise_enemy(void);
void draw_and_move_enemy(void);
void draw_lives(void);
void drawstars(void);
bool is_ship_within_asteroid(struct Asteroid* asteroid);


void init(void)
{
    enemy.appear_probability = ENEMY_APPEAR_PROBABILITY_START;
    enemy.appear_timing = ENEMY_APPEAR_TIMING_START;
    enemy.size_probability = ENEMY_BIG_PROBABILITY_START;
    current_level = START_LEVEL;
    num_lives = START_LIVES;
    extra_life = EXTRA_LIFE;
    current_score = 0;
    initialise_ship();
    initialise_game(current_level);
    show_level_timeout = SHOW_LEVEL_TIME;
    game_state = PLAY_MODE;
}

static bool spacerocks_help(void)
{
    static char *help_text[] = {
        "Spacerocks", "", "Aim", "", "The", "goal", "of", "the", "game", "is",
        "to", "blow", "up", "the", "asteroids", "and", "avoid", "being", "hit", "by",
        "them.", "Also", "you'd", "better", "watch", "out", "for", "the", "UFOs!"
    };
    static struct style_text formation[]={
        { 0, TEXT_CENTER|TEXT_UNDERLINE },
        { 2, C_RED },
        { -1, 0 }
    };
    int button;

    rb->lcd_setfont(FONT_UI);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    if (display_text(ARRAYLEN(help_text), help_text, formation, NULL)
            ==PLUGIN_USB_CONNECTED)
        return true;
    do {
        button = rb->button_get(true);
        if (button == SYS_USB_CONNECTED)
            return true;
    } while( ( button == BUTTON_NONE )
            || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
    rb->lcd_setfont(FONT_SYSFIXED);

    return false;
}

static bool _ingame;
static int spacerocks_menu_cb(int action, const struct menu_item_ex *this_item)
{
    if(action == ACTION_REQUEST_MENUITEM
       && !_ingame && ((intptr_t)this_item)==0)
        return ACTION_EXIT_MENUITEM;
    return action;
}

static int spacerocks_menu(bool ingame)
{
    int choice = 0;

    _ingame = ingame;

    MENUITEM_STRINGLIST(main_menu, "Spacerocks Menu", spacerocks_menu_cb,
                        "Resume Game", "Start New Game",
                        "Help",  "High Scores",
                        "Playback Control", "Quit");
    rb->button_clear_queue();

    while (1)
    {
        switch (rb->do_menu(&main_menu, &choice, NULL, false))
        {
            case 0:
                return 0;
            case 1:
                init();
                return 0;
            case 2:
                if(spacerocks_help())
                    return 1;
                break;
            case 3:
                highscore_show(NUM_SCORES, highscores, NUM_SCORES, true);
                break;
            case 4:
                playback_control(NULL);
                break;
            case 5:
                return 1;
            case MENU_ATTACHED_USB:
                return 1;
            default:
                break;
        }
    }
}

bool point_in_poly(struct Point* point, int num_vertices, int x, int y)
{
    struct Point* pi;
    struct Point* pj;
    int n;
    bool c = false;

    pi = point;
    pj = point + num_vertices-1;

    n = num_vertices;
    while(n--)
    {
        if((((pi->y <= y) && (y < pj->y)) || ((pj->y <= y) && (y < pi->y))) &&
           (x < (pj->x - pi->x) * (y - pi->y) / (pj->y - pi->y) + pi->x))
            c = !c;

        pj = pi;
        pi++;
    }

    return c;
}

void move_point(struct Point* point)
{
    point->x += point->dx;
    point->y += point->dy;

    /*check bounds on the x-axis:*/
    point->x %= SCALED_WIDTH;
    if(point->x < 0)
        point->x += SCALED_WIDTH;

    /*Check bounds on the y-axis:*/
    point->y %= SCALED_HEIGHT;
    if(point->y < 0)
        point->y += SCALED_HEIGHT;
}

void create_ship_trail(struct TrailPoint* tpoint)
{
    tpoint->position.dx = -( ship.vertices[0].x - ship.vertices[2].x )/10;
    tpoint->position.dy = -( ship.vertices[0].y - ship.vertices[2].y )/10;
}

void create_explosion_trail(struct TrailPoint* tpoint)
{
    tpoint->position.dx = (rb->rand()%5001)-2500;
    tpoint->position.dy = (rb->rand()%5001)-2500;
}

void create_trail_blaze(int colour, struct Point* position)
{
    int numtoadd;
    struct TrailPoint* tpoint;
    int n;

    if(colour != SHIP_EXPLOSION_COLOUR)
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
    while(n-- && numtoadd)
    {
        /* find a space in the array of trail_points that is NULL or DEAD or
           whatever and place this one here. */
        if(tpoint->alive <= 0)
        {
            numtoadd--;
            /* take a random x point anywhere between bottom two points of ship. */
            /* ship.position.x; */
            tpoint->position.x = (ship.vertices[2].x + (rb->rand()%18000)-9000)
                                    + position->x;
            tpoint->position.y = (ship.vertices[2].y + (rb->rand()%18000)-9000)
                                    + position->y;

            switch(colour)
            {
                case SHIP_EXPLOSION_COLOUR:
                    create_explosion_trail(tpoint);
                    tpoint->alive = 510;
#ifdef HAVE_LCD_COLOR
                    tpoint->r = SHIP_R;
                    tpoint->g = SHIP_G;
                    tpoint->b = SHIP_B;
                    tpoint->dec = 2;
#endif
                break;
                case ASTEROID_EXPLOSION_COLOUR:
                    create_explosion_trail(tpoint);
                    tpoint->alive = 510;
#ifdef HAVE_LCD_COLOR
                    tpoint->r = ASTEROID_R;
                    tpoint->g = ASTEROID_G;
                    tpoint->b = ASTEROID_B;
                    tpoint->dec = 2;
#endif
                break;
                case ENEMY_EXPLOSION_COLOUR:
                    create_explosion_trail(tpoint);
                    tpoint->alive = 510;
#ifdef HAVE_LCD_COLOR
                    tpoint->r = ENEMY_R;
                    tpoint->g = ENEMY_G;
                    tpoint->b = ENEMY_B;
                    tpoint->dec = 2;
#endif
                break;
                case THRUST_COLOUR:
                    create_ship_trail(tpoint);
                    tpoint->alive = 175;
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
        }
        tpoint++;
    }
}

void draw_trail_blaze(void)
{
    struct TrailPoint* tpoint;
    int n;

    /* loop through, if alive then move and draw.
       when drawn, countdown it's timer.
       if zero kill it! */

    tpoint = trail_points;
    n = NUM_TRAIL_POINTS;
    while(n--)
    {
        if(tpoint->alive)
        {
            if(game_state != PAUSE_MODE)
            {
                tpoint->alive -= 10;
                move_point(&(tpoint->position));
            }
#ifdef HAVE_LCD_COLOR
            /* intensity = tpoint->alive/2; */
            if(tpoint->r >= tpoint->dec) tpoint->r -= tpoint->dec;
            if(tpoint->g >= tpoint->dec) tpoint->g -= tpoint->dec;
            if(tpoint->b >= tpoint->dec) tpoint->b -= tpoint->dec;
            SET_FG(LCD_RGBPACK(tpoint->r, tpoint->g, tpoint->b));
#endif
            rb->lcd_drawpixel(tpoint->position.x/SCALE, tpoint->position.y/SCALE);
        }
        tpoint++;
    }
}

/*Check if point is within a rectangle*/
bool is_point_within_rectangle(struct Point* rect, struct Point* p, int size)
{
#if SHOW_COL
    int aTLx = rect->x - size;
    int aTLy = rect->y - size;
    int aBRx = rect->x + size;
    int aBRy = rect->y + size;
    rb->lcd_hline( aTLx/SCALE, aBRx/SCALE, aTLy/SCALE);
    rb->lcd_vline( aTLx/SCALE, aTLy/SCALE, aBRy/SCALE);
    rb->lcd_hline( aTLx/SCALE, aBRx/SCALE, aBRy/SCALE);
    rb->lcd_vline( aBRx/SCALE, aBRy/SCALE, aTLy/SCALE);
    return (p->x > aTLx && p->x < aBRx && p->y > aTLy && p->y < aBRy);
#else
    return (p->x > rect->x - size && p->x < rect->x + size &&
            p->y > rect->y - size && p->y < rect->y + size);
#endif
}

/* Draw polygon */
void draw_polygon(struct Point* vertices, int px, int py, int num_vertices)
{
    int n, t1, t2, oldX, oldY;
    struct Point *p;
    bool bDrawAll = px < WRAP_GAP || LCD_WIDTH - px < WRAP_GAP ||
        py < WRAP_GAP || LCD_HEIGHT - py < WRAP_GAP;

    p = vertices + num_vertices - 1;
    oldX = p->x/SCALE + px;
    oldY = p->y/SCALE + py;
    p = vertices;
    n = num_vertices;
    while(n--)
    {
        t1 = p->x/SCALE + px;
        t2 = p->y/SCALE + py;

        rb->lcd_drawline(oldX, oldY, t1, t2);

        if(bDrawAll)
        {
            rb->lcd_drawline(oldX - LCD_WIDTH, oldY, t1 - LCD_WIDTH, t2);
            rb->lcd_drawline(oldX + LCD_WIDTH, oldY, t1 + LCD_WIDTH, t2);
            rb->lcd_drawline(oldX - LCD_WIDTH, oldY + LCD_HEIGHT,
                             t1 - LCD_WIDTH, t2 + LCD_HEIGHT);
            rb->lcd_drawline(oldX + LCD_WIDTH, oldY + LCD_HEIGHT,
                             t1 + LCD_WIDTH, t2 + LCD_HEIGHT);

            rb->lcd_drawline(oldX, oldY - LCD_HEIGHT, t1, t2 - LCD_HEIGHT);
            rb->lcd_drawline(oldX, oldY + LCD_HEIGHT, t1, t2 + LCD_HEIGHT);
            rb->lcd_drawline(oldX - LCD_WIDTH, oldY - LCD_HEIGHT,
                             t1 - LCD_WIDTH, t2 - LCD_HEIGHT);
            rb->lcd_drawline(oldX + LCD_WIDTH, oldY - LCD_HEIGHT,
                             t1 + LCD_WIDTH, t2 - LCD_HEIGHT);
        }
        oldX = t1;
        oldY = t2;
        p++;
    }
}

void animate_and_draw_explosion(struct Point* point, int num_points,
                                int xoffset, int yoffset)
{
    int n = num_points;
    while(n--)
    {
        if(game_state != PAUSE_MODE)
        {
            point->x += point->dx;
            point->y += point->dy;
        }
        rb->lcd_fillrect( point->x/SCALE + xoffset, point->y/SCALE + yoffset,
                          POINT_SIZE, POINT_SIZE );
        point++;
    }
}

/*stop movement of ship, 'cos that's what happens when you go into hyperspace.*/
void hyperspace(void)
{
    ship.position.dx = ship.position.dy = 0;
    ship.position.x = (rb->rand()%SCALED_WIDTH);
    ship.position.y = (rb->rand()%SCALED_HEIGHT);
}

void initialise_enemy(void)
{
    struct Point* point;
    int n;
    int size;

    if(rb->rand()%100 > enemy.size_probability)
    {
        size = BIG_SHIP;
        enemy.size_probability++;
        if(enemy.size_probability > 90)
        {
            enemy.size_probability = ENEMY_BIG_PROBABILITY_START;
        }
    }
    else
    {
        size = LITTLE_SHIP;
        enemy.size_probability = ENEMY_BIG_PROBABILITY_START;
    }

    enemy_missile.survived = 0;
    enemy.exists = true;
    enemy.explode_countdown = 0;
    enemy.last_time_appeared = *rb->current_tick;
    point = enemy.vertices;
    for(n = 0; n < NUM_ENEMY_VERTICES*2; n += 2)
    {
        point->x  = enemy_vertices[n];
        point->y  = enemy_vertices[n+1];
        point->x *= SCALE/size;
        point->y *= SCALE/size;
        point++;
    }

    if(ship.position.x >= SCALED_WIDTH/2)
    {
        enemy.position.dx  = ENEMY_SPEED;
        enemy.position.x   = 0;
    }
    else
    {
        enemy.position.dx  = -ENEMY_SPEED;
        enemy.position.x   = SCALED_WIDTH;
    }

    if(ship.position.y >= SCALED_HEIGHT/2)
    {
        enemy.position.dy  = ENEMY_SPEED;
        enemy.position.y   = 0;
    }
    else
    {
        enemy.position.dy  = -ENEMY_SPEED;
        enemy.position.y   = SCALED_HEIGHT;
    }

    enemy.position.dx *= SCALE/10;
    enemy.position.dy *= SCALE/10;
}

void draw_and_move_enemy(void)
{
    int enemy_x, enemy_y;

    SET_FG(COL_ENEMY);

    if(enemy.exists)
    {
        enemy_x = enemy.position.x/SCALE;
        enemy_y = enemy.position.y/SCALE;
        if(!enemy.explode_countdown)
        {
            draw_polygon(enemy.vertices, enemy_x, enemy_y, NUM_ENEMY_VERTICES);
            rb->lcd_drawline(enemy.vertices[0].x/SCALE + enemy_x,
                             enemy.vertices[0].y/SCALE + enemy_y,
                             enemy.vertices[3].x/SCALE + enemy_x,
                             enemy.vertices[3].y/SCALE + enemy_y);

            if(game_state != PAUSE_MODE)
            {
                enemy.position.x += enemy.position.dx;
                enemy.position.y += enemy.position.dy;

                if(enemy.position.x > SCALED_WIDTH || enemy.position.x < 0)
                    enemy.exists = false;

                if(enemy.position.y > SCALED_HEIGHT)
                    enemy.position.y = 0;
                else if(enemy.position.y < 0)
                    enemy.position.y = SCALED_HEIGHT;

                if((rb->rand()%1000) < 10)
                    enemy.position.dy = -enemy.position.dy;
            }
        }
        else
        {
            /* animate_and_draw_explosion(enemy.vertices, NUM_ENEMY_VERTICES,
                                          enemy_x, enemy.position.y/SCALE); */
            if(game_state != PAUSE_MODE)
            {
                enemy.explode_countdown--;
                if(!enemy.explode_countdown)
                    enemy.exists = false;
            }
        }
    }
    else
    {
        if (TIME_AFTER(*rb->current_tick,
                       enemy.last_time_appeared+enemy.appear_timing))
        {
            if(rb->rand()%100 >= enemy.appear_probability)
                initialise_enemy();
        }
    }

    if(!enemy_missile.survived)
    {
        /*if no missile and the enemy is here and not exploding..then shoot baby!*/
        if( !enemy.explode_countdown && enemy.exists &&
            !ship.waiting_for_space && game_state == PLAY_MODE &&
            (rb->rand()%10) >= 5 )
        {
            enemy_missile.position.x  = enemy.position.x;
            enemy_missile.position.y  = enemy.position.y;

            /*lame, needs to be sorted - it's trying to shoot at the ship*/
            if(ABS(enemy.position.y - ship.position.y) <= 5*SCALE)
                enemy_missile.position.dy = 0;
            else if( enemy.position.y < ship.position.y)
                enemy_missile.position.dy = 1;
            else
                enemy_missile.position.dy = -1;

            if(ABS(enemy.position.x - ship.position.x) <= 5*SCALE)
                enemy_missile.position.dx = 0;
            else if( enemy.position.x < ship.position.x)
                enemy_missile.position.dx = 1;
            else
                enemy_missile.position.dx = -1;

            while(enemy_missile.position.dx == 0 &&
                  enemy_missile.position.dy == 0)
            {
                enemy_missile.position.dx = rb->rand()%2-1;
                enemy_missile.position.dy = rb->rand()%2-1;
            }

            enemy_missile.position.dx *= SCALE;
            enemy_missile.position.dy *= SCALE;
            enemy_missile.survived = ENEMY_MISSILE_SURVIVAL_LENGTH;
        }
    }
    else
    {
        rb->lcd_fillrect( enemy_missile.position.x/SCALE,
                          enemy_missile.position.y/SCALE,
                          POINT_SIZE, POINT_SIZE );
        if(game_state != PAUSE_MODE)
        {
            move_point(&enemy_missile.position);
            enemy_missile.survived--;
        }
    }
}

void add_score(int val)
{
    current_score += val;
    if(current_score >= extra_life)
    {
        num_lives++;
        extra_life += EXTRA_LIFE;
    }
}

bool is_point_within_asteroid(struct Asteroid* asteroid, struct Point* point)
{
    if(!is_point_within_rectangle(&asteroid->position, point, asteroid->radius))
        return false;

    if(point_in_poly(asteroid->vertices, NUM_ASTEROID_VERTICES,
                     point->x - asteroid->position.x,
                     point->y - asteroid->position.y))
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
                create_trail_blaze(ASTEROID_EXPLOSION_COLOUR, &p);
                break;

            case MEDIUM:
                create_asteroid(SMALL, p.x, p.y);
                create_asteroid(SMALL, p.x, p.y);
                break;

            case LARGE:
                create_asteroid(MEDIUM, p.x, p.y);
                create_asteroid(MEDIUM, p.x, p.y);
                break;
        }
        return true;
    }
    else
        return false;
}

bool is_point_within_enemy(struct Point* point)
{
    if( is_point_within_rectangle(&enemy.position, point, 7*SCALE) )
    {
        add_score(5);
        /* enemy_missile.survived = 0; */
        enemy.explode_countdown = EXPLOSION_LENGTH;
        /* initialise_explosion(enemy.vertices, NUM_ENEMY_VERTICES); */
        create_trail_blaze(ENEMY_EXPLOSION_COLOUR, &enemy.position);
        return true;
    }
    else
        return false;
}

bool is_ship_within_asteroid(struct Asteroid* asteroid)
{
    struct Point p;

    p.x = ship.position.x + ship.vertices[0].x;
    p.y = ship.position.y + ship.vertices[0].y;
    if(is_point_within_asteroid(asteroid, &p))
        return true;

    p.x = ship.position.x + ship.vertices[1].x;
    p.y = ship.position.y + ship.vertices[1].y;
    if(is_point_within_asteroid(asteroid, &p))
        return true;

    p.x = ship.position.x + ship.vertices[3].x;
    p.y = ship.position.y + ship.vertices[3].y;
    if(is_point_within_asteroid(asteroid, &p))
        return true;

    return false;
}

void initialise_explosion(struct Point* point, int num_points)
{
    int n;

    point->x += point->dx;
    point->y += point->dy;
    n = num_points;
    while(n--)
    {
        point->dx = point->x;
        point->dy = point->y;
        point++;
    }
}

/* Check for collsions between the missiles and the asteroids and the ship */
void check_collisions(void)
{
    struct Missile* missile;
    struct Asteroid* asteroid;
    int m, n;
    bool asteroids_onscreen = false;
    bool ship_cant_be_placed = false;

    asteroid = asteroids_array;
    m = MAX_NUM_ASTEROIDS;
    while(m--)
    {
        /*if the asteroids exists then test missile collision:*/
        if (asteroid->exists)
        {
            missile = missiles_array;
            n = MAX_NUM_MISSILES;
            while(n--)
            {
                /*if the missiles exists:*/
                if(missile->survived > 0)
                {
                    /*has the missile hit the asteroid?*/
                    if(is_point_within_asteroid(asteroid, &missile->position) ||
                       is_point_within_asteroid(asteroid, &missile->oldpoint))
                    {
                        add_score(1);
                        missile->survived = 0;
                        break;
                    }
                }
                missile++;
            }

            /*now check collision with ship:*/
            if (asteroid->exists && !ship.waiting_for_space && !ship.explode_countdown)
            {
                if (is_ship_within_asteroid(asteroid))
                {
                    add_score(1);
                    if (!ship.invulnerable)
                    {
                        /*if not invulnerable, blow up ship*/
                        ship.explode_countdown = EXPLOSION_LENGTH;
                        /* initialise_explosion(ship.vertices, NUM_SHIP_VERTICES); */
                        create_trail_blaze(SHIP_EXPLOSION_COLOUR, &ship.position);
                    }
                }

                /*has the enemy missile blown something up?*/
                if (asteroid->exists && enemy_missile.survived)
                {
                    if(is_point_within_asteroid(asteroid, &enemy_missile.position))
                    {
                        enemy_missile.survived = 0;
                    }

                    /*if it still exists, check if ship is waiting for space:*/
                    if (asteroid->exists && ship.waiting_for_space)
                    {
                        ship_cant_be_placed |=
                            is_point_within_rectangle(&ship.position,
                                                      &asteroid->position,
                                                      SPACE_CHECK_SIZE);
                    }
                }
            }
        }

        /*is an asteroid still exploding?*/
        if (asteroid->explode_countdown)
            asteroids_onscreen = true;

        asteroid++;
    }

    /*now check collision between ship and enemy*/
    if(enemy.exists && !enemy.explode_countdown &&
       !ship.waiting_for_space && !ship.explode_countdown)
    {
        /*has the enemy collided with the ship?*/
        if(is_point_within_enemy(&ship.position))
        {
            if (!ship.invulnerable)
            {
                ship.explode_countdown = EXPLOSION_LENGTH;
                /* initialise_explosion(ship.vertices, NUM_SHIP_VERTICES); */
                create_trail_blaze(SHIP_EXPLOSION_COLOUR, &ship.position);
            }
            create_trail_blaze(ENEMY_EXPLOSION_COLOUR, &enemy.position);
        }

        if (enemy.exists && !enemy.explode_countdown)
        {
            /*Now see if the enemy has been shot at by the ships missiles:*/
            missile = missiles_array;
            n = MAX_NUM_MISSILES;
            while(n--)
            {
                if (missile->survived > 0 &&
                    is_point_within_enemy(&missile->position))
                {
                    missile->survived = 0;
                    break;
                }
                missile++;
            }
        }
    }

    /*test collision with enemy missile and ship:*/
    if (!ship_cant_be_placed && enemy_missile.survived > 0 &&
        point_in_poly(ship.vertices, NUM_SHIP_VERTICES,
                      enemy_missile.position.x - ship.position.x,
                      enemy_missile.position.y - ship.position.y))
    {
        if (!ship.invulnerable)
        {
            ship.explode_countdown = EXPLOSION_LENGTH;
            /* initialise_explosion(ship.vertices, NUM_SHIP_VERTICES); */
            create_trail_blaze(SHIP_EXPLOSION_COLOUR, &ship.position);
        }
        enemy_missile.survived = 0;
        enemy_missile.position.x = enemy_missile.position.y = 0;
    }

    if(!ship_cant_be_placed)
        ship.waiting_for_space = false;

    /*if all asteroids cleared then start again:*/
    if(asteroid_count == 0 && !enemy.exists && !asteroids_onscreen)
    {
        current_level++;
        game_state = SHOW_LEVEL;
        enemy.appear_probability += 5;
        enemy.appear_timing -= 200;
        if (enemy.appear_probability >= 100)
            enemy.appear_probability = ENEMY_APPEAR_PROBABILITY_START;
        show_level_timeout = SHOW_LEVEL_TIME;
    }
}

/*************************************************
** Creates a new asteroid of the given 4type (size)
** and at the given location.
*************************************************/
void create_asteroid(enum asteroid_type type, int x, int y)
{
    struct Asteroid* asteroid;
    int n;

    asteroid = asteroids_array;
    n = MAX_NUM_ASTEROIDS;
    while(n--)
    {
        if(!asteroid->exists && !asteroid->explode_countdown)
        {
            initialise_asteroid(asteroid, type);
            asteroid->position.x = x;
            asteroid->position.y = y;
            break;
        }
        asteroid++;
    }
}

/* Initialise a missile */
void initialise_missile(struct Missile* missile)
{
    missile->position.x = ship.position.x + ship.vertices[0].x;
    missile->position.y = ship.position.y + ship.vertices[0].y;
    missile->position.dx = (ship.vertices[0].x - ship.vertices[2].x)/2;
    missile->position.dy = (ship.vertices[0].y - ship.vertices[2].y)/2;
    missile->survived = MISSILE_SURVIVAL_LENGTH;
    missile->oldpoint.x = missile->position.x;
    missile->oldpoint.y = missile->position.y;
}

/* Draw and Move all the missiles */
void draw_and_move_missiles(void)
{
    struct Missile* missile;
    struct Point vertices[2];
    int n;

    SET_FG(COL_MISSILE);

    missile = missiles_array;
    n = MAX_NUM_MISSILES;
    while(n--)
    {
        if(missile->survived)
        {
            vertices[0].x = 0;
            vertices[0].y = 0;
            vertices[1].x = -missile->position.dx;
            vertices[1].y = -missile->position.dy;
            draw_polygon(vertices, missile->position.x/SCALE,
                         missile->position.y/SCALE, 2);

            if(game_state != PAUSE_MODE)
            {
                missile->oldpoint.x = missile->position.x;
                missile->oldpoint.y = missile->position.y;
                move_point(&missile->position);
                missile->survived--;
            }
        }
        missile++;
    }
}

void draw_lives(void)
{
    int n;
#if (LARGE_LCD)
    int px = (LCD_WIDTH-1 - 4);
    int py = (LCD_HEIGHT-1 - 6);
#else
    int px = (LCD_WIDTH-1 - 3);
    int py = (LCD_HEIGHT-1 - 4);
#endif

    SET_FG(COL_PLAYER);

    n = num_lives-1;
    while(n--)
    {
        draw_polygon(lives_points, px, py, NUM_SHIP_VERTICES);
#if (LARGE_LCD)
        px -= 8;
#else
        px -= 6;
#endif
    }
}

/*Fire the next missile*/
void fire_missile(void)
{
    struct Missile* missile;
    int n;

    if (!ship.explode_countdown && !ship.waiting_for_space)
    {
        missile = missiles_array;
        n = MAX_NUM_MISSILES;
        while(n--)
        {
            if(!missile->survived)
            {
                initialise_missile(missile);
                break;
            }
            missile++;
        }
    }
}

/* Initialise the passed Asteroid */
void initialise_asteroid(struct Asteroid* asteroid, enum asteroid_type type)
{
    const short *asteroid_vertices;
    struct Point* point;
    int n;

    asteroid->exists = true;
    asteroid->type = type;
    asteroid->explode_countdown = 0;

    /*Set the radius of the asteroid:*/
    asteroid->radius = (int)type*SCALE*3;

    /*shall we move Clockwise and Fast*/
    n = rb->rand()%100;
    if(n < 25)
    {
        asteroid->speed_cos = FAST_ROT_CW_COS;
        asteroid->speed_sin = FAST_ROT_CW_SIN;
    }
    else if(n < 50)
    {
        asteroid->speed_cos = FAST_ROT_ACW_COS;
        asteroid->speed_sin = FAST_ROT_ACW_SIN;
    }
    else if(n < 75)
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
        point++;
    }

    do
    {
        /*Set the position randomly:*/
        asteroid->position.x = (rb->rand()%SCALED_WIDTH);
        asteroid->position.y = (rb->rand()%SCALED_HEIGHT);
    } while (is_point_within_rectangle(&ship.position, &asteroid->position,
                                       SPACE_CHECK_SIZE));

    do {
        asteroid->position.dx = (rb->rand()%ASTEROID_SPEED)-ASTEROID_SPEED/2;
    } while (asteroid->position.dx == 0);

    do {
        asteroid->position.dy = (rb->rand()%ASTEROID_SPEED)-ASTEROID_SPEED/2;
    } while (asteroid->position.dy == 0);

    asteroid->position.dx *= SCALE/10;
    asteroid->position.dy *= SCALE/10;

    /*Now rotate the asteroid a bit, so they all look a bit different*/
    for(n = (rb->rand()%30)+2; n--; )
        rotate_asteroid(asteroid);

    /*great, we've created an asteroid, don't forget to increment the total:*/
    asteroid_count++;
}

/*Initialise the ship*/
void initialise_ship(void)
{
    struct Point* point;
    struct Point* lives_point;
    int n;

    ship.position.x = CENTER_LCD_X * SCALE;
    ship.position.y = CENTER_LCD_Y * SCALE;
    ship.position.dx = 0;
    ship.position.dy = 0;
    ship.explode_countdown  = 0;
    ship.spawn_time = SPAWN_TIME;
    ship.invulnerable = 1;

    point = ship.vertices;
    lives_point = lives_points;
    for(n = 0; n < NUM_SHIP_VERTICES*2; n += 2)
    {
        point->x = ship_vertices[n];
        point->y = ship_vertices[n+1];
        point->x *= SCALE;
        point->y *= SCALE;
        /*grab a copy of the ships points for the lives display:*/
        lives_point->x = point->x;
        lives_point->y = point->y;

        point++;
        lives_point++;
    }
}

void rotate_asteroid(struct Asteroid* asteroid)
{
    struct Point* point;
    int n;
    long xtemp;
    
    point = asteroid->vertices;
    for(n = NUM_ASTEROID_VERTICES+1; --n;)
    {
        xtemp = point->x;
        point->x = xtemp*asteroid->speed_cos/SIN_COS_SCALE -
            point->y*asteroid->speed_sin/SIN_COS_SCALE;
        point->y = point->y*asteroid->speed_cos/SIN_COS_SCALE +
            xtemp*asteroid->speed_sin/SIN_COS_SCALE;
        point++;
    }
}

/*************************************************
** Draws the ship, moves the ship and creates a new
** one if it's finished exploding.
**************************************************/
void draw_and_move_ship(void)
{
    if (ship.invulnerable &&
        (ship.spawn_time > BLINK_TIME || ship.spawn_time % 2 == 0))
    {
        SET_FG(COL_INVULN);
    }
    else
    {
        SET_FG(COL_PLAYER);
    }

    if(!ship.explode_countdown)
    {
        /* make sure ship is invulnerable until spawn time over */
        if (ship.spawn_time)
        {
            ship.spawn_time--;
            if (ship.spawn_time <= 0)
            {
                ship.invulnerable = 0;
            }
        }
        if(!ship.waiting_for_space)
        {
            draw_polygon(ship.vertices, ship.position.x/SCALE,
                         ship.position.y/SCALE, NUM_SHIP_VERTICES);
            if(game_state != PAUSE_MODE && game_state != GAME_OVER)
            {
                move_point(&ship.position);
            }
        }
    }
    else
    {
        /* animate_and_draw_explosion(ship.vertices, NUM_SHIP_VERTICES,
                                      ship.position.x/SCALE,
                                      ship.position.y/SCALE); */
        if(game_state != PAUSE_MODE)
        {
            ship.explode_countdown--;
            if(!ship.explode_countdown)
            {
                num_lives--;
                if(!num_lives)
                {
                    game_state = GAME_OVER;
                }
                else
                {
                    initialise_ship();
                    ship.waiting_for_space = true;
                }
            }
        }
    }
}

void thrust_ship(void)
{
    if(!ship.waiting_for_space)
    {
        ship.position.dx += ( ship.vertices[0].x - ship.vertices[2].x )/20;
        ship.position.dy += ( ship.vertices[0].y - ship.vertices[2].y )/20;

        /*if dx and dy are below a certain threshold, then set 'em to 0
          but to do this we need to ascertain if the spacehip as moved on screen
          for more than a certain amount. */

        create_trail_blaze(THRUST_COLOUR, &ship.position);
    }
}

/**************************************************
** Rotate the ship using the passed sin & cos values
***************************************************/
void rotate_ship(int c, int s)
{
    struct Point* point;
    int n;
    long xtemp;
    
    if(!ship.waiting_for_space && !ship.explode_countdown)
    {
        point = ship.vertices;
        for(n=NUM_SHIP_VERTICES+1;--n;)
        {
            xtemp = point->x;
            point->x = xtemp*c/SIN_COS_SCALE - point->y*s/SIN_COS_SCALE;
            point->y = point->y*c/SIN_COS_SCALE + xtemp*s/SIN_COS_SCALE;
            point++;
        }
    }
}

void drawstars()
{
    struct Point* p;
    int n;

    SET_FG(COL_STARS);

    p = stars;
    n = NUM_STARS;
    while(n--)
    {
        rb->lcd_drawpixel(p->x , p->y);
        p++;
    }
}

/*************************************************
**  Draw And Move all Asteroids
*************************************************/
void draw_and_move_asteroids(void)
{
    struct Asteroid* asteroid;
    int n;

    SET_FG(COL_ASTEROID);

    asteroid = asteroids_array;
    n = MAX_NUM_ASTEROIDS;
    while(n--)
    {
        if(game_state != PAUSE_MODE)
        {
            if(asteroid->exists)
            {
                move_point(&asteroid->position);
                rotate_asteroid(asteroid);
                draw_polygon(asteroid->vertices, asteroid->position.x/SCALE,
                             asteroid->position.y/SCALE, NUM_ASTEROID_VERTICES);
            }
            else if(asteroid->explode_countdown)
            {
                /* animate_and_draw_explosion(asteroid->vertices,
                                              NUM_ASTEROID_VERTICES,
                                              asteroid->position.x/SCALE,
                                              asteroid->position.y/SCALE); */
                asteroid->explode_countdown--;
            }
        }
        else
        {
            if(asteroid->exists)
                draw_polygon(asteroid->vertices, asteroid->position.x/SCALE,
                             asteroid->position.y/SCALE, NUM_ASTEROID_VERTICES);
        }
        asteroid++;
    }
}

void create_stars(void)
{
    struct TrailPoint* tpoint;
    struct Point* p;
    int n;

    p = stars;
    n = NUM_STARS;
    while(n--)
    {
        p->x = (rb->rand()%LCD_WIDTH);
        p->y = (rb->rand()%LCD_HEIGHT);
        p++;
    }

    tpoint = trail_points;
    n = NUM_TRAIL_POINTS;
    while(--n)
    {
        tpoint->alive = 0;
        tpoint++;
    }
}

/*************************************************
** Creates start_num number of new asteroids of
** full size.
**************************************************/
void initialise_game(int start_num)
{
    struct Asteroid* asteroid;
    struct Missile* missile;
    int n;
    asteroid_count = next_missile_count = next_thrust_count = 0;

    /*no enemy*/
    enemy.exists = 0;
    enemy_missile.survived = 0;

    /*clear asteroids*/
    asteroid = asteroids_array;
    n = MAX_NUM_ASTEROIDS;
    while(n--)
    {
        asteroid->exists = false;
        asteroid++;
    }

    /*make some LARGE asteroids*/
    for(n = 0; n < start_num; n++)
        initialise_asteroid(&asteroids_array[n], LARGE);

    /*ensure all missiles are out of action:  */
    missile = missiles_array;
    n = MAX_NUM_MISSILES;
    while(--n)
    {
        missile->survived = 0;
        missile++;
    }
}

static int spacerocks_game_loop(void)
{
    char s[20];
    char level[10];
    int button;
    int end;
    int position;

    /*create stars once, and once only:*/
    create_stars();

    if (spacerocks_menu(false)!=0)
        return 0;

    SET_BG(LCD_BLACK);

    while(true)
    {
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;
        rb->lcd_clear_display();
        SET_FG(COL_TEXT);
        switch(game_state)
        {
            case GAME_OVER:
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
                if (spacerocks_menu(false)!=0)
                    return 0;
                break;

            case PAUSE_MODE:
                rb->snprintf(s, sizeof(s), "score %d ", current_score);
                rb->lcd_putsxy(1,LCD_HEIGHT-8, s);
                rb->lcd_putsxy(CENTER_LCD_X - 15,
                               CENTER_LCD_Y + CENTER_LCD_Y/2 - 4, "pause");
                draw_and_move_missiles();
                draw_lives();
                draw_and_move_ship();
                break;

            case PLAY_MODE:
                rb->snprintf(s, sizeof(s), "score %d ", current_score);
                rb->lcd_putsxy(1,LCD_HEIGHT-8, s);
                draw_and_move_missiles();
                draw_lives();
                check_collisions();
                draw_and_move_ship();
                break;

            case SHOW_LEVEL:
                rb->snprintf(s, sizeof(s), "score %d ", current_score);
                rb->lcd_putsxy(1,LCD_HEIGHT-8, s);
                rb->snprintf(level, sizeof(level), "stage %d ", current_level);
                rb->lcd_putsxy(CENTER_LCD_X - 20,
                               CENTER_LCD_Y + CENTER_LCD_Y/2 - 4, level);
                draw_and_move_ship();
                draw_lives();
                show_level_timeout--;
                if(!show_level_timeout)
                {
                    initialise_game(current_level);
                    game_state = PLAY_MODE;
                    draw_lives();
                }
                break;
        }
        draw_trail_blaze();
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
                if (spacerocks_menu(true)!=0)
                    return 0;
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
                if(game_state == PLAY_MODE || game_state == SHOW_LEVEL)
                    rotate_ship(SHIP_ROT_ACW_COS, SHIP_ROT_ACW_SIN);
                break;

            case (AST_RIGHT):
            case (AST_RIGHT | BUTTON_REPEAT):
                if(game_state == PLAY_MODE || game_state == SHOW_LEVEL)
                    rotate_ship(SHIP_ROT_CW_COS, SHIP_ROT_CW_SIN);
                break;

            case (AST_THRUST):
            case (AST_THRUST | BUTTON_REPEAT):
                if(game_state == PLAY_MODE || game_state == SHOW_LEVEL)
                {
                    if (!next_thrust_count)
                    {
                        next_thrust_count = 5;
                        thrust_ship();
                    }
                }
                break;

            case (AST_HYPERSPACE):
                if(game_state == PLAY_MODE)
                    hyperspace();
                /*maybe shield if it gets too hard  */
                break;

            case (AST_FIRE):
            case (AST_FIRE | BUTTON_REPEAT):
                if(game_state == PLAY_MODE)
                {
                    if(!next_missile_count)
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

        if(next_missile_count)
            next_missile_count--;

        if(next_thrust_count)
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

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* universal font */
    rb->lcd_setfont(FONT_SYSFIXED);
    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */
    highscore_load(HIGH_SCORE, highscores, NUM_SCORES);

    spacerocks_game_loop();

    rb->lcd_setfont(FONT_UI);
    highscore_save(HIGH_SCORE, highscores, NUM_SCORES);
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

    return PLUGIN_OK;
}
