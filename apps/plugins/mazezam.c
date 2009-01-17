/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006, 2008 Malcolm Tyrrell
 *
 * MazezaM - a Rockbox version of my ZX Spectrum game from 2002
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
#include "lib/configfile.h"
#include "lib/helper.h"
#include "lib/pluginlib_actions.h"
#include "lib/playback_control.h"

/* Include standard plugin macro */
PLUGIN_HEADER

/* The plugin actions of interest. */
const struct button_mapping *plugin_contexts[]
= {generic_directions, generic_actions};

MEM_FUNCTION_WRAPPERS;

/* Use the standard plugin buttons rather than a hard-to-maintain list of
 * MazezaM specific buttons. */
#define MAZEZAM_UP                  PLA_UP
#define MAZEZAM_UP_REPEAT           PLA_UP_REPEAT
#define MAZEZAM_DOWN                PLA_DOWN
#define MAZEZAM_DOWN_REPEAT         PLA_DOWN_REPEAT
#define MAZEZAM_LEFT                PLA_LEFT
#define MAZEZAM_LEFT_REPEAT         PLA_LEFT_REPEAT
#define MAZEZAM_RIGHT               PLA_RIGHT
#define MAZEZAM_RIGHT_REPEAT        PLA_RIGHT_REPEAT
#define MAZEZAM_MENU                PLA_QUIT

/* All the text is here */
#define MAZEZAM_TEXT_GAME_OVER       "Game Over"
#define MAZEZAM_TEXT_LIVES           "Level %d, Lives %d"
#define MAZEZAM_TEXT_CHECKPOINT      "Checkpoint reached"
#define MAZEZAM_TEXT_WELLDONE_TITLE  "You have escaped!"
#define MAZEZAM_TEXT_WELLDONE_OPTION "Goodbye"
#define MAZEZAM_TEXT_MAZEZAM_MENU    "MazezaM Menu"
#define MAZEZAM_TEXT_RETRY_LEVEL     "Retry level"
#define MAZEZAM_TEXT_AUDIO_PLAYBACK  "Audio playback"
#define MAZEZAM_TEXT_QUIT            "Quit"
#define MAZEZAM_TEXT_BACK            "Return"
#define MAZEZAM_TEXT_MAIN_MENU       "MazezaM"
#define MAZEZAM_TEXT_CONTINUE        "Play from checkpoint"
#define MAZEZAM_TEXT_PLAY_GAME       "Play game"
#define MAZEZAM_TEXT_PLAY_NEW_GAME   "Play new game"

#define MAZEZAM_START_LIVES         3 /* how many lives at game start */
#define MAZEZAM_FIRST_CHECKPOINT    3 /* The level at the first checkpoint */
#define MAZEZAM_CHECKPOINT_INTERVAL 4 /* A checkpoint every _ levels */

#ifdef HAVE_LCD_COLOR
#define MAZEZAM_HEADING_COLOR      LCD_RGBPACK(255,255,  0) /* Yellow    */
#define MAZEZAM_BORDER_COLOR       LCD_RGBPACK(  0,  0,255) /* Blue      */
#define MAZEZAM_COLOR              LCD_RGBPACK(255,255,255) /* White     */
#define MAZEZAM_BG_COLOR           LCD_RGBPACK(  0,  0,  0) /* Black     */
#define MAZEZAM_WALL_COLOR         LCD_RGBPACK(100,100,100) /* Dark gray */
#define MAZEZAM_PLAYER_COLOR       LCD_RGBPACK(255,255,255) /* White     */
#define MAZEZAM_GATE_COLOR         LCD_RGBPACK(100,100,100) /* Dark gray */

/* the rows are coloured sequentially */
#define MAZEZAM_NUM_CHUNK_COLORS   8
static const unsigned chunk_colors[MAZEZAM_NUM_CHUNK_COLORS] = {
    LCD_RGBPACK(255,192, 32), /* Orange      */
    LCD_RGBPACK(255,  0,  0), /* Red         */
    LCD_RGBPACK(  0,255,  0), /* Green       */
    LCD_RGBPACK(  0,255,255), /* Cyan        */
    LCD_RGBPACK(255,175,175), /* Pink        */
    LCD_RGBPACK(255,255,  0), /* Yellow      */
    LCD_RGBPACK(  0,  0,255), /* Blue        */
    LCD_RGBPACK(255,  0,255), /* Magenta     */
};

