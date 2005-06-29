/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2002 Gilles Roux, 2003 Garrett Derner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include <ctype.h>

#if PLUGIN_API_VERSION < 3
#error Scrollbar function requires PLUGIN_API_VERSION 3 at least
#endif

#define SETTINGS_FILE	"/.rockbox/viewers/viewer.dat"

#define WRAP_TRIM          44  /* Max number of spaces to trim (arbitrary) */
#define MAX_COLUMNS        64  /* Max displayable string len (over-estimate) */
#define MAX_WIDTH         910  /* Max line length in WIDE mode */
#define READ_PREV_ZONE    910  /* Arbitrary number less than SMALL_BLOCK_SIZE */
#define SMALL_BLOCK_SIZE  0x1000 /* 4k: Smallest file chunk we will read */
#define LARGE_BLOCK_SIZE  0x2000 /* 8k: Preferable size of file chunk to read */
#define BUFFER_SIZE       0x3000 /* 12k: Mem reserved for buffered file data */
#define TOP_SECTOR     buffer
#define MID_SECTOR     (buffer + SMALL_BLOCK_SIZE)
#define BOTTOM_SECTOR  (buffer + 2*(SMALL_BLOCK_SIZE))

/* Out-Of-Bounds test for any pointer to data in the buffer */
#define BUFFER_OOB(p)    ((p) < buffer || (p) >= buffer_end)

/* Does the buffer contain the beginning of the file? */
#define BUFFER_BOF()     (file_pos==0)

/* Does the buffer contain the end of the file? */
#define BUFFER_EOF()     (file_size-file_pos <= BUFFER_SIZE)

/* Formula for the endpoint address outside of buffer data */
#define BUFFER_END() \
 ((BUFFER_EOF()) ? (file_size-file_pos+buffer) : (buffer+BUFFER_SIZE))

/* Is the entire file being shown in one screen? */
#define ONE_SCREEN_FITS_ALL() \
 (next_screen_ptr==NULL && screen_top_ptr==buffer && BUFFER_BOF())

/* Is a scrollbar called for on the current screen? */
#define NEED_SCROLLBAR() \
 ((!(ONE_SCREEN_FITS_ALL())) && (scrollbar_mode[view_mode]==SB_ON))

/* variable button definitions */

/* Recorder keys */
#if CONFIG_KEYPAD == RECORDER_PAD
#define VIEWER_QUIT BUTTON_OFF
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MODE_WRAP BUTTON_F1
#define VIEWER_MODE_LINE BUTTON_F2
#define VIEWER_MODE_WIDTH BUTTON_F3
#define VIEWER_MODE_PAGE (BUTTON_ON | BUTTON_F1)
#define VIEWER_MODE_SCROLLBAR (BUTTON_ON | BUTTON_F3)
#define VIEWER_LINE_UP (BUTTON_ON | BUTTON_UP)
#define VIEWER_LINE_DOWN (BUTTON_ON | BUTTON_DOWN)
#define VIEWER_COLUMN_LEFT (BUTTON_ON | BUTTON_LEFT)
#define VIEWER_COLUMN_RIGHT (BUTTON_ON | BUTTON_RIGHT)

/* Ondio keys */
#elif CONFIG_KEYPAD == ONDIO_PAD
#define VIEWER_QUIT BUTTON_OFF
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MODE_WRAP (BUTTON_MENU | BUTTON_LEFT)
#define VIEWER_MODE_LINE (BUTTON_MENU | BUTTON_UP)
#define VIEWER_MODE_WIDTH (BUTTON_MENU | BUTTON_RIGHT)
#define VIEWER_MODE_PAGE (BUTTON_MENU | BUTTON_DOWN)
#define VIEWER_MODE_SCROLLBAR (BUTTON_MENU | BUTTON_OFF)

/* Player keys */
#elif CONFIG_KEYPAD == PLAYER_PAD
#define VIEWER_QUIT BUTTON_STOP
#define VIEWER_PAGE_UP BUTTON_LEFT
#define VIEWER_PAGE_DOWN BUTTON_RIGHT
#define VIEWER_SCREEN_LEFT (BUTTON_MENU | BUTTON_LEFT)
#define VIEWER_SCREEN_RIGHT (BUTTON_MENU | BUTTON_RIGHT)
#define VIEWER_MODE_WRAP (BUTTON_ON | BUTTON_LEFT)
#define VIEWER_MODE_LINE (BUTTON_ON | BUTTON_MENU | BUTTON_RIGHT)
#define VIEWER_MODE_WIDTH (BUTTON_ON | BUTTON_RIGHT)

