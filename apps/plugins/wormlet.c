/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Philipp Pertermann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

/* size of the field the worm lives in */
#define FIELD_RECT_X 1
#define FIELD_RECT_Y 1
#define FIELD_RECT_WIDTH  (LCD_WIDTH - 45)
#define FIELD_RECT_HEIGHT (LCD_HEIGHT - 2)

/* size of the ring of the worm 
   choos a value that is a power of 2 to help 
   the compiler optimize modul operations*/
#define MAX_WORM_SEGMENTS 64   

/* when the game starts */
#define INITIAL_WORM_LENGTH 10 

/* num of pixel the worm grows per eaten food */
#define WORM_PER_FOOD 7        

/* num of worms creeping in the FIELD */
#define MAX_WORMS 3            

/* minimal distance between a worm and an argh 
   when a new argh is made */
#define MIN_ARGH_DIST 5

/**
 * All the properties that a worm has.
 */
static struct worm {
    /* The worm is stored in a ring of xy coordinates */
    char x[MAX_WORM_SEGMENTS];
    char y[MAX_WORM_SEGMENTS];

    int head;      /* index of the head within the buffer */
    int tail;      /* index of the tail within the buffer */
    int growing;   /* number of cyles the worm still keeps growing */
    bool alive;    /* the worms living state */

    /* direction vector in which the worm moves */
    int dirx; /* only values -1 0 1 allowed */
    int diry; /* only values -1 0 1 allowed */

    /* this method is used to fetch the direction the user
       has selected. It can be one of the values
       human_player1, human_player2, remote_player, virtual_player. 
       All these values are fuctions, that can change the direction 
       of the worm */
    void (*fetch_worm_direction)(struct worm *w);
} worms[MAX_WORMS];

/* stores the highscore - besides it was scored by a virtual player */
static int highscore;

#define MAX_FOOD  5 /* maximal number of food items */
#define FOOD_SIZE 3 /* the width and height of a food */

/* The arrays store the food coordinates  */
static char foodx[MAX_FOOD];
static char foody[MAX_FOOD];

#define MAX_ARGH  100    /* maximal number of argh items */
#define ARGH_SIZE 4      /* the width and height of a argh */
#define ARGHS_PER_FOOD 2 /* number of arghs produced per eaten food */

/* The arrays store the argh coordinates */
static char arghx[MAX_ARGH];
static char arghy[MAX_ARGH];

/* the number of arghs that are currently in use */ 
static int argh_count;

#ifdef DEBUG_WORMLET
/* just a buffer used for debug output */
static char debugout[15];
#endif

/* the number of ticks each game cycle should take */ 
#define SPEED 14

/* the number of active worms (dead or alive) */
static int worm_count = MAX_WORMS;

/* in multiplayer mode: en- / disables the remote worm control
   in singleplayer mode: toggles 4 / 2 button worm control */
static bool use_remote = false;

/* return values of check_collision */
#define COLLISION_NONE 0
#define COLLISION_WORM 1
#define COLLISION_FOOD 2
#define COLLISION_ARGH 3
#define COLLISION_FIELD 4

/* constants for use as directions.
   Note that the values are ordered clockwise.
   Thus increasing / decreasing the values
   is equivalent to right / left turns. */
#define WEST  0
#define NORTH 1
#define EAST  2
#define SOUTH 3

/* direction of human player 1 */
static int player1_dir = EAST; 
/* direction of human player 2 */
static int player2_dir = EAST; 
/* direction of human player 3 */
static int player3_dir = EAST;

/* the number of (human) players that currently
   control a worm */
static int players = 1;

/* the rockbox plugin api */
static struct plugin_api* rb;

#ifdef DEBUG_WORMLET
static void set_debug_out(char *str){
    strcpy(debugout, str);
}
#endif

/**
 * Returns the direction id in which the worm
 * currently is creeping.
 * @param struct worm *w The worm that is to be investigated. 
 *        w Must not be null.
 * @return int A value 0 <= value < 4
 *         Note the predefined constants NORTH, SOUTH, EAST, WEST
 */
static int get_worm_dir(struct worm *w) {
    int retVal ;
    if (w->dirx == 0) {
        if (w->diry == 1) {
            retVal = SOUTH;
        } else {
            retVal = NORTH;
        }
    } else {
        if (w->dirx == 1) {
            retVal = EAST;
        } else {
            retVal = WEST;
        }
    }
    return retVal;
}

/**
 * Set the direction of the specified worm with a direction id.
 * Increasing the value by 1 means to turn the worm direction
 * to right by 90 degree.
 * @param struct worm *w The worm that is to be altered. w Must not be null.
 * @param int dir The new direction in which the worm is to creep.
 *        dir must be  0 <= dir < 4. Use predefined constants 
 *        NORTH, SOUTH, EAST, WEST
 */
static void set_worm_dir(struct worm *w, int dir) {
    switch (dir) {
        case WEST:
            w->dirx = -1;
            w->diry = 0;
            break;
         case NORTH:
            w->dirx = 0;
            w->diry = - 1;
            break;
        case EAST:
            w->dirx = 1;
            w->diry = 0;
            break;
        case SOUTH:
            w->dirx = 0;
            w->diry = 1;
            break;
   }
}

/**
 * Returns the current length of the worm array. This
 * is also a value for the number of bends that are in the worm.
 * @return int a positive value with 0 <= value < MAX_WORM_SEGMENTS
 */
static int get_worm_array_length(struct worm *w) {
    /* initial simple calculation will be overwritten if wrong. */
    int retVal = w->head - w->tail;

    /* if the worm 'crosses' the boundaries of the ringbuffer */
    if (retVal < 0) {
        retVal = w->head + MAX_WORM_SEGMENTS - w->tail;
    }

    return retVal;
}

/**
 * Returns the score the specified worm. The score is the length
 * of the worm.
 * @param struct worm *w The worm that is to be investigated.
 *        w must not be null.
 * @return int The length of the worm (>= 0).
 */
static int get_score(struct worm *w) {
    int retval = 0;
    int length = get_worm_array_length(w);
    int i;
    for (i = 0; i < length; i++) {

        /* The iteration iterates the length of the worm.
           Here's the conversion to the true indices within the worm arrays. */
        int linestart = (w->tail + i  ) % MAX_WORM_SEGMENTS;
        int lineend   = (linestart + 1) % MAX_WORM_SEGMENTS;
        int startx = w->x[linestart];
        int starty = w->y[linestart];
        int endx = w->x[lineend];
        int endy = w->y[lineend];

        int minimum, maximum;

        if (startx == endx) {
            minimum = MIN(starty, endy);
            maximum = MAX(starty, endy);
        } else {
            minimum = MIN(startx, endx);
            maximum = MAX(startx, endx);
        }
            retval += abs(maximum - minimum);
    }
    return retval;
}

