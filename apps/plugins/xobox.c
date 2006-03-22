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
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

PLUGIN_HEADER

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define PAUSE BUTTON_MODE
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define SELECT BUTTON_SELECT

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_4G_PAD)

#define QUIT (BUTTON_SELECT | BUTTON_MENU)
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define PAUSE BUTTON_SELECT
#define SELECT BUTTON_SELECT
#define UP BUTTON_MENU
#define DOWN BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD 

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define PAUSE BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define QUIT BUTTON_A
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define PAUSE BUTTON_MENU

#else
#error Unsupported keypad
#endif

#define MOVE_NO 0               /* player movement */
#define MOVE_UP 1               /*    1    */
#define MOVE_DN 2               /*  3 0 4  */
#define MOVE_LT 3               /*    2    */
#define MOVE_RT 4

#define MOVE_UUR 0              /* ball movement (12 ways) */
#define MOVE_UR  1
#define MOVE_URR 2
#define MOVE_DRR 3              /*    UUL110UUR    */
#define MOVE_DR  4              /*   UL10    1UR   */
#define MOVE_DDR 5              /* ULL9   .   2URR */
#define MOVE_DDL 6              /* DLL8       3DRR */
#define MOVE_DL  7              /*   DL7     4DR   */
#define MOVE_DLL 8              /*    DDL6 5DDR    */
#define MOVE_ULL 9
#define MOVE_UL  10
#define MOVE_UUL 11

#define CUBE_SIZE 8             /* 8x22=176 */
#define STARTING_QIXES 2
#define MAX_LEVEL 10
#define MAX_QIXES MAX_LEVEL+STARTING_QIXES
#define BOARD_W ((int)LCD_WIDTH/CUBE_SIZE)
#define BOARD_H ((int)LCD_HEIGHT/CUBE_SIZE)
#define BOARD_X (LCD_WIDTH-BOARD_W*CUBE_SIZE)/2
#define BOARD_Y (LCD_HEIGHT-BOARD_H*CUBE_SIZE)/2

#ifdef HAVE_LCD_COLOR
#define CLR_RED  LCD_RGBPACK(255,0,0)   /* used to imply danger */
#define CLR_BLUE LCD_RGBPACK(0,0,128)   /* used for menu selection */
#define CLR_CYAN LCD_RGBPACK(0,128,128) /* used for frame and filling */
#define PLR_COL  LCD_WHITE              /* color used for the player */
#else
#define CLR_RED  LCD_DARKGRAY   /* used to imply danger */
#define CLR_BLUE LCD_BLACK      /* used for menu selection */
#define CLR_CYAN LCD_LIGHTGRAY  /* used for frame and filling */
#define PLR_COL  LCD_BLACK      /* color used for the player */
#endif

#define EMPTIED LCD_BLACK       /* empty spot */
#define FILLED  CLR_CYAN        /* filled spot */
#define TRAIL   CLR_RED         /* the red trail of the player */
#define QIX     LCD_WHITE
#define UNCHECKED 0
#define CHECKED 1
#define PIC_QIX 0
#define PIC_PLAYER 1

/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 50

static struct plugin_api *rb;
static bool quit = false;

static unsigned short board[BOARD_H][BOARD_W],
    testboard[BOARD_H][BOARD_W], boardcopy[BOARD_H][BOARD_W];

/*
   00001000 0x08 - 01110111 0x77
   00011100 0x1c - 01110111 0x77
   00111110 0x3e - 01100011 0x63
   01111111 0x7f - 00000000 0x00
   00111110 0x3e - 01100011 0x63
   00011100 0x1c - 01110111 0x77
   00001000 0x08 - 01110111 0x77
   00000000 0x00 - 00000000 0x00
 */
const unsigned char pics[2][8] = {
    {0x08, 0x1c, 0x3e, 0x7f, 0x3e, 0x1c, 0x08, 0x00},   /* Alien (QIX) */
    {0x77, 0x77, 0x63, 0x00, 0x63, 0x77, 0x77, 0x00}    /* Player (XONIX) */
};

