/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

PLUGIN_HEADER

/* global settings file
 * binary file, so dont use .cfg
 *
 * setting file format
 *
 * part                byte count
 * --------------------------------
 * 'TVGS'               4
 * version              1
 * word_mode            1
 * line_mode            1
 * view_mode            1
 * alignment            1
 * encoding             1
 * scrollbar_mode       1
 * need_scrollbar       1
 * page_mode            1
 * page_number_mode     1
 * title_mode           1
 * scroll_mode          1
 * autoscroll_speed     1
 * font name            MAX_PATH
 */
#define GLOBAL_SETTINGS_FILE     VIEWERS_DIR "/viewer.dat"

/* temporary file */
#define GLOBAL_SETTINGS_TMP_FILE VIEWERS_DIR "/viewer_file.tmp"

#define GLOBAL_SETTINGS_HEADER   "\x54\x56\x47\x53" /* header="TVGS" */
#define GLOBAL_SETTINGS_H_SIZE   5
#define GLOBAL_SETTINGS_VERSION       0x32          /* version=2 */
#define GLOBAL_SETTINGS_FIRST_VERSION 0x31

/* preferences and bookmarks at each file
 * binary file, so dont use .cfg
 *
 * setting file format
 *
 * part                byte count
 * --------------------------------
 * 'TVS'                3
 * version              1
 * file count           2
 * [1st file]
 *   file path          MAX_PATH
 *   next file pos      2
 *   [preferences]
 *     word_mode        1
 *     line_mode        1
 *     view_mode        1
 *     alignment        1
 *     encoding         1
 *     scrollbar_mode   1
 *     need_scrollbar   1
 *     page_mode        1
 *     header_mode      1
 *     footer_mode      1
 *     scroll_mode      1
 *     autoscroll_speed 1
 *     font name        MAX_PATH
 *   bookmark count     1
 *   [1st bookmark]
 *     file_position    4
 *     page             2
 *     line             1
 *     flag             1
 *   [2nd bookmark]
 *   ...
 *   [last bookmark]
 * [2nd file]
 * ...
 * [last file]
 */
#define SETTINGS_FILE        VIEWERS_DIR "/viewer_file.dat"

/* temporary file */
#define SETTINGS_TMP_FILE    VIEWERS_DIR "/viewer_file.tmp"

#define SETTINGS_HEADER        "\x54\x56\x53" /* header="TVS" */
#define SETTINGS_H_SIZE        4
#define SETTINGS_VERSION       0x33           /* version=3 */
#define SETTINGS_FIRST_VERSION 0x32

#define WRAP_TRIM          44  /* Max number of spaces to trim (arbitrary) */
#define NARROW_MAX_COLUMNS 64  /* Max displayable string len [narrow] (over-estimate) */
#define WIDE_MAX_COLUMNS  128  /* Max displayable string len [wide] (over-estimate) */
#define MAX_WIDTH         910  /* Max line length in WIDE mode */
#define READ_PREV_ZONE    (block_size*9/10)  /* Arbitrary number less than SMALL_BLOCK_SIZE */
#define SMALL_BLOCK_SIZE  block_size /* Smallest file chunk we will read */
#define LARGE_BLOCK_SIZE  (block_size << 1) /* Preferable size of file chunk to read */
#define TOP_SECTOR     buffer
#define MID_SECTOR     (buffer + SMALL_BLOCK_SIZE)
#define BOTTOM_SECTOR  (buffer + (SMALL_BLOCK_SIZE << 1))
#undef SCROLLBAR_WIDTH
#define SCROLLBAR_WIDTH rb->global_settings->scrollbar_width
#define MAX_PAGE         9999

#define BOOKMARK_SIZE       8
#define MAX_BOOKMARKS      10 /* user setting bookmarks + last read page */

#define BOOKMARK_LAST       1
#define BOOKMARK_USER       2

#ifndef HAVE_LCD_BITMAP
#define BOOKMARK_ICON       "\xee\x84\x81\x00"
#endif

#define PREFERENCES_SIZE    (12 + MAX_PATH)

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
#define VIEWER_BOOKMARK BUTTON_F2

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
#define VIEWER_BOOKMARK BUTTON_F2

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
#define VIEWER_BOOKMARK (BUTTON_MENU|BUTTON_OFF)

/* Player keys */
#elif CONFIG_KEYPAD == PLAYER_PAD
#define VIEWER_QUIT BUTTON_STOP
#define VIEWER_PAGE_UP BUTTON_LEFT
#define VIEWER_PAGE_DOWN BUTTON_RIGHT
#define VIEWER_SCREEN_LEFT (BUTTON_ON|BUTTON_LEFT)
#define VIEWER_SCREEN_RIGHT (BUTTON_ON|BUTTON_RIGHT)
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_PLAY
#define VIEWER_BOOKMARK BUTTON_ON

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
#define VIEWER_BOOKMARK (BUTTON_ON | BUTTON_SELECT)

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
#define VIEWER_BOOKMARK BUTTON_SELECT

/* iFP7xx keys */
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define VIEWER_QUIT BUTTON_PLAY
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MODE
#define VIEWER_AUTOSCROLL BUTTON_SELECT
#define VIEWER_BOOKMARK (BUTTON_LEFT|BUTTON_SELECT)

/* iAudio X5 keys */
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_SELECT
#define VIEWER_AUTOSCROLL BUTTON_PLAY
#define VIEWER_BOOKMARK BUTTON_REC

/* GIGABEAT keys */
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_A
#define VIEWER_BOOKMARK BUTTON_SELECT

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
#define VIEWER_BOOKMARK (BUTTON_DOWN|BUTTON_SELECT)

/* Sansa Fuze keys */
#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define VIEWER_QUIT         (BUTTON_HOME|BUTTON_REPEAT)
#define VIEWER_PAGE_UP      BUTTON_UP
#define VIEWER_PAGE_DOWN    BUTTON_DOWN
#define VIEWER_SCREEN_LEFT  BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU         BUTTON_SELECT|BUTTON_REPEAT
#define VIEWER_AUTOSCROLL   BUTTON_SELECT|BUTTON_DOWN
#define VIEWER_LINE_UP      BUTTON_SCROLL_BACK
#define VIEWER_LINE_DOWN    BUTTON_SCROLL_FWD
#define VIEWER_BOOKMARK     BUTTON_SELECT

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
#define VIEWER_BOOKMARK (BUTTON_DOWN | BUTTON_SELECT)

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
#define VIEWER_BOOKMARK (BUTTON_DOWN|BUTTON_SELECT)

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
#define VIEWER_BOOKMARK (BUTTON_DOWN|BUTTON_SELECT)

/* iriver H10 keys */
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_SCROLL_UP
#define VIEWER_PAGE_DOWN BUTTON_SCROLL_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_REW
#define VIEWER_AUTOSCROLL BUTTON_PLAY
#define VIEWER_BOOKMARK BUTTON_FF

/*M-Robe 500 keys */
#elif CONFIG_KEYPAD == MROBE500_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_RC_PLAY
#define VIEWER_PAGE_DOWN BUTTON_RC_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_RC_HEART
#define VIEWER_AUTOSCROLL BUTTON_RC_MODE
#define VIEWER_BOOKMARK BUTTON_CENTER

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
#define VIEWER_BOOKMARK BUTTON_SELECT

/*M-Robe 100 keys */
#elif CONFIG_KEYPAD == MROBE100_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_DISPLAY
#define VIEWER_BOOKMARK BUTTON_SELECT

/* iAUdio M3 keys */
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define VIEWER_QUIT BUTTON_REC
#define VIEWER_PAGE_UP BUTTON_RC_VOL_UP
#define VIEWER_PAGE_DOWN BUTTON_RC_VOL_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_RC_REW
#define VIEWER_SCREEN_RIGHT BUTTON_RC_FF
#define VIEWER_MENU BUTTON_RC_MENU
#define VIEWER_AUTOSCROLL BUTTON_RC_MODE
#define VIEWER_RC_QUIT BUTTON_RC_REC
#define VIEWER_BOOKMARK BUTTON_RC_PLAY

/* Cowon D2 keys */
#elif CONFIG_KEYPAD == COWON_D2_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_PAGE_UP BUTTON_MINUS
#define VIEWER_PAGE_DOWN BUTTON_PLUS
#define VIEWER_BOOKMARK (BUTTON_MENU|BUTTON_PLUS)

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_VOLUP
#define VIEWER_PAGE_DOWN BUTTON_VOLDOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_PLAY
#define VIEWER_RC_QUIT BUTTON_STOP
#define VIEWER_BOOKMARK (BUTTON_LEFT|BUTTON_PLAY)

/* Creative Zen Vision:M keys */
#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define VIEWER_QUIT BUTTON_BACK
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_SELECT
#define VIEWER_BOOKMARK BUTTON_PLAY

/* Philips HDD1630 keys */
#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_VIEW
#define VIEWER_BOOKMARK BUTTON_SELECT

/* Philips SA9200 keys */
#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_PAGE_UP BUTTON_UP
#define VIEWER_PAGE_DOWN BUTTON_DOWN
#define VIEWER_SCREEN_LEFT BUTTON_PREV
#define VIEWER_SCREEN_RIGHT BUTTON_NEXT
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_AUTOSCROLL BUTTON_PLAY
#define VIEWER_BOOKMARK BUTTON_RIGHT

