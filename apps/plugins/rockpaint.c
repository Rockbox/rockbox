/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Antoine Cellerier <dionoea -at- videolan -dot- org>
 * Based on parts of rockpaint 0.45, Copyright (C) 2005 Eli Sherer
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

/**
 * TODO:
 *  - implement 2 layers with alpha colors
 *  - take brush width into account when drawing shapes
 *  - handle bigger than screen bitmaps
 */

#include "plugin.h"
#include "lib/pluginlib_bmp.h"
#include "lib/rgb_hsv.h"
#include "lib/playback_control.h"

#include "pluginbitmaps/rockpaint.h"
#include "pluginbitmaps/rockpaint_hsvrgb.h"


/***********************************************************************
 * Buttons
 ***********************************************************************/

#if CONFIG_KEYPAD == IRIVER_H300_PAD
#define ROCKPAINT_QUIT      BUTTON_OFF
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_ON
#define ROCKPAINT_TOOLBAR   BUTTON_REC
#define ROCKPAINT_TOOLBAR2  BUTTON_MODE
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define ROCKPAINT_QUIT      ( ~BUTTON_MAIN )
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      ( BUTTON_SELECT | BUTTON_MENU )
#define ROCKPAINT_TOOLBAR   ( BUTTON_MENU | BUTTON_LEFT )
#define ROCKPAINT_TOOLBAR2  ( BUTTON_MENU | BUTTON_RIGHT )
#define ROCKPAINT_UP        BUTTON_MENU
#define ROCKPAINT_DOWN      BUTTON_PLAY
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif ( CONFIG_KEYPAD == IAUDIO_X5M5_PAD )
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_PLAY
#define ROCKPAINT_TOOLBAR   BUTTON_REC
#define ROCKPAINT_TOOLBAR2  ( BUTTON_REC | BUTTON_LEFT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_A
#define ROCKPAINT_TOOLBAR2  ( BUTTON_A | BUTTON_LEFT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      ( BUTTON_SELECT | BUTTON_POWER )
#define ROCKPAINT_TOOLBAR   BUTTON_REC
#define ROCKPAINT_TOOLBAR2  ( BUTTON_REC | BUTTON_LEFT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define ROCKPAINT_QUIT      (BUTTON_HOME|BUTTON_REPEAT)
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      ( BUTTON_SELECT | BUTTON_DOWN )
#define ROCKPAINT_TOOLBAR   ( BUTTON_SELECT | BUTTON_LEFT )
#define ROCKPAINT_TOOLBAR2  ( BUTTON_SELECT | BUTTON_RIGHT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif ( CONFIG_KEYPAD == IRIVER_H10_PAD )
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_FF
#define ROCKPAINT_MENU      BUTTON_PLAY
#define ROCKPAINT_TOOLBAR   BUTTON_REW
#define ROCKPAINT_TOOLBAR2  ( BUTTON_REW | BUTTON_LEFT )
#define ROCKPAINT_UP        BUTTON_SCROLL_UP
#define ROCKPAINT_DOWN      BUTTON_SCROLL_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define ROCKPAINT_QUIT      BUTTON_BACK
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_PLAY
#define ROCKPAINT_TOOLBAR2  ( BUTTON_PLAY | BUTTON_LEFT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif ( CONFIG_KEYPAD == COWON_D2_PAD )
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_MENU      BUTTON_MENU

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define ROCKPAINT_QUIT      BUTTON_BACK
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_PLAY
#define ROCKPAINT_TOOLBAR2  ( BUTTON_PLAY | BUTTON_LEFT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_VIEW
#define ROCKPAINT_TOOLBAR2  BUTTON_PLAYLIST
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_PLAY
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_PREV
#define ROCKPAINT_TOOLBAR2  BUTTON_NEXT
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_PLAY
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_RIGHT
#define ROCKPAINT_TOOLBAR2  BUTTON_LEFT
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_PREV
#define ROCKPAINT_RIGHT     BUTTON_NEXT

#elif ( CONFIG_KEYPAD == ONDAVX747_PAD )
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_MENU      BUTTON_MENU

#elif ( CONFIG_KEYPAD == ONDAVX777_PAD )
#define ROCKPAINT_QUIT      BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER

#elif ( CONFIG_KEYPAD == SAMSUNG_YH_PAD )
#define ROCKPAINT_QUIT      BUTTON_REC
#define ROCKPAINT_DRAW      BUTTON_PLAY
#define ROCKPAINT_MENU      BUTTON_FFWD
#define ROCKPAINT_TOOLBAR   BUTTON_REW
#define ROCKPAINT_TOOLBAR2  ( BUTTON_REW | BUTTON_LEFT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define ROCKPAINT_QUIT      BUTTON_REC
#define ROCKPAINT_DRAW      BUTTON_PLAY
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_OK
#define ROCKPAINT_TOOLBAR2  BUTTON_CANCEL
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_PREV
#define ROCKPAINT_RIGHT     BUTTON_NEXT

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_PLAYPAUSE
#define ROCKPAINT_TOOLBAR   BUTTON_BACK
#define ROCKPAINT_TOOLBAR2  (BUTTON_BACK|BUTTON_PLAYPAUSE)
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_HOME
#define ROCKPAINT_TOOLBAR   BUTTON_VOL_UP
#define ROCKPAINT_TOOLBAR2  BUTTON_VOL_DOWN
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_VOL_DOWN
#define ROCKPAINT_TOOLBAR   BUTTON_PREV
#define ROCKPAINT_TOOLBAR2  BUTTON_NEXT
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif CONFIG_KEYPAD == SAMSUNG_YPR0_PAD
#define ROCKPAINT_QUIT      BUTTON_BACK
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_MENU
#define ROCKPAINT_TOOLBAR   BUTTON_USER
#define ROCKPAINT_TOOLBAR2  ( BUTTON_USER | BUTTON_REPEAT )
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif (CONFIG_KEYPAD == HM60X_PAD)
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      (BUTTON_POWER | BUTTON_SELECT)
#define ROCKPAINT_TOOLBAR   (BUTTON_POWER | BUTTON_UP)
#define ROCKPAINT_TOOLBAR2  (BUTTON_POWER | BUTTON_LEFT)
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#elif (CONFIG_KEYPAD == HM801_PAD)
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      BUTTON_PLAY
#define ROCKPAINT_TOOLBAR   BUTTON_PREV
#define ROCKPAINT_TOOLBAR2  BUTTON_NEXT
#define ROCKPAINT_UP        BUTTON_UP
#define ROCKPAINT_DOWN      BUTTON_DOWN
#define ROCKPAINT_LEFT      BUTTON_LEFT
#define ROCKPAINT_RIGHT     BUTTON_RIGHT

#else
#error "Please define keys for this keypad"
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef ROCKPAINT_QUIT
#define ROCKPAINT_QUIT      BUTTON_TOPLEFT
#endif
#ifndef ROCKPAINT_DRAW
#define ROCKPAINT_DRAW      BUTTON_CENTER
#endif
#ifndef ROCKPAINT_MENU
#define ROCKPAINT_MENU      BUTTON_TOPRIGHT
#endif
#ifndef ROCKPAINT_TOOLBAR
#define ROCKPAINT_TOOLBAR   BUTTON_BOTTOMLEFT
#endif
#ifndef ROCKPAINT_TOOLBAR2
#define ROCKPAINT_TOOLBAR2  BUTTON_BOTTOMRIGHT
#endif
#ifndef ROCKPAINT_UP
#define ROCKPAINT_UP        BUTTON_TOPMIDDLE
#endif
#ifndef ROCKPAINT_DOWN
#define ROCKPAINT_DOWN      BUTTON_BOTTOMMIDDLE
#endif
#ifndef ROCKPAINT_LEFT
#define ROCKPAINT_LEFT      BUTTON_MIDLEFT
#endif
#ifndef ROCKPAINT_RIGHT
#define ROCKPAINT_RIGHT     BUTTON_MIDRIGHT
#endif
#endif

/***********************************************************************
 * Palette Default Colors
 ***********************************************************************/
#define COLOR_BLACK        LCD_RGBPACK(0,0,0)
#define COLOR_WHITE        LCD_RGBPACK(255,255,255)
#define COLOR_DARKGRAY     LCD_RGBPACK(128,128,128)
#define COLOR_LIGHTGRAY    LCD_RGBPACK(192,192,192)
#define COLOR_RED          LCD_RGBPACK(128,0,0)
#define COLOR_LIGHTRED     LCD_RGBPACK(255,0,0)
#define COLOR_DARKYELLOW   LCD_RGBPACK(128,128,0)
#define COLOR_YELLOW       LCD_RGBPACK(255,255,0)
#define COLOR_GREEN        LCD_RGBPACK(0,128,0)
#define COLOR_LIGHTGREN    LCD_RGBPACK(0,255,0)
#define COLOR_CYAN         LCD_RGBPACK(0,128,128)
#define COLOR_LIGHTCYAN    LCD_RGBPACK(0,255,255)
#define COLOR_BLUE         LCD_RGBPACK(0,0,128)
#define COLOR_LIGHTBLUE    LCD_RGBPACK(0,0,255)
#define COLOR_PURPLE       LCD_RGBPACK(128,0,128)
#define COLOR_PINK         LCD_RGBPACK(255,0,255)
#define COLOR_BROWN        LCD_RGBPACK(128,64,0)
#define COLOR_LIGHTBROWN   LCD_RGBPACK(255,128,64)

/***********************************************************************
 * Program Colors
 ***********************************************************************/
#define ROCKPAINT_PALETTE       LCD_RGBPACK(0,64,128)
#define ROCKPAINT_SELECTED      LCD_RGBPACK(128,192,255)

#define ROWS    LCD_HEIGHT
#define COLS    LCD_WIDTH

/**
 * Toolbar positioning stuff ... don't read this unless you really need to
 *
 * TB Toolbar
 * SP Separator
 * SC Selected Color
 * PL Palette
 * TL Tools
 */

/* Separator sizes */
#define TB_SP_MARGIN 3
#define TB_SP_WIDTH (2+2*TB_SP_MARGIN)

/* Selected color sizes */
#define TB_SC_SIZE 12

/* Palette sizes */
#define TB_PL_COLOR_SIZE 7
#define TB_PL_COLOR_SPACING 2
#define TB_PL_WIDTH ( 9 * TB_PL_COLOR_SIZE + 8 * TB_PL_COLOR_SPACING )
#define TB_PL_HEIGHT ( TB_PL_COLOR_SIZE * 2 + TB_PL_COLOR_SPACING )

/* Tools sizes */
#define TB_TL_SIZE 8
#define TB_TL_SPACING 2
#define TB_TL_WIDTH ( 7 * ( TB_TL_SIZE + TB_TL_SPACING ) - TB_TL_SPACING )
#define TB_TL_HEIGHT ( 2 * TB_TL_SIZE + TB_TL_SPACING )

/* Menu button size ... gruik */
#define TB_MENU_MIN_WIDTH 30

/* Selected colors position */
#define TB_SC_FG_TOP 2
#define TB_SC_FG_LEFT 2
#define TB_SC_BG_TOP (TB_SC_FG_TOP+TB_PL_COLOR_SIZE*2+TB_PL_COLOR_SPACING-TB_SC_SIZE)
#define TB_SC_BG_LEFT (TB_SC_FG_LEFT+TB_PL_COLOR_SIZE*2+TB_PL_COLOR_SPACING-TB_SC_SIZE)

/* Palette position */
#define TB_PL_TOP TB_SC_FG_TOP
#define TB_PL_LEFT (TB_SC_BG_LEFT + TB_SC_SIZE + TB_PL_COLOR_SPACING)

/* Tools position */
#define TB_TL_TOP TB_SC_FG_TOP
#define TB_TL_LEFT ( TB_PL_LEFT + TB_PL_WIDTH-1 + TB_SP_WIDTH )

#if TB_TL_LEFT + TB_TL_WIDTH + TB_MENU_MIN_WIDTH >= LCD_WIDTH
#undef TB_TL_TOP
#undef TB_TL_LEFT
#define TB_TL_TOP ( TB_PL_TOP + TB_PL_HEIGHT + 4 )
#define TB_TL_LEFT TB_SC_FG_LEFT
#endif

/* Menu button position */
#define TB_MENU_TOP ( TB_TL_TOP + (TB_TL_HEIGHT-8)/2 )
#define TB_MENU_LEFT ( TB_TL_LEFT + TB_TL_WIDTH-1 + TB_SP_WIDTH )

#define TB_HEIGHT ( TB_TL_TOP + TB_TL_HEIGHT + 1 )


