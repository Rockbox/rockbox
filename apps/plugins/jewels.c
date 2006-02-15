/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Adam Boot
*
* Color graphics from Gweled (http://sebdelestaing.free.fr/gweled/)
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

PLUGIN_HEADER

/* button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define BEJEWELED_UP     BUTTON_UP
#define BEJEWELED_DOWN   BUTTON_DOWN
#define BEJEWELED_LEFT   BUTTON_LEFT
#define BEJEWELED_RIGHT  BUTTON_RIGHT
#define BEJEWELED_QUIT   BUTTON_OFF
#define BEJEWELED_START  BUTTON_ON
#define BEJEWELED_SELECT BUTTON_PLAY
#define BEJEWELED_RESUME BUTTON_F1

#elif CONFIG_KEYPAD == ONDIO_PAD
#define BEJEWELED_UP         BUTTON_UP
#define BEJEWELED_DOWN       BUTTON_DOWN
#define BEJEWELED_LEFT       BUTTON_LEFT
#define BEJEWELED_RIGHT      BUTTON_RIGHT
#define BEJEWELED_QUIT       BUTTON_OFF
#define BEJEWELED_START      BUTTON_RIGHT
#define BEJEWELED_SELECT     (BUTTON_MENU|BUTTON_REL)
#define BEJEWELED_SELECT_PRE BUTTON_MENU
#define BEJEWELED_RESUME     (BUTTON_MENU|BUTTON_OFF)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BEJEWELED_UP     BUTTON_UP
#define BEJEWELED_DOWN   BUTTON_DOWN
#define BEJEWELED_LEFT   BUTTON_LEFT
#define BEJEWELED_RIGHT  BUTTON_RIGHT
#define BEJEWELED_QUIT   BUTTON_OFF
#define BEJEWELED_START  BUTTON_ON
#define BEJEWELED_SELECT BUTTON_SELECT
#define BEJEWELED_RESUME BUTTON_MODE

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define BEJEWELED_SCROLLWHEEL
#define BEJEWELED_UP         BUTTON_MENU
#define BEJEWELED_DOWN       BUTTON_PLAY
#define BEJEWELED_LEFT       BUTTON_LEFT
#define BEJEWELED_RIGHT      BUTTON_RIGHT
#define BEJEWELED_PREV       BUTTON_SCROLL_BACK
#define BEJEWELED_NEXT       BUTTON_SCROLL_FWD
#define BEJEWELED_QUIT       (BUTTON_SELECT|BUTTON_MENU)
#define BEJEWELED_START      BUTTON_PLAY
#define BEJEWELED_SELECT     (BUTTON_SELECT|BUTTON_REL)
#define BEJEWELED_SELECT_PRE BUTTON_SELECT
#define BEJEWELED_RESUME     (BUTTON_SELECT|BUTTON_PLAY)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define BEJEWELED_UP     BUTTON_UP
#define BEJEWELED_DOWN   BUTTON_DOWN
#define BEJEWELED_LEFT   BUTTON_LEFT
#define BEJEWELED_RIGHT  BUTTON_RIGHT
#define BEJEWELED_QUIT   BUTTON_PLAY
#define BEJEWELED_START  BUTTON_MODE
#define BEJEWELED_SELECT BUTTON_SELECT
#define BEJEWELED_RESUME BUTTON_EQ

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define BEJEWELED_UP     BUTTON_UP
#define BEJEWELED_DOWN   BUTTON_DOWN
#define BEJEWELED_LEFT   BUTTON_LEFT
#define BEJEWELED_RIGHT  BUTTON_RIGHT
#define BEJEWELED_QUIT   BUTTON_POWER
#define BEJEWELED_START  BUTTON_PLAY
#define BEJEWELED_SELECT BUTTON_MENU
#define BEJEWELED_RESUME BUTTON_REC

#else
    #error BEJEWELED: Unsupported keypad
#endif

/* use 30x30 tiles (iPod Video) */
#if (LCD_HEIGHT == 240) && (LCD_WIDTH == 320)
#define TILE_WIDTH  30
#define TILE_HEIGHT 30
#define YOFS 0
#define NUM_SCORES 10

/* use 22x22 tiles (H300, iPod Color) */
#elif (LCD_HEIGHT == 176) && (LCD_WIDTH == 220)
#define TILE_WIDTH  22
#define TILE_HEIGHT 22
#define YOFS 0
#define NUM_SCORES 10

/* use 16x16 tiles (iPod Nano) */
#elif (LCD_HEIGHT == 132) && (LCD_WIDTH == 176)
#define TILE_WIDTH  16
#define TILE_HEIGHT 16
#define YOFS 4
#define NUM_SCORES 10

/* use 16x16 tiles (H100, iAudio X5, iPod 3G, iPod 4G grayscale) */
#elif (LCD_HEIGHT == 128) && (LCD_WIDTH == 160)
#define TILE_WIDTH  16
#define TILE_HEIGHT 16
#define YOFS 0
#define NUM_SCORES 10

/* use 10x8 tiles (iFP 700) */
#elif (LCD_HEIGHT == 64) && (LCD_WIDTH == 128)
#define TILE_WIDTH 10
#define TILE_HEIGHT 8
#define YOFS 0
#define NUM_SCORES 8

/* use 10x8 tiles (Recorder, Ondio) */
#elif (LCD_HEIGHT == 64) && (LCD_WIDTH == 112)
#define TILE_WIDTH 10
#define TILE_HEIGHT 8
#define YOFS 0
#define NUM_SCORES 8

#else
  #error BEJEWELED: Unsupported LCD
#endif

/* tile background colors */
#if defined(HAVE_LCD_COLOR)
static const unsigned bejeweled_bkgd[2] = {
    LCD_RGBPACK(104, 63, 63),
    LCD_RGBPACK(83, 44, 44)
};
#endif

/* save files */
#define SCORE_FILE PLUGIN_DIR "/bejeweled.score"
#define SAVE_FILE  PLUGIN_DIR "/bejeweled.save"

/* final game return status */
#define BJ_END  3
#define BJ_USB  2
#define BJ_QUIT 1
#define BJ_LOSE 0

/* swap directions */
#define SWAP_UP    0
#define SWAP_RIGHT 1
#define SWAP_DOWN  2
#define SWAP_LEFT  3

/* play board dimension */
#define BJ_HEIGHT 9
#define BJ_WIDTH  8

/* next level threshold */
#define LEVEL_PTS 100

/* animation frame rate */
#define FPS 20

/* global rockbox api */
static struct plugin_api* rb;

/* external bitmaps */
extern const fb_data bejeweled_jewels[];

/* the tile struct
 * type is the jewel number 0-7
 * falling if the jewel is falling
 * delete marks the jewel for deletion
 */
struct tile {
    int type;
    bool falling;
    bool delete;
};

/* the game context struct
 * score is the current level score
 * segments is the number of cleared segments in the current run
 * level is the current level
 * highscores is the list of high scores
 * resume denotes whether to resume the currently loaded game
 * dirty denotes whether the high scores are out of sync with the saved file
 * playboard is the game playing board (first row is hidden)
 */
struct game_context {
    unsigned int score;
    unsigned int segments;
    unsigned int level;
    unsigned short highscores[NUM_SCORES];
    bool resume;
    bool dirty;
    struct tile playboard[BJ_HEIGHT][BJ_WIDTH];
};

/*****************************************************************************
* bejeweled_init() initializes bejeweled data structures.
******************************************************************************/
static void bejeweled_init(struct game_context* bj) {
    /* seed the rand generator */
    rb->srand(*rb->current_tick);

    /* check for resumed game */
    if(bj->resume) {
        bj->resume = false;
        return;
    }

    /* reset scoring */
    bj->level = 1;
    bj->score = 0;
    bj->segments = 0;

    /* clear playing board */
    rb->memset(bj->playboard, 0, sizeof(bj->playboard));
}

/*****************************************************************************
* bejeweled_setcolors() set the foreground and background colors.
******************************************************************************/
static inline void bejeweled_setcolors(void) {
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(49, 26, 26));
    rb->lcd_set_foreground(LCD_RGBPACK(210, 181, 181));
#endif
}

