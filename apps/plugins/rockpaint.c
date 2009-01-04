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
 *  - cache fonts when building the font preview (else it only works well on simulators because they have "fast" disk read)
 */

#include "plugin.h"
#include "lib/bmp.h"
#include "lib/rgb_hsv.h"

PLUGIN_HEADER

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
#define ROCKPAINT_QUIT      BUTTON_POWER
#define ROCKPAINT_DRAW      BUTTON_SELECT
#define ROCKPAINT_MENU      ( BUTTON_SELECT | BUTTON_DOWN )
/* FIXME:
#define ROCKPAINT_TOOLBAR   BUTTON_HOME
#define ROCKPAINT_TOOLBAR2  ( BUTTON_HOME | BUTTON_LEFT ) */
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

#elif ( CONFIG_KEYPAD == COWOND2_PAD )
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

#define SPLASH_SCREEN PLUGIN_APPS_DIR "/rockpaint/splash.bmp"
#define ROCKPAINT_TITLE_FONT  2

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
static void draw_toolbars(bool update);
static void inv_cursor(bool update);
static void restore_screen(void);
static void clear_drawing(void);
static void goto_menu(void);
static int load_bitmap( const char *filename );
static int save_bitmap( char *filename );
static void draw_rect_full( int x1, int y1, int x2, int y2 );
extern int errno;

/***********************************************************************
 * Global variables
 ***********************************************************************/

#if !defined(SIMULATOR) || defined(__MINGW32__) || defined(__CYGWIN__)
int errno;
#endif

static const struct plugin_api* rb;

MEM_FUNCTION_WRAPPERS(rb);

static int drawcolor=0; /* Current color (in palette) */
static int bgdrawcolor=9; /* Current background color (in palette) */
bool isbg = false; /* gruik ugly hack alert */

static int preview=false; /* Is preview mode on ? */

/* TODO: clean this up */
static int x=0, y=0; /* cursor position */
static int prev_x=-1, prev_y=-1; /* previous saved cursor position */
static int prev_x2=-1, prev_y2=-1;
static int prev_x3=-1, prev_y3=-1;
static int tool_mode=-1;


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

enum Tools tool = Brush;

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

extern fb_data rockpaint[];
extern fb_data rockpaint_hsvrgb[];

/* Maximum string size allowed for the text tool */
#define MAX_TEXT 255

static union
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
        char text[MAX_TEXT+1];
        char font[MAX_PATH+1];
        char old_font[MAX_PATH+1];
        int fh_buf[30];
        int fw_buf[30];
        char fontname_buf[30][MAX_PATH];
    } text;
} buffer;

/* Current filename */
static char filename[MAX_PATH+1];

/* Font preview buffer */
//#define FONT_PREVIEW_WIDTH ((LCD_WIDTH-30)/8)
//#define FONT_PREVIEW_HEIGHT 1000
//static unsigned char fontpreview[FONT_PREVIEW_WIDTH*FONT_PREVIEW_HEIGHT];

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

        buffer_mono_bitmap_part( buf, buf_width, buf_height, bits, ofs, 0, width, x, y, width - ofs, pf->height);

        x += width - ofs;
        ofs = 0;
    }
}

/***********************************************************************
 * Menu handling
 ***********************************************************************/
struct menu_items
{
    int value;
    char label[16]; /* GRUIK ? */
};

#define MENU_ESC -1242
enum {
        /* Generic menu items */
        MENU_END = -42, MENU_TITLE = -12,
        /* Main menu */
        MAIN_MENU_RESUME,
        MAIN_MENU_NEW, MAIN_MENU_LOAD, MAIN_MENU_SAVE,
        MAIN_MENU_BRUSH_SIZE, MAIN_MENU_BRUSH_SPEED, MAIN_MENU_COLOR,
        MAIN_MENU_GRID_SIZE,
        MAIN_MENU_EXIT,
        /* Select action menu */
        SELECT_MENU_CUT, SELECT_MENU_COPY, SELECT_MENU_INVERT,
        SELECT_MENU_HFLIP, SELECT_MENU_VFLIP, SELECT_MENU_ROTATE90,
        SELECT_MENU_ROTATE180, SELECT_MENU_ROTATE270,
        SELECT_MENU_CANCEL,
        /* Text menu */
        TEXT_MENU_TEXT, TEXT_MENU_FONT,
        TEXT_MENU_PREVIEW, TEXT_MENU_APPLY, TEXT_MENU_CANCEL,
     };

static struct menu_items main_menu[]=
    { { MENU_TITLE, "RockPaint" },
      { MAIN_MENU_RESUME, "Resume" },
      { MAIN_MENU_NEW, "New" },
      { MAIN_MENU_LOAD, "Load" },
      { MAIN_MENU_SAVE, "Save" },
      { MAIN_MENU_BRUSH_SIZE, "Brush Size" },
      { MAIN_MENU_BRUSH_SPEED, "Brush Speed" },
      { MAIN_MENU_COLOR, "Choose Color" },
      { MAIN_MENU_GRID_SIZE, "Grid Size" },
      { MAIN_MENU_EXIT, "Exit" },
      { MENU_END, "" } };

static struct menu_items size_menu[] =
    { { MENU_TITLE, "Choose Size" },
      {  1, "1x" },
      {  2, "2x" },
      {  4, "4x" },
      {  8, "8x" },
      { MENU_END, "" } };

static struct menu_items speed_menu[] =
    { { MENU_TITLE, "Choose Speed" },
      {  1, "1x" },
      {  2, "2x" },
      {  4, "4x" },
      { MENU_END, "" } };

static struct menu_items gridsize_menu[] =
    { { MENU_TITLE, "Grid Size" },
      { 0, "No grid" },
      { 5, "5px" },
      { 10, "10px" },
      { 20, "20px" },
      { MENU_END, "" } };

static struct menu_items select_menu[] =
    { { MENU_TITLE, "Select..." },
      { SELECT_MENU_CUT, "Cut" },
      { SELECT_MENU_COPY, "Copy" },
      { SELECT_MENU_INVERT, "Invert" },
      { SELECT_MENU_HFLIP, "Horizontal flip" },
      { SELECT_MENU_VFLIP, "Vertical flip" },
//      { SELECT_MENU_ROTATE90, "Rotate 90°" },
      { SELECT_MENU_ROTATE180, "Rotate 180°" },
//      { SELECT_MENU_ROTATE270, "Rotate 270°" },
      { SELECT_MENU_CANCEL, "Cancel" },
      { MENU_END, "" } };

