/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei
 *
 * Clone of 2048 by Gabriele Cirulli
 *
 * Thanks to [Saint], saratoga, and gevaerts for answering all my n00b
 * questions :)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* includes */

#include <plugin.h>

#include "lib/display_text.h"

#if LCD_DEPTH<4

/* pesky english spellings! */
/* it's GRAY, not GREY! */
#include "lib/grey.h"
/* who cares... */

#endif

#include "lib/helper.h"
#include "lib/highscore.h"
#include "lib/playback_control.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

#ifdef HAVE_LCD_BITMAP
#include "pluginbitmaps/_2048_background.h"
#include "pluginbitmaps/_2048_tiles.h"
#endif

/* defines */

#define ANIM_SLEEPTIME 5
#define BACKUP_FILE PLUGIN_GAMES_DATA_DIR "/2048.backup" /* where to save when the battery dies */
#define GRID_SIZE 4
#define HISCORES_FILE PLUGIN_GAMES_DATA_DIR "/2048.score"
#define NUM_SCORES 5
#define NUM_STARTING_TILES 2
#define RESUME_FILE PLUGIN_GAMES_DATA_DIR "/2048.save"
#define WHAT_FONT FONT_UI
#define SPACES (GRID_SIZE*GRID_SIZE)
#define MIN_SPACE (BMPHEIGHT__2048_tiles*0.134) /* space between tiles */
#define MAX_UNDOS 64
#define VERT_SPACING 4
#define WINNING_TILE 2048

/* screen-specific configuration */

#if LCD_WIDTH<LCD_HEIGHT
/* tall screens */
#define TITLE_X 0
#define TITLE_Y 0
#define BASE_Y (BMPHEIGHT__2048_tiles*1.5)
#define BASE_X (BMPHEIGHT__2048_tiles*.5-MIN_SPACE)
#define SCORE_X 0
#define SCORE_Y (max_numeral_height)
#define BEST_SCORE_X 0
#define BEST_SCORE_Y (2*max_numeral_height)
#else
/* wide screens or square screens*/
#define TITLE_X 0
#define TITLE_Y 0
#define BASE_X (LCD_WIDTH-(GRID_SIZE*BMPHEIGHT__2048_tiles)-(((GRID_SIZE+1)*MIN_SPACE)))
#define BASE_Y (BMPHEIGHT__2048_tiles*.5-MIN_SPACE)
#define SCORE_X 0
#define SCORE_Y (max_numeral_height)
#define BEST_SCORE_X 0
#define BEST_SCORE_Y (2*max_numeral_height)
#endif

#define BACKGROUND_X (BASE_X-MIN_SPACE)
#define BACKGROUND_Y (BASE_Y-MIN_SPACE)

/* uncomment to enable undos */
/* #define ENABLE_UNDO */

/* uncomment to enable the saving of backups */
/* #define ENABLE_BACKUPS */
/* key mappings */

#define KEY_UP PLA_UP
#define KEY_DOWN PLA_DOWN
#define KEY_LEFT PLA_LEFT
#define KEY_RIGHT PLA_RIGHT
#define KEY_EXIT PLA_CANCEL
#define KEY_UNDO PLA_SELECT

/* colors */

#define BACKGROUND (LCD_RGBPACK(0xfa, 0xf8, 0xef))
#define BOARD_BACKGROUND (LCD_RGBPACK(0xbb, 0xad, 0xa0))
#define TEXT_COLOR (LCD_RGBPACK(0x77, 0x6e, 0x65))

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/* What is needed to save/load a game */
struct undo_data_node {
    /* basically a simplified version of game_ctx_t */
    int grid[GRID_SIZE][GRID_SIZE];
    int score;
};

/* undo data */
struct undo_data_t {
    struct undo_data_node undo_stack[MAX_UNDOS];
    int top;
};

/* game data */
struct game_ctx_t {
    int grid[GRID_SIZE][GRID_SIZE];
    int score;
    int cksum; /* sum of grid, XORed by score */
    bool already_won;
};

static struct game_ctx_t ctx_data;
/* use a pointer to make save/load easier */
static struct game_ctx_t *ctx=&ctx_data;
static struct undo_data_t undo_data;
static struct undo_data_t *undo=&undo_data;

/* temporary data */
static bool merged_grid[GRID_SIZE][GRID_SIZE];
static int old_grid[GRID_SIZE][GRID_SIZE];
static int max_numeral_width=-1, max_numeral_height=-1;
static bool loaded=false;
static int winning_tile_width;

