/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Eli Sherer
 *               2007 Antoine Cellerier
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
#include "helper.h"

PLUGIN_HEADER

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define PAUSE BUTTON_MODE
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#define RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == ARCHOS_AV300_PAD)

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define PAUSE BUTTON_ON
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#define QUIT (BUTTON_SELECT | BUTTON_MENU)
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define PAUSE BUTTON_SELECT
#define MENU_UP BUTTON_SCROLL_FWD
#define MENU_DOWN BUTTON_SCROLL_BACK
#define UP BUTTON_MENU
#define DOWN BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define PAUSE BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define PAUSE BUTTON_A

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define PAUSE BUTTON_REC


#elif CONFIG_KEYPAD == IRIVER_H10_PAD

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define UP BUTTON_SCROLL_UP
#define DOWN BUTTON_SCROLL_DOWN
#define PAUSE BUTTON_PLAY

#elif CONFIG_KEYPAD == RECORDER_PAD

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define DOWN BUTTON_DOWN
#define UP BUTTON_UP
#define PAUSE BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define DOWN BUTTON_DOWN
#define UP BUTTON_UP
#define PAUSE BUTTON_MENU

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)

#define QUIT BUTTON_BACK
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define PAUSE BUTTON_PLAY

#elif (CONFIG_KEYPAD == MROBE100_PAD)

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define PAUSE BUTTON_DISPLAY

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD

#define QUIT BUTTON_RC_REC
#define LEFT BUTTON_RC_REW
#define RIGHT BUTTON_RC_FF
#define UP BUTTON_RC_VOL_UP
#define DOWN BUTTON_RC_VOL_DOWN
#define PAUSE BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == COWOND2_PAD

#define QUIT BUTTON_POWER

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef QUIT
#define QUIT  BUTTON_TOPLEFT
#endif
#ifndef LEFT
#define LEFT  BUTTON_MIDLEFT
#endif
#ifndef RIGHT
#define RIGHT BUTTON_MIDRIGHT
#endif
#ifndef UP
#define UP    BUTTON_TOPMIDDLE
#endif
#ifndef DOWN
#define DOWN  BUTTON_BOTTOMMIDDLE
#endif
#ifndef PAUSE
#define PAUSE BUTTON_CENTER
#endif
#endif

#define MOVE_NO 0               /* player movement */
#define MOVE_UP 1               /*    1    */
#define MOVE_DN 2               /*  3 0 4  */
#define MOVE_LT 3               /*    2    */
#define MOVE_RT 4

/* ball movement (12 ways) */
/*    UUL   UR    */
/*   UL       UR   */
/* ULL    .    URR */
/* DLL         DRR */
/*   DL       DR   */
/*    DDL   DDR    */

#define DIR_UU (1<<7)
#define DIR_U  (1<<6)
#define DIR_RR (1<<5)
#define DIR_R  (1<<4)
#define DIR_DD (1<<3)
#define DIR_D  (1<<2)
#define DIR_LL (1<<1)
#define DIR_L  (1<<0)

#define MOVE_UUR ( DIR_UU | DIR_R  )
#define MOVE_UR  ( DIR_U  | DIR_R  )
#define MOVE_URR ( DIR_U  | DIR_RR )
#define MOVE_DRR ( DIR_D  | DIR_RR )
#define MOVE_DR  ( DIR_D  | DIR_R  )
#define MOVE_DDR ( DIR_DD | DIR_R  )
#define MOVE_DDL ( DIR_DD | DIR_L  )
#define MOVE_DL  ( DIR_D  | DIR_L  )
#define MOVE_DLL ( DIR_D  | DIR_LL )
#define MOVE_ULL ( DIR_U  | DIR_LL )
#define MOVE_UL  ( DIR_U  | DIR_L  )
#define MOVE_UUL ( DIR_UU | DIR_L  )

#if (LCD_WIDTH>112) && (LCD_HEIGHT>64)
#   define CUBE_SIZE 8             /* 8x22=176 */
#   define pos(a) ((a)>>3)
#else
#   define CUBE_SIZE 4
#   define pos(a) ((a)>>2)
#endif