static struct menu_items text_menu[] =
    { { MENU_TITLE, "Text" },
      { TEXT_MENU_TEXT, "Set text" },
      { TEXT_MENU_FONT, "Change font" },
      { TEXT_MENU_PREVIEW, "Preview" },
      { TEXT_MENU_APPLY, "Apply" },
      { TEXT_MENU_CANCEL, "Cancel" },
      { MENU_END, "" } };

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

static int menu_display( struct menu_items menu[], int prev_value )
{
    int i,
        fh, /* font height */
        width, height,
        a, b,
        selection=1,
        menu_length,
        top, left;
    int scroll = 0, onscreen = 0;

    rb->lcd_getstringsize( menu[0].label, &width, &fh );
    for( i=1; menu[i].value != MENU_END; i++ )
    {
        if( prev_value == menu[i].value )
        {
            selection = i;
        }
        rb->lcd_getstringsize( menu[i].label, &a, &b );
        if( a > width ) width = a;
        if( b > fh ) fh = b;
    }
    menu_length = i;
    fh++;
    width += 10;

    height = menu_length * fh + 4 + 2 + 2;
    if( height >= LCD_HEIGHT )
    {
        scroll = 1;
        onscreen = ( LCD_HEIGHT - 4 - 2 - 2 )/fh;
        height = onscreen * fh + 4 + 2 + 2;
        width += 5;
    }
    else
    {
        onscreen = menu_length;
    }

    draw_window( height, width, &top, &left, menu[0].label );

    while( 1 )
    {
        for( i = (scroll == 0 ? 1 : scroll);
             i < (scroll ? scroll + onscreen - 1:onscreen);
             i++ )
        {
            if( i == selection )
            {
                rb->lcd_set_foreground( COLOR_WHITE );
                rb->lcd_set_background( COLOR_BLUE );
            }
            else
            {
                rb->lcd_set_foreground( COLOR_BLACK );
                rb->lcd_set_background( COLOR_LIGHTGRAY );
            }
            rb->lcd_putsxy( left+2,
                            top+6+fh*(i-(scroll == 0 ? 0 : scroll-1 )),
                            menu[i].label );
        }
        if( scroll )
        {
            int scroll_height = ((height-fh-4-2)*onscreen)/menu_length;
            rb->lcd_set_foreground( COLOR_BLACK );
            rb->lcd_vline( left+width-5, top+fh+4, top+height-2 );
            rb->lcd_fillrect( left+width-4,
  top+fh+4+((height-4-2-fh-scroll_height)*(scroll-1))/(menu_length-onscreen),
                              3, scroll_height );
        }
        rb->lcd_update();

        switch( rb->button_get(true) )
        {
            case ROCKPAINT_UP:
            case ROCKPAINT_UP|BUTTON_REPEAT:
                selection = (selection + menu_length-1)%menu_length;
                if( !selection ) selection = menu_length-1;
                break;

            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN|BUTTON_REPEAT:
                selection = (selection + 1)%menu_length;
                if( !selection ) selection++;
                break;

            case ROCKPAINT_LEFT:
                restore_screen();
                return MENU_ESC;

            case ROCKPAINT_RIGHT:
            case ROCKPAINT_DRAW:
                restore_screen();
                return menu[selection].value;
        }
        if( scroll )
        {
            if( selection < scroll )
            {
                scroll = selection;
                draw_window( height, width, NULL, NULL, menu[0].label );
            }
            if( selection >= scroll + onscreen - 1 )
            {
                scroll++;
                draw_window( height, width, NULL, NULL, menu[0].label );
            }
        }
    }
}

/***********************************************************************
 * File browser
 ***********************************************************************/

char bbuf[MAX_PATH+1]; /* used by file and font browsers */
char bbuf_s[MAX_PATH+1]; /* used by file and font browsers */

static bool browse( char *dst, int dst_size, const char *start )
{
#define WIDTH ( LCD_WIDTH - 20 )
#define HEIGHT ( LCD_HEIGHT - 20 )
#define LINE_SPACE 2
    int top, top_inside, left;

    DIR *d;
    struct dirent *de;
    int fvi = 0; /* first visible item */
    int lvi = 0; /* last visible item */
    int si = 0; /* selected item */
    int li = 0; /* last item */
    int i;

    int fh;
    char *a;

    rb->lcd_getstringsize( "Ap", NULL, &fh );

    rb->strcpy( bbuf, start );
    a = bbuf+rb->strlen(bbuf)-1;
    if( *a != '/' )
    {
        a[1] = '/';
        a[2] = '\0';
    }

    while( 1 )
    {
        d = rb->opendir( bbuf );
        if( !d )
        {
            /*
            if( errno == ENOTDIR )
            {*/
                /* this is a file */
                bbuf[rb->strlen(bbuf)-1] = '\0';
                rb->strncpy( dst, bbuf, dst_size );
                return true;
            /*}
            else if( errno == EACCES || errno == ENOENT )
            {
                bbuf[0] = '/'; bbuf[1] = '\0';
                d = rb->opendir( "/" );
            }
            else
            {
                return false;
            }*/
        }
        top_inside = draw_window( HEIGHT, WIDTH, &top, &left, bbuf );
        i = 0;
        li = -1;
        while( i < fvi )
        {
            rb->readdir( d );
            i++;
        }
        while( top_inside+(i-fvi)*(fh+LINE_SPACE) < HEIGHT )
        {
            de = rb->readdir( d );
            if( !de )
            {
                li = i-1;
                break;
            }
            rb->lcd_set_foreground((si==i?COLOR_WHITE:COLOR_BLACK));
            rb->lcd_set_background((si==i?COLOR_BLUE:COLOR_LIGHTGRAY));
            rb->lcd_putsxy( left+10,
                            top_inside+(i-fvi)*(fh+LINE_SPACE),
                            de->d_name );
            if( si == i )
                rb->strcpy( bbuf_s, de->d_name );
            i++;
        }
        lvi = i-1;
        if( li == -1 )
        {
            if( !rb->readdir( d ) )
            {
                li = lvi;
            }
        }
        rb->closedir( d );

        rb->lcd_update();

        switch( rb->button_get(true) )
        {
            case ROCKPAINT_UP:
            case ROCKPAINT_UP|BUTTON_REPEAT:
                if( si > 0 )
                {
                    si--;
                    if( si<fvi )
                    {
                        fvi--;
                    }
                }
                break;

            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN|BUTTON_REPEAT:
                if( li == -1 || si < li )
                {
                    si++;
                    if( si>lvi )
                    {
                        fvi++;
                    }
                }
                break;

            case ROCKPAINT_LEFT:
                if( bbuf[0] == '/' && !bbuf[1] ) return false;
                bbuf_s[0] = '.';
                bbuf_s[1] = '.';
                bbuf_s[2] = '\0';
            case ROCKPAINT_RIGHT:
            case ROCKPAINT_DRAW:
                if( *bbuf_s == '.' && !bbuf_s[1] ) break;
                a = bbuf;
                while( *a ) a++;
                if( *bbuf_s == '.' && bbuf_s[1] == '.' && !bbuf_s[2] )
                {
                    a--;
                    if( a == bbuf ) break;
                    if( *a == '/' ) a--;
                    while( *a != '/' ) a--;
                    *++a = '\0';
                    break;
                }
                rb->strcpy( a, bbuf_s );
                while( *a ) a++;
                *a++ = '/';
                *a = '\0';
                fvi = si = 0;
                break;
        }
    }

#undef WIDTH
#undef HEIGHT
#undef LINE_SPACE
}

