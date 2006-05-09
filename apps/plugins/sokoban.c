/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Eric Linenberg
 * February 2003: Robert Hak performs a cleanup/rewrite/feature addition.
 *                Eric smiles.  Bjorn cries.  Linus say 'huh?'.
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

#define SOKOBAN_TITLE       "Sokoban"
#define SOKOBAN_TITLE_FONT  2

#define LEVELS_FILE         PLUGIN_DIR "/sokoban.levels"

#define ROWS                16
#define COLS                20
/* Use all but 8k of the plugin buffer for board data */
#define SOKOBAN_LEVEL_SIZE  (ROWS*COLS)
#define MAX_BUFFERED_BOARDS (PLUGIN_BUFFER_SIZE - 0x2000)/SOKOBAN_LEVEL_SIZE
#define MAX_UNDOS           5

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_OFF
#define SOKOBAN_UNDO BUTTON_ON
#define SOKOBAN_LEVEL_UP BUTTON_F3
#define SOKOBAN_LEVEL_DOWN BUTTON_F1
#define SOKOBAN_LEVEL_REPEAT BUTTON_F2

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_OFF
#define SOKOBAN_UNDO_PRE BUTTON_MENU
#define SOKOBAN_UNDO (BUTTON_MENU | BUTTON_REL)
#define SOKOBAN_LEVEL_UP (BUTTON_MENU | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_DOWN (BUTTON_MENU | BUTTON_LEFT)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_MENU | BUTTON_UP)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_OFF
#define SOKOBAN_UNDO BUTTON_ON
#define SOKOBAN_LEVEL_UP BUTTON_MODE
#define SOKOBAN_LEVEL_DOWN BUTTON_REC
#define SOKOBAN_LEVEL_REPEAT BUTTON_SELECT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
#define SOKOBAN_UP BUTTON_MENU
#define SOKOBAN_DOWN BUTTON_PLAY
#define SOKOBAN_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_LEVEL_UP (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_DOWN (BUTTON_SELECT | BUTTON_LEFT)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_PLAY)

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_LEVEL_UP (BUTTON_PLAY | BUTTON_UP)
#define SOKOBAN_LEVEL_DOWN (BUTTON_PLAY | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT BUTTON_REC

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_A
#define SOKOBAN_UNDO BUTTON_MENU
#define SOKOBAN_LEVEL_UP (BUTTON_POWER | BUTTON_UP)
#define SOKOBAN_LEVEL_DOWN (BUTTON_POWER | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT BUTTON_SELECT

#endif

#ifdef HAVE_LCD_COLOR
#define WALL_COLOR         LCD_RGBPACK(16,20,180)   /* Color of the walls */
#define FREE_TARGET_COLOR  LCD_RGBPACK(251,158,25)  /* Color of a 'target' without a block on top */
#define USED_TARGET_COLOR  LCD_RGBPACK(255,255,255) /* Color of a 'target' with a block on top */
#define FREE_BLOCK_COLOR   LCD_RGBPACK(22,130,53)   /* Color of a block when it's not on a 'target' */
#define USED_BLOCK_COLOR   LCD_RGBPACK(22,130,53)   /* Color of a block when it is on a 'target' */
#define CHAR_COLOR         LCD_BLACK                /* Color of the 'character' */
#define BG_COLOR           LCD_RGBPACK(181,199,231) /* Background color. Default Rockbox light blue. */

#elif LCD_DEPTH > 1
#define MEDIUM_GRAY LCD_BRIGHTNESS(127)
#endif

static void init_undo(void);
static void undo(void);
static void add_undo(int button);

static int read_levels(int initialize_count);
static void load_level(void);
static void draw_level(void);

static void init_boards(void);
static void update_screen(void);
static bool sokoban_loop(void);

/* The Location, Undo and LevelInfo structs are OO-flavored.
 * (oooh!-flavored as Schnueff puts it.)  It makes more you have to know,
 * but the overall data layout becomes more manageable. */