/* first init_game will set this, when it is exceeded, it will be updated in the slide functions */
static int best_score;

static bool abnormal_exit=true;
/* TODO
 * Sounds!
 * Better animations!
 */
static struct highscore highscores[NUM_SCORES];

#ifdef HAVE_LCD_COLORS
/* map tile values to colors */
struct tile_color {
    int value;
    unsigned int fg;
    unsigned int bg;
};
struct tile_color tile_colors[11] = {
    {2, LCD_RGBPACK(0x77, 0x6e, 0x65), LCD_RGBPACK(0xee, 0xe4, 0xda)},
    {4, LCD_RGBPACK(0x77, 0x6e, 0x65), LCD_RGBPACK(0xed, 0xe0, 0xc8)},
    {8, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xf2, 0xb1, 0x79)},
    {16, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xf5, 0x95, 0x63)},
    {32, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xf6, 0x7c, 0x5f)},
    {64, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xf6, 0x5e, 0x3b)},
    {128, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xed, 0xcf, 0x72)},
    {256, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xed, 0xcc, 0x61)},
    {512, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xed, 0xc8, 0x50)},
    {1024, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xed, 0xc5, 0x3f)},
    {2048, LCD_RGBPACK(0xf9, 0xf6, 0xf2), LCD_RGBPACK(0xed, 0xc2, 0x2e)}
};
#endif

/* returns a random int between min and max */
static inline int rand_range(int min, int max)
{
    return rb->rand()%(max-min+1)+min;
}

/* prepares to exit */
static void cleanup(void)
{
    backlight_use_settings();
}

/* returns 2 or 4 */
static inline int rand_2_or_4(void)
{
    /* 1 in 10 chance of a four */
    if(rb->rand()%10==0)
        return 4;
    else
        return 2;
}

/* display the help text */
static bool do_help(void)
{
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_setfont(FONT_UI);
    char* help_text[]= {"2048", "", "Aim",
                        "", "Join", "the", "numbers", "to", "get", "to", "the", "2048", "tile!", "", "",
                        "How", "to", "Play", "",
                        "", "Use", "the", "directional", "keys", "to", "move", "the", "tiles.", "When",
                        "two", "tiles", "with", "the", "same", "number", "touch,", "they", "merge", "into", "one!"};
    struct style_text style[]={
        {0, TEXT_CENTER|TEXT_UNDERLINE},
        {2, C_RED},
        {15, C_RED}, {16, C_RED}, {17,C_RED},
        LAST_STYLE_ITEM
    };
    return display_text(ARRAYLEN(help_text), help_text, style,NULL,true);
}

/*** the logic for sliding ***/

/* this is the helper function that does the actual tile moving */

static inline void slide_internal(int startx, int starty,
                                  int stopx, int stopy,
                                  int dx, int dy,
                                  int lookx, int looky)
{
    int best_score_before=best_score;
    for(int y=starty;y!=stopy;y+=dy)
    {
        for(int x=startx;x!=stopx;x+=dx)
        {
            if(ctx->grid[x+lookx][y+looky]==ctx->grid[x][y] && ctx->grid[x][y] && !merged_grid[x+lookx][y+looky] && !merged_grid[x][y]) /* Slide into */
            {
                /* Each merged tile cannot be merged again */
                merged_grid[x+lookx][y+looky]=true;
                ctx->grid[x+lookx][y+looky]=2*ctx->grid[x][y];
                ctx->score+=ctx->grid[x+lookx][y+looky];
                ctx->grid[x][y]=0;
            }
            else if(ctx->grid[x+lookx][y+looky]==0) /* Empty! */
            {
                ctx->grid[x+lookx][y+looky]=ctx->grid[x][y];
                ctx->grid[x][y]=0;
            }
        }
    }
    if(ctx->score>best_score_before)
        best_score=ctx->score;
}

/* these functions move each tile 1 space in the direction specified via calls to slide_internal */

/* Up
   0
   1 ^ ^ ^ ^
   2 ^ ^ ^ ^
   3 ^ ^ ^ ^
   0 1 2 3
*/
static void up(void)
{
    slide_internal(0, 1,  /* start values */
                   GRID_SIZE, GRID_SIZE, /* stop values */
                   1, 1, /* delta values */
                   0, -1); /* lookahead values */
}
/* Down
   0 v v v v
   1 v v v v
   2 v v v v
   3
   0 1 2 3
*/
static void down(void)
{
    slide_internal(0, GRID_SIZE-2,
                   GRID_SIZE, -1,
                   1, -1,
                   0, 1);
}
/* Left
   0   < < <
   1   < < <
   2   < < <
   3   < < <
   0 1 2 3
*/
static void left(void)
{
    slide_internal(1, 0,
                   GRID_SIZE, GRID_SIZE,
                   1, 1,
                   -1, 0);
}
/* Right
   0 > > >
   1 > > >
   2 > > >
   3 > > >
   0 1 2 3
*/
static void right(void)
{
    slide_internal(GRID_SIZE-2, 0, /* start */
                   -1, GRID_SIZE, /* stop */
                   -1, 1, /* delta */
                   1, 0); /* lookahead */
}
static int biggest_tile=0;