/*****************************************************************************
* bejeweled_drawboard() redraws the entire game board.
******************************************************************************/
static void bejeweled_drawboard(struct game_context* bj) {
    int i, j;
    int w, h;
    unsigned int tempscore;
    char *title = "Level";
    char str[6];

    tempscore = (bj->score>LEVEL_PTS ? LEVEL_PTS : bj->score);

    /* clear screen */
    rb->lcd_clear_display();

    /* draw separator lines */
    rb->lcd_vline(BJ_WIDTH*TILE_WIDTH, 0, LCD_HEIGHT);
    rb->lcd_hline(BJ_WIDTH*TILE_WIDTH, LCD_WIDTH, 18);
    rb->lcd_hline(BJ_WIDTH*TILE_WIDTH, LCD_WIDTH, LCD_HEIGHT-10);

    /* dispay playing board */
    for(i=0; i<BJ_HEIGHT-1; i++){
        for(j=0; j<BJ_WIDTH; j++){
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(bejeweled_bkgd[(i+j)%2]);
            rb->lcd_fillrect(j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                                TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_transparent_part(bejeweled_jewels,
                           0, TILE_HEIGHT*(bj->playboard[i+1][j].type),
                           TILE_WIDTH, j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#else
            rb->lcd_bitmap_part(bejeweled_jewels,
                           0, TILE_HEIGHT*(bj->playboard[i+1][j].type),
                           TILE_WIDTH, j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#endif
        }
    }

    /* draw progress bar */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(104, 63, 63));
#endif
    rb->lcd_fillrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4,
                     (LCD_HEIGHT-10)-(((LCD_HEIGHT-10)-18)*
                         tempscore/LEVEL_PTS),
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     ((LCD_HEIGHT-10)-18)*tempscore/LEVEL_PTS);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(83, 44, 44));
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4+1,
                     (LCD_HEIGHT-10)-(((LCD_HEIGHT-10)-18)*
                         tempscore/LEVEL_PTS)+1,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-2,
                     ((LCD_HEIGHT-10)-18)*tempscore/LEVEL_PTS-1);
    bejeweled_setcolors();
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4,
                     (LCD_HEIGHT-10)-(((LCD_HEIGHT-10)-18)*
                         tempscore/LEVEL_PTS),
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     ((LCD_HEIGHT-10)-18)*tempscore/LEVEL_PTS+1);
#endif

    /* print text */
    rb->lcd_getstringsize(title, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-w/2, 1, title);

    rb->snprintf(str, 4, "%d", bj->level);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-w/2, 10, str);

    rb->snprintf(str, 6, "%d", (bj->level-1)*LEVEL_PTS+bj->score);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-w/2,
                   LCD_HEIGHT-8,
                   str);

    rb->lcd_update();
}