/* We use the same three values in 2 structs.  Makeing them a struct
 * hopefully ensures that if you change things in one, the other changes
 * as well. */
struct LevelInfo {
    short level;
    short moves;
    short boxes_to_go;
};

/* What a given location on the board looks like at a given time */
struct Location {
    char spot;
    short row;
    short col;
};

/* A single level of undo.  Each undo move can affect upto,
 * but not more then, 3 spots on the board */
struct Undo {
    struct LevelInfo level;
    struct Location location[3];
};

struct Board {
    char spaces[ROWS][COLS];
};

/* Our full undo history */
static struct UndoInfo {
    short count;    /* How many undos are there in history */
    short current;  /* Which history is the current undo */
    struct Undo history[MAX_UNDOS];
} undo_info;

/* Our playing board */
static struct BoardInfo {
    char board[ROWS][COLS];
    struct LevelInfo level;
    struct Location player;
    int max_level;               /* How many levels do we have? */
    int loaded_level;            /* Which level is in memory */
} current_info;

static struct BufferedBoards {
    struct Board levels[MAX_BUFFERED_BOARDS];
    int low;
} buffered_boards;

static struct plugin_api* rb;

static void init_undo(void)
{
    undo_info.count = 0;
    undo_info.current = 0;
}

static void undo(void)
{
    struct Undo *undo;
    int i = 0;
    short row, col;

    if (undo_info.count == 0)
        return;

    /* Update board info */
    undo = &undo_info.history[undo_info.current];

    rb->memcpy(&current_info.level, &undo->level, sizeof(undo->level));
    rb->memcpy(&current_info.player, &undo->location[0], sizeof(undo->location[0]));

    row = undo->location[0].row;
    col = undo->location[0].col;
    current_info.board[row][col] = '@';

    /* Update the two other possible spots */
    for (i = 1; i < 3; i++) {
        if (undo->location[i].spot != '\0') {
            row = undo->location[i].row;
            col = undo->location[i].col;
            current_info.board[row][col] = undo->location[i].spot;
            undo->location[i].spot = '\0';
        }
    }

    /* Remove this undo from the list */
    if (undo_info.current == 0) {
        if (undo_info.count > 1)
            undo_info.current = MAX_UNDOS - 1;
    } else {
        undo_info.current--;
    }

    undo_info.count--;

    return;
}

static void add_undo(int button)
{
    struct Undo *undo;
    int row, col, i;
    bool storable;

    if ((button != BUTTON_LEFT) && (button != BUTTON_RIGHT) &&
        (button != SOKOBAN_UP) && (button != SOKOBAN_DOWN))
        return;

    if (undo_info.count != 0) {
        if (undo_info.current < (MAX_UNDOS - 1))
            undo_info.current++;
        else
            undo_info.current = 0;
    }

    /* Make what follows more readable */
    undo = &undo_info.history[undo_info.current];

    /* Store our level info */
    rb->memcpy(&undo->level, &current_info.level, sizeof(undo->level));

    /* Store our player info */
    rb->memcpy(&undo->location[0], &current_info.player, sizeof(undo->location[0]));

    /* Now we need to store upto 2 blocks that may be affected.
     * If player.spot is NULL, then there is no info stored
     * for that block */

    row = current_info.player.row;
    col = current_info.player.col;

    /* This must stay as _1_ because the first block (0) is the player */
    for (i = 1; i < 3; i++) {
        storable = true;

        switch (button) {
        case BUTTON_LEFT:
            col--;
            if (col < 0)
                storable = false;
            break;

        case BUTTON_RIGHT:
            col++;
            if (col >= COLS)
                storable = false;
            break;

        case SOKOBAN_UP:
            row--;
            if (row < 0)
                storable = false;
            break;

        case SOKOBAN_DOWN:
            row++;
            if (row >= ROWS)
                storable = false;
            break;

        default:
            return;
        }

        if (storable) {
            undo->location[i].col = col;
            undo->location[i].row = row;
            undo->location[i].spot = current_info.board[row][col];
        } else {
            undo->location[i].spot = '\0';
        }
    }

    if (undo_info.count < MAX_UNDOS)
        undo_info.count++;
}