static struct qix
{
    short velocity;             /* velocity */
    short x, y;                 /* position on screen */
    short angle;                /* angle */
} qixes[MAX_QIXES];             /* black_qix */

static struct splayer
{
    short i, j;                 /* position on board */
    short move, score, level, lives;
    bool drawing;
    bool gameover;
} player;

/*************************** STACK STUFF **********************/

/* the stack */
#define STACK_SIZE BOARD_W*BOARD_H
static struct pos
{
    int x, y;                   /* position on board */
} stack[STACK_SIZE];
static int stackPointer;

#define div(a,b) (((a)/(b)))

static bool pop (struct pos *p)
{
    if (stackPointer > 0) {
        p->x = stack[stackPointer].x;
        p->y = stack[stackPointer].y;
        stackPointer--;
        return true;
    } else
        return false;           /* SE */
}

static bool push (struct pos *p)
{
    if (stackPointer < STACK_SIZE - 1) {
        stackPointer++;
        stack[stackPointer].x = p->x;
        stack[stackPointer].y = p->y;
        return true;
    } else
        return false;           /* SOF */
}

static void emptyStack (void)
{
    stackPointer = 0;
    /* int x, y; 
       while(pop(&x, &y)); */
}

/*********************** END OF STACK STUFF *********************/


/* calculate the new x coordinate of the ball according to angle and speed */
static int get_newx (int x, int len, int deg)
{
    int dx;
    if ((deg == MOVE_DRR) || (deg == MOVE_URR))
        dx = 2;
    else if ((deg == MOVE_DDR) || (deg == MOVE_UUR) || (deg == MOVE_DR)
             || (deg == MOVE_UR))
        dx = 1;
    else if ((deg == MOVE_DDL) || (deg == MOVE_UUL) || (deg == MOVE_DL)
             || (deg == MOVE_UL))
        dx = -1;
    else
        dx = -2;
    return x + dx * len;
}

/* calculate the new y coordinate of the ball according to angle and speed */
static int get_newy (int y, int len, int deg)
{
    int dy;
    if ((deg == MOVE_DRR) || (deg == MOVE_DLL) || (deg == MOVE_DR)
        || (deg == MOVE_DL))
        dy = 1;
    else if ((deg == MOVE_DDR) || (deg == MOVE_DDL))
        dy = 2;
    else if ((deg == MOVE_UUR) || (deg == MOVE_UUL))
        dy = -2;
    else
        dy = -1;
    return y + dy * len;
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
    int i, j;                   /* testboard */
    for (j = 0; j < BOARD_H; j++)
        for (i = 0; i < BOARD_W; i++)
            testboard[j][i] = UNCHECKED;
}

/* initializes the game board on with the player,qix's and black qix */
static void init_board (void)
{
    int i, j;
    for (j = 0; j < BOARD_H; j++)
        for (i = 0; i < BOARD_W; i++) { /* make a nice cyan frame */
            if ((i == 0) || (j == 1) || (j == 0) || (i == BOARD_W - 1)
                || (j == BOARD_H - 1) || (j == BOARD_H - 2))
                board[j][i] = FILLED;
            else
                board[j][i] = EMPTIED;
        }
    /* (level+2) is the number of qixes */
    for (j = 0; j < player.level + STARTING_QIXES; j++) {
        qixes[j].velocity = t_rand (2) + 1;     /* 1 or 2 pix-per-sec */

        /* not on frame */
        qixes[j].x =
            BOARD_X + t_rand (((BOARD_W - 4) * CUBE_SIZE) - 2 * CUBE_SIZE) +
            2 * CUBE_SIZE;
        qixes[j].y =
            BOARD_Y + t_rand (((BOARD_H - 6) * CUBE_SIZE) - 2 * CUBE_SIZE) +
            3 * CUBE_SIZE;

        qixes[j].angle = t_rand (12);
    }
    /*black_qix.velocity=1;
       black_qix.x=BOARD_X+(BOARD_W*CUBE_SIZE)/2-CUBE_SIZE/2;
       black_qix.y=BOARD_Y+(BOARD_H*CUBE_SIZE)-CUBE_SIZE-CUBE_SIZE/2;
       black_qix.angle=MOVE_UR; */
    player.move = MOVE_NO;
    player.drawing = false;
    player.i = BOARD_W / 2;
    player.j = 1;
}