static void draw_pixel(int x,int y);
static void draw_line( int x1, int y1, int x2, int y2 );
static void draw_rect( int x1, int y1, int x2, int y2 );
static void draw_rect_full( int x1, int y1, int x2, int y2 );
static void draw_toolbars(bool update);
static void inv_cursor(bool update);
static void restore_screen(void);
static void clear_drawing(void);
static void reset_tool(void);
static void goto_menu(void);
static int load_bitmap( const char *filename );
static int save_bitmap( char *filename );

/***********************************************************************
 * Global variables
 ***********************************************************************/

static int drawcolor=0; /* Current color (in palette) */
static int bgdrawcolor=9; /* Current background color (in palette) */
static int img_height = ROWS;
static int img_width = COLS;
bool isbg = false; /* gruik ugly hack alert */

static int preview=false; /* Is preview mode on ? */

/* TODO: clean this up */
static int x=0, y=0; /* cursor position */
static int prev_x=-1, prev_y=-1; /* previous saved cursor position */
static int prev_x2=-1, prev_y2=-1;
static int prev_x3=-1, prev_y3=-1;


static int bsize=1; /* brush size */
static int bspeed=1; /* brush speed */

enum Tools { Brush = 0, /* Regular brush */
             Fill = 1, /* Fill a shape with current color */
             SelectRectangle = 2,
             ColorPicker = 3, /* Pick a color */
             Line = 4, /* Draw a line between two points */
             Unused = 5, /* THIS IS UNUSED ... */
             Curve = 6,
             Text = 7,
             Rectangle = 8, /* Draw a rectangle */
             RectangleFull = 9,
             Oval = 10, /* Draw an oval */
             OvalFull = 11,
             LinearGradient = 12,
             RadialGradient = 13
           };

enum States { State0 = 0, /* initial state */
              State1,
              State2,
              State3,
            };

enum Tools tool = Brush;
enum States state = State0;

static bool quit=false;
static int gridsize=0;

static fb_data rp_colors[18] =
{
    COLOR_BLACK, COLOR_DARKGRAY, COLOR_RED, COLOR_DARKYELLOW,
    COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_PURPLE, COLOR_BROWN,
    COLOR_WHITE, COLOR_LIGHTGRAY, COLOR_LIGHTRED, COLOR_YELLOW,
    COLOR_LIGHTGREN, COLOR_LIGHTCYAN, COLOR_LIGHTBLUE, COLOR_PINK,
    COLOR_LIGHTBROWN
};

static fb_data save_buffer[ ROWS*COLS ];

struct tool_func {
    void (*state_func)(void);
    void (*preview_func)(void);
};

struct incdec_ctx {
    int max;
    int step[2];
    bool wrap;
};
struct incdec_ctx incdec_x = { COLS, { 1, 4}, true };
struct incdec_ctx incdec_y = { ROWS, { 1, 4}, true };

/* Maximum string size allowed for the text tool */
#define MAX_TEXT 256

union buf
{
    /* Used by fill and gradient algorithms */
    struct
    {
        short x;
        short y;
    } coord[ ROWS*COLS ];

    /* Used by bezier curve algorithms */
    struct
    {
        short x1, y1;
        short x2, y2;
        short x3, y3;
        short x4, y4;
        short depth;
    } bezier[ (ROWS*COLS)/5 ]; /* We have 4.5 times more data per struct
                                * than coord ... so we divide to take
                                * less memory. */

    /* Used to cut/copy/paste data */
    fb_data clipboard[ ROWS*COLS ];

    /* Used for text mode */
    struct
    {
        char text[MAX_TEXT];
        char font[MAX_PATH];
        bool initialized;
        size_t cache_used;
        /* fonts from cache_first to cache_last are stored. */
        int cache_first;
        int cache_last;
        /* save these so that cache can be re-used next time. */
        int fvi;
        int si;
    } text;
};

static union buf *buffer;
static bool audio_buf = false;

/* Current filename */
static char filename[MAX_PATH];

static bool incdec_value(int *pval, struct incdec_ctx *ctx, bool inc, bool bigstep)
{
    bool of = true;
    int step = ctx->step[bigstep?1:0];
    step = inc?step: -step;
    *pval += step;
    if (ctx->wrap)
    {
        if (*pval < 0) *pval += ctx->max;
        else if (*pval >= ctx->max) *pval -= ctx->max;
        else of = false;
    }
    else
    {
        if (*pval < 0) *pval = 0;
        else if (*pval > ctx->max) *pval = ctx->max;
        else of = false;
    }
    return of;
}

/***********************************************************************
 * Offscreen buffer/Text/Fonts handling
 *
 * Parts of code taken from firmware/drivers/lcd-16bit.c
 ***********************************************************************/
static void buffer_mono_bitmap_part(
                              fb_data *buf, int buf_width, int buf_height,
                              const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height )
/* this function only draws the foreground part of the bitmap */
{
    const unsigned char *src_end;
    fb_data *dst, *dst_end;
    unsigned fgcolor = rb->lcd_get_foreground();

    /* nothing to draw? */
    if( ( width <= 0 ) || ( height <= 0 ) || ( x >= buf_width )
        || ( y >= buf_height ) || ( x + width <= 0 ) || ( y + height <= 0 ) )
        return;

    /* clipping */
    if( x < 0 )
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if( y < 0 )
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if( x + width > buf_width )
        width = buf_width - x;
    if( y + height > buf_height )
        height = buf_height - y;

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y &= 7;
    src_end = src + width;

    dst = buf + y*buf_width + x;

    do
    {
        const unsigned char *src_col = src++;
        unsigned data = *src_col >> src_y;
        fb_data *dst_col = dst++;
        int numbits = 8 - src_y;

        dst_end = dst_col + height * buf_width;
        do
        {
            if( data & 0x01 )
                *dst_col = fgcolor; /* FIXME ? */

            dst_col += buf_width;

            data >>= 1;
            if( --numbits == 0 )
            {
                src_col += stride;
                data = *src_col;
                numbits = 8;
            }
        } while( dst_col < dst_end );
    } while( src < src_end );
}

/* draw alpha bitmap for anti-alias font */
#define ALPHA_COLOR_FONT_DEPTH 2
#define ALPHA_COLOR_LOOKUP_SHIFT (1 << ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_LOOKUP_SIZE ((1 << ALPHA_COLOR_LOOKUP_SHIFT) - 1)
#define ALPHA_COLOR_PIXEL_PER_BYTE (8 >> ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_PIXEL_PER_WORD (32 >> ALPHA_COLOR_FONT_DEPTH)
#ifdef CPU_ARM
#define BLEND_INIT do {} while (0)
#define BLEND_START(acc, color, alpha) \
    asm volatile("mul %0, %1, %2" : "=&r" (acc) : "r" (color), "r" (alpha))
#define BLEND_CONT(acc, color, alpha) \
    asm volatile("mla %0, %1, %2, %0" : "+&r" (acc) : "r" (color), "r" (alpha))
#define BLEND_OUT(acc) do {} while (0)
#elif defined(CPU_COLDFIRE)
#define ALPHA_BITMAP_READ_WORDS
#define BLEND_INIT coldfire_set_macsr(EMAC_UNSIGNED)
#define BLEND_START(acc, color, alpha) \
    asm volatile("mac.l %0, %1, %%acc0" :: "%d" (color), "d" (alpha))
#define BLEND_CONT BLEND_START
#define BLEND_OUT(acc) asm volatile("movclr.l %%acc0, %0" : "=d" (acc))
#else
#define BLEND_INIT do {} while (0)
#define BLEND_START(acc, color, alpha) ((acc) = (color) * (alpha))
#define BLEND_CONT(acc, color, alpha) ((acc) += (color) * (alpha))
#define BLEND_OUT(acc) do {} while (0)
#endif

/* Blend the given two colors */
static inline unsigned blend_two_colors(unsigned c1, unsigned c2, unsigned a)
{
    a += a >> (ALPHA_COLOR_LOOKUP_SHIFT - 1);
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
    c1 = swap16(c1);
    c2 = swap16(c2);
#endif
    unsigned c1l = (c1 | (c1 << 16)) & 0x07e0f81f;
    unsigned c2l = (c2 | (c2 << 16)) & 0x07e0f81f;
    unsigned p;
    BLEND_START(p, c1l, a);
    BLEND_CONT(p, c2l, ALPHA_COLOR_LOOKUP_SIZE + 1 - a);
    BLEND_OUT(p);
    p = (p >> ALPHA_COLOR_LOOKUP_SHIFT) & 0x07e0f81f;
    p |= (p >> 16);
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
    return swap16(p);
#else
    return p;
#endif
}

static void buffer_alpha_bitmap_part(
                              fb_data *buf, int buf_width, int buf_height,
                              const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height )
{
    fb_data *dst;
    unsigned fg_pattern = rb->lcd_get_foreground();

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= buf_width) ||
         (y >= buf_height) || (x + width <= 0) || (y + height <= 0))
        return;

    /* initialize blending */
    BLEND_INIT;

    /* clipping */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > buf_width)
        width = buf_width - x;
    if (y + height > buf_height)
        height = buf_height - y;

    dst = buf + y*buf_width + x;

    int col, row = height;
    unsigned data, pixels;
    unsigned skip_end = (stride - width);
    unsigned skip_start = src_y * stride + src_x;

#ifdef ALPHA_BITMAP_READ_WORDS
    uint32_t *src_w = (uint32_t *)((uintptr_t)src & ~3);
    skip_start += ALPHA_COLOR_PIXEL_PER_BYTE * ((uintptr_t)src & 3);
    src_w += skip_start / ALPHA_COLOR_PIXEL_PER_WORD;
    data = letoh32(*src_w++);
#else
    src += skip_start / ALPHA_COLOR_PIXEL_PER_BYTE;
    data = *src;
#endif
    pixels = skip_start % ALPHA_COLOR_PIXEL_PER_WORD;
    data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
#ifdef ALPHA_BITMAP_READ_WORDS
    pixels = 8 - pixels;
#endif

    do
    {
        col = width;
#ifdef ALPHA_BITMAP_READ_WORDS
#define UPDATE_SRC_ALPHA    do { \
            if (--pixels) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
            { \
                data = letoh32(*src_w++); \
                pixels = ALPHA_COLOR_PIXEL_PER_WORD; \
            } \
        } while (0)
#elif ALPHA_COLOR_PIXEL_PER_BYTE == 2
#define UPDATE_SRC_ALPHA    do { \
            if (pixels ^= 1) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
                data = *(++src); \
        } while (0)
#else
#define UPDATE_SRC_ALPHA    do { \
            if (pixels = (++pixels % ALPHA_COLOR_PIXEL_PER_BYTE)) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
                data = *(++src); \
        } while (0)
#endif

        do
        {
            *dst=blend_two_colors(*dst, fg_pattern,
                                  data & ALPHA_COLOR_LOOKUP_SIZE );
            dst++;
            UPDATE_SRC_ALPHA;
        }
        while (--col);
#ifdef ALPHA_BITMAP_READ_WORDS
        if (skip_end < pixels)
        {
            pixels -= skip_end;
            data >>= skip_end * ALPHA_COLOR_LOOKUP_SHIFT;
        } else {
            pixels = skip_end - pixels;
            src_w += pixels / ALPHA_COLOR_PIXEL_PER_WORD;
            pixels %= ALPHA_COLOR_PIXEL_PER_WORD;
            data = letoh32(*src_w++);
            data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
            pixels = 8 - pixels;
        }
#else
        if (skip_end)
        {
            pixels += skip_end;
            if (pixels >= ALPHA_COLOR_PIXEL_PER_BYTE)
            {
                src += pixels / ALPHA_COLOR_PIXEL_PER_BYTE;
                pixels %= ALPHA_COLOR_PIXEL_PER_BYTE;
                data = *src;
                data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
            } else
                data >>= skip_end * ALPHA_COLOR_LOOKUP_SHIFT;
        }
#endif
        dst += LCD_WIDTH - width;
    } while (--row);
}

static void buffer_putsxyofs( fb_data *buf, int buf_width, int buf_height,
                              int x, int y, int ofs, const unsigned char *str )
{
    unsigned short ch;
    unsigned short *ucs;

    struct font *pf = rb->font_get( FONT_UI );
    if( !pf ) pf = rb->font_get( FONT_SYSFIXED );

    ucs = rb->bidi_l2v( str, 1 );

    while( (ch = *ucs++) != 0 && x < buf_width )
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = rb->font_get_width( pf, ch );

        if( ofs > width )
        {
            ofs -= width;
            continue;
        }

        bits = rb->font_get_bits( pf, ch );

        if (pf->depth)
            buffer_alpha_bitmap_part( buf, buf_width, buf_height, bits, ofs, 0,
                                      width, x, y, width - ofs, pf->height);
        else
            buffer_mono_bitmap_part( buf, buf_width, buf_height, bits, ofs, 0,
                                     width, x, y, width - ofs, pf->height);

        x += width - ofs;
        ofs = 0;
    }
}

