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
#include <ctype.h>
#include "lib/playback_control.h"
#include "lib/oldmenuapi.h"

PLUGIN_HEADER

#define SETTINGS_FILE   VIEWERS_DIR "/viewer.dat" /* binary file, so dont use .cfg */
#define BOOKMARKS_FILE  VIEWERS_DIR "/viewer_bookmarks.dat"

#define WRAP_TRIM          44  /* Max number of spaces to trim (arbitrary) */
#define MAX_COLUMNS        64  /* Max displayable string len (over-estimate) */
#define MAX_WIDTH         910  /* Max line length in WIDE mode */
#define READ_PREV_ZONE    910  /* Arbitrary number less than SMALL_BLOCK_SIZE */
#define SMALL_BLOCK_SIZE  0x1000 /* 4k: Smallest file chunk we will read */
#define LARGE_BLOCK_SIZE  0x2000 /* 8k: Preferable size of file chunk to read */
#define TOP_SECTOR     buffer
#define MID_SECTOR     (buffer + SMALL_BLOCK_SIZE)
#define BOTTOM_SECTOR  (buffer + 2*(SMALL_BLOCK_SIZE))
#define SCROLLBAR_WIDTH     6

#define MAX_BOOKMARKED_FILES ((buffer_size/(signed)sizeof(struct bookmarked_file_info))-1)

/* Out-Of-Bounds test for any pointer to data in the buffer */
#define BUFFER_OOB(p)    ((p) < buffer || (p) >= buffer_end)

/* Does the buffer contain the beginning of the file? */
#define BUFFER_BOF()     (file_pos==0)

/* Does the buffer contain the end of the file? */
#define BUFFER_EOF()     (file_size-file_pos <= buffer_size)

/* Formula for the endpoint address outside of buffer data */
#define BUFFER_END() \
 ((BUFFER_EOF()) ? (file_size-file_pos+buffer) : (buffer+buffer_size))

/* Is the entire file being shown in one screen? */
#define ONE_SCREEN_FITS_ALL() \
 (next_screen_ptr==NULL && screen_top_ptr==buffer && BUFFER_BOF())

/* Is a scrollbar called for on the current screen? */
#define NEED_SCROLLBAR() \
 ((!(ONE_SCREEN_FITS_ALL())) && (prefs.scrollbar_mode==SB_ON))

/* variable button definitions */

/* Recorder keys */
#if CONFIG_KEYPAD == RECORDER_PAD
#define VIEWER_QUIT BUTTON_OFF
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_F1
#define VIEWER_AUTOSCROLL BUTTON_PLAY
#define VIEWER_LINE_UP (BUTTON_ON | BUTTON_UP)
#define VIEWER_LINE_DOWN (BUTTON_ON | BUTTON_DOWN)
#define VIEWER_COLUMN_LEFT (BUTTON_ON | BUTTON_LEFT)
#define VIEWER_COLUMN_RIGHT (BUTTON_ON | BUTTON_RIGHT)

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define VIEWER_QUIT BUTTON_OFF
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_F1
#define VIEWER_AUTOSCROLL BUTTON_SELECT
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
#define VIEWER_MENU (BUTTON_MENU|BUTTON_REPEAT)
#define VIEWER_AUTOSCROLL_PRE BUTTON_MENU
#define VIEWER_AUTOSCROLL (BUTTON_MENU|BUTTON_REL)

/* Player keys */
#elif CONFIG_KEYPAD == PLAYER_PAD
#define VIEWER_QUIT BUTTON_STOP
#define VIEWER_PAGE_UP BUTTON_LEFT
#define VIEWER_PAGE_DOWN BUTTON_RIGHT
#define VIEWER_SCREEN_LEFT (BUTTON_ON|BUTTON_LEFT)
#define VIEWER_SCREEN_RIGHT (BUTTON_ON|BUTTON_RIGHT)
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_PLAY

/* iRiver H1x0 && H3x0 keys */
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define VIEWER_QUIT BUTTON_OFF
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MODE
#define VIEWER_AUTOSCROLL BUTTON_SELECT
#define VIEWER_LINE_UP (BUTTON_ON | BUTTON_UP)
#define VIEWER_LINE_DOWN (BUTTON_ON | BUTTON_DOWN)
#define VIEWER_COLUMN_LEFT (BUTTON_ON | BUTTON_LEFT)
#define VIEWER_COLUMN_RIGHT (BUTTON_ON | BUTTON_RIGHT)

#define VIEWER_RC_QUIT BUTTON_RC_STOP

/* iPods */
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define VIEWER_QUIT_PRE BUTTON_SELECT
#define VIEWER_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define VIEWER_PAGE_UP BUTTON_SCROLL_BACK
#define VIEWER_PAGE_DOWN BUTTON_SCROLL_FWD
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_PLAY