#ifndef HAVE_LCD_BITMAP
/* draws the grid and score */
/* old text-only version */
static void draw(void)
{
    rb->lcd_set_background(BACKGROUND);
    rb->lcd_clear_display();
    /* Draw the grid */
    /* find the biggest tile */
    for(int x=0;x<GRID_SIZE;++x)
    {
        for(int y=0;y<GRID_SIZE;++y)
            if(ctx->grid[x][y]>biggest_tile)
                biggest_tile=ctx->grid[x][y];
    }
    char str[32];
    rb->snprintf(str, 31,"%d", biggest_tile);
    int biggest_tile_width=rb->strlen(str)*max_numeral_width+MIN_SPACE;
    char emptyCell[32];
    emptyCell[31]=0;
    rb->strcpy(emptyCell, str);
    for(int i=0;i<32 && emptyCell[i];++i)
    {
        emptyCell[i]=' ';
    }
    for(int y=0;y<GRID_SIZE;++y)
    {
        for(int x=0;x<GRID_SIZE;++x)
        {
            if(ctx->grid[x][y])
            {
                if(ctx->grid[x][y]>biggest_tile)
                    biggest_tile=ctx->grid[x][y];
                rb->snprintf(str,31,"%d", ctx->grid[x][y]);
                /* find the appropriate colors */
                bool color_found=false;
                unsigned int fgcolor=LCD_WHITE, bgcolor=LCD_BLACK;
                for(unsigned int i=0;i<sizeof(tile_colors)/sizeof(struct tile_color) && !color_found;++i)
                {
                    if(ctx->grid[x][y]==tile_colors[i].value)
                    {
                        fgcolor=tile_colors[i].fg;
                        bgcolor=tile_colors[i].bg;
                        color_found=true;
                    }
                }
                rb->lcd_set_foreground(fgcolor);
                rb->lcd_set_background(bgcolor);
                rb->lcd_putsxy(biggest_tile_width*x,y*max_numeral_height+max_numeral_height,str);
            }
            else
            {
                rb->lcd_set_background(LCD_RGBPACK(0xcd, 0xc0, 0xb4));
                rb->lcd_putsxy(biggest_tile_width*x, y*max_numeral_height+max_numeral_height,emptyCell);
            }
        }
    }
    rb->lcd_set_background(TEXT_COLOR);
    rb->lcd_set_foreground(LCD_WHITE);
    /* Now draw the score, and the game title */
    rb->snprintf(str, 31, "Score: %d", ctx->score);
    int str_width, str_height;
    rb->font_getstringsize(str, &str_width, &str_height, WHAT_FONT);
    int score_leftmost=LCD_WIDTH-str_width-1;
    /* Check if there is enough space to display "Score: ", otherwise, only display the score */
    if(score_leftmost>=0)
        rb->lcd_putsxy(score_leftmost,0,str);
    else
        rb->lcd_putsxy(score_leftmost,0,str+rb->strlen("Score: "));
    /* Reuse the same string for the title */
    rb->lcd_set_foreground(TEXT_COLOR);
    rb->lcd_set_background(BACKGROUND);
    rb->snprintf(str, 31, "%d", WINNING_TILE);
    rb->font_getstringsize(str, &str_width, &str_height, WHAT_FONT);
    if(str_width<score_leftmost)
    {
        rb->lcd_putsxy(0,0,str);
    }
    rb->lcd_update();
}
#else
/* new drawing function */

