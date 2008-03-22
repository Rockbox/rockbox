/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * ### line of auto-generated stuff I don't understand ###
 *
 * Copyright (C) 2006 Malcolm Tyrrell
 *
 * MazezaM - a Rockbox version of my ZX Spectrum game from 2002
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "configfile.h"
#include "helper.h"

/* Include standard plugin macro */
PLUGIN_HEADER

static struct plugin_api* rb;

MEM_FUNCTION_WRAPPERS(rb);

#if CONFIG_KEYPAD == RECORDER_PAD
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_PLAY

#define MAZEZAM_RETRY               BUTTON_F1
#define MAZEZAM_RETRY_KEYNAME       "[F1]"
#define MAZEZAM_QUIT                BUTTON_OFF
#define MAZEZAM_QUIT_KEYNAME        "[OFF]"

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_F1
#define MAZEZAM_RETRY_KEYNAME       "[F1]"
#define MAZEZAM_QUIT                BUTTON_OFF
#define MAZEZAM_QUIT_KEYNAME        "[OFF]"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_RIGHT

#define MAZEZAM_RETRY               BUTTON_MENU
#define MAZEZAM_RETRY_KEYNAME       "[MENU]"
#define MAZEZAM_QUIT                BUTTON_OFF
#define MAZEZAM_QUIT_KEYNAME        "[OFF]"

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_REC
#define MAZEZAM_RETRY_KEYNAME       "[REC]"
#define MAZEZAM_QUIT                BUTTON_POWER
#define MAZEZAM_QUIT_KEYNAME        "[POWER]"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MAZEZAM_UP                  BUTTON_MENU
#define MAZEZAM_DOWN                BUTTON_PLAY
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_SELECT
#define MAZEZAM_RETRY_KEYNAME       "[SELECT]"
#define MAZEZAM_QUIT                (BUTTON_SELECT | BUTTON_REPEAT)
#define MAZEZAM_QUIT_KEYNAME        "[SELECT] (held)"

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_ON
#define MAZEZAM_RETRY_KEYNAME       "[ON]"
#define MAZEZAM_QUIT                BUTTON_OFF
#define MAZEZAM_QUIT_KEYNAME        "[OFF]"

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_A
#define MAZEZAM_RETRY_KEYNAME       "[A]"
#define MAZEZAM_QUIT                BUTTON_POWER
#define MAZEZAM_QUIT_KEYNAME        "[POWER]"

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_REC
#define MAZEZAM_RETRY_KEYNAME       "[REC]"
#define MAZEZAM_QUIT                BUTTON_POWER
#define MAZEZAM_QUIT_KEYNAME        "[POWER]"

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define MAZEZAM_UP                  BUTTON_SCROLL_UP
#define MAZEZAM_DOWN                BUTTON_SCROLL_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_PLAY

#define MAZEZAM_RETRY               BUTTON_PLAY
#define MAZEZAM_RETRY_KEYNAME       "[PLAY]"
#define MAZEZAM_QUIT                BUTTON_POWER
#define MAZEZAM_QUIT_KEYNAME        "[POWER]"

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_PLAY
#define MAZEZAM_RETRY_KEYNAME       "[PLAY]"
#define MAZEZAM_QUIT                BUTTON_BACK
#define MAZEZAM_QUIT_KEYNAME        "[BACK]"

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_DISPLAY
#define MAZEZAM_RETRY_KEYNAME       "[DISPLAY]"
#define MAZEZAM_QUIT                BUTTON_POWER
#define MAZEZAM_QUIT_KEYNAME        "[POWER]"

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define MAZEZAM_UP                  BUTTON_RC_VOL_UP
#define MAZEZAM_DOWN                BUTTON_RC_VOL_DOWN
#define MAZEZAM_LEFT                BUTTON_RC_REW
#define MAZEZAM_RIGHT               BUTTON_RC_FF
#define MAZEZAM_SELECT              BUTTON_RC_PLAY

#define MAZEZAM_RETRY               BUTTON_RC_MODE
#define MAZEZAM_RETRY_KEYNAME       "[MODE]"
#define MAZEZAM_QUIT                BUTTON_RC_REC
#define MAZEZAM_QUIT_KEYNAME        "[REC]"

#elif (CONFIG_KEYPAD == COWOND2_PAD)
#define MAZEZAM_UP                  BUTTON_UP
#define MAZEZAM_DOWN                BUTTON_DOWN
#define MAZEZAM_LEFT                BUTTON_LEFT
#define MAZEZAM_RIGHT               BUTTON_RIGHT
#define MAZEZAM_SELECT              BUTTON_SELECT

