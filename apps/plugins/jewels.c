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
#include "lib/highscore.h"
#include "lib/playback_control.h"
#include "pluginbitmaps/jewels.h"

#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/* button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_PLAY
#define JEWELS_CANCEL BUTTON_OFF
#define HK_SELECT "PLAY"
#define HK_CANCEL "OFF"

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_OFF
#define HK_SELECT "SELECT"
#define HK_CANCEL "OFF"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_MENU
#define JEWELS_CANCEL BUTTON_OFF
#define HK_SELECT "MENU"
#define HK_CANCEL "OFF"

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_OFF
#define JEWELS_RC_CANCEL BUTTON_RC_STOP
#define HK_SELECT "SELECT"
#define HK_CANCEL "OFF"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define JEWELS_SCROLLWHEEL
#define JEWELS_UP     BUTTON_MENU
#define JEWELS_DOWN   BUTTON_PLAY
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_PREV   BUTTON_SCROLL_BACK
#define JEWELS_NEXT   BUTTON_SCROLL_FWD
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL (BUTTON_SELECT | BUTTON_MENU)
#define HK_SELECT "SELECT"
#define HK_CANCEL "SEL + MENU"

#elif (CONFIG_KEYPAD == IPOD_3G_PAD)
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_UP     BUTTON_SCROLL_BACK
#define JEWELS_DOWN   BUTTON_SCROLL_FWD
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_MENU
#define HK_SELECT "SELECT"
#define HK_CANCEL "MENU"

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_PLAY
#define HK_SELECT "SELECT"
#define HK_CANCEL "PLAY"

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "SELECT"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "SELECT"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define JEWELS_SCROLLWHEEL
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_PREV   BUTTON_SCROLL_BACK
#define JEWELS_NEXT   BUTTON_SCROLL_FWD
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "SELECT"
#define HK_CANCEL "POWER"

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define JEWELS_SCROLLWHEEL
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_PREV   BUTTON_SCROLL_BACK
#define JEWELS_NEXT   BUTTON_SCROLL_FWD
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL (BUTTON_HOME|BUTTON_REPEAT)
#define HK_SELECT "SELECT"
#define HK_CANCEL "HOME"

#elif CONFIG_KEYPAD == SANSA_C200_PAD || \
CONFIG_KEYPAD == SANSA_CLIP_PAD || \
CONFIG_KEYPAD == SANSA_M200_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "SELECT"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define JEWELS_UP     BUTTON_SCROLL_UP
#define JEWELS_DOWN   BUTTON_SCROLL_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_PLAY
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "PLAY"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_BACK
#define HK_SELECT "SELECT"
#define HK_CANCEL "BACK"

#elif CONFIG_KEYPAD == MROBE100_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "SELECT"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define JEWELS_UP     BUTTON_RC_VOL_UP
#define JEWELS_DOWN   BUTTON_RC_VOL_DOWN
#define JEWELS_LEFT   BUTTON_RC_REW
#define JEWELS_RIGHT  BUTTON_RC_FF
#define JEWELS_SELECT BUTTON_RC_PLAY
#define JEWELS_CANCEL BUTTON_RC_REC
#define HK_SELECT "PLAY"
#define HK_CANCEL "REC"

#define JEWELS_RC_CANCEL BUTTON_REC

#elif CONFIG_KEYPAD == COWON_D2_PAD
#define JEWELS_CANCEL BUTTON_POWER
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define JEWELS_UP     BUTTON_STOP
#define JEWELS_DOWN   BUTTON_PLAY
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_MENU
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "MENU"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_BACK
#define HK_SELECT "MIDDLE"
#define HK_CANCEL "BACK"

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "SELECT"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_PREV
#define JEWELS_RIGHT  BUTTON_NEXT
#define JEWELS_SELECT BUTTON_PLAY
#define JEWELS_CANCEL BUTTON_POWER
#define HK_SELECT "PLAY"
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == ONDAVX747_PAD || \
CONFIG_KEYPAD == ONDAVX777_PAD || \
CONFIG_KEYPAD == MROBE500_PAD
#define JEWELS_CANCEL BUTTON_POWER
#define HK_CANCEL "POWER"

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_PLAY
#define JEWELS_CANCEL BUTTON_REW
#define HK_SELECT "PLAY"
#define HK_CANCEL "REWIND"

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_PREV
#define JEWELS_RIGHT  BUTTON_NEXT
#define JEWELS_SELECT BUTTON_OK
#define JEWELS_CANCEL BUTTON_REC
#define HK_SELECT "OK"
#define HK_CANCEL "REC"

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef JEWELS_UP
#define JEWELS_UP     BUTTON_TOPMIDDLE
#endif
#ifndef JEWELS_DOWN
#define JEWELS_DOWN   BUTTON_BOTTOMMIDDLE
#endif
#ifndef JEWELS_LEFT
#define JEWELS_LEFT   BUTTON_MIDLEFT
#endif
#ifndef JEWELS_RIGHT
#define JEWELS_RIGHT  BUTTON_MIDRIGHT
#endif
#ifndef JEWELS_SELECT
#define JEWELS_SELECT BUTTON_CENTER
#define HK_SELECT "CENTER"
#endif
#ifndef JEWELS_CANCEL
#define JEWELS_CANCEL BUTTON_TOPLEFT
#define HK_CANCEL "TOPLEFT"
#endif
#endif