/* iFP7xx keys */
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define VIEWER_QUIT BUTTON_PLAY
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MODE
#define VIEWER_AUTOSCROLL BUTTON_SELECT

/* iAudio X5 keys */
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_SELECT
#define VIEWER_AUTOSCROLL BUTTON_PLAY

/* GIGABEAT keys */
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_A

/* Sansa E200 keys */
#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_SELECT
#define VIEWER_AUTOSCROLL BUTTON_REC
#define VIEWER_LINE_UP BUTTON_SCROLL_BACK
#define VIEWER_LINE_DOWN BUTTON_SCROLL_FWD

/* Sansa Fuze keys */
#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define VIEWER_QUIT         BUTTON_POWER
#define VIEWER_PAGE_UP      BUTTON_UP
#define VIEWER_PAGE_DOWN    BUTTON_DOWN
#define VIEWER_SCREEN_LEFT  BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU         BUTTON_SELECT|BUTTON_REPEAT
#define VIEWER_AUTOSCROLL   BUTTON_SELECT|BUTTON_DOWN
#define VIEWER_LINE_UP      BUTTON_SCROLL_BACK
#define VIEWER_LINE_DOWN    BUTTON_SCROLL_FWD

/* Sansa C200 keys */
#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_VOL_UP
#define VIEWER_PAGE_DOWN BUTTON_VOL_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_SELECT
#define VIEWER_AUTOSCROLL BUTTON_REC
#define VIEWER_LINE_UP BUTTON_UP
#define VIEWER_LINE_DOWN BUTTON_DOWN

/* Sansa Clip keys */
#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_VOL_UP
#define VIEWER_PAGE_DOWN BUTTON_VOL_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_SELECT
#define VIEWER_AUTOSCROLL BUTTON_HOME
#define VIEWER_LINE_UP BUTTON_UP
#define VIEWER_LINE_DOWN BUTTON_DOWN

/* Sansa M200 keys */
#elif CONFIG_KEYPAD == SANSA_M200_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_VOL_UP
#define VIEWER_PAGE_DOWN BUTTON_VOL_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU  (BUTTON_SELECT | BUTTON_UP)
#define VIEWER_AUTOSCROLL (BUTTON_SELECT | BUTTON_REL)
#define VIEWER_LINE_UP BUTTON_UP
#define VIEWER_LINE_DOWN BUTTON_DOWN

/* iriver H10 keys */
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_SCROLL_UP
#define VIEWER_PAGE_DOWN BUTTON_SCROLL_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_REW
#define VIEWER_AUTOSCROLL BUTTON_PLAY

/*M-Robe 500 keys */
#elif CONFIG_KEYPAD == MROBE500_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_RC_PLAY
#define VIEWER_PAGE_DOWN BUTTON_RC_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_RC_HEART
#define VIEWER_AUTOSCROLL BUTTON_RC_MODE

/*Gigabeat S keys */
#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define VIEWER_QUIT BUTTON_BACK
#define VIEWER_PAGE_UP BUTTON_PREV
#define VIEWER_PAGE_DOWN BUTTON_NEXT
#define VIEWER_SCREEN_LEFT (BUTTON_PLAY | BUTTON_LEFT)
#define VIEWER_SCREEN_RIGHT (BUTTON_PLAY | BUTTON_RIGHT)
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL_PRE BUTTON_PLAY
#define VIEWER_AUTOSCROLL (BUTTON_PLAY|BUTTON_REL)
#define VIEWER_LINE_UP BUTTON_UP
#define VIEWER_LINE_DOWN BUTTON_DOWN
#define VIEWER_COLUMN_LEFT BUTTON_LEFT
#define VIEWER_COLUMN_RIGHT BUTTON_RIGHT

/*M-Robe 100 keys */
#elif CONFIG_KEYPAD == MROBE100_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_DISPLAY

/* iAUdio M3 keys */
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define VIEWER_QUIT BUTTON_RC_REC
#define VIEWER_PAGE_UP BUTTON_RC_VOL_UP
#define VIEWER_PAGE_DOWN BUTTON_RC_VOL_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_RC_REW
#define VIEWER_SCREEN_RIGHT BUTTON_RC_FF
#define VIEWER_MENU BUTTON_RC_MENU
#define VIEWER_AUTOSCROLL BUTTON_RC_MODE
#define VIEWER_RC_QUIT BUTTON_REC

/* Cowon D2 keys */
#elif CONFIG_KEYPAD == COWOND2_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_MENU BUTTON_MENU

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_VOLUP
#define VIEWER_PAGE_DOWN BUTTON_VOLDOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_PLAY
#define VIEWER_RC_QUIT BUTTON_STOP