/* iRiver H1x0 && H3x0 keys */
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define VIEWER_QUIT BUTTON_OFF
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MODE_WRAP BUTTON_REC
#define VIEWER_MODE_LINE BUTTON_MODE
#define VIEWER_MODE_WIDTH BUTTON_SELECT
#define VIEWER_MODE_PAGE (BUTTON_ON | BUTTON_MODE)
#define VIEWER_MODE_SCROLLBAR (BUTTON_ON | BUTTON_REC)
#define VIEWER_LINE_UP (BUTTON_ON | BUTTON_UP)
#define VIEWER_LINE_DOWN (BUTTON_ON | BUTTON_DOWN)
#define VIEWER_COLUMN_LEFT (BUTTON_ON | BUTTON_LEFT)
#define VIEWER_COLUMN_RIGHT (BUTTON_ON | BUTTON_RIGHT)

#endif

enum {
    WRAP=0,
    CHOP,
    WORD_MODES
} word_mode = 0;
static unsigned char *word_mode_str[] = {"wrap", "chop", "words"};

enum {
    NORMAL=0,
    JOIN,
#ifdef HAVE_LCD_BITMAP
    REFLOW, /* Makes no sense for the player */
#endif
    EXPAND,
    LINE_MODES,
#ifndef HAVE_LCD_BITMAP
    REFLOW  /* Sorting it behind LINE_MODES effectively disables it. */
#endif
} line_mode = 0;

static unsigned char *line_mode_str[] = {
    [NORMAL] = "normal", [JOIN] = "join",       [REFLOW] = "reflow",
    [EXPAND] = "expand", [LINE_MODES] = "lines"
};

enum {
    NARROW=0,
    WIDE,
    VIEW_MODES
} view_mode = 0;
static unsigned char *view_mode_str[] = {"narrow", "wide", "view"};

#ifdef HAVE_LCD_BITMAP
enum {
    SB_OFF=0,
    SB_ON,
    SCROLLBAR_MODES
} scrollbar_mode[VIEW_MODES] = {SB_OFF, SB_ON};
static unsigned char *scrollbar_mode_str[] = {"off", "on", "scrollbar"};
static bool need_scrollbar;

enum {
    NO_OVERLAP=0,
    OVERLAP,
    PAGE_MODES
} page_mode = 0;
static unsigned char *page_mode_str[] = {"don't overlap", "overlap", "pages"};
#endif

static unsigned char buffer[BUFFER_SIZE + 1];
static unsigned char line_break[] = {0,0x20,'-',9,0xB,0xC};
static int display_columns; /* number of (pixel) columns on the display */
static int display_lines; /* number of lines on the display */
static int draw_columns; /* number of (pixel) columns available for text */
static int par_indent_spaces; /* number of spaces to indent first paragraph */
static int fd;
static char *file_name;
static long file_size;
static bool mac_text;
static long file_pos; /* Position of the top of the buffer in the file */
static unsigned char *buffer_end; /*Set to BUFFER_END() when file_pos changes*/
static int max_line_len;
static unsigned char *screen_top_ptr;
static unsigned char *next_screen_ptr;
static unsigned char *next_screen_to_draw_ptr;
static unsigned char *next_line_ptr;
static struct plugin_api* rb;

static unsigned char glyph_width[256];

#define ADVANCE_COUNTERS(c) do { width += glyph_width[c]; k++; } while(0)
#define LINE_IS_FULL ((k>MAX_COLUMNS-1) || (width > draw_columns))
static unsigned char* crop_at_width(const unsigned char* p)
{
    int k,width;

    k=width=0;
    while (!LINE_IS_FULL)
        ADVANCE_COUNTERS(p[k]);

    return (unsigned char*) p+k-1;
}

static unsigned char* find_first_feed(const unsigned char* p, int size)
{
    int i;

    for (i=0; i < size; i++)
        if (p[i] == 0)
            return (unsigned char*) p+i;

    return NULL;
}

static unsigned char* find_last_feed(const unsigned char* p, int size)
{
    int i;

    for (i=size-1; i>=0; i--)
        if (p[i] == 0)
            return (unsigned char*) p+i;

    return NULL;
}

static unsigned char* find_last_space(const unsigned char* p, int size)
{
    int i, j, k;

    k = (line_mode==JOIN) || (line_mode==REFLOW) ? 0:1;

    for (i=size-1; i>=0; i--)
        for (j=k; j < (int) sizeof(line_break); j++)
            if (p[i] == line_break[j])
                return (unsigned char*) p+i;

    return NULL;
}

