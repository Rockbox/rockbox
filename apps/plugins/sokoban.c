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

#if LCD_DEPTH >= 2
extern const fb_data sokoban_tiles[];
#endif

#define SOKOBAN_TITLE       "Sokoban"

#define LEVELS_FILE         PLUGIN_DIR "/sokoban.levels"

#define ROWS                16
#define COLS                20
#define SOKOBAN_LEVEL_SIZE  (ROWS*COLS)
#define MAX_BUFFERED_BOARDS 500
/* Use either all but 12k of the plugin buffer for board data
   or just enough for MAX_BUFFERED_BOARDS, which ever is less */
#if (PLUGIN_BUFFER_SIZE - 0x3000)/SOKOBAN_LEVEL_SIZE < MAX_BUFFERED_BOARDS
#define NUM_BUFFERED_BOARDS (PLUGIN_BUFFER_SIZE - 0x3000)/SOKOBAN_LEVEL_SIZE
#else
#define NUM_BUFFERED_BOARDS MAX_BUFFERED_BOARDS
#endif
/* Use 4k plus remaining plugin buffer (-8k for prog) for undo, up to 32k */
#if PLUGIN_BUFFER_SIZE - NUM_BUFFERED_BOARDS*SOKOBAN_LEVEL_SIZE - 0x2000 > \
    0x7FFF
#define MAX_UNDOS           0x7FFF
#else
#define MAX_UNDOS           PLUGIN_BUFFER_SIZE - \
                            NUM_BUFFERED_BOARDS*SOKOBAN_LEVEL_SIZE - 0x2000
#endif

/* Move/push definitions for undo */
enum {
    SOKOBAN_PUSH_LEFT,
    SOKOBAN_PUSH_RIGHT,
    SOKOBAN_PUSH_UP,
    SOKOBAN_PUSH_DOWN,
    SOKOBAN_MOVE_LEFT,
    SOKOBAN_MOVE_RIGHT,
    SOKOBAN_MOVE_UP,
    SOKOBAN_MOVE_DOWN
};
#define SOKOBAN_MOVE_DIFF   (SOKOBAN_MOVE_LEFT-SOKOBAN_PUSH_LEFT)
#define SOKOBAN_MOVE_MIN    SOKOBAN_MOVE_LEFT

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_OFF
#define SOKOBAN_UNDO BUTTON_ON
#define SOKOBAN_REDO BUTTON_PLAY
#define SOKOBAN_LEVEL_UP BUTTON_F3
#define SOKOBAN_LEVEL_DOWN BUTTON_F1
#define SOKOBAN_LEVEL_REPEAT BUTTON_F2

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_OFF
#define SOKOBAN_UNDO BUTTON_ON
#define SOKOBAN_REDO BUTTON_PLAY
#define SOKOBAN_LEVEL_UP BUTTON_F3
#define SOKOBAN_LEVEL_DOWN BUTTON_F1
#define SOKOBAN_LEVEL_REPEAT BUTTON_F2

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_OFF
#define SOKOBAN_UNDO_PRE BUTTON_MENU
#define SOKOBAN_UNDO (BUTTON_MENU | BUTTON_REL)
#define SOKOBAN_REDO (BUTTON_MENU | BUTTON_DOWN)
#define SOKOBAN_LEVEL_UP (BUTTON_MENU | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_DOWN (BUTTON_MENU | BUTTON_LEFT)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_MENU | BUTTON_UP)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_OFF
#define SOKOBAN_UNDO BUTTON_REC
#define SOKOBAN_REDO BUTTON_MODE
#define SOKOBAN_LEVEL_UP (BUTTON_ON | BUTTON_UP)
#define SOKOBAN_LEVEL_DOWN (BUTTON_ON | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT BUTTON_ON

#define SOKOBAN_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
#define SOKOBAN_UP BUTTON_MENU
#define SOKOBAN_DOWN BUTTON_PLAY
#define SOKOBAN_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO (BUTTON_SELECT | BUTTON_PLAY)
#define SOKOBAN_LEVEL_UP (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_DOWN (BUTTON_SELECT | BUTTON_LEFT)

/* fixme: if/when simultaneous button presses work for X5,
   add redo & level repeat */