/* calculates the percentage of the screen filling */
static int percentage (void)
{
    int i, j, filled = 0;
    for (j = 2; j < BOARD_H - 2; j++)
        for (i = 1; i < BOARD_W - 1; i++)
            if (board[j][i] == FILLED)
                filled++;
    return filled * 100 / ((BOARD_W - 2) * (BOARD_H - 4));
}

/* draw the board on with all the game figures */
static void refresh_board (void)
{
    int i, j;
    char str[25];

    rb->lcd_set_background (LCD_BLACK);
    rb->lcd_clear_display ();
    for (j = 0; j < BOARD_H; j++)
        for (i = 0; i < BOARD_W; i++) {
            rb->lcd_set_foreground (board[j][i]);
            rb->lcd_fillrect (BOARD_X + CUBE_SIZE * i, BOARD_Y + CUBE_SIZE * j,
                              CUBE_SIZE, CUBE_SIZE);
        }
    rb->lcd_set_foreground (LCD_BLACK);
    rb->lcd_set_background (CLR_CYAN);
    rb->snprintf (str, sizeof (str), "Level %d", player.level + 1);
    rb->lcd_putsxy (BOARD_X, BOARD_Y, str);
    rb->snprintf (str, sizeof (str), "%d%%", percentage ());
    rb->lcd_putsxy (BOARD_X + CUBE_SIZE * BOARD_W - 24, BOARD_Y, str);
    rb->snprintf (str, sizeof (str), "Score: %d", player.score);
    rb->lcd_putsxy (BOARD_X, BOARD_Y + CUBE_SIZE * BOARD_H - 8, str);
    rb->snprintf (str, sizeof (str), "%d Lives", player.lives);
    rb->lcd_putsxy (BOARD_X + CUBE_SIZE * BOARD_W - 60,
                    BOARD_Y + CUBE_SIZE * BOARD_H - 8, str);

    rb->lcd_set_foreground (PLR_COL);
    rb->lcd_set_background (board[player.j][player.i]);
    rb->lcd_mono_bitmap (pics[PIC_PLAYER], player.i * CUBE_SIZE + BOARD_X,
                         player.j * CUBE_SIZE + BOARD_Y, CUBE_SIZE, CUBE_SIZE);
    rb->lcd_set_background (EMPTIED);
    rb->lcd_set_foreground (LCD_WHITE);
    for (j = 0; j < player.level + STARTING_QIXES; j++)
        rb->lcd_mono_bitmap (pics[PIC_QIX], qixes[j].x + BOARD_X,
                             qixes[j].y + BOARD_Y, CUBE_SIZE, CUBE_SIZE);
    rb->lcd_set_foreground (LCD_BLACK);

    rb->lcd_update ();
}