static unsigned char* find_next_line(const unsigned char* cur_line, bool *is_short)
{
    const unsigned char *next_line = NULL;
    int size, i, j, k, width, search_len, spaces, newlines;
    bool first_chars;
    unsigned char c;

    if (is_short != NULL) 
        *is_short = true;
    
    if BUFFER_OOB(cur_line)
        return NULL;

    if (view_mode == WIDE) {
        search_len = MAX_WIDTH;
    }
    else {   /* view_mode == NARROW */
        search_len = crop_at_width(cur_line) - cur_line; 
    }

    size = BUFFER_OOB(cur_line+search_len) ? buffer_end-cur_line : search_len;

    if ((line_mode == JOIN) || (line_mode == REFLOW)) {
        /* Need to scan ahead and possibly increase search_len and size,
         or possibly set next_line at second hard return in a row. */
        next_line = NULL;
        first_chars=true;
        for (j=k=width=spaces=newlines=0; ; j++) {
            if (BUFFER_OOB(cur_line+j))
                return NULL;
            if (LINE_IS_FULL) {
                size = search_len = j;
                break;
            }

            c = cur_line[j];
            switch (c) {
                case ' ':
                    if (line_mode == REFLOW) {
                        if (newlines > 0) {
                            size = j;
                            next_line = cur_line + size;
                            return (unsigned char*) next_line;
                        }
                        if (j==0) /* i=1 is intentional */
                            for (i=0; i<par_indent_spaces; i++)
                                ADVANCE_COUNTERS(' '); 
                    }
                    if (!first_chars) spaces++;
                    break;

                case 0:
                    if (newlines > 0) {
                        size = j;
                        next_line = cur_line + size - spaces;
                        if (next_line != cur_line)
                            return (unsigned char*) next_line;
                        break;
                    }
                    
                    newlines++;
                    size += spaces -1;
                    if (BUFFER_OOB(cur_line+size) || size > 2*search_len)
                        return NULL;
                    search_len = size;
                    spaces = first_chars? 0:1;
                    break;

                default:
                    if (line_mode==JOIN || newlines>0) {
                        while (spaces) {
                            spaces--;
                            ADVANCE_COUNTERS(' ');
                            if (LINE_IS_FULL) {
                                size = search_len = j;
                                break;
                            }
                        }
                        newlines=0;
                   } else if (spaces) {
                        /* REFLOW, multiple spaces between words: count only 
                         * one. If more are needed, they will be added
                         * while drawing. */ 
                        search_len = size;
                        spaces=0;
                        ADVANCE_COUNTERS(' ');
                        if (LINE_IS_FULL) {
                            size = search_len = j;
                            break;
                        }
                    } 
                    first_chars = false;
                    ADVANCE_COUNTERS(c);
                    break;
            }
        }
    }
    else {
        /* find first hard return */
        next_line = find_first_feed(cur_line, size);
    }

    if (next_line == NULL)
        if (size == search_len) {
            if (word_mode == WRAP)  /* Find last space */
                next_line = find_last_space(cur_line, size);

            if (next_line == NULL)
                next_line = crop_at_width(cur_line);
            else
                if (word_mode == WRAP)
                    for (i=0;
                    i<WRAP_TRIM && isspace(next_line[0]) && !BUFFER_OOB(next_line);
                    i++)
                        next_line++;
        }

    if (line_mode == EXPAND)
        if (!BUFFER_OOB(next_line))  /* Not Null & not out of bounds */
            if (next_line[0] == 0)
                if (next_line != cur_line)
                    return (unsigned char*) next_line;

    /* If next_line is pointing to a zero, increment it; i.e.,
     leave the terminator at the end of cur_line. If pointing
     to a hyphen, increment only if there is room to display
     the hyphen on current line (won't apply in WIDE mode,
     since it's guarenteed there won't be room). */
    if (!BUFFER_OOB(next_line))  /* Not Null & not out of bounds */
        if (next_line[0] == 0 ||
        (next_line[0] == '-' && next_line-cur_line < draw_columns))
            next_line++;

    if (BUFFER_OOB(next_line))
        return NULL;

    if (is_short)
        *is_short = false;
    
    return (unsigned char*) next_line;
}

static unsigned char* find_prev_line(const unsigned char* cur_line)
{
    const unsigned char *prev_line = NULL;
    const unsigned char *p;

    if BUFFER_OOB(cur_line)
        return NULL;

    /* To wrap consistently at the same places, we must
     start with a known hard return, then work downwards.
     We can either search backwards for a hard return,
     or simply start wrapping downwards from top of buffer.
       If current line is not near top of buffer, this is
     a file with long lines (paragraphs). We would need to
     read earlier sectors before we could decide how to
     properly wrap the lines above the current line, but
     it probably is not worth the disk access. Instead,
     start with top of buffer and wrap down from there.
     This may result in some lines wrapping at different
     points from where they wrap when scrolling down.
       If buffer is at top of file, start at top of buffer. */

    if ((line_mode == JOIN) || (line_mode == REFLOW))
        prev_line = p = NULL;
    else
        prev_line = p = find_last_feed(buffer, cur_line-buffer-1);
        /* Null means no line feeds in buffer above current line. */

    if (prev_line == NULL)
        if (BUFFER_BOF() || cur_line - buffer > READ_PREV_ZONE)
            prev_line = p = buffer;
        /* (else return NULL and read previous block) */

    /* Wrap downwards until too far, then use the one before. */
    while (p < cur_line && p != NULL) {
        prev_line = p;
        p = find_next_line(prev_line, NULL);
    }

    if (BUFFER_OOB(prev_line))
        return NULL;

    return (unsigned char*) prev_line;
}