#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_LEVEL_UP BUTTON_PLAY
#define SOKOBAN_LEVEL_DOWN BUTTON_REC

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_A
#define SOKOBAN_UNDO BUTTON_SELECT
#define SOKOBAN_REDO BUTTON_POWER
#define SOKOBAN_LEVEL_UP (BUTTON_MENU | BUTTON_UP)
#define SOKOBAN_LEVEL_DOWN (BUTTON_MENU | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT BUTTON_MENU

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_QUIT BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO BUTTON_REC
#define SOKOBAN_LEVEL_UP (BUTTON_SELECT | BUTTON_UP)
#define SOKOBAN_LEVEL_DOWN (BUTTON_SELECT | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_RIGHT)

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define SOKOBAN_UP BUTTON_SCROLL_UP
#define SOKOBAN_DOWN BUTTON_SCROLL_DOWN
#define SOKOBAN_QUIT BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_REW
#define SOKOBAN_UNDO (BUTTON_REW | BUTTON_REL)
#define SOKOBAN_REDO BUTTON_FF
#define SOKOBAN_LEVEL_UP (BUTTON_PLAY | BUTTON_SCROLL_UP)
#define SOKOBAN_LEVEL_DOWN (BUTTON_PLAY | BUTTON_SCROLL_DOWN)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_PLAY | BUTTON_RIGHT)

#endif

#ifdef HAVE_LCD_COLOR
/* Background color. Default Rockbox light blue. */
#define BG_COLOR LCD_RGBPACK(181, 199, 231)

#elif LCD_DEPTH >= 2
#define MEDIUM_GRAY LCD_BRIGHTNESS(127)
#endif

/* The Location, Undo and LevelInfo structs are OO-flavored.
 * (oooh!-flavored as Schnueff puts it.)  It makes more you have to know,
 * but the overall data layout becomes more manageable. */

/* Level data & stats */
struct LevelInfo {
    short level;
    short moves;
    short pushes;
    short boxes_to_go;
};

struct Location {
    short row;
    short col;
};

struct Board {
    char spaces[ROWS][COLS];
};

/* Our full undo history */
static struct UndoInfo {
    short count;    /* How many undos are left */
    short current;  /* Which history is the current undo */
    short max;      /* Which history is the max redoable */
    char history[MAX_UNDOS];
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
    struct Board levels[NUM_BUFFERED_BOARDS];
    int low;
} buffered_boards;

static struct plugin_api* rb;

static void init_undo(void)
{
    undo_info.count = 0;
    undo_info.current = -1;
    undo_info.max = -1;
}

static void get_delta(char direction, short *d_r, short *d_c)
{
    switch (direction) {
        case SOKOBAN_PUSH_LEFT:
        case SOKOBAN_MOVE_LEFT:
            *d_r = 0;
            *d_c = -1;
            break;
        case SOKOBAN_PUSH_RIGHT:
        case SOKOBAN_MOVE_RIGHT:
            *d_r = 0;
            *d_c = 1;
            break;
        case SOKOBAN_PUSH_UP:
        case SOKOBAN_MOVE_UP:
            *d_r = -1;
            *d_c = 0;
            break;
        case SOKOBAN_PUSH_DOWN:
        case SOKOBAN_MOVE_DOWN:
            *d_r = 1;
            *d_c = 0;
    }
}

static void undo(void)
{
    char undo;
    short r, c;
    short d_r = 0, d_c = 0; /* delta row & delta col */
    char *space_cur, *space_next, *space_prev;
    bool undo_push = false;

    /* If no more undos or we've wrapped all the way around, quit */
    if (undo_info.count == 0 || undo_info.current-1 == undo_info.max)
        return;

    undo = undo_info.history[undo_info.current];

    if (undo < SOKOBAN_MOVE_MIN)
        undo_push = true;

    get_delta(undo, &d_r, &d_c);

    r = current_info.player.row;
    c = current_info.player.col;

    /* Give the 3 spaces we're going to use better names */
    space_cur = &current_info.board[r][c];
    space_next = &current_info.board[r + d_r][c + d_c];
    space_prev = &current_info.board[r - d_r][c - d_c];

    /* Update board info */
    if (undo_push) {
        /* Moving box from goal to blank */
        if (*space_next == '%' && *space_cur == '@')
            current_info.level.boxes_to_go++;
        /* Moving box from blank to goal */
        else if (*space_next == '$' && *space_cur == '+')
            current_info.level.boxes_to_go--;

        /* Move box off of next space... */
        *space_next = (*space_next == '%' ? '.' : ' ');
        /* ...and on to current space */
        *space_cur = (*space_cur == '+' ? '%' : '$');

        current_info.level.pushes--;
    } else
        /* Just move player off of current space */
        *space_cur = (*space_cur == '+' ? '.' : ' ');
    /* Move player back to previous space */
    *space_prev = (*space_prev == '.' ? '+' : '@');

    /* Update position */
    current_info.player.row -= d_r;
    current_info.player.col -= d_c;

    current_info.level.moves--;

    /* Move to previous undo in the list */
    if (undo_info.current == 0 && undo_info.count > 1)
        undo_info.current = MAX_UNDOS - 1;
    else
        undo_info.current--;

    undo_info.count--;

    return;
}