static inline int infested_area (int i, int j)
{
    struct pos p, p1, p2, p3, p4;
    bool hit = false;
    p.x = i;
    p.y = j;
    emptyStack ();
    init_testboard ();
    if (!push (&p))
        return -1;
    while (pop (&p)) {
        hit = (boardcopy[p.y][p.x] == QIX);
        testboard[p.y][p.x] = CHECKED;
        if (hit)
            return true;        /*save some time and space */
        p1.x = p.x + 1;
        p1.y = p.y;
        p2.x = p.x - 1;
        p2.y = p.y;
        p3.x = p.x;
        p3.y = p.y + 1;
        p4.x = p.x;
        p4.y = p.y - 1;
        if ((p1.x < BOARD_W) && (p1.x >= 0) && (p1.y < BOARD_H) && (p1.y >= 0)
            && (testboard[p1.y][p1.x] == UNCHECKED))
            if (board[p1.y][p1.x] != FILLED)
                if (!push (&p1))
                    return -1;
        if ((p2.x < BOARD_W) && (p2.x >= 0) && (p2.y < BOARD_H) && (p2.y >= 0)
            && (testboard[p2.y][p2.x] == UNCHECKED))
            if (board[p2.y][p2.x] != FILLED)
                if (!push (&p2))
                    return -1;
        if ((p3.x < BOARD_W) && (p3.x >= 0) && (p3.y < BOARD_H) && (p3.y >= 0)
            && (testboard[p3.y][p3.x] == UNCHECKED))
            if (board[p3.y][p3.x] != FILLED)
                if (!push (&p3))
                    return -1;
        if ((p4.x < BOARD_W) && (p4.x >= 0) && (p4.y < BOARD_H) && (p4.y >= 0)
            && (testboard[p4.y][p4.x] == UNCHECKED))
            if (board[p4.y][p4.x] != FILLED)
                if (!push (&p4))
                    return -1;
    }
    return (hit ? 1 : 0);
}

static inline int fill_area (int i, int j)
{
    struct pos p, p1, p2, p3, p4;
    p.x = i;
    p.y = j;
    emptyStack ();
    init_testboard ();
    if (!push (&p))
        return -1;
    while (pop (&p)) {
        board[p.y][p.x] = FILLED;
        testboard[p.y][p.x] = CHECKED;
        p1.x = p.x + 1;
        p1.y = p.y;
        p2.x = p.x - 1;
        p2.y = p.y;
        p3.x = p.x;
        p3.y = p.y + 1;
        p4.x = p.x;
        p4.y = p.y - 1;
        if ((p1.x < BOARD_W) && (p1.x >= 0) && (p1.y < BOARD_H) && (p1.y >= 0)
            && (testboard[p1.y][p1.x] == UNCHECKED))
            if (board[p1.y][p1.x] == EMPTIED)
                if (!push (&p1))
                    return -1;
        if ((p2.x < BOARD_W) && (p2.x >= 0) && (p2.y < BOARD_H) && (p2.y >= 0)
            && (testboard[p2.y][p2.x] == UNCHECKED))
            if (board[p2.y][p2.x] == EMPTIED)
                if (!push (&p2))
                    return -1;
        if ((p3.x < BOARD_W) && (p3.x >= 0) && (p3.y < BOARD_H) && (p3.y >= 0)
            && (testboard[p3.y][p3.x] == UNCHECKED))
            if (board[p3.y][p3.x] == EMPTIED)
                if (!push (&p3))
                    return -1;
        if ((p4.x < BOARD_W) && (p4.x >= 0) && (p4.y < BOARD_H) && (p4.y >= 0)
            && (testboard[p4.y][p4.x] == UNCHECKED))
            if (board[p4.y][p4.x] == EMPTIED)
                if (!push (&p4))
                    return -1;
    }
    return 1;
}


/* take care of stuff after xonix has landed on a filled spot */
static void complete_trail (int fill)
{
    int i, j, ret;
    for (j = 0; j < BOARD_H; j++)
        for (i = 0; i < BOARD_W; i++) {
            if (board[j][i] == TRAIL) {
                if (fill)
                    board[j][i] = FILLED;
                else
                    board[j][i] = EMPTIED;
            }
            boardcopy[j][i] = board[j][i];
        }

    if (fill) {
        for (i = 0; i < player.level + STARTING_QIXES; i++) /* add qixes to board */
            boardcopy[div (qixes[i].y - BOARD_Y, CUBE_SIZE)][div
                                                             (qixes[i].x - BOARD_X,
                                                              CUBE_SIZE)] = QIX;
    
        for (j = 1; j < BOARD_H - 1; j++)
            for (i = 0; i < BOARD_W - 0; i++)
                if (board[j][i] != FILLED) {
                    ret = infested_area (i, j);
                    if (ret < 0)
                        quit = true;
                    else if (ret == 0) {
                        ret = fill_area (i, j);
                        if (ret < 0)
                            quit = true;
                    }
                }
     }
}