#define STARTING_QIXES 2
#define MAX_LEVEL 10
#define MAX_QIXES MAX_LEVEL+STARTING_QIXES
#define BOARD_W ((int)(LCD_WIDTH/CUBE_SIZE))
#define BOARD_H ((int)(LCD_HEIGHT/CUBE_SIZE))
#define BOARD_X (LCD_WIDTH-BOARD_W*CUBE_SIZE)/2
#define BOARD_Y (LCD_HEIGHT-BOARD_H*CUBE_SIZE)/2

#ifdef HAVE_LCD_COLOR
#define CLR_RED  LCD_RGBPACK(255,0,0)         /* used to imply danger */
#define CLR_LTBLUE LCD_RGBPACK(125, 145, 180) /* used for frame and filling */
#define PLR_COL  LCD_WHITE                    /* color used for the player */
#elif LCD_DEPTH>=2
#define CLR_RED  LCD_DARKGRAY     /* used to imply danger */
#define CLR_LTBLUE LCD_LIGHTGRAY  /* used for frame and filling */
#define PLR_COL  LCD_BLACK        /* color used for the player */
#endif

#if LCD_DEPTH>=2
#define EMPTIED LCD_BLACK       /* empty spot */
#define FILLED  CLR_LTBLUE      /* filled spot */
#define TRAIL   CLR_RED         /* the red trail of the player */
#define QIX     LCD_WHITE
#else
#define EMPTIED 0
#define FILLED 1
#define TRAIL 2
#define QIX 3
#endif
#define UNCHECKED 0
#define CHECKED 1
#define PAINTED -1
#define PIC_QIX 0
#define PIC_PLAYER 1

#define MENU_START 0
#define MENU_QUIT 1

/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
static int speed = 6; /* CYCLETIME = (11-speed)*10 ms */
static int difficulty = 75; /* Percentage of screen that needs to be filled
                             * in order to win the game */

static const struct plugin_api *rb;

MEM_FUNCTION_WRAPPERS(rb);

static bool quit = false;

static unsigned int board[BOARD_H][BOARD_W];
static int testboard[BOARD_H][BOARD_W];

#if CUBE_SIZE == 8
/*
   00011000 0x18 - 11100111 0xe7
   00111100 0x3c - 11100111 0xe7
   01111110 0x7e - 11000011 0xc3
   11111111 0xff - 00000000 0x00
   11111111 0xff - 00000000 0x00
   01111110 0x7e - 11000011 0xc3
   00111100 0x3c - 11100111 0xe7
   00011000 0x18 - 11100111 0xe7
 */
const unsigned char pics[2][8] = {
    {0x18, 0x3c, 0x7e, 0xff, 0xff, 0x7e, 0x3c, 0x18},   /* Alien (QIX) */
    {0xe7, 0xe7, 0xc3, 0x00, 0x00, 0xc3, 0xe7, 0xe7}    /* Player (XONIX) */
};
#elif CUBE_SIZE == 4
/*
   0110 0x6 - 1001 0x9
   1111 0xf - 0110 0x6
   1111 0xf - 0110 0x6
   0110 0x6 - 1001 0x9
 */
const unsigned char pics[2][4] = {
    {0x6, 0xf, 0xf, 0x6},   /* Alien (QIX) */
    {0x9, 0x6, 0x6, 0x9}    /* Player (XONIX) */
};
#else
#error Incorrect CUBE_SIZE value.
#endif

static struct qix
{
    int velocity;             /* velocity */
    int x, y;                 /* position on screen */
    int angle;                /* angle */
} qixes[MAX_QIXES];             /* black_qix */

static struct splayer
{
    int i, j;                 /* position on board */
    int move, score, level, lives;
    bool drawing;
    bool gameover;
} player;

static int percentage_cache;

/*************************** STACK STUFF **********************/

/* the stack */
#define STACK_SIZE (2*BOARD_W*BOARD_H)
static struct pos
{
    int x, y;                   /* position on board */
} stack[STACK_SIZE];
static int stackPointer;

static inline bool pop (struct pos *p)
{
    if (stackPointer > 0) {
        p->x = stack[stackPointer].x;
        p->y = stack[stackPointer].y;
        stackPointer--;
        return true;
    } else
        return false;           /* SE */
}

static inline bool push (struct pos *p)
{
    if (stackPointer < STACK_SIZE - 1) {
        stackPointer++;
        stack[stackPointer].x = p->x;
        stack[stackPointer].y = p->y;
        return true;
    } else
        return false;           /* SOF */
}