/* Onda VX747 keys */
#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_MENU BUTTON_MENU
#define VIEWER_BOOKMARK (BUTTON_RIGHT|BUTTON_POWER)

/* Onda VX777 keys */
#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define VIEWER_QUIT BUTTON_POWER
#define VIEWER_BOOKMARK (BUTTON_RIGHT|BUTTON_POWER)

/* SAMSUNG YH-820 / YH-920 / YH-925 keys */
#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define VIEWER_QUIT         BUTTON_REC
#define VIEWER_PAGE_UP      BUTTON_UP
#define VIEWER_PAGE_DOWN    BUTTON_DOWN
#define VIEWER_SCREEN_LEFT  BUTTON_LEFT
#define VIEWER_SCREEN_RIGHT BUTTON_RIGHT
#define VIEWER_MENU         BUTTON_PLAY
#define VIEWER_AUTOSCROLL   BUTTON_REW
#define VIEWER_BOOKMARK     BUTTON_FFWD

/* Packard Bell Vibe 500 keys */
#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define VIEWER_QUIT         BUTTON_REC
#define VIEWER_PAGE_UP      BUTTON_OK
#define VIEWER_PAGE_DOWN    BUTTON_CANCEL
#define VIEWER_LINE_UP      BUTTON_UP
#define VIEWER_LINE_DOWN    BUTTON_DOWN
#define VIEWER_SCREEN_LEFT  BUTTON_PREV
#define VIEWER_SCREEN_RIGHT BUTTON_NEXT
#define VIEWER_MENU         BUTTON_MENU
#define VIEWER_AUTOSCROLL   BUTTON_PLAY
#define VIEWER_BOOKMARK     BUTTON_POWER

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifdef VIEWER_QUIT
#define VIEWER_QUIT2        BUTTON_TOPLEFT
#else
#define VIEWER_QUIT         BUTTON_TOPLEFT
#endif
#ifdef VIEWER_PAGE_UP
#define VIEWER_PAGE_UP2     BUTTON_TOPMIDDLE
#else
#define VIEWER_PAGE_UP      BUTTON_TOPMIDDLE
#endif
#ifdef VIEWER_PAGE_DOWN
#define VIEWER_PAGE_DOWN2   BUTTON_BOTTOMMIDDLE
#else
#define VIEWER_PAGE_DOWN    BUTTON_BOTTOMMIDDLE
#endif
#ifndef VIEWER_SCREEN_LEFT
#define VIEWER_SCREEN_LEFT  BUTTON_MIDLEFT
#endif
#ifndef VIEWER_SCREEN_RIGHT
#define VIEWER_SCREEN_RIGHT BUTTON_MIDRIGHT
#endif
#ifdef VIEWER_MENU
#define VIEWER_MENU2        BUTTON_TOPRIGHT
#else
#define VIEWER_MENU         BUTTON_TOPRIGHT
#endif
#ifndef VIEWER_AUTOSCROLL
#define VIEWER_AUTOSCROLL   BUTTON_CENTER
#endif
#endif

/* stuff for the bookmarking */
struct bookmark_info {
    long file_position;
    int  page;
    int  line;
    unsigned char flag;
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

    enum {
        LEFT=0,
        RIGHT,
    } alignment;

    enum codepages encoding;

    enum {
        SB_OFF=0,
        SB_ON,
    } scrollbar_mode;
    bool need_scrollbar;

    enum {
        NO_OVERLAP=0,
        OVERLAP,
    } page_mode;

    enum {
        HD_NONE = 0,
        HD_PATH,
        HD_SBAR,
        HD_BOTH,
    } header_mode;

    enum {
        FT_NONE = 0,
        FT_PAGE,
        FT_SBAR,
        FT_BOTH,
    } footer_mode;

    enum {
        PAGE=0,
        LINE,
    } scroll_mode;

    int autoscroll_speed;

    unsigned char font[MAX_PATH];
};

enum {
    VIEWER_FONT_MENU = 0,
    VIEWER_FONT_TEXT,
};

struct preferences prefs;
struct preferences old_prefs;

static unsigned char *buffer;
static long buffer_size;
static long block_size = 0x1000;
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
static long last_file_pos;
static unsigned char *buffer_end; /*Set to BUFFER_END() when file_pos changes*/
static int max_line_len;
static int max_width;
static int max_columns;
static int cline = 1;
static int cpage = 1;
static int lpage = 0;
static unsigned char *screen_top_ptr;
static unsigned char *next_screen_ptr;
static unsigned char *next_screen_to_draw_ptr;
static unsigned char *next_line_ptr;
static unsigned char *last_screen_top_ptr = NULL;
#ifdef HAVE_LCD_BITMAP
static struct font *pf;
static int header_height = 0;
static int footer_height = 0;
#endif
struct bookmark_info bookmarks[MAX_BOOKMARKS];
static int bookmark_count;

/* UTF-8 BOM */
#define BOM "\xef\xbb\xbf"
#define BOM_SIZE 3

static bool is_bom = false;

/* We draw a diacritic char over a non-diacritic one. Therefore, such chars are
 * not considered to occupy space, therefore buffers might have more than
 * max_columns characters. The DIACRITIC_FACTOR is the max ratio between all
 * characters and non-diacritic characters in the buffer
 */
#define DIACRITIC_FACTOR 2

/* calculate the width of a UCS character (zero width for diacritics) */
static int glyph_width(unsigned short ch)
{
    if (ch == 0)
        ch = ' ';

#ifdef HAVE_LCD_BITMAP
    if (rb->is_diacritic(ch, NULL))
        return 0;

    return rb->font_get_width(pf, ch);
#else
    return 1;
#endif
}

/* get UCS character from string */
static unsigned char* get_ucs(const unsigned char* str, unsigned short* ch)
{
    unsigned char utf8_tmp[6];
    int count = 2;

    if (prefs.encoding == UTF_8)
        return (unsigned char*)rb->utf8decode(str, ch);

    rb->iso_decode(str, utf8_tmp, prefs.encoding, count);
    rb->utf8decode(utf8_tmp, ch);

    /* return a pointer after the parsed section of the string */
#ifdef HAVE_LCD_BITMAP
    if (prefs.encoding >= SJIS && *str >= 0x80
        && !(prefs.encoding == SJIS && *str > 0xA0 && *str < 0xE0))
        return (unsigned char*)str+2;
    else
#endif
        return (unsigned char*)str+1;
}

/* decode iso string into UTF-8 string */
static unsigned char *decode2utf8(const unsigned char *src, unsigned char *dst,
                                  int skip_width, int disp_width)
{
    unsigned short ucs[max_columns * DIACRITIC_FACTOR + 1];
    unsigned short ch;
    const unsigned char *oldstr;
    const unsigned char *str = src;
    unsigned char *utf8 = dst;
    int chars = 0;
    int idx = 0;
    int width = max_width;

    if (prefs.alignment == LEFT)
    {
        /* skip the skip_width */
        if (skip_width > 0)
        {
            while (skip_width > 0 && *str != '\0')
            {
                oldstr = str;
                str = get_ucs(oldstr, &ch);
                skip_width -= glyph_width(ch);
            }
            if (skip_width < 0)
                str = oldstr;
        }

        /* decode until string end or disp_width reached */
        while(*str != '\0')
        {
            str = get_ucs(str, &ch);
            disp_width -= glyph_width(ch);
            if (disp_width < 0)
                break;
            utf8 = rb->utf8encode(ch, utf8);
        }
    }
    else
    {
        while (width > 0 && *str != '\0')
        {
            str = get_ucs(str, &ch);
            ucs[chars++] = ch;
        }
        ucs[chars] = 0;

        skip_width = max_width - skip_width - disp_width;
        if (skip_width > 0)
        {
            while (skip_width > 0 && chars-- > 0)
                skip_width -= glyph_width(ucs[chars]);

            if (skip_width < 0)
                chars++;
        }
        else
        {
            idx = chars;
            while (disp_width > 0 && idx-- > 0)
                disp_width -= glyph_width(ucs[idx]);

            if (disp_width < 0 || idx < 0)
                idx++;
        }

        for (  ; idx < chars; idx++)
            utf8 = rb->utf8encode(ucs[idx], utf8);
    }

    *utf8 = '\0';

    /* return a pointer after the dst string ends */
    return utf8;
}

/* set max_columns and max_width */
static void calc_max_width(void)
{
    if (prefs.view_mode == NARROW)
    {
        max_columns = NARROW_MAX_COLUMNS;
        max_width   = draw_columns;
    }
    else
    {
        max_columns = WIDE_MAX_COLUMNS;
        max_width   = 2 * draw_columns;
    }
}

static bool done = false;
static int col = 0;

static inline void advance_conters(unsigned short ch, int* k, int* width)
{
#ifdef HAVE_LCD_BITMAP
    /* diacritics do not count */
    if (rb->is_diacritic(ch, NULL))
        return;
#endif

    *width += glyph_width(ch);
    (*k)++;
}

static inline bool line_is_full(int k, int width)
{
    return ((k >= max_columns - 1) || (width >= max_width));
}

static unsigned char* crop_at_width(const unsigned char* p)
{
    int k,width;
    unsigned short ch;
    const unsigned char *oldp = p;

    k=width=0;

    while (!line_is_full(k, width)) {
        oldp = p;
        if (BUFFER_OOB(p))
           break;
        p = get_ucs(p, &ch);
        advance_conters(ch, &k, &width);
    }

    return (unsigned char*)oldp;
}