static void draw(void)
{
    /* use this table to find the coordinates of the tiles in the bitmap */
    struct int_pair {
        int a;
        int b;
    };
    /* using this table for offsets in the bitmap is the fastest option */
    /* on targets with EXTREMELY small memory, this could be calculated
       with offset=BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*ilog2(ctx->grid[x][y]) */
    const struct int_pair image_table[] = {
        { 0, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles },
        { 2, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*2 },
        { 4, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*3 },
        { 8, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*4 },
        { 16, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*5 },
        { 32, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*6 },
        { 64, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*7 },
        { 128, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*8 },
        { 256, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*9 },
        { 512, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*10 },
        { 1024, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*11 },
        { 2048, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*12 },
        { 4096, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*13 },
        { 8192, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*14 },
        { 16384, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*15 },
        { 32768, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*16 },
        { 65536, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*17 },
        { 131072, BMPWIDTH__2048_tiles-BMPHEIGHT__2048_tiles*18 }
    };
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(BACKGROUND);
#endif
    rb->lcd_clear_display();
    rb->lcd_bitmap(_2048_background, BACKGROUND_X, BACKGROUND_Y, BMPWIDTH__2048_background, BMPWIDTH__2048_background);

    /*
      grey_gray_bitmap(_2048_background, BACKGROUND_X, BACKGROUND_Y, BMPWIDTH__2048_background, BMPHEIGHT__2048_background);
    */

    for(int y=0;y<GRID_SIZE;++y)
    {
        for(int x=0;x<GRID_SIZE;++x)
        {
            for(unsigned int i=0;i<sizeof(image_table)/sizeof(struct int_pair);++i)
            {
                if(image_table[i].a==ctx->grid[x][y])
                {

#ifndef HAVE_LCD_COLOR
                    /* on B+W screens, tiles 128-2048 have too little contrast */
                    /* to correct this, negate the bitmap */
#endif

                    rb->lcd_bitmap_part(_2048_tiles, /* source */
                                        image_table[i].b, 0, /* source upper left corner */
                                        STRIDE(SCREEN_MAIN, BMPWIDTH__2048_tiles, BMPHEIGHT__2048_tiles), /* stride */
                                        (BMPHEIGHT__2048_tiles+MIN_SPACE)*x+BASE_X, (BMPHEIGHT__2048_tiles+MIN_SPACE)*y+BASE_Y, /* dest upper-left corner */
                                        BMPHEIGHT__2048_tiles, BMPHEIGHT__2048_tiles); /* size of the cut section */
                    break;
                }
            }
        }
    }
    /* draw the title */
    char buf[32];
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(TEXT_COLOR);
#endif
    rb->snprintf(buf, 31, "%d", WINNING_TILE);

    /* check if the title will go into the grid */
    int w, h;
    rb->lcd_setfont(FONT_UI);
    rb->font_getstringsize(buf, &w, &h, FONT_UI);
    if(w+TITLE_X>=BACKGROUND_X && h+TITLE_Y>=BACKGROUND_Y)
    {
        /* if it goes into the grid, use the system font, which should be smaller */
        rb->lcd_setfont(FONT_SYSFIXED);
    }
    rb->lcd_putsxy(TITLE_X, TITLE_Y, buf);

    /* draw the score */
    rb->snprintf(buf, 31, "Score: %d", ctx->score);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(BOARD_BACKGROUND);
#endif
    rb->lcd_setfont(FONT_UI);
    rb->font_getstringsize(buf, &w, &h, FONT_UI);
    if(w+SCORE_X>=BACKGROUND_X && h+SCORE_Y>=BACKGROUND_Y)
    {
        /* score overflows */
        /* first see if it fits with Score: and FONT_SYSFIXED */
        rb->lcd_setfont(FONT_SYSFIXED);
        rb->font_getstringsize(buf, &w, &h, FONT_SYSFIXED);
        if(w+SCORE_X < BACKGROUND_X)
            /* it fits, go and draw it */
            goto draw_lbl;

        /* now try with S: and FONT_UI */
        rb->snprintf(buf, 31, "S: %d", ctx->score);
        rb->font_getstringsize(buf, &w, &h, FONT_UI);
        rb->lcd_setfont(FONT_UI);
        if(w+SCORE_X<BACKGROUND_X)
            goto draw_lbl;

        /* now try with S: and FONT_SYSFIXED */
        rb->snprintf(buf, 31, "S: %d", ctx->score);
        rb->font_getstringsize(buf, &w, &h, FONT_SYSFIXED);
        rb->lcd_setfont(FONT_SYSFIXED);
        if(w+SCORE_X<BACKGROUND_X)
            goto draw_lbl;

        /* then try without Score: and FONT_UI */
        rb->snprintf(buf, 31, "%d", ctx->score);
        rb->font_getstringsize(buf, &w, &h, FONT_UI);
        rb->lcd_setfont(FONT_UI);
        if(w+SCORE_X<BACKGROUND_X)
            goto draw_lbl;

        /* as a last resort, don't use Score: and use the system font */
        rb->snprintf(buf, 31, "%d", ctx->score);
        rb->lcd_setfont(FONT_SYSFIXED);
    }
draw_lbl:
    rb->lcd_putsxy(SCORE_X, SCORE_Y, buf);

    /* draw the best score */

    rb->snprintf(buf, 31, "Best: %d", best_score);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(BOARD_BACKGROUND);
#endif
    rb->lcd_setfont(FONT_UI);
    rb->font_getstringsize(buf, &w, &h, FONT_UI);
    if(w+BEST_SCORE_X>=BACKGROUND_X && h+BEST_SCORE_Y>=BACKGROUND_Y)
    {
        /* score overflows */
        /* first see if it fits with Score: and FONT_SYSFIXED */
        rb->lcd_setfont(FONT_SYSFIXED);
        rb->font_getstringsize(buf, &w, &h, FONT_SYSFIXED);
        if(w+BEST_SCORE_X < BACKGROUND_X)
            /* it fits, go and draw it */
            goto draw_best;

        /* now try with S: and FONT_UI */
        rb->snprintf(buf, 31, "B: %d", best_score);
        rb->font_getstringsize(buf, &w, &h, FONT_UI);
        rb->lcd_setfont(FONT_UI);
        if(w+BEST_SCORE_X<BACKGROUND_X)
            goto draw_best;

        /* now try with S: and FONT_SYSFIXED */
        rb->snprintf(buf, 31, "B: %d", best_score);
        rb->font_getstringsize(buf, &w, &h, FONT_SYSFIXED);
        rb->lcd_setfont(FONT_SYSFIXED);
        if(w+BEST_SCORE_X<BACKGROUND_X)
            goto draw_best;

        /* then try without Score: and FONT_UI */
        rb->snprintf(buf, 31, "%d", best_score);
        rb->font_getstringsize(buf, &w, &h, FONT_UI);
        rb->lcd_setfont(FONT_UI);
        if(w+BEST_SCORE_X<BACKGROUND_X)
            goto draw_best;

        /* as a last resort, don't use Score: and use the system font */
        rb->snprintf(buf, 31, "%d", best_score);
        rb->lcd_setfont(FONT_SYSFIXED);
    }
draw_best:
    rb->lcd_putsxy(BEST_SCORE_X, BEST_SCORE_Y, buf);

    rb->lcd_update();
    /* revert the font back */
    rb->lcd_setfont(WHAT_FONT);
}
#endif /* else */
/* place a 2 or 4 in a random empty space */
static void place_random(void)
{
    int xpos[SPACES],ypos[SPACES];
    int back=0;
    /* get the indexes of empty spaces */
    for(int y=0;y<GRID_SIZE;++y)
        for(int x=0;x<GRID_SIZE;++x)
        {
            if(!ctx->grid[x][y])
            {
                xpos[back]=x;
                ypos[back]=y;
                ++back;
            }
        }
    if(!back)
        /* no empty spaces */
        return;
    int idx=rand_range(0,back-1);
    ctx->grid[xpos[idx]][ypos[idx]]=rand_2_or_4();
}