static void fill_buffer(long pos, unsigned char* buf, unsigned size)
{
    /* Read from file and preprocess the data */
    /* To minimize disk access, always read on sector boundaries */
    unsigned numread, i;
    bool found_CR = false;

    rb->lseek(fd, pos, SEEK_SET);
    numread = rb->read(fd, buf, size);
    rb->button_clear_queue(); /* clear button queue */

    for(i = 0; i < numread; i++) {
        switch(buf[i]) {
            case '\r':
                if (mac_text) {
                    buf[i] = 0;
                }
                else {
                    buf[i] = ' ';
                    found_CR = true;
                }
                break;

            case '\n':
                buf[i] = 0;
                found_CR = false;
                break;

            case 0:  /* No break between case 0 and default, intentionally */
                buf[i] = ' ';
            default:
                if (found_CR) {
                    buf[i - 1] = 0;
                    found_CR = false;
                    mac_text = true;
                }
                break;
        }
    }
}

static int read_and_synch(int direction)
{
/* Read next (or prev) block, and reposition global pointers. */
/* direction: 1 for down (i.e., further into file), -1 for up */
    int move_size, move_vector, offset;
    unsigned char *fill_buf;
    
    if (direction == -1) /* up */ {
        move_size = SMALL_BLOCK_SIZE;
        offset = 0;
        fill_buf = TOP_SECTOR;
        rb->memcpy(BOTTOM_SECTOR, MID_SECTOR, SMALL_BLOCK_SIZE);
        rb->memcpy(MID_SECTOR, TOP_SECTOR, SMALL_BLOCK_SIZE);
    }
    else /* down */ {
        if (view_mode == WIDE) {
            /* WIDE mode needs more buffer so we have to read smaller blocks */
            move_size = SMALL_BLOCK_SIZE;
            offset = LARGE_BLOCK_SIZE;
            fill_buf = BOTTOM_SECTOR;
            rb->memcpy(TOP_SECTOR, MID_SECTOR, SMALL_BLOCK_SIZE);
            rb->memcpy(MID_SECTOR, BOTTOM_SECTOR, SMALL_BLOCK_SIZE);
        }
        else {
            move_size = LARGE_BLOCK_SIZE;
            offset = SMALL_BLOCK_SIZE;
            fill_buf = MID_SECTOR;
            rb->memcpy(TOP_SECTOR, BOTTOM_SECTOR, SMALL_BLOCK_SIZE);
        }
    }
    move_vector = direction * move_size;
    screen_top_ptr -= move_vector;
    file_pos += move_vector;
    buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
    fill_buffer(file_pos + offset, fill_buf, move_size);
    return move_vector;
}

static void viewer_scroll_up(void)
{
    unsigned char *p;

    p = find_prev_line(screen_top_ptr);
    if (p == NULL && !BUFFER_BOF()) {
        read_and_synch(-1);
        p = find_prev_line(screen_top_ptr);
    }
    if (p != NULL)
        screen_top_ptr = p;
}

#ifdef HAVE_LCD_BITMAP
static void viewer_scrollbar(void) {
    int w, h, items, min_shown, max_shown;

    rb->lcd_getstringsize("o", &w, &h);
    items = (int) file_size;  /* (SH1 int is same as long) */
    min_shown = (int) file_pos + (screen_top_ptr - buffer);

    if (next_screen_ptr == NULL)
        max_shown = items;
    else
        max_shown = min_shown + (next_screen_ptr - screen_top_ptr);

    rb->scrollbar(0, 0, w-1, LCD_HEIGHT, items, min_shown, max_shown, VERTICAL);
}
#endif