static void init_boards(void)
{
    current_info.level.level = 0;
    current_info.level.moves = 0;
    current_info.level.boxes_to_go = 0;
    current_info.player.row = 0;
    current_info.player.col = 0;
    current_info.player.spot = ' ';
    current_info.max_level = 0;
    current_info.loaded_level = 0;

    buffered_boards.low = 0;

    init_undo();
}

static int read_levels(int initialize_count)
{
    int fd = 0;
    int len;
    int lastlen = 0;
    int row = 0;
    int level_count = 0;
    char buffer[COLS + 3]; /* COLS plus CR/LF and \0 */
    int endpoint = current_info.level.level-1;

    if (endpoint < buffered_boards.low)
        endpoint = current_info.level.level - MAX_BUFFERED_BOARDS;

    if (endpoint < 0) endpoint = 0;

    buffered_boards.low = endpoint;
    endpoint += MAX_BUFFERED_BOARDS;

    if ((fd = rb->open(LEVELS_FILE, O_RDONLY)) < 0) {
        rb->splash(0, true, "Unable to open %s", LEVELS_FILE);
        return -1;
    }

    do {
        len = rb->read_line(fd, buffer, sizeof(buffer));
        if (len >= 3) {
            /* This finds lines that are more than 1 or 2 characters
             * shorter than they should be.  Due to the possibility of
             * a mixed unix and dos CR/LF file format, I'm not going to
             * do a precise check */
            if (len < COLS) {
                rb->splash(0, true, "Error in levels file: short line");
                return -1;
            }
            if (level_count >= buffered_boards.low && level_count < endpoint) {
                int index = level_count - buffered_boards.low;
                rb->memcpy(
                        buffered_boards.levels[index].spaces[row],buffer,COLS);
            }
            row++;
        } else if (len) {
            if (lastlen < 3) {
                /* Two short lines in a row means new level */
                level_count++;
                if (level_count >= endpoint && !initialize_count) break;
                if (level_count && row != ROWS) {
                    rb->splash(0, true, "Error in levels file: short board");
                    return -1;
                }
                row = 0;
            }
        }
    } while ((lastlen=len));

    rb->close(fd);
    if (initialize_count) {
        /* Plus one because there aren't trailing short lines in the file */
        current_info.max_level = level_count + 1;
    }
    return 0;
}

/* return non-zero on error */
static void load_level(void)
{
    int c = 0;
    int r = 0;
    int index = current_info.level.level - buffered_boards.low - 1;
    struct Board *level;

    if (index < 0 || index >= MAX_BUFFERED_BOARDS) {
        read_levels(false);
        index=index<0?MAX_BUFFERED_BOARDS-1:0;
    }
    level = &buffered_boards.levels[index];

    current_info.player.spot=' ';
    current_info.level.boxes_to_go = 0;
    current_info.level.moves = 0;
    current_info.loaded_level = current_info.level.level;

    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            current_info.board[r][c] = level->spaces[r][c];

            if (current_info.board[r][c] == '.')
                current_info.level.boxes_to_go++;

            else if (current_info.board[r][c] == '@') {
                current_info.player.row = r;
                current_info.player.col = c;
            }
        }
    }
}
#define STAT_WIDTH (LCD_WIDTH-(COLS * magnify))

