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
#include "playback_control.h"

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

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_MENU
#define JEWELS_CANCEL BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_OFF
#define JEWELS_RC_CANCEL BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define JEWELS_SCROLLWHEEL
#define JEWELS_UP     BUTTON_MENU
#define JEWELS_DOWN   BUTTON_PLAY
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_PREV   BUTTON_SCROLL_BACK
#define JEWELS_NEXT   BUTTON_SCROLL_FWD
#define JEWELS_SELECT BUTTON_SELECT

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define JEWELS_UP     BUTTON_SCROLL_UP
#define JEWELS_DOWN   BUTTON_SCROLL_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_PLAY
#define JEWELS_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_BACK

#elif CONFIG_KEYPAD == MROBE100_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define JEWELS_UP     BUTTON_RC_VOL_UP
#define JEWELS_DOWN   BUTTON_RC_VOL_DOWN
#define JEWELS_LEFT   BUTTON_RC_REW
#define JEWELS_RIGHT  BUTTON_RC_FF
#define JEWELS_SELECT BUTTON_RC_PLAY
#define JEWELS_CANCEL BUTTON_RC_REC

#define JEWELS_RC_CANCEL BUTTON_REC

#elif CONFIG_KEYPAD == COWOND2_PAD
#define JEWELS_UP     BUTTON_UP
#define JEWELS_DOWN   BUTTON_DOWN
#define JEWELS_LEFT   BUTTON_LEFT
#define JEWELS_RIGHT  BUTTON_RIGHT
#define JEWELS_SELECT BUTTON_SELECT
#define JEWELS_CANCEL BUTTON_POWER

#else
#error No keymap defined!
#endif

/* use 30x30 tiles (iPod Video, Gigabeat) */
#if (LCD_HEIGHT == 240) && (LCD_WIDTH == 320) || \
      ((LCD_HEIGHT == 320) && (LCD_WIDTH == 240))
#define TILE_WIDTH  30
#define TILE_HEIGHT 30
#define YOFS 0
#define NUM_SCORES 10

/* use 22x22 tiles (H300, iPod Color) */
#elif ((LCD_HEIGHT == 176) && (LCD_WIDTH == 220)) || \
      ((LCD_HEIGHT == 220) && (LCD_WIDTH == 176))
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

/* use 14x14 tiles (H10 5/6 GB) */
#elif (LCD_HEIGHT == 128) && (LCD_WIDTH == 128)
#define TILE_WIDTH  14
#define TILE_HEIGHT 14
#define YOFS 0
#define NUM_SCORES 10

/* use 13x13 tiles (iPod Mini) */
#elif (LCD_HEIGHT == 110) && (LCD_WIDTH == 138)
#define TILE_WIDTH  13
#define TILE_HEIGHT 13
#define YOFS 6
#define NUM_SCORES 10

/* use 12x12 tiles (iAudio M3) */
#elif (LCD_HEIGHT == 96) && (LCD_WIDTH == 128)
#define TILE_WIDTH  12
#define TILE_HEIGHT 12
#define YOFS 0
#define NUM_SCORES 9

/* use 10x10 tiles (Sansa c200) */
#elif (LCD_HEIGHT == 80) && (LCD_WIDTH == 132)
#define TILE_WIDTH 10
#define TILE_HEIGHT 10
#define YOFS 0
#define NUM_SCORES 8

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
  #error JEWELS: Unsupported LCD
#endif

/* save files */
#define SCORE_FILE PLUGIN_GAMES_DIR "/jewels.score"
#define SAVE_FILE  PLUGIN_GAMES_DIR "/jewels.save"

/* final game return status */
#define BJ_QUIT_FROM_GAME 4
#define BJ_END            3
#define BJ_USB            2
#define BJ_QUIT           1
#define BJ_LOSE           0

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

/* Game types */
enum game_type {
    GAME_TYPE_NORMAL,
    GAME_TYPE_PUZZLE
};

/* menu values */
#define FONT_HEIGHT 8
#define MAX_MITEMS  6
#define MENU_WIDTH  100

/* menu results */
enum menu_result {
    MRES_NONE,
    MRES_NEW,
    MRES_PUZZLE,
    MRES_SAVE,
    MRES_RESUME,
    MRES_SCORES,
    MRES_HELP,
    MRES_QUIT,
    MRES_PLAYBACK,
    MRES_EXIT
};