/* Creative Zen Vision:M keys */
#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define VIEWER_QUIT BUTTON_BACK
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_SELECT

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef VIEWER_QUIT
#define VIEWER_QUIT         BUTTON_TOPLEFT
#endif
#ifndef VIEWER_PAGE_UP
#define VIEWER_PAGE_UP      BUTTON_TOPMIDDLE
#endif
#ifndef VIEWER_PAGE_DOWN
#define VIEWER_PAGE_DOWN    BUTTON_BOTTOMMIDDLE
#endif
#ifndef VIEWER_SCREEN_LEFT
#define VIEWER_SCREEN_LEFT  BUTTON_MIDLEFT
#endif
#ifndef VIEWER_SCREEN_RIGHT
#define VIEWER_SCREEN_RIGHT BUTTON_MIDRIGHT
#endif
#ifndef VIEWER_MENU
#define VIEWER_MENU         BUTTON_TOPRIGHT
#endif
#ifndef VIEWER_AUTOSCROLL
#define VIEWER_AUTOSCROLL   BUTTON_CENTER
#endif
#endif

/* stuff for the bookmarking */
struct bookmarked_file_info {
    long file_position;
    int  top_ptr_pos;
    char filename[MAX_PATH];
};

struct bookmark_file_data {
    signed int bookmarked_files_count;
    struct bookmarked_file_info bookmarks[];
};

struct preferences {
    enum {
        WRAP=0,
        CHOP,
    } word_mode;

    enum {
        NORMAL=0,
        JOIN,
        EXPAND,
        REFLOW, /* won't be set on charcell LCD, must be last */
    } line_mode;

    enum {
        NARROW=0,
        WIDE,
    } view_mode;

    enum codepages encoding;

#ifdef HAVE_LCD_BITMAP
    enum {
        SB_OFF=0,
        SB_ON,
    } scrollbar_mode;
    bool need_scrollbar;

    enum {
        NO_OVERLAP=0,
        OVERLAP,
    } page_mode;
#endif /* HAVE_LCD_BITMAP */

    enum {
        PAGE=0,
        LINE,
    } scroll_mode;

    int autoscroll_speed;
    
};

struct preferences prefs;
struct preferences old_prefs;

static unsigned char *buffer;
static long buffer_size;
static unsigned char line_break[] = {0,0x20,9,0xB,0xC,'-'};
static int display_columns; /* number of (pixel) columns on the display */
static int display_lines; /* number of lines on the display */
static int draw_columns; /* number of (pixel) columns available for text */
static int par_indent_spaces; /* number of spaces to indent first paragraph */
static int fd;
static const char *file_name;
static long file_size;
static long start_position; /* position in the file after the viewer is started */
static bool mac_text;
static long file_pos; /* Position of the top of the buffer in the file */
static unsigned char *buffer_end; /*Set to BUFFER_END() when file_pos changes*/
static int max_line_len;
static unsigned char *screen_top_ptr;
static unsigned char *next_screen_ptr;
static unsigned char *next_screen_to_draw_ptr;
static unsigned char *next_line_ptr;
static const struct plugin_api* rb;
#ifdef HAVE_LCD_BITMAP
static struct font *pf;
#endif


int glyph_width(int ch)
{
    if (ch == 0)
        ch = ' ';

#ifdef HAVE_LCD_BITMAP
    return rb->font_get_width(pf, ch);
#else
    return 1;
#endif
}

unsigned char* get_ucs(const unsigned char* str, unsigned short* ch)
{
    unsigned char utf8_tmp[6];
    int count;

    if (prefs.encoding == UTF_8)
        return (unsigned char*)rb->utf8decode(str, ch);

    count = BUFFER_OOB(str+2)? 1:2;
    rb->iso_decode(str, utf8_tmp, prefs.encoding, count);
    rb->utf8decode(utf8_tmp, ch);

#ifdef HAVE_LCD_BITMAP
    if ((prefs.encoding == SJIS && *str > 0xA0 && *str < 0xE0) || prefs.encoding < SJIS)
        return (unsigned char*)str+1;
    else
#endif
        return (unsigned char*)str+2;
}

bool done = false;
int col = 0;