static void update_screen(void)
{
    int b = 0, c = 0;
    int rows = 0, cols = 0;
    char s[25];

#if LCD_HEIGHT == 128        /* magnify is the number of pixels for each block */
    int magnify = 6;         /* 6 on h1x0, 9 on h3x0, and 4 on everything else */
#elif LCD_HEIGHT >= 176
    int magnify = 9;
#else
    int magnify = 4;
#endif

    /* load the board to the screen */
    for (rows=0 ; rows < ROWS ; rows++) {
        for (cols = 0 ; cols < COLS ; cols++) {
            c = cols * magnify;
            b = rows * magnify;

            switch(current_info.board[rows][cols]) {
            case 'X': /* black space */
                break;

            case '#': /* this is a wall */
#if HAVE_LCD_COLOR
                rb->lcd_set_foreground(WALL_COLOR);
                rb->lcd_fillrect(c, b, magnify, magnify);
                rb->lcd_set_foreground(LCD_BLACK);
#elif LCD_DEPTH > 1
                rb->lcd_set_foreground(MEDIUM_GRAY);
                rb->lcd_fillrect(c, b, magnify, magnify);
                rb->lcd_set_foreground(LCD_BLACK);
#else
                {
                    int i, j;
                    for (i = c; i < c + magnify; i++)
                        for (j = b; j < b + magnify; j++)
                            if ((i ^ j) & 1)
                                rb->lcd_drawpixel(i, j);
                }
#endif
                break;

            case '.': /* this is a home location */
#ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(FREE_TARGET_COLOR);
                rb->lcd_fillrect(c+(magnify/2)-1, b+(magnify/2)-1, magnify/2,
                                 magnify/2);
#else
                rb->lcd_drawrect(c+(magnify/2)-1, b+(magnify/2)-1, magnify/2,
                                 magnify/2);
#endif
                break;

            case '$': /* this is a box */
#ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(FREE_BLOCK_COLOR);
#endif
                rb->lcd_drawrect(c, b, magnify, magnify);  /* Free boxes are not filled in */
                break;

            case '@': /* this is you */
                {
                    int max = magnify - 1;
                    int middle = max / 2;
                    int ldelta = (middle + 1) / 2;
#ifdef HAVE_LCD_COLOR
                    rb->lcd_set_foreground(CHAR_COLOR);
#endif
                    rb->lcd_drawline(c, b+middle, c+max, b+middle);
                    rb->lcd_drawline(c+middle, b, c+middle, b+max-ldelta);
                    rb->lcd_drawline(c+max-middle, b,
                                     c+max-middle, b+max-ldelta);
                    rb->lcd_drawline(c+middle, b+max-ldelta,
                                     c+middle-ldelta, b+max);
                    rb->lcd_drawline(c+max-middle, b+max-ldelta,
                                     c+max-middle+ldelta, b+max);
                }
                break;

            case '%': /* this is a box on a home spot */

#ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(USED_BLOCK_COLOR);
                rb->lcd_fillrect(c, b, magnify, magnify);
#else
                rb->lcd_drawrect(c, b, magnify, magnify);
#endif
#ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(USED_TARGET_COLOR);
                rb->lcd_fillrect(c+(magnify/2)-1, b+(magnify/2)-1, magnify/2,
                                 magnify/2);
#else
                rb->lcd_drawrect(c+(magnify/2)-1, b+(magnify/2)-1, magnify/2,
                                 magnify/2);
#endif
                break;
            }
        }
    }

    rb->snprintf(s, sizeof(s), "%d", current_info.level.level);
    rb->lcd_putsxy(LCD_WIDTH-STAT_WIDTH+4, 22, s);
    rb->snprintf(s, sizeof(s), "%d", current_info.level.moves);
    rb->lcd_putsxy(LCD_WIDTH-STAT_WIDTH+4, 54, s);

    rb->lcd_drawrect(LCD_WIDTH-STAT_WIDTH,0,STAT_WIDTH,32);
    rb->lcd_drawrect(LCD_WIDTH-STAT_WIDTH,32,STAT_WIDTH,LCD_HEIGHT-32);
    rb->lcd_putsxy(LCD_WIDTH-STAT_WIDTH+1, 10, "Level");
    rb->lcd_putsxy(LCD_WIDTH-STAT_WIDTH+1, 42, "Moves");

    /* print out the screen */
    rb->lcd_update();
}