/***********************************************************************
 * Font browser
 *
 * FIXME: This still needs some work ... it currently only works fine
 * on the simulators, disk spins too much on real targets -> rendered
 * font buffer needed.
 ***********************************************************************/
static bool browse_fonts( char *dst, int dst_size )
{
    char old_font[MAX_PATH];
#define WIDTH ( LCD_WIDTH - 20 )
#define HEIGHT ( LCD_HEIGHT - 20 )
#define LINE_SPACE 2
    int top, top_inside = 0, left;

    DIR *d;
    struct dirent *de;
    int fvi = 0; /* first visible item */
    int lvi = 0; /* last visible item */
    int si = 0; /* selected item */
    int osi = 0; /* old selected item */
    int li = 0; /* last item */
    int nvih = 0; /* next visible item height */
    int i;
    int b_need_redraw = 1; /* Do we need to redraw ? */

    int cp = 0; /* current position */
    int fh; /* font height */

    #define fh_buf buffer.text.fh_buf /* 30 might not be enough ... */
    #define fw_buf buffer.text.fw_buf
    int fw;
    #define fontname_buf buffer.text.fontname_buf

    rb->snprintf( old_font, MAX_PATH,
                  FONT_DIR "/%s.fnt",
                  rb->global_settings->font_file );

    while( 1 )
    {
        if( !b_need_redraw )
        {
            /* we don't need to redraw ... but we need to unselect
             * the previously selected item */
            cp = top_inside + LINE_SPACE;
            for( i = 0; i+fvi < osi; i++ )
            {
                cp += fh_buf[i] + LINE_SPACE;
            }
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect( left+10, cp, fw_buf[i], fh_buf[i] );
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }

        if( b_need_redraw )
        {
            b_need_redraw = 0;

            d = rb->opendir( FONT_DIR "/" );
            if( !d )
            {
                return false;
            }
            top_inside = draw_window( HEIGHT, WIDTH, &top, &left, "Fonts" );
            i = 0;
            li = -1;
            while( i < fvi )
            {
                rb->readdir( d );
                i++;
            }
            cp = top_inside+LINE_SPACE;

            rb->lcd_set_foreground(COLOR_BLACK);
            rb->lcd_set_background(COLOR_LIGHTGRAY);

            while( cp < top+HEIGHT )
            {
                de = rb->readdir( d );
                if( !de )
                {
                    li = i-1;
                    break;
                }
                if( rb->strlen( de->d_name ) < 4
                    || rb->strcmp( de->d_name + rb->strlen( de->d_name ) - 4,
                                   ".fnt" ) )
                    continue;
                rb->snprintf( bbuf, MAX_PATH, FONT_DIR "/%s",
                              de->d_name );
                rb->font_load( bbuf );
                rb->font_getstringsize( de->d_name, &fw, &fh, FONT_UI );
                if( nvih > 0 )
                {
                    nvih -= fh;
                    fvi++;
                    if( nvih < 0 ) nvih = 0;
                    i++;
                    continue;
                }
                if( cp + fh >= top+HEIGHT )
                {
                    nvih = fh;
                    break;
                }
                rb->lcd_putsxy( left+10, cp, de->d_name );
                fh_buf[i-fvi] = fh;
                fw_buf[i-fvi] = fw;
                cp += fh + LINE_SPACE;
                rb->strcpy( fontname_buf[i-fvi], bbuf );
                i++;
            }
            lvi = i-1;
            if( li == -1 )
            {
                if( !(de = rb->readdir( d ) ) )
                {
                    li = lvi;
                }
                else if( !nvih && !rb->strlen( de->d_name ) < 4
                    && !rb->strcmp( de->d_name + rb->strlen( de->d_name ) - 4,
                                   ".fnt" ) )
                {
                    rb->snprintf( bbuf, MAX_PATH, FONT_DIR "/%s",
                          de->d_name );
                    rb->font_load( bbuf );
                    rb->font_getstringsize( de->d_name, NULL, &fh, FONT_UI );
                    nvih = fh;
                }
            }
            rb->font_load( old_font );
            rb->closedir( d );
        }

        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        cp = top_inside + LINE_SPACE;
        for( i = 0; i+fvi < si; i++ )
        {
            cp += fh_buf[i] + LINE_SPACE;
        }
        rb->lcd_fillrect( left+10, cp, fw_buf[i], fh_buf[i] );
        rb->lcd_set_drawmode(DRMODE_SOLID);

        rb->lcd_update_rect( left, top, WIDTH, HEIGHT );

        osi = si;
        i = fvi;
        switch( rb->button_get(true) )
        {
            case ROCKPAINT_UP:
            case ROCKPAINT_UP|BUTTON_REPEAT:
                if( si > 0 )
                {
                    si--;
                    if( si<fvi )
                    {
                        fvi = si;
                    }
                }
                break;

            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN|BUTTON_REPEAT:
                if( li == -1 || si < li )
                {
                    si++;
                }
                break;

            case ROCKPAINT_LEFT:
                return false;

            case ROCKPAINT_RIGHT:
            case ROCKPAINT_DRAW:
                rb->snprintf( dst, dst_size, "%s", fontname_buf[si-fvi] );
                return true;
        }

        if( i != fvi || si > lvi )
        {
            b_need_redraw = 1;
        }

        if( si<=lvi )
        {
            nvih = 0;
        }
    }
#undef fh_buf
#undef fw_buf
#undef fontname_buf
#undef WIDTH
#undef HEIGHT
#undef LINE_SPACE
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

    enum BaseColor { Hue = 0, Saturation = 1, Value = 2,
                     Red = 3, Green = 4, Blue = 5 };
    enum BaseColor current = Red;
    bool has_changed;

    char str[6] = "";

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
        rb->snprintf( str, 6, "%d", hue/10 );
        rb->lcd_putsxy( left + 117, top + 20, str );
        rb->snprintf( str, 6, "%d.%d", saturation/255, ((saturation*100)/255)%100 );
        rb->lcd_putsxy( left + 117, top + 30, str );
        rb->snprintf( str, 6, "%d.%d", value/255, ((value*100)/255)%100 );
        rb->lcd_putsxy( left + 117, top + 40, str );
        rb->snprintf( str, 6, "%d", red );
        rb->lcd_putsxy( left + 117, top + 50, str );
        rb->snprintf( str, 6, "%d", green );
        rb->lcd_putsxy( left + 117, top + 60, str );
        rb->snprintf( str, 6, "%d", blue );
        rb->lcd_putsxy( left + 117, top + 70, str );
        rb->lcd_setfont( FONT_UI );

#define CURSOR( l ) \
        rb->lcd_bitmap_transparent_part( rockpaint_hsvrgb, 1, 1, 16, left+l+1, top+20, 6, 58 ); \
        rb->lcd_bitmap_transparent_part( rockpaint_hsvrgb, 8, 10*current, 16, left+l, top+19+10*current, 8, 10 );
        CURSOR( 5 );
#undef CURSOR

        rb->lcd_set_foreground( color );
        rb->lcd_fillrect( left+15, top+85, 100, 8 );

        rb->lcd_update();

        switch( rb->button_get(true) )
        {
            case ROCKPAINT_UP:
                current = ( current + 5 )%6;
                break;

            case ROCKPAINT_DOWN:
                current = (current + 1 )%6;
                break;

            case ROCKPAINT_LEFT:
                has_changed = true;
                switch( current )
                {
                    case Hue:
                        hue = ( hue + 3600 - 10 )%3600;
                        break;
                    case Saturation:
                        if( saturation ) saturation--;
                        break;
                    case Value:
                        if( value ) value--;
                        break;
                    case Red:
                        if( red ) red--;
                        break;
                    case Green:
                        if( green ) green--;
                        break;
                    case Blue:
                        if( blue ) blue--;
                        break;
                }
                break;

            case ROCKPAINT_LEFT|BUTTON_REPEAT:
                has_changed = true;
                switch( current )
                {
                    case Hue:
                        hue = ( hue + 3600 - 100 )%3600;
                        break;
                    case Saturation:
                        if( saturation >= 8 ) saturation-=8;
                        else saturation = 0;
                        break;
                    case Value:
                        if( value >= 8 ) value-=8;
                        else value = 0;
                        break;
                    case Red:
                        if( red >= 8 ) red-=8;
                        else red = 0;
                        break;
                    case Green:
                        if( green >= 8 ) green-=8;
                        else green = 0;
                        break;
                    case Blue:
                        if( blue >= 8 ) blue-=8;
                        else blue = 0;
                        break;
                }
                break;

            case ROCKPAINT_RIGHT:
                has_changed = true;
                switch( current )
                {
                    case Hue:
                        hue = ( hue + 10 )%3600;
                        break;
                    case Saturation:
                        if( saturation < 0xff ) saturation++;
                        break;
                    case Value:
                        if( value < 0xff ) value++;
                        break;
                    case Red:
                        if( red < 0xff ) red++;
                        break;
                    case Green:
                        if( green < 0xff ) green++;
                        break;
                    case Blue:
                        if( blue < 0xff ) blue++;
                        break;
                }
                break;

            case ROCKPAINT_RIGHT|BUTTON_REPEAT:
                has_changed = true;
                switch( current )
                {
                    case Hue:
                        hue = ( hue + 100 )%3600;
                        break;
                    case Saturation:
                        if( saturation < 0xff - 8 ) saturation+=8;
                        else saturation = 0xff;
                        break;
                    case Value:
                        if( value < 0xff - 8 ) value+=8;
                        else value = 0xff;
                        break;
                    case Red:
                        if( red < 0xff - 8 ) red+=8;
                        else red = 0xff;
                        break;
                    case Green:
                        if( green < 0xff - 8 ) green+=8;
                        else green = 0xff;
                        break;
                    case Blue:
                        if( blue < 0xff - 8 ) blue+=8;
                        else blue = 0xff;
                        break;
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
        rb->lcd_drawrect( x<COLS-PSIZE ? x + 2 : x - PSIZE, y<ROWS-PSIZE ? y + 2: y - PSIZE, PSIZE - 2, PSIZE - 2 );
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_fillrect( x<COLS-PSIZE ? x+3 : x - PSIZE+1, y<ROWS-PSIZE ? y +3: y - PSIZE+1, PSIZE-4, PSIZE-4 );
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
    rb->memcpy( buffer.clipboard, save_buffer, COLS*ROWS*sizeof( fb_data ) );
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
                    buffer.clipboard+(y2-i)*COLS+x1,
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
            save_buffer[y1*COLS+x1+i] = buffer.clipboard[y1*COLS+x2-i];
        }
    }
    restore_screen();
    rb->lcd_update();
}