/* menu commands */
enum menu_cmd {
    MCMD_NONE,
    MCMD_NEXT,
    MCMD_PREV,
    MCMD_SELECT
};

/* menus */
struct jewels_menu {
    char *title;
    bool hasframe;
    int selected;
    int itemcnt;
    struct jewels_menuitem {
        char *text;
        enum menu_result res;
    } items[MAX_MITEMS];
} bjmenu[] = {
    {"Jewels", false, 0, 6,
        {{"New Game",    MRES_NEW},
         {"Puzzle",      MRES_PUZZLE},
         {"Resume Game", MRES_RESUME},
         {"High Scores", MRES_SCORES},
         {"Help",        MRES_HELP},
         {"Quit",        MRES_QUIT}}},
    {"Menu", true, 0, 5,
        {{"Audio Playback", MRES_PLAYBACK },
         {"Resume Game", MRES_RESUME},
         {"Save Game",   MRES_SAVE},
         {"End Game",    MRES_QUIT},
         {"Exit Jewels", MRES_EXIT}}}
};

/* global rockbox api */
static struct plugin_api* rb;

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
 * type is the game type (normal or puzzle)
 * highscores is the list of high scores
 * resume denotes whether to resume the currently loaded game
 * dirty denotes whether the high scores are out of sync with the saved file
 * playboard is the game playing board (first row is hidden)
 * num_jewels is the number of different jewels to use
 */
struct game_context {
    unsigned int score;
    unsigned int segments;
    unsigned int level;
    unsigned int type;
    unsigned int highscores[NUM_SCORES];
    bool resume;
    bool dirty;
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

/*****************************************************************************
* jewels_init() initializes jewels data structures.
******************************************************************************/
static void jewels_init(struct game_context* bj) {
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
* jewels_setcolors() set the foreground and background colors.
******************************************************************************/
static inline void jewels_setcolors(void) {
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(49, 26, 26));
    rb->lcd_set_foreground(LCD_RGBPACK(210, 181, 181));
#endif
}

/*****************************************************************************
* jewels_drawboard() redraws the entire game board.
******************************************************************************/
static void jewels_drawboard(struct game_context* bj) {
    int i, j;
    int w, h;
    unsigned int tempscore;
    char *title = "Level";
    char str[10];

    tempscore = (bj->score>LEVEL_PTS ? LEVEL_PTS : bj->score);

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
                           TILE_WIDTH, j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#else
            rb->lcd_bitmap_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard[i+1][j].type),
                           TILE_WIDTH, j*TILE_WIDTH, i*TILE_HEIGHT+YOFS,
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
    jewels_setcolors();
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
                   LCD_HEIGHT-8, str);

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
                     LCD_WIDTH*tempscore/LEVEL_PTS,
                     (LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/2);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(83, 44, 44));
    rb->lcd_drawrect(1, (8*TILE_HEIGHT+YOFS)
                           +(LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/4+1,
                     LCD_WIDTH*tempscore/LEVEL_PTS-1,
                     (LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/2-2);
    jewels_setcolors();
    rb->lcd_drawrect(0, (8*TILE_HEIGHT+YOFS)
                           +(LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/4,
                     LCD_WIDTH*tempscore/LEVEL_PTS+1,
                     (LCD_HEIGHT-14-(8*TILE_HEIGHT+YOFS))/2);
#endif

    /* print text */
    rb->snprintf(str, 10, "%s %d", title, bj->level);
    rb->lcd_putsxy(1, LCD_HEIGHT-10, str);

    rb->snprintf(str, 6, "%d", (bj->level-1)*LEVEL_PTS+bj->score);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy((LCD_WIDTH-2)-w, LCD_HEIGHT-10, str);

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
                     (8*TILE_HEIGHT+YOFS)-(8*TILE_HEIGHT+YOFS)
                        *tempscore/LEVEL_PTS,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     (8*TILE_HEIGHT+YOFS)*tempscore/LEVEL_PTS);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(83, 44, 44));
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4+1,
                     (8*TILE_HEIGHT+YOFS)-(8*TILE_HEIGHT+YOFS)
                        *tempscore/LEVEL_PTS+1,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2-2,
                     (8*TILE_HEIGHT+YOFS)*tempscore/LEVEL_PTS-1);
    jewels_setcolors();
    rb->lcd_drawrect(BJ_WIDTH*TILE_WIDTH+(LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/4,
                     (8*TILE_HEIGHT+YOFS)-(8*TILE_HEIGHT+YOFS)
                        *tempscore/LEVEL_PTS,
                     (LCD_WIDTH-BJ_WIDTH*TILE_WIDTH)/2,
                     (8*TILE_HEIGHT+YOFS)*tempscore/LEVEL_PTS+1);
#endif

    /* print text */
    rb->snprintf(str, 10, "%s %d", title, bj->level);
    rb->lcd_putsxy(1, LCD_HEIGHT-(LCD_HEIGHT-(8*TILE_HEIGHT+YOFS))/2-3, str);

    rb->snprintf(str, 6, "%d", (bj->level-1)*LEVEL_PTS+bj->score);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy((LCD_WIDTH-2)-w,
                   LCD_HEIGHT-(LCD_HEIGHT-(8*TILE_HEIGHT+YOFS))/2-3, str);

#endif /* layout */

    rb->lcd_update();
}