static inline void emptyStack (void)
{
    stackPointer = 0;
}

/*********************** END OF STACK STUFF *********************/

/* calculate the new x coordinate of the ball according to angle and speed */
static inline int get_newx (int x, int len, int deg)
{
    if (deg & DIR_R)
        return x + len;
    else if (deg & DIR_L)
        return x - len;
    else if (deg & DIR_RR)
        return x + len * 2;
    else /* (def & DIR_LL) */
        return x - len * 2;
}

/* calculate the new y coordinate of the ball according to angle and speed */
static inline int get_newy (int y, int len, int deg)
{
    if (deg & DIR_D)
        return y + len;
    else if (deg & DIR_U)
        return y - len;
    else if (deg & DIR_DD)
        return y + len * 2;
    else /* (deg & DIR_UU) */
        return y - len * 2;
}

/* make random function get it's value from the device ticker */
static inline void randomize (void)
{
    rb->srand (*rb->current_tick);
}

/* get a random number between 0 and range-1 */
static int t_rand (int range)
{
    return rb->rand () % range;
}

/* initializes the test help board */
static void init_testboard (void)
{
    int j;                   /* testboard */
    for (j = 0; j < BOARD_H; j++)
        /* UNCHEKED == (int)0 */
        rb->memset( testboard[j], 0, BOARD_W * sizeof( int ) );
}

/* initializes the game board on with the player,qix's and black qix */
static void init_board (void)
{
    int i, j;
    for (j = 0; j < BOARD_H; j++)
        for (i = 0; i < BOARD_W; i++) { /* make a nice cyan frame */
            if ((i == 0) || (j <= 1) || (i == BOARD_W - 1)
                || (j >= BOARD_H - 2))
                board[j][i] = FILLED;
            else
                board[j][i] = EMPTIED;
        }

    /* (level+2) is the number of qixes */
    for (j = 0; j < player.level + STARTING_QIXES; j++) {
        qixes[j].velocity = t_rand (2) + 1;     /* 1 or 2 pix-per-sec */

        /* not on frame */
        qixes[j].x = CUBE_SIZE*2 + 2*t_rand (((BOARD_W-4)*CUBE_SIZE)/2);
        qixes[j].y = CUBE_SIZE*2 + 2*t_rand (((BOARD_H-4)*CUBE_SIZE)/2);

        const int angle_table[] = {
            MOVE_UUR, MOVE_UR, MOVE_URR, MOVE_DRR, MOVE_DR, MOVE_DDR,
            MOVE_UUL, MOVE_UL, MOVE_ULL, MOVE_DLL, MOVE_DL, MOVE_DDL };
        qixes[j].angle = angle_table[t_rand (12)];
#if CUBE_SIZE == 4
        /* Work arround a nasty bug. FIXME */
        if( qixes[j].angle & (DIR_LL|DIR_RR|DIR_UU|DIR_DD) )
            qixes[j].velocity = 1;
#endif
    }
    /*black_qix.velocity=1;
       black_qix.x=BOARD_X+(BOARD_W*CUBE_SIZE)/2-CUBE_SIZE/2;
       black_qix.y=BOARD_Y+(BOARD_H*CUBE_SIZE)-CUBE_SIZE-CUBE_SIZE/2;
       black_qix.angle=MOVE_UR; */
    player.move = MOVE_NO;
    player.drawing = false;
    player.i = BOARD_W / 2;
    player.j = 1;

    percentage_cache = 0;
}

/* calculates the percentage of the screen filling */
static int percentage (void)
{
    int i, j, filled = 0;
    for (j = 2; j < BOARD_H - 2; j++)
        for (i = 1; i < BOARD_W - 1; i++)
            if (board[j][i] == FILLED)
                filled++;
    return (filled * 100) / ((BOARD_W - 2) * (BOARD_H - 4));
}