/* copies old_grid to ctx->grid */
static void restore_old_grid(void)
{
    memcpy(&ctx->grid, &old_grid, sizeof(int)*SPACES);
}

/* checks for a win or loss */
static bool check_gameover(void)
{
    int numempty=0;
    for(int y=0;y<GRID_SIZE;++y)
    {
        for(int x=0;x<GRID_SIZE;++x)
        {
            if(ctx->grid[x][y]==0)
                ++numempty;
            if(ctx->grid[x][y]==WINNING_TILE && !ctx->already_won)
            {
                /* Let the user see the tile in its full glory... */
                draw();
                ctx->already_won=true;
                rb->splash(HZ*2,"You win!");
                const struct text_message prompt={(const char*[]){"Keep playing?"}, 1};
                enum yesno_res keepgoing=rb->gui_syncyesno_run(&prompt, NULL, NULL);
                if(keepgoing==YESNO_NO)
                    return true;
                else
                    return false;
            }
        }
    }
    if(!numempty)
    {
        /* No empty spaces, check for valid moves */
        /* Then, get the current score */
        int oldscore=ctx->score;
        memset(&merged_grid,0,SPACES*sizeof(bool));
        up();
        if(memcmp(&old_grid, &ctx->grid, sizeof(int)*SPACES))
        {
            restore_old_grid();
            ctx->score=oldscore;
            return false;
        }
        restore_old_grid();
        memset(&merged_grid,0,SPACES*sizeof(bool));
        down();
        if(memcmp(&old_grid, &ctx->grid, sizeof(int)*SPACES))
        {
            restore_old_grid();
            ctx->score=oldscore;
            return false;
        }
        restore_old_grid();
        memset(&merged_grid,0,SPACES*sizeof(bool));
        left();
        if(memcmp(&old_grid, &ctx->grid, sizeof(int)*SPACES))
        {
            restore_old_grid();
            ctx->score=oldscore;
            return false;
        }
        restore_old_grid();
        memset(&merged_grid,0,SPACES*sizeof(bool));
        right();
        if(memcmp(&old_grid, &ctx->grid, sizeof(int)*SPACES))
        {
            restore_old_grid();
            ctx->score=oldscore;
            return false;
        }
        /* no more legal moves */
        ctx->score=oldscore;
        draw(); /* Shame the player :) */
        rb->splash(HZ*2, "Game Over!");
        return true;
    }
    return false;
}