/*****************************************************************************
* bejeweled_putjewels() makes the jewels fall to fill empty spots and adds
* new random jewels at the empty spots at the top of each row.
******************************************************************************/
static void bejeweled_putjewels(struct game_context* bj){
    int i, j, k;
    bool mark, done;
    long lasttick, currenttick;

    /* loop to make all the jewels fall */
    while(true) {
        /* mark falling jewels and add new jewels to hidden top row*/
        mark = false;
        done = true;
        for(j=0; j<BJ_WIDTH; j++) {
            if(bj->playboard[1][j].type == 0) {
                bj->playboard[0][j].type = rb->rand()%7+1;
            }
            for(i=BJ_HEIGHT-2; i>=0; i--) {
                if(!mark && bj->playboard[i+1][j].type == 0) {
                    mark = true;
                    done = false;
                }
                if(mark) bj->playboard[i][j].falling = true;
            }
            /*if(bj->playboard[1][j].falling) {
                bj->playboard[0][j].type = rb->rand()%7+1;
                bj->playboard[0][j].falling = true;
            }*/
            mark = false;
        }

        /* break if there are no falling jewels */
        if(done) break;

        /* animate falling jewels */
        lasttick = *rb->current_tick;

        for(k=1; k<=8; k++) {
            for(i=BJ_HEIGHT-2; i>=0; i--) {
                for(j=0; j<BJ_WIDTH; j++) {
                    if(bj->playboard[i][j].falling &&
                       bj->playboard[i][j].type != 0) {
                        /* clear old position */
#ifdef HAVE_LCD_COLOR
                        if(i == 0 && YOFS) {
                            rb->lcd_set_foreground(rb->lcd_get_background());
                        } else {
                            rb->lcd_set_foreground(bejeweled_bkgd[(i-1+j)%2]);
                        }
                        rb->lcd_fillrect(j*TILE_WIDTH, (i-1)*TILE_HEIGHT+YOFS,
                                            TILE_WIDTH, TILE_HEIGHT);
                        if(bj->playboard[i+1][j].type == 0) {
                            rb->lcd_set_foreground(bejeweled_bkgd[(i+j)%2]);
                            rb->lcd_fillrect(j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                                            TILE_WIDTH, TILE_HEIGHT);
                        }
#else
                        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        rb->lcd_fillrect(j*TILE_WIDTH, (i-1)*TILE_HEIGHT+YOFS,
                                            TILE_WIDTH, TILE_HEIGHT);
                        if(bj->playboard[i+1][j].type == 0) {
                            rb->lcd_fillrect(j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                                            TILE_WIDTH, TILE_HEIGHT);
                        }
                        rb->lcd_set_drawmode(DRMODE_SOLID);
#endif

                        /* draw new position */
#ifdef HAVE_LCD_COLOR
                        rb->lcd_bitmap_transparent_part(bejeweled_jewels, 0,
                                       TILE_HEIGHT*(bj->playboard[i][j].type),
                                       TILE_WIDTH, j*TILE_WIDTH,
                                       (i-1)*TILE_HEIGHT+YOFS+
                                           ((((TILE_HEIGHT<<10)*k)/8)>>10),
                                       TILE_WIDTH, TILE_HEIGHT);
#else
                        rb->lcd_bitmap_part(bejeweled_jewels, 0,
                                       TILE_HEIGHT*(bj->playboard[i][j].type),
                                       TILE_WIDTH, j*TILE_WIDTH,
                                       (i-1)*TILE_HEIGHT+YOFS+
                                           ((((TILE_HEIGHT<<10)*k)/8)>>10),
                                       TILE_WIDTH, TILE_HEIGHT);
#endif
                    }
                }
            }

            rb->lcd_update();
            bejeweled_setcolors();

            /* framerate limiting */
            currenttick = *rb->current_tick;
            if(currenttick-lasttick < HZ/FPS) {
                rb->sleep((HZ/FPS)-(currenttick-lasttick));
            }
            lasttick = currenttick;
        }

        /* shift jewels down */
        for(j=0; j<BJ_WIDTH; j++) {
            for(i=BJ_HEIGHT-1; i>=1; i--) {
                if(bj->playboard[i-1][j].falling) {
                    bj->playboard[i][j].type = bj->playboard[i-1][j].type;
                }
            }
        }

        /* clear out top row */
        for(j=0; j<BJ_WIDTH; j++) {
            bj->playboard[0][j].type = 0;
        }

        /* mark everything not falling */
        for(i=0; i<BJ_HEIGHT; i++) {
            for(j=0; j<BJ_WIDTH; j++) {
                bj->playboard[i][j].falling = false;
            }
        }
    }
}