/***********************************************************************
 * Menu handling
 ***********************************************************************/
enum {
        /* Main menu */
        MAIN_MENU_RESUME,
        MAIN_MENU_NEW, MAIN_MENU_LOAD, MAIN_MENU_SAVE,
        MAIN_MENU_SET_WIDTH, MAIN_MENU_SET_HEIGHT,
        MAIN_MENU_BRUSH_SIZE, MAIN_MENU_BRUSH_SPEED, MAIN_MENU_COLOR,
        MAIN_MENU_GRID_SIZE,
        MAIN_MENU_PLAYBACK_CONTROL,
        MAIN_MENU_EXIT,
     };
enum {
        /* Select action menu */
        SELECT_MENU_CUT, SELECT_MENU_COPY,
        SELECT_MENU_INVERT, SELECT_MENU_HFLIP, SELECT_MENU_VFLIP,
        SELECT_MENU_ROTATE90, SELECT_MENU_ROTATE180, SELECT_MENU_ROTATE270,
        SELECT_MENU_CANCEL,
     };
enum {
        /* Text menu */
        TEXT_MENU_TEXT, TEXT_MENU_FONT,
        TEXT_MENU_PREVIEW, TEXT_MENU_APPLY, TEXT_MENU_CANCEL,
     };

MENUITEM_STRINGLIST(main_menu, "RockPaint", NULL,
                    "Resume", "New", "Load", "Save",
                    "Set Width", "Set Height",
                    "Brush Size", "Brush Speed",
                    "Choose Color", "Grid Size",
                    "Playback Control", "Exit");
MENUITEM_STRINGLIST(select_menu, "Select...", NULL,
                    "Cut", "Copy",
                    "Invert", "Horizontal Flip", "Vertical Flip",
                    "Rotate 90°", "Rotate 180°", "Rotate 270°",
                    "Cancel");
MENUITEM_STRINGLIST(text_menu, "Text", NULL,
                    "Set Text", "Change Font",
                    "Preview", "Apply", "Cancel");
static const int times_list[] = { 1, 2, 4, 8 };
static const int gridsize_list[] = { 0, 5, 10, 20 };
static const struct opt_items times_options[] = {
    { "1x", -1 }, { "2x", -1 }, { "4x", -1 }, { "8x", -1 }
};
static const struct opt_items gridsize_options[] = {
    { "No grid", -1 }, { "5px", -1 }, { "10px", -1 }, { "20px", -1 }
};

static int draw_window( int height, int width,
                         int *top, int *left,
                         const char *title )
{
    int fh;
    rb->lcd_getstringsize( title, NULL, &fh );
    fh++;

    const int _top = ( LCD_HEIGHT - height ) / 2;
    const int _left = ( LCD_WIDTH - width ) / 2;
    if( top ) *top = _top;
    if( left ) *left = _left;
    rb->lcd_set_background(COLOR_BLUE);
    rb->lcd_set_foreground(COLOR_LIGHTGRAY);
    rb->lcd_fillrect( _left, _top, width, height );
    rb->lcd_set_foreground(COLOR_BLUE);
    rb->lcd_fillrect( _left, _top, width, fh+4 );
    rb->lcd_set_foreground(COLOR_WHITE);
    rb->lcd_putsxy( _left+2, _top+2, title );
    rb->lcd_set_foreground(COLOR_BLACK);
    rb->lcd_drawrect( _left, _top, width, height );
    return _top+fh+4;
}

/***********************************************************************
 * File browser
 ***********************************************************************/

char bbuf[MAX_PATH]; /* used by file and font browsers */
char bbuf_s[MAX_PATH]; /* used by file and font browsers */
struct tree_context *tree = NULL;

static bool check_extention(const char *filename, const char *ext)
{
    const char *p = rb->strrchr( filename, '.' );
    return ( p != NULL && !rb->strcasecmp( p, ext ) );
}

/* only displayes directories and .bmp files */
static bool callback_show_item(char *name, int attr, struct tree_context *tc)
{
    (void) tc;
    if( ( attr & ATTR_DIRECTORY ) ||
        ( !(attr & ATTR_DIRECTORY) && check_extention( name, ".bmp" ) ) )
    {
        return true;
    }
    return false;
}

static bool browse( char *dst, int dst_size, const char *start )
{
    struct browse_context browse;

    rb->browse_context_init(&browse, SHOW_ALL,
                            BROWSE_SELECTONLY|BROWSE_NO_CONTEXT_MENU,
                            NULL, NOICON, start, NULL);

    browse.callback_show_item = callback_show_item;
    browse.buf = dst;
    browse.bufsize = dst_size;

    rb->rockbox_browse(&browse);

    return (browse.flags & BROWSE_SELECTED);
}

/***********************************************************************
 * Font browser
 *
 * FIXME: This still needs some work ... it currently only works fine
 * on the simulators, disk spins too much on real targets -> rendered
 * font buffer needed.
 ***********************************************************************/
/*
 * cache font preview handling assumes:
 * - fvi doesn't decrease by more than 1.
 *   In other words, cache_first-1 must be cached before cache_first-2 is cached.
 * - there is enough space to store all preview currently displayed.
 */
static bool browse_fonts( char *dst, int dst_size )
{
#define LINE_SPACE 2
#define PREVIEW_SIZE(x) ((x)->size)
#define PREVIEW_NEXT(x) (struct font_preview *)((char*)(x) + PREVIEW_SIZE(x))

    struct tree_context backup;
    struct entry *dc, *e;
    int dirfilter = SHOW_FONT;

    struct font_preview {
        unsigned short width;
        unsigned short height;
        size_t size;    /* to avoid calculating size each time. */
        fb_data preview[0];
    } *font_preview = NULL;

    int top = 0;

    int fvi = 0; /* first visible item */
    int lvi = 0; /* last visible item */
    int si = 0; /* selected item */
    int li = 0; /* last item */
    int nvih = 0; /* next visible item height */
    int i;
    bool need_redraw = true; /* Do we need to redraw ? */
    bool reset_font = false;
    bool ret = false;

    int cp = 0; /* current position */
    int sp = 0; /* selected position */
    int fh, fw; /* font height, width */

    unsigned char *cache = (unsigned char *) buffer + sizeof(buffer->text);
    size_t cache_size = sizeof(*buffer) - sizeof(buffer->text);
    size_t cache_used = 0;
    int cache_first = 0, cache_last = -1;
    char *a;

    rb->snprintf( bbuf_s, MAX_PATH, FONT_DIR "/%s.fnt",
                  rb->global_settings->font_file );

    tree = rb->tree_get_context();
    backup = *tree;
    dc = rb->tree_get_entries(tree);
    a = backup.currdir+rb->strlen(backup.currdir)-1;
    if( *a != '/' )
    {
        *++a = '/';
    }
    rb->strcpy( a+1, dc[tree->selected_item].name );
    tree->dirfilter = &dirfilter;
    tree->browse = NULL;
    rb->strcpy( bbuf, FONT_DIR "/" );
    rb->set_current_file( bbuf );

    if( buffer->text.initialized )
    {
        cache_used = buffer->text.cache_used;
        cache_first = buffer->text.cache_first;
        cache_last = buffer->text.cache_last;
        fvi = buffer->text.fvi;
        si = buffer->text.si;
    }
    buffer->text.initialized = true;

    while( 1 )
    {
        if( !need_redraw )
        {
            /* we don't need to redraw ... but we need to unselect
             * the previously selected item */
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect( 10, sp, LCD_WIDTH-10, font_preview->height );
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }

        if( need_redraw )
        {
            need_redraw = false;

            rb->lcd_set_foreground(COLOR_BLACK);
            rb->lcd_set_background(COLOR_LIGHTGRAY);
            rb->lcd_clear_display();

            rb->font_getstringsize( "Fonts", NULL, &fh, FONT_UI );
            rb->lcd_putsxy( 2, 2, "Fonts" );
            top = fh + 4 + LINE_SPACE;

            font_preview = (struct font_preview *) cache;
            /* get first font preview to be displayed. */
            for( i = cache_first; i < cache_last && i < fvi; i++ )
            {
                font_preview = PREVIEW_NEXT(font_preview);
            }
            for( ; fvi < lvi && nvih > 0; fvi++ )
            {
                nvih -= font_preview->height + LINE_SPACE;
                font_preview = PREVIEW_NEXT(font_preview);
            }
            nvih = 0;
            i = fvi;

            cp = top;
            while( cp <= LCD_HEIGHT+LINE_SPACE && i < tree->filesindir )
            {
                e = &dc[i];
                if( i < cache_first || i > cache_last )
                {
                    size_t siz;
                    reset_font = true;
                    rb->snprintf( bbuf, MAX_PATH, FONT_DIR "/%s", e->name );
                    rb->font_getstringsize( e->name, &fw, &fh, FONT_UI );
                    if( fw > LCD_WIDTH ) fw = LCD_WIDTH;
                    siz = (sizeof(struct font_preview) + fw*fh*FB_DATA_SZ+3) & ~3;
                    if( i < cache_first )
                    {
                        /* insert font preview to the top. */
                        cache_used = 0;
                        for( ; cache_first <= cache_last; cache_first++ )
                        {
                            font_preview = (struct font_preview *) (cache + cache_used);
                            size_t size = PREVIEW_SIZE(font_preview);
                            if( cache_used + size >= cache_size - siz )
                                break;
                            cache_used += size;
                        }
                        cache_last = cache_first-1;
                        cache_first = i;
                        rb->memmove( cache+siz, cache, cache_used );
                        font_preview = (struct font_preview *) cache;
                    }
                    else /* i > cache_last */
                    {
                        /* add font preview to the bottom. */
                        font_preview = (struct font_preview *) cache;
                        while( cache_used >= cache_size - siz )
                        {
                            cache_used -= PREVIEW_SIZE(font_preview);
                            font_preview = PREVIEW_NEXT(font_preview);
                            cache_first++;
                        }
                        cache_last = i;
                        rb->memmove( cache, font_preview, cache_used );
                        font_preview = (struct font_preview *) (cache + cache_used);
                    }
                    cache_used += siz;
                    /* create preview cache. */
                    font_preview->width = fw;
                    font_preview->height = fh;
                    font_preview->size = siz;
                    /* clear with background. */
                    for( siz = fw*fh; siz > 0; )
                    {
                        font_preview->preview[--siz] = COLOR_LIGHTGRAY;
                    }
                    buffer_putsxyofs( font_preview->preview,
                        fw, fh, 0, 0, 0, e->name );
                }
                else
                {
                    fw = font_preview->width;
                    fh = font_preview->height;
                }
                if( cp + fh >= LCD_HEIGHT )
                {
                    nvih = fh;
                    break;
                }
                rb->lcd_bitmap( font_preview->preview, 10, cp, fw, fh );
                cp += fh + LINE_SPACE;
                i++;
                font_preview = PREVIEW_NEXT(font_preview);
            }
            lvi = i-1;
            li = tree->filesindir-1;
            if( reset_font )
            {
             // fixme   rb->font_load(NULL, bbuf_s );
                reset_font = false;
            }
            if( lvi-fvi+1 < tree->filesindir )
            {
                rb->gui_scrollbar_draw( rb->screens[SCREEN_MAIN], 0, top,
                                       9, LCD_HEIGHT-top,
                                       tree->filesindir, fvi, lvi+1, VERTICAL );
            }
        }

        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        sp = top;
        font_preview = (struct font_preview *) cache;
        for( i = cache_first; i < si; i++ )
        {
            if( i >= fvi )
                sp += font_preview->height + LINE_SPACE;
            font_preview = PREVIEW_NEXT(font_preview);
        }
        rb->lcd_fillrect( 10, sp, LCD_WIDTH-10, font_preview->height );
        rb->lcd_set_drawmode(DRMODE_SOLID);

        rb->lcd_update();

        switch( rb->button_get(true) )
        {
            case ROCKPAINT_UP:
            case ROCKPAINT_UP|BUTTON_REPEAT:
                if( si > 0 )
                {
                    si--;
                    if( si < fvi )
                    {
                        fvi = si;
                        nvih = 0;
                        need_redraw = true;
                    }
                }
                break;

            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN|BUTTON_REPEAT:
                if( si < li )
                {
                    si++;
                    if( si > lvi )
                    {
                        need_redraw = true;
                    }
                }
                break;

            case ROCKPAINT_RIGHT:
            case ROCKPAINT_DRAW:
                ret = true;
                rb->snprintf( dst, dst_size, FONT_DIR "/%s", dc[si].name );
                /* fall through */
            case ROCKPAINT_LEFT:
            case ROCKPAINT_QUIT:
                buffer->text.cache_used = cache_used;
                buffer->text.cache_first = cache_first;
                buffer->text.cache_last = cache_last;
                buffer->text.fvi = fvi;
                buffer->text.si = si;
                *tree = backup;
                rb->set_current_file( backup.currdir );
                return ret;
        }
    }
#undef LINE_SPACE
#undef PREVIEW_SIZE
#undef PREVIEW_NEXT
}