#elif LCD_DEPTH > 1

#define MAZEZAM_HEADING_GRAY       LCD_BLACK
#define MAZEZAM_BORDER_GRAY        LCD_DARKGRAY
#define MAZEZAM_GRAY               LCD_BLACK
#define MAZEZAM_BG_GRAY            LCD_WHITE
#define MAZEZAM_WALL_GRAY          LCD_DARKGRAY
#define MAZEZAM_PLAYER_GRAY        LCD_BLACK
#define MAZEZAM_GATE_GRAY          LCD_BLACK
#define MAZEZAM_CHUNK_EDGE_GRAY    LCD_BLACK

#define MAZEZAM_NUM_CHUNK_GRAYS    2
static const unsigned chunk_gray[MAZEZAM_NUM_CHUNK_GRAYS] = {
    LCD_LIGHTGRAY,
    LCD_DARKGRAY,
};
/* darker version of the above */
static const unsigned chunk_gray_shade[MAZEZAM_NUM_CHUNK_GRAYS] = {
    LCD_DARKGRAY,
    LCD_BLACK,
};
#endif

#define MAZEZAM_DELAY_CHECKPOINT   HZ
#define MAZEZAM_DELAY_LIVES        HZ
#define MAZEZAM_DELAY_GAME_OVER     (3 * HZ) / 2

/* maximum height of a level */
#define MAZEZAM_MAX_LINES         11
/* maximum number of chunks on a line */
#define MAZEZAM_MAX_CHUNKS        5

/* A structure for storing level data in unparsed form */
struct mazezam_level {
    short height;                  /* the number of lines */
    short width;                   /* the width */
    short entrance;                /* the line on which the entrance lies */
    short exit;                    /* the line on which the exit lies */
    char *line[MAZEZAM_MAX_LINES]; /* the chunk data in string form */
};

/* The number of levels. */
#define MAZEZAM_NUM_LEVELS        10

/* The levels. In theory, they could be stored in a file so this data
 * structure should not be accessed outside parse_level()
 *
 * These levels are copyright (C) 2002 Malcolm Tyrrell. They're
 * probably covered by the GPL as they constitute part of the source
 * code of this plugin, but you may distibute them seperately with
 * other Free Software if you want. You can download them from:
 * http://webpages.dcu.ie/~tyrrelma/MazezaM.
 */
static const struct mazezam_level level_data[MAZEZAM_NUM_LEVELS] = {
    {2,7,0,0,{" $  $"," $  $$"}},
    {3,8,2,1,{"  $  $$$","  $ $ $"," $ $ $"}},
    {4,14,1,3,{" $$$$$ $$ $$","   $$  $$  $$","$$  $ $$ $$$",
                 "   $$$$$$$$  $"}},
    {6,7,4,2,{"   $"," $$$$"," $$$ $$"," $ $ $"," $ $$","$ $$"}},
    {6,13,0,0,{"   $$$$$","$ $$$$$  $$$"," $ $$$ $$$$",
                 "$ $  $$$$$$$"," $$$ $   $$","$ $ $ $$  $"}},
    {11,5,10,0,{"  $"," $ $$"," $$","$ $"," $ $"," $$$","$ $",
                 " $ $"," $ $","$ $$"," $"}},
    {7,16,0,6,{"      $$$$$$$","  $$$$ $$$$ $ $","$$ $$ $$$$$$ $ $",
                 "$      $ $"," $$$$$$$$$$$$$$"," $ $$    $ $$$",
                 "  $   $$$     $$"}},
    {4,15,2,0,{" $$$$  $$$$  $$"," $ $$ $$ $ $$"," $ $$ $$$$ $$",
                 " $ $$  $$$$  $"}},
    {7,9,6,2,{" $  $$$$"," $  $ $$"," $ $$$$ $","$ $$  $","  $   $$$",
         " $$$$$$","  $"}},
    {10,14,8,0,{"            $"," $$$$$$$$$$  $"," $$$       $$",
                 " $ $$$$$$$$ $"," $$$ $$$  $$$"," $$$   $  $$$",
                 " $ $$$$$$$ $$"," $ $ $    $$$"," $$$$$$$$$$$$",
                 ""}}
};