/**
 * Determines wether the line specified by startx, starty, endx, endy intersects
 * the rectangle specified by x, y, width, height. Note that the line must be exactly
 * horizontal or vertical (startx == endx or starty == endy). 
 * @param int startx The x coordinate of the start point of the line.
 * @param int starty The y coordinate of the start point of the line.
 * @param int endx The x coordinate of the end point of the line.
 * @param int endy The y coordinate of the end point of the line.
 * @param int x The x coordinate of the top left corner of the rectangle.
 * @param int y The y coordinate of the top left corner of the rectangle.
 * @param int width The width of the rectangle.
 * @param int height The height of the rectangle.
 * @return bool Returns true if the specified line intersects with the recangle.
 */
static bool line_in_rect(int startx, int starty, int endx, int endy, int x, int y, int width, int height) {
    bool retval = false;
    int simple, simplemin, simplemax;
    int compa, compb, compmin, compmax;
    int temp;
    if (startx == endx) {
        simple = startx;
        simplemin = x;
        simplemax = x + width;

        compa = starty;
        compb = endy;
        compmin = y;
        compmax = y + height;
    } else {
        simple = starty;
        simplemin = y;
        simplemax = y + height;

        compa = startx;
        compb = endx;
        compmin = x;
        compmax = x + width;
    };

    temp = compa;
    compa = MIN(compa, compb);
    compb = MAX(temp, compb);

    if (simplemin <= simple && simple <= simplemax) {
        if ((compmin <= compa && compa <= compmax) ||
            (compmin <= compb && compb <= compmax) ||
            (compa <= compmin && compb >= compmax)) {
            retval = true;
        }
    }
    return retval;
}

/** 
 * Tests wether the specified worm intersects with the rect.
 * @param struct worm *w The worm to be investigated
 * @param int x The x coordinate of the top left corner of the rect
 * @param int y The y coordinate of the top left corner of the rect
 * @param int widht The width of the rect
 * @param int height The height of the rect
 * @return bool Returns true if the worm intersects with the rect
 */
static bool worm_in_rect(struct worm *w, int x, int y, int width, int height) {
    bool retval = false;


    /* get_worm_array_length is expensive -> buffer the value */
    int wormLength = get_worm_array_length(w);
    int i;

    /* test each entry that is part of the worm */
    for (i = 0; i < wormLength && retval == false; i++) {

        /* The iteration iterates the length of the worm.
           Here's the conversion to the true indices within the worm arrays. */
        int linestart = (w->tail + i  ) % MAX_WORM_SEGMENTS;
        int lineend   = (linestart + 1) % MAX_WORM_SEGMENTS;
        int startx = w->x[linestart];
        int starty = w->y[linestart];
        int endx = w->x[lineend];
        int endy = w->y[lineend];

        retval = line_in_rect(startx, starty, endx, endy, x, y, width, height);
    }

    return retval;
}

/**
 * Checks wether a specific food in the food arrays is at the
 * specified coordinates.
 * @param int foodIndex The index of the food in the food arrays
 * @param int x the x coordinate.
 * @param int y the y coordinate.
 * @return Returns true if the coordinate hits the food specified by
 * foodIndex.
 */
static bool specific_food_collision(int foodIndex, int x, int y) {
    bool retVal = false;
    if (x >= foodx[foodIndex]             &&
        x <  foodx[foodIndex] + FOOD_SIZE &&
        y >= foody[foodIndex]             &&
        y <  foody[foodIndex] + FOOD_SIZE) {

        retVal = true;
    }
    return retVal;
}

/**
 * Returns the index of the food that is at the
 * given coordinates. If no food is at the coordinates
 * -1 is returned.
 * @return int  -1 <= value < MAX_FOOD
 */
static int food_collision(int x, int y) {
    int i = 0;
    int retVal = -1;
    for (i = 0; i < MAX_FOOD; i++) {
        if (specific_food_collision(i, x, y)) {
            retVal = i;
            break;
        }
    }
    return retVal;
}

/**
 * Checks wether a specific argh in the argh arrays is at the
 * specified coordinates.
 * @param int arghIndex The index of the argh in the argh arrays
 * @param int x the x coordinate.
 * @param int y the y coordinate.
 * @return Returns true if the coordinate hits the argh specified by
 * arghIndex.
 */
static bool specific_argh_collision(int arghIndex, int x, int y) {

    if ( x >= arghx[arghIndex] &&
         y >= arghy[arghIndex] &&
         x <  arghx[arghIndex] + ARGH_SIZE &&
         y <  arghy[arghIndex] + ARGH_SIZE )
    {
        return true;
    }

    return false;
}

/**
 * Returns the index of the argh that is at the
 * given coordinates. If no argh is at the coordinates
 * -1 is returned.
 * @param int x The x coordinate.
 * @param int y The y coordinate.
 * @return int  -1 <= value < argh_count <= MAX_ARGH
 */
static int argh_collision(int x, int y) {
    int i = 0;
    int retVal = -1;

    /* search for the argh that has the specified coords */
    for (i = 0; i < argh_count; i++) {
        if (specific_argh_collision(i, x, y)) {
            retVal = i;
            break;
        }
    }
    return retVal;
}

/**
 * Checks wether the worm collides with the food at the specfied food-arrays.
 * @param int foodIndex The index of the food in the arrays. Ensure the value is
 * 0 <= foodIndex <= MAX_FOOD
 * @return Returns true if the worm collides with the specified food.
 */
static bool worm_food_collision(struct worm *w, int foodIndex)
{
    bool retVal = false;

    retVal = worm_in_rect(w, foodx[foodIndex], foody[foodIndex], 
                          FOOD_SIZE - 1, FOOD_SIZE - 1);

    return retVal;
}

/**
 * Returns true if the worm hits the argh within the next moves (unless
 * the worm changes it's direction).
 * @param struct worm *w - The worm to investigate
 * @param int argh_idx - The index of the argh 
 * @param int moves - The number of moves that are considered.
 * @return Returns false if the specified argh is not hit within the next
 *         moves.
 */
static bool worm_argh_collision_in_moves(struct worm *w, int argh_idx, int moves){
    bool retVal = false;
    int x1, y1, x2, y2;
    x1 = w->x[w->head];
    y1 = w->y[w->head];

    x2 = w->x[w->head] + moves * w->dirx;
    y2 = w->y[w->head] + moves * w->diry;

    retVal = line_in_rect(x1, y1, x2, y2, arghx[argh_idx], arghy[argh_idx], 
                         ARGH_SIZE, ARGH_SIZE);
    return retVal;
}

/**
 * Checks wether the worm collides with the argh at the specfied argh-arrays.
 * @param int arghIndex The index of the argh in the arrays.
 * Ensure the value is 0 <= arghIndex < argh_count <= MAX_ARGH
 * @return Returns true if the worm collides with the specified argh.
 */
static bool worm_argh_collision(struct worm *w, int arghIndex)
{
    bool retVal = false;

    retVal = worm_in_rect(w, arghx[arghIndex], arghy[arghIndex], 
                          ARGH_SIZE - 1, ARGH_SIZE - 1);

    return retVal;
}

/**
 * Find new coordinates for the food stored in foodx[index], foody[index]
 * that don't collide with any other food or argh
 * @param int index 
 * Ensure that 0 <= index < MAX_FOOD.
 */