/***********************************************************************
 * HSVRGB Color chooser
 ***********************************************************************/
static unsigned int color_chooser( unsigned int color )
{
    int red = RGB_UNPACK_RED( color );
    int green = RGB_UNPACK_GREEN( color );
    int blue = RGB_UNPACK_BLUE( color );
    int hue, saturation, value;
    int r, g, b; /* temp variables */
    int i, top, left;
    int button;
    int *pval;
    static struct incdec_ctx ctxs[] = {
        { 3600, { 10, 100}, true },  /* hue */
        { 0xff, {  1,   8}, false }, /* the others */
    };

    enum BaseColor { Hue = 0, Saturation = 1, Value = 2,
                     Red = 3, Green = 4, Blue = 5 };
    enum BaseColor current = Red;
    bool has_changed;

    restore_screen();

    rgb2hsv( red, green, blue, &hue, &saturation, &value );

    while( 1 )
    {
        has_changed = false;
        color = LCD_RGBPACK( red, green, blue );

#define HEIGHT  ( 100 )
#define WIDTH   ( 150 )

        top = draw_window( HEIGHT, WIDTH, NULL, &left, "Color chooser" );
        top -= 15;

        for( i=0; i<100; i++ )
        {
            hsv2rgb( i*36, saturation, value, &r, &g, &b );
            rb->lcd_set_foreground( LCD_RGBPACK( r, g, b ) );
            rb->lcd_vline( left+15+i, top+20, top+27 );
            hsv2rgb( hue, i*255/100, value, &r, &g, &b );
            rb->lcd_set_foreground( LCD_RGBPACK( r, g, b ) );
            rb->lcd_vline( left+15+i, top+30, top+37 );
            hsv2rgb( hue, saturation, i*255/100, &r, &g, &b );
            rb->lcd_set_foreground( LCD_RGBPACK( r, g, b ) );
            rb->lcd_vline( left+15+i, top+40, top+47 );
            rb->lcd_set_foreground( LCD_RGBPACK( i*255/100, green, blue ) );
            rb->lcd_vline( left+15+i, top+50, top+57 );
            rb->lcd_set_foreground( LCD_RGBPACK( red, i*255/100, blue ) );
            rb->lcd_vline( left+15+i, top+60, top+67 );
            rb->lcd_set_foreground( LCD_RGBPACK( red, green, i*255/100 ) );
            rb->lcd_vline( left+15+i, top+70, top+77 );
        }

        rb->lcd_set_foreground(COLOR_BLACK);
#define POSITION( a, i ) \
        rb->lcd_drawpixel( left+14+i, top + 19 + a ); \
        rb->lcd_drawpixel( left+16+i, top + 19 + a ); \
        rb->lcd_drawpixel( left+14+i, top + 28 + a ); \
        rb->lcd_drawpixel( left+16+i, top + 28 + a );
        POSITION( 0, hue/36 );
        POSITION( 10, saturation*99/255 );
        POSITION( 20, value*99/255 );
        POSITION( 30, red*99/255 );
        POSITION( 40, green*99/255 );
        POSITION( 50, blue*99/255 );
#undef POSITION
        rb->lcd_set_background(COLOR_LIGHTGRAY);
        rb->lcd_setfont( FONT_SYSFIXED );
        rb->lcd_putsxyf( left + 117, top + 20, "%d", hue/10 );
        rb->lcd_putsxyf( left + 117, top + 30, "%d.%d",
                saturation/255, ((saturation*100)/255)%100 );
        rb->lcd_putsxyf( left + 117, top + 40, "%d.%d",
                value/255, ((value*100)/255)%100 );
        rb->lcd_putsxyf( left + 117, top + 50, "%d", red );
        rb->lcd_putsxyf( left + 117, top + 60, "%d", green );
        rb->lcd_putsxyf( left + 117, top + 70, "%d", blue );
        rb->lcd_setfont( FONT_UI );

#define CURSOR( l ) \
        rb->lcd_bitmap_transparent_part( rockpaint_hsvrgb, 1, 1, 16, left+l+1, top+20, 6, 58 ); \
        rb->lcd_bitmap_transparent_part( rockpaint_hsvrgb, 8, 10*current, 16, left+l, top+19+10*current, 8, 10 );
        CURSOR( 5 );
#undef CURSOR

        rb->lcd_set_foreground( color );
        rb->lcd_fillrect( left+15, top+85, 100, 8 );

        rb->lcd_update();

        switch( button = rb->button_get(true) )
        {
            case ROCKPAINT_UP:
                current = ( current + 5 )%6;
                break;

            case ROCKPAINT_DOWN:
                current = ( current + 1 )%6;
                break;

            case ROCKPAINT_LEFT:
            case ROCKPAINT_LEFT|BUTTON_REPEAT:
            case ROCKPAINT_RIGHT:
            case ROCKPAINT_RIGHT|BUTTON_REPEAT:
                has_changed = true;
                switch( current )
                {
                    case Hue:
                        pval = &hue;
                        break;
                    case Saturation:
                        pval = &saturation;
                        break;
                    case Value:
                        pval = &value;
                        break;
                    case Red:
                        pval = &red;
                        break;
                    case Green:
                        pval = &green;
                        break;
                    case Blue:
                        pval = &blue;
                        break;
                    default:
                        pval = NULL;
                        break;
                }
                if (pval)
                {
                    incdec_value(pval, &ctxs[(current != Hue? 1: 0)],
                        (button&ROCKPAINT_RIGHT), (button&BUTTON_REPEAT));
                }
                break;

            case ROCKPAINT_DRAW:
                return color;
        }
        if( has_changed )
        {
            switch( current )
            {
                case Hue:
                case Saturation:
                case Value:
                    hsv2rgb( hue, saturation, value, &red, &green, &blue );
                    break;

                case Red:
                case Green:
                case Blue:
                    rgb2hsv( red, green, blue, &hue, &saturation, &value );
                    break;
            }
        }
#undef HEIGHT
#undef WIDTH
    }
}

/***********************************************************************
 * Misc routines
 ***********************************************************************/
static void init_buffer(void)
{
    int i;
    fb_data color = rp_colors[ bgdrawcolor ];
    for( i = 0; i < ROWS*COLS; i++ )
    {
        save_buffer[i] = color;
    }
}

static void draw_pixel(int x,int y)
{
    if( !preview )
    {
        if( x < 0 || x >= COLS || y < 0 || y >= ROWS ) return;
        if( isbg )
        {
            save_buffer[ x+y*COLS ] = rp_colors[bgdrawcolor];
        }
        else
        {
            save_buffer[ x+y*COLS ] = rp_colors[drawcolor];
        }
    }
    rb->lcd_drawpixel(x,y);
}

static void color_picker( int x, int y )
{
    if( preview )
    {
        rb->lcd_set_foreground( save_buffer[ x+y*COLS ] );
#define PSIZE 12
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        if( x >= COLS - PSIZE ) x -= PSIZE + 2;
        if( y >= ROWS - PSIZE ) y -= PSIZE + 2;
        rb->lcd_drawrect( x + 2, y + 2, PSIZE - 2, PSIZE - 2 );
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_fillrect( x + 3, y + 3, PSIZE - 4, PSIZE - 4 );
#undef PSIZE
        rb->lcd_set_foreground( rp_colors[ drawcolor ] );
    }
    else
    {
        rp_colors[ drawcolor ] = save_buffer[ x+y*COLS ];
    }
}