/* loads highscores from disk */
/* creates an empty structure if the file does not exist */
static void load_hs(void)
{
    if(rb->file_exists(HISCORES_FILE))
        highscore_load(HISCORES_FILE, highscores, NUM_SCORES);
    else
        memset(highscores, 0, sizeof(struct highscore)*NUM_SCORES);
}

/* save the current grid on the undo stack */
static void undo_push(void)
{
    if(undo->top==MAX_UNDOS)
    {
        memmove(undo->undo_stack, &(undo->undo_stack[1]), (MAX_UNDOS-1)*sizeof(struct undo_data_node));
        --undo->top;
    }
    undo->undo_stack[undo->top].score=ctx->score;
    memcpy(undo->undo_stack[undo->top].grid, ctx->grid, sizeof(int)*SPACES);
    ++undo->top;
}

#ifdef ENABLE_UNDOS /* stop warnings */
/* undo the user's last move */
static void pop_undo(void)
{
    if(undo->top>0)
    {
        --undo->top;
        memcpy(ctx->grid, undo->undo_stack[undo->top].grid, sizeof(int)*SPACES);
        ctx->score=undo->undo_stack[undo->top].score;
    }
}
#endif

/* initialize the data structures */
static void init_game(bool newgame)
{
    best_score=highscores[0].score;
    if(newgame)
    {
        /* initialize the game context */
        memset(ctx->grid, 0, sizeof(int)*SPACES);
        for(int i=0;i<NUM_STARTING_TILES;++i)
        {
            place_random();
        }
        ctx->score=0;
        ctx->already_won=false;
        memset(undo, 0, sizeof(struct undo_data_t));
    }
    /* using the menu resets the font */
    /* set it again here */
    rb->lcd_setfont(WHAT_FONT);
    /* Now calculate font sizes, etc */
    max_numeral_width=-1;
    max_numeral_height=-1;
    /* Find the width of the widest character so the drawing can be nicer */
    for(char c='0';c<='9';++c)
    {
        int width=rb->font_get_width(rb->font_get(WHAT_FONT), c);
        if(width>max_numeral_width)
            max_numeral_width=width;
    }
    biggest_tile=0;
    /* Now get the height of the font */
    rb->font_getstringsize("0123456789", NULL, &max_numeral_height,WHAT_FONT);
    max_numeral_height+=VERT_SPACING;
    /* Find the size of the winning tile, used to draw the title */
    char buf[32];
    rb->snprintf(buf, 31, "%d", WINNING_TILE);
    rb->font_getstringsize(buf, &winning_tile_width, NULL, WHAT_FONT);
    backlight_ignore_timeout();
    rb->lcd_clear_display();
    draw();
}

/* save a backup copy of the game */
static void save_backup(void)
{
#ifdef ENABLE_BACKUPS
    int fd=rb->open(BACKUP_FILE,O_WRONLY|O_CREAT, 0666);
    if(fd<0)
    {
        rb->splash(2*HZ, "Saving failed.");
        return;
    }
    ctx->cksum=0;
    for(int x=0;x<GRID_SIZE;++x)
        for(int y=0;y<GRID_SIZE;++y)
            ctx->cksum+=ctx->grid[x][y];
    ctx->cksum^=ctx->score;
    rb->write(fd, ctx,sizeof(struct game_ctx_t));
    rb->close(fd);
#endif
}