static int make_food(int index) {

    int x = 0;
    int y = 0;
    bool collisionDetected = false;
    int tries = 0;
    int i;

    do {
        /* make coordinates for a new food so that
           the entire food lies within the FIELD */
        x = rb->rand() % (FIELD_RECT_WIDTH  - FOOD_SIZE);
        y = rb->rand() % (FIELD_RECT_HEIGHT - FOOD_SIZE);
        tries ++;

        /* Ensure that the new food doesn't collide with any
           existing foods or arghs.
           If one or more corners of the new food hit any existing
           argh or food a collision is detected.
        */
        collisionDetected = 
            food_collision(x                , y                ) >= 0 ||
            food_collision(x                , y + FOOD_SIZE - 1) >= 0 ||
            food_collision(x + FOOD_SIZE - 1, y                ) >= 0 ||
            food_collision(x + FOOD_SIZE - 1, y + FOOD_SIZE - 1) >= 0 ||
            argh_collision(x                , y                ) >= 0 || 
            argh_collision(x                , y + FOOD_SIZE - 1) >= 0 ||
            argh_collision(x + FOOD_SIZE - 1, y                ) >= 0 ||
            argh_collision(x + FOOD_SIZE - 1, y + FOOD_SIZE - 1) >= 0;

        /* use coordinates for further testing */
        foodx[index] = x;
        foody[index] = y;

        /* now test wether we accidently hit the worm with food ;) */
        i = 0;
        for (i = 0; i < worm_count && !collisionDetected; i++) {
            collisionDetected |= worm_food_collision(&worms[i], index);
        }
    }
    while (collisionDetected);
    return tries;
}

/**
 * Clears a food from the lcd buffer.
 * @param int index The index of the food arrays under which
 * the coordinates of the desired food can be found. Ensure 
 * that the value is 0 <= index <= MAX_FOOD.
 */
static void clear_food(int index)
{
    /* remove the old food from the screen */
    rb->lcd_clearrect(foodx[index] + FIELD_RECT_X, 
                  foody[index] + FIELD_RECT_Y, 
                  FOOD_SIZE, FOOD_SIZE);
}

/**
 * Draws a food in the lcd buffer.
 * @param int index The index of the food arrays under which
 * the coordinates of the desired food can be found. Ensure 
 * that the value is 0 <= index <= MAX_FOOD.
 */
static void draw_food(int index)
{
    /* draw the food object */
    rb->lcd_fillrect(foodx[index] + FIELD_RECT_X, 
                 foody[index] + FIELD_RECT_Y, 
                 FOOD_SIZE, FOOD_SIZE);
    rb->lcd_clearrect(foodx[index] + FIELD_RECT_X + 1, 
                  foody[index] + FIELD_RECT_Y + 1, 
                  FOOD_SIZE - 2, FOOD_SIZE - 2);
}

/**
 * Find new coordinates for the argh stored in arghx[index], arghy[index]
 * that don't collide with any other food or argh.
 * @param int index 
 * Ensure that 0 <= index < argh_count < MAX_ARGH.
 */
static int make_argh(int index)
{
    int x = -1; 
    int y = -1; 
    bool collisionDetected = false;
    int tries = 0;
    int i;

    do {
        /* make coordinates for a new argh so that
           the entire food lies within the FIELD */
        x = rb->rand() % (FIELD_RECT_WIDTH  - ARGH_SIZE);
        y = rb->rand() % (FIELD_RECT_HEIGHT - ARGH_SIZE);
        tries ++;

        /* Ensure that the new argh doesn't intersect with any
           existing foods or arghs.
           If one or more corners of the new argh hit any existing
           argh or food an intersection is detected.
        */
        collisionDetected = 
            food_collision(x                , y                ) >= 0 || 
            food_collision(x                , y + ARGH_SIZE - 1) >= 0 ||
            food_collision(x + ARGH_SIZE - 1, y                ) >= 0 ||
            food_collision(x + ARGH_SIZE - 1, y + ARGH_SIZE - 1) >= 0 ||
            argh_collision(x                , y                ) >= 0 || 
            argh_collision(x                , y + ARGH_SIZE - 1) >= 0 ||
            argh_collision(x + ARGH_SIZE - 1, y                ) >= 0 ||
            argh_collision(x + ARGH_SIZE - 1, y + ARGH_SIZE - 1) >= 0;

        /* use the candidate coordinates to make a real argh */
        arghx[index] = x;
        arghy[index] = y;

        /* now test wether we accidently hit the worm with argh ;) */
        for (i = 0; i < worm_count && !collisionDetected; i++) {
            collisionDetected |= worm_argh_collision(&worms[i], index);
            collisionDetected |= worm_argh_collision_in_moves(&worms[i], index, 
                                                              MIN_ARGH_DIST);
        }
    }
    while (collisionDetected);
    return tries;
}

/**
 * Draws an argh in the lcd buffer.
 * @param int index The index of the argh arrays under which
 * the coordinates of the desired argh can be found. Ensure 
 * that the value is 0 <= index < argh_count <= MAX_ARGH.
 */
static void draw_argh(int index)
{
    /* draw the new argh */
    rb->lcd_fillrect(arghx[index] + FIELD_RECT_X, 
                 arghy[index] + FIELD_RECT_Y, 
                 ARGH_SIZE, ARGH_SIZE);
}

static void virtual_player(struct worm *w);
/**
 * Initialzes the specified worm with INITIAL_WORM_LENGTH
 * and the tail at the specified position. The worm will
 * be initialized alive and creeping EAST.
 * @param struct worm *w The worm that is to be initialized
 * @param int x The x coordinate at which the tail of the worm starts.
 *        x must be 0 <= x < FIELD_RECT_WIDTH.
 * @param int y The y coordinate at which the tail of the worm starts
 *        y must be 0 <= y < FIELD_RECT_WIDTH.
 */
static void init_worm(struct worm *w, int x, int y){
        /* initialize the worm size */
        w->head = 1;
        w->tail = 0;

        w->x[w->head] = x + 1;
        w->y[w->head] = y;

        w->x[w->tail] = x;
        w->y[w->tail] = y;

        /* set the initial direction the worm creeps to */
        w->dirx = 1;
        w->diry = 0;

        w->growing = INITIAL_WORM_LENGTH - 1;
        w->alive = true;
        w->fetch_worm_direction = virtual_player;
}

/** 
 * Writes the direction that was stored for
 * human player 1 into the specified worm. This function
 * may be used to be stored in worm.fetch_worm_direction. 
 * The value of 
 * the direction is read from player1_dir.
 * @param struct worm *w - The worm of which the direction 
 * is altered.
 */
static void human_player1(struct worm *w) {
    set_worm_dir(w, player1_dir);
}

/** 
 * Writes the direction that was stored for
 * human player 2 into the specified worm. This function
 * may be used to be stored in worm.fetch_worm_direction. 
 * The value of 
 * the direction is read from player2_dir.
 * @param struct worm *w - The worm of which the direction 
 * is altered.
 */
static void human_player2(struct worm *w) {
    set_worm_dir(w, player2_dir);
}

/** 
 * Writes the direction that was stored for
 * human player using a remote control 
 * into the specified worm. This function
 * may be used to be stored in worm.fetch_worm_direction. 
 * The value of 
 * the direction is read from player3_dir.
 * @param struct worm *w - The worm of which the direction 
 * is altered.
 */