static void draw_select_rectangle( int x1, int y1, int x2, int y2 )
/* This is a preview mode only function */
{
    int i,a;
    if( x1 > x2 )
    {
        i = x1;
        x1 = x2;
        x2 = i;
    }
    if( y1 > y2 )
    {
        i = y1;
        y1 = y2;
        y2 = i;
    }
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    i = 0;
    for( a = x1; a < x2; a++, i++ )
        if( i%2 )
            rb->lcd_drawpixel( a, y1 );
    for( a = y1; a < y2; a++, i++ )
        if( i%2 )
            rb->lcd_drawpixel( x2, a );
    if( y2 != y1 )
        for( a = x2; a > x1; a--, i++ )
            if( i%2 )
                rb->lcd_drawpixel( a, y2 );
    if( x2 != x1 )
        for( a = y2; a > y1; a--, i++ )
            if( i%2 )
                rb->lcd_drawpixel( x1, a );
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

static void copy_to_clipboard( void )
{
    /* This needs to be optimised ... but i'm lazy ATM */
    rb->memcpy( buffer->clipboard, save_buffer, COLS*ROWS*sizeof( fb_data ) );
}

/* no preview mode handling atm ... do we need it ? (one if) */
static void draw_invert( int x1, int y1, int x2, int y2 )
{
    int i;
    if( x1 > x2 )
    {
        i = x1;
        x1 = x2;
        x2 = i;
    }
    if( y1 > y2 )
    {
        i = y1;
        y1 = y2;
        y2 = i;
    }

    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_fillrect( x1, y1, x2-x1+1, y2-y1+1 );
    rb->lcd_set_drawmode(DRMODE_SOLID);

    for( ; y1<=y2; y1++ )
    {
        for( i = x1; i<=x2; i++ )
        {
            save_buffer[ y1*COLS + i ] = ~save_buffer[ y1*COLS + i ];
        }
    }
    /*if( update )*/ rb->lcd_update();
}

static void draw_hflip( int x1, int y1, int x2, int y2 )
{
    int i;
    if( x1 > x2 )
    {
        i = x1;
        x1 = x2;
        x2 = i;
    }
    if( y1 > y2 )
    {
        i = y1;
        y1 = y2;
        y2 = i;
    }

    copy_to_clipboard();

    for( i = 0; i <= y2 - y1; i++ )
    {
        rb->memcpy( save_buffer+(y1+i)*COLS+x1,
                    buffer->clipboard+(y2-i)*COLS+x1,
                    (x2-x1+1)*sizeof( fb_data ) );
    }
    restore_screen();
    rb->lcd_update();
}

static void draw_vflip( int x1, int y1, int x2, int y2 )
{
    int i;
    if( x1 > x2 )
    {
        i = x1;
        x1 = x2;
        x2 = i;
    }
    if( y1 > y2 )
    {
        i = y1;
        y1 = y2;
        y2 = i;
    }

    copy_to_clipboard();

    for( ; y1 <= y2; y1++ )
    {
        for( i = 0; i <= x2 - x1; i++ )
        {
            save_buffer[y1*COLS+x1+i] = buffer->clipboard[y1*COLS+x2-i];
        }
    }
    restore_screen();
    rb->lcd_update();
}

/* direction: -1 = left, 1 = right */
static void draw_rot_90_deg( int x1, int y1, int x2, int y2, int direction )
{
    int i, j;
    if( x1 > x2 )
    {
        i = x1;
        x1 = x2;
        x2 = i;
    }
    if( y1 > y2 )
    {
        i = y1;
        y1 = y2;
        y2 = i;
    }

    copy_to_clipboard();

    fb_data color = rp_colors[ bgdrawcolor ];
    const int width = x2 - x1, height = y2 - y1;
    const int sub_half = width/2-height/2, add_half = (width+height)/2;
    if( width > height )
    {
        for( i = 0; i <= height; i++ )
        {
            for( j = 0; j < sub_half; j++ )
                save_buffer[(y1+i)*COLS+x1+j] = color;
            for( j = add_half+1; j <= width; j++ )
                save_buffer[(y1+i)*COLS+x1+j] = color;
        }
    }
    else if( width < height )
    {
        for( j = 0; j <= width; j++ )
        {
            for( i = 0; i < -sub_half; i++ )
                save_buffer[(y1+i)*COLS+x1+j] = color;
            for( i = add_half+1; i <= height; i++ )
                save_buffer[(y1+i)*COLS+x1+j] = color;
        }
    }
    int x3 = x1 + sub_half, y3 = y1 - sub_half;
    int is = x3<0?-x3:0, ie = COLS-x3-1, js = y3<0?-y3:0, je = ROWS-y3-1;
    if( ie > height ) ie = height;
    if( je > width ) je = width;
    for( i = is; i <= ie; i++ )
    {
        for( j = js; j <= je; j++ )
        {
            int x, y;
            if(direction > 0)
            {
                x = x1+j;
                y = y1+height-i;
            }
            else
            {
                x = x1+width-j;
                y = y1+i;
            }
            save_buffer[(y3+j)*COLS+x3+i] = buffer->clipboard[y*COLS+x];
        }
    }
    restore_screen();
    rb->lcd_update();
}

static void draw_paste_rectangle( int src_x1, int src_y1, int src_x2,
                                  int src_y2, int x1, int y1, int cut )
{
    int i, width, height;
    if( cut )
    {
        i = drawcolor;
        drawcolor = bgdrawcolor;
        draw_rect_full( src_x1, src_y1, src_x2, src_y2 );
        drawcolor = i;
    }
    if( src_x1 > src_x2 )
    {
        i = src_x1;
        src_x1 = src_x2;
        src_x2 = i;
    }
    if( src_y1 > src_y2 )
    {
        i = src_y1;
        src_y1 = src_y2;
        src_y2 = i;
    }
    width = src_x2 - src_x1 + 1;
    height = src_y2 - src_y1 + 1;
    /* clipping */
    if( x1 + width > COLS )
        width = COLS - x1;
    if( y1 + height > ROWS )
        height = ROWS - y1;

    rb->lcd_bitmap_part( buffer->clipboard, src_x1, src_y1, COLS,
                         x1, y1, width, height );
    if( !preview )
    {
        for( i = 0; i < height; i++ )
        {
            rb->memcpy( save_buffer+(y1+i)*COLS+x1,
                        buffer->clipboard+(src_y1+i)*COLS+src_x1,
                        width*sizeof( fb_data ) );
        }
    }
}

static void show_grid( bool update )
{
    int i;
    if( gridsize > 0 )
    {
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        for( i = gridsize; i < img_width; i+= gridsize )
        {
            rb->lcd_vline( i, 0, img_height-1 );
        }
        for( i = gridsize; i < img_height; i+= gridsize )
        {
            rb->lcd_hline( 0, img_width-1, i );
        }
        rb->lcd_set_drawmode(DRMODE_SOLID);
        if( update ) rb->lcd_update();
    }
}

static void draw_text( int x, int y )
{
    int selected = 0;
    int current_font_id = rb->global_status->font_id[SCREEN_MAIN];
    buffer->text.text[0] = '\0';
    buffer->text.font[0] = '\0';
    while( 1 )
    {
        switch( rb->do_menu( &text_menu, &selected, NULL, NULL ) )
        {
            case TEXT_MENU_TEXT:
                rb->lcd_set_foreground(COLOR_BLACK);
                rb->kbd_input( buffer->text.text, MAX_TEXT );
                break;

            case TEXT_MENU_FONT:
                if (current_font_id != rb->global_status->font_id[SCREEN_MAIN])
                    rb->font_unload(current_font_id);
                if(browse_fonts( buffer->text.font, MAX_PATH ) )
                {
                    current_font_id = rb->font_load(buffer->text.font );
                    rb->lcd_setfont(current_font_id);
                }
                break;

            case TEXT_MENU_PREVIEW:
                rb->lcd_set_foreground( rp_colors[ drawcolor ] );
                while( 1 )
                {
                    int button;
                    restore_screen();
                    rb->lcd_putsxy( x, y, buffer->text.text );
                    rb->lcd_update();
                    switch( button = rb->button_get( true ) )
                    {
                        case ROCKPAINT_LEFT:
                        case ROCKPAINT_LEFT | BUTTON_REPEAT:
                        case ROCKPAINT_RIGHT:
                        case ROCKPAINT_RIGHT | BUTTON_REPEAT:
                            incdec_value(&x, &incdec_x,
                                (button&ROCKPAINT_RIGHT), (button&BUTTON_REPEAT));
                            break;

                        case ROCKPAINT_UP:
                        case ROCKPAINT_UP | BUTTON_REPEAT:
                        case ROCKPAINT_DOWN:
                        case ROCKPAINT_DOWN | BUTTON_REPEAT:
                            incdec_value(&y, &incdec_y,
                                (button&ROCKPAINT_DOWN), (button&BUTTON_REPEAT));
                            break;

                        case ROCKPAINT_DRAW:
                            break;
                        default:
                            if(rb->default_event_handler(button)
                                == SYS_USB_CONNECTED)
                                button = ROCKPAINT_DRAW;
                            break;
                    }
                    if( button == ROCKPAINT_DRAW ) break;
                }
                break;

            case TEXT_MENU_APPLY:
                rb->lcd_set_foreground( rp_colors[ drawcolor ] );
                buffer_putsxyofs( save_buffer, COLS, ROWS, x, y, 0,
                                  buffer->text.text );
            case TEXT_MENU_CANCEL:
            default:
                restore_screen();
                if( buffer->text.font[0] )
                {
                    rb->snprintf( buffer->text.font, MAX_PATH,
                                  FONT_DIR "/%s.fnt",
                                  rb->global_settings->font_file );
                    if (current_font_id != rb->global_status->font_id[SCREEN_MAIN])
                        rb->font_unload(current_font_id);
                    rb->lcd_setfont(FONT_UI);
                }
                return;
        }
    }
}

static void draw_brush( int x, int y )
{
    int i,j;
    for( i=-bsize/2+(bsize+1)%2; i<=bsize/2; i++ )
    {
        for( j=-bsize/2+(bsize+1)%2; j<=bsize/2; j++ )
        {
            draw_pixel( x+i, y+j );
        }
    }
}

/* This is an implementation of Bresenham's line algorithm.
 * See http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm.
 */
static void draw_line( int x1, int y1, int x2, int y2 )
{
    int x = x1;
    int y = y1;
    int deltax = x2 - x1;
    int deltay = y2 - y1;
    int i;

    int xerr = abs(deltax);
    int yerr = abs(deltay);
    int xstep = deltax > 0 ? 1 : -1;
    int ystep = deltay > 0 ? 1 : -1;
    int err;

    if (yerr > xerr)
    {
        /* more vertical */
        err = yerr;
        xerr <<= 1;
        yerr <<= 1;

        /* to leave off the last pixel of the line, leave off the "+ 1" */
        for (i = err + 1; i; --i)
        {
            draw_pixel(x, y);
            y += ystep;
            err -= xerr;
            if (err < 0) {
                x += xstep;
                err += yerr;
            }
        }
    }
    else
    {
        /* more horizontal */
        err = xerr;
        xerr <<= 1;
        yerr <<= 1;

        for (i = err + 1; i; --i)
        {
            draw_pixel(x, y);
            x += xstep;
            err -= yerr;
            if (err < 0) {
                y += ystep;
                err += xerr;
            }
        }
    }
}

static void draw_curve( int x1, int y1, int x2, int y2,
                       int xa, int ya, int xb, int yb )
{
    int i = 0;
    short xl1, yl1;
    short xl2, yl2;
    short xl3, yl3;
    short xl4, yl4;
    short xr1, yr1;
    short xr2, yr2;
    short xr3, yr3;
    short xr4, yr4;
    short depth;
    short xh, yh;

    if( x1 == x2 && y1 == y2 )
    {
        draw_pixel( x1, y1 );
        return;
    }

//    if( preview )
    {
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        if( xa == -1 || ya == -1 )
        {
            rb->lcd_drawline( x1, y1, xb, yb );
            rb->lcd_drawline( x2, y2, xb, yb );
        }
        else
        {
            rb->lcd_drawline( x1, y1, xa, ya );
            rb->lcd_drawline( x2, y2, xb, yb );
        }
        rb->lcd_set_drawmode(DRMODE_SOLID);
    }

    if( xa == -1 || ya == -1 )
    /* We only have 3 of the points
     * This will currently only be used in preview mode */
    {
#define PUSH( a1, b1, a2, b2, a3, b3, d ) \
        buffer->bezier[i].x1 = a1; \
        buffer->bezier[i].y1 = b1; \
        buffer->bezier[i].x2 = a2; \
        buffer->bezier[i].y2 = b2; \
        buffer->bezier[i].x3 = a3; \
        buffer->bezier[i].y3 = b3; \
        buffer->bezier[i].depth = d; \
        i++;
#define POP( a1, b1, a2, b2, a3, b3, d ) \
        i--; \
        a1 = buffer->bezier[i].x1; \
        b1 = buffer->bezier[i].y1; \
        a2 = buffer->bezier[i].x2; \
        b2 = buffer->bezier[i].y2; \
        a3 = buffer->bezier[i].x3; \
        b3 = buffer->bezier[i].y3; \
        d = buffer->bezier[i].depth;

        PUSH( x1<<4, y1<<4, xb<<4, yb<<4, x2<<4, y2<<4, 0 );
        while( i )
        {
            /* de Casteljau's algorithm (see wikipedia) */
            POP( xl1, yl1, xb, yb, xr3, yr3, depth );
            if( depth < 10 ) /* check that the stack's 'i' doesn't overflow */
            {
                xl2 = ( xl1 + xb )>>1;
                yl2 = ( yl1 + yb )>>1;
                xr2 = ( xb + xr3 )>>1;
                yr2 = ( yb + yr3 )>>1;
                xr1 = ( xl2 + xr2 )>>1;
                yr1 = ( yl2 + yr2 )>>1;
                xl3 = xr1;
                yl3 = yr1;
                PUSH( xl1, yl1, xl2, yl2, xl3, yl3, depth+1 );
                PUSH( xr1, yr1, xr2, yr2, xr3, yr3, depth+1 );
            }
            else
            {
                draw_line( ((xl1>>3)+1)>>1, ((yl1>>3)+1)>>1,
                          ((xr3>>3)+1)>>1, ((yr3>>3)+1)>>1 );
            }
        }
#undef PUSH
#undef POP
    }
    else /* We have the 4 points */
    {
#define PUSH( a1, b1, a2, b2, a3, b3, a4, b4, d ) \
        buffer->bezier[i].x1 = a1; \
        buffer->bezier[i].y1 = b1; \
        buffer->bezier[i].x2 = a2; \
        buffer->bezier[i].y2 = b2; \
        buffer->bezier[i].x3 = a3; \
        buffer->bezier[i].y3 = b3; \
        buffer->bezier[i].x4 = a4; \
        buffer->bezier[i].y4 = b4; \
        buffer->bezier[i].depth = d; \
        i++;
#define POP( a1, b1, a2, b2, a3, b3, a4, b4, d ) \
        i--; \
        a1 = buffer->bezier[i].x1; \
        b1 = buffer->bezier[i].y1; \
        a2 = buffer->bezier[i].x2; \
        b2 = buffer->bezier[i].y2; \
        a3 = buffer->bezier[i].x3; \
        b3 = buffer->bezier[i].y3; \
        a4 = buffer->bezier[i].x4; \
        b4 = buffer->bezier[i].y4; \
        d = buffer->bezier[i].depth;

        PUSH( x1<<4, y1<<4, xa<<4, ya<<4, xb<<4, yb<<4, x2<<4, y2<<4, 0 );
        while( i )
        {
            /* de Casteljau's algorithm (see wikipedia) */
            POP( xl1, yl1, xa, ya, xb, yb, xr4, yr4, depth );
            if( depth < 10 ) /* check that the stack's 'i' doesn't overflow */
            {
                xl2 = ( xl1 + xa )>>1;
                yl2 = ( yl1 + ya )>>1;
                xh = ( xa + xb )>>1;
                yh = ( ya + yb )>>1;
                xr3 = ( xb + xr4 )>>1;
                yr3 = ( yb + yr4 )>>1;
                xl3 = ( xl2 + xh )>>1;
                yl3 = ( yl2 + yh )>>1;
                xr2 = ( xr3 + xh )>>1;
                yr2 = ( yr3 + yh )>>1;
                xl4 = ( xl3 + xr2 )>>1;
                yl4 = ( yl3 + yr2 )>>1;
                xr1 = xl4;
                yr1 = yl4;
                PUSH( xl1, yl1, xl2, yl2, xl3, yl3, xl4, yl4, depth+1 );
                PUSH( xr1, yr1, xr2, yr2, xr3, yr3, xr4, yr4, depth+1 );
            }
            else
            {
                draw_line( ((xl1>>3)+1)>>1, ((yl1>>3)+1)>>1,
                          ((xr4>>3)+1)>>1, ((yr4>>3)+1)>>1 );
            }
        }
#undef PUSH
#undef POP
    }
}

static void draw_rect( int x1, int y1, int x2, int y2 )
{
    draw_line( x1, y1, x1, y2 );
    draw_line( x1, y1, x2, y1 );
    draw_line( x1, y2, x2, y2 );
    draw_line( x2, y1, x2, y2 );
}

static void togglebg( void )
{
    if( isbg )
    {
        rb->lcd_set_foreground( rp_colors[ drawcolor ] );
    }
    else
    {
        rb->lcd_set_foreground( rp_colors[ bgdrawcolor ] );
    }
    isbg = !isbg;
}

static void draw_rect_full( int x1, int y1, int x2, int y2 )
{
    /* GRUIK */
    int x;
    togglebg();
    if( x1 > x2 )
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }
    x = x1;
    do {
        draw_line( x, y1, x, y2 );
    } while( ++x <= x2 );
    togglebg();
    draw_rect( x1, y1, x2, y2 );
}