static unsigned char* find_first_feed(const unsigned char* p, int size)
{
    int s = 0;
    unsigned short ch;
    const unsigned char *oldp = p;
    const unsigned char *lbrkp = NULL;
    int j;
    int width = 0;

    while(s <= size)
    {
        if (*p == 0)
            return (unsigned char*)p;
        oldp = p;
        p = get_ucs(p, &ch);

        if (prefs.word_mode == WRAP)
        {
            for (j = 0; j < ((int) sizeof(line_break)); j++)
            {
                if (ch == line_break[j])
                {
                    lbrkp = p;
                    break;
                }
            }
        }

        width += glyph_width(ch);
        if (width > max_width)
            return (lbrkp == NULL)? (unsigned char*)oldp : (unsigned char*)lbrkp;

        s += (p - oldp);
    }

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

    i = size;
    if (!BUFFER_OOB(&p[i]))
        for (j=k; j < ((int) sizeof(line_break)) - 1; j++) {
            if (p[i] == line_break[j])
                return (unsigned char*) p+i;
        }

    if (prefs.word_mode == WRAP) {
        for (i=size-1; i>=0; i--) {
            for (j=k; j < (int) sizeof(line_break) - 1; j++) {
                if (p[i] == line_break[j])
                    return (unsigned char*) p+i;
            }
        }
    }

    return NULL;
}

static unsigned char* find_next_line(const unsigned char* cur_line, bool *is_short)
{
    const unsigned char *next_line = NULL;
    int size, i, j, j_next, j_prev, k, width, search_len, spaces, newlines;
    bool first_chars;
    unsigned short ch;

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
        j_next=j=k=width=spaces=newlines=0;
        while (1) {
            const unsigned char *p, *oldp;

            j_prev = j;
            j = j_next;

            if (BUFFER_OOB(cur_line+j))
                return NULL;
            if (line_is_full(k, width)) {
                size = search_len = j_prev;
                break;
            }

            oldp = p = &cur_line[j];
            p = get_ucs(p, &ch);
            j_next = j + (p - oldp);

            switch (ch) {
                case ' ':
                    if (prefs.line_mode == REFLOW) {
                        if (newlines > 0) {
                            size = j;
                            next_line = cur_line + size;
                            return (unsigned char*) next_line;
                        }
                        if (j==0) /* i=1 is intentional */
                            for (i=0; i<par_indent_spaces; i++)
                                advance_conters(' ', &k, &width);
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
                            advance_conters(' ', &k, &width);
                            if (line_is_full(k, width)) {
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
                        advance_conters(' ', &k, &width);
                        if (line_is_full(k, width)) {
                            size = search_len = j;
                            break;
                        }
                    }
                    first_chars = false;
                    advance_conters(ch, &k, &width);
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

            if (next_line == NULL) {
                next_line = crop_at_width(cur_line);
            }
            else {
                if (prefs.word_mode == WRAP) {
                    for (i=0;i<WRAP_TRIM;i++) {
                        if (!(isspace(next_line[0]) && !BUFFER_OOB(next_line)))
                            break;
                        next_line++;
                    }
                }
            }
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
    {
        if (BUFFER_EOF() && next_line != cur_line)
            return (unsigned char*) next_line;
        return NULL;
    }

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
    while (p != NULL && p < cur_line) {
        prev_line = p;
        p = find_next_line(prev_line, NULL);
    }

    if (BUFFER_OOB(prev_line))
        return NULL;

    return (unsigned char*) prev_line;
}

static void check_bom(void)
{
    unsigned char bom[BOM_SIZE];
    off_t orig = rb->lseek(fd, 0, SEEK_CUR);

    is_bom = false;

    rb->lseek(fd, 0, SEEK_SET);

    if (rb->read(fd, bom, BOM_SIZE) == BOM_SIZE)
        is_bom = !memcmp(bom, BOM, BOM_SIZE);

    rb->lseek(fd, orig, SEEK_SET);
}

static void fill_buffer(long pos, unsigned char* buf, unsigned size)
{
    /* Read from file and preprocess the data */
    /* To minimize disk access, always read on sector boundaries */
    unsigned numread, i;
    bool found_CR = false;
    off_t offset = rb->lseek(fd, pos, SEEK_SET);

    if (offset == 0 && prefs.encoding == UTF_8 && is_bom)
        rb->lseek(fd, BOM_SIZE, SEEK_SET);

    numread = rb->read(fd, buf, size - 1);
    buf[numread] = 0;
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

static int viewer_find_bookmark(int page, int line)
{
    int i;

    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].page == page && bookmarks[i].line == line)
            return i;
    }
    return -1;
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

static void get_next_line_position(unsigned char **line_begin,
                                   unsigned char **line_end,
                                   bool *is_short)
{
    int resynch_move;

    *line_begin = *line_end;
    *line_end = find_next_line(*line_begin, is_short);

    if (*line_end == NULL && !BUFFER_EOF())
    {
        resynch_move = read_and_synch(1); /* Read block & move ptrs */
        *line_begin -= resynch_move;
        if (next_line_ptr > buffer)
            next_line_ptr -= resynch_move;

        *line_end = find_next_line(*line_begin, is_short);
    }
}

static void increment_current_line(void)
{
    if (cline < display_lines)
        cline++;
    else if (cpage < MAX_PAGE)
    {
        cpage++;
        cline = 1;
    }
}

static void decrement_current_line(void)
{
    if (cline > 1)
        cline--;
    else if (cpage > 1)
    {
        cpage--;
        cline = display_lines;
    }
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

    decrement_current_line();
}

static void viewer_scroll_down(bool autoscroll)
{
    if (cpage == lpage)
        return;

    if (next_line_ptr != NULL)
        screen_top_ptr = next_line_ptr;

    if (prefs.scroll_mode == LINE || autoscroll)
        increment_current_line();
}

static void viewer_scroll_to_top_line(void)
{
    int line;

    for (line = cline; line > 1; line--)
        viewer_scroll_up();
}

#ifdef HAVE_LCD_BITMAP
static void viewer_scrollbar(void) {
    int items, min_shown, max_shown, sb_begin_y, sb_height;

    items = (int) file_size;  /* (SH1 int is same as long) */
    min_shown = (int) file_pos + (screen_top_ptr - buffer);

    if (next_screen_ptr == NULL)
        max_shown = items;
    else
        max_shown = min_shown + (next_screen_ptr - screen_top_ptr);

    sb_begin_y = header_height;
    sb_height  = LCD_HEIGHT - header_height - footer_height;

    rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],0, sb_begin_y,
                           SCROLLBAR_WIDTH-1, sb_height,
                           items, min_shown, max_shown, VERTICAL);
}
#endif