static void viewer_draw(int col)
{
    int i, j, k, line_len, resynch_move, spaces, left_col=0;
    int width, extra_spaces, indent_spaces, spaces_per_word;
    bool multiple_spacing, line_is_short;
    unsigned char *line_begin;
    unsigned char *line_end;
    unsigned char c;
    unsigned char scratch_buffer[MAX_COLUMNS + 1];


    /* If col==-1 do all calculations but don't display */
    if (col != -1) {
#ifdef HAVE_LCD_BITMAP
        left_col = need_scrollbar? 1:0;
#else
        left_col = 0;
#endif
        rb->lcd_clear_display();
    }
    max_line_len = 0;
    line_begin = line_end = screen_top_ptr;

    for (i = 0; i < display_lines; i++) {
        if (BUFFER_OOB(line_end))
            break;  /* Happens after display last line at BUFFER_EOF() */

        line_begin = line_end;
        line_end = find_next_line(line_begin, &line_is_short);

        if (line_end == NULL) {
            if (BUFFER_EOF()) {
                if (i < display_lines - 1 && !BUFFER_BOF()) {
                    if (col != -1)
                        rb->lcd_clear_display();

                    for (; i < display_lines - 1; i++)
                        viewer_scroll_up();

                    line_begin = line_end = screen_top_ptr;
                    i = -1;
                    continue;
                }
                else {
                    line_end = buffer_end;
                }
            }
            else {
                resynch_move = read_and_synch(1); /* Read block & move ptrs */
                line_begin -= resynch_move;
                if (i > 0)
                    next_line_ptr -= resynch_move;

                line_end = find_next_line(line_begin, NULL);
                if (line_end == NULL)  /* Should not really happen */
                    break;
            }
        }
        line_len = line_end - line_begin;

        if (line_mode == JOIN) {
            if (line_begin[0] == 0) {
                line_begin++;
                if (word_mode == CHOP)
                    line_end++;
                else
                    line_len--;
            }
            for (j=k=spaces=0; j < line_len; j++) {
                if (k == MAX_COLUMNS)
                    break;

                c = line_begin[j];
                switch (c) {
                    case ' ':
                        spaces++;
                        break;
                    case 0:
                        spaces = 0;
                        scratch_buffer[k++] = ' ';
                        break;
                    default:
                        while (spaces) {
                            spaces--;
                            scratch_buffer[k++] = ' ';
                            if (k == MAX_COLUMNS - 1)
                                break;
                        }
                        scratch_buffer[k++] = c;
                        break;
                }
            }
        
            if (col != -1)
                if (k > col) {
                    scratch_buffer[k] = 0;
                    rb->lcd_puts(left_col, i, scratch_buffer + col);
                }
        }
        else if (line_mode == REFLOW) {
            if (line_begin[0] == 0) {
                line_begin++;
                if (word_mode == CHOP)
                    line_end++;
                else
                    line_len--;
            }

            indent_spaces = 0;
            if (!line_is_short) {
                multiple_spacing = false;
                for (j=width=spaces=0; j < line_len; j++) {
                    c = line_begin[j];
                    switch (c) {
                        case ' ':
                        case 0:
                            if ((j==0) && (word_mode==WRAP))
                                /* special case: indent the paragraph, 
                                 * don't count spaces */
                                indent_spaces = par_indent_spaces;
                            else if (!multiple_spacing) 
                                spaces++;
                            multiple_spacing = true;
                            break;
                        default:
                            multiple_spacing = false;
                            width += glyph_width[c];
                            k++;
                            break;
                    }
                }
                if (multiple_spacing) spaces--;
                
                if (spaces) {
                    /* total number of spaces to insert between words */
                    extra_spaces = (draw_columns-width) / glyph_width[' '] 
                            - indent_spaces;
                    /* number of spaces between each word*/
                    spaces_per_word = extra_spaces / spaces;
                    /* number of words with n+1 spaces (to fill up) */
                    extra_spaces = extra_spaces % spaces;
                    if (spaces_per_word > 2) { /* too much spacing is awful */
                        spaces_per_word = 3;
                        extra_spaces = 0;
                    } 
                } else { /* this doesn't matter much... no spaces anyway */
                    spaces_per_word = extra_spaces = 0;
                }
            } else { /* end of a paragraph: don't fill line */
                spaces_per_word = 1;
                extra_spaces = 0;
            }
            
            multiple_spacing = false;
            for (j=k=spaces=0; j < line_len; j++) {
                if (k == MAX_COLUMNS)
                    break;

                c = line_begin[j];
                switch (c) {
                    case ' ':
                    case 0:
                        if (j==0 && word_mode==WRAP) { /* indent paragraph */
                            for (j=0; j<par_indent_spaces; j++)
                                scratch_buffer[k++] = ' ';
                            j=0;
                        } 
                        else if (!multiple_spacing) {
                            for (width = spaces<extra_spaces ? -1:0; width < spaces_per_word; width++)
                                    scratch_buffer[k++] = ' ';
                            spaces++;
                        }
                        multiple_spacing = true;
                        break;
                    default:
                        scratch_buffer[k++] = c;
                        multiple_spacing = false;
                        break;
                }
            }
                    
            if (col != -1)
                if (k > col) {
                    scratch_buffer[k] = 0;
                    rb->lcd_puts(left_col, i, scratch_buffer + col);
                }
        } 
        else { /* line_mode != JOIN && line_mode != REFLOW */
            if (col != -1)
                if (line_len > col) {
                    c = line_end[0];
                    line_end[0] = 0;
                    rb->lcd_puts(left_col, i, line_begin + col);
                    line_end[0] = c;
                }
        }
        if (line_len > max_line_len)
            max_line_len = line_len;

        if (i == 0)
            next_line_ptr = line_end;
    }
    next_screen_ptr = line_end;
    if (BUFFER_OOB(next_screen_ptr))
        next_screen_ptr = NULL;

#ifdef HAVE_LCD_BITMAP
    next_screen_to_draw_ptr = page_mode==OVERLAP? line_begin: next_screen_ptr;

    if (need_scrollbar)
        viewer_scrollbar();

    if (col != -1)
        rb->lcd_update();
#else
    next_screen_to_draw_ptr = next_screen_ptr;
#endif
}

static void viewer_top(void)
{
    /* Read top of file into buffer
      and point screen pointer to top */
    file_pos = 0;
    buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
    screen_top_ptr = buffer;
    fill_buffer(0, buffer, BUFFER_SIZE);
}