static void draw_oval( int x1, int y1, int x2, int y2, bool full )
{
    /* TODO: simplify :) */
    int cx = (x1+x2)>>1;
    int cy = (y1+y2)>>1;

    int rx = (x1-x2)>>1;
    int ry = (y1-y2)>>1;
    if( rx < 0 ) rx *= -1;
    if( ry < 0 ) ry *= -1;

    if( rx == 0 || ry == 0 )
    {
        draw_line( x1, y1, x2, y2 );
        return;
    }

    int x,y;
    int dst, old_dst;

    for( x = 0; x < rx; x++ )
    {
        y = 0;
        dst = -0xfff;
        do {
            old_dst = dst;
            dst = ry * ry * x * x + rx * rx * y * y - rx * rx * ry * ry;
            y++;
        } while( dst < 0 );
        if( -old_dst < dst ) y--;
        if( full )
        {
            draw_line( cx+x, cy, cx+x, cy+y );
            draw_line( cx+x, cy, cx+x, cy-y );
            draw_line( cx-x, cy, cx-x, cy+y );
            draw_line( cx-x, cy, cx-x, cy-y );
        }
        else
        {
            draw_pixel( cx+x, cy+y );
            draw_pixel( cx+x, cy-y );
            draw_pixel( cx-x, cy+y );
            draw_pixel( cx-x, cy-y );
        }
    }
    for( y = 0; y < ry; y++ )
    {
        x = 0;
        dst = -0xfff;
        do {
            old_dst = dst;
            dst = ry * ry * x * x + rx * rx * y * y - rx * rx * ry * ry;
            x++;
        } while( dst < 0 );
        if( -old_dst < dst ) x--;
        if( full )
        {
            draw_line( cx+x, cy, cx+x, cy+y );
            draw_line( cx+x, cy, cx+x, cy-y );
            draw_line( cx-x, cy, cx-x, cy+y );
            draw_line( cx-x, cy, cx-x, cy-y );
        }
        else
        {
            draw_pixel( cx+x, cy+y );
            draw_pixel( cx+x, cy-y );
            draw_pixel( cx-x, cy+y );
            draw_pixel( cx-x, cy-y );
        }
    }
}

static void draw_oval_empty( int x1, int y1, int x2, int y2 )
{
    draw_oval( x1, y1, x2, y2, false );
}

static void draw_oval_full( int x1, int y1, int x2, int y2 )
{
    togglebg();
    draw_oval( x1, y1, x2, y2, true );
    togglebg();
    draw_oval( x1, y1, x2, y2, false );
}

static void draw_fill( int x0, int y0 )
{
#define PUSH( a, b ) \
    draw_pixel( (int)a, (int)b ); \
    buffer->coord[i].x = a; \
    buffer->coord[i].y = b; \
    i++;
#define POP( a, b ) \
    i--; \
    a = buffer->coord[i].x; \
    b = buffer->coord[i].y;

    unsigned int i=0;
    short x = x0;
    short y = y0;
    unsigned int prev_color = save_buffer[ x0+y0*COLS ];

    if( preview )
        return;
    if( prev_color == rp_colors[ drawcolor ] ) return;

    PUSH( x, y );

    while( i != 0 )
    {
        POP( x, y );
        if( x > 0 && save_buffer[x-1+y*COLS] == prev_color )
        {
            PUSH( x-1, y );
        }
        if( x < COLS-1 && save_buffer[x+1+y*COLS] == prev_color )
        {
            PUSH( x+1, y );
        }
        if( y > 0 && save_buffer[x+(y-1)*COLS] == prev_color )
        {
            PUSH( x, y-1 );
        }
        if( y < ROWS - 1 && save_buffer[x+(y+1)*COLS] == prev_color )
        {
            PUSH( x, y+1 );
        }
    }
#undef PUSH
#undef POP

}

/* For preview purposes only */
/* use same algorithm as draw_line() to draw line. */
static void line_gradient( int x1, int y1, int x2, int y2 )
{
    int h1, s1, v1, h2, s2, v2, r, g, b;
    int xerr = x2 - x1, yerr = y2 - y1, xstep, ystep;
    int i, delta, err;
    fb_data color1, color2;

    if( xerr == 0 && yerr == 0 )
    {
        draw_pixel( x1, y1 );
        return;
    }

    xstep = xerr > 0 ? 1 : -1;
    ystep = yerr > 0 ? 1 : -1;
    xerr = abs(xerr) << 1;
    yerr = abs(yerr) << 1;

    color1 = rp_colors[ bgdrawcolor ];
    color2 = rp_colors[ drawcolor ];

    r = RGB_UNPACK_RED( color1 );
    g = RGB_UNPACK_GREEN( color1 );
    b = RGB_UNPACK_BLUE( color1 );
    rgb2hsv( r, g, b, &h1, &s1, &v1 );

    r = RGB_UNPACK_RED( color2 );
    g = RGB_UNPACK_GREEN( color2 );
    b = RGB_UNPACK_BLUE( color2 );
    rgb2hsv( r, g, b, &h2, &s2, &v2 );

    if( xerr > yerr )
    {
        err = xerr>>1;
        delta = err+1;
        /* to leave off the last pixel of the line, leave off the "+ 1" */
        for (i = delta; i; --i)
        {
            hsv2rgb( h2+((h1-h2)*i)/delta,
                     s2+((s1-s2)*i)/delta,
                     v2+((v1-v2)*i)/delta,
                     &r, &g, &b );
            rp_colors[ drawcolor ] = LCD_RGBPACK( r, g, b );
            rb->lcd_set_foreground( rp_colors[ drawcolor ] );
            draw_pixel(x1, y1);
            x1 += xstep;
            err -= yerr;
            if (err < 0) {
                y1 += ystep;
                err += xerr;
            }
        }
    }
    else /* yerr >= xerr */
    {
        err = yerr>>1;
        delta = err+1;
        for (i = delta; i; --i)
        {
            hsv2rgb( h2+((h1-h2)*i)/delta,
                     s2+((s1-s2)*i)/delta,
                     v2+((v1-v2)*i)/delta,
                     &r, &g, &b );
            rp_colors[ drawcolor ] = LCD_RGBPACK( r, g, b );
            rb->lcd_set_foreground( rp_colors[ drawcolor ] );
            draw_pixel(x1, y1);
            y1 += ystep;
            err -= xerr;
            if (err < 0) {
                x1 += xstep;
                err += yerr;
            }
        }
    }
    rp_colors[ drawcolor ] = color2;
}

/* macros used by linear_gradient() and radial_gradient(). */
#define PUSH( _x, _y ) \
    save_buffer[(_x)+(_y)*COLS] = mark_color; \
    buffer->coord[i].x = (short)(_x); \
    buffer->coord[i].y = (short)(_y); \
    i++;
#define POP( _x, _y ) \
    i--; \
    _x = (int)buffer->coord[i].x; \
    _y = (int)buffer->coord[i].y;
#define PUSH2( _x, _y ) \
    j--; \
    buffer->coord[j].x = (short)(_x); \
    buffer->coord[j].y = (short)(_y);
#define POP2( _x, _y ) \
    _x = (int)buffer->coord[j].x; \
    _y = (int)buffer->coord[j].y; \
    j++;

static void linear_gradient( int x1, int y1, int x2, int y2 )
{
    int r1 = RGB_UNPACK_RED( rp_colors[ bgdrawcolor ] );
    int g1 = RGB_UNPACK_GREEN( rp_colors[ bgdrawcolor ] );
    int b1 = RGB_UNPACK_BLUE( rp_colors[ bgdrawcolor ] );
    int r2 = RGB_UNPACK_RED( rp_colors[ drawcolor ] );
    int g2 = RGB_UNPACK_GREEN( rp_colors[ drawcolor ] );
    int b2 = RGB_UNPACK_BLUE( rp_colors[ drawcolor ] );
    fb_data color = rp_colors[ drawcolor ];

    int h1, s1, v1, h2, s2, v2, r, g, b;

    /* radius^2 */
    int radius2 = ( x1 - x2 ) * ( x1 - x2 ) + ( y1 - y2 ) * ( y1 - y2 );
    int dist2, i=0, j=COLS*ROWS;

    /* We only propagate the gradient to neighboring pixels with the same
     * color as ( x1, y1 ) */
    fb_data prev_color = save_buffer[ x1+y1*COLS ];
    /* to mark pixel that the pixel is already in LIFO. */
    fb_data mark_color = ~prev_color;

    int x = x1;
    int y = y1;

    if( radius2 == 0 ) return;
    if( preview )
    {
        line_gradient( x1, y1, x2, y2 );
        return;
    }
    if( rp_colors[ drawcolor ] == rp_colors[ bgdrawcolor ] )
    {
        draw_fill( x1, y1 );
        return;
    }

    rgb2hsv( r1, g1, b1, &h1, &s1, &v1 );
    rgb2hsv( r2, g2, b2, &h2, &s2, &v2 );

    PUSH( x, y );

    while( i > 0 )
    {
        POP( x, y );

        dist2 = ( x2 - x1 ) * ( x - x1 ) + ( y2 - y1 ) * ( y - y1 );
        if( dist2 <= 0 )
        {
            rp_colors[ drawcolor ] = rp_colors[ bgdrawcolor ];
        }
        else if( dist2 < radius2 )
        {
            hsv2rgb( h1+((h2-h1)*dist2)/radius2,
                     s1+((s2-s1)*dist2)/radius2,
                     v1+((v2-v1)*dist2)/radius2,
                     &r, &g, &b );
            rp_colors[ drawcolor ] = LCD_RGBPACK( r, g, b );
        }
        else
        {
            rp_colors[ drawcolor ] = color;
        }
        if( rp_colors[ drawcolor ] == prev_color )
        {
            /* "mark" that pixel was checked. correct color later. */
            PUSH2( x, y );
            rp_colors[ drawcolor ] = mark_color;
        }
        rb->lcd_set_foreground( rp_colors[ drawcolor ] );
        draw_pixel( x, y );

        if( x > 0 && save_buffer[x-1+y*COLS] == prev_color )
        {
            PUSH( x-1, y );
        }
        if( x < COLS-1 && save_buffer[x+1+y*COLS] == prev_color )
        {
            PUSH( x+1, y );
        }
        if( y > 0 && save_buffer[x+(y-1)*COLS] == prev_color )
        {
            PUSH( x, y-1 );
        }
        if( y < ROWS - 1 && save_buffer[x+(y+1)*COLS] == prev_color )
        {
            PUSH( x, y+1 );
        }
    }
    while (j < COLS*ROWS)
    {
        /* correct color. */
        POP2( x, y );
        rp_colors[ drawcolor ] = prev_color;
        rb->lcd_set_foreground( rp_colors[ drawcolor ] );
        draw_pixel( x, y );
    }
    rp_colors[ drawcolor ] = color;
}

static void radial_gradient( int x1, int y1, int x2, int y2 )
{
    int r1 = RGB_UNPACK_RED( rp_colors[ bgdrawcolor ] );
    int g1 = RGB_UNPACK_GREEN( rp_colors[ bgdrawcolor ] );
    int b1 = RGB_UNPACK_BLUE( rp_colors[ bgdrawcolor ] );
    int r2 = RGB_UNPACK_RED( rp_colors[ drawcolor ] );
    int g2 = RGB_UNPACK_GREEN( rp_colors[ drawcolor ] );
    int b2 = RGB_UNPACK_BLUE( rp_colors[ drawcolor ] );
    fb_data color = rp_colors[ drawcolor ];

    int h1, s1, v1, h2, s2, v2, r, g, b;

    /* radius^2 */
    int radius2 = ( x1 - x2 ) * ( x1 - x2 ) + ( y1 - y2 ) * ( y1 - y2 );
    int dist2, i=0, j=COLS*ROWS;

    /* We only propagate the gradient to neighboring pixels with the same
     * color as ( x1, y1 ) */
    fb_data prev_color = save_buffer[ x1+y1*COLS ];
    /* to mark pixel that the pixel is already in LIFO. */
    fb_data mark_color = ~prev_color;

    int x = x1;
    int y = y1;

    if( radius2 == 0 ) return;
    if( preview )
    {
        line_gradient( x1, y1, x2, y2 );
        return;
    }
    if( rp_colors[ drawcolor ] == rp_colors[ bgdrawcolor ] )
    {
        draw_fill( x1, y1 );
        return;
    }

    rgb2hsv( r1, g1, b1, &h1, &s1, &v1 );
    rgb2hsv( r2, g2, b2, &h2, &s2, &v2 );

    PUSH( x, y );

    while( i > 0 )
    {
        POP( x, y );

        dist2 = ( x - x1 ) * ( x - x1 ) + ( y - y1 ) * ( y - y1 );
        if( dist2 < radius2 )
        {
            hsv2rgb( h1+((h2-h1)*dist2)/radius2,
                     s1+((s2-s1)*dist2)/radius2,
                     v1+((v2-v1)*dist2)/radius2,
                     &r, &g, &b );
            rp_colors[ drawcolor ] = LCD_RGBPACK( r, g, b );
        }
        else
        {
            rp_colors[ drawcolor ] = color;
        }
        if( rp_colors[ drawcolor ] == prev_color )
        {
            /* "mark" that pixel was checked. correct color later. */
            PUSH2( x, y );
            rp_colors[ drawcolor ] = mark_color;
        }
        rb->lcd_set_foreground( rp_colors[ drawcolor ] );
        draw_pixel( x, y );

        if( x > 0 && save_buffer[x-1+y*COLS] == prev_color )
        {
            PUSH( x-1, y );
        }
        if( x < COLS-1 && save_buffer[x+1+y*COLS] == prev_color )
        {
            PUSH( x+1, y );
        }
        if( y > 0 && save_buffer[x+(y-1)*COLS] == prev_color )
        {
            PUSH( x, y-1 );
        }
        if( y < ROWS - 1 && save_buffer[x+(y+1)*COLS] == prev_color )
        {
            PUSH( x, y+1 );
        }
    }
    while (j < COLS*ROWS)
    {
        /* correct color. */
        POP2( x, y );
        rp_colors[ drawcolor ] = prev_color;
        rb->lcd_set_foreground( rp_colors[ drawcolor ] );
        draw_pixel( x, y );
    }
    rp_colors[ drawcolor ] = color;
}