#define TILE_WIDTH  BMPWIDTH_jewels
#define TILE_HEIGHT (BMPHEIGHT_jewels/23)

#if LCD_HEIGHT < LCD_WIDTH
/* This calculation assumes integer division w/ LCD_HEIGHT/TILE_HEIGHT */
#define YOFS        LCD_HEIGHT-((LCD_HEIGHT/TILE_HEIGHT)*TILE_HEIGHT)
#else
#define YOFS        0
#endif

#define NUM_SCORES  5

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
#define MAX_FPS 20

/* text margin */
#define MARGIN 5

/* Game types */
enum game_type {
    GAME_TYPE_NORMAL,
    GAME_TYPE_PUZZLE
};

/* external bitmaps */
extern const fb_data jewels[];

/* tile background colors */
#ifdef HAVE_LCD_COLOR
static const unsigned jewels_bkgd[2] = {
    LCD_RGBPACK(104, 63, 63),
    LCD_RGBPACK(83, 44, 44)
};
#endif

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
 * tmp_type is the select type in the menu
 * type is the game type (normal or puzzle)
 * playboard is the game playing board (first row is hidden)
 * num_jewels is the number of different jewels to use
 */
struct game_context {
    unsigned int score;
    unsigned int segments;
    unsigned int level;
    unsigned int type;
    unsigned int tmp_type;
    struct tile playboard[BJ_HEIGHT][BJ_WIDTH];
    unsigned int num_jewels;
};

#define MAX_NUM_JEWELS 7

#define MAX_PUZZLE_TILES 4
#define NUM_PUZZLE_LEVELS 10

struct puzzle_tile {
    int x;
    int y;
    int tile_type;
};

struct puzzle_level {
    unsigned int num_jewels;
    unsigned int num_tiles;
    struct puzzle_tile tiles[MAX_PUZZLE_TILES];
};

#define PUZZLE_TILE_UP      1
#define PUZZLE_TILE_DOWN    2
#define PUZZLE_TILE_LEFT    4
#define PUZZLE_TILE_RIGHT   8

struct puzzle_level puzzle_levels[NUM_PUZZLE_LEVELS] = {
    { 5, 2, { {3, 3, PUZZLE_TILE_RIGHT},
              {4, 2, PUZZLE_TILE_LEFT} } },
    { 5, 2, { {3, 2, PUZZLE_TILE_DOWN},
              {3, 4, PUZZLE_TILE_UP} } },
    { 6, 3, { {3, 2, PUZZLE_TILE_DOWN},
              {3, 4, PUZZLE_TILE_UP|PUZZLE_TILE_DOWN},
              {3, 6, PUZZLE_TILE_UP} } },
    { 6, 3, { {3, 2, PUZZLE_TILE_RIGHT},
              {4, 3, PUZZLE_TILE_LEFT|PUZZLE_TILE_RIGHT},
              {5, 4, PUZZLE_TILE_LEFT} } },
    { 6, 2, { {3, 4, PUZZLE_TILE_RIGHT},
              {4, 2, PUZZLE_TILE_LEFT} } },
    { 6, 2, { {3, 2, PUZZLE_TILE_DOWN},
              {4, 4, PUZZLE_TILE_UP} } },
    { 7, 4, { {3, 2, PUZZLE_TILE_RIGHT|PUZZLE_TILE_DOWN},
              {4, 3, PUZZLE_TILE_LEFT|PUZZLE_TILE_DOWN},
              {3, 4, PUZZLE_TILE_RIGHT|PUZZLE_TILE_UP},
              {4, 4, PUZZLE_TILE_LEFT|PUZZLE_TILE_UP} } },
    { 6, 3, { {3, 2, PUZZLE_TILE_DOWN},
              {4, 4, PUZZLE_TILE_UP|PUZZLE_TILE_DOWN},
              {3, 6, PUZZLE_TILE_UP} } },
    { 7, 3, { {2, 2, PUZZLE_TILE_RIGHT},
              {4, 1, PUZZLE_TILE_LEFT|PUZZLE_TILE_RIGHT},
              {5, 4, PUZZLE_TILE_LEFT} } },
    { 7, 4, { {3, 0, PUZZLE_TILE_RIGHT|PUZZLE_TILE_DOWN},
              {5, 0, PUZZLE_TILE_LEFT|PUZZLE_TILE_DOWN},
              {2, 7, PUZZLE_TILE_RIGHT|PUZZLE_TILE_UP},
              {4, 7, PUZZLE_TILE_LEFT|PUZZLE_TILE_UP} } },
};