/* This data structure which holds information about the rows */
struct chunk_data {
    /* the number of chunks on a line */
    short l_num[MAZEZAM_MAX_LINES];
    /* the width of a chunk */
    short c_width[MAZEZAM_MAX_LINES][MAZEZAM_MAX_CHUNKS];
    /* the inset of a chunk */
    short c_inset[MAZEZAM_MAX_LINES][MAZEZAM_MAX_CHUNKS];
};

/* Parsed level data */
struct level_info {
  short width;
  short height;
  short entrance;
  short exit;
  struct chunk_data cd;
};

/* The state variable used to hold the state of the plugin */
static enum {
    STATE_QUIT,          /* The player wants to quit */
    STATE_USB_CONNECTED, /* A USB cable has been inserted */
    STATE_PARSE_ERROR,   /* There's a parse error in the levels */
    STATE_WELLDONE,      /* The player has finished the game */

    STATE_IN_APPLICATION,

    STATE_MAIN_MENU      /* The player is at the main menu */
      = STATE_IN_APPLICATION,
    STATE_GAME_OVER,      /* The player is out of lives */

    STATE_IN_GAME,

    STATE_COMPLETED      /* A level has been completed */
      = STATE_IN_GAME,
      
    STATE_FAILED,        /* The player wants to retry the level */
    STATE_GAME_MENU,     /* The player wan't to access the in-game menu */
 
    STATE_IN_LEVEL,
} state;

/* The various constants needed for configuration files.
 * See apps/plugins/lib/configfile.*
 */
#define MAZEZAM_CONFIG_FILENAME    "mazezam.data"
#define MAZEZAM_CONFIG_NUM_ITEMS   1
#define MAZEZAM_CONFIG_VERSION     0
#define MAZEZAM_CONFIG_MINVERSION  0
#define MAZEZAM_CONFIG_LEVELS_NAME "restart_level"

/* A structure containing the data that is written to
 * the configuration file
 */
struct resume_data {
  int level; /* level at which to restart the game */
};

#if LCD_DEPTH > 1
/* Store the display settings so they are reintroduced during menus */
static struct {
  fb_data* backdrop;
  unsigned foreground;
  unsigned background;
} lcd_settings;
#endif

/*****************************************************************************
* Store the LCD settings
******************************************************************************/
static void store_lcd_settings(void) 
{
    /* Store the old settings */
#if LCD_DEPTH > 1
    lcd_settings.backdrop = rb->lcd_get_backdrop();
    lcd_settings.foreground = rb->lcd_get_foreground();
    lcd_settings.background = rb->lcd_get_background();
#endif
}

/*****************************************************************************
* Restore the LCD settings to their defaults
******************************************************************************/
static void restore_lcd_settings(void) {
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

    /* Restore the old settings */
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(lcd_settings.foreground);
    rb->lcd_set_background(lcd_settings.background);
    rb->lcd_set_backdrop(lcd_settings.backdrop);
#endif
}

/*****************************************************************************
* Adjust the LCD settings to suit MazezaM levels
******************************************************************************/
static void plugin_lcd_settings(void) {
    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

    /* Set the new settings */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(MAZEZAM_BG_COLOR);
    rb->lcd_set_backdrop(NULL);
#elif LCD_DEPTH > 1
    rb->lcd_set_background(MAZEZAM_BG_GRAY);
    rb->lcd_set_backdrop(NULL);
#endif
}

/*****************************************************************************
* Parse the level data from the level_data structure. This could be
* replaced by a file read. Returns true if the level parsed correctly.
******************************************************************************/
static bool parse_level(short level, struct level_info* li)
{
    int i,j;
    char c,clast;

    li->width    = level_data[level].width;
    li->height   = level_data[level].height;
    li->entrance = level_data[level].entrance;
    li->exit     = level_data[level].exit;

    /* for each line in the level */
    for (i = 0; i<level_data[level].height; i++) {
        if (level_data[level].line[i] == NULL)
            return false;
        else {
            j = 0;
            li->cd.l_num[i] = 0;
            clast = ' '; /* the character we last considered */
            while ((c = level_data[level].line[i][j]) != '\0') {
                if (c != ' ') {
                    if (clast == ' ') {
                        li->cd.l_num[i] += 1;
                        if (li->cd.l_num[i] > MAZEZAM_MAX_CHUNKS)
                            return false;
                        li->cd.c_inset[i][li->cd.l_num[i] - 1] = j;
                        li->cd.c_width[i][li->cd.l_num[i] - 1] = 1;
                    }
                    else
                        li->cd.c_width[i][li->cd.l_num[i] - 1] += 1;
                }
                clast = c;
                j++;
            }
        }
    }
    return true;
}