/* draw the board on with all the game figures */
static void refresh_board (void)
{
    int i, j;
    char str[25];

#if LCD_DEPTH>=2
    rb->lcd_set_background (LCD_BLACK);
#else
    rb->lcd_clear_display ();
#endif
    for (j = 0; j < BOARD_H; j++)
    {
        unsigned last_color = board[j][0];
        int last_i = 0;
        for (i = 1; i < BOARD_W; i++) {
            if( last_color != board[j][i] )
            {
#if LCD_DEPTH>=2
                rb->lcd_set_foreground (last_color);
#else
                if (last_color != EMPTIED)
#endif
                rb->lcd_fillrect (BOARD_X + CUBE_SIZE * (last_i),
                                  BOARD_Y + CUBE_SIZE * j,
                                  CUBE_SIZE  * (i - last_i), CUBE_SIZE );
                last_color = board[j][i];
                last_i = i;
            }
        }
#if LCD_DEPTH>=2
        rb->lcd_set_foreground (last_color);
#else
        if (last_color != EMPTIED)
#endif
        rb->lcd_fillrect (BOARD_X + CUBE_SIZE * (last_i),
                          BOARD_Y + CUBE_SIZE * j,
                          CUBE_SIZE * (i - last_i), CUBE_SIZE);
    }

#if LCD_DEPTH>=2
    rb->lcd_set_foreground (LCD_BLACK);
    rb->lcd_set_background (CLR_LTBLUE);
#else
    rb->lcd_set_drawmode (DRMODE_COMPLEMENT);
#endif
    rb->snprintf (str, sizeof (str), "Level %d", player.level + 1);
    rb->lcd_putsxy (BOARD_X, BOARD_Y, str);
    rb->snprintf (str, sizeof (str), "%d%%", percentage_cache);
    rb->lcd_putsxy (BOARD_X + CUBE_SIZE * BOARD_W - 24, BOARD_Y, str);
    rb->snprintf (str, sizeof (str), "Score: %d", player.score);
    rb->lcd_putsxy (BOARD_X, BOARD_Y + CUBE_SIZE * BOARD_H - 8, str);
    rb->snprintf (str, sizeof (str),
                 (player.lives != 1) ? "%d Lives" : "%d Life", player.lives);
#if LCD_DEPTH>=2
    rb->lcd_putsxy (BOARD_X + CUBE_SIZE * BOARD_W - 60,
                    BOARD_Y + CUBE_SIZE * BOARD_H - 8, str);
#else
    rb->lcd_putsxy (BOARD_X + CUBE_SIZE * BOARD_W - 40,
                    BOARD_Y + CUBE_SIZE * BOARD_H - 8, str);
#endif

#if LCD_DEPTH>=2
    rb->lcd_set_foreground (PLR_COL);
    rb->lcd_set_background (board[player.j][player.i]);
#endif
    rb->lcd_mono_bitmap (pics[PIC_PLAYER], player.i * CUBE_SIZE + BOARD_X,
                         player.j * CUBE_SIZE + BOARD_Y, CUBE_SIZE, CUBE_SIZE);

#if LCD_DEPTH>=2
    rb->lcd_set_background (EMPTIED);
    rb->lcd_set_foreground (LCD_WHITE);
    rb->lcd_set_drawmode (DRMODE_FG);
#else
    rb->lcd_set_drawmode (DRMODE_FG);
#endif
    for (j = 0; j < player.level + STARTING_QIXES; j++)
        rb->lcd_mono_bitmap (pics[PIC_QIX], qixes[j].x + BOARD_X,
                             qixes[j].y + BOARD_Y, CUBE_SIZE, CUBE_SIZE);
#if LCD_DEPTH>=2
    rb->lcd_set_foreground (LCD_BLACK);
#endif
    rb->lcd_set_drawmode (DRMODE_SOLID);

    rb->lcd_update ();
}

static inline int infested_area (int i, int j, int v)
{
    struct pos p;
    p.x = i;
    p.y = j;
    emptyStack ();
    if (!push (&p))
        return -1;
    while (pop (&p)) {
        if (testboard[p.y][p.x] == v) continue;
        if (testboard[p.y][p.x] > UNCHECKED)
            return 1; /* This area was previously flagged as infested */
        testboard[p.y][p.x] = v;
        if (board[p.y][p.x] == QIX)
            return 1; /* Infested area */
        {
            struct pos p1 = { p.x+1, p.y };
            if ((p1.x < BOARD_W)
             && (board[p1.y][p1.x] != FILLED)
             && (!push (&p1)))
                return -1;
        }
        {
            struct pos p1 = { p.x-1, p.y };
            if ((p1.x >= 0)
             && (board[p1.y][p1.x] != FILLED)
             && (!push (&p1)))
                return -1;
        }
        {
            struct pos p1 = { p.x, p.y+1 };
            if ((p1.y < BOARD_H)
             && (board[p1.y][p1.x] != FILLED)
             && (!push (&p1)))
                return -1;
        }
        {
            struct pos p1 = { p.x, p.y-1 };
            if ((p1.y >= 0)
             && (board[p1.y][p1.x] != FILLED)
             && (!push (&p1)))
                return -1;
        }
    }
    return 0;
}