#define SAVE_FILE PLUGIN_GAMES_DIR "/jewels.save"

#define HIGH_SCORE PLUGIN_GAMES_DIR "/jewels.score"
struct highscore highest[NUM_SCORES];

static bool resume_file = false;

/*****************************************************************************
* jewels_setcolors() set the foreground and background colors.
******************************************************************************/
static inline void jewels_setcolors(void) {
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(49, 26, 26));
    rb->lcd_set_foreground(LCD_RGBPACK(210, 181, 181));
#endif
}

/*****************************************************************************
* jewels_loadgame() loads the saved game and returns load success.
******************************************************************************/
static bool jewels_loadgame(struct game_context* bj)
{
    int fd;
    bool loaded = false;

    /* open game file */
    fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0) return loaded;

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &bj->tmp_type, sizeof(bj->tmp_type)) <= 0) break;
        if(rb->read(fd, &bj->type, sizeof(bj->type)) <= 0) break;
        if(rb->read(fd, &bj->score, sizeof(bj->score)) <= 0) break;
        if(rb->read(fd, &bj->level, sizeof(bj->level)) <= 0) break;
        if(rb->read(fd, &bj->segments, sizeof(bj->segments)) <= 0) break;
        if(rb->read(fd, &bj->num_jewels, sizeof(bj->num_jewels)) <= 0) break;
        if(rb->read(fd, bj->playboard, sizeof(bj->playboard)) <= 0) break;
        loaded = true;
        break;
    }

    rb->close(fd);

    return loaded;
}

/*****************************************************************************
* jewels_savegame() saves the current game state.
******************************************************************************/
static void jewels_savegame(struct game_context* bj)
{
    int fd;
    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    if(fd < 0) return;

    rb->write(fd, &bj->tmp_type, sizeof(bj->tmp_type));
    rb->write(fd, &bj->type, sizeof(bj->type));
    rb->write(fd, &bj->score, sizeof(bj->score));
    rb->write(fd, &bj->level, sizeof(bj->level));
    rb->write(fd, &bj->segments, sizeof(bj->segments));
    rb->write(fd, &bj->num_jewels, sizeof(bj->num_jewels));
    rb->write(fd, bj->playboard, sizeof(bj->playboard));
    rb->close(fd);
}

/*****************************************************************************
* jewels_drawboard() redraws the entire game board.
******************************************************************************/
static void jewels_drawboard(struct game_context* bj) {
    int i, j;
    int w, h;
    unsigned int tempscore;
    unsigned int size;
    char *title = "Level";
    char str[10];

    if (bj->type == GAME_TYPE_NORMAL) {
        tempscore = (bj->score>LEVEL_PTS ? LEVEL_PTS : bj->score);
        size = LEVEL_PTS;
    } else {
        tempscore = (bj->level>NUM_PUZZLE_LEVELS ? NUM_PUZZLE_LEVELS : bj->level);
        size = NUM_PUZZLE_LEVELS;
    }

    /* clear screen */
    rb->lcd_clear_display();

    /* dispay playing board */
    for(i=0; i<BJ_HEIGHT-1; i++){
        for(j=0; j<BJ_WIDTH; j++){
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(jewels_bkgd[(i+j)%2]);
            rb->lcd_fillrect(j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                                TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_transparent_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard[i+1][j].type),
                           STRIDE(  SCREEN_MAIN, 
                                    BMPWIDTH_jewels, BMPHEIGHT_jewels), 
                           j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#else
            rb->lcd_bitmap_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard[i+1][j].type),
                           STRIDE(  SCREEN_MAIN, 
                                    BMPWIDTH_jewels, BMPHEIGHT_jewels),
                           j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#endif
        }
    }

#if LCD_WIDTH > LCD_HEIGHT /* horizontal layout */

    /* draw separator lines */
    jewels_setcolors();
    rb->lcd_vline(BJ_WIDTH*TILE_WIDTH, 0, LCD_HEIGHT-1);

    rb->lcd_hline(BJ_WIDTH*TILE_WIDTH, LCD_WIDTH-1, 18);
    rb->lcd_hline(BJ_WIDTH*TILE_WIDTH, LCD_WIDTH-1, LCD_HEIGHT-10);
    
    /* draw progress bar */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(104, 63, 63));