#define MAZEZAM_RETRY               BUTTON_SELECT
#define MAZEZAM_RETRY_KEYNAME       "[PLAY]"
#define MAZEZAM_QUIT                BUTTON_POWER
#define MAZEZAM_QUIT_KEYNAME        "[POWER]"

#else
#error No keymap defined!
#endif

/* The gap for the border around the heading in text pages. In fact, 2 is
 * really the only acceptable value.
 */
#define MAZEZAM_MENU_BORDER        2
#define MAZEZAM_EXTRA_LIFE         2 /* get an extra life every _ levels */
#define MAZEZAM_START_LIVES        3 /* how many lives at game start */

#ifdef HAVE_LCD_COLOR
#define MAZEZAM_HEADING_COLOR      LCD_RGBPACK(255,255,  0) /* Yellow      */
#define MAZEZAM_BORDER_COLOR       LCD_RGBPACK(  0,  0,255) /* Blue        */
#define MAZEZAM_TEXT_COLOR         LCD_RGBPACK(255,255,255) /* White       */
#define MAZEZAM_BG_COLOR           LCD_RGBPACK(  0,  0,  0) /* Black       */
#define MAZEZAM_WALL_COLOR         LCD_RGBPACK(100,100,100) /* Dark gray   */
#define MAZEZAM_PLAYER_COLOR       LCD_RGBPACK(255,255,255) /* White       */
#define MAZEZAM_GATE_COLOR         LCD_RGBPACK(100,100,100) /* Dark gray   */

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
#define MAZEZAM_TEXT_GRAY          LCD_BLACK
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

#define MAZEZAM_GAMEOVER_TEXT      "Game Over"
#define MAZEZAM_GAMEOVER_DELAY     (3 * HZ) / 2
#define MAZEZAM_LEVEL_LIVES_TEXT   "Level %d, Lives %d"
#define MAZEZAM_LEVEL_LIVES_DELAY  HZ
#define MAZEZAM_WELLDONE_DELAY     4 * HZ

/* The maximum number of lines that a text page can display.
 * This must be 4 or less if the Archos recorder is to be
 * supported.
 */
#define MAZEZAM_TEXT_MAXLINES      4

/* A structure for holding text pages */
struct textpage {
    /* Ensure 1 < num_lines <= MAZEZAM_TEXT_MAXLINES */
    short num_lines;
    char *line[MAZEZAM_TEXT_MAXLINES]; /* text of lines */
};

/* The text page for the welcome screen */
static const struct textpage title_page = {
    4,
    {"MazezaM", "play game", "instructions", "quit"}
};

/* The number of help screens */
#define MAZEZAM_NUM_HELP_PAGES   4

/* The instruction screens */
static const struct textpage help_page[] = {
    {4,{"Instructions","10 mazezams","bar your way","to freedom"}},
    {4,{"Instructions","Push the rows","left and right","to escape"}},
    {4,{"Instructions","Press " MAZEZAM_RETRY_KEYNAME " to","retry a level",
                                                              "(lose 1 life)"}},
    {4,{"Instructions","Press " MAZEZAM_QUIT_KEYNAME,"to quit","the game"}}
};

/* the text of the screen that asks for a quit confirmation */
static const struct textpage confirm_page = {
    4,
    {"Quit","Are you sure?","yes","no"}
};

/* the text of the screen at the end of the game */
static const struct textpage welldone_page = {
    3,
    {"Well Done","You have","escaped",""}
};

/* the text of the screen asking if the user wants to
 * resume or start a new game.
 */
static const struct textpage resume_page = {
    3,
    {"Checkpoint", "continue", "new game"}
};

/* maximum height of a level */
#define MAZEZAM_MAX_LINES         11
/* maximum number of chunks on a line */
#define MAZEZAM_MAX_CHUNKS        5

/* A structure for holding levels */
struct mazezam_level {
    short height;                  /* the number of lines */
    short width;                   /* the width */
    short entrance;                /* the line on which the entrance lies */
    short exit;                    /* the line on which the exit lies */
    char *line[MAZEZAM_MAX_LINES]; /* the chunk data in string form */
};