#define ADVANCE_COUNTERS(c) { width += glyph_width(c); k++; }
#define LINE_IS_FULL ((k>=MAX_COLUMNS-1) ||( width >= draw_columns))
#define LINE_IS_NOT_FULL ((k<MAX_COLUMNS-1) &&( width < draw_columns))
static unsigned char* crop_at_width(const unsigned char* p)
{
    int k,width;
    unsigned short ch;
    const unsigned char *oldp = p;

    k=width=0;

    while (LINE_IS_NOT_FULL) {
        oldp = p;
        p = get_ucs(p, &ch);
        ADVANCE_COUNTERS(ch);
    }

    return (unsigned char*)oldp;
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

    k = (prefs.line_mode==JOIN) || (prefs.line_mode==REFLOW) ? 0:1;

    if (!BUFFER_OOB(&p[size]))
        for (j=k; j < ((int) sizeof(line_break)) - 1; j++)
            if (p[size] == line_break[j])
                return (unsigned char*) p+size;

    for (i=size-1; i>=0; i--)
        for (j=k; j < (int) sizeof(line_break); j++)
        {
            if (!((p[i] == '-') && (prefs.word_mode == WRAP)))
                if (p[i] == line_break[j])
                    return (unsigned char*) p+i;
        }

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

    if (prefs.view_mode == WIDE) {
        search_len = MAX_WIDTH;
    }
    else {   /* prefs.view_mode == NARROW */
        search_len = crop_at_width(cur_line) - cur_line;
    }

    size = BUFFER_OOB(cur_line+search_len) ? buffer_end-cur_line : search_len;

    if ((prefs.line_mode == JOIN) || (prefs.line_mode == REFLOW)) {
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
                    if (prefs.line_mode == REFLOW) {
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
                    if (prefs.line_mode==JOIN || newlines>0) {
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
            if (prefs.word_mode == WRAP)  /* Find last space */
                next_line = find_last_space(cur_line, size);

            if (next_line == NULL)
                next_line = crop_at_width(cur_line);
            else
                if (prefs.word_mode == WRAP)
                    for (i=0;
                    i<WRAP_TRIM && isspace(next_line[0]) && !BUFFER_OOB(next_line);
                    i++)
                        next_line++;
        }

    if (prefs.line_mode == EXPAND)
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
        if (next_line[0] == 0)/* ||
        (next_line[0] == '-' && next_line-cur_line < draw_columns)) */
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

    if ((prefs.line_mode == JOIN) || (prefs.line_mode == REFLOW))
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
        if (prefs.view_mode == WIDE) {
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

static void viewer_scroll_down(void)
{
    if (next_screen_ptr != NULL)
        screen_top_ptr = next_line_ptr;
}

#ifdef HAVE_LCD_BITMAP
static void viewer_scrollbar(void) {
    int items, min_shown, max_shown;

    items = (int) file_size;  /* (SH1 int is same as long) */
    min_shown = (int) file_pos + (screen_top_ptr - buffer);

    if (next_screen_ptr == NULL)
        max_shown = items;
    else
        max_shown = min_shown + (next_screen_ptr - screen_top_ptr);

    rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],0, 0, SCROLLBAR_WIDTH-1,
                         LCD_HEIGHT, items, min_shown, max_shown, VERTICAL);
}
#endif

static void viewer_draw(int col)
{
    int i, j, k, line_len, line_width, resynch_move, spaces, left_col=0;
    int width, extra_spaces, indent_spaces, spaces_per_word;
    bool multiple_spacing, line_is_short;
    unsigned short ch;
    unsigned char *str, *oldstr;
    unsigned char *line_begin;
    unsigned char *line_end;
    unsigned char c;
    unsigned char scratch_buffer[MAX_COLUMNS + 1];
    unsigned char utf8_buffer[MAX_COLUMNS*4 + 1];
    unsigned char *endptr;

    /* If col==-1 do all calculations but don't display */
    if (col != -1) {
#ifdef HAVE_LCD_BITMAP
        left_col = prefs.need_scrollbar? SCROLLBAR_WIDTH:0;
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

        /* calculate line_len */
        str = oldstr = line_begin;
        j = -1;
        while (str < line_end) {
            oldstr = str;
            str = crop_at_width(str);
            j++;
        }
        line_width = j*draw_columns;
        while (oldstr < line_end) {
            oldstr = get_ucs(oldstr, &ch);
            line_width += glyph_width(ch);
        }

        if (prefs.line_mode == JOIN) {
            if (line_begin[0] == 0) {
                line_begin++;
                if (prefs.word_mode == CHOP)
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
            if (col != -1) {
                scratch_buffer[k] = 0;
                endptr = rb->iso_decode(scratch_buffer + col, utf8_buffer,
                                        prefs.encoding, draw_columns/glyph_width('i'));
                *endptr = 0;
            }
        }
        else if (prefs.line_mode == REFLOW) {
            if (line_begin[0] == 0) {
                line_begin++;
                if (prefs.word_mode == CHOP)
                    line_end++;
                else
                    line_len--;
            }

            indent_spaces = 0;
            if (!line_is_short) {
                multiple_spacing = false;
                width=spaces=0;
                for (str = line_begin; str < line_end; ) {
                    str = get_ucs(str, &ch);
                    switch (ch) {
                        case ' ':
                        case 0:
                            if ((str == line_begin) && (prefs.word_mode==WRAP))
                                /* special case: indent the paragraph,
                                 * don't count spaces */
                                indent_spaces = par_indent_spaces;
                            else if (!multiple_spacing)
                                spaces++;
                            multiple_spacing = true;
                            break;
                        default:
                            multiple_spacing = false;
                            width += glyph_width(ch);
                            break;
                    }
                }
                if (multiple_spacing) spaces--;

                if (spaces) {
                    /* total number of spaces to insert between words */
                    extra_spaces = (draw_columns-width)/glyph_width(' ')
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
                        if (j==0 && prefs.word_mode==WRAP) { /* indent paragraph */
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

            if (col != -1) {
                scratch_buffer[k] = 0;
                endptr = rb->iso_decode(scratch_buffer + col, utf8_buffer,
                                        prefs.encoding, k-col);
                *endptr = 0;
            }
        }
        else { /* prefs.line_mode != JOIN && prefs.line_mode != REFLOW */
            if (col != -1)
                if (line_width > col) {
                    str = oldstr = line_begin;
                    k = col;
                    width = 0;
                    while( (width<draw_columns) && (oldstr<line_end) )
                    {
                        oldstr = get_ucs(oldstr, &ch);
                        if (k > 0) {
                            k -= glyph_width(ch);
                            line_begin = oldstr;
                        } else {
                            width += glyph_width(ch);
                        }
                    }

                    if(prefs.view_mode==WIDE)
                        endptr = rb->iso_decode(line_begin, utf8_buffer,
                                            prefs.encoding, oldstr-line_begin);
                    else
                        endptr = rb->iso_decode(line_begin, utf8_buffer,
                                            prefs.encoding, line_end-line_begin);
                    *endptr = 0;
                }
        }
        if (col != -1 && line_width > col)
#ifdef HAVE_LCD_BITMAP
            rb->lcd_putsxy(left_col, i*pf->height, utf8_buffer);
#else
            rb->lcd_puts(left_col, i, utf8_buffer);
#endif
        if (line_width > max_line_len)
            max_line_len = line_width;

        if (i == 0)
            next_line_ptr = line_end;
    }
    next_screen_ptr = line_end;
    if (BUFFER_OOB(next_screen_ptr))
        next_screen_ptr = NULL;

#ifdef HAVE_LCD_BITMAP
    next_screen_to_draw_ptr = prefs.page_mode==OVERLAP? line_begin: next_screen_ptr;

    if (prefs.need_scrollbar)
        viewer_scrollbar();
#else
    next_screen_to_draw_ptr = next_screen_ptr;
#endif

    if (col != -1)
        rb->lcd_update();
}

static void viewer_top(void)
{
    /* Read top of file into buffer
      and point screen pointer to top */
    if (file_pos != 0)
    {
        file_pos = 0;
        buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
        fill_buffer(0, buffer, buffer_size);
    }

    screen_top_ptr = buffer;
}

static void viewer_bottom(void)
{
    /* Read bottom of file into buffer
      and point screen pointer to bottom */
    long last_sectors;

    if (file_size > buffer_size) {
        /* Find last buffer in file, round up to next sector boundary */
        last_sectors = file_size - buffer_size + SMALL_BLOCK_SIZE;
        last_sectors /= SMALL_BLOCK_SIZE;
        last_sectors *= SMALL_BLOCK_SIZE;
    }
    else {
        last_sectors = 0;
    }

    if (file_pos != last_sectors)
    {
        file_pos = last_sectors;
        buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
        fill_buffer(last_sectors, buffer, buffer_size);
    }

    screen_top_ptr = buffer_end-1;
}

#ifdef HAVE_LCD_BITMAP
static void init_need_scrollbar(void) {
    /* Call viewer_draw in quiet mode to initialize next_screen_ptr,
     and thus ONE_SCREEN_FITS_ALL(), and thus NEED_SCROLLBAR() */
    viewer_draw(-1);
    prefs.need_scrollbar = NEED_SCROLLBAR();
    draw_columns = prefs.need_scrollbar? display_columns-SCROLLBAR_WIDTH : display_columns;
    par_indent_spaces = draw_columns/(5*glyph_width(' '));
}
#else
#define init_need_scrollbar()
#endif

static bool viewer_init(void)
{
#ifdef HAVE_LCD_BITMAP

    pf = rb->font_get(FONT_UI);

    display_lines = LCD_HEIGHT / pf->height;
    draw_columns = display_columns = LCD_WIDTH;
#else
    /* REAL fixed pitch :) all chars use up 1 cell */
    display_lines = 2;
    draw_columns = display_columns = 11;
    par_indent_spaces = 2;
#endif

    fd = rb->open(file_name, O_RDONLY);
    if (fd==-1)
        return false;

    file_size = rb->filesize(fd);
    if (file_size==-1)
        return false;

    /* Init mac_text value used in processing buffer */
    mac_text = false;

    return true;
}

static void viewer_default_settings(void)
{
    prefs.word_mode = WRAP;
    prefs.line_mode = NORMAL;
    prefs.view_mode = NARROW;
    prefs.scroll_mode = PAGE;
#ifdef HAVE_LCD_BITMAP
    prefs.page_mode = NO_OVERLAP;
    prefs.scrollbar_mode = SB_OFF;
#endif
    prefs.autoscroll_speed = 1;
    /* Set codepage to system default */
    prefs.encoding = rb->global_settings->default_codepage;
}

static void viewer_load_settings(void) /* same name as global, but not the same file.. */
{
    int settings_fd, i;
    struct bookmark_file_data *data;
    struct bookmarked_file_info this_bookmark;
    
    /* read settings file */
    settings_fd=rb->open(SETTINGS_FILE, O_RDONLY);
    if ((settings_fd >= 0) && (rb->filesize(settings_fd) == sizeof(struct preferences)))
    {
        rb->read(settings_fd, &prefs, sizeof(struct preferences));
        rb->close(settings_fd);
    }
    else
    {
        /* load default settings if there is no settings file */
        viewer_default_settings();
    }

    rb->memcpy(&old_prefs, &prefs, sizeof(struct preferences));

    data = (struct bookmark_file_data*)buffer; /* grab the text buffer */
    data->bookmarked_files_count = 0;

    /* read bookmarks if file exists */
    settings_fd = rb->open(BOOKMARKS_FILE, O_RDONLY);
    if (settings_fd >= 0)
    {
        /* figure out how many items to read */
        rb->read(settings_fd, &data->bookmarked_files_count, sizeof(signed int)); 
        if (data->bookmarked_files_count > MAX_BOOKMARKED_FILES)
            data->bookmarked_files_count = MAX_BOOKMARKED_FILES;
        rb->read(settings_fd, data->bookmarks, 
                 sizeof(struct bookmarked_file_info) * data->bookmarked_files_count);
        rb->close(settings_fd);
    }

    file_pos = 0;
    screen_top_ptr = buffer;

    /* check if current file is in list */
    for (i=0; i < data->bookmarked_files_count; i++)
    {
        if (!rb->strcmp(file_name, data->bookmarks[i].filename)) 
        {
            int screen_pos = data->bookmarks[i].file_position + data->bookmarks[i].top_ptr_pos;
            int screen_top = screen_pos % buffer_size;
            file_pos = screen_pos - screen_top;
            screen_top_ptr = buffer + screen_top;
            break;
        }    
    }

    this_bookmark.file_position = file_pos;
    this_bookmark.top_ptr_pos = screen_top_ptr - buffer;

    rb->memset(&this_bookmark.filename[0],0,MAX_PATH);
    rb->strcpy(this_bookmark.filename,file_name);

    /* prevent potential slot overflow */
    if (i >= data->bookmarked_files_count) 
    {
        if (i < MAX_BOOKMARKED_FILES) 
            data->bookmarked_files_count++;    
        else        
            i = MAX_BOOKMARKED_FILES-1;
    }    

    /* write bookmark file with spare slot in first position 
       to be filled in by viewer_save_settings */
    settings_fd = rb->open(BOOKMARKS_FILE, O_WRONLY|O_CREAT);
    if (settings_fd >=0 )
    {     
        /* write count */
        rb->write (settings_fd, &data->bookmarked_files_count, sizeof(signed int));

        /* write the current bookmark */
        rb->write (settings_fd, &this_bookmark, sizeof(struct bookmarked_file_info));

        /* write everything that was before this bookmark */
        rb->write (settings_fd, data->bookmarks, sizeof(struct bookmarked_file_info)*i);

        rb->close(settings_fd);
    }

    buffer_end = BUFFER_END();  /* Update whenever file_pos changes */

    if (BUFFER_OOB(screen_top_ptr)) 
    {
        screen_top_ptr = buffer;
    }

    fill_buffer(file_pos, buffer, buffer_size);

    /* remember the current position */
    start_position = file_pos + screen_top_ptr - buffer;

    init_need_scrollbar();
}

static void viewer_save_settings(void)/* same name as global, but not the same file.. */
{
    int settings_fd;

    /* save the viewer settings if they have been changed */
    if (rb->memcmp(&prefs, &old_prefs, sizeof(struct preferences)))
    {
        settings_fd = rb->creat(SETTINGS_FILE); /* create the settings file */

        if (settings_fd >= 0 )
        {
            rb->write (settings_fd, &prefs, sizeof(struct preferences));
            rb->close(settings_fd);
        }
    }
    
    /* save the bookmark if the position has changed */
    if (file_pos + screen_top_ptr - buffer != start_position)
    {
        settings_fd = rb->open(BOOKMARKS_FILE, O_WRONLY|O_CREAT);

        if (settings_fd >= 0 )
        {
            struct bookmarked_file_info b;
            b.file_position = file_pos + screen_top_ptr - buffer;
            b.top_ptr_pos = 0; /* this is only kept for legassy reasons */
            rb->memset(&b.filename[0],0,MAX_PATH);
            rb->strcpy(b.filename,file_name);
            rb->lseek(settings_fd,sizeof(signed int),SEEK_SET);
            rb->write (settings_fd, &b, sizeof(struct bookmarked_file_info));
            rb->close(settings_fd);
        }
    }
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
        if (col > max_line_len - 2*glyph_width('o'))
            col = max_line_len - 2*glyph_width('o');

    return col;
}

/* settings helper functions */

static bool encoding_setting(void)
{
    static struct opt_items names[NUM_CODEPAGES];
    int idx;

    for (idx = 0; idx < NUM_CODEPAGES; idx++)
    {
        names[idx].string = rb->get_codepage_name(idx);
        names[idx].voice_id = -1;
    }

    return rb->set_option("Encoding", &prefs.encoding, INT, names,
                          sizeof(names) / sizeof(names[0]), NULL);
}

static bool word_wrap_setting(void)
{
    static const struct opt_items names[] = {
        {"On",               -1},
        {"Off (Chop Words)", -1},
    };

    return rb->set_option("Word Wrap", &prefs.word_mode, INT,
                          names, 2, NULL);
}

static bool line_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Normal",       -1},
        {"Join Lines",   -1},
        {"Expand Lines", -1},
#ifdef HAVE_LCD_BITMAP
        {"Reflow Lines", -1},
#endif
    };

    return rb->set_option("Line Mode", &prefs.line_mode, INT, names,
                          sizeof(names) / sizeof(names[0]), NULL);
}

static bool view_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"No (Narrow)", -1},
        {"Yes",         -1},
    };
    bool ret;
    ret = rb->set_option("Wide View", &prefs.view_mode, INT,
                           names , 2, NULL);
    if (prefs.view_mode == NARROW)
        col = 0;
    return ret;
}

static bool scroll_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"Scroll by Page", -1},
        {"Scroll by Line", -1},
    };

    return rb->set_option("Scroll Mode", &prefs.scroll_mode, INT,
                          names, 2, NULL);
}