/*****************************************************************************
* Draw the walls of a level
******************************************************************************/
static void draw_walls(
        short size,
        short xOff,
        short yOff,
        short width,
        short height,
        short entrance,
        short exit)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(MAZEZAM_WALL_COLOR);
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(MAZEZAM_WALL_GRAY);
#endif
    /* draw the upper wall */
    rb->lcd_fillrect(0,0,xOff,yOff+(size*entrance));
    rb->lcd_fillrect(xOff,0,size*width,yOff);
    rb->lcd_fillrect(xOff+(size*width),0,LCD_WIDTH-xOff-(size*width),
                     yOff+(size*exit));

    /* draw the lower wall */
    rb->lcd_fillrect(0,yOff+(size*entrance)+size,xOff,
                     LCD_HEIGHT-yOff-(size*entrance)-size);
    rb->lcd_fillrect(xOff,yOff+(size*height),size*width,
                     LCD_HEIGHT-yOff-(size*height));
    /* Note: the exit is made one pixel thinner than necessary as a visual
     * clue that chunks cannot be pushed into it
     */
    rb->lcd_fillrect(xOff+(size*width),yOff+(size*exit)+size-1,
                     LCD_WIDTH-xOff+(size*width),
                     LCD_HEIGHT-yOff-(size*exit)-size+1);
}

/*****************************************************************************
* Draw chunk row i
******************************************************************************/
static void draw_row(
        short size,
        short xOff,
        short yOff,
        short width,
        short i, /* the row number */
        struct chunk_data *cd, /* the data about the chunks */
        short *shift /* an array of the horizontal offset of the lines */
)
{
    /* The assignment below is just a hack to make supress a warning on
     * non color targets */
    short j = width;
#ifndef HAVE_LCD_COLOR
    /* We #def these out to supress a compiler warning */
    short k;
#if LCD_DEPTH <= 1
    short l;
#endif
#endif
#ifdef HAVE_LCD_COLOR
    /* adding width to i should have a fixed, but randomising effect on
     * the choice of the colours of the top line of chunks
     */
    rb->lcd_set_foreground(chunk_colors[(i+width) % 
                           MAZEZAM_NUM_CHUNK_COLORS]);
#endif
    for (j = 0; j<cd->l_num[i]; j++) {
#ifdef HAVE_LCD_COLOR
        rb->lcd_fillrect(xOff+size*shift[i]+size*cd->c_inset[i][j],
                         yOff+size*i, cd->c_width[i][j]*size,size);
#elif LCD_DEPTH > 1
        rb->lcd_set_foreground(MAZEZAM_CHUNK_EDGE_GRAY);
        rb->lcd_drawrect(xOff+size*shift[i]+size*cd->c_inset[i][j],
                         yOff+size*i, cd->c_width[i][j]*size,size);

        /* draw shade */
        rb->lcd_set_foreground(chunk_gray_shade[(i+width) %
                               MAZEZAM_NUM_CHUNK_GRAYS]);
        rb->lcd_hline(xOff+size*shift[i]+size*cd->c_inset[i][j]+1,
                      xOff+size*shift[i]+size*cd->c_inset[i][j]+
                                                   cd->c_width[i][j]*size-3,
                      yOff+size*i+size-2);
        rb->lcd_vline(xOff+size*shift[i]+size*cd->c_inset[i][j]+
                                                   cd->c_width[i][j]*size-2,
                      yOff+size*i,
                      yOff+size*i+size-2);

        /* draw fill */
        rb->lcd_set_foreground(chunk_gray[(i+width) %
                               MAZEZAM_NUM_CHUNK_GRAYS]);
        for (k = yOff+size*i+2; k <  yOff+size*i+size-2; k += 2)
            rb->lcd_hline(xOff+size*shift[i]+size*cd->c_inset[i][j]+2,
                          xOff+size*shift[i]+size*cd->c_inset[i][j]+
                                                cd->c_width[i][j]*size-3,k);
#else
        rb->lcd_drawrect(xOff+size*shift[i]+size*cd->c_inset[i][j],
                         yOff+size*i, cd->c_width[i][j]*size,size);
        for (k = xOff+size*shift[i]+size*cd->c_inset[i][j]+2;
             k < xOff+size*shift[i]+size*cd->c_inset[i][j]+
                                                     cd->c_width[i][j]*size;
             k += 2 + (i & 1))
            for (l = yOff+size*i+2; l <  yOff+size*i+size; l += 2 + (i & 1))
                 rb->lcd_drawpixel(k, l);
#endif
    }
}