/* The number of levels. Note that the instruction screens reference this
 * number
 */
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
    {2,7,0,0,{" $  $"," $  $$",NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                  NULL}},
    {3,8,2,1,{"  $  $$$","  $ $ $"," $ $ $",NULL,NULL,NULL,NULL,NULL,NULL,
                 NULL,NULL}},
    {4,14,1,3,{" $$$$$ $$ $$","   $$  $$  $$","$$  $ $$ $$$",
                 "   $$$$$$$$  $",NULL,NULL,NULL,NULL,NULL,NULL,NULL}},
    {6,7,4,2,{"   $"," $$$$"," $$$ $$"," $ $ $"," $ $$","$ $$",
                 NULL,NULL,NULL,NULL,NULL}},
    {6,13,0,0,{"   $$$$$","$ $$$$$  $$$"," $ $$$ $$$$",
                 "$ $  $$$$$$$"," $$$ $   $$","$ $ $ $$  $",NULL,NULL,
                 NULL,NULL,NULL}},
    {11,5,10,0,{"  $"," $ $$"," $$","$ $"," $ $"," $$$","$ $",
                 " $ $"," $ $","$ $$"," $"}},
    {7,16,0,6,{"      $$$$$$$","  $$$$ $$$$ $ $","$$ $$ $$$$$$ $ $",
                 "$      $ $"," $$$$$$$$$$$$$$"," $ $$    $ $$$",
                 "  $   $$$     $$",NULL,NULL,NULL,NULL}},
    {4,15,2,0,{" $$$$  $$$$  $$"," $ $$ $$ $ $$"," $ $$ $$$$ $$",
                 " $ $$  $$$$  $",NULL,NULL,NULL,NULL,NULL,NULL,NULL}},
    {7,9,6,2,{" $  $$$$"," $  $ $$"," $ $$$$ $","$ $$  $","  $   $$$",
         " $$$$$$","  $",NULL,NULL,NULL,NULL}},
    {10,14,8,0,{"            $"," $$$$$$$$$$  $"," $$$       $$",
                 " $ $$$$$$$$ $"," $$$ $$$  $$$"," $$$   $  $$$",
                 " $ $$$$$$$ $$"," $ $ $    $$$"," $$$$$$$$$$$$",
                 "",NULL}}
};

/* This is the data structure the game uses for managing levels */
struct chunk_data {
    /* the number of chunks on a line */
    short l_num[MAZEZAM_MAX_LINES];
    /* the width of a chunk */
    short c_width[MAZEZAM_MAX_LINES][MAZEZAM_MAX_CHUNKS];
    /* the inset of a chunk */
    short c_inset[MAZEZAM_MAX_LINES][MAZEZAM_MAX_CHUNKS];
};

/* The state and exit code of the level loop */
enum level_state {
    LEVEL_STATE_LOOPING,
    LEVEL_STATE_COMPLETED,
    LEVEL_STATE_FAILED,
    LEVEL_STATE_QUIT,
    LEVEL_STATE_PARSE_ERROR,
    LEVEL_STATE_USB_CONNECTED,
};

/* The state and exit code of the text screens. I use the
 * same enum for all of them, even though there are some
 * differences.
 */
enum text_state {
    TEXT_STATE_LOOPING,
    TEXT_STATE_QUIT,
    TEXT_STATE_OKAY,
    TEXT_STATE_USB_CONNECTED,
    TEXT_STATE_PARSE_ERROR,
    TEXT_STATE_BACK,
};

/* The state and exit code of the game loop */
enum game_state {
    GAME_STATE_LOOPING,
    GAME_STATE_QUIT,
    GAME_STATE_OKAY,
    GAME_STATE_USB_CONNECTED,
    GAME_STATE_OVER,
    GAME_STATE_COMPLETED,
    GAME_STATE_PARSE_ERROR,
};

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

/* Display a screen of text. line[0] is the heading.
 * line[highlight] will be highlighted, unless highlight == 0
 */