#ifdef HAVE_LCD_BITMAP
static bool page_mode_setting(void)
{
    static const struct opt_items names[] = {
        {"No",  -1},
        {"Yes", -1},
    };

    return rb->set_option("Overlap Pages", &prefs.page_mode, INT,
                           names, 2, NULL);
}

static bool scrollbar_setting(void)
{
    static const struct opt_items names[] = {
        {"Off", -1},
        {"On",  -1}
    };

    return rb->set_option("Show Scrollbar", &prefs.scrollbar_mode, INT,
                           names, 2, NULL);
}
#endif

static bool autoscroll_speed_setting(void)
{
    return rb->set_int("Auto-scroll Speed", "", UNIT_INT, 
                       &prefs.autoscroll_speed, NULL, 1, 1, 10, NULL);
}

static bool viewer_options_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        {"Encoding",          encoding_setting },
        {"Word Wrap",         word_wrap_setting },
        {"Line Mode",         line_mode_setting },
        {"Wide View",         view_mode_setting },
#ifdef HAVE_LCD_BITMAP
        {"Show Scrollbar",    scrollbar_setting },
        {"Overlap Pages",     page_mode_setting },
#endif
        {"Scroll Mode",       scroll_mode_setting},
        {"Auto-Scroll Speed", autoscroll_speed_setting },
    };
    m = menu_init(rb, items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    result = menu_run(m);
    menu_exit(m);
#ifdef HAVE_LCD_BITMAP

    /* Show-scrollbar mode for current view-width mode */
    init_need_scrollbar();
#endif
    return result;
}

