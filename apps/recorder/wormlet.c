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

#include "config.h"

#ifdef USE_GAMES

#include <sprintf.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "menu.h"

#define MAX_WORM_LENGTH 500    /* size of the ring of the worm */
#define INITIAL_WORM_LENGTH 10 /* when the game starts */
#define WORM_PER_FOOD 7        /* num of pixel the worm grows per eaten food */

/* The worm is stored in a ring of xy coordinates */
static short wormx[MAX_WORM_LENGTH];
static short wormy[MAX_WORM_LENGTH];

static int head; /* index of the head within the buffer */
static short headx;
static short heady;

static int tail;    /* index of the tail within the buffer */
static int growing; /* number of cyles the worm still keeps growing */


#define MAX_FOOD  5 /* maximal number of food items */
#define FOOD_SIZE 3 /* the width and height of a food */
static short foodx[MAX_FOOD];
static short foody[MAX_FOOD];

#define MAX_ARGH  100    /* maximal number of argh items */
#define ARGH_SIZE 4      /* the width and height of a argh */
#define ARGHS_PER_FOOD 2 /* number of arghs produced per eaten food */
static short arghx[MAX_ARGH];
static short arghy[MAX_ARGH];
static int arghCount;

/* direction vector in which the worm moves */
static short dirx; /* only values -1 0 1 allowed */
static short diry; /* only values -1 0 1 allowed */

static int speed = 10;

/* return values of checkCollision */
#define COLLISION_NONE 0
#define COLLISION_WORM 1
#define COLLISION_FOOD 2
#define COLLISION_ARGH 3
#define COLLISION_FIELD 4

/* size of the field the worm lives in */
#define FIELD_RECT_X 1
#define FIELD_RECT_Y 1
#define FIELD_RECT_WIDTH  LCD_HEIGHT - 2
#define FIELD_RECT_HEIGHT LCD_HEIGHT - 2

/**
 * Returns the current length of the worm.
 * @return int a positive value
 */