#ifdef HAVE_LCD_BITMAP
static void viewer_show_header(void)
{
    if (prefs.header_mode == HD_SBAR || prefs.header_mode == HD_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (prefs.header_mode == HD_PATH || prefs.header_mode == HD_BOTH)
        rb->lcd_putsxy(0, header_height - pf->height, file_name);
}

static void viewer_show_footer(void)
{
    if (prefs.footer_mode == FT_SBAR || prefs.footer_mode == FT_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (prefs.footer_mode == FT_PAGE || prefs.footer_mode == FT_BOTH)
    {
        unsigned char buf[12];

        if (cline == 1)
            rb->snprintf(buf, sizeof(buf), "%d", cpage);
        else
            rb->snprintf(buf, sizeof(buf), "%d - %d", cpage, cpage+1);

        rb->lcd_putsxy(0, LCD_HEIGHT - footer_height, buf);
    }
}
#endif

static void viewer_draw(int col)
{
    int i, j, k, line_len, line_width, spaces, left_col=0;
    int width, extra_spaces, indent_spaces, spaces_per_word, spaces_width, disp_width = 0;
    bool multiple_spacing, line_is_short;
    unsigned short ch;
    unsigned char *str, *oldstr;
    unsigned char *line_begin;
    unsigned char *line_end;
    unsigned char c;
    int max_chars = max_columns * DIACRITIC_FACTOR;
    unsigned char scratch_buffer[max_chars * 4 + 1];
    unsigned char utf8_buffer[max_chars * 4 + 1];

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
        {
            if (lpage == cpage)
                break;  /* Happens after display last line at BUFFER_EOF() */

            if (lpage == 0 && cline == 1)
            {
                lpage = cpage;
                last_screen_top_ptr = screen_top_ptr;
                last_file_pos = file_pos;
            }
        }

        get_next_line_position(&line_begin, &line_end, &line_is_short);
        if (line_end == NULL)
        {
           if (BUFFER_OOB(line_begin))
                break;
           line_end = buffer_end + 1;
        }

        line_len = line_end - line_begin;

        /* calculate line_len */
        str = oldstr = line_begin;
        j = -1;
        while (str < line_end) {
            oldstr = str;
            str = crop_at_width(str);
            j++;
            if (oldstr == str)
            {
                oldstr = line_end;
                break;
            }
        }
        /* width of un-displayed part of the line */
        line_width = j*draw_columns;
        spaces_width = 0;
        while (oldstr < line_end) {
            oldstr = get_ucs(oldstr, &ch);
            /* add width of displayed part of the line */
            if (ch)
            {
                int dw = glyph_width(ch);

                /* avoid counting spaces at the end of the line */
                if (ch == ' ')
                {
                    spaces_width += dw;
                }
                else
                {
                    line_width += dw + spaces_width;
                    spaces_width = 0;
                }
            }
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
                if (k == max_chars)
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
                            if (k == max_chars - 1)
                                break;
                        }
                        scratch_buffer[k++] = c;
                        break;
                }
            }
            scratch_buffer[k] = 0;
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
                        case 0:
                        case ' ':
                            if (str == line_begin)
                            {
                                if (prefs.word_mode == WRAP && prefs.alignment == LEFT)
                                {
                                    /* special case: indent the paragraph,
                                     * don't count spaces */
                                    indent_spaces = par_indent_spaces;
                                }
                            }
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
                    extra_spaces = (max_width-width)/glyph_width(' ')
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
                if (k == max_chars)
                    break;

                c = line_begin[j];
                switch (c) {
                    case 0:
                        if (j == line_len - 1)
                            break;
                    case ' ':
                        if (j==0) {
                            /* indent paragraph */
                            if (prefs.word_mode == WRAP && prefs.alignment == LEFT)
                            {
                                for (j=0; j<par_indent_spaces; j++)
                                    scratch_buffer[k++] = ' ';
                                j=0;
                            }
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
            while (scratch_buffer[k-1] == ' ')
                k--;
            scratch_buffer[k] = 0;
        }
        else { /* prefs.line_mode != JOIN && prefs.line_mode != REFLOW */
            if (col != -1)
            {
                rb->memcpy(scratch_buffer, line_begin, line_len);
                scratch_buffer[line_len] = '\0';
            }
        }

        /* create displayed line */
        if (col != -1)
        {
            decode2utf8(scratch_buffer, utf8_buffer, col, draw_columns);
            rb->lcd_getstringsize(utf8_buffer, &disp_width, NULL);
        }

        /* display on screen the displayed part of the line */
        if (col != -1)
        {
            int dpage = (cline+i <= display_lines)?cpage:cpage+1;
            int dline = cline+i - ((cline+i <= display_lines)?0:display_lines);
            bool bflag = (viewer_find_bookmark(dpage, dline) >= 0);
#ifdef HAVE_LCD_BITMAP
            int dy = i * pf->height + header_height;
            int dx = (prefs.alignment == LEFT)? left_col : LCD_WIDTH - disp_width;
#endif
            if (bflag)
#ifdef HAVE_LCD_BITMAP
            {
                rb->lcd_set_drawmode(DRMODE_BG|DRMODE_FG);
                rb->lcd_fillrect(left_col, dy, LCD_WIDTH - left_col, pf->height);
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            }
            rb->lcd_putsxy(dx, dy, utf8_buffer);
            rb->lcd_set_drawmode(DRMODE_SOLID);
#else
            {
                rb->lcd_puts(left_col, i, BOOKMARK_ICON);
            }
            rb->lcd_puts(left_col+1, i, utf8_buffer);
#endif
        }
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

#ifdef HAVE_LCD_BITMAP
    /* show header */
    viewer_show_header();

    /* show footer */
    viewer_show_footer();
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
        rb->splash(0, "Loading...");

        file_pos = 0;
        buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
        fill_buffer(0, buffer, buffer_size);
    }

    screen_top_ptr = buffer;
    cpage = 1;
    cline = 1;
}

static void viewer_bottom(void)
{
    unsigned char *line_begin;
    unsigned char *line_end;

    rb->splash(0, "Loading...");

    if (last_screen_top_ptr)
    {
        cpage = lpage;
        cline = 1;
        screen_top_ptr = last_screen_top_ptr;
        file_pos = last_file_pos;
        fill_buffer(file_pos, buffer, buffer_size);
        buffer_end = BUFFER_END();
        return;
    }

    line_end = screen_top_ptr;

    while (!BUFFER_EOF() || !BUFFER_OOB(line_end))
    {
        get_next_line_position(&line_begin, &line_end, NULL);
        if (line_end == NULL)
            break;

        increment_current_line();
        if (cline == 1)
            screen_top_ptr = line_end;
    }
    lpage = cpage;
    cline = 1;
    last_screen_top_ptr = screen_top_ptr;
    last_file_pos = file_pos;
    buffer_end = BUFFER_END();
}

#ifdef HAVE_LCD_BITMAP
static void init_need_scrollbar(void) {
    /* Call viewer_draw in quiet mode to initialize next_screen_ptr,
     and thus ONE_SCREEN_FITS_ALL(), and thus NEED_SCROLLBAR() */
    viewer_draw(-1);
    prefs.need_scrollbar = NEED_SCROLLBAR();
    draw_columns = prefs.need_scrollbar? display_columns-SCROLLBAR_WIDTH : display_columns;
    par_indent_spaces = draw_columns/(5*glyph_width(' '));
    calc_max_width();
}

static void init_header_and_footer(void)
{
    header_height = 0;
    footer_height = 0;
    if (rb->global_settings->statusbar == STATUSBAR_TOP)
    {
        if (prefs.header_mode == HD_SBAR || prefs.header_mode == HD_BOTH)
            header_height = STATUSBAR_HEIGHT;

        if (prefs.footer_mode == FT_SBAR)
            prefs.footer_mode = FT_NONE;
        else if (prefs.footer_mode == FT_BOTH)
            prefs.footer_mode = FT_PAGE;
    }
    else if (rb->global_settings->statusbar == STATUSBAR_BOTTOM)
    {
        if (prefs.footer_mode == FT_SBAR || prefs.footer_mode == FT_BOTH)
            footer_height = STATUSBAR_HEIGHT;

        if (prefs.header_mode == HD_SBAR)
            prefs.header_mode = HD_NONE;
        else if (prefs.header_mode == HD_BOTH)
            prefs.header_mode = HD_PATH;
    }
    else /* STATUSBAR_OFF || STATUSBAR_CUSTOM */
    {
        if (prefs.header_mode == HD_SBAR)
            prefs.header_mode = HD_NONE;
        else if (prefs.header_mode == HD_BOTH)
            prefs.header_mode = HD_PATH;

        if (prefs.footer_mode == FT_SBAR)
            prefs.footer_mode = FT_NONE;
        else if (prefs.footer_mode == FT_BOTH)
            prefs.footer_mode = FT_PAGE;
    }

    if (prefs.header_mode == HD_NONE || prefs.header_mode == HD_PATH ||
        prefs.footer_mode == FT_NONE || prefs.footer_mode == FT_PAGE)
        rb->gui_syncstatusbar_draw(rb->statusbars, false);

    if (prefs.header_mode == HD_PATH || prefs.header_mode == HD_BOTH)
        header_height += pf->height;
    if (prefs.footer_mode == FT_PAGE || prefs.footer_mode == FT_BOTH)
        footer_height += pf->height;

    display_lines = (LCD_HEIGHT - header_height - footer_height) / pf->height;

    lpage = 0;
    last_file_pos = 0;
    last_screen_top_ptr = NULL;
}

static bool change_font(unsigned char *font)
{
    unsigned char buf[MAX_PATH];

    if (font == NULL || *font == '\0')
        return false;

    rb->snprintf(buf, MAX_PATH, "%s/%s.fnt", FONT_DIR, font);
    if (rb->font_load(NULL, buf) < 0) {
        rb->splash(HZ/2, "Font load failed.");
        return false;
    }

    return true;
}
#endif

static bool viewer_init(void)
{
#ifdef HAVE_LCD_BITMAP
    /* initialize fonts */
    pf = rb->font_get(FONT_UI);
    if (pf == NULL)
        return false;

    draw_columns = display_columns = LCD_WIDTH;
#else
    /* REAL fixed pitch :) all chars use up 1 cell */
    display_lines = 2;
    draw_columns = display_columns = 11;
    par_indent_spaces = 2;
#endif

    fd = rb->open(file_name, O_RDONLY);
    if (fd < 0)
        return false;

    /* Init mac_text value used in processing buffer */
    mac_text = false;

    return true;
}

/* When a file is UTF-8 file with BOM, if prefs.encoding is UTF-8,
 * then file size decreases only BOM_SIZE.
 */
static void get_filesize(void)
{
    file_size = rb->filesize(fd);
    if (file_size == -1)
        return;

    if (prefs.encoding == UTF_8 && is_bom)
        file_size -= BOM_SIZE;
}

static int bm_comp(const void *a, const void *b)
{
    struct bookmark_info *pa;
    struct bookmark_info *pb;

    pa = (struct bookmark_info*)a;
    pb = (struct bookmark_info*)b;

    if (pa->page != pb->page)
        return pa->page - pb->page;

    return pa->line - pb->line;
}

static void viewer_add_bookmark(void)
{
    if (bookmark_count >= MAX_BOOKMARKS-1)
        return;

    bookmarks[bookmark_count].file_position
        = file_pos + screen_top_ptr - buffer;
    bookmarks[bookmark_count].page = cpage;
    bookmarks[bookmark_count].line = cline;
    bookmarks[bookmark_count].flag = BOOKMARK_USER;
    bookmark_count++;
}

static int viewer_add_last_read_bookmark(void)
{
    int i;

    i = viewer_find_bookmark(cpage, cline);
    if (i >= 0)
        bookmarks[i].flag |= BOOKMARK_LAST;
    else
    {
        viewer_add_bookmark();
        i = bookmark_count-1;
        bookmarks[i].flag = BOOKMARK_LAST;
    }
    return i;
}