static void display_text_page(struct textpage text, int highlight)
{
    int w[text.num_lines], h[text.num_lines];
    int hsum,i,vgap,vnext;

    rb->lcd_clear_display();

    /* find out how big the text is so we can determine the positioning */
    hsum = 0;
    for(i = 0; i < text.num_lines; i++) {
        rb->lcd_getstringsize(text.line[i], w+i, h+i);
        hsum += h[i];
    }

    vgap = (LCD_HEIGHT-hsum)/(text.num_lines+1);

    /* The Heading */

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(MAZEZAM_BORDER_COLOR);
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(MAZEZAM_BORDER_GRAY);
#endif
    rb->lcd_drawrect((LCD_WIDTH-w[0])/2-MAZEZAM_MENU_BORDER,
                     vgap-MAZEZAM_MENU_BORDER, w[0] + 2*MAZEZAM_MENU_BORDER,
                     h[0] + 2*MAZEZAM_MENU_BORDER);
    rb->lcd_drawrect((LCD_WIDTH-w[0])/2-MAZEZAM_MENU_BORDER*2,
                     vgap-MAZEZAM_MENU_BORDER*2, w[0] + 4*MAZEZAM_MENU_BORDER,
                     h[0] + 4*MAZEZAM_MENU_BORDER);
    rb->lcd_drawline(0,vgap + h[0]/2,(LCD_WIDTH-w[0])/2-MAZEZAM_MENU_BORDER*2,
                     vgap + h[0]/2);
    rb->lcd_drawline((LCD_WIDTH-w[0])/2+w[0]+MAZEZAM_MENU_BORDER*2,
                     vgap + h[0]/2,LCD_WIDTH-1,vgap + h[0]/2);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(MAZEZAM_HEADING_COLOR);
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(MAZEZAM_HEADING_GRAY);
#endif
    rb->lcd_putsxy((LCD_WIDTH-w[0])/2,vgap,text.line[0]);

    vnext = vgap*2 + h[0];

    /* The other lines */

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(MAZEZAM_TEXT_COLOR);
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(MAZEZAM_TEXT_GRAY);
#endif
    for (i = 1; i<text.num_lines; i++) {
        rb->lcd_putsxy((LCD_WIDTH-w[i])/2,vnext,text.line[i]);

        /* add underlining if i is the highlighted line */
        if (i == highlight) {
            rb->lcd_drawline((LCD_WIDTH-w[i])/2, vnext + h[i] + 1,
                    (LCD_WIDTH-w[i])/2 + w[i], vnext + h[i] + 1);
        }

        vnext += vgap + h[i];
    }

    rb->lcd_update();
}


/* Parse the level data from the level_data structure. This could be
 * replaced by a file read. Returns true if the level parsed correctly.
 */
static bool parse_level(short level, struct chunk_data *cd,
        short *width, short *height, short *entrance, short *exit)
{
    int i,j;
    char c,clast;

    *width    = level_data[level].width;
    *height   = level_data[level].height;
    *entrance = level_data[level].entrance;
    *exit     = level_data[level].exit;

    /* for each line in the level */
    for (i = 0; i<level_data[level].height; i++) {
        if (level_data[level].line[i] == NULL)
            return false;
        else {
            j = 0;
            cd->l_num[i] = 0;
            clast = ' '; /* the character we last considered */
            while ((c = level_data[level].line[i][j]) != '\0') {
                if (c != ' ') {
                    if (clast == ' ') {
                        cd->l_num[i] += 1;
                        if (cd->l_num[i] > MAZEZAM_MAX_CHUNKS)
                            return false;
                        cd->c_inset[i][cd->l_num[i] - 1] = j;
                        cd->c_width[i][cd->l_num[i] - 1] = 1;
                    }
                    else
                        cd->c_width[i][cd->l_num[i] - 1] += 1;
                }
                clast = c;
                j++;
            }
        }
    }
    return true;
}