/* save the current game state */
static void save_game(void)
{
    rb->splash(0, "Saving...");
    int fd=rb->open(RESUME_FILE,O_WRONLY|O_CREAT, 0666);
    if(fd<0)
    {
        return;
    }
    ctx->cksum=0;
    for(int x=0;x<GRID_SIZE;++x)
        for(int y=0;y<GRID_SIZE;++y)
            ctx->cksum+=ctx->grid[x][y];
    ctx->cksum^=ctx->score;
    rb->write(fd, ctx,sizeof(struct game_ctx_t));
    /* save the undo data, even if undos are not enabled to keep save data compatible */
    rb->write(fd, undo, sizeof(struct undo_data_t));
    rb->close(fd);
    rb->lcd_update();
}

/* loads a saved game, returns true on success */
static bool load_game(void)
{
    int success=0;
    int fd=rb->open(RESUME_FILE, O_RDONLY);
    if(fd<0)
    {
        rb->remove(RESUME_FILE);
        return false;
    }
    int numread=rb->read(fd, ctx, sizeof(struct game_ctx_t));
    int calc=0;
    for(int x=0;x<GRID_SIZE;++x)
        for(int y=0;y<GRID_SIZE;++y)
            calc+=ctx->grid[x][y];
    calc^=ctx->score;
    if(numread==sizeof(struct game_ctx_t) && calc==ctx->cksum)
        ++success;
    numread=rb->read(fd, undo, sizeof(struct undo_data_t));
    if(numread==sizeof(struct undo_data_t))
        ++success;
    rb->close(fd);
    rb->remove(RESUME_FILE);
    return (success==2);
}

/* update the highscores with ctx->score */
static void hs_check_update(bool noshow)
{
    /* first, find the biggest tile to show as the level */
    int biggest=0;
    for(int x=0;x<GRID_SIZE;++x)
    {
        for(int y=0;y<GRID_SIZE;++y)
        {
            if(ctx->grid[x][y]>biggest)
                biggest=ctx->grid[x][y];
        }
    }
    int hs_idx=highscore_update(ctx->score,biggest, "", highscores,NUM_SCORES);
    if(!noshow)
    {
        /* show the scores if there is a new high score */
        if(hs_idx>=0)
        {
            rb->splashf(HZ*2,"New High Score: %d", ctx->score);
            rb->lcd_clear_display();
            highscore_show(hs_idx,highscores,NUM_SCORES,true);
        }
    }
    highscore_save(HISCORES_FILE,highscores,NUM_SCORES);
}

/* asks the user if they wish to quit */
static bool confirm_quit(void)
{
    const struct text_message prompt={(const char*[]){"Are you sure?", "This will clear your current game."}, 2};
    enum yesno_res response=rb->gui_syncyesno_run(&prompt, NULL, NULL);
    if(response==YESNO_NO)
        return false;
    else
        return true;
}

/* show the pause menu */
static int do_2048_pause_menu(void)
{
    int sel=0;
    MENUITEM_STRINGLIST(menu,"2048 Menu", NULL,
                        "Resume Game",
                        "Start New Game",
                        "High Scores",
                        "Playback Control",
                        "Help",
                        "Quit without Saving",
                        "Quit");
    bool quit=false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            draw();
            return 0;
        case 1:
        {
            if(!confirm_quit())
                break;
            else
            {
                hs_check_update(false);
                return 1;
            }
        }
        case 2:
            highscore_show(-1,highscores, NUM_SCORES, true);
            break;
        case 3:
            playback_control(NULL);
            break;
        case 4:
            do_help();
            break;
        case 5: /* quit w/o saving */
        {
            if(!confirm_quit())
                break;
            else
            {
                return 2;
            }
        }
        case 6:
            return 3;
        }
    }
    return 0;
}