static void remote_player(struct worm *w) {
    set_worm_dir(w, player3_dir);
}

/**
 * Initializes the worm-, food- and argh-arrays, draws a frame,
 * makes some food and argh and display all that stuff.
 */
static void init_wormlet(void)
{
    int i;

    for (i = 0; i< worm_count; i++) {
            /* Initialize all the worm coordinates to center. */
            int x = (int)(FIELD_RECT_WIDTH / 2);
            int y = (int)((FIELD_RECT_HEIGHT - 20)/ 2) + i * 10;

        init_worm(&worms[i], x, y);
    }

    player1_dir = EAST;
    player2_dir = EAST;
    player3_dir = EAST;

    if (players > 0) {
        worms[0].fetch_worm_direction = human_player1;
    }

    if (players > 1) {
        if (use_remote) {
            worms[1].fetch_worm_direction = remote_player;
        } else {
            worms[1].fetch_worm_direction = human_player2;
        }
    }

    if (players > 2) {
        worms[2].fetch_worm_direction = human_player2;
    }

    /* Needed when the game is restarted using BUTTON_ON */
    rb->lcd_clear_display();

    /* make and display some food and argh */
    argh_count = MAX_FOOD;
    for (i = 0; i < MAX_FOOD; i++) {
        make_food(i);
        draw_food(i);
        make_argh(i);
        draw_argh(i);
    }

    /* draw the game field */
    rb->lcd_invertrect(0, 0, FIELD_RECT_WIDTH + 2, FIELD_RECT_HEIGHT + 2);
    rb->lcd_invertrect(1, 1, FIELD_RECT_WIDTH, FIELD_RECT_HEIGHT);

    /* make everything visible */
    rb->lcd_update();
}


/** 
 * Move the worm one step further if it is alive.
 * The direction in which the worm moves is taken from dirx and diry. 
 * move_worm decreases growing if > 0. While the worm is growing the tail
 * is left untouched.
 * @param struct worm *w The worm to move. w must not be NULL.
 */
static void move_worm(struct worm *w)
{
    if (w->alive) {
        /* determine the head point and its precessor */
        int headx = w->x[w->head];
        int heady = w->y[w->head];
        int prehead = (w->head + MAX_WORM_SEGMENTS - 1) % MAX_WORM_SEGMENTS;
        int preheadx = w->x[prehead];
        int preheady = w->y[prehead];

        /* determine the old direction */
        int olddirx;
        int olddiry;
        if (headx == preheadx) {
            olddirx = 0;
            olddiry = (heady > preheady) ? 1 : -1;
        } else {
            olddiry = 0;
            olddirx = (headx > preheadx) ? 1 : -1;
        }

        /* olddir == dir?
           a change of direction means a new segment 
           has been opened */
        if (olddirx != w->dirx ||
            olddiry != w->diry) {
            w->head = (w->head + 1) % MAX_WORM_SEGMENTS;
        }

        /* new head position */
        w->x[w->head] = headx + w->dirx;
        w->y[w->head] = heady + w->diry;


        /* while the worm is growing no tail procession is necessary */
        if (w->growing > 0) {
    /* update the worms grow state */
            w->growing--;
        }
        
        /* if the worm isn't growing the tail has to be dragged */
        else {
            /* index of the end of the tail segment */ 
            int tail_segment_end = (w->tail + 1) % MAX_WORM_SEGMENTS;

            /* drag the end of the tail */
            /* only one coordinate has to be altered. Here it is 
               determined which one */ 
            int dir = 0; /* specifies wether the coord has to be in- or decreased */
            if (w->x[w->tail] == w->x[tail_segment_end]) {
                dir = (w->y[w->tail] - w->y[tail_segment_end] < 0) ? 1 : -1;
                w->y[w->tail] += dir;
            } else {
                dir = (w->x[w->tail] - w->x[tail_segment_end] < 0) ? 1 : -1;
                w->x[w->tail] += dir;
            }

            /* when the tail has been dragged so far that it meets
               the next segment start the tail segment is obsolete and
               must be freed */
            if (w->x[w->tail] == w->x[tail_segment_end] &&
                w->y[w->tail] == w->y[tail_segment_end]){
                
                /* drop the last tail point */
                w->tail = tail_segment_end;
            }
        }
    }
}

/**
 * Draws the head and clears the tail of the worm in 
 * the display buffer. lcd_update() is NOT called thus
 * the caller has to take care that the buffer is displayed.
 */
static void draw_worm(struct worm *w)
{
    /* draw the new head */
    int x = w->x[w->head];
    int y = w->y[w->head];
    if (x >= 0 && x < FIELD_RECT_WIDTH && y >= 0 && y < FIELD_RECT_HEIGHT) {
        rb->lcd_drawpixel(x + FIELD_RECT_X, y + FIELD_RECT_Y);
    }

    /* clear the space behind the worm */
    x = w->x[w->tail] ;
    y = w->y[w->tail] ;
    if (x >= 0 && x < FIELD_RECT_WIDTH && y >= 0 && y < FIELD_RECT_HEIGHT) {
        rb->lcd_clearpixel(x + FIELD_RECT_X, y + FIELD_RECT_Y);
    }
}

/**
 * Checks wether the coordinate is part of the worm. Returns
 * true if any part of the worm was hit - including the head.
 * @param x int The x coordinate 
 * @param y int The y coordinate
 * @return int The index of the worm arrays that contain x, y.
 * Returns -1 if the coordinates are not part of the worm.
 */ 
static int specific_worm_collision(struct worm *w, int x, int y)
{
    int retVal = -1;

    /* get_worm_array_length is expensive -> buffer the value */
    int wormLength = get_worm_array_length(w);
    int i;

    /* test each entry that is part of the worm */
    for (i = 0; i < wormLength && retVal == -1; i++) {

        /* The iteration iterates the length of the worm.
           Here's the conversion to the true indices within the worm arrays. */
        int linestart = (w->tail + i  ) % MAX_WORM_SEGMENTS;
        int lineend   = (linestart + 1) % MAX_WORM_SEGMENTS;
        bool samex = (w->x[linestart] == x) && (w->x[lineend] == x);
        bool samey = (w->y[linestart] == y) && (w->y[lineend] == y);
        if (samex || samey){
            int test, min, max, tmp;

            if (samey) {
                min = w->x[linestart];
                max = w->x[lineend];
                test = x; 
            } else {
                min = w->y[linestart];
                max = w->y[lineend];
                test = y;
            }

            tmp = min;
            min = MIN(min, max);
            max = MAX(tmp, max);

            if (min <= test && test <= max) {
                retVal = lineend;
            }
        }
    }
    return retVal;
}

/**
 * Increases the length of the specified worm by marking 
 * that it may grow by len pixels. Note that the worm has
 * to move to make the growing happen.
 * @param worm *w The worm that is to be altered.
 * @param int len A positive value specifying the amount of
 * pixels the worm may grow.
 */
static void add_growing(struct worm *w, int len) {
    w->growing += len;
}