/* returns the color the real pixel(x,y) on the lcd is pointing at */
static unsigned short getpixel (int x, int y)
{
    int a = div (x - BOARD_X, CUBE_SIZE), b = div (y - BOARD_Y, CUBE_SIZE);
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
static inline unsigned short next_hit (int newx, int newy)
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

/* returns true if the (side) of the block          -***-
   starting from (newx,newy) has any filled pixels  *   *
   -***-
 */
static bool line_check (int newx, int newy, int side)
{
    int i = 0;
    bool filled = false;
    for (i = 3; ((i < CUBE_SIZE - 3) && (!filled)); i++) {
        switch (side) {
            case MOVE_LT:
                filled = getpixel (newx, newy + i) == FILLED;
                break;
            case MOVE_RT:
                filled = getpixel (newx + CUBE_SIZE - 1, newy + i) == FILLED;
                break;
            case MOVE_UP:
                filled = getpixel (newx + i, newy) == FILLED;
                break;
            case MOVE_DN:
                filled = getpixel (newx + i, newy + CUBE_SIZE - 1) == FILLED;
                break;
        }
    }
    return filled;
}

static void die (void)
{
    player.lives--;
    if (player.lives == 0)
        player.gameover = true;
    else {
        refresh_board ();
        rb->splash (HZ, true, "Crash!");
        complete_trail (false);
        player.move = MOVE_NO;
        player.drawing = false;
        player.i = BOARD_W / 2;
        player.j = 1;
    }
}

static void move_qix (struct qix *q)
{
    int newx, newy, dir;
    unsigned short nexthit;
    newx = get_newx (q->x, q->velocity, q->angle);
    newy = get_newy (q->y, q->velocity, q->angle);
    nexthit = next_hit (newx, newy);
    if (nexthit == EMPTIED) {
        q->x = newx;
        q->y = newy;
    } else if (nexthit == FILLED) {
        dir = q->angle;
        switch (dir) {
            case MOVE_URR:
            case MOVE_UUR:
            case MOVE_UR:      /* up-right (can hit ceiling or right wall) */
                if (line_check (newx, newy, MOVE_UP)
                    && line_check (newx, newy, MOVE_RT))
                    q->angle = q->angle + 6;
                else if (line_check (newx, newy, MOVE_UP))
                    q->angle = 5 - q->angle;    /* 5=180/(360/12)-1 */
                else
                    q->angle = 11 - q->angle;   /* 11=360/(360/12)-1 */
                break;
            case MOVE_ULL:
            case MOVE_UUL:
            case MOVE_UL:      /* up-left (can hit ceiling or left wall) */
                if (line_check (newx, newy, MOVE_UP)
                    && line_check (newx, newy, MOVE_LT))
                    q->angle = q->angle - 6;
                else if (line_check (newx, newy, MOVE_UP))
                    q->angle = 17 - q->angle;   /* 17=540/(360/12)-1 */
                else
                    q->angle = 11 - q->angle;   /* 11=360/(360/12)-1 */
                break;
            case MOVE_DLL:
            case MOVE_DDL:
            case MOVE_DL:      /* down-left (can hit floor or left wall) */
                if (line_check (newx, newy, MOVE_DN)
                    && line_check (newx, newy, MOVE_LT))
                    q->angle = q->angle - 6;
                else if (line_check (newx, newy, MOVE_DN))
                    q->angle = 17 - q->angle;   /* 17=540/(360/12)-1 */
                else
                    q->angle = 11 - q->angle;   /* 11=360/(360/12)-1 */
                break;
            case MOVE_DRR:
            case MOVE_DDR:
            case MOVE_DR:      /* down-right (can hit floor or right wall) */
                if (line_check (newx, newy, MOVE_DN)
                    && line_check (newx, newy, MOVE_RT))
                    q->angle = q->angle + 6;
                else if (line_check (newx, newy, MOVE_DN))
                    q->angle = 5 - q->angle;    /* 5=180/(360/12)-1 */
                else
                    q->angle = 11 - q->angle;   /* 11=360/(360/12)-1 */
                break;
        }
        q->x = newx;
        q->y = newy;
    } else if (nexthit == TRAIL) {
        die();
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
        }
        player.i = newi;
        player.j = newj;
    }
    j = percentage ();
    if (j > 75) {               /* finished level */
        rb->splash (HZ * 2, true, "Level %d finished", player.level+1);
        player.score += j;
        if (player.level < MAX_LEVEL)
            player.level++;
        init_board ();
        refresh_board ();
        rb->splash (HZ * 2, true, "READY?");
    }
}