/*****************************************************************************
* bejeweled_clearjewels() finds all the connected rows and columns and
*     calculates and returns the points earned.
******************************************************************************/
static unsigned int bejeweled_clearjewels(struct game_context* bj) {
    int i, j;
    int last, run;
    unsigned int points = 0;

    /* check for connected rows */
    for(i=1; i<BJ_HEIGHT; i++) {
        last = 0;
        run = 1;
        for(j=0; j<BJ_WIDTH; j++) {
            if(bj->playboard[i][j].type == last &&
               bj->playboard[i][j].type != 0) {
                run++;

                if(run == 3) {
                    bj->segments++;
                    points += bj->segments;
                    bj->playboard[i][j].delete = true;
                    bj->playboard[i][j-1].delete = true;
                    bj->playboard[i][j-2].delete = true;
                } else if(run > 3) {
                    points++;
                    bj->playboard[i][j].delete = true;
                }
            } else {
                run = 1;
                last = bj->playboard[i][j].type;
            }
        }
    }

    /* check for connected columns */
    for(j=0; j<BJ_WIDTH; j++) {
        last = 0;
        run = 1;
        for(i=1; i<BJ_HEIGHT; i++) {
            if(bj->playboard[i][j].type != 0 &&
               bj->playboard[i][j].type == last) {
                run++;

                if(run == 3) {
                    bj->segments++;
                    points += bj->segments;
                    bj->playboard[i][j].delete = true;
                    bj->playboard[i-1][j].delete = true;
                    bj->playboard[i-2][j].delete = true;
                } else if(run > 3) {
                    points++;
                    bj->playboard[i][j].delete = true;
                }
            } else {
                run = 1;
                last = bj->playboard[i][j].type;
            }
        }
    }

    /* clear deleted jewels */
    for(i=1; i<BJ_HEIGHT; i++) {
        for(j=0; j<BJ_WIDTH; j++) {
            if(bj->playboard[i][j].delete) {
                bj->playboard[i][j].delete = false;
                bj->playboard[i][j].type = 0;
            }
        }
    }

    return points;
}

/*****************************************************************************
* bejeweled_runboard() runs the board until it settles in a fixed state and
*     returns points earned.
******************************************************************************/
static unsigned int bejeweled_runboard(struct game_context* bj) {
    unsigned int points = 0;
    unsigned int ret;

    bj->segments = 0;

    while((ret = bejeweled_clearjewels(bj)) > 0) {
        points += ret;
        bejeweled_drawboard(bj);
        bejeweled_putjewels(bj);
    }

    return points;
}