static void viewer_menu(void)
{
    int m;
    int result;
    static const struct menu_item items[] = {
        {"Quit", NULL },
        {"Viewer Options", NULL },
        {"Show Playback Menu", NULL },
        {"Return", NULL },
    };

    m = menu_init(rb, items, sizeof(items) / sizeof(*items), NULL, NULL, NULL, NULL);
    result=menu_show(m);
    switch (result)
    {
        case 0: /* quit */
            menu_exit(m);
            viewer_exit(NULL);
            done = true;
            break;
        case 1: /* change settings */
            done = viewer_options_menu();
            break;
        case 2: /* playback control */
            playback_control(rb, NULL);
            break;
        case 3: /* return */
            break;
    }
    menu_exit(m);
    viewer_draw(col);
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* file)
{
    int button, i, ok;
    int lastbutton = BUTTON_NONE;
    bool autoscroll = false;
    long old_tick;

    rb = api;
    old_tick = *rb->current_tick;

    /* get the plugin buffer */
    buffer = rb->plugin_get_buffer((size_t *)&buffer_size);

    if (!file)
        return PLUGIN_ERROR;

    file_name = file;
    ok = viewer_init();
    if (!ok) {
        rb->splash(HZ, "Error opening file.");
        return PLUGIN_ERROR;
    }

    viewer_load_settings(); /* load the preferences and bookmark */
    
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    
    viewer_draw(col);

    while (!done) {

        if(autoscroll)
        {
            if(old_tick <= *rb->current_tick - (110-prefs.autoscroll_speed*10))
            {
                viewer_scroll_down();
                viewer_draw(col);
                old_tick = *rb->current_tick;
            }
        }
        
        button = rb->button_get_w_tmo(HZ/10);
        switch (button) {
            case VIEWER_MENU:
                viewer_menu();
                break;

            case VIEWER_AUTOSCROLL:
#ifdef VIEWER_AUTOSCROLL_PRE
                if (lastbutton != VIEWER_AUTOSCROLL_PRE)
                    break;
#endif
                autoscroll = !autoscroll;
                break;

            case VIEWER_PAGE_UP:
            case VIEWER_PAGE_UP | BUTTON_REPEAT:
                if (prefs.scroll_mode == PAGE)
                {
                    /* Page up */
#ifdef HAVE_LCD_BITMAP
                    for (i = prefs.page_mode==OVERLAP? 1:0; i < display_lines; i++)
#else
                    for (i = 0; i < display_lines; i++)
#endif
                        viewer_scroll_up();
                }
                else
                    viewer_scroll_up();
                old_tick = *rb->current_tick;
                viewer_draw(col);
                break;

            case VIEWER_PAGE_DOWN:
            case VIEWER_PAGE_DOWN | BUTTON_REPEAT:
                if (prefs.scroll_mode == PAGE)
                {
                    /* Page down */
                    if (next_screen_ptr != NULL)
                        screen_top_ptr = next_screen_to_draw_ptr;
                }
                else
                    viewer_scroll_down();
                old_tick = *rb->current_tick;
                viewer_draw(col);
                break;

            case VIEWER_SCREEN_LEFT:
            case VIEWER_SCREEN_LEFT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE) {
                    /* Screen left */
                    col -= draw_columns;
                    col = col_limit(col);
                }
                else {   /* prefs.view_mode == NARROW */
                    /* Top of file */
                    viewer_top();
                }

                viewer_draw(col);
                break;

            case VIEWER_SCREEN_RIGHT:
            case VIEWER_SCREEN_RIGHT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE) {
                    /* Screen right */
                    col += draw_columns;
                    col = col_limit(col);
                }
                else {   /* prefs.view_mode == NARROW */
                    /* Bottom of file */
                    viewer_bottom();
                }

                viewer_draw(col);
                break;

#ifdef VIEWER_LINE_UP
            case VIEWER_LINE_UP:
            case VIEWER_LINE_UP | BUTTON_REPEAT:
                /* Scroll up one line */
                viewer_scroll_up();
                old_tick = *rb->current_tick;
                viewer_draw(col);
                break;

            case VIEWER_LINE_DOWN:
            case VIEWER_LINE_DOWN | BUTTON_REPEAT:
                /* Scroll down one line */
                if (next_screen_ptr != NULL)
                    screen_top_ptr = next_line_ptr;
                old_tick = *rb->current_tick;
                viewer_draw(col);
                break;
#endif
#ifdef VIEWER_COLUMN_LEFT
            case VIEWER_COLUMN_LEFT:
            case VIEWER_COLUMN_LEFT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE) {
                    /* Scroll left one column */
                    col -= glyph_width('o');
                    col = col_limit(col);
                    viewer_draw(col);
                }
                break;

            case VIEWER_COLUMN_RIGHT:
            case VIEWER_COLUMN_RIGHT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE) {
                    /* Scroll right one column */
                    col += glyph_width('o');
                    col = col_limit(col);
                    viewer_draw(col);
                }
                break;
#endif

#ifdef VIEWER_RC_QUIT
            case VIEWER_RC_QUIT:
#endif
            case VIEWER_QUIT:
                viewer_exit(NULL);
                done = true;
                break;

            default:
                if (rb->default_event_handler_ex(button, viewer_exit, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button != BUTTON_NONE)
        {
            lastbutton = button;
            rb->yield();
        }
    }
    return PLUGIN_OK;
}