/* the main menu */
#define MAIN_MENU_SIZE 2
#define MENU_START 0
#define MENU_QUIT  1
static int game_menu (void)
{
    static char menu[MAIN_MENU_SIZE][15] = { 
        "Start New Game",
        "Quit"
    };

    int button, selection = 0, sw, sh, i;
    bool quit = false;
    rb->lcd_getstringsize ("A", NULL, &sh);
    rb->lcd_getstringsize ("XOBOX", &sw, NULL);
    sh++;
    rb->lcd_set_background (LCD_WHITE);
    rb->lcd_clear_display ();
    rb->lcd_putsxy (LCD_WIDTH / 2 - sw / 2, 2, "XOBOX");
    while (!quit) {
        for (i = 0; i < MAIN_MENU_SIZE; i++) {
            rb->lcd_set_foreground ((i == selection ? LCD_WHITE : LCD_BLACK));
            rb->lcd_set_background ((i == selection ? CLR_BLUE : LCD_WHITE));
            rb->lcd_putsxy (9, sh + 4 + i * sh, menu[i]);
        }
        rb->lcd_update ();
        button = rb->button_get (true);
        switch (button) {
            case UP:
                selection = (selection + MAIN_MENU_SIZE - 1) % MAIN_MENU_SIZE;
                break;
            case DOWN:
                selection = (selection + 1) % MAIN_MENU_SIZE;
                break;
            case SELECT:
            case RIGHT:
                quit = true;
                break;
            case QUIT:
                selection = MENU_QUIT;
                quit = true;
                break;
        }
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
    init_board ();
    refresh_board ();
    rb->splash (HZ * 2, true, "READY?");
}

/* general keypad handler loop */
static int xobox_loop (void)
{
    int button = 0, ret;
    bool pause = false;
    int end;

    while (!quit) {
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;
        button = rb->button_get_w_tmo (true);
        switch (button) {
            case UP:
                player.move = MOVE_UP;
                break;
            case DOWN:
                player.move = MOVE_DN;
                break;
            case LEFT:
                player.move = MOVE_LT;
                break;
            case RIGHT:
                player.move = MOVE_RT;
                break;
            case PAUSE:
                pause = !pause;
                if (pause)
                    rb->splash (HZ, true, "PAUSED");
                break;
            case QUIT:
                quit = true;
                return PLUGIN_OK;
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
            rb->splash (HZ, true, "GAME OVER");
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
enum plugin_status plugin_start (struct plugin_api *api, void *parameter)
{
    int ret;


    (void) parameter;
    rb = api;

    rb->lcd_setfont (FONT_SYSFIXED);

    /* Permanently enable the backlight (unless the user has turned it off) */
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout (1);

    ret = PLUGIN_OK;
    
    randomize ();
    if (game_menu () == MENU_START) {
        init_game ();
        ret = xobox_loop ();
    }

    rb->backlight_set_timeout (rb->global_settings->backlight_timeout);
    rb->lcd_setfont (FONT_UI);

    return ret;
}