static void add_undo(char undo)
{
    /* Wrap around if MAX_UNDOS exceeded */
    if (undo_info.current < (MAX_UNDOS - 1))
        undo_info.current++;
    else
        undo_info.current = 0;

    undo_info.history[undo_info.current] = undo;

    if (undo_info.count < MAX_UNDOS)
        undo_info.count++;
}

static bool move(char direction, bool redo)
{
    short r, c;
    short d_r = 0, d_c = 0; /* delta row & delta col */
    char *space_cur, *space_next, *space_beyond;
    bool push = false;

    get_delta(direction, &d_r, &d_c);

    r = current_info.player.row;
    c = current_info.player.col;

    /* Check for out-of-bounds */
    if (r + 2*d_r < 0 || r + 2*d_r >= ROWS ||
        c + 2*d_c < 0 || c + 2*d_c >= COLS)
        return false;

    /* Give the 3 spaces we're going to use better names */
    space_cur = &current_info.board[r][c];
    space_next = &current_info.board[r + d_r][c + d_c];
    space_beyond = &current_info.board[r + 2*d_r][c + 2*d_c];

    if (*space_next == '$' || *space_next == '%') {
        /* Change direction from move to push for undo */
        if (direction >= SOKOBAN_MOVE_MIN)
            direction -= SOKOBAN_MOVE_DIFF;
        push = true;
    }

    /* Update board info */
    if (push) {
        /* Moving box from goal to blank */
        if (*space_next == '%' && *space_beyond == ' ')
            current_info.level.boxes_to_go++;
        /* Moving box from blank to goal */
        else if (*space_next == '$' && *space_beyond == '.')
            current_info.level.boxes_to_go--;
        /* Check for illegal move */
        else if (*space_beyond != '.' && *space_beyond != ' ')
            return false;

        /* Move player onto next space */
        *space_next = (*space_next == '%' ? '+' : '@');
        /* Move box onto space beyond next */
        *space_beyond = (*space_beyond == '.' ? '%' : '$');

        current_info.level.pushes++;
    } else {
        /* Check for illegal move */
        if (*space_next == '#' || *space_next == 'X')
            return false;

        /* Move player onto next space */
        *space_next = (*space_next == '.' ? '+' : '@');
    }
    /* Move player off of current space */
    *space_cur = (*space_cur == '+' ? '.' : ' ');

    /* Update position */
    current_info.player.row += d_r;
    current_info.player.col += d_c;

    current_info.level.moves++;

    /* Update undo_info.max to current on every normal move,
       except if it's the same as a redo. */
    /* normal move */
    if (!redo &&
          /* moves have been undone */
        ((undo_info.max != undo_info.current &&
          /* and the current move is NOT the same as the one in history */
          undo_info.history[undo_info.current+1] != direction) ||
         /* or moves have not been undone */
         undo_info.max == undo_info.current)) {
        add_undo(direction);
        undo_info.max = undo_info.current;
    } else /* redo move or move was same as redo */
        add_undo(direction); /* (just to update current) */

    return true;
}

#ifdef SOKOBAN_REDO
static bool redo(void)
{
    /* If no moves have been undone, quit */
    if (undo_info.current == undo_info.max)
        return false;

    return move(undo_info.history[(undo_info.current+1 < MAX_UNDOS ?
                                              undo_info.current+1 : 0)], true);
}
#endif