static void viewer_remove_bookmark(int i)
{
    int j;

    if (i < 0 || i >= bookmark_count)
        return;

    for (j = i+1; j < bookmark_count; j++)
        rb->memcpy(&bookmarks[j-1], &bookmarks[j],
                   sizeof(struct bookmark_info));

    bookmark_count--;
}

static void viewer_remove_last_read_bookmark(void)
{
    int i, j;

    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].flag & BOOKMARK_LAST)
        {
            if (bookmarks[i].flag == BOOKMARK_LAST)
            {
                for (j = i+1; j < bookmark_count; j++)
                    rb->memcpy(&bookmarks[j-1], &bookmarks[j],
                               sizeof(struct bookmark_info));

                bookmark_count--;
            }
            else
                bookmarks[i].flag = BOOKMARK_USER;
            break;
        }
    }
}

static int viewer_get_last_read_bookmark(void)
{
    int i;

    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].flag & BOOKMARK_LAST)
            return i;
    }
    return -1;
}

static void viewer_select_bookmark(int initval)
{
    int i;
    int ipage = 0;
    int iline = 0;
    int screen_pos;
    int screen_top;
    int selected = -1;

    struct opt_items items[bookmark_count];
    unsigned char names[bookmark_count][38];

    if (initval >= 0 && initval < bookmark_count)
    {
        ipage = bookmarks[initval].page;
        iline = bookmarks[initval].line;
    }

    rb->qsort(bookmarks, bookmark_count, sizeof(struct bookmark_info),
              bm_comp);

    for (i = 0; i < bookmark_count; i++)
    {
        rb->snprintf(names[i], sizeof(names[0]),
#if CONFIG_KEYPAD != PLAYER_PAD
                     "%sPage: %d  Line: %d",
#else
                     "%sP:%d  L:%d",
#endif
                     (bookmarks[i].flag&BOOKMARK_LAST)? "*":" ",
                     bookmarks[i].page,
                     bookmarks[i].line);
        items[i].string = names[i];
        items[i].voice_id = -1;
        if (selected < 0 && bookmarks[i].page == ipage && bookmarks[i].line == iline)
            selected = i;
    }

    rb->set_option("Select bookmark", &selected, INT, items,
                          sizeof(items) / sizeof(items[0]), NULL);

    if (selected < 0 || selected >= bookmark_count)
    {
        if (initval < 0 || (selected = viewer_get_last_read_bookmark()) < 0)
        {
            if (initval < 0)
                rb->splash(HZ, "Start the first page.");
            file_pos = 0;
            screen_top_ptr = buffer;
            cpage = 1;
            cline = 1;
            buffer_end = BUFFER_END();
            return;
        }
    }

    screen_pos = bookmarks[selected].file_position;
    screen_top = screen_pos % buffer_size;
    file_pos = screen_pos - screen_top;
    screen_top_ptr = buffer + screen_top;
    cpage = bookmarks[selected].page;
    cline = bookmarks[selected].line;
    buffer_end = BUFFER_END();
}

static void viewer_default_preferences(void)
{
    prefs.word_mode = WRAP;
    prefs.line_mode = NORMAL;
    prefs.view_mode = NARROW;
    prefs.alignment = LEFT;
    prefs.scroll_mode = PAGE;
    prefs.page_mode = NO_OVERLAP;
    prefs.scrollbar_mode = SB_OFF;
    rb->memset(prefs.font, 0, MAX_PATH);
#ifdef HAVE_LCD_BITMAP
    prefs.header_mode = HD_BOTH;
    prefs.footer_mode = FT_BOTH;
    rb->snprintf(prefs.font, MAX_PATH, "%s", rb->global_settings->font_file);
#else
    prefs.header_mode = HD_NONE;
    prefs.footer_mode = FT_NONE;
#endif
    prefs.autoscroll_speed = 1;
    /* Set codepage to system default */
    prefs.encoding = rb->global_settings->default_codepage;
}

static bool viewer_read_preferences(int pfd, int version, struct preferences *prf)
{
    unsigned char buf[PREFERENCES_SIZE];
    unsigned char *p = buf;
    int read_size = PREFERENCES_SIZE;

    if (version == 0)
        read_size--;

    if (rb->read(pfd, buf, read_size) != read_size)
        return false;

    prf->word_mode        = *p++;
    prf->line_mode        = *p++;
    prf->view_mode        = *p++;
    if (version > 0)
        prf->alignment = *p++;
    else
        prf->alignment = LEFT;
    prf->encoding         = *p++;
    prf->scrollbar_mode   = *p++;
    prf->need_scrollbar   = *p++;
    prf->page_mode        = *p++;
    prf->header_mode      = *p++;
    prf->footer_mode      = *p++;
    prf->scroll_mode      = *p++;
    prf->autoscroll_speed = *p++;
    rb->memcpy(prf->font, p, MAX_PATH);
    return true;
}

static bool viewer_write_preferences(int pfd, const struct preferences *prf)
{
    unsigned char buf[PREFERENCES_SIZE];
    unsigned char *p = buf;

    *p++ = prf->word_mode;
    *p++ = prf->line_mode;
    *p++ = prf->view_mode;
    *p++ = prf->alignment;
    *p++ = prf->encoding;
    *p++ = prf->scrollbar_mode;
    *p++ = prf->need_scrollbar;
    *p++ = prf->page_mode;
    *p++ = prf->header_mode;
    *p++ = prf->footer_mode;
    *p++ = prf->scroll_mode;
    *p++ = prf->autoscroll_speed;
    rb->memcpy(p, prf->font, MAX_PATH);

    return (rb->write(pfd, buf, sizeof(buf)) == sizeof(buf));
}

static bool viewer_read_bookmark_info(int bfd, struct bookmark_info *b)
{
    unsigned char buf[BOOKMARK_SIZE];

    if (rb->read(bfd, buf, sizeof(buf)) != sizeof(buf))
        return false;

    b->file_position = (buf[0] << 24)|(buf[1] << 16)|(buf[2] << 8)|buf[3];
    b->page = (buf[4] << 8)|buf[5];
    b->line = buf[6];
    b->flag = buf[7];

    return true;
}

static bool viewer_read_bookmark_infos(int bfd)
{
    unsigned char c;
    int i;

    if (rb->read(bfd, &c, 1) != 1)
    {
        bookmark_count = 0;
        return false;
    }

    bookmark_count = c;
    if (bookmark_count > MAX_BOOKMARKS)
        bookmark_count = MAX_BOOKMARKS;

    for (i = 0; i < bookmark_count; i++)
    {
        if (!viewer_read_bookmark_info(bfd, &bookmarks[i]))
        {
            bookmark_count = i;
            return false;
        }
    }
    return true;
}

static bool viewer_write_bookmark_info(int bfd, struct bookmark_info *b)
{
    unsigned char buf[BOOKMARK_SIZE];
    unsigned char *p = buf;
    unsigned long ul;

    ul = b->file_position;
    *p++ = ul >> 24;
    *p++ = ul >> 16;
    *p++ = ul >> 8;
    *p++ = ul;

    ul = b->page;
    *p++ = ul >> 8;
    *p++ = ul;

    *p++ = b->line;
    *p = b->flag;

    return (rb->write(bfd, buf, sizeof(buf)) == sizeof(buf));
}

static bool viewer_write_bookmark_infos(int bfd)
{
    unsigned char c = bookmark_count;
    int i;

    if (rb->write(bfd, &c, 1) != 1)
        return false;

    for (i = 0; i < bookmark_count; i++)
    {
        if (!viewer_write_bookmark_info(bfd, &bookmarks[i]))
            return false;
    }

    return true;
}

static bool viewer_load_global_settings(void)
{
    unsigned buf[GLOBAL_SETTINGS_H_SIZE];
    int sfd = rb->open(GLOBAL_SETTINGS_FILE, O_RDONLY);
    int version;
    bool res = false;

    if (sfd < 0)
        return false;

    if ((rb->read(sfd, buf, GLOBAL_SETTINGS_H_SIZE) == GLOBAL_SETTINGS_H_SIZE) ||
        (rb->memcmp(buf, GLOBAL_SETTINGS_HEADER, GLOBAL_SETTINGS_H_SIZE - 1) == 0))
    {
        version = buf[GLOBAL_SETTINGS_H_SIZE - 1] - GLOBAL_SETTINGS_FIRST_VERSION;
        res = viewer_read_preferences(sfd, version, &prefs);
    }
    rb->close(sfd);
    return res;
}

static bool viewer_save_global_settings(void)
{
    int sfd = rb->open(GLOBAL_SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC);
    unsigned char buf[GLOBAL_SETTINGS_H_SIZE];

    if (sfd < 0)
         return false;

    rb->memcpy(buf, GLOBAL_SETTINGS_HEADER, GLOBAL_SETTINGS_H_SIZE - 1);
    buf[GLOBAL_SETTINGS_H_SIZE - 1] = GLOBAL_SETTINGS_VERSION;

    if (rb->write(sfd, buf, GLOBAL_SETTINGS_H_SIZE) != GLOBAL_SETTINGS_H_SIZE ||
        !viewer_write_preferences(sfd, &prefs))
    {
        rb->close(sfd);
        rb->remove(GLOBAL_SETTINGS_TMP_FILE);
        return false;
    }
    rb->close(sfd);
    rb->remove(GLOBAL_SETTINGS_FILE);
    rb->rename(GLOBAL_SETTINGS_TMP_FILE, GLOBAL_SETTINGS_FILE);
    return true;
}