/*****************************************************************************
* jewels_showmenu() displays the chosen menu after performing the chosen
*     menu command.
******************************************************************************/
static enum menu_result jewels_showmenu(struct jewels_menu* menu,
                                           enum menu_cmd cmd) {
    int i;
    int w, h;
    int firstline;
    int adj;

    /* handle menu command */
    switch(cmd) {
        case MCMD_NEXT:
            menu->selected = (menu->selected+1)%menu->itemcnt;
            break;

        case MCMD_PREV:
            menu->selected = (menu->selected-1+menu->itemcnt)%menu->itemcnt;
            break;

        case MCMD_SELECT:
            return menu->items[menu->selected].res;

        default:
            break;
    }

    /* clear menu area */
    firstline = (LCD_HEIGHT/FONT_HEIGHT-(menu->itemcnt+3))/2;

    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect((LCD_WIDTH-MENU_WIDTH)/2, firstline*FONT_HEIGHT,
                     MENU_WIDTH, (menu->itemcnt+3)*FONT_HEIGHT);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    if(menu->hasframe) {
        rb->lcd_drawrect((LCD_WIDTH-MENU_WIDTH)/2-1, firstline*FONT_HEIGHT-1,
                         MENU_WIDTH+2, (menu->itemcnt+3)*FONT_HEIGHT+2);
        rb->lcd_hline((LCD_WIDTH-MENU_WIDTH)/2-1,
                      (LCD_WIDTH-MENU_WIDTH)/2-1+MENU_WIDTH+2,
                      (firstline+1)*FONT_HEIGHT);
    }

    /* draw menu items */
    rb->lcd_getstringsize(menu->title, &w, &h);
    rb->lcd_putsxy((LCD_WIDTH-w)/2, firstline*FONT_HEIGHT, menu->title);

    for(i=0; i<menu->itemcnt; i++) {
        if(i == menu->selected) {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        }
        rb->lcd_putsxy((LCD_WIDTH-MENU_WIDTH)/2, (firstline+i+2)*FONT_HEIGHT,
                       menu->items[i].text);
        if(i == menu->selected) {
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
    }

    adj = (firstline == 0 ? 0 : 1);
    rb->lcd_update_rect((LCD_WIDTH-MENU_WIDTH)/2-1, firstline*FONT_HEIGHT-adj,
                        MENU_WIDTH+2, (menu->itemcnt+3)*FONT_HEIGHT+2*adj);
    return MRES_NONE;
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
                                       TILE_WIDTH, j*TILE_WIDTH,
                                       (i-1)*TILE_HEIGHT+YOFS+
                                           ((((TILE_HEIGHT<<10)*k)/8)>>10),
                                       TILE_WIDTH, TILE_HEIGHT);
#else
                        rb->lcd_bitmap_part(jewels, 0,
                                       TILE_HEIGHT*(bj->playboard[i][j].type),
                                       TILE_WIDTH, j*TILE_WIDTH,
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
                               [y+1+vertmod][x+horzmod].type), TILE_WIDTH,
                           (x+horzmod)*TILE_WIDTH-horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           (y+vertmod)*TILE_HEIGHT-vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_transparent_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard[y+1][x].type),
                           TILE_WIDTH, x*TILE_WIDTH+horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           y*TILE_HEIGHT+vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