static void viewer_bottom(void)
{
    /* Read bottom of file into buffer
      and point screen pointer to bottom */
    long last_sectors;

    if (file_size > BUFFER_SIZE) {
        /* Find last buffer in file, round up to next sector boundary */
        last_sectors = file_size - BUFFER_SIZE + SMALL_BLOCK_SIZE;
        last_sectors /= SMALL_BLOCK_SIZE;
        last_sectors *= SMALL_BLOCK_SIZE;
    }
    else {
        last_sectors = 0;
    }
    file_pos = last_sectors;
    buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
    screen_top_ptr = buffer_end-1;
    fill_buffer(last_sectors, buffer, BUFFER_SIZE);
}

#ifdef HAVE_LCD_BITMAP
static void init_need_scrollbar(void) {
    /* Call viewer_draw in quiet mode to initialize next_screen_ptr,
     and thus ONE_SCREEN_FITS_ALL(), and thus NEED_SCROLLBAR() */
    viewer_draw(-1);
    need_scrollbar = NEED_SCROLLBAR();
    draw_columns = need_scrollbar? display_columns-glyph_width['o'] : display_columns;
    par_indent_spaces = draw_columns/(5*glyph_width[' ']);
}
#endif

static bool viewer_init(void)
{
#ifdef HAVE_LCD_BITMAP
    int idx, ch;
    struct font *pf;

    pf = rb->font_get(FONT_UI);
    if (pf->width != NULL)
    {   /* variable pitch font -- fill structure from font width data */
        ch = pf->defaultchar - pf->firstchar; 
        rb->memset(glyph_width, pf->width[ch], 256);
        idx = pf->firstchar;
        rb->memcpy(&glyph_width[idx], pf->width, pf->size);
        idx += pf->size;
        rb->memset(&glyph_width[idx], pf->width[ch], 256-idx);
    } 
    else /* fixed pitch font -- same width for all glyphs */
        rb->memset(glyph_width, pf->maxwidth, 256);
    
    display_lines = LCD_HEIGHT / pf->height;
    display_columns = LCD_WIDTH;
#else
    /* REAL fixed pitch :) all chars use up 1 cell */
    display_lines = 2;
    draw_columns = display_columns = 11;
    par_indent_spaces = 2;
    rb->memset(glyph_width, 1, 256);
#endif
    /*********************
    * (Could re-initialize settings here, if you
    *   wanted viewer to start the same way every time)
    word_mode = WRAP;
    line_mode = NORMAL;
    view_mode = NARROW;
#ifdef HAVE_LCD_BITMAP
    page_mode = NO_OVERLAP;
    scrollbar_mode[NARROW] = SB_OFF;
    scrollbar_mode[WIDE] = SB_ON;
#endif
    **********************/

    fd = rb->open(file_name, O_RDONLY);
    if (fd==-1)
        return false;

    file_size = rb->filesize(fd);
    if (file_size==-1)
        return false;

    /* Init mac_text value used in processing buffer */
    mac_text = false;

    /* Read top of file into buffer;
      init file_pos, buffer_end, screen_top_ptr */
    viewer_top();

#ifdef HAVE_LCD_BITMAP
    /* Init need_scrollbar value */
    init_need_scrollbar();
#endif

    return true;
}

/* in the viewer settings file, the line format is:
 * - file name (variable length)
 * - settings  (fixed length strings appended, EOL included)
 */
typedef struct {
        char word_mode[2], line_mode[2], view_mode[2];
        char file_pos[11], screen_top_ptr[11];
#ifdef HAVE_LCD_BITMAP
        char scrollbar_mode[VIEW_MODES][2], page_mode[2];
#endif
        char EOL;
} viewer_settings_string;

static void viewer_load_settings(void)
{
    int settings_fd, file_name_len, req_line_len, line_len; 
    char line[1024];
    
    settings_fd=rb->open(SETTINGS_FILE, O_RDONLY);
    if (settings_fd < 0) return;

    file_name_len = rb->strlen(file_name);
    req_line_len = file_name_len + sizeof(viewer_settings_string);
    while ((line_len = rb->read_line(settings_fd, line, sizeof(line))) > 0) {
        if ((line_len == req_line_len) && 
                (rb->strncasecmp(line, file_name, file_name_len) == 0)) {
            /* found a match, load stored values */
            viewer_settings_string *prefs = (void*) &line[file_name_len];

#ifdef HAVE_LCD_BITMAP
            /* view mode will be initialized later anyways */
            for (view_mode=0; view_mode<VIEW_MODES; ++view_mode)
                scrollbar_mode[view_mode] = 
                        rb->atoi(prefs->scrollbar_mode[view_mode]);
            
            page_mode = rb->atoi(prefs->page_mode);
#endif
            
            word_mode = rb->atoi(prefs->word_mode);
            line_mode = rb->atoi(prefs->line_mode);
            view_mode = rb->atoi(prefs->view_mode);
            
#ifdef HAVE_LCD_BITMAP
            init_need_scrollbar();
#endif
            /* the following settings are safety checked 
             * (file may have changed on disk) 
             */
            file_pos = rb->atoi(prefs->file_pos); /* should be atol() */
            if (file_pos > file_size) {
                    file_pos = 0;
                    break;
            } 
            buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
            
            screen_top_ptr = buffer + rb->atoi(prefs->screen_top_ptr);
            if (BUFFER_OOB(screen_top_ptr)) {
                    screen_top_ptr = buffer;
                    break;
            }
        }
    }
    rb->close(settings_fd);
    
    fill_buffer(file_pos, buffer, BUFFER_SIZE);
}
            