/**
 * Determins the worm that is at the coordinates x, y. The parameter
 * w is a switch parameter that changes the functionality of worm_collision.
 * If w is specified and x,y hits the head of w NULL is returned.
 * This is a useful way to determine wether the head of w hits 
 * any worm but including itself but excluding its own head. 
 * (It hits always its own head ;))
 * If w is set to NULL worm_collision returns any worm including all heads
 * that is at position of x,y.
 * @param struct worm *w The worm of which the head should be excluded in
 * the test. w may be set to NULL.
 * @param int x The x coordinate that is checked
 * @param int y The y coordinate that is checkec
 * @return struct worm*  The worm that has been hit by x,y. If no worm
 * was at the position NULL is returned.
 */
static struct worm* worm_collision(struct worm *w, int x, int y){
    struct worm *retVal = NULL;
    int i;
    for (i = 0; (i < worm_count) && (retVal == NULL); i++) {
        int collision_at = specific_worm_collision(&worms[i], x, y);
        if (collision_at != -1) {
            if (!(w == &worms[i] && collision_at == w->head)){
                retVal = &worms[i];
            }
        }
    }
    return retVal;
}

/**
 * Returns true if the head of the worm just has
 * crossed the field boundaries. 
 * @return bool true if the worm just has wrapped.
 */ 
static bool field_collision(struct worm *w)
{
    bool retVal = false;
    if ((w->x[w->head] >= FIELD_RECT_WIDTH)  ||
        (w->y[w->head] >= FIELD_RECT_HEIGHT) ||
        (w->x[w->head] < 0)                  ||
        (w->y[w->head] < 0))
    {
        retVal = true;
    }
    return retVal;
}


/**
 * Returns true if the specified coordinates are within the 
 * field specified by the FIELD_RECT_XXX constants.
 * @param int x The x coordinate of the point that is investigated
 * @param int y The y coordinate of the point that is investigated
 * @return bool Returns false if x,y specifies a point outside the 
 * field of worms.
 */
static bool is_in_field_rect(int x, int y) {
    bool retVal = false;
    retVal = (x >= 0 && x < FIELD_RECT_WIDTH &&
              y >= 0 && y < FIELD_RECT_HEIGHT);
    return retVal;
}

/**
 * Checks and returns wether the head of the w
 * is colliding with something currently.
 * @return int One of the values:
 *   COLLISION_NONE
 *   COLLISION_w
 *   COLLISION_FOOD
 *   COLLISION_ARGH
 *   COLLISION_FIELD
 */
static int check_collision(struct worm *w)
{
    int retVal = COLLISION_NONE;

    if (worm_collision(w, w->x[w->head], w->y[w->head]) != NULL)
        retVal = COLLISION_WORM;

    if (food_collision(w->x[w->head], w->y[w->head]) >= 0)
        retVal = COLLISION_FOOD;

    if (argh_collision(w->x[w->head], w->y[w->head]) >= 0)
        retVal = COLLISION_ARGH;

    if (field_collision(w))
        retVal = COLLISION_FIELD;

    return retVal;
}

/**
 * Returns the index of the food that is closest to the point
 * specified by x, y. This index may be used in the foodx and 
 * foody arrays.
 * @param int x The x coordinate of the point
 * @param int y The y coordinate of the point
 * @return int A value usable as index in foodx and foody.
 */
static int get_nearest_food(int x, int y){
    int nearestfood = 0;
    int olddistance = FIELD_RECT_WIDTH + FIELD_RECT_HEIGHT;
    int deltax = 0;
    int deltay = 0;
    int foodindex;
    for (foodindex = 0; foodindex < MAX_FOOD; foodindex++) {
        int distance;
        deltax = foodx[foodindex] - x;
        deltay = foody[foodindex] - y;
        deltax = deltax > 0 ? deltax : deltax * (-1);
        deltay = deltay > 0 ? deltay : deltay * (-1);
        distance = deltax + deltay;

        if (distance < olddistance) {
            olddistance = distance;
            nearestfood = foodindex;
        }
    }
    return nearestfood;
}

/** 
 * Returns wether the specified position is next to the worm
 * and in the direction the worm looks. Use this method to 
 * test wether this position would be hit with the next move of 
 * the worm unless the worm changes its direction.
 * @param struct worm *w - The worm to be investigated
 * @param int x - The x coordinate of the position to test.
 * @param int y - The y coordinate of the position to test.
 * @return Returns true if the worm will hit the position unless
 * it change its direction before the next move.
 */
static bool is_in_front_of_worm(struct worm *w, int x, int y) {
    bool infront = false;
    int deltax = x - w->x[w->head];
    int deltay = y - w->y[w->head];

    if (w->dirx == 0) {
        infront = (w->diry * deltay) > 0;
    } else {
        infront = (w->dirx * deltax) > 0;
    }
    return infront;
}

/**
 * Returns true if the worm will collide with the next move unless
 * it changes its direction.
 * @param struct worm *w - The worm to be investigated.
 * @return Returns true if the worm will collide with the next move
 * unless it changes its direction.
 */
static bool will_worm_collide(struct worm *w) {
    int x = w->x[w->head] + w->dirx;
    int y = w->y[w->head] + w->diry;
    bool retVal = !is_in_field_rect(x, y);
    if (!retVal) {
        retVal = (argh_collision(x, y) != -1);
    }
    
    if (!retVal) {
        retVal = (worm_collision(w, x, y) != NULL);
    }
    return retVal;
}

/** 
 * This function
 * may be used to be stored in worm.fetch_worm_direction for
 * worms that are not controlled by humans but by artificial stupidity. 
 * A direction is searched that doesn't lead to collision but to the nearest
 * food - but not very intelligent. The direction is written to the specified
 * worm.
 * @param struct worm *w - The worm of which the direction 
 * is altered.
 */
static void virtual_player(struct worm *w) {
    bool isright;
    int plana, planb, planc;
    /* find the next lunch */
    int nearestfood = get_nearest_food(w->x[w->head], w->y[w->head]);

    /* determine in which direction it is */

    /* in front of me? */
    bool infront = is_in_front_of_worm(w, foodx[nearestfood], foody[nearestfood]);

    /* left right of me? */
    int olddir = get_worm_dir(w);
    set_worm_dir(w, (olddir + 1) % 4);
    isright = is_in_front_of_worm(w, foodx[nearestfood], foody[nearestfood]);
    set_worm_dir(w, olddir);

    /* detect situation, set strategy */
    if (infront) {
        if (isright) {
            plana = olddir;
            planb = (olddir + 1) % 4;
            planc = (olddir + 3) % 4;
        } else {
            plana = olddir;
            planb = (olddir + 3) % 4;
            planc = (olddir + 1) % 4;
        }
    } else {
        if (isright) {
            plana = (olddir + 1) % 4;
            planb = olddir;
            planc = (olddir + 3) % 4;
        } else {
            plana = (olddir + 3) % 4;
            planb = olddir;
            planc = (olddir + 1) % 4;
        }
    }

    /* test for collision */
    set_worm_dir(w, plana);
    if (will_worm_collide(w)){

        /* plan b */
        set_worm_dir(w, planb);

        /* test for collision */
        if (will_worm_collide(w)) {

            /* plan c */
            set_worm_dir(w, planc);
        }
    }
}