#else
            rb->lcd_bitmap_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard
                               [y+1+vertmod][x+horzmod].type), TILE_WIDTH,
                           (x+horzmod)*TILE_WIDTH-horzmod*
                               ((((movelen<<10)*k)/8)>>10),
                           (y+vertmod)*TILE_HEIGHT-vertmod*
                               ((((movelen<<10)*k)/8)>>10)+YOFS,
                           TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_FG);
            rb->lcd_bitmap_part(jewels,
                           0, TILE_HEIGHT*(bj->playboard[y+1][x].type),
                           TILE_WIDTH, x*TILE_WIDTH+horzmod*
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
* jewels_puzzle_is_finished(bj) checks if the puzzle is finished.
******************************************************************************/
static int jewels_puzzle_is_finished(struct game_context* bj) {
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
                        return 0;
                }
                if(mytype&PUZZLE_TILE_DOWN) {
                    if(i==BJ_HEIGHT-1 ||
                       bj->playboard[i+1][j].type<=MAX_NUM_JEWELS ||
                       !((bj->playboard[i+1][j].type-MAX_NUM_JEWELS)
                         &PUZZLE_TILE_UP))
                        return 0;
                }
                if(mytype&PUZZLE_TILE_LEFT) {
                    if(j==0 || bj->playboard[i][j-1].type<=MAX_NUM_JEWELS ||
                       !((bj->playboard[i][j-1].type-MAX_NUM_JEWELS)
                         &PUZZLE_TILE_RIGHT))
                        return 0;
                }
                if(mytype&PUZZLE_TILE_RIGHT) {
                    if(j==BJ_WIDTH-1 ||
                       bj->playboard[i][j+1].type<=MAX_NUM_JEWELS ||
                       !((bj->playboard[i][j+1].type-MAX_NUM_JEWELS)
                         &PUZZLE_TILE_LEFT))
                        return 0;
                }
            }
        }
    }
    return 1;
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
* jewels_nextlevel() advances the game to the next level and returns
*     points earned.
******************************************************************************/
static unsigned int jewels_nextlevel(struct game_context* bj) {
    int i, x, y;
    unsigned int points = 0;

    switch(bj->type) {
        case GAME_TYPE_NORMAL:
            /* roll over score, change and display level */
            while(bj->score >= LEVEL_PTS) {
                bj->score -= LEVEL_PTS;
                bj->level++;
                rb->splash(HZ*2, "Level %d", bj->level);
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
            if(bj->level>NUM_PUZZLE_LEVELS) {
                rb->splash(HZ*2, "You win!");
                bj->level = 1;
            } else {
                rb->splash(HZ*2, "Level %d", bj->level);
            }
            break;
    }

    return jewels_initlevel(bj);
}

/*****************************************************************************
* jewels_recordscore() inserts a high score into the high scores list and
*     returns the high score position.
******************************************************************************/
static int jewels_recordscore(struct game_context* bj) {
    int i;
    int position = 0;
    unsigned int current, temp;

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
* jewels_loadscores() loads the high scores saved file.
******************************************************************************/
static void jewels_loadscores(struct game_context* bj) {
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
* jewels_savescores() saves the high scores saved file.
******************************************************************************/
static void jewels_savescores(struct game_context* bj) {
    int fd;

    /* write out the high scores to the save file */
    fd = rb->open(SCORE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, bj->highscores, sizeof(bj->highscores));
    rb->close(fd);
    bj->dirty = false;
}

/*****************************************************************************
* jewels_loadgame() loads the saved game and returns load success.
******************************************************************************/
static bool jewels_loadgame(struct game_context* bj) {
    int fd;
    bool loaded = false;

    /* open game file */
    fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0) return loaded;

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &bj->score, sizeof(bj->score)) <= 0) break;
        if(rb->read(fd, &bj->level, sizeof(bj->level)) <= 0) break;
        if(rb->read(fd, &bj->type, sizeof(bj->type)) <= 0) break;
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
* jewels_savegame() saves the current game state.
******************************************************************************/
static void jewels_savegame(struct game_context* bj) {
    int fd;

    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &bj->score, sizeof(bj->score));
    rb->write(fd, &bj->level, sizeof(bj->level));
    rb->write(fd, &bj->type, sizeof(bj->type));
    rb->write(fd, bj->playboard, sizeof(bj->playboard));
    rb->close(fd);

    bj->resume = true;
}