static int getWormLength(void) {
    /* initial simple calculation will be overwritten if wrong. */
    int retVal = head - tail;

    /* if the worm 'crosses' the boundaries of the ringbuffer */
    if (retVal < 0) {
        retVal = head + MAX_WORM_LENGTH - tail;
    }

    return retVal;
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
static bool specificFoodCollision(int foodIndex, int x, int y) {
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
static int foodCollision(int x, int y) {
    int i = 0;
    int retVal = -1;
    for (i = 0; i < MAX_FOOD; i++) {
        if (specificFoodCollision(i, x, y)) {
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
static bool specificArghCollision(int arghIndex, int x, int y) {

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
 * @return int  -1 <= value < arghCount <= MAX_ARGH
 */
static int arghCollision(int x, int y) {
    int i = 0;
    int retVal = -1;

    /* search for the argh that has the specified coords */
    for (i = 0; i < arghCount; i++) {
        if (specificArghCollision(i, x, y)) {
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
static bool wormFoodCollision(int foodIndex)
{
    bool retVal = false;

    /* buffer wormLength because getWormLength is expensive */
    int wormLength = getWormLength();
    int i;

    /* although all the worm gets iterated i is NOT 
       the index of the worm arrays */
    for (i = 0; i < wormLength; i++) {

        /* convert i to the true worm indices */
        int wormIndex = (tail + i) % MAX_WORM_LENGTH;
        if (specificFoodCollision(foodIndex, 
                                  wormx[wormIndex], 
                                  wormy[wormIndex])) {
            retVal = true;
            break;
        }
    }

    return retVal;
}

/**
 * Checks wether the worm collides with the argh at the specfied argh-arrays.
 * @param int arghIndex The index of the argh in the arrays.
 * Ensure the value is 0 <= arghIndex < arghCount <= MAX_ARGH
 * @return Returns true if the worm collides with the specified argh.
 */
static bool wormArghCollision(int arghIndex)
{
    bool retVal = false;

    /* buffer wormLength because getWormLength is expensive */
    int wormLength = getWormLength();
    int i;

    /* although all the worm gets iterated i is NOT 
       the index of the worm arrays */
    for (i = 0; i < wormLength; i++) {

        /* convert i to the true worm indices */
        int wormIndex = (tail + i) % MAX_WORM_LENGTH;
        if (specificArghCollision(arghIndex, 
                                  wormx[wormIndex], 
                                  wormy[wormIndex])) {
            retVal = true;
            break;
        }
    }

    return retVal;
}

/**
 * Find new coordinates for the food stored in foodx[index], foody[index]
 * that don't collide with any other food or argh
 * @param int index 
 * Ensure that 0 <= index < MAX_FOOD.
 */
static void makeFood(int index) {

    int x = 0;
    int y = 0;
    bool collisionDetected = false;

    do {
        /* make coordinates for a new food so that
           the entire food lies within the FIELD */
        x = rand() % (FIELD_RECT_WIDTH  - FOOD_SIZE);
        y = rand() % (FIELD_RECT_HEIGHT - FOOD_SIZE);

        /* Ensure that the new food doesn't collide with any
           existing foods or arghs.
           If one or more corners of the new food hit any existing
           argh or food a collision is detected.
        */
        collisionDetected = 
            foodCollision(x, y ) >= 0 ||
            foodCollision(x, y + FOOD_SIZE - 1) >= 0 ||
            foodCollision(x + FOOD_SIZE - 1, y ) >= 0 ||
            foodCollision(x + FOOD_SIZE - 1, y + FOOD_SIZE - 1) >= 0 ||
            arghCollision(x, y ) >= 0 || 
            arghCollision(x, y + FOOD_SIZE - 1) >= 0 ||
            arghCollision(x + FOOD_SIZE - 1, y ) >= 0 ||
            arghCollision(x + FOOD_SIZE - 1, y + FOOD_SIZE - 1) >= 0;

        /* use coordinates for further testing */
        foodx[index] = x;
        foody[index] = y;

        /* now test wether we accidently hit the worm with food ;) */
        collisionDetected |= wormFoodCollision(index);
    }
    while (collisionDetected);
}

/**
 * Clears a food from the lcd buffer.
 * @param int index The index of the food arrays under which
 * the coordinates of the desired food can be found. Ensure 
 * that the value is 0 <= index <= MAX_FOOD.
 */
static void clearFood(int index)
{
    /* remove the old food from the screen */
    lcd_clearrect(foodx[index] + FIELD_RECT_X, 
                  foody[index] + FIELD_RECT_Y, 
                  FOOD_SIZE, FOOD_SIZE);
}

/**
 * Draws a food in the lcd buffer.
 * @param int index The index of the food arrays under which
 * the coordinates of the desired food can be found. Ensure 
 * that the value is 0 <= index <= MAX_FOOD.
 */
static void drawFood(int index)
{
    /* draw the food object */
    lcd_fillrect(foodx[index] + FIELD_RECT_X, 
                 foody[index] + FIELD_RECT_Y, 
                 FOOD_SIZE, FOOD_SIZE);
    lcd_clearrect(foodx[index] + FIELD_RECT_X + 1, 
                  foody[index] + FIELD_RECT_Y + 1, 
                  FOOD_SIZE - 2, FOOD_SIZE - 2);
}

/**
 * Find new coordinates for the argh stored in arghx[index], arghy[index]
 * that don't collide with any other food or argh.
 * @param int index 
 * Ensure that 0 <= index < arghCount < MAX_ARGH.
 */
static void makeArgh(int index)
{
    int x; 
    int y; 
    bool collisionDetected = false;

    do {
        /* make coordinates for a new argh so that
           the entire food lies within the FIELD */
        x = rand() % (FIELD_RECT_WIDTH  - ARGH_SIZE);
        y = rand() % (FIELD_RECT_HEIGHT - ARGH_SIZE);
        /* Ensure that the new argh doesn't intersect with any
           existing foods or arghs.
           If one or more corners of the new argh hit any existing
           argh or food an intersection is detected.
        */
        collisionDetected = 
            foodCollision(x, y ) >= 0 || 
            foodCollision(x, y + ARGH_SIZE - 1) >= 0 ||
            foodCollision(x + ARGH_SIZE - 1, y ) >= 0 ||
            foodCollision(x + ARGH_SIZE - 1, y + ARGH_SIZE - 1) >= 0 ||
            arghCollision(x, y ) >= 0 || 
            arghCollision(x, y + ARGH_SIZE - 1) >= 0 ||
            arghCollision(x + ARGH_SIZE - 1, y ) >= 0 ||
            arghCollision(x + ARGH_SIZE - 1, y + ARGH_SIZE - 1) >= 0;

        /* use the candidate coordinates to make a real argh */
        arghx[index] = x;
        arghy[index] = y;

        /* now test wether we accidently hit the worm with argh ;) */
        collisionDetected |= wormArghCollision(index);
    }
    while (collisionDetected);
}

/**
 * Draws an argh in the lcd buffer.
 * @param int index The index of the argh arrays under which
 * the coordinates of the desired argh can be found. Ensure 
 * that the value is 0 <= index < arghCount <= MAX_ARGH.
 */
static void drawArgh(int index)
{
    /* draw the new argh */
    lcd_fillrect(arghx[index] + FIELD_RECT_X, 
                 arghy[index] + FIELD_RECT_Y, 
                 ARGH_SIZE, ARGH_SIZE);
}

/**
 * Initializes the worm-, food- and argh-arrays, draws a frame,
 * makes some food and argh and display all that stuff.
 */
static void initWormlet(void)
{
    int i;

    /* Initialize all the worm coordinates to 0,0. */
    memset(wormx,0,sizeof wormx);
    memset(wormy,0,sizeof wormy);

    /* Needed when the game is restarted using BUTTON_ON */
    lcd_clear_display();

    /* initialize the worm size */
    head = INITIAL_WORM_LENGTH;
    tail = 0;

    /* initialize the worm start point */
    headx = 0;
    heady = 0;

    /* set the initial direction the worm creeps to */
    dirx = 1;
    diry = 0;

    /* make and display some food and argh */
    arghCount = MAX_FOOD;
    for (i = 0; i < MAX_FOOD; i++) {
        makeFood(i);
        drawFood(i);
        makeArgh(i);
        drawArgh(i);
    }

    /* draw the game field */
    lcd_invertrect(0, 0, FIELD_RECT_WIDTH + 2, FIELD_RECT_HEIGHT + 2);
    lcd_invertrect(1, 1, FIELD_RECT_WIDTH, FIELD_RECT_HEIGHT);

    /* make everything visible */
    lcd_update();
}

/** 
 * Move the worm one step further.
 * The direction in which the worm moves is taken from dirx and diry. If the 
 * worm crosses the boundaries of the field the worm is wrapped (it enters
 * the field from the opposite side). moveWorm decreases growing if > 0.
 */
static void moveWorm(void)
{
    /* find the next array index for the head */
    head++;
    if (head >= MAX_WORM_LENGTH)
        head = 0;

    /* find the next array index for the tail */
    tail++;
    if (tail >= MAX_WORM_LENGTH)
        tail = 0;

    /* determine the new head position */
    headx += dirx;
    heady += diry;

    /* Wrap the new head position if necessary */
    if (headx >= FIELD_RECT_WIDTH)
        headx = 0;
    else
        if (headx < 0)
            headx = FIELD_RECT_WIDTH - 1;

    if (heady >= FIELD_RECT_HEIGHT)
        heady = 0;
    else
        if (heady < 0)
            heady = FIELD_RECT_HEIGHT - 1;

    /* store the new head position in the worm arrays */
    wormx[head] = headx;
    wormy[head] = heady;

    /* update the worms grow state */
    if (growing > 0)
        growing--;
}

/**
 * Draws the head and clears the tail of the worm in 
 * the display buffer. lcd_update() is NOT called thus
 * the caller has to take care that the buffer is displayed.
 */
static void drawWorm(void)
{
    /* draw the new head */
    lcd_drawpixel( wormx[head] + FIELD_RECT_X, wormy[head] + FIELD_RECT_Y);

    /* clear the space behind the worm */
    lcd_clearpixel(wormx[tail] + FIELD_RECT_X, wormy[tail] + FIELD_RECT_Y);
}

/**
 * Checks wether the coordinate is part of the worm.
 * @param x int The x coordinate 
 * @param y int The y coordinate
 * @return int The index of the worm arrays that contain x, y.
 * Returns -1 if the coordinates are not part of the worm.
 */ 
static int wormCollision(int x, int y)
{
    int retVal = -1;

    /* getWormLength is expensive -> buffer the value */
    int wormLength = getWormLength();
    int i;

    /* test each entry that is part of the worm */
    for (i = 0; i < wormLength && retVal == -1; i++) {

        /* The iteration iterates the length of the worm.
           Here's the conversion to the true indices within the worm arrays. */
        int trueIndex = (tail + i) % MAX_WORM_LENGTH;
        if (wormx[trueIndex] == x && wormy[trueIndex] == y)
            retVal = trueIndex;
    }
    return retVal;
}

/**
 * Returns true if the head of the worm just has
 * crossed the field boundaries. 
 * @return bool true if the worm just has wrapped.
 */ 
static bool fieldCollision(void)
{
    bool retVal = false;
    if ((headx == FIELD_RECT_WIDTH - 1  && dirx == -1) ||
        (heady == FIELD_RECT_HEIGHT - 1 && diry == -1) ||
        (headx == 0 && dirx ==  1) ||
        (heady == 0 && diry ==  1))
    {
        retVal = true;
    }
    return retVal;
}

/**
 * Checks and returns wether the head of the worm
 * is colliding with something currently.
 * @return int One of the values:
 *   COLLISION_NONE
 *   COLLISION_WORM
 *   COLLISION_FOOD
 *   COLLISION_ARGH
 *   COLLISION_FIELD
 */
static int checkCollision(void)
{
    int retVal = COLLISION_NONE;

    if (wormCollision(headx, heady) >= 0)
        retVal = COLLISION_WORM;

    if (foodCollision(headx, heady) >= 0)
        retVal = COLLISION_FOOD;

    if (arghCollision(headx, heady))
        retVal = COLLISION_ARGH;

    if (fieldCollision())
        retVal = COLLISION_FIELD;

    return retVal;
}

/**
 * Prints out the score board with all the status information
 * about the game.
 */
static void scoreBoard(void)
{
    char buf[15];
    char buf2[15];

    /* Title */
    snprintf(buf, sizeof (buf), "Wormlet");
    lcd_putsxy(FIELD_RECT_WIDTH + 3, 0, buf, 0);

    /* length */
    snprintf(buf, sizeof (buf), "length:");
    lcd_putsxy(FIELD_RECT_WIDTH + 3, 12, buf, 0);
    snprintf(buf, sizeof (buf), "%d  ", getWormLength() - growing);
    lcd_putsxy(FIELD_RECT_WIDTH + 3, 20, buf, 0);

    switch (checkCollision()) {
        case COLLISION_NONE:  
            snprintf(buf, sizeof(buf), "I'm hungry!  "); 
            if (growing > 0)
                snprintf(buf2, sizeof(buf2), "growing");
            else
                snprintf(buf2, sizeof(buf2), "             ");
            break;

        case COLLISION_WORM:  
            snprintf(buf, sizeof(buf), "Ouch! I bit"); 
            snprintf(buf2, sizeof(buf2), "myself!    ");
            break;

        case COLLISION_FOOD:  
            snprintf(buf, sizeof(buf), "Yummy!  "); 
            snprintf(buf2, sizeof(buf2), "growing");
            break;

        case COLLISION_ARGH:  
            snprintf(buf, sizeof(buf), "Argh! I'm   "); 
            snprintf(buf2, sizeof(buf2), "poisoned!   ");
            break;

        case COLLISION_FIELD: 
            snprintf(buf, sizeof(buf), "Boing!        "); 
            snprintf(buf2, sizeof(buf2), "Headcrash!");
            break;
    }
    lcd_putsxy(FIELD_RECT_WIDTH + 3, 32, buf, 0);
    lcd_putsxy(FIELD_RECT_WIDTH + 3, 40, buf2, 0);
}

/**
 * Checks for collisions of the worm and its environment and
 * takes appropriate actions like growing the worm or killing it.
 * @return bool Returns true if the worm is dead. Returns
 * false if the worm is healthy, up and creeping.
 */
static bool processCollisions(void)
{
    bool wormDead = false;
    int index = -1;

    wormDead = fieldCollision();

    if (!wormDead) {

        /* check if food was eaten */
        index = foodCollision(headx, heady);
        if (index != -1){
            int i;
            
            clearFood(index);
            makeFood(index);
            drawFood(index);
            
            for (i = 0; i < ARGHS_PER_FOOD; i++) {
                arghCount++;
                if (arghCount > MAX_ARGH)
                    arghCount = MAX_ARGH;
                makeArgh(arghCount - 1);
                drawArgh(arghCount - 1);
            }

            tail -= WORM_PER_FOOD;
            growing += WORM_PER_FOOD;
            if (tail < 0)
                tail += MAX_WORM_LENGTH; 

            drawWorm();
        }

        /* check if argh was eaten */
        else {
            index = arghCollision(headx, heady);
            if (index != -1)
                wormDead = true;
            else
                if (wormCollision(headx, heady) != -1)
                    wormDead = true;
        }
    }
    return wormDead;
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

    /* initialize the board and so on */
    initWormlet();

    /* change the direction of the worm */
    while (button != BUTTON_OFF && ! wormDead)
    {
        switch (button) {
            case BUTTON_UP:
                diry = -1;
                dirx = 0;
                break;

            case BUTTON_DOWN:
                diry = 1;
                dirx = 0;
                break;

            case BUTTON_LEFT:
                dirx = -1;
                diry = 0;
                break;

            case BUTTON_RIGHT:
                dirx = 1;
                diry = 0;
                break;

            case BUTTON_F2:
                speed--;
                break;

            case BUTTON_F3:
                speed ++;
                break;
        }

        moveWorm();
        wormDead = processCollisions();
        drawWorm();
        scoreBoard();
        lcd_update();
        button = button_get_w_tmo(HZ/speed);
    }
    return wormDead;
}

/**
 * Main entry point from the menu to start the game control.
 */
Menu wormlet(void)
{
    bool wormDead = false;
    int button;
    do {    

        /* button state will be overridden if
           the game quits with the death of the worm.
           Initializing button to BUTTON_OFF ensures
           that the user can hit BUTTON_OFF during the
           game to return to the menu.
        */
        button  = BUTTON_OFF;

        /* start the game */
        wormDead = run();

        /* if worm isn't dead the game was quit
           via BUTTON_OFF -> no need to wait for buttons. */
        if (wormDead) {
            do {
                button = button_get(true);
            }
            /* BUTTON_ON -> start new game */
            /* BUTTON_OFF -> back to game menu */
            while (button != BUTTON_OFF && button != BUTTON_ON);
        }
    }
    while (button != BUTTON_OFF);

    return MENU_OK;
}

#endif