static bool viewer_convert_settings(int sfd, int dfd, int old_ver)
{
    struct preferences new_prefs;
    off_t old_pos;
    off_t new_pos;
    unsigned char buf[MAX_PATH + 2];
    int settings_size;

    rb->read(sfd, buf, MAX_PATH + 2);
    rb->write(dfd, buf, MAX_PATH + 2);

    settings_size = (buf[MAX_PATH] << 8) | buf[MAX_PATH + 1];

    old_pos = rb->lseek(sfd, 0, SEEK_CUR);
    new_pos = rb->lseek(dfd, 0, SEEK_CUR);

    /*
     * when the settings size != preferences size + bookmarks size,
     * settings data are considered to be old version.
     */
    if (old_ver > 0 && ((settings_size - PREFERENCES_SIZE) % 8) == 0)
        old_ver = 0;

    if (!viewer_read_preferences(sfd, old_ver, &new_prefs))
        return false;

    if (!viewer_write_preferences(dfd, &new_prefs))
        return false;

    settings_size -= (rb->lseek(sfd, 0, SEEK_CUR) - old_pos);

    if (settings_size > 0)
    {
        rb->read(sfd, buf, settings_size);
        rb->write(dfd, buf, settings_size);
    }

    settings_size = rb->lseek(dfd, 0, SEEK_CUR) - new_pos;
    buf[0] = settings_size >> 8;
    buf[1] = settings_size;
    rb->lseek(dfd, new_pos - 2, SEEK_SET);
    rb->write(dfd, buf, 2);
    rb->lseek(dfd, settings_size, SEEK_CUR);
    return true;
}

static bool viewer_convert_settings_file(void)
{
    unsigned char buf[SETTINGS_H_SIZE+2];
    int sfd;
    int tfd;
    int i;
    int fcount;
    int version;
    bool res = true;

    if ((sfd = rb->open(SETTINGS_FILE, O_RDONLY)) < 0)
        return false;

    if ((tfd = rb->open(SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC)) < 0)
    {
        rb->close(sfd);
        return false;
    }

    rb->read(sfd, buf, SETTINGS_H_SIZE + 2);

    version = buf[SETTINGS_H_SIZE - 1] - SETTINGS_FIRST_VERSION;
    fcount  = (buf[SETTINGS_H_SIZE] << 8) | buf[SETTINGS_H_SIZE + 1];
    buf[SETTINGS_H_SIZE - 1] = SETTINGS_VERSION;

    rb->write(tfd, buf, SETTINGS_H_SIZE + 2);

    for (i = 0; i < fcount; i++)
    {
        if (!viewer_convert_settings(sfd, tfd, version))
        {
            res = false;
            break;
        }
    }

    rb->close(sfd);
    rb->close(tfd);

    if (!res)
        rb->remove(SETTINGS_TMP_FILE);
    else
    {
        rb->remove(SETTINGS_FILE);
        rb->rename(SETTINGS_TMP_FILE, SETTINGS_FILE);
    }
    return res;
}

static bool viewer_load_settings(void)
{
    unsigned char buf[MAX_PATH+2];
    unsigned int fcount;
    unsigned int i;
    bool res = false;
    int sfd;
    unsigned int size;
    int version;

    sfd = rb->open(SETTINGS_FILE, O_RDONLY);
    if (sfd < 0)
        goto read_end;

    if ((rb->read(sfd, buf, SETTINGS_H_SIZE+2) != SETTINGS_H_SIZE+2) ||
        rb->memcmp(buf, SETTINGS_HEADER, SETTINGS_H_SIZE - 1))
    {
        /* illegal setting file */
        rb->close(sfd);

        if (rb->file_exists(SETTINGS_FILE))
            rb->remove(SETTINGS_FILE);

        goto read_end;
    }

    if (buf[SETTINGS_H_SIZE - 1] != SETTINGS_VERSION)
    {
        rb->close(sfd);
        if (!viewer_convert_settings_file())
            goto read_end;

        return viewer_load_settings();
    }

    version = buf[SETTINGS_H_SIZE - 1] - SETTINGS_FIRST_VERSION;
    fcount = (buf[SETTINGS_H_SIZE] << 8) | buf[SETTINGS_H_SIZE+1];
    for (i = 0; i < fcount; i++)
    {
        if (rb->read(sfd, buf, MAX_PATH+2) != MAX_PATH+2)
            break;

        size = (buf[MAX_PATH] << 8) | buf[MAX_PATH+1];

        /*
         * when the settings size != preferences size + bookmarks size,
         * the settings file converts to the newer.
         */
        if (version > 0 && ((size - PREFERENCES_SIZE) % 8) == 0)
        {
            rb->close(sfd);
            if (!viewer_convert_settings_file())
                break;

            return viewer_load_settings();
        }

        if (rb->strcmp(buf, file_name))
        {
            if (rb->lseek(sfd, size, SEEK_CUR) < 0)
                break;
            continue;
        }
        if (!viewer_read_preferences(sfd, version, &prefs))
            break;

        res = viewer_read_bookmark_infos(sfd);
        break;
    }

    rb->close(sfd);

read_end:
    if (!res)
    {
        /* load global settings */
        if (!viewer_load_global_settings())
            viewer_default_preferences();

        file_pos = 0;
        screen_top_ptr = buffer;
        cpage = 1;
        cline = 1;
        bookmark_count = 0;
    }

    rb->memcpy(&old_prefs, &prefs, sizeof(struct preferences));
    calc_max_width();

    if (bookmark_count > 1)
        viewer_select_bookmark(-1);
    else if (bookmark_count == 1)
    {
        int screen_pos;
        int screen_top;

        screen_pos = bookmarks[0].file_position;
        screen_top = screen_pos % buffer_size;
        file_pos = screen_pos - screen_top;
        screen_top_ptr = buffer + screen_top;
        cpage = bookmarks[0].page;
        cline = bookmarks[0].line;
    }

    viewer_remove_last_read_bookmark();

    check_bom();
    get_filesize();

    buffer_end = BUFFER_END();  /* Update whenever file_pos changes */

    if (BUFFER_OOB(screen_top_ptr))
        screen_top_ptr = buffer;

    fill_buffer(file_pos, buffer, buffer_size);
    if (prefs.scroll_mode == PAGE && cline > 1)
        viewer_scroll_to_top_line();

    /* remember the current position */
    start_position = file_pos + screen_top_ptr - buffer;

#ifdef HAVE_LCD_BITMAP
    /* load prefs font if it is different than the global settings font */
    if (rb->strcmp(prefs.font, rb->global_settings->font_file)) {
        if (!change_font(prefs.font)) {
            /* fallback by resetting prefs font to the global settings font */
            rb->memset(prefs.font, 0, MAX_PATH);
            rb->snprintf(prefs.font, MAX_PATH, "%s",
                rb->global_settings->font_file);

            if (!change_font(prefs.font))
                return false;
        }
    }

    init_need_scrollbar();
    init_header_and_footer();
#endif

    return true;
}

static bool copy_bookmark_file(int sfd, int dfd, off_t start, off_t size)
{
    off_t rsize;

    if (rb->lseek(sfd, start, SEEK_SET) < 0)
        return false;

    while (size > 0)
    {
        if (size > buffer_size)
            rsize = buffer_size;
        else
            rsize = size;
        size -= rsize;

        if (rb->read(sfd, buffer, rsize) != rsize ||
            rb->write(dfd, buffer, rsize) != rsize)
            return false;
    }
    return true;
}

