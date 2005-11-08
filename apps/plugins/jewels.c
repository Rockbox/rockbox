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
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "button.h"
#include "lcd.h"

#ifdef HAVE_LCD_BITMAP

/* save files */
#define SCORE_FILE PLUGIN_DIR "/bejeweled.score"
#define SAVE_FILE  PLUGIN_DIR "/bejeweled.save"

/* final game return status */
#define BJ_END  3
#define BJ_USB  2
#define BJ_QUIT 1
#define BJ_LOSE 0

/* button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define BEJEWELED_QUIT   BUTTON_OFF
#define BEJEWELED_START  BUTTON_ON
#define BEJEWELED_SELECT BUTTON_PLAY
#define BEJEWELED_RESUME BUTTON_F1

#elif CONFIG_KEYPAD == ONDIO_PAD
#define BEJEWELED_QUIT       BUTTON_OFF
#define BEJEWELED_START      BUTTON_RIGHT
#define BEJEWELED_SELECT     (BUTTON_MENU|BUTTON_REL)
#define BEJEWELED_SELECT_PRE BUTTON_MENU
#define BEJEWELED_RESUME     (BUTTON_MENU|BUTTON_OFF)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BEJEWELED_QUIT   BUTTON_OFF
#define BEJEWELED_START  BUTTON_ON
#define BEJEWELED_SELECT BUTTON_SELECT
#define BEJEWELED_RESUME BUTTON_MODE

#elif
    #error BEJEWELED: Unsupported keypad
#endif

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

/* sleep time for animations (1/x seconds) */
#define FALL_TIMER 30
#define SWAP_TIMER 30

#if (LCD_HEIGHT == 128) && (LCD_WIDTH == 160)
/* Use 16x16 tiles */

/* size of a tile */
#define TILE_SZ 16

/* number of high scores to save */
#define NUM_SCORES 15

/* bitmaps for the jewels */
static unsigned char jewel[8][32] = {
    /* empty */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* square */
    {0x00, 0x00, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
     0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x00, 0x00,
     0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
     0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x00},
    /* plus */
    {0x00, 0xe0, 0xe0, 0x60, 0x60, 0x7e, 0x7e, 0x06,
     0x7e, 0x7e, 0x60, 0x60, 0xe0, 0xe0, 0x00, 0x00,
     0x00, 0x03, 0x03, 0x03, 0x03, 0x3f, 0x3f, 0x30,
     0x3f, 0x3f, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00},
    /* triangle */
    {0x00, 0x00, 0x00, 0x00, 0xc0, 0xf0, 0x7c, 0x1e,
     0x7c, 0xf0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x30, 0x3c, 0x3f, 0x37, 0x31, 0x30, 0x30,
     0x30, 0x31, 0x37, 0x3f, 0x3c, 0x30, 0x00, 0x00},
    /* diamond */
    {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe,
     0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00, 0x00,
     0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f,
     0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00},
    /* star */
    {0x00, 0x40, 0xc0, 0xc0, 0xc0, 0xc0, 0xf8, 0xfe,
     0xf8, 0xc0, 0xc0, 0xc0, 0xc0, 0x40, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x30, 0x1f, 0x1f, 0x0f, 0x07,
     0x0f, 0x1f, 0x1f, 0x30, 0x00, 0x00, 0x00, 0x00},
    /* circle */
    {0x00, 0xe0, 0xf8, 0xfc, 0x3c, 0x1e, 0x0e, 0x0e,
     0x0e, 0x1e, 0x3c, 0xfc, 0xf8, 0xe0, 0x00, 0x00,
     0x00, 0x03, 0x0f, 0x1f, 0x1e, 0x3c, 0x38, 0x38,
     0x38, 0x3c, 0x1e, 0x1f, 0x0f, 0x03, 0x00, 0x00},
    /* heart */
    {0x00, 0x78, 0xfc, 0xfe, 0xfe, 0xfc, 0xf8, 0xf0,
     0xf8, 0xfc, 0xfe, 0xfe, 0xfc, 0x78, 0x00, 0x00,
     0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f,
     0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00}
};

#elif (LCD_HEIGHT == 64) && (LCD_WIDTH == 112)
/* Use 8x8 tiles */

/* size of a tile */
#define TILE_SZ 8

/* number of high scores to save */
#define NUM_SCORES 8