static void viewer_save_settings(void)
{
    int settings_fd, file_name_len, req_line_len, line_len; 
    char line[1024];
    viewer_settings_string prefs;
    
    settings_fd=rb->open(SETTINGS_FILE, O_RDWR | O_CREAT);
    DEBUGF("SETTINGS_FILE: %d\n", settings_fd);
    if (settings_fd < 0) return;

    file_name_len = rb->strlen(file_name);
    req_line_len = file_name_len + sizeof(viewer_settings_string);
    while ((line_len = rb->read_line(settings_fd, line, sizeof(line))) > 0) {
        if ((line_len == req_line_len) && 
                (rb->strncasecmp(line, file_name, file_name_len) == 0)) {
            /* found a match, reposition file pointer to overwrite this line */
            rb->lseek(settings_fd, -line_len, SEEK_CUR);
            break;
        }
    }
    
    /* fill structure in order to prevent overwriting with 0s (snprintf
     * intentionally overflows so that no terminating NULLs are written 
     * to disk). */
    rb->snprintf(prefs.word_mode, 3, "%2d", word_mode);
    rb->snprintf(prefs.line_mode, 3, "%2d", line_mode);
    rb->snprintf(prefs.view_mode, 3, "%2d", view_mode);
    rb->snprintf(prefs.file_pos, 12, "%11d", file_pos);
    rb->snprintf(prefs.screen_top_ptr, 12, "%11d", screen_top_ptr-buffer);
#ifdef HAVE_LCD_BITMAP
    /* view_mode is not needed anymore */
    for (view_mode=0; view_mode<VIEW_MODES; ++view_mode) 
        rb->snprintf(prefs.scrollbar_mode[view_mode], 3, 
                        "%2d", scrollbar_mode[view_mode]);
                        
    rb->snprintf(prefs.page_mode, 3, "%2d", page_mode);
#endif
    prefs.EOL = '\n';
    
    rb->write(settings_fd, file_name, file_name_len);
    rb->write(settings_fd, &prefs, sizeof(prefs));
    rb->close(settings_fd);
}     

static void viewer_exit(void *parameter)
{
    (void)parameter;

    viewer_save_settings();
    rb->close(fd);
}

static int col_limit(int col)
{
    if (col < 0)
        col = 0;
    else
        if (col > max_line_len - 2)
            col = max_line_len - 2;

    return col;
}

enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
    bool exit=false;
    int button;
    int col = 0;
    int i;
    int ok;

    TEST_PLUGIN_API(api);
    rb = api;

    if (!file)
        return PLUGIN_ERROR;

    file_name = file;
    ok = viewer_init();
    if (!ok) {
        rb->splash(HZ, false, "Error");
        viewer_exit(NULL);
        return PLUGIN_OK;
    }

    viewer_load_settings();

    viewer_draw(col);

    while (!exit) {
        button = rb->button_get(true);
        switch (button) {

            case VIEWER_QUIT:
                viewer_exit(NULL);
                exit = true;
                break;

            case VIEWER_MODE_WRAP:
                /* Word-wrap mode: WRAP or CHOP */
                if (++word_mode == WORD_MODES)
                    word_mode = 0;

#ifdef HAVE_LCD_BITMAP
                init_need_scrollbar();
#endif
                viewer_draw(col);

                rb->splash(HZ, true, "%s %s",
                           word_mode_str[word_mode],
                           word_mode_str[WORD_MODES]);

                viewer_draw(col);
                break;

            case VIEWER_MODE_LINE:
                /* Line-paragraph mode: NORMAL, JOIN, REFLOW or EXPAND */
                if (++line_mode == LINE_MODES)
                    line_mode = 0;

                if (view_mode == WIDE) {
                    if (line_mode == JOIN)
                        if (++line_mode == LINE_MODES)
                            line_mode = 0;
                    if (line_mode == REFLOW)
                        if (++line_mode == LINE_MODES)
                            line_mode = 0;
                }

#ifdef HAVE_LCD_BITMAP
                init_need_scrollbar();
#endif
                viewer_draw(col);

                rb->splash(HZ, true, "%s %s",
                           line_mode_str[line_mode],
                           line_mode_str[LINE_MODES]);

                viewer_draw(col);
                break;

            case VIEWER_MODE_WIDTH:
                /* View-width mode: NARROW or WIDE */
                if ((line_mode == JOIN) || (line_mode == REFLOW))
                    rb->splash(HZ, true, "(no %s %s)",
                               view_mode_str[WIDE],
                               line_mode_str[line_mode]);
                else
                    if (++view_mode == VIEW_MODES)
                        view_mode = 0;

                col = 0;

                /***** Could do this after change of word-wrap mode
                * and after change of view-width mode, to normalize
                * view:
                if (screen_top_ptr > buffer + BUFFER_SIZE/2) {
                    screen_top_ptr = find_prev_line(screen_top_ptr);
                    screen_top_ptr = find_next_line(screen_top_ptr);
                }
                else {
                    screen_top_ptr = find_next_line(screen_top_ptr);
                    screen_top_ptr = find_prev_line(screen_top_ptr);
                }
                ***********/

#ifdef HAVE_LCD_BITMAP
                init_need_scrollbar();
#endif
                viewer_draw(col);

                rb->splash(HZ, true, "%s %s",
                           view_mode_str[view_mode],
                           view_mode_str[VIEW_MODES]);

                viewer_draw(col);
                break;

            case VIEWER_PAGE_UP:
            case VIEWER_PAGE_UP | BUTTON_REPEAT:
                /* Page up */
#ifdef HAVE_LCD_BITMAP
                for (i = page_mode==OVERLAP? 1:0; i < display_lines; i++)
#else
                for (i = 0; i < display_lines; i++)
#endif
                    viewer_scroll_up();

                viewer_draw(col);
                break;

            case VIEWER_PAGE_DOWN:
            case VIEWER_PAGE_DOWN | BUTTON_REPEAT:
                /* Page down */
                if (next_screen_ptr != NULL)
                    screen_top_ptr = next_screen_to_draw_ptr;

                viewer_draw(col);
                break;

            case VIEWER_SCREEN_LEFT:
            case VIEWER_SCREEN_LEFT | BUTTON_REPEAT:
                if (view_mode == WIDE) {
                    /* Screen left */
                    col -= draw_columns/glyph_width['o'];
                    col = col_limit(col);
                }
                else {   /* view_mode == NARROW */
                    /* Top of file */
                    viewer_top();
                }

                viewer_draw(col);
                break;

            case VIEWER_SCREEN_RIGHT:
            case VIEWER_SCREEN_RIGHT | BUTTON_REPEAT:
                if (view_mode == WIDE) {
                    /* Screen right */
                    col += draw_columns/glyph_width['o'];
                    col = col_limit(col);
                }
                else {   /* view_mode == NARROW */
                    /* Bottom of file */
                    viewer_bottom();
                }

                viewer_draw(col);
                break;

#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == ONDIO_PAD) \
    || (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
            case VIEWER_MODE_PAGE:
                /* Page-overlap mode */
                if (++page_mode == PAGE_MODES)
                    page_mode = 0;

                rb->splash(HZ, true, "%s %s",
                           page_mode_str[page_mode],
                           page_mode_str[PAGE_MODES]);

                viewer_draw(col);
                break;

            case VIEWER_MODE_SCROLLBAR:
                /* Show-scrollbar mode for current view-width mode */
                if (!(ONE_SCREEN_FITS_ALL())) {
                    if (++scrollbar_mode[view_mode] == SCROLLBAR_MODES)
                        scrollbar_mode[view_mode] = 0;

                    init_need_scrollbar();
                    viewer_draw(col);

                    rb->splash(HZ, true, "%s %s (%s %s)",
                               scrollbar_mode_str[SCROLLBAR_MODES],
                               scrollbar_mode_str[scrollbar_mode[view_mode]],
                               view_mode_str[view_mode],
                               view_mode_str[VIEW_MODES]);
                }
                viewer_draw(col);
                break;
#endif

#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H100_PAD) \
  || (CONFIG_KEYPAD == IRIVER_H300_PAD)
            case VIEWER_LINE_UP:
            case VIEWER_LINE_UP | BUTTON_REPEAT:
                /* Scroll up one line */
                viewer_scroll_up();
                viewer_draw(col);
                break;

            case VIEWER_LINE_DOWN:
            case VIEWER_LINE_DOWN | BUTTON_REPEAT:
                /* Scroll down one line */
                if (next_screen_ptr != NULL)
                    screen_top_ptr = next_line_ptr;

                viewer_draw(col);
                break;

            case VIEWER_COLUMN_LEFT:
            case VIEWER_COLUMN_LEFT | BUTTON_REPEAT:
                /* Scroll left one column */
                col--;
                col = col_limit(col);
                viewer_draw(col);
                break;

            case VIEWER_COLUMN_RIGHT:
            case VIEWER_COLUMN_RIGHT | BUTTON_REPEAT:
                /* Scroll right one column */
                col++;
                col = col_limit(col);
                viewer_draw(col);
                break;
#endif

            default:
                if (rb->default_event_handler_ex(button, viewer_exit, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
    return PLUGIN_OK;
}