static void init_boards(void)
{
    current_info.level.level = 0;
    current_info.level.moves = 0;
    current_info.level.pushes = 0;
    current_info.level.boxes_to_go = 0;
    current_info.player.row = 0;
    current_info.player.col = 0;
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
        endpoint = current_info.level.level - NUM_BUFFERED_BOARDS;

    if (endpoint < 0) endpoint = 0;

    buffered_boards.low = endpoint;
    endpoint += NUM_BUFFERED_BOARDS;

    if ((fd = rb->open(LEVELS_FILE, O_RDONLY)) < 0) {
        rb->splash(HZ*2, "Unable to open %s", LEVELS_FILE);
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
                rb->splash(HZ*2, "Error in levels file: short line");
                return -1;
            }
            if (level_count >= buffered_boards.low && level_count < endpoint) {
                int index = level_count - buffered_boards.low;
                rb->memcpy(
                      buffered_boards.levels[index].spaces[row], buffer, COLS);
            }
            row++;
        } else if (len) {
            if (lastlen < 3) {
                /* Two short lines in a row means new level */
                level_count++;
                if (level_count >= endpoint && !initialize_count) break;
                if (level_count && row != ROWS) {
                    rb->splash(HZ*2, "Error in levels file: short board");
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

    if (index < 0 || index >= NUM_BUFFERED_BOARDS) {
        read_levels(false);
        index = index < 0 ? NUM_BUFFERED_BOARDS-1 : 0;
    }
    level = &buffered_boards.levels[index];

    current_info.level.boxes_to_go = 0;
    current_info.level.moves = 0;
    current_info.level.pushes = 0;
    current_info.loaded_level = current_info.level.level;

    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            current_info.board[r][c] = level->spaces[r][c];

            if (current_info.board[r][c] == '.' ||
                current_info.board[r][c] == '+')
                current_info.level.boxes_to_go++;

            if (current_info.board[r][c] == '@' ||
                current_info.board[r][c] == '+') {
                current_info.player.row = r;
                current_info.player.col = c;
            }
        }
    }
}