static inline int fill_area (int i, int j)
{
    struct pos p;
    p.x = i;
    p.y = j;
    int v = testboard[p.y][p.x];
    emptyStack ();
    if (!push (&p))
        return -1;
    while (pop (&p)) {
        board[p.y][p.x] = FILLED;
        testboard[p.y][p.x] = PAINTED;
        {
            struct pos p1 = { p.x+1, p.y };
            if ((p1.x < BOARD_W)
             && (testboard[p1.y][p1.x] == v)
             && (!push (&p1)))
                return -1;
        }
        {
            struct pos p1 = { p.x-1, p.y };
            if ((p1.x >= 0)
             && (testboard[p1.y][p1.x] == v)
             && (!push (&p1)))
                return -1;
        }
        {
            struct pos p1 = { p.x, p.y+1 };
            if ((p1.y < BOARD_H)
             && (testboard[p1.y][p1.x] == v)
             && (!push (&p1)))
                return -1;
        }
        {
            struct pos p1 = { p.x, p.y-1 };
            if ((p1.y >= 0)
             && (testboard[p1.y][p1.x] == v)
             && (!push (&p1)))
                return -1;
        }
    }
    return 0;
}


/* take care of stuff after xonix has landed on a filled spot */
static void complete_trail (int fill)
{
    int i, j, ret;
    for (j = 0; j < BOARD_H; j++) {
        for (i = 0; i < BOARD_W; i++) {
            if (board[j][i] == TRAIL) {
                if (fill)
                    board[j][i] = FILLED;
                else
                    board[j][i] = EMPTIED;
            }
        }
    }

    if (fill) {
        int v = CHECKED;
        for (i = 0; i < player.level + STARTING_QIXES; i++) /* add qixes to board */
            board[pos(qixes[i].y - BOARD_Y)]
                 [pos(qixes[i].x - BOARD_X)] = QIX;

        init_testboard();
        for (j = 1; j < BOARD_H - 1; j++) {
            for (i = 0; i < BOARD_W - 0; i++) {
                if (board[j][i] != FILLED) {
                    ret = infested_area (i, j, v);
                    if (ret < 0 || ( ret == 0 && fill_area (i, j) ) )
                        quit = true;
                    v++;
                }
            }
        }

        for (i = 0; i < player.level + STARTING_QIXES; i++) /* add qixes to board */
            board[pos(qixes[i].y - BOARD_Y)]
                 [pos(qixes[i].x - BOARD_X)] = EMPTIED;
        percentage_cache = percentage();
     }

     rb->button_clear_queue();
}

/* returns the color the real pixel(x,y) on the lcd is pointing at */
static inline unsigned int getpixel (int x, int y)
{
    const int a = pos (x - BOARD_X), b = pos (y - BOARD_Y);
    if ((a > 0) && (a < BOARD_W) && (b > 0) && (b < BOARD_H))   /* if inside board */
        return board[b][a];
    else
        return FILLED;
}

/* returns the color the ball on (newx,newy) is heading at *----*
   checks the four edge points of the square if 1st of all |    |
   are a trail (cause it's a lose life situation) and 2nd  |    |
   if it's filled so it needs to bounce.                   *____*
 */
static inline unsigned int next_hit (int newx, int newy)
{
    if ((getpixel (newx, newy) == TRAIL)
        || (getpixel (newx, newy + CUBE_SIZE - 1) == TRAIL)
        || (getpixel (newx + CUBE_SIZE - 1, newy) == TRAIL)
        || (getpixel (newx + CUBE_SIZE - 1, newy + CUBE_SIZE - 1) == TRAIL))
        return TRAIL;
    else if ((getpixel (newx, newy) == FILLED)
             || (getpixel (newx, newy + CUBE_SIZE - 1) == FILLED)
             || (getpixel (newx + CUBE_SIZE - 1, newy) == FILLED)
             || (getpixel (newx + CUBE_SIZE - 1, newy + CUBE_SIZE - 1) ==
                 FILLED))
        return FILLED;
    else
        return EMPTIED;
}