static void exit_handler(void)
{
    cleanup();
    if(abnormal_exit)
        save_game();
    return;
}
static bool check_hs;
/* main game loop */
static enum plugin_status do_game(bool newgame)
{
    init_game(newgame);
    rb_atexit(&exit_handler);
    int made_move=0, move_was_undo=0;
    while(1)
    {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false); /* Save battery when idling */
#endif
        /* Wait for a button press */
        int button=pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        made_move=0;
        move_was_undo=0;
        memset(&merged_grid,0,SPACES*sizeof(bool));
        memcpy(&old_grid, &ctx->grid, sizeof(int)*SPACES);
        int grid_before_anim_step[GRID_SIZE][GRID_SIZE];
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true); /* doing work now... */
#endif
        switch(button)
        {
        case KEY_UP:
            for(int i=0;i<GRID_SIZE-1;++i)
            {
                memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES);
                up();
                if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES))
                {
                    rb->sleep(ANIM_SLEEPTIME);
                    draw();
                }
            }
            made_move=1;
            break;
        case KEY_DOWN:
            for(int i=0;i<GRID_SIZE-1;++i)
            {
                memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES);
                down();
                if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES))
                {
                    rb->sleep(ANIM_SLEEPTIME);
                    draw();
                }
            }
            made_move=1;
            break;
        case KEY_LEFT:
            for(int i=0;i<GRID_SIZE-1;++i)
            {
                memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES);
                left();
                if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES))
                {
                    rb->sleep(ANIM_SLEEPTIME);
                    draw();
                }
            }
            made_move=1;
            break;
        case KEY_RIGHT:
            for(int i=0;i<GRID_SIZE-1;++i)
            {
                memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES);
                right();
                if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*SPACES))
                {
                    rb->sleep(ANIM_SLEEPTIME);
                    draw();
                }
            }
            made_move=1;
            break;
        case KEY_EXIT:
            switch(do_2048_pause_menu())
            {
            case 0: /* resume */
                break;
            case 1: /* new game */
                init_game(true);
                made_move=1;
                continue;
                break;
            case 2: /* quit without saving */
                check_hs=true;
                rb->remove(RESUME_FILE);
                return PLUGIN_ERROR;
            case 3: /* save and quit */
                check_hs=false;
                save_game();
                return PLUGIN_ERROR;
            }
            break;
#ifdef ENABLE_UNDO
        case KEY_UNDO: /* undo */
            pop_undo();
            made_move=1;
            move_was_undo=1;
            break;
#endif
        default:
        {
            exit_on_usb(button);
            break;
        }
        }
        if(made_move)
        {
            if(!rb->battery_level_safe())
                save_backup();
            /* Check if we actually moved, then add random */
            if(memcmp(&old_grid, ctx->grid, sizeof(int)*SPACES) && !move_was_undo)
            {
                place_random();
                undo_push();
            }
            memcpy(&old_grid, ctx->grid, sizeof(int)*SPACES);
            if(check_gameover())
                return PLUGIN_OK;
            draw();
        }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false); /* back to idle */
#endif
        rb->yield();
    }
}

/* decide if this_item should be shown in the main menu */
/* used to hide resume option when there is no save */
static int mainmenu_shouldshow(int action, const struct menu_item_ex *this_item)
{
    int idx=((intptr_t)this_item);
    if(action==ACTION_REQUEST_MENUITEM && !loaded && (idx==0 || idx==5))
        return ACTION_EXIT_MENUITEM;
    return action;
}

/* show the main menu */
static enum plugin_status do_2048_menu(void)
{
    int sel=0;
    loaded=load_game();
    MENUITEM_STRINGLIST(menu,"2048 Menu", mainmenu_shouldshow, "Resume Game", "Start New Game","High Scores","Playback Control", "Help", "Quit without Saving", "Quit");
    bool quit=false;
    while(!quit)
    {
        int item;
        switch(item=rb->do_menu(&menu,&sel,NULL,false))
        {
        case 0: /* Start new game or resume a game */
        case 1:
        {
            if(item==1 && loaded)
            {
                if(!confirm_quit())
                    break;
            }
            enum plugin_status ret=do_game(item==1);
            switch(ret)
            {
            case PLUGIN_OK:
            {
                loaded=false;
                rb->remove(RESUME_FILE);
                hs_check_update(false);
                break;
            }
            case PLUGIN_USB_CONNECTED:
                save_game();
                /* Don't bother showing the high scores... */
                return ret;
            case PLUGIN_ERROR: /* exit without menu */
                if(check_hs)
                    hs_check_update(false);
                return PLUGIN_OK;
            default:
                break;
            }
            break;
        }
        case 2:
            highscore_show(-1,highscores, NUM_SCORES, true);
            break;
        case 3:
            playback_control(NULL);
            break;
        case 4:
            do_help();
            break;
        case 5:
            if(confirm_quit())
                return PLUGIN_OK;
        case 6:
            if(loaded)
                save_game();
            return PLUGIN_OK;
        default:
            break;
        }
    }
    return PLUGIN_OK;
}
enum plugin_status plugin_start(const void* param)
{
    (void)param;
    rb->srand(*(rb->current_tick));
    load_hs();
    rb->lcd_setfont(WHAT_FONT);
    enum plugin_status ret=do_2048_menu();
    highscore_save(HISCORES_FILE,highscores,NUM_SCORES);
    cleanup();
    abnormal_exit=false;
    return ret;
}