static void update_screen(void)
{
    int b = 0, c = 0;
    int rows = 0, cols = 0;
    char s[25];

/* magnify is the number of pixels for each block */
#if (LCD_HEIGHT >= 224) && (LCD_WIDTH >= 312) || \
    (LCD_HEIGHT >= 249) && (LCD_WIDTH >= 280) /* ipod 5g */
#define MAGNIFY 14
#elif (LCD_HEIGHT >= 144) && (LCD_WIDTH >= 212) || \
      (LCD_HEIGHT >= 169) && (LCD_WIDTH >= 180-4) /* h3x0, ipod color/photo */
#define MAGNIFY 9
#elif (LCD_HEIGHT >= 96) && (LCD_WIDTH >= 152) || \
      (LCD_HEIGHT >= 121) && (LCD_WIDTH >= 120) /* h1x0, ipod nano/mini */
#define MAGNIFY 6
#else /* other */
#define MAGNIFY 4
#endif

#if LCD_DEPTH < 2
    int i, j;
    int max = MAGNIFY - 1;
    int middle = max / 2;
    int ldelta = (middle + 1) / 2;
#endif

    /* load the board to the screen */
    for (rows=0; rows < ROWS; rows++) {
        for (cols = 0; cols < COLS; cols++) {
            c = cols * MAGNIFY;
            b = rows * MAGNIFY;

            switch(current_info.board[rows][cols]) {
            case 'X': /* black space */
                break;

            case '#': /* this is a wall */
#if LCD_DEPTH >= 2
                rb->lcd_bitmap_part(sokoban_tiles, 0, 1*MAGNIFY, MAGNIFY,
                                    c, b, MAGNIFY, MAGNIFY);
#else
                for (i = c; i < c + MAGNIFY; i++)
                    for (j = b; j < b + MAGNIFY; j++)
                        if ((i ^ j) & 1)
                            rb->lcd_drawpixel(i, j);
#endif
                break;

            case '$': /* this is a box */
#if LCD_DEPTH >= 2
                rb->lcd_bitmap_part(sokoban_tiles, 0, 2*MAGNIFY, MAGNIFY,
                                    c, b, MAGNIFY, MAGNIFY);
#else
                /* Free boxes are not filled in */
                rb->lcd_drawrect(c, b, MAGNIFY, MAGNIFY);
#endif
                break;

            case '*':
            case '%': /* this is a box on a goal */

#if LCD_DEPTH >= 2
                rb->lcd_bitmap_part(sokoban_tiles, 0, 3*MAGNIFY, MAGNIFY,
                                    c, b, MAGNIFY, MAGNIFY );
#else
                rb->lcd_drawrect(c, b, MAGNIFY, MAGNIFY);
                rb->lcd_drawrect(c+(MAGNIFY/2)-1, b+(MAGNIFY/2)-1, MAGNIFY/2,
                                 MAGNIFY/2);
#endif
                break;

            case '.': /* this is a goal */
#if LCD_DEPTH >= 2
                rb->lcd_bitmap_part(sokoban_tiles, 0, 4*MAGNIFY, MAGNIFY,
                                    c, b, MAGNIFY, MAGNIFY);
#else
                rb->lcd_drawrect(c+(MAGNIFY/2)-1, b+(MAGNIFY/2)-1, MAGNIFY/2,
                                 MAGNIFY/2);
#endif
                break;

            case '@': /* this is you */
#if LCD_DEPTH >= 2
                rb->lcd_bitmap_part(sokoban_tiles, 0, 5*MAGNIFY, MAGNIFY,
                                    c, b, MAGNIFY, MAGNIFY);
#else
                rb->lcd_drawline(c, b+middle, c+max, b+middle);
                rb->lcd_drawline(c+middle, b, c+middle, b+max-ldelta);
                rb->lcd_drawline(c+max-middle, b,
                                 c+max-middle, b+max-ldelta);
                rb->lcd_drawline(c+middle, b+max-ldelta,
                                 c+middle-ldelta, b+max);
                rb->lcd_drawline(c+max-middle, b+max-ldelta,
                                 c+max-middle+ldelta, b+max);
#endif
                break;

            case '+': /* this is you on drugs, erm, on a goal */
#if LCD_DEPTH >= 2
                rb->lcd_bitmap_part(sokoban_tiles, 0, 6*MAGNIFY, MAGNIFY,
                                    c, b, MAGNIFY, MAGNIFY );
#else
                rb->lcd_drawline(c, b+middle, c+max, b+middle);
                rb->lcd_drawline(c+middle, b, c+middle, b+max-ldelta);
                rb->lcd_drawline(c+max-middle, b, c+max-middle, b+max-ldelta);
                rb->lcd_drawline(c+middle, b+max-ldelta, c+middle-ldelta,
                                 b+max);
                rb->lcd_drawline(c+max-middle, b+max-ldelta,
                                 c+max-middle+ldelta, b+max);
                rb->lcd_drawline(c+middle-1, b+middle+1, c+max-middle+1,
                                 b+middle+1);
#endif
                break;

#if LCD_DEPTH >= 2
            default:
                rb->lcd_bitmap_part(sokoban_tiles, 0, 0*MAGNIFY, MAGNIFY,
                                    c, b, MAGNIFY, MAGNIFY );
#endif
            }
        }
    }

#if LCD_WIDTH-(COLS*MAGNIFY) < 32
#define STAT_SIZE 25
#define STAT_POS LCD_HEIGHT-STAT_SIZE
#define STAT_CENTER (LCD_WIDTH-120)/2

    rb->lcd_putsxy(4+STAT_CENTER, STAT_POS+4, "Level");
    rb->snprintf(s, sizeof(s), "%d", current_info.level.level);
    rb->lcd_putsxy(7+STAT_CENTER, STAT_POS+14, s);
    rb->lcd_putsxy(41+STAT_CENTER, STAT_POS+4, "Moves");
    rb->snprintf(s, sizeof(s), "%d", current_info.level.moves);
    rb->lcd_putsxy(44+STAT_CENTER, STAT_POS+14, s);
    rb->lcd_putsxy(79+STAT_CENTER, STAT_POS+4, "Pushes");
    rb->snprintf(s, sizeof(s), "%d", current_info.level.pushes);
    rb->lcd_putsxy(82+STAT_CENTER, STAT_POS+14, s);

    rb->lcd_drawrect(STAT_CENTER, STAT_POS, 38, STAT_SIZE);
    rb->lcd_drawrect(37+STAT_CENTER, STAT_POS, 39, STAT_SIZE);
    rb->lcd_drawrect(75+STAT_CENTER, STAT_POS, 45, STAT_SIZE);

#else
#define STAT_POS COLS*MAGNIFY
#define STAT_SIZE LCD_WIDTH-STAT_POS

    rb->lcd_putsxy(STAT_POS+1, 3, "Level");
    rb->snprintf(s, sizeof(s), "%d", current_info.level.level);
    rb->lcd_putsxy(STAT_POS+4, 13, s);
    rb->lcd_putsxy(STAT_POS+1, 26, "Moves");
    rb->snprintf(s, sizeof(s), "%d", current_info.level.moves);
    rb->lcd_putsxy(STAT_POS+4, 36, s);

    rb->lcd_drawrect(STAT_POS, 0, STAT_SIZE, 24);
    rb->lcd_drawrect(STAT_POS, 23, STAT_SIZE, 24);

#if LCD_HEIGHT >= 70
    rb->lcd_putsxy(STAT_POS+1, 49, "Pushes");
    rb->snprintf(s, sizeof(s), "%d", current_info.level.pushes);
    rb->lcd_putsxy(STAT_POS+4, 59, s);

    rb->lcd_drawrect(STAT_POS, 46, STAT_SIZE, 24);
#endif
#endif

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
    bool moved = true;
    int i = 0, button = 0, lastbutton = 0;
    short r = 0, c = 0;
    int w, h;
    char s[25];

    current_info.level.level = 1;

    load_level();
    update_screen();

    while (1) {
        moved = true;

        r = current_info.player.row;
        c = current_info.player.col;

        button = rb->button_get(true);

        switch(button)
        {
#ifdef SOKOBAN_RC_QUIT
            case SOKOBAN_RC_QUIT:
#endif
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
#else       /* repeat can't work here for Ondio et al */
            case SOKOBAN_UNDO | BUTTON_REPEAT:
#endif
                undo();
                rb->lcd_clear_display();
                update_screen();
                moved = false;
                break;

#ifdef SOKOBAN_REDO
            case SOKOBAN_REDO:
            case SOKOBAN_REDO | BUTTON_REPEAT:
                moved = redo();
                rb->lcd_clear_display();
                update_screen();
                break;
#endif

            case SOKOBAN_LEVEL_UP:
            case SOKOBAN_LEVEL_UP | BUTTON_REPEAT:
                /* next level */
                init_undo();
                if (current_info.level.level < current_info.max_level)
                    current_info.level.level++;

                draw_level();
                moved = false;
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

#ifdef SOKOBAN_LEVEL_REPEAT
            case SOKOBAN_LEVEL_REPEAT:
            case SOKOBAN_LEVEL_REPEAT | BUTTON_REPEAT:
                /* same level */
                init_undo();
                draw_level();
                moved = false;
                break;
#endif

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_LEFT, false);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_RIGHT, false);
                break;

            case SOKOBAN_UP:
            case SOKOBAN_UP | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_UP, false);
                break;

            case SOKOBAN_DOWN:
            case SOKOBAN_DOWN | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_DOWN, false);
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
            rb->lcd_clear_display();
            update_screen();
        }

        /* We have completed this level */
        if (current_info.level.boxes_to_go == 0) {

            if (moved) {
                rb->lcd_clear_display();
                /* Center level completed message */
                rb->snprintf(s, sizeof(s), "Level %d Complete!",
                             current_info.level.level);
                rb->lcd_getstringsize(s, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - 16 , s);
                rb->snprintf(s, sizeof(s), "%4d Moves ",
                             current_info.level.moves);
                rb->lcd_getstringsize(s, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 + 0 , s);
                rb->snprintf(s, sizeof(s), "%4d Pushes",
                             current_info.level.pushes);
                rb->lcd_getstringsize(s, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 + 8 , s);
                rb->lcd_update();
                rb->button_get(false);

                rb->sleep(HZ/2);
                for (i = 0; i < 30; i++) {
                    rb->sleep(HZ/20);
                    button = rb->button_get(false);
                    if (button && ((button & BUTTON_REL) != BUTTON_REL))
                        break;
                }
            }

            current_info.level.level++;

            /* clear undo stats */
            init_undo();

            rb->lcd_clear_display();

            if (current_info.level.level > current_info.max_level) {
                /* Center "You WIN!!" on all screen sizes */
                rb->snprintf(s, sizeof(s), "You WIN!!");
                rb->lcd_getstringsize(s, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - h/2, s);

                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                /* Display for 10 seconds or until keypress */
                for (i = 0; i < 200; i++) {
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
    int i;
    int button = 0;

    (void)(parameter);
    rb = api;

    rb->lcd_setfont(FONT_SYSFIXED);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(BG_COLOR);
#endif

    rb->lcd_clear_display();
    rb->lcd_getstringsize(SOKOBAN_TITLE, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - h/2, SOKOBAN_TITLE);
    rb->lcd_update();
    rb->sleep(HZ);

    rb->lcd_clear_display();

#if (CONFIG_KEYPAD == RECORDER_PAD) || \
    (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
    rb->lcd_putsxy(3,  6, "[OFF] Quit");
    rb->lcd_putsxy(3, 16, "[ON] Undo");
    rb->lcd_putsxy(3, 26, "[PLAY] Redo");
    rb->lcd_putsxy(3, 36, "[F1] Down a Level");
    rb->lcd_putsxy(3, 46, "[F2] Restart Level");
    rb->lcd_putsxy(3, 56, "[F3] Up a Level");
#elif CONFIG_KEYPAD == ONDIO_PAD
    rb->lcd_putsxy(3,  6, "[OFF] Quit");
    rb->lcd_putsxy(3, 16, "[MODE] Undo");
    rb->lcd_putsxy(3, 26, "[MODE+DOWN] Redo");
    rb->lcd_putsxy(3, 36, "[MODE+LEFT] Down a Level");
    rb->lcd_putsxy(3, 46, "[MODE+UP] Restart Level");
    rb->lcd_putsxy(3, 56, "[MODE+RIGHT] Up Level");
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
    rb->lcd_putsxy(3,  6, "[STOP] Quit");
    rb->lcd_putsxy(3, 16, "[REC] Undo");
    rb->lcd_putsxy(3, 26, "[MODE] Redo");
    rb->lcd_putsxy(3, 36, "[PLAY+DOWN] Down a Level");
    rb->lcd_putsxy(3, 46, "[PLAY] Restart Level");
    rb->lcd_putsxy(3, 56, "[PLAY+UP] Up a Level");
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
    rb->lcd_putsxy(3,  6, "[SELECT+MENU] Quit");
    rb->lcd_putsxy(3, 16, "[SELECT] Undo");
    rb->lcd_putsxy(3, 26, "[SELECT+PLAY] Redo");
    rb->lcd_putsxy(3, 36, "[SELECT+LEFT] Down a Level");
    rb->lcd_putsxy(3, 46, "[SELECT+RIGHT] Up a Level");
#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
    rb->lcd_putsxy(3,  6, "[POWER] Quit");
    rb->lcd_putsxy(3, 16, "[SELECT] Undo");
    rb->lcd_putsxy(3, 26, "[REC] Down a Level");
    rb->lcd_putsxy(3, 36, "[PLAY] Up Level");
#elif CONFIG_KEYPAD == GIGABEAT_PAD
    rb->lcd_putsxy(3,  6, "[A] Quit");
    rb->lcd_putsxy(3, 16, "[SELECT] Undo");
    rb->lcd_putsxy(3, 26, "[POWER] Redo");
    rb->lcd_putsxy(3, 36, "[MENU+DOWN] Down a Level");
    rb->lcd_putsxy(3, 46, "[MENU] Restart Level");
    rb->lcd_putsxy(3, 56, "[MENU+UP] Up Level");
#elif CONFIG_KEYPAD == SANSA_E200_PAD
    rb->lcd_putsxy(3,  6, "[POWER] Quit");
    rb->lcd_putsxy(3, 16, "[SELECT] Undo");
    rb->lcd_putsxy(3, 26, "[REC] Redo");
    rb->lcd_putsxy(3, 36, "[SELECT+DOWN] Down a Level");
    rb->lcd_putsxy(3, 46, "[SELECT+RIGHT] Restart Level");
    rb->lcd_putsxy(3, 56, "[SELECT+UP] Up Level");
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
    rb->lcd_putsxy(3,  6, "[POWER] Quit");
    rb->lcd_putsxy(3, 16, "[REW] Undo");
    rb->lcd_putsxy(3, 26, "[FF] Redo");
    rb->lcd_putsxy(3, 36, "[PLAY+DOWN] Down a Level");
    rb->lcd_putsxy(3, 46, "[PLAY+RIGHT] Restart Level");
    rb->lcd_putsxy(3, 56, "[PLAY+UP] Up Level");
#endif

    rb->lcd_update();
    rb->button_get(false);
    /* Display for 3 seconds or until keypress */
    for (i = 0; i < 60; i++) {
        rb->sleep(HZ/20);
        button = rb->button_get(false);
        if (button && ((button & BUTTON_REL) != BUTTON_REL))
            break;
    }
    rb->lcd_clear_display();

    init_boards();

    if (read_levels(1) != 0)
        return PLUGIN_OK;

    return sokoban_loop();
}

#endif