/*****************************************************************************
* Draw the player
******************************************************************************/
static void draw_player(
        short size,
        short xOff,
        short yOff,
        short x,
        short y)
{
    /* For drawing the player, taken from the sokoban plugin */
    short max = size - 1;
    short middle = max / 2;
    short ldelta = (middle + 1) / 2;

    /* draw the player (mostly copied from the sokoban plugin) */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(MAZEZAM_PLAYER_COLOR);
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(MAZEZAM_PLAYER_GRAY);
#endif
    rb->lcd_hline(xOff+size*x, xOff+size*x+max, yOff+size*y+middle);
    rb->lcd_vline(xOff+size*x+middle, yOff+size*y, yOff+size*y+max-ldelta);
    rb->lcd_drawline(xOff+size*x+middle, yOff+size*y+max-ldelta,
                     xOff+size*x+middle-ldelta, yOff+size*y+max);
    rb->lcd_drawline(xOff+size*x+middle, yOff+size*y+max-ldelta,
                     xOff+size*x+middle+ldelta, yOff+size*y+max);
}

/*****************************************************************************
* Draw the gate
******************************************************************************/
static void draw_gate(
        short size,
        short xOff,
        short yOff,
        short entrance)
{
    short third = size / 3;
    short twothirds = (2 * size) / 3;
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(MAZEZAM_GATE_COLOR);
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(MAZEZAM_GATE_GRAY);
#endif
    rb->lcd_hline(xOff-size,xOff-1,yOff+entrance*size+third);
    rb->lcd_hline(xOff-size,xOff-1,yOff+entrance*size+twothirds);
    rb->lcd_vline(xOff-size+third,yOff+entrance*size,
                  yOff+entrance*size+size-1);
    rb->lcd_vline(xOff-size+twothirds,yOff+entrance*size,
                  yOff+entrance*size+size-1);
}

/*****************************************************************************
* Draw the level
******************************************************************************/
static void draw_level(
        struct level_info* li,
        short *shift, /* an array of the horizontal offset of the lines */
        short x, /* player's x and y coords */
        short y)
{
    /* First we calculate the draw info */
    /* The number of pixels the side of a square should be */
    short size = (LCD_WIDTH/(li->width+2)) < (LCD_HEIGHT/li->height) ?
                 (LCD_WIDTH/(li->width+2)) : (LCD_HEIGHT/li->height);
    /* The x and y position (in pixels) of the top left corner of the
     * level
     */
    short xOff = (LCD_WIDTH - (size*li->width))/2;
    short yOff = (LCD_HEIGHT - (size*li->height))/2;
    short i;
    
    rb->lcd_clear_display();

    draw_walls(size,xOff,yOff,li->width, li->height, li->entrance, li->exit);

    /* draw the chunks */
    for (i = 0; i<li->height; i++) {
        draw_row(size,xOff,yOff,li->width,i,&(li->cd),shift);
    }

    draw_player(size,xOff,yOff,x,y);

    /* if the player has moved into the level, draw the gate */
    if (x >= 0)
        draw_gate(size,xOff,yOff,li->entrance);
}