#endif
    rb->lcd_fillrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4,
                     (LCD_HEIGHT-10)-(((LCD_HEIGHT-10)-18)*
                         tempscore/size),
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     ((LCD_HEIGHT-10)-18)*tempscore/size);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(83, 44, 44));
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4+1,
                     (LCD_HEIGHT-10)-(((LCD_HEIGHT-10)-18)*
                         tempscore/size)+1,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-2,
                     ((LCD_HEIGHT-10)-18)*tempscore/size-1);
    jewels_setcolors();
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4,
                     (LCD_HEIGHT-10)-(((LCD_HEIGHT-10)-18)*tempscore/size),
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     ((LCD_HEIGHT-10)-18)*tempscore/size+1);
#endif
    
    /* print text */
    rb->lcd_getstringsize(title, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-w/2, 1, title);
    rb->snprintf(str, 4, "%d", bj->level);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-w/2, 10, str);

    if (bj->type == GAME_TYPE_NORMAL) {
        rb->snprintf(str, 6, "%d", (bj->level-1)*LEVEL_PTS+bj->score);
        rb->lcd_getstringsize(str, &w, &h);
        rb->lcd_putsxy(LCD_WIDTH-(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-w/2,
                       LCD_HEIGHT-8, str);
    }

#elif LCD_WIDTH < LCD_HEIGHT /* vertical layout */

    /* draw separator lines */
    jewels_setcolors();
    rb->lcd_hline(0, LCD_WIDTH-1, 8*TILE_HEIGHT+YOFS);
    rb->lcd_hline(0, LCD_WIDTH-1, LCD_HEIGHT-14);
    rb->lcd_vline(LCD_WIDTH/2, LCD_HEIGHT-14, LCD_HEIGHT-1);
    
    /* draw progress bar */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(104, 63, 63));
#endif
    rb->lcd_fillrect(0, (8*TILE_HEIGHT+YOFS)
                    +(LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/4,
                     LCD_WIDTH*tempscore/size,
                     (LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/2);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(83, 44, 44));
    rb->lcd_drawrect(1, (8*TILE_HEIGHT+YOFS)
                    +(LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/4+1,
                     LCD_WIDTH*tempscore/size-1,
                     (LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/2-2);
    jewels_setcolors();
    rb->lcd_drawrect(0, (8*TILE_HEIGHT+YOFS)
                    +(LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/4,
                     LCD_WIDTH*tempscore/size+1,
                     (LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/2);
#endif
    
    /* print text */
    rb->snprintf(str, 10, "%s %d", title, bj->level);
    rb->lcd_putsxy(1, LCD_HEIGHT-10, str);
    
    if (bj->type == GAME_TYPE_NORMAL) {
        rb->snprintf(str, 6, "%d", (bj->level-1)*LEVEL_PTS+bj->score);
        rb->lcd_getstringsize(str, &w, &h);
        rb->lcd_putsxy((LCD_WIDTH-2)-w, LCD_HEIGHT-10, str);
    }


#else /* square layout */

    /* draw separator lines */
    jewels_setcolors();
    rb->lcd_hline(0, LCD_WIDTH-1, 8*TILE_HEIGHT+YOFS);
    rb->lcd_vline(BJ_WIDTH*TILE_WIDTH, 0, 8*TILE_HEIGHT+YOFS);
    rb->lcd_vline(LCD_WIDTH/2, 8*TILE_HEIGHT+YOFS, LCD_HEIGHT-1);

    /* draw progress bar */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(104, 63, 63));
#endif
    rb->lcd_fillrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4,
                     (8*TILE_HEIGHT+YOFS)-(8*TILE_HEIGHT+YOFS)*tempscore/size,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     (8*TILE_HEIGHT+YOFS)*tempscore/size);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(83, 44, 44));
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4+1,
                     (8*TILE_HEIGHT+YOFS)-(8*TILE_HEIGHT+YOFS)
                        *tempscore/size+1,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-2,
                     (8*TILE_HEIGHT+YOFS)*tempscore/size-1);
    jewels_setcolors();
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4,
                     (8*TILE_HEIGHT+YOFS)-(8*TILE_HEIGHT+YOFS)
                        *tempscore/size,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     (8*TILE_HEIGHT+YOFS)*tempscore/size+1);
#endif

    /* print text */
    rb->snprintf(str, 10, "%s %d", title, bj->level);
    rb->lcd_putsxy(1, LCD_HEIGHT-(LCD_HEIGHT-(8*TILE_HEIGHT+YOFS))/2-3, str);
    
    if (bj->type == GAME_TYPE_NORMAL) {
        rb->snprintf(str, 6, "%d", (bj->level-1)*LEVEL_PTS+bj->score);
        rb->lcd_getstringsize(str, &w, &h);
        rb->lcd_putsxy((LCD_WIDTH-2)-w,
                       LCD_HEIGHT-(LCD_HEIGHT-(8*TILE_HEIGHT+YOFS))/2-3, str);
    }

#endif /* layout */

    rb->lcd_update();
}