/*****************************************************************************
* jewels_callback() is the default event handler callback which is called
*     on usb connect and shutdown.
******************************************************************************/
static void jewels_callback(void* param) {
    struct game_context* bj = (struct game_context*) param;
    if(bj->dirty) {
        rb->splash(HZ, "Saving high scores...");
        jewels_savescores(bj);
    }
}

/*****************************************************************************
* jewels_main() is the main game subroutine, it returns the final game status.
******************************************************************************/
static int jewels_main(struct game_context* bj) {
    int i, j;
    int w, h;
    int button;
    char str[18];
    bool startgame = false;
    bool inmenu = false;
    bool selected = false;
    enum menu_cmd cmd = MCMD_NONE;
    enum menu_result res;

    /* the cursor coordinates */
    int x=0, y=0;

    /* don't resume by default */
    bj->resume = false;

    /********************
    *       menu        *
    ********************/
    rb->lcd_clear_display();

    while(!startgame) {
        res = jewels_showmenu(&bjmenu[0], cmd);
        cmd = MCMD_NONE;

        rb->snprintf(str, 18, "High Score: %d", bj->highscores[0]);
        rb->lcd_getstringsize(str, &w, &h);
        rb->lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT-8, str);
        rb->lcd_update();

        switch(res) {
            case MRES_NEW:
                startgame = true;
                bj->type = GAME_TYPE_NORMAL;
                continue;

            case MRES_PUZZLE:
                startgame = true;
                bj->type = GAME_TYPE_PUZZLE;
                continue;

            case MRES_RESUME:
                if(!jewels_loadgame(bj)) {
                    rb->splash(HZ*2, "Nothing to resume");
                    rb->lcd_clear_display();
                } else {
                    startgame = true;
                }
                continue;

            case MRES_SCORES:
                rb->lcd_clear_display();

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

                rb->lcd_update();
                while(true) {
                    button = rb->button_get(true);
                    if(button != BUTTON_NONE && !(button&BUTTON_REL)) break;
                }
                rb->lcd_clear_display();
                continue;

            case MRES_HELP:
                /* welcome screen to display key bindings */
                rb->lcd_clear_display();
                rb->snprintf(str, 5, "%s", "Help");
                rb->lcd_getstringsize(str, &w, &h);
                rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, str);
#if CONFIG_KEYPAD == RECORDER_PAD
                rb->lcd_puts(0, 2, "Controls:");
                rb->lcd_puts(0, 3, "Directions = move");
                rb->lcd_puts(0, 4, "PLAY = select");
                rb->lcd_puts(0, 5, "Long PLAY = menu");
                rb->lcd_puts(0, 6, "OFF = cancel");
#elif CONFIG_KEYPAD == ONDIO_PAD
                rb->lcd_puts(0, 2, "Controls:");
                rb->lcd_puts(0, 3, "Directions = move");
                rb->lcd_puts(0, 4, "MENU = select");
                rb->lcd_puts(0, 5, "Long MENU = menu");
                rb->lcd_puts(0, 6, "OFF = cancel");
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
                rb->lcd_puts(0, 2, "Controls:");
                rb->lcd_puts(0, 3, "Directions = move");
                rb->lcd_puts(0, 4, "SELECT = select");
                rb->lcd_puts(0, 5, "Long SELECT = menu");
                rb->lcd_puts(0, 6, "PLAY = cancel");
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                rb->lcd_puts(0, 2, "Swap pairs of jewels to");
                rb->lcd_puts(0, 3, "form connected segments");
                rb->lcd_puts(0, 4, "of three or more of the");
                rb->lcd_puts(0, 5, "same type.");
                rb->lcd_puts(0, 7, "Controls:");
                rb->lcd_puts(0, 8, "Directions to move");
                rb->lcd_puts(0, 9, "SELECT to select");
                rb->lcd_puts(0, 10, "Long SELECT to show menu");
                rb->lcd_puts(0, 11, "OFF to cancel");
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
                rb->lcd_puts(0, 2, "Swap pairs of jewels to");
                rb->lcd_puts(0, 3, "form connected segments");
                rb->lcd_puts(0, 4, "of three or more of the");
                rb->lcd_puts(0, 5, "same type.");
                rb->lcd_puts(0, 7, "Controls:");
                rb->lcd_puts(0, 8, "Directions or scroll to move");
                rb->lcd_puts(0, 9, "SELECT to select");
                rb->lcd_puts(0, 10, "Long SELECT to show menu");
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
                rb->lcd_puts(0, 2, "Swap pairs of jewels to");
                rb->lcd_puts(0, 3, "form connected segments");
                rb->lcd_puts(0, 4, "of three or more of the");
                rb->lcd_puts(0, 5, "same type.");
                rb->lcd_puts(0, 7, "Controls:");
                rb->lcd_puts(0, 8, "Directions to move");
                rb->lcd_puts(0, 9, "SELECT to select");
                rb->lcd_puts(0, 10, "Long SELECT to show menu");
                rb->lcd_puts(0, 11, "PLAY to cancel");
#elif CONFIG_KEYPAD == GIGABEAT_PAD \
   || CONFIG_KEYPAD == MROBE100_PAD
                rb->lcd_puts(0, 2, "Swap pairs of jewels to");
                rb->lcd_puts(0, 3, "form connected segments");
                rb->lcd_puts(0, 4, "of three or more of the");
                rb->lcd_puts(0, 5, "same type.");
                rb->lcd_puts(0, 7, "Controls:");
                rb->lcd_puts(0, 8, "Directions to move");
                rb->lcd_puts(0, 9, "SELECT to select");
                rb->lcd_puts(0, 10, "Long SELECT to show menu");
                rb->lcd_puts(0, 11, "POWER to cancel");
#elif CONFIG_KEYPAD == SANSA_E200_PAD \
   || CONFIG_KEYPAD == SANSA_C200_PAD
                rb->lcd_puts(0, 2, "Swap pairs of jewels to");
                rb->lcd_puts(0, 3, "form connected segments");
                rb->lcd_puts(0, 4, "of three or more of the");
                rb->lcd_puts(0, 5, "same type.");
                rb->lcd_puts(0, 7, "Controls:");
                rb->lcd_puts(0, 8, "Directions to move");
                rb->lcd_puts(0, 9, "SELECT to select");
                rb->lcd_puts(0, 10, "Long SELECT to show menu");
                rb->lcd_puts(0, 11, "POWER to cancel");
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
                rb->lcd_puts(0, 2, "Swap pairs of jewels");
                rb->lcd_puts(0, 3, "to form connected");
                rb->lcd_puts(0, 4, "segments of three or ");
                rb->lcd_puts(0, 5, "more of the");
                rb->lcd_puts(0, 6, "same type.");
                rb->lcd_puts(0, 8, "Controls:");
                rb->lcd_puts(0, 9, "Directions or scroll to move");
                rb->lcd_puts(0, 10, "PLAY to select");
                rb->lcd_puts(0, 11, "Long PLAY for menu");
                rb->lcd_puts(0, 12, "POWER to cancel");
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
                rb->lcd_puts(0, 2, "Swap pairs of jewels");
                rb->lcd_puts(0, 3, "to form connected");
                rb->lcd_puts(0, 4, "segments of three or ");
                rb->lcd_puts(0, 5, "more of the");
                rb->lcd_puts(0, 6, "same type.");
                rb->lcd_puts(0, 8, "Controls:");
                rb->lcd_puts(0, 9, "Directions or scroll to move");
                rb->lcd_puts(0, 10, "PLAY to select");
                rb->lcd_puts(0, 11, "Long PLAY for menu");
                rb->lcd_puts(0, 12, "REC to cancel");
#else
    #warning: missing help text.
#endif
                rb->lcd_update();
                while(true) {
                    button = rb->button_get(true);
                    if(button != BUTTON_NONE && !(button&BUTTON_REL)) break;
                }
                rb->lcd_clear_display();
                continue;

            case MRES_QUIT:
                return BJ_QUIT;

            default:
                break;
        }

        /* handle menu button presses */
        button = rb->button_get(true);
        switch(button){
#ifdef JEWELS_SCROLLWHEEL
            case JEWELS_PREV:
            case (JEWELS_PREV|BUTTON_REPEAT):
#endif
            case JEWELS_UP:
            case (JEWELS_UP|BUTTON_REPEAT):
                cmd = MCMD_PREV;
                break;

#ifdef JEWELS_SCROLLWHEEL
            case JEWELS_NEXT:
            case (JEWELS_NEXT|BUTTON_REPEAT):
#endif
            case JEWELS_DOWN:
            case (JEWELS_DOWN|BUTTON_REPEAT):
                cmd = MCMD_NEXT;
                break;

            case JEWELS_SELECT:
            case JEWELS_RIGHT:
                cmd = MCMD_SELECT;
                break;

#ifdef JEWELS_CANCEL
#ifdef JEWELS_RC_CANCEL
            case JEWELS_RC_CANCEL:
#endif
            case JEWELS_CANCEL:
                return BJ_QUIT;
#endif

            default:
                if(rb->default_event_handler_ex(button, jewels_callback,
                   (void*) bj) == SYS_USB_CONNECTED)
                    return BJ_USB;
                break;
        }
    }

    /********************
    *       init        *
    ********************/
    jewels_init(bj);

    /********************
    *  setup the board  *
    ********************/
    bj->score += jewels_initlevel(bj);
    if (!jewels_movesavail(bj)) {
        switch(bj->type) {
            case GAME_TYPE_NORMAL:
                return BJ_LOSE;

            case GAME_TYPE_PUZZLE:
                do {
                    rb->splash(2*HZ, "No more moves!");
                    bj->score += jewels_initlevel(bj);
                } while(!jewels_movesavail(bj));
                break;
        }
    }

    /**********************
    *        play         *
    **********************/
    while(true) {
        int no_movesavail = false;

        if(!inmenu) {
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
        } else {
            res = jewels_showmenu(&bjmenu[1], cmd);
            cmd = MCMD_NONE;
            switch(res) {
                case MRES_RESUME:
                    inmenu = false;
                    selected = false;
                    continue;

                case MRES_PLAYBACK:
                    playback_control(rb);
                    rb->lcd_setfont(FONT_SYSFIXED);
                    inmenu = false;
                    selected = false;
                    break;

                case MRES_SAVE:
                    rb->splash(HZ, "Saving game...");
                    jewels_savegame(bj);
                    return BJ_END;

                case MRES_QUIT:
                    return BJ_END;

                case MRES_EXIT:
                    return BJ_QUIT_FROM_GAME;

                default:
                    break;
            }
        }

        /* handle game button presses */
        button = rb->button_get(true);
        switch(button){
            case JEWELS_LEFT:             /* move cursor left */
            case (JEWELS_LEFT|BUTTON_REPEAT):
                if(!inmenu) {
                    if(selected) {
                        bj->score += jewels_swapjewels(bj, x, y, SWAP_LEFT);
                        selected = false;
                        if (!jewels_movesavail(bj)) no_movesavail = true;
                    } else {
                        x = (x+BJ_WIDTH-1)%BJ_WIDTH;
                    }
                }
                break;

            case JEWELS_RIGHT:            /* move cursor right */
            case (JEWELS_RIGHT|BUTTON_REPEAT):
                if(!inmenu) {
                    if(selected) {
                        bj->score += jewels_swapjewels(bj, x, y, SWAP_RIGHT);
                        selected = false;
                        if (!jewels_movesavail(bj)) no_movesavail = true;
                    } else {
                        x = (x+1)%BJ_WIDTH;
                    }
                } else {
                    cmd = MCMD_SELECT;
                }
                break;

            case JEWELS_DOWN:             /* move cursor down */
            case (JEWELS_DOWN|BUTTON_REPEAT):
                if(!inmenu) {
                    if(selected) {
                        bj->score += jewels_swapjewels(bj, x, y, SWAP_DOWN);
                        selected = false;
                        if (!jewels_movesavail(bj)) no_movesavail = true;
                    } else {
                        y = (y+1)%(BJ_HEIGHT-1);
                    }
                } else {
                    cmd = MCMD_NEXT;
                }
                break;

            case JEWELS_UP:               /* move cursor up */
            case (JEWELS_UP|BUTTON_REPEAT):
                if(!inmenu) {
                    if(selected) {
                        bj->score += jewels_swapjewels(bj, x, y, SWAP_UP);
                        selected = false;
                        if (!jewels_movesavail(bj)) no_movesavail = true;
                    } else {
                        y = (y+(BJ_HEIGHT-1)-1)%(BJ_HEIGHT-1);
                    }
                } else {
                    cmd = MCMD_PREV;
                }
                break;

#ifdef JEWELS_SCROLLWHEEL
            case JEWELS_PREV:             /* scroll backwards */
            case (JEWELS_PREV|BUTTON_REPEAT):
                if(!inmenu) {
                    if(!selected) {
                        if(x == 0) {
                            y = (y+(BJ_HEIGHT-1)-1)%(BJ_HEIGHT-1);
                        }
                        x = (x+BJ_WIDTH-1)%BJ_WIDTH;
                    }
                } else {
                    cmd = MCMD_PREV;
                }
                break;

            case JEWELS_NEXT:             /* scroll forwards */
            case (JEWELS_NEXT|BUTTON_REPEAT):
                if(!inmenu) {
                    if(!selected) {
                        if(x == BJ_WIDTH-1) {
                            y = (y+1)%(BJ_HEIGHT-1);
                        }
                        x = (x+1)%BJ_WIDTH;
                    }
                } else {
                    cmd = MCMD_NEXT;
                }
                break;
#endif

            case JEWELS_SELECT:           /* toggle selected */
                if(!inmenu) {
                    selected = !selected;
                } else {
                    cmd = MCMD_SELECT;
                }
                break;

            case (JEWELS_SELECT|BUTTON_REPEAT): /* show menu */
                if(!inmenu) inmenu = true;
                break;

#ifdef JEWELS_CANCEL
#ifdef JEWELS_RC_CANCEL
            case JEWELS_RC_CANCEL:
#endif
            case JEWELS_CANCEL:           /* end game */
                return BJ_END;
                break;
#endif

            default:
                if(rb->default_event_handler_ex(button, jewels_callback,
                   (void*) bj) == SYS_USB_CONNECTED)
                    return BJ_USB;
                break;
        }

        if (no_movesavail) {
            switch(bj->type) {
                case GAME_TYPE_NORMAL:
                    return BJ_LOSE;

                case GAME_TYPE_PUZZLE:
                    do {
                        rb->splash(2*HZ, "No more moves!");
                        bj->score += jewels_initlevel(bj);
                    } while(!jewels_movesavail(bj));
                    break;
            }
        }

        switch(bj->type) {
            case GAME_TYPE_NORMAL:
                if(bj->score >= LEVEL_PTS) bj->score = jewels_nextlevel(bj);
                break;

            case GAME_TYPE_PUZZLE:
                if(jewels_puzzle_is_finished(bj))
                    bj->score += jewels_nextlevel(bj);
                break;
        }
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
    jewels_loadscores(&bj);

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    jewels_setcolors();

    while(!exit) {
        switch(jewels_main(&bj)){
            case BJ_LOSE:
                rb->splash(HZ*2, "No more moves!");
                /* fall through to BJ_END */

            case BJ_END:
                if(!bj.resume) {
                    if((position = jewels_recordscore(&bj))) {
                        rb->snprintf(str, 19, "New high score #%d!", position);
                        rb->splash(HZ*2, str);
                    }
                }
                break;

            case BJ_USB:
                rb->lcd_setfont(FONT_UI);
                return PLUGIN_USB_CONNECTED;

            case BJ_QUIT:
                if(bj.dirty) {
                    rb->splash(HZ, "Saving high scores...");
                    jewels_savescores(&bj);
                }
                exit = true;
                break;

            case BJ_QUIT_FROM_GAME:
                if(!bj.resume) {
                    if((position = jewels_recordscore(&bj))) {
                        rb->snprintf(str, 19, "New high score #%d!", position);
                        rb->splash(HZ*2, str);
                    }
                }
                if(bj.dirty) {
                    rb->splash(HZ, "Saving high scores...");
                    jewels_savescores(&bj);
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