/*****************************************************************************
* Manage the congratulations screen
******************************************************************************/
static void welldone_screen(void)
{
    int start_selection = 0;

    MENUITEM_STRINGLIST(menu,MAZEZAM_TEXT_WELLDONE_TITLE,NULL,
                          MAZEZAM_TEXT_WELLDONE_OPTION);

    switch(rb->do_menu(&menu, &start_selection, NULL, true)){
        case MENU_ATTACHED_USB:
            state = STATE_USB_CONNECTED;
            break;
    }
}

/*****************************************************************************
* Manage the playing of a level
******************************************************************************/
static void level_loop(struct level_info* li, short* shift, short *x, short *y)
{
    int i;
    int button;
    bool blocked; /* is there a chunk in the way of the player? */

    while (state >= STATE_IN_LEVEL) {
        draw_level(li, shift, *x, *y);
        rb->lcd_update();
        button = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts, 2);
        blocked = false;

        switch (button) {
            case MAZEZAM_UP:
            case MAZEZAM_UP_REPEAT:
                if ((*y > 0) && (*x >= 0) && (*x < li->width)) {
                    for (i = 0; i < li->cd.l_num[*y-1]; i++)
                        blocked = blocked ||
                                  ((*x>=shift[*y-1]+li->cd.c_inset[*y-1][i]) &&
                                   (*x<shift[*y-1]+li->cd.c_inset[*y-1][i]+
                                                   li->cd.c_width[*y-1][i]));
                    if (!blocked) *y -= 1;
                }
                break;



            case MAZEZAM_DOWN:
            case MAZEZAM_DOWN_REPEAT:
                if ((*y < li->height-1) && (*x >= 0) && (*x < li->width)) {
                    for (i = 0; i < li->cd.l_num[*y+1]; i++)
                        blocked = blocked ||
                                  ((*x>=shift[*y+1]+li->cd.c_inset[*y+1][i]) &&
                                   (*x<shift[*y+1]+li->cd.c_inset[*y+1][i]+
                                                   li->cd.c_width[*y+1][i]));
                    if (!blocked) *y += 1;
                }
                break;

            case MAZEZAM_LEFT:
            case MAZEZAM_LEFT_REPEAT:
                if (*x > 0) {
                    for (i = 0; i < li->cd.l_num[*y]; i++)
                        blocked = blocked ||
                                  (*x == shift[*y]+li->cd.c_inset[*y][i]+
                                                   li->cd.c_width[*y][i]);
                    if (!blocked) *x -= 1;
                    else if (shift[*y] + li->cd.c_inset[*y][0] > 0) {
                        *x -= 1;
                        shift[*y] -= 1;
                    }
                }
                break;

            case MAZEZAM_RIGHT:
            case MAZEZAM_RIGHT_REPEAT:
                if (*x < li->width-1) {
                    for (i = 0; i < li->cd.l_num[*y]; i++)
                        blocked = blocked ||
                                    (*x+1 == shift[*y]+li->cd.c_inset[*y][i]);
                    if (!blocked) *x += 1;
                    else if (shift[*y] 
                              + li->cd.c_inset[*y][li->cd.l_num[*y]-1]
                              + li->cd.c_width[*y][li->cd.l_num[*y]-1]
                                 < li->width) {
                        *x += 1;
                        shift[*y] += 1;
                    }
                }
                else if (*x == li->width) state = STATE_COMPLETED;
                else if (*y == li->exit) *x += 1;
                break;

            case MAZEZAM_MENU:
                state = STATE_GAME_MENU;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    state = STATE_USB_CONNECTED;
                break;
        }
    }
}

/*****************************************************************************
* Manage the in game menu
******************************************************************************/
static void in_game_menu(void)
{
    /* The initial option is retry level */
    int start_selection = 1;

    MENUITEM_STRINGLIST(menu,MAZEZAM_TEXT_MAZEZAM_MENU, NULL,
                         MAZEZAM_TEXT_BACK,
                         MAZEZAM_TEXT_RETRY_LEVEL,
                         MAZEZAM_TEXT_AUDIO_PLAYBACK,
                         MAZEZAM_TEXT_QUIT);
    
    /* Don't show the status bar */
    switch(rb->do_menu(&menu, &start_selection, NULL, false)){
        case 1: /* retry */
            state = STATE_FAILED;
            break;

        case 2: /* Audio playback */
            playback_control(NULL);
            state = STATE_IN_LEVEL;
            break;

        case 3: /* quit */
            state = STATE_QUIT;
            break;

        case MENU_ATTACHED_USB:
            state = STATE_USB_CONNECTED;
            break;

        default: /* Back */
            state = STATE_IN_LEVEL;
            break;
    }
}