static void die (void)
{
    player.lives--;
    if (player.lives == 0)
        player.gameover = true;
    else {
        refresh_board ();
        rb->splash (HZ, "Crash!");
        complete_trail (false);
        player.move = MOVE_NO;
        player.drawing = false;
        player.i = BOARD_W / 2;
        player.j = 1;
    }
}

/* returns true if the (side) of the block          -***-
   starting from (newx,newy) has any filled pixels  *   *
                                                    -***-
 */
static inline bool line_check_lt (int newx, int newy)
{
    return    getpixel (newx, newy + CUBE_SIZE/2-1) == FILLED
           && getpixel (newx, newy + CUBE_SIZE/2  ) == FILLED;
}
static inline bool line_check_rt (int newx, int newy)
{
    return    getpixel (newx + CUBE_SIZE-1, newy + CUBE_SIZE/2-1) == FILLED
           && getpixel (newx + CUBE_SIZE-1, newy + CUBE_SIZE/2  ) == FILLED;
}
static inline bool line_check_up (int newx, int newy)
{
    return    getpixel (newx + CUBE_SIZE/2-1, newy) == FILLED
           && getpixel (newx + CUBE_SIZE/2  , newy) == FILLED;
}
static inline bool line_check_dn (int newx, int newy)
{
    return    getpixel (newx + CUBE_SIZE/2-1, newy + CUBE_SIZE-1) == FILLED
           && getpixel (newx + CUBE_SIZE/2  , newy + CUBE_SIZE-1) == FILLED;
}

static inline void move_qix (struct qix *q)
{
    int newx, newy;
    newx = get_newx (q->x, q->velocity, q->angle);
    newy = get_newy (q->y, q->velocity, q->angle);
    switch (next_hit (newx, newy))
    {
        case EMPTIED:
            q->x = newx;
            q->y = newy;
            break;
        case FILLED:
        {
            const int a = q->angle;
            q->angle =
                ((a&(DIR_UU|DIR_U))
                    ? (line_check_up (newx, newy) ? ((a&(DIR_UU|DIR_U))>>4)
                                                  : (a&(DIR_UU|DIR_U)))
                    : 0)
                |
                ((a&(DIR_RR|DIR_R))
                    ? (line_check_rt (newx, newy) ? ((a&(DIR_RR|DIR_R))>>4)
                                                  : (a&(DIR_RR|DIR_R)))
                    : 0)
                |
                ((a&(DIR_DD|DIR_D))
                    ? (line_check_dn (newx, newy) ? ((a&(DIR_DD|DIR_D))<<4)
                                                  : (a&(DIR_DD|DIR_D)))
                    : 0)
                |
                ((a&(DIR_LL|DIR_L))
                    ? (line_check_lt (newx, newy) ? ((a&(DIR_LL|DIR_L))<<4)
                                                  : (a&(DIR_LL|DIR_L)))
                    : 0);
                q->x = get_newx (q->x, q->velocity, q->angle);
                q->y = get_newy (q->y, q->velocity, q->angle);
            break;
        }
        case TRAIL:
            die();
            break;
    }
}

/* move the board forward timewise */
static inline void move_board (void)
{
    int j, newi, newj;

    for (j = 0; j < player.level + STARTING_QIXES; j++)
        move_qix (&qixes[j]);
    /* move_qix(&black_qix,true); */
    if (player.move) {
        newi = player.i;
        newj = player.j;
        switch (player.move) {
            case MOVE_UP:
                if (player.j > 1)
                    newj--;
                break;
            case MOVE_DN:
                if (player.j < BOARD_H - 2)
                    newj++;
                break;
            case MOVE_LT:
                if (player.i > 0)
                    newi--;
                break;
            case MOVE_RT:
                if (player.i < BOARD_W - 1)
                    newi++;
                break;
            default:
                break;
        }

        if ((player.drawing) && (board[newj][newi] == EMPTIED)) /* continue drawing */
            board[newj][newi] = TRAIL;
        else if ((player.drawing) && (board[newj][newi] == FILLED)) {   /* finish drawing */
            player.move = MOVE_NO;      /* stop moving */
            player.drawing = false;
            complete_trail (true);
        } else if ((board[player.j][player.i] == FILLED)
                   && (board[newj][newi] == EMPTIED)) {
            /* start drawing */
            player.drawing = true;
            board[newj][newi] = TRAIL;
        /* if the block after next is empty and we're moving onto filled, stop */
        } else if ((board[newj][newi] == FILLED)
                   && (board[newj + newj-player.j][newi + newi-player.i] == EMPTIED)) {
            player.move = MOVE_NO;
        }
        player.i = newi;
        player.j = newj;
    }
    if (percentage_cache >= difficulty) {               /* finished level */
        rb->splashf (HZ * 2, "Level %d finished", player.level+1);
        player.score += percentage_cache;
        if (player.level < MAX_LEVEL)
            player.level++;
        init_board ();
        refresh_board ();
        rb->splash (HZ * 2, "Ready?");
    }
}