/*****************************************************************************
* jewels_putjewels() makes the jewels fall to fill empty spots and adds
* new random jewels at the empty spots at the top of each row.
******************************************************************************/
static void jewels_putjewels(struct game_context* bj){
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
                bj->playboard[0][j].type = rb->rand()%bj->num_jewels+1;
            }
            for(i=BJ_HEIGHT-2; i>=0; i--) {
                if(!mark && bj->playboard[i+1][j].type == 0) {
                    mark = true;
                    done = false;
                }
                if(mark) bj->playboard[i][j].falling = true;
            }
            /*if(bj->playboard[1][j].falling) {
                bj->playboard[0][j].type = rb->rand()%bj->num_jewels+1;
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
                            rb->lcd_set_foreground(jewels_bkgd[(i-1+j)%2]);
                        }
                        rb->lcd_fillrect(j*TILE_WIDTH, (i-1)*TILE_HEIGHT+YOFS,
                                            TILE_WIDTH, TILE_HEIGHT);
                        if(bj->playboard[i+1][j].type == 0) {
                            rb->lcd_set_foreground(jewels_bkgd[(i+j)%2]);
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
                        rb->lcd_bitmap_transparent_part(jewels, 0,
                                       TILE_HEIGHT*(bj->playboard[i][j].type),
                                       STRIDE(  SCREEN_MAIN, 
                                                BMPWIDTH_jewels, 
                                                BMPHEIGHT_jewels),
                                       j*TILE_WIDTH,
                                       (i-1)*TILE_HEIGHT+YOFS+
                                           ((((TILE_HEIGHT<<10)*k)/8)>>10),
                                       TILE_WIDTH, TILE_HEIGHT);
#else
                        rb->lcd_bitmap_part(jewels, 0,
                                       TILE_HEIGHT*(bj->playboard[i][j].type),
                                       STRIDE(  SCREEN_MAIN, 
                                                BMPWIDTH_jewels, 
                                                BMPHEIGHT_jewels),
                                       j*TILE_WIDTH,
                                       (i-1)*TILE_HEIGHT+YOFS+
                                           ((((TILE_HEIGHT<<10)*k)/8)>>10),
                                       TILE_WIDTH, TILE_HEIGHT);
#endif
                    }
                }
            }

            rb->lcd_update_rect(0, 0, TILE_WIDTH*8, LCD_HEIGHT);
            jewels_setcolors();

            /* framerate limiting */
            currenttick = *rb->current_tick;
            if(currenttick-lasttick < HZ/MAX_FPS) {
                rb->sleep((HZ/MAX_FPS)-(currenttick-lasttick));
            } else {
                rb->yield();
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
* jewels_clearjewels() finds all the connected rows and columns and
*     calculates and returns the points earned.
******************************************************************************/
static unsigned int jewels_clearjewels(struct game_context* bj) {
    int i, j;
    int last, run;
    unsigned int points = 0;

    /* check for connected rows */
    for(i=1; i<BJ_HEIGHT; i++) {
        last = 0;
        run = 1;
        for(j=0; j<BJ_WIDTH; j++) {
            if(bj->playboard[i][j].type == last &&
               bj->playboard[i][j].type != 0 &&
               bj->playboard[i][j].type <= MAX_NUM_JEWELS) {
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
               bj->playboard[i][j].type == last &&
               bj->playboard[i][j].type <= MAX_NUM_JEWELS) {
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
* jewels_runboard() runs the board until it settles in a fixed state and
*     returns points earned.
******************************************************************************/
static unsigned int jewels_runboard(struct game_context* bj) {
    unsigned int points = 0;
    unsigned int ret;

    bj->segments = 0;

    while((ret = jewels_clearjewels(bj)) > 0) {
        points += ret;
        jewels_drawboard(bj);
        jewels_putjewels(bj);
    }

    return points;
}

/*****************************************************************************
* jewels_swapjewels() swaps two jewels as long as it results in points and
*     returns points earned.
******************************************************************************/
static unsigned int jewels_swapjewels(struct game_context* bj,
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
            rb->lcd_set_foreground(jewels_bkgd[(x+y)%2]);
            rb->lcd_fillrect(x*TILE_WIDTH,
                             y*TILE_HEIGHT+YOFS,
                             TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_foreground(jewels_bkgd[(x+horzmod+y+vertmod)%2]);
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
            rb->lcd_bitmap_transparent_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard
                               [y+1+vertmod][x+horzmod].type),
                           STRIDE(  SCREEN_MAIN, 
                                    BMPWIDTH_jewels, BMPHEIGHT_jewels),
                           (x+horzmod)*TILE_WIDTH-horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           (y+vertmod)*TILE_HEIGHT-vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_transparent_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard[y+1][x].type),
                           STRIDE(  SCREEN_MAIN, 
                                    BMPWIDTH_jewels, BMPHEIGHT_jewels),
                           x*TILE_WIDTH+horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           y*TILE_HEIGHT+vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#else
            rb->lcd_bitmap_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard
                               [y+1+vertmod][x+horzmod].type),
                           STRIDE(  SCREEN_MAIN, 
                                    BMPWIDTH_jewels, BMPHEIGHT_jewels),
                           (x+horzmod)*TILE_WIDTH-horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           (y+vertmod)*TILE_HEIGHT-vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_FG);
            rb->lcd_bitmap_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard[y+1][x].type),
                           STRIDE(  SCREEN_MAIN, 
                                    BMPWIDTH_jewels, BMPHEIGHT_jewels),
                           x*TILE_WIDTH+horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           y*TILE_HEIGHT+vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
#endif

            rb->lcd_update_rect(0, 0, TILE_WIDTH*8, LCD_HEIGHT);
            jewels_setcolors();

            /* framerate limiting */
            currenttick = *rb->current_tick;
            if(currenttick-lasttick < HZ/MAX_FPS) {
                rb->sleep((HZ/MAX_FPS)-(currenttick-lasttick));
            } else {
                rb->yield();
            }
            lasttick = currenttick;
        }

        /* swap jewels */
        int temp = bj->playboard[y+1][x].type;
        bj->playboard[y+1][x].type =
            bj->playboard[y+1+vertmod][x+horzmod].type;
        bj->playboard[y+1+vertmod][x+horzmod].type = temp;

        if(undo) break;

        points = jewels_runboard(bj);
        if(points == 0) {
            undo = true;
        } else {
            break;
        }
    }

    return points;
}