/*****************************************************************************
* Is the level a checkpoint
******************************************************************************/
static bool at_checkpoint(int level) 
{
    if (level <= MAZEZAM_FIRST_CHECKPOINT) 
        return level == MAZEZAM_FIRST_CHECKPOINT;
    else {
        level = level - MAZEZAM_FIRST_CHECKPOINT;
        return level % MAZEZAM_CHECKPOINT_INTERVAL == 0;
    }
}

/*****************************************************************************
* Set up and play a level
* new_level should be true if this is the first time we've encountered
* this level
******************************************************************************/
static void play_level(short level, short lives, bool new_level)
{
    struct level_info li;
    short shift[MAZEZAM_MAX_LINES]; /* amount each line has been shifted */
    short x,y;
    int i;

    state = STATE_IN_LEVEL;

    if (!(parse_level(level,&li)))
        state =  STATE_PARSE_ERROR;

    for (i = 0; i < li.height; i++)
        shift[i] = 0;

    x = -1;
    y = li.entrance;

    plugin_lcd_settings();
    rb->lcd_clear_display();

    draw_level(&li, shift, x, y);

    /* If we've just reached a checkpoint, then alert the player */
    if (new_level && at_checkpoint(level)) {
        rb->splash(MAZEZAM_DELAY_CHECKPOINT, MAZEZAM_TEXT_CHECKPOINT);
        /* Clear the splash */
        draw_level(&li, shift, x, y);
    }

#ifdef HAVE_REMOTE_LCD
    /* Splash text seems to use the remote display by
     * default. I suppose I better keep it tidy!
     */
    rb->lcd_remote_clear_display();
#endif
    rb->splashf(MAZEZAM_DELAY_LIVES, MAZEZAM_TEXT_LIVES,
               level+1, lives);

    /* ensure keys pressed during the splash screen are ignored */
    rb->button_clear_queue();

    /* this little loop just ensures we return to the game if the player
     * doesn't perform an interesting action during the in game menu */
    while (state >= STATE_IN_LEVEL) {
        level_loop(&li, shift, &x, &y);

        if (state == STATE_GAME_MENU) {
            restore_lcd_settings();
            in_game_menu();
            plugin_lcd_settings();
        }
    }
    restore_lcd_settings();
}

/*****************************************************************************
* Update the resume data based on the level reached
******************************************************************************/
static void update_resume_data(struct resume_data *r, int level)
{
    if (at_checkpoint(level))
        r->level = level;
}

/*****************************************************************************
* The loop which manages a full game of MazezaM.
******************************************************************************/
static void game_loop(struct resume_data *r)
{
    int level = r->level;
    int lives = MAZEZAM_START_LIVES;
    /* We want to know when a player reaches a level for the first time,
     * so we keep a second copy of the level. */
    int old_level = level;

    state = STATE_IN_GAME;

    while (state >= STATE_IN_GAME)
    {
        play_level(level, lives, old_level < level);
        old_level = level;

        switch (state) {
            case STATE_COMPLETED:
                level += 1;
                if (level == MAZEZAM_NUM_LEVELS)
                    state = STATE_WELLDONE;
                break;

            case STATE_FAILED:
                lives -= 1;
                if (lives == 0)
                    state = STATE_GAME_OVER;
                break;

            default:
                break;
        }

        update_resume_data(r,level);
    }

    switch (state) {
        case STATE_GAME_OVER:
#ifdef HAVE_REMOTE_LCD
            /* Splash text seems to use the remote display by
             * default. I suppose I better keep it tidy!
             */
            rb->lcd_remote_clear_display();
#endif
            rb->splash(MAZEZAM_DELAY_GAME_OVER, MAZEZAM_TEXT_GAME_OVER);
            break;

        case STATE_WELLDONE:
            welldone_screen();
            break;

        default:
            break;
    }
}