/* the main menu */
static int game_menu (void)
{
    MENUITEM_STRINGLIST(menu, "XOBOX Menu", NULL, "Start New Game",
                        "Speed","Difficulty","Quit");
    int selection = 0;
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground (rb->global_settings->fg_color);
    rb->lcd_set_background (rb->global_settings->bg_color);
#elif LCD_DEPTH>=2
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif
    for (;;) {
        rb->do_menu(&menu,&selection, NULL, false);
        if (selection==1)
            rb->set_int ("Speed", "", UNIT_INT, &speed, NULL, 1, 1, 10, NULL);
        else if (selection==2)
            rb->set_int ("Difficulty", "", UNIT_INT, &difficulty, NULL,
                         5, 50, 95, NULL);
        else
            break;
    }
    if (selection != MENU_START) {
        selection = MENU_QUIT;
    }
    return selection;
}

/* init game's variables */
static void init_game (void)
{
    player.level = 0;
    player.score = 0;
    player.lives = 3;
    player.gameover = false;
    player.drawing = false;
    rb->lcd_setfont(FONT_SYSFIXED);
    init_board ();
    refresh_board ();
    rb->splash (HZ * 2, "Ready?");
}

/* general keypad handler loop */
static int xobox_loop (void)
{
    int button = 0, ret;
    bool pause = false;
    int end;

    while (!quit) {
        end = *rb->current_tick + ((11-speed)*HZ)/100;

#ifdef HAS_BUTTON_HOLD
        if (rb->button_hold()) {
        pause = true;
        rb->splash (HZ, "PAUSED");
        }
#endif

        button = rb->button_get_w_tmo (1);
        switch (button) {
            case UP:
            case UP|BUTTON_REPEAT:
                player.move = MOVE_UP;
                break;
            case DOWN:
            case DOWN|BUTTON_REPEAT:
                player.move = MOVE_DN;
                break;
            case LEFT:
            case LEFT|BUTTON_REPEAT:
                player.move = MOVE_LT;
                break;
            case RIGHT:
            case RIGHT|BUTTON_REPEAT:
                player.move = MOVE_RT;
                break;
            case PAUSE:
                pause = !pause;
                if (pause)
                    rb->splash (HZ, "Paused");
                break;
            case QUIT:
                ret = game_menu ();
                if (ret == MENU_START)
                    init_game ();
                else
                {
                    quit = true;
                    continue;
                }
                break;
            default:
                if (rb->default_event_handler (button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (!pause) {
            move_board ();
            refresh_board ();
        }
        if (player.gameover) {
            rb->splash (HZ, "Game Over!");
            ret = game_menu ();
            if (ret == MENU_START)
                init_game ();
            else
                quit = true;
        }

        if (end > *rb->current_tick)
            rb->sleep (end - *rb->current_tick);
        else
            rb->yield ();

    }                           /* end while */
    return PLUGIN_OK;           /* for no warnings on compiling */
}

/* plugin main procedure */
enum plugin_status plugin_start (const struct plugin_api *api, const void *parameter)
{
    int ret = PLUGIN_OK;

    (void) parameter;
    rb = api;

    rb->lcd_setfont (FONT_SYSFIXED);
#if LCD_DEPTH>=2
    rb->lcd_set_backdrop(NULL);
#endif

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

    quit = false;

    randomize ();
    if (game_menu () == MENU_START) {
        init_game ();
        ret = xobox_loop ();
    }

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */
    rb->lcd_setfont (FONT_UI);

    return ret;
}