/**
 * prints out the score board with all the status information
 * about the game.
 */
static void score_board(void)
{
    char buf[15];
    char* buf2 = NULL;
    int i;
    int y = 0;
    rb->lcd_clearrect(FIELD_RECT_WIDTH + 2, 0, LCD_WIDTH - FIELD_RECT_WIDTH - 2, LCD_HEIGHT);
    for (i = 0; i < worm_count; i++) {
        int score = get_score(&worms[i]);
    
        /* high score */
        if (worms[i].fetch_worm_direction != virtual_player){
            if (highscore < score) {
                highscore = score;
            }
        }

        /* length */
        rb->snprintf(buf, sizeof (buf),"Len:%d", score);

        /* worm state */
        switch (check_collision(&worms[i])) {
            case COLLISION_NONE:  
                if (worms[i].growing > 0)
                    buf2 = "Growing";
                else {
                    if (worms[i].alive)
                        buf2 = "Hungry";
                    else
                        buf2 = "Wormed";
                }
                break;

            case COLLISION_WORM:  
                buf2 = "Wormed";
                break;

            case COLLISION_FOOD:  
                buf2 = "Growing";
                break;

            case COLLISION_ARGH:  
                buf2 = "Argh";
                break;

            case COLLISION_FIELD: 
                buf2 = "Crashed";
                break;
        }
        rb->lcd_putsxy(FIELD_RECT_WIDTH + 3, y  , buf);
        rb->lcd_putsxy(FIELD_RECT_WIDTH + 3, y+8, buf2);

        if (!worms[i].alive){
            rb->lcd_invertrect(FIELD_RECT_WIDTH + 2, y, 
                               LCD_WIDTH - FIELD_RECT_WIDTH - 2, 17);
        }
        y += 19;
    }
    rb->snprintf(buf , sizeof(buf), "Hs: %d", highscore);
#ifndef DEBUG_WORMLET
    rb->lcd_putsxy(FIELD_RECT_WIDTH + 3, LCD_HEIGHT - 8, buf);
#else
    rb->lcd_putsxy(FIELD_RECT_WIDTH + 3, LCD_HEIGHT - 8, debugout);
#endif
}

/**
 * Checks for collisions of the worm and its environment and
 * takes appropriate actions like growing the worm or killing it.
 * @return bool Returns true if the worm is dead. Returns
 * false if the worm is healthy, up and creeping.
 */
static bool process_collisions(struct worm *w)
{
    int index = -1;

    w->alive &= !field_collision(w);

    if (w->alive) {

        /* check if food was eaten */
        index = food_collision(w->x[w->head], w->y[w->head]);
        if (index != -1){
            int i;
            
            clear_food(index);
            make_food(index);
            draw_food(index);
            
            for (i = 0; i < ARGHS_PER_FOOD; i++) {
                argh_count++;
                if (argh_count > MAX_ARGH)
                    argh_count = MAX_ARGH;
                make_argh(argh_count - 1);
                draw_argh(argh_count - 1);
            }

            add_growing(w, WORM_PER_FOOD);

            draw_worm(w);
        }

        /* check if argh was eaten */
        else {
            index = argh_collision(w->x[w->head], w->y[w->head]);
            if (index != -1) {
                w->alive = false;
        }
            else {
                if (worm_collision(w, w->x[w->head], w->y[w->head]) != NULL) {
                    w->alive = false;
    }
            }
        }
    }
    return !w->alive;
}

/**
 * The main loop of the game.
 * @return bool Returns true if the game ended
 * with a dead worm. Returns false if the user
 * aborted the game manually.
 */ 
static bool run(void)
{
    int button = 0;
    int wormDead = false;

    /* ticks are counted to compensate speed variations */
    long cycle_start = 0, cycle_end = 0;
#ifdef DEBUG_WORMLET
    int ticks_to_max_cycle_reset = 20;
    long max_cycle = 0;
    char buf[20];
#endif

    /* initialize the board and so on */
    init_wormlet();

    cycle_start = *rb->current_tick;
    /* change the direction of the worm */
    while (button != BUTTON_OFF && ! wormDead)
    {
        int i;
        long cycle_duration ;
        switch (button) {
            case BUTTON_UP:
                if (players == 1 && !use_remote) {
                    player1_dir = NORTH;
                }
                break;

            case BUTTON_DOWN:
                if (players == 1 && !use_remote) {
                    player1_dir = SOUTH;
                }
                break;

            case BUTTON_LEFT:
                if (players != 1 || use_remote) {
                    player1_dir = (player1_dir + 3) % 4;
                } else {
                    player1_dir = WEST;
                }
                break;

            case BUTTON_RIGHT:
                if (players != 1 || use_remote) {
                    player1_dir = (player1_dir + 1) % 4;
                } else {
                    player1_dir = EAST;
                }
                break;

            case BUTTON_F2:
                player2_dir = (player2_dir + 3) % 4;
                break;

            case BUTTON_F3:
                player2_dir = (player2_dir + 1) % 4;
                break;

            case BUTTON_RC_VOL_UP:
                player3_dir = (player3_dir + 1) % 4;
                break;

            case BUTTON_RC_VOL_DOWN:
                player3_dir = (player3_dir + 3) % 4;
                break;

            case BUTTON_PLAY:
                do {
                    button = rb->button_get(true);
                } while (button != BUTTON_PLAY &&
                         button != BUTTON_OFF  &&
                         button != BUTTON_ON);
                break;
        }

        for (i = 0; i < worm_count; i++) {
            worms[i].fetch_worm_direction(&worms[i]);
        }

        wormDead = true;
        for (i = 0; i < worm_count; i++){
            struct worm *w = &worms[i];
            move_worm(w);
            wormDead &= process_collisions(w);
            draw_worm(w);
        }
        score_board();
        rb->lcd_update();
        if (button == BUTTON_ON) {
            wormDead = true;
        }

        /* here the wormlet game cycle ends
           thus the current tick is stored
           as end time */
        cycle_end = *rb->current_tick;

        /* The duration of the game cycle */
        cycle_duration = cycle_end - cycle_start;
        cycle_duration = MAX(0, cycle_duration);
        cycle_duration = MIN(SPEED -1, cycle_duration);


#ifdef DEBUG_WORMLET
        ticks_to_max_cycle_reset--;
        if (ticks_to_max_cycle_reset <= 0) {
            max_cycle = 0;
        }

        if (max_cycle < cycle_duration) {
            max_cycle = cycle_duration;
            ticks_to_max_cycle_reset = 20;
        }
        rb->snprintf(buf, sizeof buf, "ticks %d", max_cycle);
        set_debug_out(buf);
#endif
        /* adjust the number of ticks to wait for a button.
           This ensures that a complete game cycle including
           user input runs in constant time */
        button = rb->button_get_w_tmo(SPEED - cycle_duration);
        cycle_start = *rb->current_tick;
    }
    return wormDead;
}

#ifdef DEBUG_WORMLET

/**
 * Just a test routine that checks that worm_food_collision works
 * in some typical situations.
 */