/*****************************************************************************
* bejeweled_swapjewels() swaps two jewels as long as it results in points and
*     returns points earned.
******************************************************************************/
static unsigned int bejeweled_swapjewels(struct game_context* bj,
                                         int x, int y, int direc) {
    int k;
    int horzmod, vertmod;
    int movelen = 0;
    bool undo = false;
    unsigned int points = 0;
    long lasttick, currenttick;

    /* check for invalid parameters */
    if(x < 0 || x >= BJ_WIDTH || y < 0 || y >= BJ_HEIGHT-1 ||
       direc < SWAP_UP || direc > SWAP_LEFT) {
        return 0;
    }

    /* check for invalid directions */
    if((x == 0 && direc == SWAP_LEFT) ||
       (x == BJ_WIDTH-1 && direc == SWAP_RIGHT) ||
       (y == 0 && direc == SWAP_UP) ||
       (y == BJ_HEIGHT-2 && direc == SWAP_DOWN)) {
        return 0;
    }

    /* set direction variables */
    horzmod = 0;
    vertmod = 0;
    switch(direc) {
        case SWAP_UP:
            vertmod = -1;
            movelen = TILE_HEIGHT;
            break;
        case SWAP_RIGHT:
            horzmod = 1;
            movelen = TILE_WIDTH;
            break;
        case SWAP_DOWN:
            vertmod = 1;
            movelen = TILE_HEIGHT;
            break;
        case SWAP_LEFT:
            horzmod = -1;
            movelen = TILE_WIDTH;
            break;
    }

    while(true) {
        lasttick = *rb->current_tick;

        /* animate swapping jewels */
        for(k=0; k<=8; k++) {
            /* clear old position */
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(bejeweled_bkgd[(x+y)%2]);
            rb->lcd_fillrect(x*TILE_WIDTH,
                             y*TILE_HEIGHT+YOFS,
                             TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_foreground(bejeweled_bkgd[(x+horzmod+y+vertmod)%2]);
            rb->lcd_fillrect((x+horzmod)*TILE_WIDTH,
                             (y+vertmod)*TILE_HEIGHT+YOFS,
                             TILE_WIDTH, TILE_HEIGHT);
#else
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(x*TILE_WIDTH,
                             y*TILE_HEIGHT+YOFS,
                             TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_fillrect((x+horzmod)*TILE_WIDTH,
                             (y+vertmod)*TILE_HEIGHT+YOFS,
                             TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
            /* draw new position */
#ifdef HAVE_LCD_COLOR
            rb->lcd_bitmap_transparent_part(bejeweled_jewels,
                           0, TILE_HEIGHT*(bj->playboard
                               [y+1+vertmod][x+horzmod].type), TILE_WIDTH,
                           (x+horzmod)*TILE_WIDTH-horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           (y+vertmod)*TILE_HEIGHT-vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_transparent_part(bejeweled_jewels,
                           0, TILE_HEIGHT*(bj->playboard[y+1][x].type),
                           TILE_WIDTH, x*TILE_WIDTH+horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           y*TILE_HEIGHT+vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#else
            rb->lcd_bitmap_part(bejeweled_jewels,
                           0, TILE_HEIGHT*(bj->playboard
                               [y+1+vertmod][x+horzmod].type), TILE_WIDTH,
                           (x+horzmod)*TILE_WIDTH-horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           (y+vertmod)*TILE_HEIGHT-vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_FG);
            rb->lcd_bitmap_part(bejeweled_jewels,
                           0, TILE_HEIGHT*(bj->playboard[y+1][x].type),
                           TILE_WIDTH, x*TILE_WIDTH+horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           y*TILE_HEIGHT+vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
#endif

            rb->lcd_update();
            bejeweled_setcolors();

            /* framerate limiting */
            currenttick = *rb->current_tick;
            if(currenttick-lasttick < HZ/FPS) {
                rb->sleep((HZ/FPS)-(currenttick-lasttick));
            }
            lasttick = currenttick;
        }

        /* swap jewels */
        int temp = bj->playboard[y+1][x].type;
        bj->playboard[y+1][x].type =
            bj->playboard[y+1+vertmod][x+horzmod].type;
        bj->playboard[y+1+vertmod][x+horzmod].type = temp;

        if(undo) break;

        points = bejeweled_runboard(bj);
        if(points == 0) {
            undo = true;
        } else {
            break;
        }
    }

    return points;
}

/*****************************************************************************
* bejeweled_movesavail() uses pattern matching to see if there are any
*     available move left.
******************************************************************************/
static bool bejeweled_movesavail(struct game_context* bj) {
    int i, j;
    bool moves = false;
    int mytype;

    for(i=1; i<BJ_HEIGHT; i++) {
        for(j=0; j<BJ_WIDTH; j++) {
            mytype = bj->playboard[i][j].type;

            /* check horizontal patterns */
            if(j <= BJ_WIDTH-3) {
                if(i > 1) {
                    if(bj->playboard[i-1][j+1].type == mytype) {
                        if(bj->playboard[i-1][j+2].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[i][j+2].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[i][j+1].type == mytype) {
                        if(bj->playboard[i-1][j+2].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(j <= BJ_WIDTH-4) {
                    if(bj->playboard[i][j+3].type == mytype) {
                        if(bj->playboard[i][j+1].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[i][j+2].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(i < BJ_HEIGHT-1) {
                    if(bj->playboard[i][j+1].type == mytype) {
                        if(bj->playboard[i+1][j+2].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[i+1][j+1].type == mytype) {
                        if(bj->playboard[i][j+2].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[i+1][j+2].type == mytype)
                            {moves = true; break;}
                    }
                }
            }

            /* check vertical patterns */
            if(i <= BJ_HEIGHT-3) {
                if(j > 0) {
                    if(bj->playboard[i+1][j-1].type == mytype) {
                        if(bj->playboard[i+2][j-1].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[i+2][j].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[i+1][j].type == mytype) {
                        if(bj->playboard[i+2][j-1].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(i <= BJ_HEIGHT-4) {
                    if(bj->playboard[i+3][j].type == mytype) {
                        if(bj->playboard[i+1][j].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[i+2][j].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(j < BJ_WIDTH-1) {
                    if(bj->playboard[i+1][j].type == mytype) {
                        if(bj->playboard[i+2][j+1].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[i+1][j+1].type == mytype) {
                        if(bj->playboard[i+2][j].type == mytype)
                            {moves = true; break;}
                        if (bj->playboard[i+2][j+1].type == mytype)
                            {moves = true; break;}
                    }
                }
            }
        }

        if(moves) break;
    }

    return moves;
}

/*****************************************************************************
* bejeweled_nextlevel() advances the game to the next level and returns
*     points earned.
******************************************************************************/
static unsigned int bejeweled_nextlevel(struct game_context* bj) {
    int i, x, y;
    unsigned int points = 0;

    /* roll over score, change and display level */
    while(bj->score >= LEVEL_PTS) {
        bj->score -= LEVEL_PTS;
        bj->level++;
        rb->splash(HZ*2, true, "Level %d", bj->level);
        bejeweled_drawboard(bj);
    }

    /* randomly clear some jewels */
    for(i=0; i<16; i++) {
        x = rb->rand()%8;
        y = rb->rand()%8;

        if(bj->playboard[y][x].type != 0) {
            points++;
            bj->playboard[y][x].type = 0;
        }
    }
    bejeweled_drawboard(bj);

    /* run the play board */
    bejeweled_putjewels(bj);
    points += bejeweled_runboard(bj);
    return points;
}

/*****************************************************************************
* bejeweld_recordscore() inserts a high score into the high scores list and
*     returns the high score position.
******************************************************************************/
static int bejeweled_recordscore(struct game_context* bj) {
    int i;
    int position = 0;
    unsigned short current, temp;

    /* calculate total score */
    current = (bj->level-1)*LEVEL_PTS+bj->score;
    if(current <= 0) return 0;

    /* insert the current score into the high scores */
    for(i=0; i<NUM_SCORES; i++) {
        if(current >= bj->highscores[i]) {
            if(!position) {
                position = i+1;
                bj->dirty = true;
            }
            temp = bj->highscores[i];
            bj->highscores[i] = current;
            current = temp;
        }
    }

    return position;
 }

/*****************************************************************************
* bejeweled_loadscores() loads the high scores saved file.
******************************************************************************/
static void bejeweled_loadscores(struct game_context* bj) {
    int fd;

    bj->dirty = false;

    /* clear high scores */
    rb->memset(bj->highscores, 0, sizeof(bj->highscores));

    /* open scores file */
    fd = rb->open(SCORE_FILE, O_RDONLY);
    if(fd < 0) return;

    /* read in high scores */
    if(rb->read(fd, bj->highscores, sizeof(bj->highscores)) <= 0) {
        /* scores are bad, reset */
        rb->memset(bj->highscores, 0, sizeof(bj->highscores));
    }

    rb->close(fd);
}

/*****************************************************************************
* bejeweled_savescores() saves the high scores saved file.
******************************************************************************/
static void bejeweled_savescores(struct game_context* bj) {
    int fd;

    /* write out the high scores to the save file */
    fd = rb->open(SCORE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, bj->highscores, sizeof(bj->highscores));
    rb->close(fd);
    bj->dirty = false;
}

/*****************************************************************************
* bejeweled_loadgame() loads the saved game and returns load success.
******************************************************************************/
static bool bejeweled_loadgame(struct game_context* bj) {
    int fd;
    bool loaded = false;

    /* open game file */
    fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0) return loaded;

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &bj->score, sizeof(bj->score)) <= 0) break;
        if(rb->read(fd, &bj->level, sizeof(bj->level)) <= 0) break;
        if(rb->read(fd, bj->playboard, sizeof(bj->playboard)) <= 0) break;
        bj->resume = true;
        loaded = true;
        break;
    }

    rb->close(fd);

    /* delete saved file */
    rb->remove(SAVE_FILE);
    return loaded;
}

/*****************************************************************************
* bejeweled_savegame() saves the current game state.
******************************************************************************/
static void bejeweled_savegame(struct game_context* bj) {
    int fd;

    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &bj->score, sizeof(bj->score));
    rb->write(fd, &bj->level, sizeof(bj->level));
    rb->write(fd, bj->playboard, sizeof(bj->playboard));
    rb->close(fd);

    bj->resume = true;
}

/*****************************************************************************
* bejeweled_callback() is the default event handler callback which is called
*     on usb connect and shutdown.
******************************************************************************/
static void bejeweled_callback(void* param) {
    struct game_context* bj = (struct game_context*) param;
    if(bj->dirty) {
        rb->splash(HZ, true, "Saving high scores...");
        bejeweled_savescores(bj);
    }
}

/*****************************************************************************
* bejeweled() is the main game subroutine, it returns the final game status.
******************************************************************************/
static int bejeweled(struct game_context* bj) {
    int i, j;
    int w, h;
    int button;
    int lastbutton = BUTTON_NONE;
    char str[18];
    char *title = "Bejeweled";
    bool startgame = false;
    bool showscores = false;
    bool selected = false;

    /* the cursor coordinates */
    int x=0, y=0;

    /* don't resume by default */
    bj->resume = false;

    /********************
    *       menu        *
    ********************/
    while(!startgame){
        rb->lcd_clear_display();

        if(!showscores) {
             /* welcome screen to display key bindings */
            rb->lcd_getstringsize(title, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, title);
#if CONFIG_KEYPAD == RECORDER_PAD
            rb->lcd_puts(0, 1, "ON to start");
            rb->lcd_puts(0, 2, "F1 to save/resume");
            rb->lcd_puts(0, 3, "OFF to exit");
            rb->lcd_puts(0, 4, "PLAY to select");
            rb->lcd_puts(0, 5, "& show high scores");
            rb->lcd_puts(0, 6, "Directions to move");
            rb->snprintf(str, 18, "High Score: %d", bj->highscores[0]);
            rb->lcd_puts(0, 7, str);
#elif CONFIG_KEYPAD == ONDIO_PAD
            rb->lcd_puts(0, 1, "RIGHT to start");
            rb->lcd_puts(0, 2, "MENU+OFF to sv/res");
            rb->lcd_puts(0, 3, "OFF to exit");
            rb->lcd_puts(0, 4, "MENU to select");
            rb->lcd_puts(0, 5, "& show high scores");
            rb->lcd_puts(0, 6, "Directions to move");
            rb->snprintf(str, 18, "High Score: %d", bj->highscores[0]);
            rb->lcd_puts(0, 7, str);
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
            rb->lcd_puts(0, 2, "ON to start");
            rb->lcd_puts(0, 3, "MODE to save/resume");
            rb->lcd_puts(0, 4, "OFF to exit");
            rb->lcd_puts(0, 5, "SELECT to select");
            rb->lcd_puts(0, 6, " and show high scores");
            rb->lcd_puts(0, 7, "Directions to move");
            rb->snprintf(str, 18, "High Score: %d", bj->highscores[0]);
            rb->lcd_puts(0, 9, str);
#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
            rb->lcd_puts(0, 2, "PLAY to start");
            rb->lcd_puts(0, 3, "SELECT+PLAY to save/resume");
            rb->lcd_puts(0, 4, "SELECT+MENU to exit");
            rb->lcd_puts(0, 5, "SELECT to select");
            rb->lcd_puts(0, 6, " and show high scores");
            rb->lcd_puts(0, 7, "Scroll or Directions to move");
            rb->lcd_puts(0, 8, "Directions to swap");
            rb->snprintf(str, 18, "High Score: %d", bj->highscores[0]);
            rb->lcd_puts(0, 10, str);
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
            rb->lcd_puts(0, 1, "MODE to start");
            rb->lcd_puts(0, 2, "EQ to save/resume");
            rb->lcd_puts(0, 3, "PLAY to exit");
            rb->lcd_puts(0, 4, "SELECT to select");
            rb->lcd_puts(0, 5, "& show high scores");
            rb->lcd_puts(0, 6, "Directions to move");
            rb->snprintf(str, 18, "High Score: %d", bj->highscores[0]);
            rb->lcd_puts(0, 7, str);
#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
            rb->lcd_puts(0, 2, "PLAY to start");
            rb->lcd_puts(0, 3, "REC to save/resume");
            rb->lcd_puts(0, 4, "POWER to exit");
            rb->lcd_puts(0, 5, "MENU to select");
            rb->lcd_puts(0, 6, " and show high scores");
            rb->lcd_puts(0, 7, "Directions to move");
            rb->snprintf(str, 18, "High Score: %d", bj->highscores[0]);
#endif
        } else {
            /* room for a title? */
            j = 0;
            if(LCD_HEIGHT-NUM_SCORES*8 >= 8) {
                rb->snprintf(str, 12, "%s", "High Scores");
                rb->lcd_getstringsize(str, &w, &h);
                rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, str);
                j = 2;
            }

            /* print high scores */
            for(i=0; i<NUM_SCORES; i++) {
                rb->snprintf(str, 11, "#%02d: %d", i+1, bj->highscores[i]);
                rb->lcd_puts(0, i+j, str);
            }
        }

        rb->lcd_update();

        /* handle menu button presses */
        button = rb->button_get(true);
        switch(button){
            case BEJEWELED_START: /* start playing */
                startgame = true;
                break;

            case BEJEWELED_QUIT:  /* quit program */
                if(showscores) {
                    showscores = 0;
                    break;
                }
                return BJ_QUIT;

            case BEJEWELED_RESUME:/* resume game */
                if(!bejeweled_loadgame(bj)) {
                    rb->splash(HZ*2, true, "Nothing to resume");
                } else {
                    startgame = true;
                }
                break;

            case BEJEWELED_SELECT:/* toggle high scores */
#ifdef BEJEWELED_SELECT_PRE
                if(lastbutton != BEJEWELED_SELECT_PRE) break;
#endif
                showscores = !showscores;
                break;

            default:
                if(rb->default_event_handler_ex(button, bejeweled_callback,
                   (void*) bj) == SYS_USB_CONNECTED)
                    return BJ_USB;
                break;
        }

        if(button != BUTTON_NONE) lastbutton = button;
    }

    lastbutton = BUTTON_NONE;

    /********************
    *       init        *
    ********************/
    bejeweled_init(bj);

    /********************
    *  setup the board  *
    ********************/
    bejeweled_drawboard(bj);
    bejeweled_putjewels(bj);
    bj->score += bejeweled_runboard(bj);
    if (!bejeweled_movesavail(bj)) return BJ_LOSE;

    /**********************
    *        play         *
    **********************/
    while(true) {
        /* refresh the board */
        bejeweled_drawboard(bj);

        /* display the cursor */
        if(selected) {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(x*TILE_WIDTH, y*TILE_HEIGHT+YOFS,
                             TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        } else {
            rb->lcd_drawrect(x*TILE_WIDTH, y*TILE_HEIGHT+YOFS,
                             TILE_WIDTH, TILE_HEIGHT);
        }
        rb->lcd_update_rect(x*TILE_WIDTH, y*TILE_HEIGHT+YOFS,
                            TILE_WIDTH, TILE_HEIGHT);

        /* handle game button presses */
        button = rb->button_get(true);
        switch(button){
            case BEJEWELED_RESUME:           /* save and end game */
                rb->splash(HZ, true, "Saving game...");
                bejeweled_savegame(bj);
                return BJ_END;

            case BEJEWELED_QUIT:             /* end game */
                return BJ_END;

            case BEJEWELED_LEFT:             /* move cursor left */
            case (BEJEWELED_LEFT|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_LEFT);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    x = (x+BJ_WIDTH-1)%BJ_WIDTH;
                }
                break;

            case BEJEWELED_RIGHT:            /* move cursor right */
            case (BEJEWELED_RIGHT|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_RIGHT);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    x = (x+1)%BJ_WIDTH;
                }
                break;

            case BEJEWELED_DOWN:             /* move cursor down */
            case (BEJEWELED_DOWN|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_DOWN);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    y = (y+1)%(BJ_HEIGHT-1);
                }
                break;

            case BEJEWELED_UP:               /* move cursor up */
            case (BEJEWELED_UP|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_UP);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    y = (y+(BJ_HEIGHT-1)-1)%(BJ_HEIGHT-1);
                }
                break;

#ifdef BEJEWELED_SCROLLWHEEL
            case BEJEWELED_PREV:             /* scroll backwards */
            case (BEJEWELED_PREV|BUTTON_REPEAT):
                if(!selected) {
                    if(x == 0) {
                        y = (y+(BJ_HEIGHT-1)-1)%(BJ_HEIGHT-1);
                    }
                    x = (x+BJ_WIDTH-1)%BJ_WIDTH;
                }
                break;

            case BEJEWELED_NEXT:             /* scroll forwards */
            case (BEJEWELED_NEXT|BUTTON_REPEAT):
                if(!selected) {
                    if(x == BJ_WIDTH-1) {
                        y = (y+1)%(BJ_HEIGHT-1);
                    }
                    x = (x+1)%BJ_WIDTH;
                }
                break;
#endif

            case BEJEWELED_SELECT:           /* toggle selected */
#ifdef BEJEWELED_SELECT_PRE
                if(lastbutton != BEJEWELED_SELECT_PRE) break;
#endif
                selected = !selected;
                break;

            default:
                if(rb->default_event_handler_ex(button, bejeweled_callback,
                   (void*) bj) == SYS_USB_CONNECTED)
                    return BJ_USB;
                break;
        }

        if(button != BUTTON_NONE) lastbutton = button;
        if(bj->score >= LEVEL_PTS) bj->score = bejeweled_nextlevel(bj);
    }
}

/*****************************************************************************
* plugin entry point.
******************************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    struct game_context bj;
    bool exit = false;
    int position;
    char str[19];

    /* plugin init */
    (void)parameter;
    rb = api;
    /* end of plugin init */

    /* load high scores */
    bejeweled_loadscores(&bj);

    rb->lcd_setfont(FONT_SYSFIXED);
    bejeweled_setcolors();

    while(!exit) {
        switch(bejeweled(&bj)){
            case BJ_LOSE:
                rb->splash(HZ*2, true, "No more moves!");
                /* fall through to BJ_END */

            case BJ_END:
                if(!bj.resume) {
                    if((position = bejeweled_recordscore(&bj))) {
                        rb->snprintf(str, 19, "New high score #%d!", position);
                        rb->splash(HZ*2, true, str);
                    }
                }
                break;

            case BJ_USB:
                rb->lcd_setfont(FONT_UI);
                return PLUGIN_USB_CONNECTED;

            case BJ_QUIT:
                if(bj.dirty) {
                    rb->splash(HZ, true, "Saving high scores...");
                    bejeweled_savescores(&bj);
                }
                exit = true;
                break;

            default:
                break;
        }
    }

    rb->lcd_setfont(FONT_UI);
    return PLUGIN_OK;
}

#endif /* HAVE_LCD_BITMAP */