/*****************************************************************************
* Load the resume data from the config file. The data is
* stored in both r and old.
******************************************************************************/
static void resume_load_data (struct resume_data *r, struct resume_data *old)
{
    struct configdata config[] = {
        {TYPE_INT,0,MAZEZAM_NUM_LEVELS-1, { .int_p = &(r->level) },
         MAZEZAM_CONFIG_LEVELS_NAME,NULL}
    };

    if (configfile_load(MAZEZAM_CONFIG_FILENAME,config,
                         MAZEZAM_CONFIG_NUM_ITEMS, MAZEZAM_CONFIG_VERSION) < 0)
        r->level = 0;
    /* an extra precaution */
    else if ((r->level < 0) || (MAZEZAM_NUM_LEVELS <= r->level))
        r->level = 0;

    old->level = r->level;
}

/*****************************************************************************
* Save the resume data in the config file, but only if necessary
******************************************************************************/
static void resume_save_data (struct resume_data *r, struct resume_data *old)
{
    struct configdata config[] = {
        {TYPE_INT,0,MAZEZAM_NUM_LEVELS-1, {.int_p = &(r->level) },
         MAZEZAM_CONFIG_LEVELS_NAME,NULL}
    };

    /* To reduce disk usage, only write the file if the resume data has
     * changed.
     */
    if (old->level != r->level)
        configfile_save(MAZEZAM_CONFIG_FILENAME,config,
                          MAZEZAM_CONFIG_NUM_ITEMS, MAZEZAM_CONFIG_MINVERSION);
}

/*****************************************************************************
* Offer a main menu with no continue option 
******************************************************************************/
static int main_menu_without_continue(int* start_selection) 
{
    MENUITEM_STRINGLIST(menu,MAZEZAM_TEXT_MAIN_MENU,NULL,
                          MAZEZAM_TEXT_PLAY_GAME,
                          MAZEZAM_TEXT_QUIT);
    return rb->do_menu(&menu, start_selection, NULL, false);
}

/*****************************************************************************
* Offer a main menu with a continue option
******************************************************************************/
static int main_menu_with_continue(int* start_selection)
{
    MENUITEM_STRINGLIST(menu,MAZEZAM_TEXT_MAIN_MENU,NULL,
                          MAZEZAM_TEXT_CONTINUE,
                          MAZEZAM_TEXT_PLAY_NEW_GAME,
                          MAZEZAM_TEXT_QUIT);
    return rb->do_menu(&menu, start_selection, NULL, false);
}

/*****************************************************************************
* Manages the main menu 
******************************************************************************/
static void main_menu(void)
{
    /* The initial option is "play game" */
    int start_selection = 0;
    int choice = 0;
    struct resume_data r_data, old_data;

    /* Load data */
    resume_load_data(&r_data, &old_data);

    while (state >= STATE_IN_APPLICATION) {
        if (r_data.level == 0)
            choice = main_menu_without_continue(&start_selection);
        else
            choice = main_menu_with_continue(&start_selection);

        switch(choice) {
            case 0: /* Continue */
                state = STATE_IN_GAME;
                game_loop(&r_data);
                break;

            case 1: /* Quit or Play new game */
                if (r_data.level == 0)
                    state = STATE_QUIT;
                else { /* Play new game */
                    r_data.level = 0;
                    state = STATE_IN_GAME;
                    game_loop(&r_data);
                }
                break;

            case MENU_ATTACHED_USB:
                state = STATE_USB_CONNECTED;
                break;

            default: /* Quit */
                state = STATE_QUIT;
                break;
        }
    }

    /* I'm not sure if it's appropriate to write to disk on USB events.
     * Currently, I do so.
     */
    resume_save_data(&r_data, &old_data);
}

/*****************************************************************************
* Plugin entry point 
******************************************************************************/
enum plugin_status plugin_start(const void* parameter)
{
    enum plugin_status plugin_state;

    /* Usual plugin stuff */
    (void)parameter;

    store_lcd_settings();

    state = STATE_MAIN_MENU;
    main_menu();   

    switch (state) {
        case STATE_USB_CONNECTED:
            plugin_state = PLUGIN_USB_CONNECTED;
            break;

        case STATE_PARSE_ERROR:
            plugin_state = PLUGIN_ERROR;
            break;

        default:
            plugin_state = PLUGIN_OK;
            break;
    }

    return plugin_state;
}