static void test_worm_food_collision(void) {
    int collision_count = 0;
    int i;
    rb->lcd_clear_display();
    init_worm(&worms[0], 10, 10);
    add_growing(&worms[0], 10);
    set_worm_dir(&worms[0], EAST);
    for (i = 0; i < 10; i++) {
        move_worm(&worms[0]);
        draw_worm(&worms[0]);
    }

    set_worm_dir(&worms[0], SOUTH);
    for (i = 0; i < 10; i++) {
        move_worm(&worms[0]);
        draw_worm(&worms[0]);
    }

    foodx[0] = 15;
    foody[0] = 12;
    for (foody[0] = 20; foody[0] > 0; foody[0] --) {
        char buf[20];
        bool collision;
        draw_worm(&worms[0]);
        draw_food(0);
        collision = worm_food_collision(&worms[0], 0);
        if (collision) {
            collision_count++;
        }
        rb->snprintf(buf, sizeof buf, "collisions: %d", collision_count);
        rb->lcd_putsxy(0, LCD_HEIGHT -8, buf);
        rb->lcd_update();
    }
    if (collision_count != FOOD_SIZE) {
        rb->button_get(true);
    }


    foody[0] = 15;
    for (foodx[0] = 30; foodx[0] > 0; foodx[0] --) {
        char buf[20];
        bool collision;
        draw_worm(&worms[0]);
        draw_food(0);
        collision = worm_food_collision(&worms[0], 0);
        if (collision) {
            collision_count ++;
        }
        rb->snprintf(buf, sizeof buf, "collisions: %d", collision_count);
        rb->lcd_putsxy(0, LCD_HEIGHT -8, buf);
        rb->lcd_update();
    }
    if (collision_count != FOOD_SIZE * 2) {
        rb->button_get(true);
    }

}


static bool expensive_worm_in_rect(struct worm *w, int rx, int ry, int rw, int rh){
    int x, y;
    bool retVal = false;
    for (x = rx; x < rx + rw; x++){
        for (y = ry; y < ry + rh; y++) {
            if (specific_worm_collision(w, x, y) != -1) {
                retVal = true;
            }
        }
    }
    return retVal;
}

static void test_worm_argh_collision(void) {
    int i;
    int dir;
    int collision_count = 0;
    rb->lcd_clear_display();
    init_worm(&worms[0], 10, 10);
    add_growing(&worms[0], 40);
    for (dir = 0; dir < 4; dir++) {
        set_worm_dir(&worms[0], (EAST + dir) % 4);
        for (i = 0; i < 10; i++) {
            move_worm(&worms[0]);
            draw_worm(&worms[0]);
        }
    }

    arghx[0] = 12;
    for (arghy[0] = 0; arghy[0] < FIELD_RECT_HEIGHT - ARGH_SIZE; arghy[0]++){
        char buf[20];
        bool collision;
        draw_argh(0);
        collision = worm_argh_collision(&worms[0], 0);
        if (collision) {
            collision_count ++;
        }
        rb->snprintf(buf, sizeof buf, "collisions: %d", collision_count);
        rb->lcd_putsxy(0, LCD_HEIGHT -8, buf);
        rb->lcd_update();
    }
    if (collision_count != ARGH_SIZE * 2) {
        rb->button_get(true);
    }

    arghy[0] = 12;
    for (arghx[0] = 0; arghx[0] < FIELD_RECT_HEIGHT - ARGH_SIZE; arghx[0]++){
        char buf[20];
        bool collision;
        draw_argh(0);
        collision = worm_argh_collision(&worms[0], 0);
        if (collision) {
            collision_count ++;
        }
        rb->snprintf(buf, sizeof buf, "collisions: %d", collision_count);
        rb->lcd_putsxy(0, LCD_HEIGHT -8, buf);
        rb->lcd_update();
    }
    if (collision_count != ARGH_SIZE * 4) {
        rb->button_get(true);
    }
}

static int testline_in_rect(void) {
    int testfailed = -1;

    int rx = 10;
    int ry = 15;
    int rw = 20;
    int rh = 25;

    /* Test 1 */
    int x1 = 12;
    int y1 = 8;
    int x2 = 12;
    int y2 = 42;

    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_update();
        rb->lcd_putsxy(0, 0, "failed 1");
        rb->button_get(true);
        testfailed = 1;
    }

    /* test 2 */
    y2 = 20;
    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 2");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 2;
    }

    /* test 3 */
    y1 = 30;
    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 3");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 3;
    }

    /* test 4 */
    y2 = 45;
    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 4");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 4;
    }

    /* test 5 */
    y1 = 50;
    if (line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) ||
        line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 5");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 5;
    }

    /* test 6 */
    y1 = 5;
    y2 = 7;
    if (line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) ||
        line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 6");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 6;
    }

    /* test 7 */
    x1 = 8;
    y1 = 20;
    x2 = 35;
    y2 = 20;
    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 7");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 7;
    }

    /* test 8 */
    x2 = 12;
    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 8");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 8;
    }

    /* test 9 */
    x1 = 25;
    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 9");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 9;
    }

    /* test 10 */
    x2 = 37;
    if (!line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
        !line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 10");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 10;
    }

    /* test 11 */
    x1 = 42;
    if (line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) ||
        line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 11");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 11;
    }

    /* test 12 */
    x1 = 5;
    x2 = 7;
    if (line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) ||
        line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh)) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 12");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 12;
    }

    /* test 13 */
    rx = 9;
    ry = 15;
    rw = FOOD_SIZE;
    rh = FOOD_SIZE;

    x1 = 10;
    y1 = 10;
    x2 = 10;
    y2 = 20;
    if (!(line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
          line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh))) {
        rb->lcd_drawrect(rx, ry, rw, rh);
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_putsxy(0, 0, "failed 13");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 13;
    }

    /* test 14 */
    rx = 9;
    ry = 15;
    rw = 4;
    rh = 4;

    x1 = 10;
    y1 = 10;
    x2 = 10;
    y2 = 19;
    if (!(line_in_rect(x1, y1, x2, y2, rx, ry, rw, rh) &&
          line_in_rect(x2, y2, x1, y1, rx, ry, rw, rh))) {
        rb->lcd_drawline(x1, y1, x2, y2);
        rb->lcd_invertrect(rx, ry, rw, rh);
        rb->lcd_putsxy(0, 0, "failed 14");
        rb->lcd_update();
        rb->button_get(true);
        testfailed = 14;
    }

    rb->lcd_clear_display();

    return testfailed;
}

/**
 * Just a test routine to test wether specific_worm_collision might work properly
 */
static int test_specific_worm_collision(void) {
    int collisions = 0;
    int dir;
    int x = 0;
    int y = 0;
    char buf[20];
    rb->lcd_clear_display();
    init_worm(&worms[0], 10, 20);
    add_growing(&worms[0], 20 - INITIAL_WORM_LENGTH);

    for (dir = EAST; dir < EAST + 4; dir++) {
        int i;
        set_worm_dir(&worms[0], dir % 4);
        for (i = 0; i < 5; i++) {
            if (!(dir % 4 == NORTH && i == 9)) {
                move_worm(&worms[0]);
                draw_worm(&worms[0]);
            }
        }
    }

    for (y = 15; y < 30; y ++){
        for (x = 5; x < 20; x++) {
            if (specific_worm_collision(&worms[0], x, y) != -1) {
                collisions ++;
            }
            rb->lcd_invertpixel(x + FIELD_RECT_X, y + FIELD_RECT_Y);
            rb->snprintf(buf, sizeof buf, "collisions %d", collisions);
            rb->lcd_putsxy(0, LCD_HEIGHT - 8, buf);
            rb->lcd_update();
        }
    }
    if (collisions != 21) {
        rb->button_get(true);
    }
    return collisions;
}