/* bitmaps for the jewels */
static unsigned char jewel[8][8] = {
    /* empty */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* square */
    {0x00, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x00},
    /* plus */
    {0x1c, 0x14, 0x77, 0x41, 0x77, 0x14, 0x1c, 0x00},
    /* triangle */
    {0x60, 0x78, 0x4e, 0x43, 0x4e, 0x78, 0x60, 0x00},
    /* diamond */
    {0x08, 0x1c, 0x3e, 0x7f, 0x3e, 0x1c, 0x08, 0x00},
    /* star */
    {0x08, 0x68, 0x3c, 0x1f, 0x3c, 0x68, 0x08, 0x00},
    /* circle */
    {0x1c, 0x3e, 0x63, 0x63, 0x63, 0x3e, 0x1c, 0x00},
    /* heart */
    {0x0e, 0x1f, 0x3e, 0x7c, 0x3e, 0x1f, 0x0e, 0x00}
};

#else
  #error BEJEWELED: Unsupported LCD size
#endif

/* global rockbox api */
static struct plugin_api* rb;

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
    struct tile playboard[BJ_WIDTH][BJ_HEIGHT];
};

/*****************************************************************************
* bejeweled_init() initializes bejeweled data structures.
******************************************************************************/
void bejeweled_init(struct game_context* bj) {
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
* bejeweled_drawboard() redraws the entire game board.
******************************************************************************/
void bejeweled_drawboard(struct game_context* bj) {
    int i, j;
    int w, h;
    unsigned int tempscore;
    char *title = "Level";
    char str[6];

    tempscore = (bj->score>LEVEL_PTS ? LEVEL_PTS : bj->score);

    /* clear screen */
    rb->lcd_clear_display();

    /* draw separator lines */
    rb->lcd_vline(BJ_WIDTH*TILE_SZ, 0, LCD_HEIGHT);
    rb->lcd_hline(BJ_WIDTH*TILE_SZ, LCD_WIDTH, 18);
    rb->lcd_hline(BJ_WIDTH*TILE_SZ, LCD_WIDTH, LCD_HEIGHT-10);

    /* draw progress bar */
    rb->lcd_fillrect(BJ_WIDTH*TILE_SZ+(LCD_WIDTH-BJ_WIDTH*TILE_SZ)/4,
                     (LCD_HEIGHT-10)-(((LCD_HEIGHT-10)-18)*
                         tempscore/LEVEL_PTS),
                     (LCD_WIDTH-BJ_WIDTH*TILE_SZ)/2,
                     ((LCD_HEIGHT-10)-18)*tempscore/LEVEL_PTS);

    /* dispay playing board */
    for(i=0; i<BJ_HEIGHT-1; i++){
        for(j=0; j<BJ_WIDTH; j++){
            rb->lcd_mono_bitmap(jewel[bj->playboard[j][i+1].type],
                                j*TILE_SZ, i*TILE_SZ, TILE_SZ, TILE_SZ);
        }
    }

    /* print text */
    rb->lcd_getstringsize(title, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_SZ)/2-w/2, 1, title);

    rb->snprintf(str, 4, "%d", bj->level);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_SZ)/2-w/2, 10, str);

    rb->snprintf(str, 6, "%d", (bj->level-1)*LEVEL_PTS+bj->score);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_SZ)/2-w/2,
                   LCD_HEIGHT-8,
                   str);

    rb->lcd_update();
}