/*****************************************************************************
* jewels_movesavail() uses pattern matching to see if there are any
*     available move left.
******************************************************************************/
static bool jewels_movesavail(struct game_context* bj) {
    int i, j;
    bool moves = false;
    int mytype;

    for(i=1; i<BJ_HEIGHT; i++) {
        for(j=0; j<BJ_WIDTH; j++) {
            mytype = bj->playboard[i][j].type;
            if(mytype == 0 || mytype > MAX_NUM_JEWELS) continue;

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
* jewels_puzzle_is_finished() checks if the puzzle is finished.
******************************************************************************/
static bool jewels_puzzle_is_finished(struct game_context* bj) {
    unsigned int i, j;
    for(i=0; i<BJ_HEIGHT; i++) {
        for(j=0; j<BJ_WIDTH; j++) {
            int mytype = bj->playboard[i][j].type;
            if(mytype>MAX_NUM_JEWELS) {
                mytype -= MAX_NUM_JEWELS;
                if(mytype&PUZZLE_TILE_UP) {
                    if(i==0 || bj->playboard[i-1][j].type<=MAX_NUM_JEWELS ||
                       !((bj->playboard[i-1][j].type-MAX_NUM_JEWELS)
                         &PUZZLE_TILE_DOWN))
                        return false;
                }
                if(mytype&PUZZLE_TILE_DOWN) {
                    if(i==BJ_HEIGHT-1 ||
                       bj->playboard[i+1][j].type<=MAX_NUM_JEWELS ||
                       !((bj->playboard[i+1][j].type-MAX_NUM_JEWELS)
                         &PUZZLE_TILE_UP))
                        return false;
                }
                if(mytype&PUZZLE_TILE_LEFT) {
                    if(j==0 || bj->playboard[i][j-1].type<=MAX_NUM_JEWELS ||
                       !((bj->playboard[i][j-1].type-MAX_NUM_JEWELS)
                         &PUZZLE_TILE_RIGHT))
                        return false;
                }
                if(mytype&PUZZLE_TILE_RIGHT) {
                    if(j==BJ_WIDTH-1 ||
                       bj->playboard[i][j+1].type<=MAX_NUM_JEWELS ||
                       !((bj->playboard[i][j+1].type-MAX_NUM_JEWELS)
                         &PUZZLE_TILE_LEFT))
                        return false;
                }
            }
        }
    }
    return true;
}

/*****************************************************************************
* jewels_initlevel() initialises a level.
******************************************************************************/
static unsigned int jewels_initlevel(struct game_context* bj) {
    unsigned int points = 0;

    switch(bj->type) {
        case GAME_TYPE_NORMAL:
            bj->num_jewels = MAX_NUM_JEWELS;
            break;

        case GAME_TYPE_PUZZLE:
        {
            unsigned int i, j;
            struct puzzle_tile *tile;

            bj->num_jewels = puzzle_levels[bj->level-1].num_jewels;

            for(i=0; i<BJ_HEIGHT; i++) {
                for(j=0; j<BJ_WIDTH; j++) {
                    bj->playboard[i][j].type = (rb->rand()%bj->num_jewels)+1;
                    bj->playboard[i][j].falling = false;
                    bj->playboard[i][j].delete = false;
                }
            }
            jewels_runboard(bj);
            tile = puzzle_levels[bj->level-1].tiles;
            for(i=0; i<puzzle_levels[bj->level-1].num_tiles; i++, tile++) {
                bj->playboard[tile->y+1][tile->x].type = MAX_NUM_JEWELS
                                                         +tile->tile_type;
            }
        }
        break;
    }

    jewels_drawboard(bj);

    /* run the play board */
    jewels_putjewels(bj);
    points += jewels_runboard(bj);
    return points;
}

/*****************************************************************************
* jewels_init() initializes jewels data structures.
******************************************************************************/
static void jewels_init(struct game_context* bj) {
    /* seed the rand generator */
    rb->srand(*rb->current_tick);

    bj->type = bj->tmp_type;
    bj->level = 1;
    bj->score = 0;
    bj->segments = 0;

    jewels_setcolors();

    /* clear playing board */
    rb->memset(bj->playboard, 0, sizeof(bj->playboard));
    do {
        bj->score += jewels_initlevel(bj);
    } while(!jewels_movesavail(bj));
}

/*****************************************************************************
* jewels_nextlevel() advances the game to the next bj->level and returns
*     points earned.
******************************************************************************/
static void jewels_nextlevel(struct game_context* bj) {
    int i, x, y;
    unsigned int points = 0;

    switch(bj->type) {
        case GAME_TYPE_NORMAL:
            /* roll over score, change and display level */
            while(bj->score >= LEVEL_PTS) {
                bj->score -= LEVEL_PTS;
                bj->level++;
                rb->splashf(HZ*2, "Level %d", bj->level);
                jewels_drawboard(bj);
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
            break;

        case GAME_TYPE_PUZZLE:
            bj->level++;
            rb->splashf(HZ*2, "Level %d", bj->level);
            break;
    }

    points += jewels_initlevel(bj);
    bj->score += points;
}

static bool jewels_help(void)
{
    rb->lcd_setfont(FONT_UI);
#define WORDS (sizeof help_text / sizeof (char*))
    static char *help_text[] = {
        "Jewels", "", "Aim", "",
        "Swap", "pairs", "of", "jewels", "to", "form", "connected", 
        "segments", "of", "three", "or", "more", "of", "the", "same",
        "type.", "",
        "The", "goal", "of", "the", "game", "is", "to", "score", "as", "many",
        "points", "as", "possible", "before", "running", "out", "of",
        "available", "moves.", "", "",
        "Controls", "",
        "Directions",
#ifdef JEWELS_SCROLLWHEEL
            "or", "scroll",
#endif
            "to", "move", "",
        HK_SELECT, "to", "select", "",
        HK_CANCEL, "to", "go", "to", "menu"
    };
    static struct style_text formation[]={
        { 0, TEXT_CENTER|TEXT_UNDERLINE },
        { 2, C_RED },
        { 42, C_RED },
        { -1, 0 }
    };
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    int button;
    if (display_text(WORDS, help_text, formation, NULL))
        return true;
    do {
        button = rb->button_get(true);
        if (rb->default_event_handler (button) == SYS_USB_CONNECTED) {
            return true;
        }
    } while( ( button == BUTTON_NONE )
          || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
    rb->lcd_setfont(FONT_SYSFIXED);

    return false;
}

static bool _ingame;
static int jewels_menu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = ((intptr_t)this_item);
    if(action == ACTION_REQUEST_MENUITEM
       && !_ingame && (i==0 || i==6))
        return ACTION_EXIT_MENUITEM;
    return action;
}
/*****************************************************************************
* jewels_game_menu() shows the game menu.
******************************************************************************/
static int jewels_game_menu(struct game_context* bj, bool ingame)
{
    rb->button_clear_queue();
    int choice = 0;

    _ingame = ingame;

    static struct opt_items mode[] = {
        { "Normal", -1 },
        { "Puzzle", -1 },
    };

    MENUITEM_STRINGLIST (main_menu, "Jewels Menu", jewels_menu_cb,
                             "Resume Game",
                             "Start New Game",
                             "Mode",
                             "Help",
                             "High Scores",
                             "Playback Control",
                             "Quit without Saving",
                             "Quit");

    while (1) {
        switch (rb->do_menu(&main_menu, &choice, NULL, false)) {
            case 0:
                jewels_setcolors();
                if(resume_file)
                    rb->remove(SAVE_FILE);
                return 0;
            case 1:
                jewels_init(bj);
                return 0;
            case 2:
                rb->set_option("Mode", &bj->tmp_type, INT, mode, 2, NULL);
                break;
            case 3:
                if(jewels_help())
                    return 1;
                break;
            case 4:
                highscore_show(NUM_SCORES, highest, NUM_SCORES, true);
                break;
            case 5:
                playback_control(NULL);
                break;
            case 6:
                return 1;
            case 7:
                if (ingame) {
                    rb->splash(HZ*1, "Saving game ...");
                    jewels_savegame(bj);
                }
                return 1;
            case MENU_ATTACHED_USB:
                return 1;
            default:
                break;
        }
    }
}

static int jewels_main(struct game_context* bj) {
    int button;
    int position;
    bool selected = false;
    bool no_movesavail;
    int x=0, y=0;

    bool loaded = jewels_loadgame(bj);
    resume_file = loaded;
    if (jewels_game_menu(bj, loaded)!=0)
        return 0;

    resume_file = false;
    while(true) {
        no_movesavail = false;

        /* refresh the board */
        jewels_drawboard(bj);

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
        rb->yield();
        button = rb->button_get(true);
        switch(button){
            case JEWELS_LEFT:             /* move cursor left */
            case (JEWELS_LEFT|BUTTON_REPEAT):
                if(selected) {
                    bj->score += jewels_swapjewels(bj, x, y, SWAP_LEFT);
                    selected = false;
                    if (!jewels_movesavail(bj)) no_movesavail = true;
                } else {
                    x = (x+BJ_WIDTH-1)%BJ_WIDTH;
                }
                break;

            case JEWELS_RIGHT:            /* move cursor right */
            case (JEWELS_RIGHT|BUTTON_REPEAT):
                if(selected) {
                    bj->score += jewels_swapjewels(bj, x, y, SWAP_RIGHT);
                    selected = false;
                    if (!jewels_movesavail(bj)) no_movesavail = true;
                } else {
                    x = (x+1)%BJ_WIDTH;
                }
                break;

            case JEWELS_DOWN:             /* move cursor down */
            case (JEWELS_DOWN|BUTTON_REPEAT):
                if(selected) {
                    bj->score += jewels_swapjewels(bj, x, y, SWAP_DOWN);
                    selected = false;
                    if (!jewels_movesavail(bj)) no_movesavail = true;
                } else {
                    y = (y+1)%(BJ_HEIGHT-1);
                }
                break;

            case JEWELS_UP:               /* move cursor up */
            case (JEWELS_UP|BUTTON_REPEAT):
                if(selected) {
                    bj->score += jewels_swapjewels(bj, x, y, SWAP_UP);
                    selected = false;
                    if (!jewels_movesavail(bj)) no_movesavail = true;
                } else {
                    y = (y+(BJ_HEIGHT-1)-1)%(BJ_HEIGHT-1);
                }
                break;

#ifdef JEWELS_SCROLLWHEEL
            case JEWELS_PREV:             /* scroll backwards */
            case (JEWELS_PREV|BUTTON_REPEAT):
                if(!selected) {
                    if(x == 0) {
                        y = (y+(BJ_HEIGHT-1)-1)%(BJ_HEIGHT-1);
                    }
                    x = (x+BJ_WIDTH-1)%BJ_WIDTH;
                }
                break;

            case JEWELS_NEXT:             /* scroll forwards */
            case (JEWELS_NEXT|BUTTON_REPEAT):
                if(!selected) {
                    if(x == BJ_WIDTH-1) {
                        y = (y+1)%(BJ_HEIGHT-1);
                    }
                    x = (x+1)%BJ_WIDTH;
                }
                break;
#endif

            case JEWELS_SELECT:           /* toggle selected */
                selected = !selected;
                break;

#ifdef JEWELS_RC_CANCEL
            case JEWELS_RC_CANCEL:
#endif
            case JEWELS_CANCEL:           /* end game */
                if (jewels_game_menu(bj, true)!=0)
                    return 0;
                break;

            default:
                if (rb->default_event_handler (button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }

        switch(bj->type) {
            case GAME_TYPE_NORMAL:
                if(bj->score >= LEVEL_PTS)
                    jewels_nextlevel(bj);
                break;
            case GAME_TYPE_PUZZLE:
                if (jewels_puzzle_is_finished(bj)) {
                    if (bj->level < NUM_PUZZLE_LEVELS) {
                        jewels_nextlevel(bj);
                    } else {
                        rb->splash(2*HZ, "Congratulations!");
                        rb->splash(2*HZ, "You have finished the game!");
                        if (jewels_game_menu(bj, false)!=0) {
                            return 0;
                        }
                    }
                break;
                }
        }

        if (no_movesavail) {
            switch(bj->type) {
                case GAME_TYPE_NORMAL:
                    rb->splash(HZ*2, "Game Over!");
                    rb->lcd_clear_display();
                    bj->score += (bj->level-1)*LEVEL_PTS;
                    position=highscore_update(bj->score, bj->level, "",
                                              highest, NUM_SCORES);
                    if (position == 0)
                        rb->splash(HZ*2, "New High Score");
                    if (position != -1)
                    highscore_show(position, highest, NUM_SCORES, true);
                    break;
                case GAME_TYPE_PUZZLE:
                    rb->splash(2*HZ, "Game Over");
                    break;
            }
            if (jewels_game_menu(bj, false)!=0) {
                return 0;
            }
        }
    }
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    /* load high scores */
    highscore_load(HIGH_SCORE,highest,NUM_SCORES);

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    struct game_context bj;
    bj.tmp_type = GAME_TYPE_NORMAL;
    jewels_main(&bj);
    highscore_save(HIGH_SCORE,highest,NUM_SCORES);
    rb->lcd_setfont(FONT_UI);

    return PLUGIN_OK;
}

#endif /* HAVE_LCD_BITMAP */