static bool viewer_save_settings(void)
{
    unsigned char buf[MAX_PATH+2];
    unsigned int fcount = 0;
    unsigned int i;
    int idx;
    int ofd;
    int tfd;
    off_t first_copy_size = 0;
    off_t second_copy_start_pos = 0;
    off_t size;

    /* add reading page to bookmarks */
    idx = viewer_find_bookmark(cpage, cline);
    if (idx >= 0)
        bookmarks[idx].flag |= BOOKMARK_LAST;
    else
    {
        viewer_add_bookmark();
        bookmarks[bookmark_count-1].flag = BOOKMARK_LAST;
    }

    tfd = rb->open(SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC);
    if (tfd < 0)
        return false;

    ofd = rb->open(SETTINGS_FILE, O_RDWR);
    if (ofd >= 0)
    {
        if ((rb->read(ofd, buf, SETTINGS_H_SIZE+2) != SETTINGS_H_SIZE+2) ||
            rb->memcmp(buf, SETTINGS_HEADER, SETTINGS_H_SIZE - 1))
        {
            rb->close(ofd);
            goto save_err;
        }

        if (buf[SETTINGS_H_SIZE - 1] != SETTINGS_VERSION)
        {
            rb->close(ofd);
            if (!viewer_convert_settings_file())
                goto save_err;

            viewer_save_settings();
        }

        fcount = (buf[SETTINGS_H_SIZE] << 8) | buf[SETTINGS_H_SIZE+1];

        for (i = 0; i < fcount; i++)
        {
            if (rb->read(ofd, buf, MAX_PATH+2) != MAX_PATH+2)
            {
                rb->close(ofd);
                goto save_err;
            }
            size = (buf[MAX_PATH] << 8) | buf[MAX_PATH+1];
            if (rb->strcmp(buf, file_name))
            {
                if (rb->lseek(ofd, size, SEEK_CUR) < 0)
                {
                    rb->close(ofd);
                    goto save_err;
                }
            }
            else
            {
                first_copy_size = rb->lseek(ofd, 0, SEEK_CUR);
                if (first_copy_size < 0)
                {
                    rb->close(ofd);
                    goto save_err;
                }
                second_copy_start_pos = first_copy_size + size;
                first_copy_size -= MAX_PATH+2;
                fcount--;
                break;
            }
        }
        if (first_copy_size == 0)
            first_copy_size = rb->filesize(ofd);

        if (!copy_bookmark_file(ofd, tfd, 0, first_copy_size))
        {
            rb->close(ofd);
            goto save_err;
        }
        if (second_copy_start_pos > 0)
        {
            if (!copy_bookmark_file(ofd, tfd, second_copy_start_pos,
                                    rb->filesize(ofd) - second_copy_start_pos))
            {
                rb->close(ofd);
                goto save_err;
            }
        }
        rb->close(ofd);
    }
    else
    {
        rb->memcpy(buf, SETTINGS_HEADER, SETTINGS_H_SIZE - 1);
        buf[SETTINGS_H_SIZE-1] = SETTINGS_VERSION;
        buf[SETTINGS_H_SIZE  ] = 0;
        buf[SETTINGS_H_SIZE+1] = 0;
        if (rb->write(tfd, buf, SETTINGS_H_SIZE+2) != SETTINGS_H_SIZE+2)
            goto save_err;
    }

    /* copy to current read file's bookmarks */
    rb->memset(buf, 0, MAX_PATH);
    rb->snprintf(buf, MAX_PATH, "%s", file_name);

    size = PREFERENCES_SIZE + bookmark_count * BOOKMARK_SIZE + 1;
    buf[MAX_PATH] = size >> 8;
    buf[MAX_PATH+1] = size;

    if (rb->write(tfd, buf, MAX_PATH+2) != MAX_PATH+2)
        goto save_err;

    if (!viewer_write_preferences(tfd, &prefs))
        goto save_err;

    if (!viewer_write_bookmark_infos(tfd))
        goto save_err;

    if (rb->lseek(tfd, SETTINGS_H_SIZE, SEEK_SET) < 0)
        goto save_err;

    fcount++;
    buf[0] = fcount >> 8;
    buf[1] = fcount;

    if (rb->write(tfd, buf, 2) != 2)
        goto save_err;

    rb->close(tfd);

    rb->remove(SETTINGS_FILE);
    rb->rename(SETTINGS_TMP_FILE, SETTINGS_FILE);

    return true;

save_err:
    rb->close(tfd);
    rb->remove(SETTINGS_TMP_FILE);
    return false;
}

static void viewer_exit(void *parameter)
{
    (void)parameter;

    /* save preference and bookmarks */
    if (!viewer_save_settings())
        rb->splash(HZ, "Can't save preference and bookmarks.");

    rb->close(fd);
#ifdef HAVE_LCD_BITMAP
    if (rb->strcmp(prefs.font, rb->global_settings->font_file))
        change_font(rb->global_settings->font_file);
#endif
}

static void calc_page(void)
{
    int i;
    unsigned char *line_begin;
    unsigned char *line_end;
    off_t sfp;
    unsigned char *sstp;

    rb->splash(0, "Calculating page/line number...");

    /* add reading page to bookmarks */
    viewer_add_last_read_bookmark();

    rb->qsort(bookmarks, bookmark_count, sizeof(struct bookmark_info),
              bm_comp);

    cpage = 1;
    cline = 1;
    file_pos = 0;
    screen_top_ptr = buffer;
    buffer_end = BUFFER_END();

    fill_buffer(file_pos, buffer, buffer_size);
    line_end = line_begin = buffer;

    for (i = 0; i < bookmark_count; i++)
    {
        sfp = bookmarks[i].file_position;
        sstp = buffer;

        while ((line_begin > sstp || sstp >= line_end) ||
               (file_pos > sfp || sfp >= file_pos + BUFFER_END() - buffer))
        {
            get_next_line_position(&line_begin, &line_end, NULL);
            if (line_end == NULL)
                break;

            next_line_ptr = line_end;

            if (sstp == buffer &&
                file_pos <= sfp && sfp < file_pos + BUFFER_END() - buffer)
                sstp = sfp - file_pos + buffer;

            increment_current_line();
        }

        decrement_current_line();
        bookmarks[i].page = cpage;
        bookmarks[i].line = cline;
        bookmarks[i].file_position = file_pos + (line_begin - buffer);
        increment_current_line();
    }

    /* remove reading page's bookmark */
    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].flag & BOOKMARK_LAST)
        {
            int screen_pos;
            int screen_top;

            screen_pos = bookmarks[i].file_position;
            screen_top = screen_pos % buffer_size;
            file_pos = screen_pos - screen_top;
            screen_top_ptr = buffer + screen_top;

            cpage = bookmarks[i].page;
            cline = bookmarks[i].line;
            bookmarks[i].flag ^= BOOKMARK_LAST;
            buffer_end = BUFFER_END();

            fill_buffer(file_pos, buffer, buffer_size);

            if (bookmarks[i].flag == 0)
                viewer_remove_bookmark(i);

            if (prefs.scroll_mode == PAGE && cline > 1)
                viewer_scroll_to_top_line();
            break;
        }
    }
}

static int col_limit(int col)
{
    if (col < 0)
        col = 0;
    else
        if (col >= max_width - draw_columns)
            col = max_width - draw_columns;

    return col;
}

/* settings helper functions */