/*****************************************************************************
* bejeweled_putjewels() makes the jewels fall to fill empty spots and adds
* new random jewels at the empty spots at the top of each row.
******************************************************************************/
void bejeweled_putjewels(struct game_context* bj){
    int i, j, k;
    bool mark, done;

    /* loop to make all the jewels fall */
    while(true) {
        /* mark falling jewels and add new jewels to hidden top row*/
        mark = false;
        done = true;
        for(j=0; j<BJ_WIDTH; j++) {
            if(bj->playboard[j][1].type == 0) {
                bj->playboard[j][0].type = rb->rand()%7+1;
            }
            for(i=BJ_HEIGHT-2; i>=0; i--) {
                if(!mark && bj->playboard[j][i+1].type == 0) {
                    mark = true;
                    done = false;
                }
                if(mark) bj->playboard[j][i].falling = true;
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
        for(k=TILE_SZ/8; k<=TILE_SZ; k+=TILE_SZ/8) {
            rb->sleep(HZ/FALL_TIMER);
            for(i=0; i<BJ_HEIGHT-1; i++) {
                for(j=0; j<BJ_WIDTH; j++) {
                    if(bj->playboard[j][i].falling &&
                       bj->playboard[j][i].type != 0) {
                        /* clear old position */
                        rb->lcd_mono_bitmap(jewel[0],
                                            j*TILE_SZ,
                                            (i-1)*TILE_SZ+k-2,
                                            TILE_SZ, TILE_SZ);
                        /* draw new position */
                        rb->lcd_mono_bitmap(jewel[bj->playboard[j][i].type],
                                            j*TILE_SZ,
                                            (i-1)*TILE_SZ+k,
                                            TILE_SZ, TILE_SZ);
                    }
                }
            }
            rb->lcd_update();
        }

        /* shift jewels down */
        for(j=0; j<BJ_WIDTH; j++) {
            for(i=BJ_HEIGHT-1; i>=1; i--) {
                if(bj->playboard[j][i-1].falling) {
                    bj->playboard[j][i].type = bj->playboard[j][i-1].type;
                }
            }
        }

        /* clear out top row */
        for(j=0; j<BJ_WIDTH; j++) {
            bj->playboard[j][0].type = 0;
        }

        /* mark everything not falling */
        for(i=0; i<BJ_HEIGHT; i++) {
            for(j=0; j<BJ_WIDTH; j++) {
                bj->playboard[j][i].falling = false;
            }
        }
    }
}

/*****************************************************************************
* bejeweled_clearjewels() finds all the connected rows and columns and
*     calculates and returns the points earned.
******************************************************************************/
unsigned int bejeweled_clearjewels(struct game_context* bj) {
    int i, j;
    int last, run;
    unsigned int points = 0;

    /* check for connected rows */
    for(i=1; i<BJ_HEIGHT; i++) {
        last = 0;
        run = 1;
        for(j=0; j<BJ_WIDTH; j++) {
            if(bj->playboard[j][i].type == last &&
               bj->playboard[j][i].type != 0) {
                run++;

                if(run == 3) {
                    bj->segments++;
                    points += bj->segments;
                    bj->playboard[j][i].delete = true;
                    bj->playboard[j-1][i].delete = true;
                    bj->playboard[j-2][i].delete = true;
                } else if(run > 3) {
                    points++;
                    bj->playboard[j][i].delete = true;
                }
            } else {
                run = 1;
                last = bj->playboard[j][i].type;
            }
        }
    }

    /* check for connected columns */
    for(j=0; j<BJ_WIDTH; j++) {
        last = 0;
        run = 1;
        for(i=1; i<BJ_HEIGHT; i++) {
            if(bj->playboard[j][i].type != 0 &&
               bj->playboard[j][i].type == last) {
                run++;

                if(run == 3) {
                    bj->segments++;
                    points += bj->segments;
                    bj->playboard[j][i].delete = true;
                    bj->playboard[j][i-1].delete = true;
                    bj->playboard[j][i-2].delete = true;
                } else if(run > 3) {
                    points++;
                    bj->playboard[j][i].delete = true;
                }
            } else {
                run = 1;
                last = bj->playboard[j][i].type;
            }
        }
    }

    /* clear deleted jewels */
    for(i=1; i<BJ_HEIGHT; i++) {
        for(j=0; j<BJ_WIDTH; j++) {
            if(bj->playboard[j][i].delete) {
                bj->playboard[j][i].delete = false;
                bj->playboard[j][i].type = 0;
            }
        }
    }

    bejeweled_drawboard(bj);
    return points;
}

/*****************************************************************************
* bejeweled_runboard() runs the board until it settles in a fixed state and
*     returns points earned.
******************************************************************************/
unsigned int bejeweled_runboard(struct game_context* bj) {
    unsigned int points = 0;
    unsigned int ret;

    bj->segments = 0;

    while((ret = bejeweled_clearjewels(bj)) > 0) {
        points += ret;
        bejeweled_putjewels(bj);
    }

    return points;
}

/*****************************************************************************
* bejeweled_swapjewels() swaps two jewels as long as it results in points and
*     returns points earned.
******************************************************************************/
unsigned int bejeweled_swapjewels(struct game_context* bj,
                                  int x, int y, int direc) {
    int k;
    int horzmod, vertmod;
    bool undo = false;
    unsigned int points = 0;

    /* check for invalid parameters */
    if(x < 0 || x >= BJ_WIDTH || y < 0 || y >= BJ_HEIGHT-1 ||
       direc < SWAP_UP || direc > SWAP_LEFT) return 0;

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
            vertmod = -1; break;
        case SWAP_RIGHT:
            horzmod = 1;  break;
        case SWAP_DOWN:
            vertmod = 1;  break;
        case SWAP_LEFT:
            horzmod = -1; break;
    }

    while(true) {
        /* animate swapping jewels */
        for(k=TILE_SZ/8; k<=TILE_SZ; k+=TILE_SZ/8) {
            rb->sleep(HZ/SWAP_TIMER);
            /* clear old position */
            rb->lcd_mono_bitmap(jewel[0],
                                x*TILE_SZ+horzmod*(k-TILE_SZ/8),
                                y*TILE_SZ+vertmod*(k-TILE_SZ/8),
                                TILE_SZ, TILE_SZ);
            rb->lcd_mono_bitmap(jewel[0],
                                (x+horzmod)*TILE_SZ+horzmod*(k-TILE_SZ/8)*-1,
                                (y+vertmod)*TILE_SZ+vertmod*(k-TILE_SZ/8)*-1,
                                TILE_SZ, TILE_SZ);
            /* draw new position */
            rb->lcd_mono_bitmap(jewel[bj->playboard[x][y+1].type],
                                x*TILE_SZ+horzmod*k,
                                y*TILE_SZ+vertmod*k,
                                TILE_SZ, TILE_SZ);
            rb->lcd_set_drawmode(DRMODE_FG);
            rb->lcd_mono_bitmap(jewel[bj->playboard
                                [x+horzmod][y+1+vertmod].type],
                                (x+horzmod)*TILE_SZ+horzmod*k*-1,
                                (y+vertmod)*TILE_SZ+vertmod*k*-1,
                                TILE_SZ, TILE_SZ);
            rb->lcd_set_drawmode(DRMODE_SOLID);

            rb->lcd_update();
        }

        /* swap jewels */
        int temp = bj->playboard[x][y+1].type;
        bj->playboard[x][y+1].type =
            bj->playboard[x+horzmod][y+1+vertmod].type;
        bj->playboard[x+horzmod][y+1+vertmod].type = temp;

        if(undo) break;

        points = bejeweled_runboard(bj);
        if(points == 0) {undo = true;} else {break;}
    }

    return points;
}

/*****************************************************************************
* bejeweled_movesavail() uses pattern matching to see if there are any
*     available move left.
******************************************************************************/
bool bejeweled_movesavail(struct game_context* bj) {
    int i, j;
    bool moves = false;
    int mytype;

    for(i=1; i<BJ_HEIGHT; i++) {
        for(j=0; j<BJ_WIDTH; j++) {
            mytype = bj->playboard[j][i].type;

            /* check horizontal patterns */
            if(j <= BJ_WIDTH-3) {
                if(i > 1) {
                    if(bj->playboard[j+1][i-1].type == mytype) {
                        if(bj->playboard[j+2][i-1].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[j+2][i].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[j+1][i].type == mytype) {
                        if(bj->playboard[j+2][i-1].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(j <= BJ_WIDTH-4) {
                    if(bj->playboard[j+3][i].type == mytype) {
                        if(bj->playboard[j+1][i].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[j+2][i].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(i < BJ_HEIGHT-1) {
                    if(bj->playboard[j+1][i].type == mytype) {
                        if(bj->playboard[j+2][i+1].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[j+1][i+1].type == mytype) {
                        if(bj->playboard[j+2][i].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[j+2][i+1].type == mytype)
                            {moves = true; break;}
                    }
                }
            }

            /* check vertical patterns */
            if(i <= BJ_HEIGHT-3) {
                if(j > 0) {
                    if(bj->playboard[j-1][i+1].type == mytype) {
                        if(bj->playboard[j-1][i+2].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[j][i+2].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[j][i+1].type == mytype) {
                        if(bj->playboard[j-1][i+2].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(i <= BJ_HEIGHT-4) {
                    if(bj->playboard[j][i+3].type == mytype) {
                        if(bj->playboard[j][i+1].type == mytype)
                            {moves = true; break;}
                        if(bj->playboard[j][i+2].type == mytype)
                            {moves = true; break;}
                    }
                }

                if(j < BJ_WIDTH-1) {
                    if(bj->playboard[j][i+1].type == mytype) {
                        if(bj->playboard[j+1][i+2].type == mytype)
                            {moves = true; break;}
                    }
                    if(bj->playboard[j+1][i+1].type == mytype) {
                        if(bj->playboard[j][i+2].type == mytype)
                            {moves = true; break;}
                        if (bj->playboard[j+1][i+2].type == mytype)
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
unsigned int bejeweled_nextlevel(struct game_context* bj) {
    int i, x, y;
    unsigned int points = 0;

    /* roll over score, change and display level */
    while(bj->score >= LEVEL_PTS) {
        bj->score -= LEVEL_PTS;
        bj->level++;
        bejeweled_drawboard(bj);
        rb->splash(HZ*2, true, "Level %d", bj->level);
        bejeweled_drawboard(bj);
    }

    /* randomly clear some jewels */
    for(i=0; i<16; i++) {
        x = rb->rand()%8;
        y = rb->rand()%8;

        if(bj->playboard[x][y].type != 0) {
            points++;
            bj->playboard[x][y].type = 0;
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
 int bejeweled_recordscore(struct game_context* bj) {
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
void bejeweled_loadscores(struct game_context* bj) {
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
* bejeweled_savescores() saves the high scores.
******************************************************************************/
void bejeweled_savescores(struct game_context* bj) {
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
bool bejeweled_loadgame(struct game_context* bj) {
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
void bejeweled_savegame(struct game_context* bj) {
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
void bejeweled_callback(void* param) {
    struct game_context* bj = (struct game_context*) param;
    if(bj->dirty) {
        rb->splash(HZ, true, "Saving high scores...");
        bejeweled_savescores(bj);
    }
}

/*****************************************************************************
* bejeweled() is the main game subroutine, it returns the final game status.
******************************************************************************/
int bejeweled(struct game_context* bj) {
    int i, j;
    int w, h;
    int button;
    int lastbutton = BUTTON_NONE;
    char str[18];
    char *title = "Bejeweled";
    bool breakout = false;
    bool showscores = false;
    bool selected = false;

    /* the cursor coordinates */
    int x=0, y=0;

    /* don't resume by deafult */
    bj->resume = false;

    /********************
    *       menu        *
    ********************/
    while(true){
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
#endif
        } else {
            /* room for a title? */
            j = 0;
            if(LCD_HEIGHT-NUM_SCORES*8 >= 8) {
                rb->snprintf(str, 12, "%s", "High Scores");
                rb->lcd_getstringsize(str, &w, &h);
                rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, str);
                j = 1;
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
                breakout = true;
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
                    breakout = true;
                }
                break;

            case BEJEWELED_SELECT:/* toggle high scores */
#ifdef BEJEWELED_SELECT_PRE
                if(lastbutton != BEJEWELED_SELECT_PRE) break;
#endif
                showscores ^= 1;
                break;

            default:
                if(rb->default_event_handler_ex(button, bejeweled_callback,
                   (void*) bj) == SYS_USB_CONNECTED)
                    return BJ_USB;
                break;
        }

        if(breakout) break;
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

    /**********************
    *        play         *
    **********************/
    while(true) {
        /* refresh the board */
        bejeweled_drawboard(bj);

        /* display the cursor */
        if(selected) {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        } else {
            rb->lcd_drawrect(x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ);
        }
        rb->lcd_update_rect(x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ);

        /* handle game button presses */
        button = rb->button_get(true);
        switch(button){
            case BEJEWELED_RESUME:           /* save and end game */
                rb->splash(HZ, true, "Saving game...");
                bejeweled_savegame(bj);
                /* fall through to BEJEWELED_QUIT */

            case BEJEWELED_QUIT:             /* end game */
                return BJ_END;

            case BUTTON_LEFT:                /* move cursor left */
            case (BUTTON_LEFT|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_LEFT);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    x = (x+BJ_WIDTH-1)%BJ_WIDTH;
                }
                break;

            case BUTTON_RIGHT:               /* move cursor right */
            case (BUTTON_RIGHT|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_RIGHT);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    x = (x+1)%BJ_WIDTH;
                }
                break;

            case BUTTON_DOWN:                /* move cursor down */
            case (BUTTON_DOWN|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_DOWN);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    y = (y+1)%(BJ_HEIGHT-1);
                }
                break;

            case BUTTON_UP:                  /* move cursor up */
            case (BUTTON_UP|BUTTON_REPEAT):
                if(selected) {
                    bj->score += bejeweled_swapjewels(bj, x, y, SWAP_UP);
                    selected = false;
                    if (!bejeweled_movesavail(bj)) return BJ_LOSE;
                } else {
                    y = (y+(BJ_HEIGHT-1)-1)%(BJ_HEIGHT-1);
                }
                break;

            case BEJEWELED_SELECT:           /* toggle selected */
#ifdef BEJEWELED_SELECT_PRE
                if(lastbutton != BEJEWELED_SELECT_PRE) break;
#endif
                selected ^= 1;
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
    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;
    /* end of plugin init */

    /* load high scores */
    bejeweled_loadscores(&bj);

    rb->lcd_setfont(FONT_SYSFIXED);

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

#endif