static void draw_paste_rectangle( int src_x1, int src_y1, int src_x2,
                                  int src_y2, int x1, int y1, int mode )
{
    int i;
    if( mode == SELECT_MENU_CUT )
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
    rb->lcd_bitmap_part( buffer.clipboard, src_x1, src_y1, COLS,
                         x1, y1, src_x2-src_x1+1, src_y2-src_y1+1 );
    if( !preview )
    {
        for( i = 0; i <= src_y2 - src_y1; i++ )
        {
            rb->memcpy( save_buffer+(y1+i)*COLS+x1,
                        buffer.clipboard+(src_y1+i)*COLS+src_x1,
                        (src_x2 - src_x1 + 1)*sizeof( fb_data ) );
        }
    }
}

static void show_grid( bool update )
{
    int i;
    if( gridsize > 0 )
    {
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        for( i = gridsize; i < COLS; i+= gridsize )
        {
            rb->lcd_vline( i, 0, ROWS-1 );
        }
        for( i = gridsize; i < ROWS; i+= gridsize )
        {
            rb->lcd_hline( 0, COLS-1, i );
        }
        rb->lcd_set_drawmode(DRMODE_SOLID);
        if( update ) rb->lcd_update();
    }
}

static void draw_text( int x, int y )
{
    buffer.text.text[0] = '\0';
    rb->snprintf( buffer.text.old_font, MAX_PATH,
                  FONT_DIR "/%s.fnt",
                  rb->global_settings->font_file );
    while( 1 )
    {
        int m = TEXT_MENU_TEXT;
        switch( m = menu_display( text_menu, m ) )
        {
            case TEXT_MENU_TEXT:
                rb->kbd_input( buffer.text.text, MAX_TEXT );
                restore_screen();
                break;

            case TEXT_MENU_FONT:
                if( browse_fonts( buffer.text.font, MAX_PATH ) )
                {
                    rb->font_load( buffer.text.font );
                }
                break;

            case TEXT_MENU_PREVIEW:
                rb->lcd_set_foreground( rp_colors[ drawcolor ] );
                while( 1 )
                {
                    unsigned int button;
                    restore_screen();
                    rb->lcd_putsxy( x, y, buffer.text.text );
                    rb->lcd_update();
                    switch( button = rb->button_get( true ) )
                    {
                        case ROCKPAINT_LEFT:
                        case ROCKPAINT_LEFT | BUTTON_REPEAT:
                            x-=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                            if (x<0) x=COLS-1;
                            break;

                        case ROCKPAINT_RIGHT:
                        case ROCKPAINT_RIGHT | BUTTON_REPEAT:
                            x+=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                            if (x>=COLS) x=0;
                            break;

                        case ROCKPAINT_UP:
                        case ROCKPAINT_UP | BUTTON_REPEAT:
                            y-=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                            if (y<0) y=ROWS-1;
                            break;

                        case ROCKPAINT_DOWN:
                        case ROCKPAINT_DOWN | BUTTON_REPEAT:
                            y+=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                            if (y>=ROWS-1) y=0;
                            break;

                        case ROCKPAINT_DRAW:
                            button = 1242;
                            break;
                    }
                    if( button == 1242 ) break;
                }
                break;

            case TEXT_MENU_APPLY:
                rb->lcd_set_foreground( rp_colors[ drawcolor ] );
                buffer_putsxyofs( save_buffer, COLS, ROWS, x, y, 0,
                                  buffer.text.text );
            case TEXT_MENU_CANCEL:
                restore_screen();
                rb->font_load( buffer.text.old_font );
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
        for (i = abs(deltay) + 1; i; --i)
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
        
        for (i = abs(deltax) + 1; i; --i)
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
        buffer.bezier[i].x1 = a1; \
        buffer.bezier[i].y1 = b1; \
        buffer.bezier[i].x2 = a2; \
        buffer.bezier[i].y2 = b2; \
        buffer.bezier[i].x3 = a3; \
        buffer.bezier[i].y3 = b3; \
        buffer.bezier[i].depth = d; \
        i++;
#define POP( a1, b1, a2, b2, a3, b3, d ) \
        i--; \
        a1 = buffer.bezier[i].x1; \
        b1 = buffer.bezier[i].y1; \
        a2 = buffer.bezier[i].x2; \
        b2 = buffer.bezier[i].y2; \
        a3 = buffer.bezier[i].x3; \
        b3 = buffer.bezier[i].y3; \
        d = buffer.bezier[i].depth;
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
        buffer.bezier[i].x1 = a1; \
        buffer.bezier[i].y1 = b1; \
        buffer.bezier[i].x2 = a2; \
        buffer.bezier[i].y2 = b2; \
        buffer.bezier[i].x3 = a3; \
        buffer.bezier[i].y3 = b3; \
        buffer.bezier[i].x4 = a4; \
        buffer.bezier[i].y4 = b4; \
        buffer.bezier[i].depth = d; \
        i++;
#define POP( a1, b1, a2, b2, a3, b3, a4, b4, d ) \
        i--; \
        a1 = buffer.bezier[i].x1; \
        b1 = buffer.bezier[i].y1; \
        a2 = buffer.bezier[i].x2; \
        b2 = buffer.bezier[i].y2; \
        a3 = buffer.bezier[i].x3; \
        b3 = buffer.bezier[i].y3; \
        a4 = buffer.bezier[i].x4; \
        b4 = buffer.bezier[i].y4; \
        d = buffer.bezier[i].depth;

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
    int x = x1;
    togglebg();
    if( x < x2 )
    {
        do {
            draw_line( x, y1, x, y2 );
        } while( ++x <= x2 );
    }
    else
    {
        do {
            draw_line( x, y1, x, y2 );
        } while( --x >= x2 );
    }
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
    buffer.coord[i].x = a; \
    buffer.coord[i].y = b; \
    i++;
#define POP( a, b ) \
    i--; \
    a = buffer.coord[i].x; \
    b = buffer.coord[i].y;

    unsigned int i=0;
    short x = x0;
    short y = y0;
    unsigned int prev_color = save_buffer[ x0+y0*COLS ];

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
static void line_gradient( int x1, int y1, int x2, int y2 )
{
    int r1, g1, b1;
    int r2, g2, b2;
    int h1, s1, v1, h2, s2, v2, r, g, b;
    int w, h, x, y;

    bool a = false;

    x1 <<= 1;
    y1 <<= 1;
    x2 <<= 1;
    y2 <<= 1;

    w = x1 - x2;
    h = y1 - y2;

    if( w == 0 && h == 0 )
    {
        draw_pixel( x1>>1, y1>>1 );
        return;
    }

    r1 = RGB_UNPACK_RED( rp_colors[ bgdrawcolor ] );
    g1 = RGB_UNPACK_GREEN( rp_colors[ bgdrawcolor ] );
    b1 = RGB_UNPACK_BLUE( rp_colors[ bgdrawcolor ] );
    r2 = RGB_UNPACK_RED( rp_colors[ drawcolor ] );
    g2 = RGB_UNPACK_GREEN( rp_colors[ drawcolor ] );
    b2 = RGB_UNPACK_BLUE( rp_colors[ drawcolor ] );

    if( w < 0 )
    {
        w *= -1;
        a = true;
    }
    if( h < 0 )
    {
        h *= -1;
        a = !a;
    }
    if( a )
    {
        r = r1;
        r1 = r2;
        r2 = r;
        g = g1;
        g1 = g2;
        g2 = g;
        b = b1;
        b1 = b2;
        b2 = b;
    }

    rgb2hsv( r1, g1, b1, &h1, &s1, &v1 );
    rgb2hsv( r2, g2, b2, &h2, &s2, &v2 );

    if( w > h )
    {
        if( x1 > x2 )
        {
            x = x2;
            y = y2;
            x2 = x1;
            y2 = y1;
            x1 = x;
            y1 = y;
        }
        w = x1 - x2;
        h = y1 - y2;
        while( x1 <= x2 )
        {
            hsv2rgb( h1+((h2-h1)*(x1-x2))/w,
                     s1+((s2-s1)*(x1-x2))/w,
                     v1+((v2-v1)*(x1-x2))/w,
                     &r, &g, &b );
            rp_colors[ drawcolor ] = LCD_RGBPACK( r, g, b );
            rb->lcd_set_foreground( rp_colors[ drawcolor ] );
            draw_pixel( (x1+1)>>1, (y1+1)>>1 );
            x1+=2;
            y1 = y2 - ( x2 - x1 ) * h / w;
        }
    }
    else /* h > w */
    {
        if( y1 > y2 )
        {
            x = x2;
            y = y2;
            x2 = x1;
            y2 = y1;
            x1 = x;
            y1 = y;
        }
        w = x1 - x2;
        h = y1 - y2;
        while( y1 <= y2 )
        {
            hsv2rgb( h1+((h2-h1)*(y1-y2))/h,
                     s1+((s2-s1)*(y1-y2))/h,
                     v1+((v2-v1)*(y1-y2))/h,
                     &r, &g, &b );
            rp_colors[ drawcolor ] = LCD_RGBPACK( r, g, b );
            rb->lcd_set_foreground( rp_colors[ drawcolor ] );
            draw_pixel( (x1+1)>>1, (y1+1)>>1 );
            y1+=2;
            x1 = x2 - ( y2 - y1 ) * w / h;
        }
    }
    if( a )
    {
        rp_colors[ drawcolor ] = LCD_RGBPACK( r1, g1, b1 );
    }
    else
    {
        rp_colors[ drawcolor ] = LCD_RGBPACK( r2, g2, b2 );
    }
}

static void linear_gradient( int x1, int y1, int x2, int y2 )
{
    int r1 = RGB_UNPACK_RED( rp_colors[ bgdrawcolor ] );
    int g1 = RGB_UNPACK_GREEN( rp_colors[ bgdrawcolor ] );
    int b1 = RGB_UNPACK_BLUE( rp_colors[ bgdrawcolor ] );
    int r2 = RGB_UNPACK_RED( rp_colors[ drawcolor ] );
    int g2 = RGB_UNPACK_GREEN( rp_colors[ drawcolor ] );
    int b2 = RGB_UNPACK_BLUE( rp_colors[ drawcolor ] );

    int h1, s1, v1, h2, s2, v2, r, g, b;

    /* radius^2 */
    int radius2 = ( x1 - x2 ) * ( x1 - x2 ) + ( y1 - y2 ) * ( y1 - y2 );
    int dist2, i=0;

    /* We only propagate the gradient to neighboring pixels with the same
     * color as ( x1, y1 ) */
    unsigned int prev_color = save_buffer[ x1+y1*COLS ];

    int x = x1;
    int y = y1;

    if( radius2 == 0 ) return;
    if( preview )
    {
        line_gradient( x1, y1, x2, y2 );
    }

    rgb2hsv( r1, g1, b1, &h1, &s1, &v1 );
    rgb2hsv( r2, g2, b2, &h2, &s2, &v2 );

#define PUSH( x0, y0 ) \
    buffer.coord[i].x = (short)(x0);                                    \
    buffer.coord[i].y = (short)(y0);                                    \
    i++;
#define POP( a, b ) \
    i--; \
    a = (int)buffer.coord[i].x; \
    b = (int)buffer.coord[i].y;

    PUSH( x, y );

    while( i != 0 )
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
            rp_colors[ drawcolor ] = LCD_RGBPACK( r2, g2, b2 );
        }
        if( rp_colors[ drawcolor ] == prev_color )
        {
            if( rp_colors[ drawcolor ])
                rp_colors[ drawcolor ]--; /* GRUIK */
            else
                rp_colors[ drawcolor ]++; /* GRUIK */
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
#undef PUSH
#undef POP

    rp_colors[ drawcolor ] = LCD_RGBPACK( r2, g2, b2 );
}

static void radial_gradient( int x1, int y1, int x2, int y2 )
{
    int r1 = RGB_UNPACK_RED( rp_colors[ bgdrawcolor ] );
    int g1 = RGB_UNPACK_GREEN( rp_colors[ bgdrawcolor ] );
    int b1 = RGB_UNPACK_BLUE( rp_colors[ bgdrawcolor ] );
    int r2 = RGB_UNPACK_RED( rp_colors[ drawcolor ] );
    int g2 = RGB_UNPACK_GREEN( rp_colors[ drawcolor ] );
    int b2 = RGB_UNPACK_BLUE( rp_colors[ drawcolor ] );

    int h1, s1, v1, h2, s2, v2, r, g, b;

    /* radius^2 */
    int radius2 = ( x1 - x2 ) * ( x1 - x2 ) + ( y1 - y2 ) * ( y1 - y2 );
    int dist2, i=0;

    /* We only propagate the gradient to neighboring pixels with the same
     * color as ( x1, y1 ) */
    unsigned int prev_color = save_buffer[ x1+y1*COLS ];

    int x = x1;
    int y = y1;

    if( radius2 == 0 ) return;
    if( preview )
    {
        line_gradient( x1, y1, x2, y2 );
    }

    rgb2hsv( r1, g1, b1, &h1, &s1, &v1 );
    rgb2hsv( r2, g2, b2, &h2, &s2, &v2 );

#define PUSH( x0, y0 ) \
    buffer.coord[i].x = (short)(x0);                                    \
    buffer.coord[i].y = (short)(y0);                                    \
    i++;
#define POP( a, b ) \
    i--; \
    a = (int)buffer.coord[i].x; \
    b = (int)buffer.coord[i].y;

    PUSH( x, y );

    while( i != 0 )
    {
        POP( x, y );

        if( ( dist2 = (x1-(x))*(x1-(x))+(y1-(y))*(y1-(y)) ) < radius2 )
        {
            hsv2rgb( h1+((h2-h1)*dist2)/radius2,
                     s1+((s2-s1)*dist2)/radius2,
                     v1+((v2-v1)*dist2)/radius2,
                     &r, &g, &b );
            rp_colors[ drawcolor ] = LCD_RGBPACK( r, g, b );
        }
        else
        {
            rp_colors[ drawcolor ] = LCD_RGBPACK( r2, g2, b2 );
        }
        if( rp_colors[ drawcolor ] == prev_color )
        {
            if( rp_colors[ drawcolor ])
                rp_colors[ drawcolor ]--; /* GRUIK */
            else
                rp_colors[ drawcolor ]++; /* GRUIK */
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
#undef PUSH
#undef POP

    rp_colors[ drawcolor ] = LCD_RGBPACK( r2, g2, b2 );
}

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
                    prev_x = -1;
                    prev_y = -1;
                    prev_x2 = -1;
                    prev_y2 = -1;
                    prev_x3 = -1;
                    prev_y3 = -1;
                    preview = false;
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
                inv_cursor(false);
                x-=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                if (x<0) x=COLS-1;
                inv_cursor(true);
                break;

            case ROCKPAINT_RIGHT:
            case ROCKPAINT_RIGHT | BUTTON_REPEAT:
                inv_cursor(false);
                x+=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                if (x>=COLS) x=0;
                inv_cursor(true);
                break;

            case ROCKPAINT_UP:
            case ROCKPAINT_UP | BUTTON_REPEAT:
                inv_cursor(false);
                y-=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                if (y<LCD_HEIGHT-TB_HEIGHT)
                {
                    return;
                }
                inv_cursor(true);
                break;

            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN | BUTTON_REPEAT:
                inv_cursor(false);
                y+=bspeed * ( button & BUTTON_REPEAT ? 4 : 1 );
                if (y>=LCD_HEIGHT)
                {
                    y = 0;
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
    rb->lcd_hline(x-4,x-1,y);
    rb->lcd_hline(x+1,x+4,y);
    rb->lcd_vline(x,y-4,y-1);
    rb->lcd_vline(x,y+1,y+4);
    rb->lcd_set_foreground(rp_colors[drawcolor]);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    if( update ) rb->lcd_update();
}

static void restore_screen(void)
{
    rb->lcd_bitmap( save_buffer, 0, 0, COLS, ROWS );
}

static void clear_drawing(void)
{
    init_buffer();
    rb->lcd_set_foreground( rp_colors[ bgdrawcolor ] );
    rb->lcd_fillrect( 0, 0, COLS, ROWS );
    rb->lcd_update();
}

static void goto_menu(void)
{
    int multi;

    while( 1 )
    {
        switch( menu_display( main_menu, 1 ) )
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
                if (!filename[0])
                    rb->strcpy(filename,"/");
                if( !rb->kbd_input( filename, MAX_PATH ) )
                {
                    if(rb->strlen(filename) <= 4 ||
                    rb->strcasecmp(&filename[rb->strlen(filename)-4], ".bmp"))
                        rb->strcat(filename, ".bmp");
                    save_bitmap( filename );
                    rb->splashf( 1*HZ, "File saved (%s)", filename );
                }
                break;

            case MAIN_MENU_BRUSH_SIZE:
                multi = menu_display( size_menu, bsize );
                if( multi != - 1 )
                    bsize = multi;
                break;

            case MAIN_MENU_BRUSH_SPEED:
                multi = menu_display( speed_menu, bspeed );
                if( multi != -1 )
                    bspeed = multi;
                break;

            case MAIN_MENU_COLOR:
                rp_colors[drawcolor] = color_chooser( rp_colors[drawcolor] );
                break;

            case MAIN_MENU_GRID_SIZE:
                multi = menu_display( gridsize_menu, gridsize );
                if( multi != - 1 )
                    gridsize = multi;
                break;

            case MAIN_MENU_EXIT:
                quit=true;
                return;

            case MAIN_MENU_RESUME:
            case MENU_ESC:
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
    tool_mode = -1;
    preview = false;
}

static bool rockpaint_loop( void )
{
    int button=0,i,j;
    int accelaration;

    while (!quit) {
        button = rb->button_get(true);

        if( tool == Brush && prev_x != -1 )
        {
            accelaration = 1;
        }
        else if( button & BUTTON_REPEAT )
        {
            accelaration = 4;
        }
        else
        {
            accelaration = 1;
        }

        switch(button)
        {
            case ROCKPAINT_QUIT:
                rb->lcd_set_drawmode(DRMODE_SOLID);
                return PLUGIN_OK;

            case ROCKPAINT_MENU:
                inv_cursor(false);
                goto_menu();
                inv_cursor(true);
                break;

            case ROCKPAINT_DRAW:
                inv_cursor(false);
                switch( tool )
                {
                    case Brush:
                        if( prev_x == -1 ) prev_x = 1;
                        else prev_x = -1;
                        break;

                    case SelectRectangle:
                    case Line:
                    case Curve:
                    case Rectangle:
                    case RectangleFull:
                    case Oval:
                    case OvalFull:
                    case LinearGradient:
                    case RadialGradient:
                        /* Curve uses 4 points, others use 2 */
                        if( prev_x == -1 || prev_y == -1 )
                        {
                            prev_x = x;
                            prev_y = y;
                            preview = true;
                        }
                        else if( tool == Curve
                                 && ( prev_x2 == -1 || prev_y2 == -1 ) )
                        {
                            prev_x2 = x;
                            prev_y2 = y;
                        }
                        else if( tool == SelectRectangle
                                 && ( prev_x2 == -1 || prev_y2 == -1 ) )
                        {
                            tool_mode = menu_display( select_menu,
                                                      SELECT_MENU_CUT );
                            switch( tool_mode )
                            {
                                case SELECT_MENU_CUT:
                                case SELECT_MENU_COPY:
                                    prev_x2 = x;
                                    prev_y2 = y;
                                    copy_to_clipboard();
                                    if( prev_x < x ) x = prev_x;
                                    if( prev_y < y ) y = prev_y;
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
                                    break;
                                case SELECT_MENU_ROTATE180:
                                    draw_hflip( prev_x, prev_y, x, y );
                                    draw_vflip( prev_x, prev_y, x, y );
                                    reset_tool();
                                    break;
                                case SELECT_MENU_ROTATE270:
                                    break;

                                case SELECT_MENU_CANCEL:
                                    reset_tool();
                                    break;

                                case MENU_ESC:
                                    break;
                            }
                        }
                        else if( tool == Curve
                                 && ( prev_x3 == -1 || prev_y3 == -1 ) )
                        {
                            prev_x3 = x;
                            prev_y3 = y;
                        }
                        else
                        {
                            preview = false;
                            switch( tool )
                            {
                                case SelectRectangle:
                                    draw_paste_rectangle( prev_x, prev_y,
                                                          prev_x2, prev_y2,
                                                          x, y, tool_mode );
                                    break;
                                case Line:
                                    draw_line( prev_x, prev_y, x, y );
                                    break;
                                case Curve:
                                    draw_curve( prev_x, prev_y,
                                               prev_x2, prev_y2,
                                               prev_x3, prev_y3,
                                               x, y );
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
                            reset_tool();
                        }
                        break;

                    case Fill:
                        draw_fill( x, y );
                        break;

                    case ColorPicker:
                        color_picker( x, y );
                        break;

                    case Text:
                        draw_text( x, y );
                        break;

                    default:
                        break;
                }
                inv_cursor(true);
                break;

            case ROCKPAINT_DRAW|BUTTON_REPEAT:
                if( tool == Curve )
                {
                    /* 3 point bezier curve */
                    preview = false;
                    draw_curve( prev_x, prev_y,
                               prev_x2, prev_y2,
                               -1, -1,
                               x, y );
                    reset_tool();
                    restore_screen();
                    inv_cursor( true );
                }
                break;

            case ROCKPAINT_TOOLBAR:
                i = x; j = y;
                x = 10;
                toolbar();
                x = i; y = j;
                restore_screen();
                inv_cursor(true);
                break;

            case ROCKPAINT_TOOLBAR2:
                i = x; j = y;
                x = 110;
                toolbar();
                x = i; y = j;
                restore_screen();
                inv_cursor(true);
                break;

            case ROCKPAINT_LEFT:
            case ROCKPAINT_LEFT | BUTTON_REPEAT:
                inv_cursor(false);
                x-=bspeed * accelaration;
                if (x<0) x=COLS-1;
                inv_cursor(true);
                break;

            case ROCKPAINT_RIGHT:
            case ROCKPAINT_RIGHT | BUTTON_REPEAT:
                inv_cursor(false);
                x+=bspeed * accelaration;
                if (x>=COLS) x=0;
                inv_cursor(true);
                break;

            case ROCKPAINT_UP:
            case ROCKPAINT_UP | BUTTON_REPEAT:
                inv_cursor(false);
                y-=bspeed * accelaration;
                if (y<0) y=ROWS-1;
                inv_cursor(true);
                break;

            case ROCKPAINT_DOWN:
            case ROCKPAINT_DOWN | BUTTON_REPEAT:
                inv_cursor(false);
                y+=bspeed * accelaration;
                if (y>=ROWS)
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
        if( tool == Brush && prev_x == 1 )
        {
            inv_cursor(false);
            draw_brush( x, y );
            inv_cursor(true);
        }
        if( preview || tool == ColorPicker )
            /* always preview color picker */
        {
            restore_screen();
            switch( tool )
            {
                case SelectRectangle:
                    if( prev_x2 == -1 || prev_y2 == -1 )
                    {
                        /* we are defining the selection */
                        draw_select_rectangle( prev_x, prev_y, x, y );
                    }
                    else
                    {
                        /* we are pasting the selected data */
                        draw_paste_rectangle( prev_x, prev_y, prev_x2,
                                              prev_y2, x, y, tool_mode );
                        prev_x3 = prev_x2-prev_x;
                        if( prev_x3 < 0 ) prev_x3 *= -1;
                        prev_y3 = prev_y2-prev_y;
                        if( prev_y3 < 0 ) prev_y3 *= -1;
                        draw_select_rectangle( x, y, x+prev_x3, y+prev_y3 );
                        prev_x3 = -1;
                        prev_y3 = -1;
                    }
                    break;

                case Brush:
                    break;

                case Line:
                    draw_line( prev_x, prev_y, x, y );
                    break;

                case Curve:
                    if( prev_x2 == -1 || prev_y2 == -1 )
                    {
                        draw_line( prev_x, prev_y, x, y );
                    }
                    else
                    {
                        draw_curve( prev_x, prev_y,
                                   prev_x2, prev_y2,
                                   prev_x3, prev_y3,
                                   x, y );
                    }
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

                case Fill:
                    break;

                case ColorPicker:
                    preview = true;
                    color_picker( x, y );
                    preview = false;
                    break;

                case LinearGradient:
                    line_gradient( prev_x, prev_y, x, y );
                    break;

                case RadialGradient:
                    line_gradient( prev_x, prev_y, x, y );
                    break;

                case Text:
                default:
                    break;
            }
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
    int l;

    bm.data = (char*)save_buffer;
    ret = rb->read_bmp_file( file, &bm, ROWS*COLS*sizeof( fb_data ),
                             FORMAT_NATIVE, NULL );

    if((bm.width > COLS ) || ( bm.height > ROWS ))
        return -1;

    for( l = bm.height-1; l > 0; l-- )
    {
        rb->memmove( save_buffer+l*COLS, save_buffer+l*bm.width,
                        sizeof( fb_data )*bm.width );
    }
    for( l = 0; l < bm.height; l++ )
    {
        rb->memset( save_buffer+l*COLS+bm.width, rp_colors[ bgdrawcolor ],
                    sizeof( fb_data )*(COLS-bm.width) );
    }
    rb->memset( save_buffer+COLS*bm.height, rp_colors[ bgdrawcolor ],
                sizeof( fb_data )*COLS*(ROWS-bm.height) );

    return ret;
}

static int save_bitmap( char *file )
{
    struct bitmap bm;
    bm.data = (char*)save_buffer;
    bm.height = ROWS;
    bm.width = COLS;
    bm.format = FORMAT_NATIVE;
    return save_bmp_file( file, &bm, rb );
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    /** must have stuff **/
    rb = api;

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