static void test_make_argh(void){
    int dir;
    int seed = 0;
    int hit = 0;
    int failures = 0;
    int last_failures = 0;
    int i, worm_idx;
    rb->lcd_clear_display();
    worm_count = 3;

    for (worm_idx = 0; worm_idx < worm_count; worm_idx++) {
        init_worm(&worms[worm_idx], 10 + worm_idx * 20, 20);
        add_growing(&worms[worm_idx], 40 - INITIAL_WORM_LENGTH);
    }

    for (dir = EAST; dir < EAST + 4; dir++) {
        for (worm_idx = 0; worm_idx < worm_count; worm_idx++) {
            set_worm_dir(&worms[worm_idx], dir % 4);
        for (i = 0; i < 10; i++) {
            if (!(dir % 4 == NORTH && i == 9)) {
                    move_worm(&worms[worm_idx]);
                    draw_worm(&worms[worm_idx]);
                }
            }
        }
    }

    rb->lcd_update();

    for (seed = 0; hit < 20; seed += 2) {
            char buf[20];
        int x, y;
        rb->srand(seed);
        x = rb->rand() % (FIELD_RECT_WIDTH  - ARGH_SIZE);
        y = rb->rand() % (FIELD_RECT_HEIGHT - ARGH_SIZE);

        for (worm_idx = 0; worm_idx < worm_count; worm_idx++){
            if (expensive_worm_in_rect(&worms[worm_idx], x, y, ARGH_SIZE, ARGH_SIZE)) {
                int tries = 0;
            rb->srand(seed);

                tries = make_argh(0);
                if ((x == arghx[0] && y == arghy[0]) || tries < 2) {
                failures ++;
            }

                rb->snprintf(buf, sizeof buf, "(%d;%d) fail%d try%d", x, y, failures, tries);
            rb->lcd_putsxy(0, LCD_HEIGHT - 8, buf);
            rb->lcd_update();
                rb->lcd_invertrect(x + FIELD_RECT_X, y+ FIELD_RECT_Y, ARGH_SIZE, ARGH_SIZE);
                rb->lcd_update();
                draw_argh(0);
                rb->lcd_update();
            rb->lcd_invertrect(x + FIELD_RECT_X, y + FIELD_RECT_Y, ARGH_SIZE, ARGH_SIZE);
            rb->lcd_clearrect(arghx[0] + FIELD_RECT_X, arghy[0] + FIELD_RECT_Y, ARGH_SIZE, ARGH_SIZE);

                if (failures > last_failures) {
                rb->button_get(true);
            }
                last_failures = failures;
            hit ++;
        }
    }
    }
}

static void test_worm_argh_collision_in_moves(void) {
    int hit_count = 0;
    int i;
    rb->lcd_clear_display();
    init_worm(&worms[0], 10, 20);

    arghx[0] = 20;
    arghy[0] = 18;
    draw_argh(0);

    set_worm_dir(&worms[0], EAST);
    for (i = 0; i < 20; i++) {
        char buf[20];
        move_worm(&worms[0]);
        draw_worm(&worms[0]);
        if (worm_argh_collision_in_moves(&worms[0], 0, 5)){
            hit_count ++;
        }
        rb->snprintf(buf, sizeof buf, "in 5 moves hits: %d", hit_count);
        rb->lcd_putsxy(0, LCD_HEIGHT - 8, buf);
        rb->lcd_update();
    }    
    if (hit_count != ARGH_SIZE + 5) {
        rb->button_get(true);
    }    
}
#endif /* DEBUG_WORMLET */

extern bool use_old_rect;

/**
 * Main entry point
 */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    bool worm_dead = false;
    int button;
    
    TEST_PLUGIN_API(api);
    (void)(parameter);

    rb = api;
    rb->lcd_setfont(FONT_SYSFIXED);

#ifdef DEBUG_WORMLET
    testline_in_rect();
    test_worm_argh_collision_in_moves();
    test_make_argh();
    test_worm_food_collision();
    test_worm_argh_collision();
    test_specific_worm_collision();
#endif 

    /* Setup screen */
    do {
        char buf[20];
        char* ptr;
        rb->lcd_clear_display();

        /* first line players */
        rb->snprintf(buf, sizeof buf, "%d Players    UP/DN", players);
        rb->lcd_puts(0, 0, buf);

        /* second line worms */
        rb->snprintf(buf, sizeof buf, "%d Worms        L/R", worm_count);
        rb->lcd_puts(0, 1, buf);

        /* third line control */
        if (players > 1) {
            if (use_remote) {
                ptr = "Remote Control F1";
            } else {
                ptr = "No Rem. Control F1";
            }
        } else {
            if (players > 0) {
                if (use_remote) {
                    ptr = "2 Key Control   F1";
                } else {
                    ptr = "4 Key Control   F1";
                }
            } else {
                ptr = "Out Of Control";
            }
        }
        rb->lcd_puts(0, 2, ptr);
        rb->lcd_update();

        /* user selection */
        button = rb->button_get(true);
        switch (button) {
            case BUTTON_UP:
                if (players < 3) {
                    players ++;
                    if (players > worm_count) {
                        worm_count = players;
                    }
                    if (players > 2) {
                        use_remote = true;
                    }
                }
                break;
            case BUTTON_DOWN:
                if (players > 0) {
                    players --;
                }
                break;
            case BUTTON_LEFT: 
                if (worm_count > 1) {
                    worm_count--;
                    if (worm_count < players) {
                        players = worm_count;
                    }
                }
                break;
            case BUTTON_RIGHT:
                if (worm_count < MAX_WORMS) {
                    worm_count ++;
                }
                break;
            case BUTTON_F1:
                use_remote = !use_remote;
                if (players > 2) {
                    use_remote = true;
                }
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    } while (button != BUTTON_PLAY && 
             button != BUTTON_OFF  && button != BUTTON_ON);

    rb->lcd_clear_display();
    /* end of setup */

    do {    

        /* button state will be overridden if
           the game quits with the death of the worm.
           Initializing button to BUTTON_OFF ensures
           that the user can hit BUTTON_OFF during the
           game to return to the menu.
        */
        button  = BUTTON_OFF;

        /* start the game */
        worm_dead = run();

        /* if worm isn't dead the game was quit
           via BUTTON_OFF -> no need to wait for buttons. */
        if (worm_dead) {
            do {
                button = rb->button_get(true);
            }
            /* BUTTON_ON -> start new game */
            /* BUTTON_OFF -> back to game menu */
            while (button != BUTTON_OFF && button != BUTTON_ON);
        }
    }
    while (button != BUTTON_OFF);

    return PLUGIN_OK;
}

#endif