#undef PUSH
#undef POP
#undef PUSH2
#undef POP2

static void draw_toolbars(bool update)
{
    int i;
#define TOP (LCD_HEIGHT-TB_HEIGHT)
    rb->lcd_set_background( COLOR_LIGHTGRAY );
    rb->lcd_set_foreground( COLOR_LIGHTGRAY );
    rb->lcd_fillrect( 0, TOP, LCD_WIDTH, TB_HEIGHT );
    rb->lcd_set_foreground( COLOR_BLACK );
    rb->lcd_drawrect( 0, TOP, LCD_WIDTH, TB_HEIGHT );

    rb->lcd_set_foreground( rp_colors[ bgdrawcolor ] );
    rb->lcd_fillrect( TB_SC_BG_LEFT, TOP+TB_SC_BG_TOP,
                      TB_SC_SIZE, TB_SC_SIZE );
    rb->lcd_set_foreground(ROCKPAINT_PALETTE);
    rb->lcd_drawrect( TB_SC_BG_LEFT, TOP+TB_SC_BG_TOP,
                      TB_SC_SIZE, TB_SC_SIZE );
    rb->lcd_set_foreground( rp_colors[ drawcolor ] );
    rb->lcd_fillrect( TB_SC_FG_LEFT, TOP+TB_SC_FG_TOP,
                      TB_SC_SIZE, TB_SC_SIZE );
    rb->lcd_set_foreground(ROCKPAINT_PALETTE);
    rb->lcd_drawrect( TB_SC_FG_LEFT, TOP+TB_SC_FG_TOP,
                      TB_SC_SIZE, TB_SC_SIZE );

    for( i=0; i<18; i++ )
    {
        rb->lcd_set_foreground( rp_colors[i] );
        rb->lcd_fillrect(
                TB_PL_LEFT+(i%9)*( TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING ),
                TOP+TB_PL_TOP+(i/9)*( TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING),
                TB_PL_COLOR_SIZE, TB_PL_COLOR_SIZE );
        rb->lcd_set_foreground( ROCKPAINT_PALETTE );
        rb->lcd_drawrect(
                TB_PL_LEFT+(i%9)*( TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING ),
                TOP+TB_PL_TOP+(i/9)*( TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING),
                TB_PL_COLOR_SIZE, TB_PL_COLOR_SIZE );
    }

#define SEPARATOR( x, y ) \
    rb->lcd_set_foreground( COLOR_WHITE ); \
    rb->lcd_vline( x, TOP+y, TOP+y+TB_PL_HEIGHT-1 ); \
    rb->lcd_set_foreground( COLOR_DARKGRAY ); \
    rb->lcd_vline( x+1, TOP+y, TOP+y+TB_PL_HEIGHT-1 );
    SEPARATOR( TB_PL_LEFT + TB_PL_WIDTH - 1 + TB_SP_MARGIN, TB_PL_TOP );

    rb->lcd_bitmap_transparent( rockpaint, TB_TL_LEFT, TOP+TB_TL_TOP,
                                TB_TL_WIDTH, TB_TL_HEIGHT );
    rb->lcd_set_foreground(ROCKPAINT_PALETTE);
    rb->lcd_drawrect( TB_TL_LEFT+(TB_TL_SIZE+TB_TL_SPACING)*(tool/2),
                      TOP+TB_TL_TOP+(TB_TL_SIZE+TB_TL_SPACING)*(tool%2),
                      TB_TL_SIZE, TB_TL_SIZE );

    SEPARATOR( TB_TL_LEFT + TB_TL_WIDTH - 1 + TB_SP_MARGIN, TB_TL_TOP );

    rb->lcd_setfont( FONT_SYSFIXED );
    rb->lcd_putsxy( TB_MENU_LEFT, TOP+TB_MENU_TOP, "Menu" );
    rb->lcd_setfont( FONT_UI );
#undef TOP

    if( update ) rb->lcd_update();
}

static void toolbar( void )
{
    int button, i, j;
    restore_screen();
    draw_toolbars( false );
    y = LCD_HEIGHT-TB_HEIGHT/2;
    inv_cursor( true );
    while( 1 )
    {
        switch( button = rb->button_get( true ) )
        {
            case ROCKPAINT_DRAW:
#define TOP ( LCD_HEIGHT - TB_HEIGHT )
                if( y >= TOP + TB_SC_FG_TOP
                    && y < TOP + TB_SC_FG_TOP + TB_SC_SIZE
                    && x >= TB_SC_FG_LEFT
                    && x < TB_SC_FG_LEFT + TB_SC_SIZE )
                {
                    /* click on the foreground color */
                    rp_colors[drawcolor] = color_chooser( rp_colors[drawcolor] );
                }
                else if( y >= TOP + TB_SC_BG_TOP
                         && y < TOP + TB_SC_BG_TOP + TB_SC_SIZE
                         && x >= TB_SC_BG_LEFT
                         && x < TB_SC_BG_LEFT + TB_SC_SIZE )
                {
                    /* click on the background color */
                    i = drawcolor;
                    drawcolor = bgdrawcolor;
                    bgdrawcolor = i;
                }
                else if( y >= TOP + TB_PL_TOP
                         && y < TOP + TB_PL_TOP + TB_PL_HEIGHT
                         && x >= TB_PL_LEFT
                         && x < TB_PL_LEFT + TB_PL_WIDTH )
                {
                    /* click on the palette */
                    i = (x - TB_PL_LEFT)%(TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING);
                    j = (y - (TOP+TB_PL_TOP) )%(TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING);
                    if( i >= TB_PL_COLOR_SIZE || j >= TB_PL_COLOR_SIZE )
                        break;
                    i = ( x - TB_PL_LEFT )/(TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING);
                    j = ( y - (TOP+TB_PL_TOP) )/(TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING);
                    drawcolor = j*(TB_PL_COLOR_SIZE+TB_PL_COLOR_SPACING)+i;
                }
                else if( y >= TOP+TB_TL_TOP
                         && y < TOP + TB_TL_TOP + TB_TL_HEIGHT
                         && x >= TB_TL_LEFT
                         && x <= TB_TL_LEFT + TB_TL_WIDTH )
                {
                    /* click on the tools */
                    i = (x - TB_TL_LEFT ) % (TB_TL_SIZE+TB_TL_SPACING);
                    j = (y - (TOP+TB_TL_TOP) ) %(TB_TL_SIZE+TB_TL_SPACING);
                    if( i >= TB_TL_SIZE || j >= TB_TL_SIZE ) break;
                    i = ( x - TB_TL_LEFT )/(TB_TL_SIZE+TB_TL_SPACING);
                    j = ( y - (TOP+TB_TL_TOP) )/(TB_TL_SIZE+TB_TL_SPACING);
                    tool = i*2+j;
                    reset_tool();
                    if( tool == Text )
                    {
                        buffer->text.initialized = false;
                    }
                }
                else if( x >= TB_MENU_LEFT && y >= TOP+TB_MENU_TOP-2)
                {
                    /* menu button */
                    goto_menu();
                }
#undef TOP
                restore_screen();
                draw_toolbars( false );
                inv_cursor( true );
                break;

            case ROCKPAINT_LEFT:
            case ROCKPAINT_LEFT | BUTTON_REPEAT:
            case ROCKPAINT_RIGHT:
            case ROCKPAINT_RIGHT | BUTTON_REPEAT:
                inv_cursor(false);
                incdec_value(&x, &incdec_x,
                    (button&ROCKPAINT_RIGHT), (button&BUTTON_REPEAT));
                inv_cursor(true);
                break;

            case ROCKPAINT_UP:
            case ROCKPAINT_UP | BUTTON_REPEAT:
            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN | BUTTON_REPEAT:
                inv_cursor(false);
                if (incdec_value(&y, &incdec_y,
                        (button&ROCKPAINT_DOWN), (button&BUTTON_REPEAT))
                    || y < LCD_HEIGHT-TB_HEIGHT)
                {
                    /* went out of region. exit toolbar. */
                    return;
                }
                inv_cursor(true);
                break;

            case ROCKPAINT_TOOLBAR:
            case ROCKPAINT_TOOLBAR2:
                return;
        }
        if( quit ) return;
    }
}

static void inv_cursor(bool update)
{
    rb->lcd_set_foreground(COLOR_BLACK);
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    /* cross painting */
    rb->lcd_hline(x-4,x+4,y);
    rb->lcd_vline(x,y-4,y+4);
    rb->lcd_set_foreground(rp_colors[drawcolor]);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    if( update ) rb->lcd_update();
}