static bool encoding_setting(void)
{
    static struct opt_items names[NUM_CODEPAGES];
    int idx;
    bool res;
    enum codepages oldenc = prefs.encoding;

    for (idx = 0; idx < NUM_CODEPAGES; idx++)
    {
        names[idx].string = rb->get_codepage_name(idx);
        names[idx].voice_id = -1;
    }

    res = rb->set_option("Encoding", &prefs.encoding, INT, names,
                          sizeof(names) / sizeof(names[0]), NULL);

    /* When prefs.encoding changes into UTF-8 or changes from UTF-8,
     * filesize (file_size) might change.
     * In addition, if prefs.encoding is UTF-8, then BOM does not read.
     */
    if (oldenc != prefs.encoding && (oldenc == UTF_8 || prefs.encoding == UTF_8))
    {
        check_bom();
        get_filesize();
        fill_buffer(file_pos, buffer, buffer_size);
    }

    return res;
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
    calc_max_width();
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

static bool header_setting(void)
{
    int len = (rb->global_settings->statusbar == STATUSBAR_TOP)? 4 : 2;
    struct opt_items names[len];

    names[0].string   = "None";
    names[0].voice_id = -1;
    names[1].string   = "File path";
    names[1].voice_id = -1;

    if (rb->global_settings->statusbar == STATUSBAR_TOP)
    {
        names[2].string   = "Status bar";
        names[2].voice_id = -1;
        names[3].string   = "Both";
        names[3].voice_id = -1;
    }

    return rb->set_option("Show Header", &prefs.header_mode, INT,
                         names, len, NULL);
}

static bool footer_setting(void)
{
    int len = (rb->global_settings->statusbar == STATUSBAR_BOTTOM)? 4 : 2;
    struct opt_items names[len];

    names[0].string   = "None";
    names[0].voice_id = -1;
    names[1].string   = "Page Num";
    names[1].voice_id = -1;

    if (rb->global_settings->statusbar == STATUSBAR_BOTTOM)
    {
        names[2].string   = "Status bar";
        names[2].voice_id = -1;
        names[3].string   = "Both";
        names[3].voice_id = -1;
    }

    return rb->set_option("Show Footer", &prefs.footer_mode, INT,
                           names, len, NULL);
}

static int font_comp(const void *a, const void *b)
{
    struct opt_items *pa;
    struct opt_items *pb;

    pa = (struct opt_items *)a;
    pb = (struct opt_items *)b;

    return rb->strcmp(pa->string, pb->string);
}

static bool font_setting(void)
{
    int count = 0;
    DIR *dir;
    struct dirent *entry;
    int i = 0;
    int len;
    int new_font = 0;
    int old_font;
    bool res;
    int size = 0;

    dir = rb->opendir(FONT_DIR);
    if (!dir)
    {
        rb->splash(HZ/2, "Font dir is not accessible");
        return false;
    }

    while (1)
    {
        entry = rb->readdir(dir);

        if (entry == NULL)
            break;

        len = rb->strlen(entry->d_name);
        if (len < 4 || rb->strcmp(entry->d_name + len-4, ".fnt"))
            continue;
        size += len-3;
        count++;
    }
    rb->closedir(dir);

    struct opt_items names[count];
    unsigned char font_names[size];
    unsigned char *p = font_names;

    dir = rb->opendir(FONT_DIR);
    if (!dir)
    {
        rb->splash(HZ/2, "Font dir is not accessible");
        return false;
    }

    while (1)
    {
        entry = rb->readdir(dir);

        if (entry == NULL)
            break;

        len = rb->strlen(entry->d_name);
        if (len < 4 || rb->strcmp(entry->d_name + len-4, ".fnt"))
            continue;

        rb->snprintf(p, len-3, "%s", entry->d_name);
        names[i].string = p;
        names[i].voice_id = -1;
        p += len-3;
        i++;
        if (i >= count)
            break;
    }
    rb->closedir(dir);

    rb->qsort(names, count, sizeof(struct opt_items), font_comp);

    for (i = 0; i < count; i++)
    {
        if (!rb->strcmp(names[i].string, prefs.font))
        {
            new_font = i;
            break;
        }
    }
    old_font = new_font;

    res = rb->set_option("Select Font", &new_font, INT,
                         names, count, NULL);

    if (new_font != old_font)
    {
        /* load selected font */
        if (!change_font((unsigned char *)names[new_font].string)) {
            /* revert by re-loading the preferences font */
            change_font(prefs.font);
            return false;
        }
        rb->memset(prefs.font, 0, MAX_PATH);
        rb->snprintf(prefs.font, MAX_PATH, "%s", names[new_font].string);
    }

    return res;
}

static bool alignment_setting(void)
{
    static const struct opt_items names[] = {
        {"Left", -1},
        {"Right", -1},
    };

    return rb->set_option("Alignment", &prefs.alignment, INT,
                           names , 2, NULL);
}
#endif

static bool autoscroll_speed_setting(void)
{
    return rb->set_int("Auto-scroll Speed", "", UNIT_INT,
                       &prefs.autoscroll_speed, NULL, 1, 1, 10, NULL);
}

MENUITEM_FUNCTION(encoding_item, 0, "Encoding", encoding_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(word_wrap_item, 0, "Word Wrap", word_wrap_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(line_mode_item, 0, "Line Mode", line_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(view_mode_item, 0, "Wide View", view_mode_setting,
                  NULL, NULL, Icon_NOICON);
#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(alignment_item, 0, "Alignment", alignment_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(scrollbar_item, 0, "Show Scrollbar", scrollbar_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(page_mode_item, 0, "Overlap Pages", page_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(header_item, 0, "Show Header", header_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(footer_item, 0, "Show Footer", footer_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(font_item, 0, "Font", font_setting,
                  NULL, NULL, Icon_NOICON);
#endif
MENUITEM_FUNCTION(scroll_mode_item, 0, "Scroll Mode", scroll_mode_setting,
                  NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(autoscroll_speed_item, 0, "Auto-Scroll Speed",
                  autoscroll_speed_setting, NULL, NULL, Icon_NOICON);
MAKE_MENU(option_menu, "Viewer Options", NULL, Icon_NOICON,
            &encoding_item, &word_wrap_item, &line_mode_item, &view_mode_item,
#ifdef HAVE_LCD_BITMAP
            &alignment_item, &scrollbar_item, &page_mode_item, &header_item,
            &footer_item, &font_item,
#endif
            &scroll_mode_item, &autoscroll_speed_item);

static bool viewer_options_menu(bool is_global)
{
    bool result;
    struct preferences tmp_prefs;

    rb->memcpy(&tmp_prefs, &prefs, sizeof(struct preferences));

    result = (rb->do_menu(&option_menu, NULL, NULL, false) == MENU_ATTACHED_USB);

    if (!is_global && rb->memcmp(&tmp_prefs, &prefs, sizeof(struct preferences)))
    {
        /* Show-scrollbar mode for current view-width mode */
#ifdef HAVE_LCD_BITMAP
        init_need_scrollbar();
        init_header_and_footer();
#endif
        calc_page();
    }
    return result;
}

static void viewer_menu(void)
{
    int result;

    MENUITEM_STRINGLIST(menu, "Viewer Menu", NULL,
                        "Return", "Viewer Options",
                        "Show Playback Menu", "Select Bookmark",
                        "Global Settings", "Quit");

    result = rb->do_menu(&menu, NULL, NULL, false);
    switch (result)
    {
        case 0: /* return */
            break;
        case 1: /* change settings */
            done = viewer_options_menu(false);
            break;
        case 2: /* playback control */
            playback_control(NULL);
            break;
        case 3: /* select bookmark */
            viewer_select_bookmark(viewer_add_last_read_bookmark());
            viewer_remove_last_read_bookmark();
            fill_buffer(file_pos, buffer, buffer_size);
            if (prefs.scroll_mode == PAGE && cline > 1)
                viewer_scroll_to_top_line();
            break;
        case 4: /* change global settings */
            {
                struct preferences orig_prefs;

                rb->memcpy(&orig_prefs, &prefs, sizeof(struct preferences));
                if (!viewer_load_global_settings())
                    viewer_default_preferences();
                done = viewer_options_menu(true);
                viewer_save_global_settings();
                rb->memcpy(&prefs, &orig_prefs, sizeof(struct preferences));
            }
            break;
        case 5: /* quit */
            viewer_exit(NULL);
            done = true;
            break;
    }
    viewer_draw(col);
}

enum plugin_status plugin_start(const void* file)
{
    int button, i, ok;
    int lastbutton = BUTTON_NONE;
    bool autoscroll = false;
    long old_tick;

    old_tick = *rb->current_tick;

    /* get the plugin buffer */
    buffer = rb->plugin_get_buffer((size_t *)&buffer_size);
    if (buffer_size == 0)
    {
        rb->splash(HZ, "buffer does not allocate !!");
        return PLUGIN_ERROR;
    }
    block_size = buffer_size / 3;
    buffer_size = 3 * block_size;

    if (!file)
        return PLUGIN_ERROR;

    file_name = file;
    ok = viewer_init();
    if (!ok) {
        rb->splash(HZ, "Error opening file.");
        return PLUGIN_ERROR;
    }

    if (!viewer_load_settings()) /* load the preferences and bookmark */
        return PLUGIN_ERROR;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    viewer_draw(col);

    while (!done) {

        if(autoscroll)
        {
            if(old_tick <= *rb->current_tick - (110-prefs.autoscroll_speed*10))
            {
                viewer_scroll_down(true);
                viewer_draw(col);
                old_tick = *rb->current_tick;
            }
        }

        button = rb->button_get_w_tmo(HZ/10);

        if (prefs.view_mode != WIDE) {
            /* when not in wide view mode, the SCREEN_LEFT and SCREEN_RIGHT
               buttons jump to the beginning and end of the file. To stop
               users doing this by accident, replace non-held occurrences
               with page up/down instead. */
            if (button == VIEWER_SCREEN_LEFT)
                button = VIEWER_PAGE_UP;
            else if (button == VIEWER_SCREEN_RIGHT)
                button = VIEWER_PAGE_DOWN;
        }

        switch (button) {
            case VIEWER_MENU:
#ifdef VIEWER_MENU2
            case VIEWER_MENU2:
#endif
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
#ifdef VIEWER_PAGE_UP2
            case VIEWER_PAGE_UP2:
            case VIEWER_PAGE_UP2 | BUTTON_REPEAT:
#endif
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
#ifdef VIEWER_PAGE_DOWN2
            case VIEWER_PAGE_DOWN2:
            case VIEWER_PAGE_DOWN2 | BUTTON_REPEAT:
#endif
                if (prefs.scroll_mode == PAGE)
                {
                    /* Page down */
                    if (next_screen_ptr != NULL)
                    {
                        screen_top_ptr = next_screen_to_draw_ptr;
                        if (cpage < MAX_PAGE)
                            cpage++;
                    }
                }
                else
                    viewer_scroll_down(autoscroll);
                old_tick = *rb->current_tick;
                viewer_draw(col);
                break;

            case VIEWER_SCREEN_LEFT:
            case VIEWER_SCREEN_LEFT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE) {
                    /* Screen left */
                    col = col_limit(col - draw_columns);
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
                    col = col_limit(col + draw_columns);
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
                viewer_scroll_down(autoscroll);
                increment_current_line();
                old_tick = *rb->current_tick;
                viewer_draw(col);
                break;
#endif
#ifdef VIEWER_COLUMN_LEFT
            case VIEWER_COLUMN_LEFT:
            case VIEWER_COLUMN_LEFT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE) {
                    /* Scroll left one column */
                    col = col_limit(col - glyph_width('o'));
                    viewer_draw(col);
                }
                break;

            case VIEWER_COLUMN_RIGHT:
            case VIEWER_COLUMN_RIGHT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE) {
                    /* Scroll right one column */
                    col = col_limit(col + glyph_width('o'));
                    viewer_draw(col);
                }
                break;
#endif

#ifdef VIEWER_RC_QUIT
            case VIEWER_RC_QUIT:
#endif
            case VIEWER_QUIT:
#ifdef VIEWER_QUIT2
            case VIEWER_QUIT2:
#endif
                viewer_exit(NULL);
                done = true;
                break;

            case VIEWER_BOOKMARK:
                {
                    int idx = viewer_find_bookmark(cpage, cline);

                    if (idx < 0)
                    {
                        if (bookmark_count >= MAX_BOOKMARKS-1)
                            rb->splash(HZ/2, "No more add bookmark.");
                        else
                        {
                            viewer_add_bookmark();
                            rb->splash(HZ/2, "Bookmark add.");
                        }
                    }
                    else
                    {
                        viewer_remove_bookmark(idx);
                        rb->splash(HZ/2, "Bookmark remove.");
                    }
                    viewer_draw(col);
                }
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