/* Draw the level */
static void draw_level(
        struct chunk_data *cd, /* the data about the chunks */
        short *shift, /* an array of the horizontal offset of the lines */
        short width,
        short height,
        short entrance,
        short exit,
        short x, /* player's x and y coords */
        short y)
{
    /* The number of pixels the side of a square should be */
    short size = (LCD_WIDTH/(width+2)) < (LCD_HEIGHT/height) ?
                 (LCD_WIDTH/(width+2)) : (LCD_HEIGHT/height);
    /* The x and y position (in pixels) of the top left corner of the
     * level
     */
    short xOff = (LCD_WIDTH - (size*width))/2;
    short yOff = (LCD_HEIGHT - (size*height))/2;
    /* For drawing the player, taken from the sokoban plugin */
    short max = size - 1;
    short middle = max / 2;
    short ldelta = (middle + 1) / 2;
    short i,j;
    short third = size / 3;
    short twothirds = (2 * size) / 3;
#ifndef HAVE_LCD_COLOR
    /* We #def these out to supress a compiler warning */
    short k;
#if LCD_DEPTH <= 1
    short l;
#endif
#endif

    rb->lcd_clear_display();

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

    /* draw the chunks */
    for (i = 0; i<height; i++) {
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
            rb->lcd_drawline(xOff+size*shift[i]+size*cd->c_inset[i][j]+1,
                             yOff+size*i+size-2,
                             xOff+size*shift[i]+size*cd->c_inset[i][j]+
                                                       cd->c_width[i][j]*size-3,
                             yOff+size*i+size-2);
            rb->lcd_drawline(xOff+size*shift[i]+size*cd->c_inset[i][j]+
                                                       cd->c_width[i][j]*size-2,
                             yOff+size*i,
                             xOff+size*shift[i]+size*cd->c_inset[i][j]+
                                                       cd->c_width[i][j]*size-2,
                             yOff+size*i+size-2);

            /* draw fill */
            rb->lcd_set_foreground(chunk_gray[(i+width) %
                                   MAZEZAM_NUM_CHUNK_GRAYS]);
            for (k = yOff+size*i+2; k <  yOff+size*i+size-2; k += 2)
                rb->lcd_drawline(xOff+size*shift[i]+size*cd->c_inset[i][j]+2,k,
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

    /* draw the player (mostly copied from the sokoban plugin) */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(MAZEZAM_PLAYER_COLOR);
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(MAZEZAM_PLAYER_GRAY);
#endif
    rb->lcd_drawline(xOff+size*x, yOff+size*y+middle,
                     xOff+size*x+max, yOff+size*y+middle);
    rb->lcd_drawline(xOff+size*x+middle, yOff+size*y,
                     xOff+size*x+middle, yOff+size*y+max-ldelta);
    rb->lcd_drawline(xOff+size*x+middle, yOff+size*y+max-ldelta,
                     xOff+size*x+middle-ldelta, yOff+size*y+max);
    rb->lcd_drawline(xOff+size*x+middle, yOff+size*y+max-ldelta,
                     xOff+size*x+middle+ldelta, yOff+size*y+max);

    /* draw the gate, if the player has moved into the level */
    if (x >= 0) {
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_foreground(MAZEZAM_GATE_COLOR);
#elif LCD_DEPTH > 1
        rb->lcd_set_foreground(MAZEZAM_GATE_GRAY);
#endif
        rb->lcd_drawline(xOff-size,yOff+entrance*size+third,
                         xOff-1,yOff+entrance*size+third);
        rb->lcd_drawline(xOff-size,yOff+entrance*size+twothirds,
                         xOff-1,yOff+entrance*size+twothirds);
        rb->lcd_drawline(xOff-size+third,yOff+entrance*size,
                         xOff-size+third,yOff+entrance*size+size-1);
        rb->lcd_drawline(xOff-size+twothirds,yOff+entrance*size,
                         xOff-size+twothirds,yOff+entrance*size+size-1);
    }
}

/* Manage the congratulations screen */
static enum text_state welldone_screen(void)
{
    int button = BUTTON_NONE;
    enum text_state state = TEXT_STATE_LOOPING;

    display_text_page(welldone_page, 0);

    while (state == TEXT_STATE_LOOPING) {
        button = rb->button_get(true);

        switch (button) {
            case MAZEZAM_QUIT:
                state = TEXT_STATE_QUIT;
                break;

            case MAZEZAM_SELECT:
#if CONFIG_KEYPAD != ONDIO_PAD
            case MAZEZAM_RIGHT:
#endif
                state = TEXT_STATE_OKAY;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    state = TEXT_STATE_USB_CONNECTED;
                break;
        }
    }

    return state;
}

/* Manage the quit confimation screen */
static enum text_state quitconfirm_loop(void)
{
    int button = BUTTON_NONE;
    enum text_state state = TEXT_STATE_LOOPING;
    short select = 2;

    display_text_page(confirm_page, select + 1);

    /* Wait for a button release. This is useful when a repeated button
     * press is used for quit.
     */
    while ((rb->button_get(true) & BUTTON_REL) != BUTTON_REL);

    while (state == TEXT_STATE_LOOPING) {
        display_text_page(confirm_page, select + 1);

        button = rb->button_get(true);

        switch (button) {
            case MAZEZAM_QUIT:
                state = TEXT_STATE_QUIT;
                break;

            case MAZEZAM_UP:
            case MAZEZAM_DOWN:
                select = (2 - select) + 1;
                break;

            case MAZEZAM_SELECT:
#if CONFIG_KEYPAD != ONDIO_PAD
            case MAZEZAM_RIGHT:
#endif
                if (select == 1)
                    state = TEXT_STATE_QUIT;
                else
                    state = TEXT_STATE_OKAY;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    state = TEXT_STATE_USB_CONNECTED;
                break;
        }
    }

    return state;
}

/* Manage the playing of a level */
static enum level_state level_loop(short level, short lives)
{
    struct chunk_data cd;
    short shift[MAZEZAM_MAX_LINES]; /* amount each line has been shifted */
    short width;
    short height;
    short entrance;
    short exit;
    short i;
    short x,y;
    int button;
    enum level_state state = LEVEL_STATE_LOOPING;
    bool blocked; /* is there a chunk in the way of the player? */

    if (!(parse_level(level,&cd,&width,&height,&entrance,&exit)))
        return LEVEL_STATE_PARSE_ERROR;

    for (i = 0; i < height; i++)
        shift[i] = 0;

    x = -1;
    y = entrance;

    draw_level(&cd, shift, width, height, entrance, exit, x, y);

#ifdef HAVE_REMOTE_LCD
    /* Splash text seems to use the remote display by
     * default. I suppose I better keep it tidy!
     */
    rb->lcd_remote_clear_display();
#endif
    rb->splash(MAZEZAM_LEVEL_LIVES_DELAY, MAZEZAM_LEVEL_LIVES_TEXT,
               level+1, lives);

    /* ensure keys pressed during the splash screen are ignored */
    rb->button_clear_queue();

    while (state == LEVEL_STATE_LOOPING) {
        draw_level(&cd, shift, width, height, entrance, exit, x, y);
        rb->lcd_update();
        button = rb->button_get(true);
        blocked = false;

        switch (button) {
            case MAZEZAM_UP:
            case MAZEZAM_UP | BUTTON_REPEAT:
                if ((y > 0) && (x >= 0) && (x < width)) {
                    for (i = 0; i < cd.l_num[y-1]; i++)
                        blocked = blocked ||
                                  ((x>=shift[y-1]+cd.c_inset[y-1][i]) &&
                                   (x<shift[y-1]+cd.c_inset[y-1][i]+
                                                           cd.c_width[y-1][i]));
                    if (!blocked) y -= 1;
                }
                break;

            case MAZEZAM_DOWN:
            case MAZEZAM_DOWN | BUTTON_REPEAT:
                if ((y < height-1) && (x >= 0) && (x < width)) {
                    for (i = 0; i < cd.l_num[y+1]; i++)
                        blocked = blocked ||
                                  ((x>=shift[y+1]+cd.c_inset[y+1][i]) &&
                                   (x<shift[y+1]+cd.c_inset[y+1][i]+
                                                           cd.c_width[y+1][i]));
                    if (!blocked) y += 1;
                }
                break;

            case MAZEZAM_LEFT:
            case MAZEZAM_LEFT | BUTTON_REPEAT:
                if (x > 0) {
                    for (i = 0; i < cd.l_num[y]; i++)
                        blocked = blocked ||
                                  (x == shift[y]+cd.c_inset[y][i]+
                                                              cd.c_width[y][i]);
                    if (!blocked) x -= 1;
                    else if (shift[y] + cd.c_inset[y][0] > 0) {
                        x -= 1;
                        shift[y] -= 1;
                    }
                }
                break;

            case MAZEZAM_RIGHT:
            case MAZEZAM_RIGHT | BUTTON_REPEAT:
                if (x < width-1) {
                    for (i = 0; i < cd.l_num[y]; i++)
                        blocked = blocked || (x+1 == shift[y]+cd.c_inset[y][i]);
                    if (!blocked) x += 1;
                    else if (shift[y] + cd.c_inset[y][cd.l_num[y]-1] +
                                         cd.c_width[y][cd.l_num[y]-1] < width) {
                        x += 1;
                        shift[y] += 1;
                    }
                }
                else if (x == width) state = LEVEL_STATE_COMPLETED;
                else if (y == exit) x += 1;
                break;

            case MAZEZAM_RETRY:
                state = LEVEL_STATE_FAILED;
                break;

            case MAZEZAM_QUIT:
                switch (quitconfirm_loop()) {
                    case TEXT_STATE_QUIT:
                        state = LEVEL_STATE_QUIT;
                        break;

                    case TEXT_STATE_USB_CONNECTED:
                        state = LEVEL_STATE_USB_CONNECTED;
                        break;

                    default:
                        break;
                }
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    state = LEVEL_STATE_USB_CONNECTED;
                break;
        }
    }

    return state;
}

/* The loop which manages a full game of MazezaM */
static enum game_state game_loop(struct resume_data *r)
{
    enum game_state state = GAME_STATE_LOOPING;
    int level = r->level;
    int lives = MAZEZAM_START_LIVES;

    rb->lcd_clear_display();

    while (state == GAME_STATE_LOOPING)
    {
        switch (level_loop(level,lives)) {
            case LEVEL_STATE_COMPLETED:
                level += 1;
                if (!((level - r->level) % MAZEZAM_EXTRA_LIFE))
                    lives += 1;
                break;

            case LEVEL_STATE_QUIT:
                state = GAME_STATE_QUIT;
                break;

            case LEVEL_STATE_FAILED:
                lives -= 1;
                break;

            case LEVEL_STATE_PARSE_ERROR:
                state = GAME_STATE_PARSE_ERROR;
                break;

            case LEVEL_STATE_USB_CONNECTED:
                state = GAME_STATE_USB_CONNECTED;
                break;

            default:
                break;
        }
        if (lives == 0)
            state = GAME_STATE_OVER;
        else if (level == MAZEZAM_NUM_LEVELS)
            state = GAME_STATE_COMPLETED;
    }

    switch (state) {
        case GAME_STATE_OVER:
#ifdef HAVE_REMOTE_LCD
            /* Splash text seems to use the remote display by
             * default. I suppose I better keep it tidy!
             */
            rb->lcd_remote_clear_display();
#endif
            rb->splash(MAZEZAM_GAMEOVER_DELAY, MAZEZAM_GAMEOVER_TEXT);
            break;

        case GAME_STATE_COMPLETED:
            switch (welldone_screen()) {
                case TEXT_STATE_QUIT:
                    state = GAME_STATE_QUIT;
                    break;

                case TEXT_STATE_USB_CONNECTED:
                    state = GAME_STATE_USB_CONNECTED;
                    break;

                default:
                    state = GAME_STATE_OKAY;
                    break;
            }
            break;

        default:
            break;
    }

    /* This particular resume game logic is designed to make
     * players prove they can solve a level more than once
     */
    if (level > r->level + 1)
        r->level += 1;

    return state;
}

/* Manage the instruction screen */
static enum text_state instruction_loop(void)
{
    int button;
    enum text_state state = TEXT_STATE_LOOPING;
    int page = 0;

    while (state == TEXT_STATE_LOOPING) {
        display_text_page(help_page[page], 0);
        button = rb->button_get(true);

        switch (button) {
            case MAZEZAM_LEFT:
                page -= 1;
                break;

            case MAZEZAM_SELECT:
#if CONFIG_KEYPAD != ONDIO_PAD
            case MAZEZAM_RIGHT:
#endif
                page += 1;
                break;

            case MAZEZAM_QUIT:
                state = TEXT_STATE_QUIT;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    state = TEXT_STATE_USB_CONNECTED;
                break;

        }

        if ((page < 0) || (page >= MAZEZAM_NUM_HELP_PAGES))
            state = TEXT_STATE_OKAY;
    }

    return state;
}

/* Manage the text screen that offers the user the option of
 * resuming or starting a new game
 */
static enum text_state resume_game_loop (struct resume_data *r)
{
    int button = BUTTON_NONE;
    enum text_state state = TEXT_STATE_LOOPING;
    short select = 0;

    /* if the resume level is 0, don't bother asking */
    if (r->level == 0) return TEXT_STATE_OKAY;

    display_text_page(resume_page, select + 1);

    while (state == TEXT_STATE_LOOPING) {
        display_text_page(resume_page, select + 1);

        button = rb->button_get(true);

        switch (button) {
            case MAZEZAM_QUIT:
                state = TEXT_STATE_QUIT;
                break;

            case MAZEZAM_LEFT:
                state = TEXT_STATE_BACK;
                break;

            case MAZEZAM_UP:
            case MAZEZAM_DOWN:
                select = 1 - select;
                break;

            case MAZEZAM_SELECT:
#if CONFIG_KEYPAD != ONDIO_PAD
            case MAZEZAM_RIGHT:
#endif
                if (select == 1) {
                    /* The player wants to play a new game. I could ask
                     * for confirmation here, but the only penalty is
                     * playing through some already completed levels,
                     * so I don't think it's necessary
                     */
                    r->level = 0;
                }
                state = TEXT_STATE_OKAY;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    state = TEXT_STATE_USB_CONNECTED;
                break;
        }
    }

    return state;
}

/* Load the resume data from the config file. The data is
 * stored in both r and old.
 */
static void resume_load_data (struct resume_data *r, struct resume_data *old)
{
    struct configdata config[] = {
        {TYPE_INT,0,MAZEZAM_NUM_LEVELS-1,&(r->level),
         MAZEZAM_CONFIG_LEVELS_NAME,NULL,NULL}
    };

    if (configfile_load(MAZEZAM_CONFIG_FILENAME,config,MAZEZAM_CONFIG_NUM_ITEMS,
                        MAZEZAM_CONFIG_VERSION) < 0)
        r->level = 0;
    /* an extra precaution */
    else if ((r->level < 0) || (MAZEZAM_NUM_LEVELS <= r->level))
        r->level = 0;

    old->level = r->level;
}

/* Save the resume data in the config file, but only if necessary */
static void resume_save_data (struct resume_data *r, struct resume_data *old)
{
    struct configdata config[] = {
        {TYPE_INT,0,MAZEZAM_NUM_LEVELS-1,&(r->level),
         MAZEZAM_CONFIG_LEVELS_NAME,NULL,NULL}
    };

    /* To reduce disk usage, only write the file if the resume data has
     * changed.
     */
    if (old->level != r->level)
        configfile_save(MAZEZAM_CONFIG_FILENAME,config,MAZEZAM_CONFIG_NUM_ITEMS,
                        MAZEZAM_CONFIG_MINVERSION);
}

/* The loop which manages the welcome screen and menu */
static enum text_state welcome_loop(void)
{
    int button;
    short select = 0;
    enum text_state state = TEXT_STATE_LOOPING;
    struct resume_data r_data, old_data;

    /* Load data */
    resume_load_data(&r_data, &old_data);

    while (state == TEXT_STATE_LOOPING) {
        display_text_page(title_page, select + 1);
        button = rb->button_get(true);

        switch (button) {
            case MAZEZAM_QUIT:
                state = TEXT_STATE_QUIT;
                break;

            case MAZEZAM_UP:
                select = (select + (title_page.num_lines - 2)) %
                         (title_page.num_lines - 1);
                break;

            case MAZEZAM_DOWN:
                select = (select + 1) % (title_page.num_lines - 1);
                break;

            case MAZEZAM_SELECT:
#if CONFIG_KEYPAD != ONDIO_PAD
            case MAZEZAM_RIGHT:
#endif
                if (select == 0) { /* play game */
                    switch (resume_game_loop(&r_data)) {
                        case TEXT_STATE_QUIT:
                            state = TEXT_STATE_QUIT;
                            break;

                        case TEXT_STATE_USB_CONNECTED:
                            state = TEXT_STATE_USB_CONNECTED;
                            break;

                        case TEXT_STATE_BACK:
                            break;

                        default: { /* Ouch! This nesting is too deep! */
                            switch (game_loop(&r_data)) {
                                case GAME_STATE_QUIT:
                                    state = TEXT_STATE_QUIT;
                                    break;

                                case GAME_STATE_USB_CONNECTED:
                                    state = TEXT_STATE_USB_CONNECTED;
                                    break;

                                case GAME_STATE_PARSE_ERROR:
                                    state = TEXT_STATE_PARSE_ERROR;
                                    break;

                                default:
                                    break;
                            }
                            break;
                        }
                    }
                }
                else if (select == 1) { /* Instructions */
                    switch (instruction_loop()) {
                        case TEXT_STATE_QUIT:
                            state = TEXT_STATE_QUIT;
                            break;

                        case TEXT_STATE_USB_CONNECTED:
                            state = TEXT_STATE_USB_CONNECTED;
                            break;

                        default:
                            break;
                    }
                }
                else /* Quit */
                    state = TEXT_STATE_QUIT;

                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    state = TEXT_STATE_USB_CONNECTED;
                break;
        }
    }

    /* I'm not sure if it's appropriate to write to disk on USB events.
     * Currently, I do so.
     */
    resume_save_data(&r_data, &old_data);

    return state;
}

/* Plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    enum plugin_status state;

    /* Usual plugin stuff */
    (void)parameter;
    rb = api;

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(MAZEZAM_BG_COLOR);
    rb->lcd_set_backdrop(NULL);
#elif LCD_DEPTH > 1
    rb->lcd_set_background(MAZEZAM_BG_GRAY);
#endif
    rb->lcd_setfont(FONT_SYSFIXED);

    /* initialise the config file module */
    configfile_init(rb);

    switch (welcome_loop()) {
        case TEXT_STATE_USB_CONNECTED:
            state = PLUGIN_USB_CONNECTED;
            break;

        case TEXT_STATE_PARSE_ERROR:
            state = PLUGIN_ERROR;
            break;

        default:
            state = PLUGIN_OK;
            break;
    }

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

    return state;
}