static void restore_screen(void)
{
    rb->lcd_bitmap( save_buffer, 0, 0, COLS, ROWS );
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_vline( img_width, 0, ROWS );
    rb->lcd_hline( 0, COLS, img_height );
    rb->lcd_drawpixel( img_width, img_height );
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

static void clear_drawing(void)
{
    init_buffer();
    img_height = ROWS;
    img_width = COLS;
    rb->lcd_set_foreground( rp_colors[ bgdrawcolor ] );
    rb->lcd_fillrect( 0, 0, COLS, ROWS );
    rb->lcd_update();
}

static void goto_menu(void)
{
    int multi;
    int selected = 0;

    while( 1 )
    {
        switch( rb->do_menu( &main_menu, &selected, NULL, false ) )
        {
            case MAIN_MENU_NEW:
                clear_drawing();
                return;

            case MAIN_MENU_LOAD:
                if( browse( filename, MAX_PATH, "/" ) )
                {
                    if( load_bitmap( filename ) <= 0 )
                    {
                        rb->splashf( 1*HZ, "Error while loading %s",
                                    filename );
                        clear_drawing();
                    }
                    else
                    {
                        rb->splashf( 1*HZ, "Image loaded (%s)", filename );
                        restore_screen();
                        inv_cursor(true);
                        return;
                    }
                }
                break;

            case MAIN_MENU_SAVE:
                rb->lcd_set_foreground(COLOR_BLACK);
                if (!filename[0])
                    rb->strcpy(filename,"/");
                if( !rb->kbd_input( filename, MAX_PATH ) )
                {
                    if( !check_extention( filename, ".bmp" ) )
                        rb->strcat(filename, ".bmp");
                    save_bitmap( filename );
                    rb->splashf( 1*HZ, "File saved (%s)", filename );
                }
                break;

            case MAIN_MENU_SET_WIDTH:
                rb->set_int( "Set Width", "px", UNIT_INT, &img_width,
                             NULL, 1, 1, COLS, NULL );
                break;
            case MAIN_MENU_SET_HEIGHT:
                rb->set_int( "Set Height", "px", UNIT_INT, &img_height,
                             NULL, 1, 1, ROWS, NULL );
                break;
            case MAIN_MENU_BRUSH_SIZE:
                for(multi = 0; multi<4; multi++)
                    if(bsize == times_list[multi]) break;
                rb->set_option( "Brush Size", &multi, INT, times_options, 4, NULL );
                if( multi >= 0 )
                    bsize = times_list[multi];
                break;

            case MAIN_MENU_BRUSH_SPEED:
                for(multi = 0; multi<3; multi++)
                    if(bspeed == times_list[multi]) break;
                rb->set_option( "Brush Speed", &multi, INT, times_options, 3, NULL );
                if( multi >= 0 ) {
                    bspeed = times_list[multi];
                    incdec_x.step[0] = bspeed;
                    incdec_x.step[1] = bspeed * 4;
                    incdec_y.step[0] = bspeed;
                    incdec_y.step[1] = bspeed * 4;
                }
                break;

            case MAIN_MENU_COLOR:
                rp_colors[drawcolor] = color_chooser( rp_colors[drawcolor] );
                break;

            case MAIN_MENU_GRID_SIZE:
                for(multi = 0; multi<4; multi++)
                    if(gridsize == gridsize_list[multi]) break;
                rb->set_option( "Grid Size", &multi, INT, gridsize_options, 4, NULL );
                if( multi >= 0 )
                    gridsize = gridsize_list[multi];
                break;

            case MAIN_MENU_PLAYBACK_CONTROL:
                if (!audio_buf)
                    playback_control( NULL );
                else
                    rb->splash(HZ, "Cannot restart playback");
                break;

            case MAIN_MENU_EXIT:
                restore_screen();
                quit=true;
                return;

            case MAIN_MENU_RESUME:
            default:
                restore_screen();
                return;
        }/* end switch */
    }/* end while */
}

static void reset_tool( void )
{
    prev_x = -1;
    prev_y = -1;
    prev_x2 = -1;
    prev_y2 = -1;
    prev_x3 = -1;
    prev_y3 = -1;
    /* reset state */
    state = State0;
    /* always preview color picker */
    preview = (tool == ColorPicker);
}

/* brush tool */
static void state_func_brush(void)
{
    if( state == State0 )
    {
        state = State1;
    }
    else
    {
        state = State0;
    }
}

/* fill tool */
static void state_func_fill(void)
{
    draw_fill( x, y );
    restore_screen();
}

/* select rectangle tool */
static void state_func_select(void)
{
    int mode;
    if( state == State0 )
    {
        prev_x = x;
        prev_y = y;
        preview = true;
        state = State1;
    }
    else if( state == State1 )
    {
        mode = rb->do_menu( &select_menu, NULL, NULL, false );
        switch( mode )
        {
            case SELECT_MENU_CUT:
            case SELECT_MENU_COPY:
                prev_x2 = x;
                prev_y2 = y;
                if( prev_x < x ) x = prev_x;
                if( prev_y < y ) y = prev_y;
                prev_x3 = abs(prev_x2 - prev_x);
                prev_y3 = abs(prev_y2 - prev_y);
                copy_to_clipboard();
                state = (mode == SELECT_MENU_CUT? State2: State3);
                break;

            case SELECT_MENU_INVERT:
                draw_invert( prev_x, prev_y, x, y );
                reset_tool();
                break;

            case SELECT_MENU_HFLIP:
                draw_hflip( prev_x, prev_y, x, y );
                reset_tool();
                break;

            case SELECT_MENU_VFLIP:
                draw_vflip( prev_x, prev_y, x, y );
                reset_tool();
                break;

            case SELECT_MENU_ROTATE90:
                draw_rot_90_deg( prev_x, prev_y, x, y, 1 );
                reset_tool();
                break;

            case SELECT_MENU_ROTATE180:
                draw_hflip( prev_x, prev_y, x, y );
                draw_vflip( prev_x, prev_y, x, y );
                reset_tool();
                break;

            case SELECT_MENU_ROTATE270:
                draw_rot_90_deg( prev_x, prev_y, x, y, -1 );
                reset_tool();
                break;

            case SELECT_MENU_CANCEL:
                reset_tool();
                break;

            default:
                break;
        }
        restore_screen();
    }
    else
    {
        preview = false;
        draw_paste_rectangle( prev_x, prev_y, prev_x2, prev_y2,
                              x, y, state == State2 );
        reset_tool();
        restore_screen();
    }
}

static void preview_select(void)
{
    if( state == State1 )
    {
        /* we are defining the selection */
        draw_select_rectangle( prev_x, prev_y, x, y );
    }
    else
    {
        /* we are pasting the selected data */
        draw_paste_rectangle( prev_x, prev_y, prev_x2, prev_y2,
                              x, y, state == State2 );
        draw_select_rectangle( x, y, x+prev_x3, y+prev_y3 );
    }
}

/* color picker tool */
static void state_func_picker(void)
{
    preview = false;
    color_picker( x, y );
    reset_tool();
}

static void preview_picker(void)
{
    color_picker( x, y );
}

/* curve tool */
static void state_func_curve(void)
{
    if( state == State0 )
    {
        prev_x = x;
        prev_y = y;
        preview = true;
        state = State1;
    }
    else if( state == State1 )
    {
        prev_x2 = x;
        prev_y2 = y;
        state = State2;
    }
    else if( state == State2 )
    {
        prev_x3 = x;
        prev_y3 = y;
        state = State3;
    }
    else
    {
        preview = false;
        draw_curve( prev_x, prev_y, prev_x2, prev_y2,
                   prev_x3, prev_y3, x, y );
        reset_tool();
        restore_screen();
    }
}

static void preview_curve(void)
{
    if( state == State1 )
    {
        draw_line( prev_x, prev_y, x, y );
    }
    else
    {
        draw_curve( prev_x, prev_y, prev_x2, prev_y2,
                   prev_x3, prev_y3, x, y );
    }
}

/* text tool */
static void state_func_text(void)
{
    draw_text( x, y );
}

/* tools which take 2 point */
static void preview_2point(void);
static void state_func_2point(void)
{
    if( state == State0 )
    {
        prev_x = x;
        prev_y = y;
        state = State1;
        preview = true;
    }
    else
    {
        preview = false;
        preview_2point();
        reset_tool();
        restore_screen();
    }
}

static void preview_2point(void)
{
    if( state == State1 )
    {
        switch( tool )
        {
            case Line:
                draw_line( prev_x, prev_y, x, y );
                break;
            case Rectangle:
                draw_rect( prev_x, prev_y, x, y );
                break;
            case RectangleFull:
                draw_rect_full( prev_x, prev_y, x, y );
                break;
            case Oval:
                draw_oval_empty( prev_x, prev_y, x, y );
                break;
            case OvalFull:
                draw_oval_full( prev_x, prev_y, x, y );
                break;
            case LinearGradient:
                linear_gradient( prev_x, prev_y, x, y );
                break;
            case RadialGradient:
                radial_gradient( prev_x, prev_y, x, y );
                break;
            default:
                break;
        }
        if( !preview )
        {
            reset_tool();
            restore_screen();
        }
    }
}

static const struct tool_func tools[14] = {
    [Brush]           = { state_func_brush,  NULL },
    [Fill]            = { state_func_fill,   NULL },
    [SelectRectangle] = { state_func_select, preview_select },
    [ColorPicker]     = { state_func_picker, preview_picker },
    [Line]            = { state_func_2point, preview_2point },
    [Unused]          = { NULL, NULL },
    [Curve]           = { state_func_curve,  preview_curve },
    [Text]            = { state_func_text,   NULL },
    [Rectangle]       = { state_func_2point, preview_2point },
    [RectangleFull]   = { state_func_2point, preview_2point },
    [Oval]            = { state_func_2point, preview_2point },
    [OvalFull]        = { state_func_2point, preview_2point },
    [LinearGradient]  = { state_func_2point, preview_2point },
    [RadialGradient]  = { state_func_2point, preview_2point },
};

static bool rockpaint_loop( void )
{
    int button = 0, i, j;
    bool bigstep;

    x = 10;
    toolbar();
    x = 0; y = 0;
    restore_screen();
    inv_cursor(true);

    while (!quit) {
        button = rb->button_get(true);
        bigstep = (button & BUTTON_REPEAT) && !(tool == Brush && state == State1);

        switch(button)
        {
            case ROCKPAINT_QUIT:
                if (state != State0)
                {
                    reset_tool();
                    restore_screen();
                    inv_cursor(true);
                }
                else
                {
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                    return PLUGIN_OK;
                }
                break;

            case ROCKPAINT_MENU:
                goto_menu();
                restore_screen();
                inv_cursor(true);
                break;

            case ROCKPAINT_DRAW:
                if( tools[tool].state_func )
                {
                    inv_cursor(false);
                    tools[tool].state_func();
                    inv_cursor(true);
                }
                break;

            case ROCKPAINT_DRAW|BUTTON_REPEAT:
                if( tool == Curve && state != State0 )
                {
                    /* 3 point bezier curve */
                    preview = false;
                    draw_curve( prev_x, prev_y, prev_x2, prev_y2,
                               -1, -1, x, y );
                    reset_tool();
                    restore_screen();
                    inv_cursor( true );
                }
                break;

            case ROCKPAINT_TOOLBAR:
            case ROCKPAINT_TOOLBAR2:
                i = x; j = y;
                x = (button == ROCKPAINT_TOOLBAR2) ? 110: 10;
                toolbar();
                x = i; y = j;
                restore_screen();
                inv_cursor(true);
                break;

            case ROCKPAINT_LEFT:
            case ROCKPAINT_LEFT | BUTTON_REPEAT:
            case ROCKPAINT_RIGHT:
            case ROCKPAINT_RIGHT | BUTTON_REPEAT:
                inv_cursor(false);
                incdec_value(&x, &incdec_x,
                    (button&ROCKPAINT_RIGHT), bigstep);
                inv_cursor(true);
                break;

            case ROCKPAINT_UP:
            case ROCKPAINT_UP | BUTTON_REPEAT:
            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN | BUTTON_REPEAT:
                inv_cursor(false);
                if (incdec_value(&y, &incdec_y,
                        (button&ROCKPAINT_DOWN), bigstep)
                    && (button&ROCKPAINT_DOWN))
                {
                    toolbar();
                    restore_screen();
                }
                inv_cursor(true);
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if( tool == Brush && state == State1 )
        {
            inv_cursor(false);
            draw_brush( x, y );
            inv_cursor(true);
        }
        if( preview && tools[tool].preview_func )
        {
            restore_screen();
            tools[tool].preview_func();
            inv_cursor( true );
        }
        if( gridsize > 0 )
        {
            show_grid( true );
            show_grid( false );
        }
    }

    return PLUGIN_OK;
}

static int load_bitmap( const char *file )
{
    struct bitmap bm;
    bool ret;
    int i, j;
    fb_data color = rp_colors[ bgdrawcolor ];

    bm.data = (char*)save_buffer;
    ret = rb->read_bmp_file( file, &bm, ROWS*COLS*sizeof( fb_data ),
                             FORMAT_NATIVE, NULL );

    if((bm.width > COLS ) || ( bm.height > ROWS ))
        return -1;

    img_width = bm.width;
    img_height = bm.height;
    for( i = bm.height-1; i >= 0; i-- )
    {
        rb->memmove( save_buffer+i*COLS, save_buffer+i*bm.width,
                        sizeof( fb_data )*bm.width );
        for( j = bm.width; j < COLS; j++ )
            save_buffer[j+i*COLS] = color;
    }
    for( i = bm.height*COLS; i < ROWS*COLS; i++ )
        save_buffer[i] = color;

    return ret;
}

static int save_bitmap( char *file )
{
    struct bitmap bm;
    int i;
    for(i = 0; i < img_height; i++)
    {
        rb->memcpy( buffer->clipboard+i*img_width, save_buffer+i*COLS,
                        sizeof( fb_data )*img_width );
    }
    bm.data = (char*)buffer->clipboard;
    bm.height = img_height;
    bm.width = img_width;
    bm.format = FORMAT_NATIVE;
    return save_bmp_file( file, &bm );
}

enum plugin_status plugin_start(const void* parameter)
{
    size_t buffer_size;
    unsigned char *temp;
    temp = rb->plugin_get_buffer(&buffer_size);
    if (buffer_size < sizeof(*buffer) + 3)
    {
        /* steal from audiobuffer if plugin buffer is too small */
        temp = rb->plugin_get_audio_buffer(&buffer_size);
        if (buffer_size < sizeof(*buffer) + 3)
        {
            rb->splash(HZ, "Not enough memory");
            return PLUGIN_ERROR;
        }
        audio_buf = true;
    }
    buffer = (union buf*) (((uintptr_t)temp + 3) & ~3);

    rb->lcd_set_foreground(COLOR_WHITE);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_fillrect(0,0,LCD_WIDTH,LCD_HEIGHT);
    rb->splash( HZ/2, "Rock Paint");

    rb->lcd_clear_display();

    filename[0] = '\0';

    if( parameter )
    {
        if( load_bitmap( parameter ) <= 0 )
        {
            rb->splash( 1*HZ, "File Open Error");
            clear_drawing();
        }
        else
        {
            rb->splashf( 1*HZ, "Image loaded (%s)", (char *)parameter );
            restore_screen();
            rb->strcpy( filename, parameter );
        }
    }
    else
    {
        clear_drawing();
    }
    inv_cursor(true);

    return rockpaint_loop();
}