static void draw_level(void)
{
    load_level();
    rb->lcd_clear_display();
    update_screen();
}

static bool sokoban_loop(void)
{
    char new_spot;
    bool moved = true;
    int i = 0, button = 0, lastbutton = 0;
    short r = 0, c = 0;

    current_info.level.level = 1;

    load_level();
    update_screen();

    while (1) {
        moved = true;

        r = current_info.player.row;
        c = current_info.player.col;

        button = rb->button_get(true);

        add_undo(button);

        switch(button)
        {
            case SOKOBAN_QUIT:
                /* get out of here */
#ifdef HAVE_LCD_COLOR /* reset background color */
                rb->lcd_set_background(rb->global_settings->bg_color);
#endif
                return PLUGIN_OK;

            case SOKOBAN_UNDO:
#ifdef SOKOBAN_UNDO_PRE
                if (lastbutton != SOKOBAN_UNDO_PRE)
                    break;
#else       /* repeat can't work here for Ondio */
            case SOKOBAN_UNDO | BUTTON_REPEAT:
#endif
                /* this is UNDO */
                undo();
                rb->lcd_clear_display();
                update_screen();
                moved = false;
                break;

            case SOKOBAN_LEVEL_UP:
            case SOKOBAN_LEVEL_UP | BUTTON_REPEAT:
                /* increase level */
                init_undo();
                current_info.level.boxes_to_go=0;
                moved = true;
                break;

            case SOKOBAN_LEVEL_DOWN:
            case SOKOBAN_LEVEL_DOWN | BUTTON_REPEAT:
                /* previous level */
                init_undo();
                if (current_info.level.level > 1)
                    current_info.level.level--;

                draw_level();
                moved = false;
                break;

            case SOKOBAN_LEVEL_REPEAT:
            case SOKOBAN_LEVEL_REPEAT | BUTTON_REPEAT:
                /* same level */
                init_undo();
                draw_level();
                moved = false;
                break;

            case BUTTON_LEFT:
                switch(current_info.board[r][c-1])
                {
                    case ' ': /* if it is a blank spot */
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r][c-1];
                        current_info.board[r][c-1] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$':
                        switch(current_info.board[r][c-2])
                        {
                            case ' ': /* going from blank to blank */
                                current_info.board[r][c-2] = current_info.board[r][c-1];
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r][c-2] = '%';
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r][c-2]) {
                            case ' ': /* we are going from a home to a blank */
                                current_info.board[r][c-2] = '$';
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.': /* if we are going from a home to home */
                                current_info.board[r][c-2] = '%';
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.col--;
                break;

            case BUTTON_RIGHT: /* if it is a blank spot */
                switch(current_info.board[r][c+1]) {
                    case ' ':
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r][c+1];
                        current_info.board[r][c+1] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$':
                        switch(current_info.board[r][c+2]) {
                            case ' ': /* going from blank to blank */
                                current_info.board[r][c+2] = current_info.board[r][c+1];
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r][c+2] = '%';
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r][c+2]) {
                            case ' ': /* going from a home to a blank */
                                current_info.board[r][c+2] = '$';
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.':
                                current_info.board[r][c+2] = '%';
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.col++;
                break;

            case SOKOBAN_UP:
                switch(current_info.board[r-1][c]) {
                    case ' ': /* if it is a blank spot */
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r-1][c];
                        current_info.board[r-1][c] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$':
                        switch(current_info.board[r-2][c]) {
                            case ' ': /* going from blank to blank */
                                current_info.board[r-2][c] = current_info.board[r-1][c];
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r-2][c] = '%';
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r-2][c]) {
                            case ' ': /* we are going from a home to a blank */
                                current_info.board[r-2][c] = '$';
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.': /* if we are going from a home to home */
                                current_info.board[r-2][c] = '%';
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.row--;
                break;

            case SOKOBAN_DOWN:
                switch(current_info.board[r+1][c]) {
                    case ' ': /* if it is a blank spot */
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r+1][c];
                        current_info.board[r+1][c] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$':
                        switch(current_info.board[r+2][c]) {
                            case ' ': /* going from blank to blank */
                                current_info.board[r+2][c] = current_info.board[r+1][c];
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r+2][c] = '%';
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r+2][c]) {
                            case ' ': /* going from a home to a blank */
                                current_info.board[r+2][c] = '$';
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.': /* going from a home to home */
                                current_info.board[r+2][c] = '%';
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.row++;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;

                moved = false;
                break;
        }

        if (button != BUTTON_NONE)
            lastbutton = button;

        if (moved) {
            current_info.level.moves++;
            rb->lcd_clear_display();
            update_screen();
        }

        /* We have completed this level */
        if (current_info.level.boxes_to_go == 0) {
            current_info.level.level++;

            /* clear undo stats */
            init_undo();

            rb->lcd_clear_display();

            if (current_info.level.level > current_info.max_level) {
                /* Center "You WIN!!" on all screen sizes */
                rb->lcd_putsxy(LCD_WIDTH/2 - 27,(LCD_HEIGHT/2) - 4 ,
                               "You WIN!!");

                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                /* Pattern Fills whole screen now on all screen sizes */
                for (i = 0; i < 30000 ; i++) {
                    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
                    rb->lcd_update();
                    rb->sleep(HZ/20);

                    button = rb->button_get(false);
                    if (button && ((button & BUTTON_REL) != BUTTON_REL))
                        break;
                }
                rb->lcd_set_drawmode(DRMODE_SOLID);

                return PLUGIN_OK;
            }

            load_level();
            update_screen();
        }

    } /* end while */

    return PLUGIN_OK;
}


enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int w, h;
    int len;

    (void)(parameter);
    rb = api;

    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_getstringsize(SOKOBAN_TITLE, &w, &h);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(BG_COLOR);
#endif

    /* Get horizontel centering for text */
    len = w;
    if (len%2 != 0)
        len =((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    rb->lcd_clear_display();
    rb->lcd_putsxy(LCD_WIDTH/2-len,(LCD_HEIGHT/2)-h, SOKOBAN_TITLE);
    rb->lcd_update();
    rb->sleep(HZ*2);

    rb->lcd_clear_display();

#if CONFIG_KEYPAD == RECORDER_PAD
    rb->lcd_putsxy(3,  6, "[OFF] Quit");
    rb->lcd_putsxy(3, 16, "[ON] Undo");
    rb->lcd_putsxy(3, 26, "[F1] Down a Level");
    rb->lcd_putsxy(3, 36, "[F2] Restart Level");
    rb->lcd_putsxy(3, 46, "[F3] Up a Level");
#elif CONFIG_KEYPAD == ONDIO_PAD
    rb->lcd_putsxy(3,  6, "[OFF] Quit");
    rb->lcd_putsxy(3, 16, "[MODE] Undo");
    rb->lcd_putsxy(3, 26, "[M-LEFT] Down a Level");
    rb->lcd_putsxy(3, 36, "[M-UP] Restart Level");
    rb->lcd_putsxy(3, 46, "[M-RIGHT] Up Level");
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
    rb->lcd_putsxy(3,  6, "[STOP] Stop");
    rb->lcd_putsxy(3, 16, "[PLAY] Undo");
    rb->lcd_putsxy(3, 26, "[REC] Down a Level");
    rb->lcd_putsxy(3, 36, "[SELECT] Restart Level");
    rb->lcd_putsxy(3, 46, "[MODE] + Up a Level");
#endif

    rb->lcd_update();
    rb->sleep(HZ*2);
    rb->lcd_clear_display();

    init_boards();

    if (read_levels(1) != 0) {
        rb->splash(HZ*2, true, "Failed loading levels!");
        return PLUGIN_OK;
    }

    return sokoban_loop();
}

#endif
