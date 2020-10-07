/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016-2020 Franklin Wei
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

/* ================================
 * Rockbox frontend for sgt-puzzles
 * ================================
 *
 * This file contains the majority of the rockbox-specific code for
 * the sgt-puzzles port. It implements a set of functions for the
 * backend to call to actually run the games, as well as rockbox UI
 * code (menus, input, etc). For a good overview of the rest of the
 * puzzles code, see:
 *
 * <https://www.chiark.greenend.org.uk/~sgtatham/puzzles/devel/>.
 *
 * For documentation of the contents of this file, read on.
 *
 * ---------------------
 * Contents of this file
 * ---------------------
 *
 * By rough order of appearnce in this file:
 *
 *  1) "Zoom" feature
 *
 *     This file contains re-implementations of drawing primitives
 *     (lines, fills, text drawing, etc.) adapted to draw into a
 *     custom `zoom_fb' framebuffer at a magnification of ZOOM_FACTOR
 *     (compile-time constant, currently 3x). These are used if the
 *     global `zoom_enabled' switch is true.
 *
 *     The zoom feature uses a modal interface with two modes: viewing
 *     mode, and interaction mode.
 *
 *     Viewing mode is entered by default and is used to pan around the
 *     game viewport without interacting with the backend game at
 *     all. Pressing "select" while in viewing mode activates
 *     interaction mode. Instead of panning around, interaction mode
 *     sends keystrokes directly to the backend game.
 *
 *     It used to be that the zoomed viewport would remain entirely
 *     static while the user was in interaction mode. This made the
 *     zoom feature rather klunky to use because it required frequent
 *     mode switching. In commit 5094aaa, this behavior was changed so
 *     that the frontend can now query the backend for the on-screen
 *     cursor location and move the viewport accordingly through the
 *     new midend_get_cursor_location() API (which is not yet merged
 *     into Simon's tree as of October 2020).
 *
 *  2) Font management
 *
 *     Rockbox's bitmap fonts don't allow for easy rendering of text
 *     of arbitrary size. We work around this by shipping a "font
 *     pack" of pre-rendered fonts in a continuous size range spanning
 *     10px to 36px. The code here facilitates dynamic
 *     loading/unloading of fonts from this font pack.
 *
 *     Font loading efficiency is enhanced by a feature called the
 *     "font table" which remembers the set of fonts used by each
 *     individual puzzle to allow for pre-loading during subsequent
 *     runs. On targets with physical hard drives, this enhances
 *     startup time by loading the fonts while the disk is already
 *     spinning (from loading the plugin).
 *
 *  3) Drawing API
 *
 *     The sgt-puzzles backend wants a set of function pointers to the
 *     usual drawing primitives. [1] If the `zoom_enabled' switch is
 *     on, these call upon the "zoomed" drawing routines in (1).
 *
 *     In the normal un-zoomed case, these functions generally rely on
 *     the usual lcd_* or the pluginlib's xlcd_* API, with some
 *     exceptions: we implement a fixed-point antialiased line
 *     algorithm function and a hacky approximation of a polygon fill
 *     using triangles. [2]
 *
 *     Some things to note: "blitters" are used to save and restore a
 *     rectangular region of the screen; "clipping" refers to
 *     temporarily bounding draw operations to a rectangular region
 *     (this is implemented with rockbox viewports).
 *
 *  4) Input tuning and game-specific modes
 *
 *     The input schemes of most of the games in the sgt-puzzles
 *     collection are able to be played fairly well with only
 *     directional keys and a "select" button. Other games, however,
 *     need some special accommodations. These are enabled by
 *     `tune_input()' based on the game name and are listed below:
 *
 *     a) Mouse mode
 *
 *        This mode is designed to accommodate puzzles without a
 *        keyboard or cursor interface (currently only "Loopy"). We
 *        remap the cursor keys to move an on-screen cursor rather
 *        than sending arrow keys to the game.
 *
 *        We also have the option of sending a right-click event when
 *        the "select" key is held; unfortunately, this conflicts with
 *        being able to drag the cursor while the virtual "mouse
 *        button" is depressed. This restriction is enforced by
 *        `tune_input()'.
 *
 *     b) Numerical chooser spinbox
 *
 *        Games that require keyboard input beyond the arrow keys and
 *        "select" (currently Filling, Keen, Solo, Towers, Undead, and
 *        Unequal) are accommodated via a spinbox-like interface.
 *
 *        In these games, the user first uses the directional keys to
 *        position the cursor, and then presses "select" to activate
 *        the spinbox mode. Then, the left and right keys are remapped
 *        to scroll through the list of possible keystrokes accepted
 *        by the game (retrieved through the midend_request_keys() API
 *        call). Once the user is happy with their selection, they
 *        press "select" again to deactivate the chooser, and the
 *        arrow keys regain their original function.
 *
 *     c) Force centering while zoomed
 *
 *        (This isn't an input adaptation but it doesn't quite fit
 *        anywhere else.)
 *
 *        In Inertia, we want to keep the ball centered on screen so
 *        that the user can see everything in all directions.
 *
 *     d) Long-press maps to spacebar; chording; falling edge events
 *
 *        These are grouped because the first features two are
 *        dependent on the last. This dependency is enforced with an
 *        `assert()'.
 *
 *        Some games want a spacebar event -- so we send one on a
 *        long-press of "select". However, we usually send key events
 *        to the game immediately after the key is depressed, so we
 *        can't distinguish a hold vs. a short click.
 *
 *        A similar issue arises when we want to allow chording of
 *        multiple keypresses (this is only used for Untangle, which
 *        allows diagonal movements by chording two arrow keys) -- if
 *        we detect that a key has just been pressed, we don't know if
 *        the user is going to press more keys later on to form a
 *        chorded input.
 *
 *        In both of these scenarios we disambiguate the possible
 *        cases by waiting until a key has been released before we
 *        send the input keystroke(s) to the game.
 *
 *  5) Game configuration and preset management
 *
 *     The backend games specify a hierarchy of user-adjustable game
 *     configuration parameters that control aspects of puzzle
 *     generation, etc. Also supplied are a set of "presets" that
 *     specify a predetermined set of configuration parameters.
 *
 *  6) In-game help
 *
 *     The sgt-puzzles manual (src/puzzles.but) contains a chapter
 *     describing each game. To aid the user in learning each puzzle,
 *     each game plugin contains a compiled-in version of each
 *     puzzle's corresponding manual chapter.
 *
 *     The compiled-in help text is automatically generated from the
 *     puzzles.but file with a system of shell scripts (genhelp.sh),
 *     which also performs LZ4 compression on the text to conserve
 *     memory on smaller targets. The output of this script is found
 *     in the help/ directory. On-target LZ4 decompression is handled
 *     by lz4tiny.c.
 *
 *  7) Debug menu
 *
 *     The debug menu is activated by clicking "Quick help" five times
 *     in a row. Sorry, Android. This is helpful for benchmarking
 *     optimizations and selecting the activating the input
 *     accommodations described in (4).
 *
 * --------------------
 * Building and linking
 * --------------------
 *
 * Each sgt-*.rock executable is produced by statically compiling the
 * backend (i.e. game-specific) source file and help file (see (6)
 * above) against a set of common source files.
 *
 * The backend source files are listed in SOURCES.games; the common
 * source files are in SOURCES.
 *
 * ----------
 * References
 * ----------
 *  [1]: https://www.chiark.greenend.org.uk/~sgtatham/puzzles/devel/drawing.html#drawing
 *  [2]: https://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
 */

#include "plugin.h"

#include "help.h"
#include "lz4tiny.h"

#include "src/puzzles.h"

#include "lib/helper.h"
#include "lib/keymaps.h"
#include "lib/playback_control.h"
#include "lib/simple_viewer.h"
#include "lib/xlcd.h"

#include "fixedpoint.h"

#include "pluginbitmaps/puzzles_cursor.h"

/* how many ticks between timer callbacks */
#define TIMER_INTERVAL (HZ / 50)

/* Disable some features if we're memory constrained (c200v2) */
#if PLUGIN_BUFFER_SIZE > 0x14000
#define DEBUG_MENU
#define FONT_CACHING
#endif

/* background color (mimicking from the JS frontend) */
#define BG_R .9f /* very light gray */
#define BG_G .9f
#define BG_B .9f
#define BG_COLOR LCD_RGBPACK((int)(255*BG_R), (int)(255*BG_G), (int)(255*BG_B))

/* used for invalid config value message */
#define ERROR_COLOR LCD_RGBPACK(255, 0, 0)

/* subtract two to allow for the fixed and UI fonts */
#define MAX_FONTS (MAXUSERFONTS - 2)
#define FONT_TABLE PLUGIN_GAMES_DATA_DIR "/.sgt-puzzles.fnttab"

/* font bundle size range (in pixels) */
#define BUNDLE_MIN 7
#define BUNDLE_MAX 36
#define BUNDLE_COUNT (BUNDLE_MAX - BUNDLE_MIN + 1)

/* max length of C_STRING config vals */
#define MAX_STRLEN 128

/* attempt to increment a numeric config value up to this much */
#define CHOOSER_MAX_INCR 2

/* max font table line */
#define MAX_LINE 128

/* Sorry. */
#define MURICA

#ifdef MURICA
#define midend_serialize midend_serialise
#define midend_deserialize midend_deserialise
#define frontend_default_color frontend_default_colour
#define midend_colors midend_colours
#endif

/* magnification factor */
#define ZOOM_FACTOR 3

/* distance to pan per click (in pixels) */
#define PAN_X (MIN(LCD_HEIGHT, LCD_WIDTH) / 4)
#define PAN_Y (MIN(LCD_HEIGHT, LCD_WIDTH) / 4)

/* utility macros */
#undef ABS
#define ABS(a) ((a)<0?-(a):(a))
#define SWAP(a, b, t) do { t = a; a = b; b = t; } while(0)

/* fixed-point stuff (for antialiased lines) */
#define fp_fpart(f, bits) ((f) & ((1 << (bits)) - 1))
#define fp_rfpart(f, bits) ((1 << (bits)) - fp_fpart(f, bits))
#define FRACBITS 16

static void fix_size(void);
static int pause_menu(void);
static inline void plot(fb_data *fb, int w, int h,
                        unsigned x, unsigned y, unsigned long a,
                        unsigned long r1, unsigned long g1, unsigned long b1,
                        unsigned cl, unsigned cr, unsigned cu, unsigned cd);
static void zoom_clamp_panning(void);

static midend *me = NULL;
static unsigned *colors = NULL;
static int ncolors = 0;
static long last_keystate = 0;

#if defined(FOR_REAL) && defined(DEBUG_MENU)
/* the "debug menu" is hidden by default in order to avoid the
 * naturally ensuing complaints from users */
static bool debug_mode = false;
static int help_times = 0;
#endif

/* clipping stuff */
static fb_data *lcd_fb;
static struct viewport clip_rect;
static bool clipped = false, zoom_enabled = false, view_mode = true, mouse_mode = false;

static int mouse_x, mouse_y;

extern bool audiobuf_available; /* defined in rbmalloc.c */

static fb_data *zoom_fb; /* dynamically allocated */
static int zoom_x = -1, zoom_y = -1, zoom_w, zoom_h, zoom_clipu, zoom_clipd, zoom_clipl, zoom_clipr;
static bool zoom_force_center;
static int cur_font = FONT_UI;

static bool need_draw_update = false;
static int ud_l = 0, ud_u = 0, ud_r = LCD_WIDTH, ud_d = LCD_HEIGHT;

static char *titlebar = NULL;

/* some games can run in a separate thread for a larger stack (only
 * Solo for now) */
static int thread = -1;

/* how to process the input (custom, per-game) */
static struct {
    bool want_spacebar; /* send spacebar event on long-press of select */
    bool falling_edge; /* send events upon button release, not initial press */
    bool ignore_repeats; /* ignore repeated button events (currently in all games but Untangle) */
    bool rclick_on_hold; /* if in mouse mode, send right-click on long-press of select */
    bool numerical_chooser; /* repurpose select to activate a numerical chooser */
} input_settings;

static bool accept_input = true;

/* last timer call */
static long last_tstamp;
static bool timer_on = false;

static bool load_success;

/* debug settings */
/* ...did I mention there's a secret debug menu? */
static struct {
    int slowmo_factor;
    bool timerflash, clipoff, shortcuts, no_aa, polyanim, highlight_cursor;
} debug_settings;

// used in menu titles - make sure to initialize!
static char menu_desc[32];

/*
 * These are re-implementations of many rockbox drawing functions, adapted to
 * draw into a custom framebuffer (used for the zoom feature):
 */

static void zoom_drawpixel(int x, int y)
{
    if(y < zoom_clipu || y >= zoom_clipd)
        return;

    if(x < zoom_clipl || x >= zoom_clipr)
        return;

#if LCD_DEPTH > 24
    unsigned int pix = rb->lcd_get_foreground();
    zoom_fb[y * zoom_w + x].b = RGB_UNPACK_BLUE(pix);
    zoom_fb[y * zoom_w + x].g = RGB_UNPACK_GREEN(pix);
    zoom_fb[y * zoom_w + x].r = RGB_UNPACK_RED(pix);
    zoom_fb[y * zoom_w + x].x = 255;
#elif LCD_DEPTH == 24
    /* I hate these */
    unsigned int pix = rb->lcd_get_foreground();
    zoom_fb[y * zoom_w + x].b = RGB_UNPACK_BLUE(pix);
    zoom_fb[y * zoom_w + x].g = RGB_UNPACK_GREEN(pix);
    zoom_fb[y * zoom_w + x].r = RGB_UNPACK_RED(pix);
#else
    zoom_fb[y * zoom_w + x] = rb->lcd_get_foreground();
#endif
}

static void zoom_hline(int l, int r, int y)
{
    if(y < zoom_clipu || y >= zoom_clipd)
        return;
    if(l > r)
    {
        int t = l;
        l = r;
        r = t;
    }

    if(l < zoom_clipl)
        l = zoom_clipl;
    if(r >= zoom_clipr)
        r = zoom_clipr;

#if LCD_DEPTH > 24
    fb_data pixel = { RGB_UNPACK_BLUE(rb->lcd_get_foreground()),
                      RGB_UNPACK_GREEN(rb->lcd_get_foreground()),
                      RGB_UNPACK_RED(rb->lcd_get_foreground()),
                      255
    };
#elif LCD_DEPTH == 24
    fb_data pixel = { RGB_UNPACK_BLUE(rb->lcd_get_foreground()),
                      RGB_UNPACK_GREEN(rb->lcd_get_foreground()),
                      RGB_UNPACK_RED(rb->lcd_get_foreground()) };
#else
    fb_data pixel = rb->lcd_get_foreground();
#endif

    fb_data *ptr = zoom_fb + y * zoom_w + l;
    for(int i = 0; i < r - l; ++i)
        *ptr++ = pixel;
}

static void zoom_fillcircle(int cx, int cy, int radius)
{
    /* copied straight from xlcd_draw.c */
    int d = 3 - (radius * 2);
    int x = 0;
    int y = radius;
    while(x <= y)
    {
        zoom_hline(cx - x, cx + x, cy + y);
        zoom_hline(cx - x, cx + x, cy - y);
        zoom_hline(cx - y, cx + y, cy + x);
        zoom_hline(cx - y, cx + y, cy - x);
        if(d < 0)
        {
            d += (x * 4) + 6;
        }
        else
        {
            d += ((x - y) * 4) + 10;
            --y;
        }
        ++x;
    }
}

static void zoom_drawcircle(int cx, int cy, int radius)
{
    int d = 3 - (radius * 2);
    int x = 0;
    int y = radius;
    while(x <= y)
    {
        zoom_drawpixel(cx + x, cy + y);
        zoom_drawpixel(cx - x, cy + y);
        zoom_drawpixel(cx + x, cy - y);
        zoom_drawpixel(cx - x, cy - y);
        zoom_drawpixel(cx + y, cy + x);
        zoom_drawpixel(cx - y, cy + x);
        zoom_drawpixel(cx + y, cy - x);
        zoom_drawpixel(cx - y, cy - x);
        if(d < 0)
        {
            d += (x * 4) + 6;
        }
        else
        {
            d += ((x - y) * 4) + 10;
            --y;
        }
        ++x;
    }
}

/* This format is pretty crazy: each byte holds the states of 8 pixels
 * in a column, with bit 0 being the topmost pixel. Who needs cache
 * efficiency? */
static void zoom_mono_bitmap(const unsigned char *bits, int x, int y, int w, int h)
{
    unsigned int pix = rb->lcd_get_foreground();
    for(int i = 0; i < h / 8 + 1; ++i)
    {
        for(int j = 0; j < w; ++j)
        {
            unsigned char column = bits[i * w + j];
            for(int dy = 0; dy < 8; ++dy)
            {
                if(column & 1)
                {
#if LCD_DEPTH > 24
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j].b = RGB_UNPACK_BLUE(pix);
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j].g = RGB_UNPACK_GREEN(pix);
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j].r = RGB_UNPACK_RED(pix);
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j].x = 255;
#elif LCD_DEPTH == 24
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j].b = RGB_UNPACK_BLUE(pix);
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j].g = RGB_UNPACK_GREEN(pix);
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j].r = RGB_UNPACK_RED(pix);
#else
                    zoom_fb[(y + i * 8 + dy) * zoom_w + x + j] = pix;
#endif
                }
                column >>= 1;
            }
        }
    }
}

/*
 * Rockbox's alpha format is actually pretty sane: each byte holds
 * alpha values for two horizontally adjacent pixels. Low half is
 * leftmost pixel. See lcd-16bit-common.c for more info.
 */
static void zoom_alpha_bitmap(const unsigned char *bits, int x, int y, int w, int h)
{
    const unsigned char *ptr = bits;
    unsigned char buf = 0;
    int n_read = 0; /* how many 4-bit nibbles we've read (read new when even) */

    unsigned int pix = rb->lcd_get_foreground();
    unsigned int r = RGB_UNPACK_RED(pix), g = RGB_UNPACK_GREEN(pix), b = RGB_UNPACK_BLUE(pix);

    for(int i = 0; i < h; ++i)
    {
        for(int j = 0; j < w; ++j)
        {
            if(n_read % 2 == 0)
            {
                buf = *ptr++;
            }
            int pix_alpha = (n_read++ % 2) ? buf >> 4 : buf & 0xF; /* in reverse order */

            pix_alpha = 15 - pix_alpha; /* correct order now, still 0-F */

            int plot_alpha = (pix_alpha << 4) | pix_alpha; /* so 0xF -> 0xFF, 0x1 -> 0x11, etc. */

            plot(zoom_fb, zoom_w, zoom_h, x + j, y + i, plot_alpha, r, g, b,
                 0, zoom_w, 0, zoom_h);
        }
    }
}

/*
 * Font management routines
 *
 * Many puzzles need a dynamic font size, especially when zooming
 * in. Rockbox's default font set does not provide the consistency we
 * need across different sizes, so instead we ship a custom font pack
 * for sgt-puzzles, available from [1] or through Rockbox Utility.
 *
 * The font pack consists of 3 small-size fonts, and the Deja Vu
 * Sans/Mono fonts, rasterized in sizes from 10px to BUNDLE_MAX
 * (currently 36).
 *
 * The font loading code below tries to be smart about loading fonts:
 * when games are saved, the set of fonts that were loaded during
 * execution is written to a "font table" on disk. On subsequent
 * loads, the fonts in this table are precached while the game is
 * loaded (and the disk is spinning, on hard drive devices). We also
 * have a form of LRU caching implemented to dynamically evict fonts
 * from Rockbox's in-memory cache, which is of limited size.
 *
 * [1]: http://download.rockbox.org/useful/sgt-fonts.zip
 */

static struct bundled_font {
     /*
      * -3 = never tried loading, or unloaded,
      * -2 = failed to load,
      * [-1,): loaded successfully (FONT_SYSFIXED = -1)
      */
    int status;
    int last_use;
} *loaded_fonts = NULL; /* monospace are first, then proportional */

static int n_fonts, access_counter = -1;

/* called on exit and before entering help viewer (workaround for a
   possible bug in simple_viewer) */
static void unload_fonts(void)
{
    for(int i = 0; i < 2 * BUNDLE_COUNT; ++i)
        if(loaded_fonts[i].status > 0) /* don't unload FONT_UI */
        {
            rb->font_unload(loaded_fonts[i].status);
            loaded_fonts[i].status = -3;
        }
    access_counter = -1;
    rb->lcd_setfont(cur_font = FONT_UI);
}

static void init_fonttab(void)
{
    loaded_fonts = smalloc(2 * BUNDLE_COUNT * sizeof(struct bundled_font));
    for(int i = 0; i < 2 * BUNDLE_COUNT; ++i)
        loaded_fonts[i].status = -3;
    access_counter = 0;
    n_fonts = 0;
}

static void font_path(char *buf, int type, int size)
{
    if(size < 10) /* Deja Vu only goes down to 10px, below that it's a giant blob */
    {
        if(size < 7)
            size = 7; /* we're not going to force anyone to read 05-Tiny :P */
        /* we also don't care about monospace/proportional at this point */
        switch(size)
        {
        case 7:
            rb->snprintf(buf, MAX_PATH, FONT_DIR "/07-Fixed.fnt");
            break;
        case 8:
            rb->snprintf(buf, MAX_PATH, FONT_DIR "/08-Rockfont.fnt");
            break;
        case 9:
            rb->snprintf(buf, MAX_PATH, FONT_DIR "/09-Fixed.fnt");
            break;
        default:
            assert(false);
        }
    }
    else
        rb->snprintf(buf, MAX_PATH, FONT_DIR "/%02d-%s.fnt", size, type == FONT_FIXED ? "DejaVuSansMono" : "DejaVuSans");
}

static void rb_setfont(int type, int size)
{
    LOGF("rb_setfont(type=%d, size=%d)", type, size);

    /*
     * First, clamp to range. No puzzle should ever need this large of
     * a font, anyways.
     */
    if(size > BUNDLE_MAX)
        size = BUNDLE_MAX;

    if(size < 10)
    {
        if(size < 7) /* no teeny-tiny fonts */
            size = 7;
        /* assume monospace for 7-9px fonts */
        type = FONT_FIXED;
    }

    LOGF("target font type, size: %d, %d", type, size);

    int font_idx = (type == FONT_FIXED ? 0 : BUNDLE_COUNT) + size - BUNDLE_MIN;

    LOGF("font index: %d, status=%d", font_idx, loaded_fonts[font_idx].status);

    switch(loaded_fonts[font_idx].status)
    {
    case -3:
    {
        /* never loaded */
        LOGF("font %d is not resident, trying to load", font_idx);

        char buf[MAX_PATH];
        font_path(buf, type, size);

        LOGF("font should be at: %s", buf);

        if(n_fonts >= MAX_FONTS)
        {
            /* unload an old font */
            LOGF("too many resident fonts, evicting LRU");

            int oldest_use = -1, oldest_idx = -1;
            for(int i = 0; i < 2 * BUNDLE_COUNT; ++i)
            {
                if((loaded_fonts[i].status >= 0 && loaded_fonts[i].last_use < oldest_use) || oldest_use < 0)
                {
                    oldest_use = loaded_fonts[i].last_use;
                    oldest_idx = i;
                }
            }
            assert(oldest_idx >= 0);

            LOGF("evicting %d", oldest_idx);
            rb->font_unload(loaded_fonts[oldest_idx].status);
            loaded_fonts[oldest_idx].status = -3;
            n_fonts--;
        }

        loaded_fonts[font_idx].status = rb->font_load(buf);
        if(loaded_fonts[font_idx].status < 0)
        {
            LOGF("failed to load font %s", buf);
            goto fallback;
        }
        loaded_fonts[font_idx].last_use = access_counter++;
        n_fonts++;
        cur_font = loaded_fonts[font_idx].status;
        rb->lcd_setfont(cur_font);
        break;
    }
    case -2:
    case -1:
        goto fallback;
    default:
        loaded_fonts[font_idx].last_use = access_counter++;
        cur_font = loaded_fonts[font_idx].status;
        rb->lcd_setfont(cur_font);
        break;
    }

    return;

fallback:
    LOGF("could not load font of desired size; falling back to system font");

    cur_font = (type == FONT_FIXED) ? FONT_SYSFIXED : FONT_UI;
    rb->lcd_setfont(cur_font);

    LOGF("set font to %d", cur_font);

    return;
}

/*** Drawing API (normal, no zoom) ***/

static void offset_coords(int *x, int *y)
{
    if(clipped)
    {
        *x -= clip_rect.x;
        *y -= clip_rect.y;
    }
}

static void rb_color(int n)
{
    if(n < 0)
    {
        fatal("bad color %d", n);
        return;
    }
    rb->lcd_set_foreground(colors[n]);
}

/* clipping is implemented through viewports and offsetting
 * coordinates */
static void rb_clip(void *handle, int x, int y, int w, int h)
{
    if(!zoom_enabled)
    {
        if(!debug_settings.clipoff)
        {
            LOGF("rb_clip(%d %d %d %d)", x, y, w, h);
            clip_rect.x = MAX(0, x);
            clip_rect.y = MAX(0, y);
            clip_rect.width  = MIN(LCD_WIDTH, w);
            clip_rect.height = MIN(LCD_HEIGHT, h);
            clip_rect.font = FONT_UI;
            clip_rect.drawmode = DRMODE_SOLID;
#if LCD_DEPTH > 1
            clip_rect.fg_pattern = LCD_DEFAULT_FG;
            clip_rect.bg_pattern = LCD_DEFAULT_BG;
#endif
            rb->lcd_set_viewport(&clip_rect);
            clipped = true;
        }
    }
    else
    {
        zoom_clipu = y;
        zoom_clipd = y + h;
        zoom_clipl = x;
        zoom_clipr = x + w;
    }
}

static void rb_unclip(void *handle)
{
    if(!zoom_enabled)
    {
        LOGF("rb_unclip");
        rb->lcd_set_viewport(NULL);
        clipped = false;
    }
    else
    {
        zoom_clipu = 0;
        zoom_clipd = zoom_h;
        zoom_clipl = 0;
        zoom_clipr = zoom_w;
    }
}

static void rb_draw_text(void *handle, int x, int y, int fonttype,
                         int fontsize, int align, int color, const char *text)
{
    (void) fontsize;

    LOGF("rb_draw_text(%d %d \"%s\" size=%d)", x, y, text, fontsize);

    rb_color(color);
    rb_setfont(fonttype, fontsize); /* size will be clamped if too large */

    int w, h;
    rb->font_getstringsize(text, &w, &h, cur_font);

    LOGF("getting string size of font %d: %dx%d\n", cur_font, w, h);

    if(align & ALIGN_VNORMAL)
        y -= h;
    else if(align & ALIGN_VCENTRE)
        y -= h / 2;

    if(align & ALIGN_HCENTRE)
        x -= w / 2;
    else if(align & ALIGN_HRIGHT)
        x -= w;

    LOGF("calculated origin: (%d, %d) size: (%d, %d)", x, y, w, h);

    if(!zoom_enabled)
    {
        offset_coords(&x, &y);

        rb->lcd_set_drawmode(DRMODE_FG);
        rb->lcd_putsxy(x, y, text);
        rb->lcd_set_drawmode(DRMODE_SOLID);
    }
    else
    {
        /* we need to access the font bitmap directly */
        struct font *pf = rb->font_get(cur_font);

        while(*text)
        {
            /* still only reads 1 byte */
            unsigned short c = *text++;
            const unsigned char *bits = rb->font_get_bits(pf, c);
            int width = rb->font_get_width(pf, c);

            /* straight from lcd-bitmap-common.c */
#if defined(HAVE_LCD_COLOR)
            if (pf->depth)
            {
                /* lcd_alpha_bitmap_part isn't exported directly. However,
                 * we can create a null bitmap struct with only an alpha
                 * channel to make lcd_bmp_part call it for us. */

                zoom_alpha_bitmap(bits, x, y, width, pf->height);
            }
            else
#endif
                zoom_mono_bitmap(bits, x, y, width, pf->height);
            x += width;
        }
    }
}

static void rb_draw_rect(void *handle, int x, int y, int w, int h, int color)
{
    rb_color(color);
    if(!zoom_enabled)
    {
        LOGF("rb_draw_rect(%d, %d, %d, %d, %d)", x, y, w, h, color);
        offset_coords(&x, &y);
        rb->lcd_fillrect(x, y, w, h);
    }
    else
    {
        /* TODO: clipping */
        for(int i = y; i < y + h; ++i)
        {
            zoom_hline(x, x + w, i);
        }
    }
}

/* a goes from 0-255, with a = 255 being fully opaque and a = 0 transparent */
static inline void plot(fb_data *fb, int w, int h,
                        unsigned x, unsigned y, unsigned long a,
                        unsigned long r1, unsigned long g1, unsigned long b1,
                        unsigned cl, unsigned cr, unsigned cu, unsigned cd)
{
    /* This is really quite possibly the least efficient way of doing
       this. A better way would be in draw_antialiased_line(), but the
       problem with that is that the algorithms I investigated at
       least were incorrect at least part of the time and didn't make
       drawing much faster overall. */
    if(!(cl <= x && x < cr && cu <= y && y < cd))
        return;

    fb_data *ptr = fb + y * w + x;
    fb_data orig = *ptr;
    unsigned long r2, g2, b2;
#if LCD_DEPTH < 24
    r2 = RGB_UNPACK_RED(orig);
    g2 = RGB_UNPACK_GREEN(orig);
    b2 = RGB_UNPACK_BLUE(orig);
#else
    r2 = orig.r;
    g2 = orig.g;
    b2 = orig.b;
#endif

    unsigned long r, g, b;
    r = ((r1 * a) + (r2 * (256 - a))) >> 8;
    g = ((g1 * a) + (g2 * (256 - a))) >> 8;
    b = ((b1 * a) + (b2 * (256 - a))) >> 8;

#if LCD_DEPTH < 24
    *ptr = LCD_RGBPACK(r, g, b);
#elif LCD_DEPTH > 24
    *ptr = (fb_data) {b, g, r, 255};
#else
    *ptr = (fb_data) {b, g, r};
#endif
}

/* speed benchmark: 34392 lines/sec vs 112687 non-antialiased
 * lines/sec at full optimization on ipod6g */

/* expects UN-OFFSET coordinates, directly access framebuffer */
static void draw_antialiased_line(fb_data *fb, int w, int h, int x0, int y0, int x1, int y1)
{
    /* fixed-point Wu's algorithm, modified for integer-only endpoints */

    /* passed to plot() to avoid re-calculation */
    unsigned short l = 0, r = w, u = 0, d = h;
    if(!zoom_enabled)
    {
        if(clipped)
        {
            l = clip_rect.x;
            r = clip_rect.x + clip_rect.width;
            u = clip_rect.y;
            d = clip_rect.y + clip_rect.height;
        }
    }
    else
    {
        l = zoom_clipl;
        r = zoom_clipr;
        u = zoom_clipu;
        d = zoom_clipd;
    }

    bool steep = ABS(y1 - y0) > ABS(x1 - x0);
    int tmp;
    if(steep)
    {
        SWAP(x0, y0, tmp);
        SWAP(x1, y1, tmp);
    }
    if(x0 > x1)
    {
        SWAP(x0, x1, tmp);
        SWAP(y0, y1, tmp);
    }

    int dx, dy;
    dx = x1 - x0;
    dy = y1 - y0;

    if((dx << FRACBITS) == 0)
        return; /* bail out */

    long gradient = fp_div(dy << FRACBITS, dx << FRACBITS, FRACBITS);
    long intery = (y0 << FRACBITS);

    unsigned color = rb->lcd_get_foreground();
    unsigned long r1, g1, b1;
    r1 = RGB_UNPACK_RED(color);
    g1 = RGB_UNPACK_GREEN(color);
    b1 = RGB_UNPACK_BLUE(color);

    /* main loop */
    if(steep)
    {
        for(int x = x0; x <= x1; ++x, intery += gradient)
        {
            unsigned y = intery >> FRACBITS;
            unsigned alpha = fp_fpart(intery, FRACBITS) >> (FRACBITS - 8);

            plot(fb, w, h, y, x, (1 << 8) - alpha, r1, g1, b1, l, r, u, d);
            plot(fb, w, h, y + 1, x, alpha, r1, g1, b1, l, r, u, d);
        }
    }
    else
    {
        for(int x = x0; x <= x1; ++x, intery += gradient)
        {
            unsigned y = intery >> FRACBITS;
            unsigned alpha = fp_fpart(intery, FRACBITS) >> (FRACBITS - 8);

            plot(fb, w, h, x, y, (1 << 8) - alpha, r1, g1, b1, l, r, u, d);
            plot(fb, w, h, x, y + 1, alpha, r1, g1, b1, l, r, u, d);
        }
    }
}

static void rb_draw_line(void *handle, int x1, int y1, int x2, int y2,
                         int color)
{
    rb_color(color);

    if(!zoom_enabled)
    {
        LOGF("rb_draw_line(%d, %d, %d, %d, %d)", x1, y1, x2, y2, color);
#if defined(FOR_REAL) && defined(DEBUG_MENU)
        if(debug_settings.no_aa)
        {
            offset_coords(&x1, &y1);
            offset_coords(&x2, &y2);
            rb->lcd_drawline(x1, y1, x2, y2);
        }
        else
#endif
            draw_antialiased_line(lcd_fb, LCD_WIDTH, LCD_HEIGHT, x1, y1, x2, y2);
    }
    else
    {
        /* draw_antialiased_line uses rb->lcd_get_foreground() to get
         * the color */

        draw_antialiased_line(zoom_fb, zoom_w, zoom_h, x1, y1, x2, y2);
    }
}

#if 0
/*
 * draw filled polygon
 * originally by Sebastian Leonhardt (ulmutul)
 * 'count' : number of coordinate pairs
 * 'pxy': array of coordinates. pxy[0]=x0,pxy[1]=y0,...
 * note: provide space for one extra coordinate, because the starting point
 * will automatically be inserted as end point.
 */

/*
 * helper function:
 * find points of intersection between polygon and scanline
 */

#define MAX_INTERSECTION 32

static void fill_poly_line(int scanline, int count, int *pxy)
{
    int i;
    int j;
    int num_of_intersects;
    int direct, old_direct;
    //intersections of every line with scanline (y-coord)
    int intersection[MAX_INTERSECTION];
    /* add starting point as ending point */
    pxy[count*2] = pxy[0];
    pxy[count*2+1] = pxy[1];

    old_direct=0;
    num_of_intersects=0;
    for (i=0; i<count*2; i+=2) {
        int x1=pxy[i];
        int y1=pxy[i+1];
        int x2=pxy[i+2];
        int y2=pxy[i+3];
        // skip if line is outside of scanline
        if (y1 < y2) {
            if (scanline < y1 || scanline > y2)
                continue;
        }
        else {
            if (scanline < y2 || scanline > y1)
                continue;
        }
        // calculate x-coord of intersection
        if (y1==y2) {
            direct=0;
        }
        else {
            direct = y1>y2 ? 1 : -1;
            // omit double intersections, if both lines lead in the same direction
            intersection[num_of_intersects] =
                x1+((scanline-y1)*(x2-x1))/(y2-y1);
            if ( (direct!=old_direct)
                 || (intersection[num_of_intersects] != intersection[num_of_intersects-1])
                )
                ++num_of_intersects;
        }
        old_direct = direct;
    }

    // sort points of intersection
    for (i=0; i<num_of_intersects-1; ++i) {
        for (j=i+1; j<num_of_intersects; ++j) {
            if (intersection[j]<intersection[i]) {
                int temp=intersection[i];
                intersection[i]=intersection[j];
                intersection[j]=temp;
            }
        }
    }
    // draw
    for (i=0; i<num_of_intersects; i+=2) {
        rb->lcd_hline(intersection[i], intersection[i+1], scanline);
    }
}

/* two extra elements at end of pxy needed */
static void v_fillarea(int count, int *pxy)
{
    int i;
    int y1, y2;

    // find min and max y coords
    y1=y2=pxy[1];
    for (i=3; i<count*2; i+=2) {
        if (pxy[i] < y1) y1 = pxy[i];
        else if (pxy[i] > y2) y2 = pxy[i];
    }

    for (i=y1; i<=y2; ++i) {
        fill_poly_line(i, count, pxy);
    }
}
#endif

/* I'm a horrible person: this was copy-pasta'd straight from
 * xlcd_draw.c */

/* sort the given coordinates by increasing x value */
static void sort_points_by_increasing_x(int* x1, int* y1,
                                        int* x2, int* y2,
                                        int* x3, int* y3)
{
    int x, y;
    if (*x1 > *x3)
    {
        if (*x2 < *x3)       /* x2 < x3 < x1 */
        {
            x = *x1; *x1 = *x2; *x2 = *x3; *x3 = x;
            y = *y1; *y1 = *y2; *y2 = *y3; *y3 = y;
        }
        else if (*x2 > *x1)  /* x3 < x1 < x2 */
        {
            x = *x1; *x1 = *x3; *x3 = *x2; *x2 = x;
            y = *y1; *y1 = *y3; *y3 = *y2; *y2 = y;
        }
        else               /* x3 <= x2 <= x1 */
        {
            x = *x1; *x1 = *x3; *x3 = x;
            y = *y1; *y1 = *y3; *y3 = y;
        }
    }
    else
    {
        if (*x2 < *x1)       /* x2 < x1 <= x3 */
        {
            x = *x1; *x1 = *x2; *x2 = x;
            y = *y1; *y1 = *y2; *y2 = y;
        }
        else if (*x2 > *x3)  /* x1 <= x3 < x2 */
        {
            x = *x2; *x2 = *x3; *x3 = x;
            y = *y2; *y2 = *y3; *y3 = y;
        }
        /* else already sorted */
    }
}

#define sort_points_by_increasing_y(x1, y1, x2, y2, x3, y3)     \
    sort_points_by_increasing_x(y1, x1, y2, x2, y3, x3)

/* draw a filled triangle, using horizontal lines for speed */
static void zoom_filltriangle(int x1, int y1,
                              int x2, int y2,
                              int x3, int y3)
{
    long fp_x1, fp_x2, fp_dx1, fp_dx2;
    int y;
    sort_points_by_increasing_y(&x1, &y1, &x2, &y2, &x3, &y3);

    if (y1 < y3)  /* draw */
    {
        fp_dx1 = ((x3 - x1) << 16) / (y3 - y1);
        fp_x1  = (x1 << 16) + (1<<15) + (fp_dx1 >> 1);

        if (y1 < y2)  /* first part */
        {
            fp_dx2 = ((x2 - x1) << 16) / (y2 - y1);
            fp_x2  = (x1 << 16) + (1<<15) + (fp_dx2 >> 1);
            for (y = y1; y < y2; y++)
            {
                zoom_hline(fp_x1 >> 16, fp_x2 >> 16, y);
                fp_x1 += fp_dx1;
                fp_x2 += fp_dx2;
            }
        }
        if (y2 < y3)  /* second part */
        {
            fp_dx2 = ((x3 - x2) << 16) / (y3 - y2);
            fp_x2 = (x2 << 16) + (1<<15) + (fp_dx2 >> 1);
            for (y = y2; y < y3; y++)
            {
                zoom_hline(fp_x1 >> 16, fp_x2 >> 16, y);
                fp_x1 += fp_dx1;
                fp_x2 += fp_dx2;
            }
        }
    }
}

/* Should probably refactor this */
static void rb_draw_poly(void *handle, int *coords, int npoints,
                         int fillcolor, int outlinecolor)
{
    if(!zoom_enabled)
    {
        LOGF("rb_draw_poly");

        if(fillcolor >= 0)
        {
            rb_color(fillcolor);
#if 1
            /* serious hack: draw a bunch of triangles between adjacent points */
            /* this generally works, even with some concave polygons */
            for(int i = 2; i < npoints; ++i)
            {
                int x1, y1, x2, y2, x3, y3;
                x1 = coords[0];
                y1 = coords[1];
                x2 = coords[(i - 1) * 2];
                y2 = coords[(i - 1) * 2 + 1];
                x3 = coords[i * 2];
                y3 = coords[i * 2 + 1];
                offset_coords(&x1, &y1);
                offset_coords(&x2, &y2);
                offset_coords(&x3, &y3);
                xlcd_filltriangle(x1, y1,
                                  x2, y2,
                                  x3, y3);

#ifdef DEBUG_MENU
                if(debug_settings.polyanim)
                {
                    rb->lcd_update();
                    rb->sleep(HZ/4);
                }
#endif
#if 0
                /* debug code */
                rb->lcd_set_foreground(LCD_RGBPACK(255,0,0));
                rb->lcd_drawpixel(x1, y1);
                rb->lcd_drawpixel(x2, y2);
                rb->lcd_drawpixel(x3, y3);
                rb->lcd_update();
                rb->sleep(HZ);
                rb_color(fillcolor);
                rb->lcd_drawpixel(x1, y1);
                rb->lcd_drawpixel(x2, y2);
                rb->lcd_drawpixel(x3, y3);
                rb->lcd_update();
#endif
            }
#else
            int *pxy = smalloc(sizeof(int) * 2 * npoints + 2);
            /* copy points, offsetted */
            for(int i = 0; i < npoints; ++i)
            {
                pxy[2 * i + 0] = coords[2 * i + 0];
                pxy[2 * i + 1] = coords[2 * i + 1];
                offset_coords(&pxy[2*i+0], &pxy[2*i+1]);
            }
            v_fillarea(npoints, pxy);
            sfree(pxy);
#endif
        }

        /* draw outlines last so they're not covered by the fill */
        assert(outlinecolor >= 0);
        rb_color(outlinecolor);

        for(int i = 1; i < npoints; ++i)
        {
            int x1, y1, x2, y2;
            x1 = coords[2 * (i - 1)];
            y1 = coords[2 * (i - 1) + 1];
            x2 = coords[2 * i];
            y2 = coords[2 * i + 1];
            if(debug_settings.no_aa)
            {
                offset_coords(&x1, &y1);
                offset_coords(&x2, &y2);
                rb->lcd_drawline(x1, y1,
                                 x2, y2);
            }
            else
                draw_antialiased_line(lcd_fb, LCD_WIDTH, LCD_HEIGHT, x1, y1, x2, y2);

#ifdef DEBUG_MENU
            if(debug_settings.polyanim)
            {
                rb->lcd_update();
                rb->sleep(HZ/4);
            }
#endif
        }

        int x1, y1, x2, y2;
        x1 = coords[0];
        y1 = coords[1];
        x2 = coords[2 * (npoints - 1)];
        y2 = coords[2 * (npoints - 1) + 1];
        if(debug_settings.no_aa)
        {
            offset_coords(&x1, &y1);
            offset_coords(&x2, &y2);

            rb->lcd_drawline(x1, y1,
                             x2, y2);
        }
        else
            draw_antialiased_line(lcd_fb, LCD_WIDTH, LCD_HEIGHT, x1, y1, x2, y2);
    }
    else
    {
        LOGF("rb_draw_poly");

        if(fillcolor >= 0)
        {
            rb_color(fillcolor);

            /* serious hack: draw a bunch of triangles between adjacent points */
            /* this generally works, even with some concave polygons */
            for(int i = 2; i < npoints; ++i)
            {
                int x1, y1, x2, y2, x3, y3;
                x1 = coords[0];
                y1 = coords[1];
                x2 = coords[(i - 1) * 2];
                y2 = coords[(i - 1) * 2 + 1];
                x3 = coords[i * 2];
                y3 = coords[i * 2 + 1];
                zoom_filltriangle(x1, y1,
                                  x2, y2,
                                  x3, y3);
            }
        }

        /* draw outlines last so they're not covered by the fill */
        assert(outlinecolor >= 0);
        rb_color(outlinecolor);

        for(int i = 1; i < npoints; ++i)
        {
            int x1, y1, x2, y2;
            x1 = coords[2 * (i - 1)];
            y1 = coords[2 * (i - 1) + 1];
            x2 = coords[2 * i];
            y2 = coords[2 * i + 1];
            draw_antialiased_line(zoom_fb, zoom_w, zoom_h, x1, y1, x2, y2);
        }

        int x1, y1, x2, y2;
        x1 = coords[0];
        y1 = coords[1];
        x2 = coords[2 * (npoints - 1)];
        y2 = coords[2 * (npoints - 1) + 1];
        draw_antialiased_line(zoom_fb, zoom_w, zoom_h, x1, y1, x2, y2);
    }
}

static void rb_draw_circle(void *handle, int cx, int cy, int radius,
                           int fillcolor, int outlinecolor)
{
    if(!zoom_enabled)
    {
        LOGF("rb_draw_circle(%d, %d, %d)", cx, cy, radius);
        offset_coords(&cx, &cy);

        if(fillcolor >= 0)
        {
            rb_color(fillcolor);
            xlcd_fillcircle(cx, cy, radius - 1);
        }

        assert(outlinecolor >= 0);
        rb_color(outlinecolor);
        xlcd_drawcircle(cx, cy, radius - 1);
    }
    else
    {
        if(fillcolor >= 0)
        {
            rb_color(fillcolor);
            zoom_fillcircle(cx, cy, radius - 1);
        }

        assert(outlinecolor >= 0);
        rb_color(outlinecolor);
        zoom_drawcircle(cx, cy, radius - 1);
    }
}

/* blitters allow the game code to save/restore a piece of the
 * framebuffer */
struct blitter {
    bool have_data;
    int x, y;
    struct bitmap bmp;
};

/* originally from emcc.c */
static void trim_rect(int *x, int *y, int *w, int *h)
{
    int x0, x1, y0, y1;

    /*
     * Reduce the size of the copied rectangle to stop it going
     * outside the bounds of the canvas.
     */

    /* Transform from x,y,w,h form into coordinates of all edges */
    x0 = *x;
    y0 = *y;
    x1 = *x + *w;
    y1 = *y + *h;

    int screenw = zoom_enabled ? zoom_w : LCD_WIDTH;
    int screenh = zoom_enabled ? zoom_h : LCD_HEIGHT;

    /* Clip each coordinate at both extremes of the canvas */
    x0 = (x0 < 0 ? 0 : x0 > screenw ? screenw: x0);
    x1 = (x1 < 0 ? 0 : x1 > screenw ? screenw: x1);
    y0 = (y0 < 0 ? 0 : y0 > screenh ? screenh: y0);
    y1 = (y1 < 0 ? 0 : y1 > screenh ? screenh: y1);

    /* Transform back into x,y,w,h to return */
    *x = x0;
    *y = y0;
    *w = x1 - x0;
    *h = y1 - y0;
}

static blitter *rb_blitter_new(void *handle, int w, int h)
{
    LOGF("rb_blitter_new");
    blitter *b = snew(blitter);
    b->bmp.width = w;
    b->bmp.height = h;
    b->bmp.data = smalloc(w * h * sizeof(fb_data));
    b->have_data = false;
    return b;
}

static void rb_blitter_free(void *handle, blitter *bl)
{
    LOGF("rb_blitter_free");
    sfree(bl->bmp.data);
    sfree(bl);
    return;
}

/* copy a section of the framebuffer */
static void rb_blitter_save(void *handle, blitter *bl, int x, int y)
{
    /* no viewport offset */
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
#error no vertical stride
#else
    if(bl && bl->bmp.data)
    {
        int w = bl->bmp.width, h = bl->bmp.height;
        int screen_w = zoom_enabled ? zoom_w : LCD_WIDTH;

        trim_rect(&x, &y, &w, &h);

        fb_data *fb = zoom_enabled ? zoom_fb : lcd_fb;
        LOGF("rb_blitter_save(%d, %d, %d, %d)", x, y, w, h);
        for(int i = 0; i < h; ++i)
        {
            /* copy line-by-line */
            rb->memcpy(bl->bmp.data + sizeof(fb_data) * i * w,
                       fb + (y + i) * screen_w + x,
                       w * sizeof(fb_data));
        }
        bl->x = x;
        bl->y = y;
        bl->have_data = true;
    }
#endif
}

static void rb_blitter_load(void *handle, blitter *bl, int x, int y)
{
    LOGF("rb_blitter_load");
    if(!bl->have_data)
        return;
    int w = bl->bmp.width, h = bl->bmp.height;

    if(x == BLITTER_FROMSAVED) x = bl->x;
    if(y == BLITTER_FROMSAVED) y = bl->y;

    if(!zoom_enabled)
        offset_coords(&x, &y);

    trim_rect(&x, &y, &w, &h);

    if(!zoom_enabled)
    {
        rb->lcd_bitmap((fb_data*)bl->bmp.data, x, y, w, h);
    }
    else
    {
        for(int i = 0; i < h; ++i)
        {
            rb->memcpy(zoom_fb + (y + i) * zoom_w + x,
                       bl->bmp.data + sizeof(fb_data) * i * w,
                       w * sizeof(fb_data));
        }
    }
}

static void rb_draw_update(void *handle, int x, int y, int w, int h)
{
    LOGF("rb_draw_update(%d, %d, %d, %d)", x, y, w, h);

    /*
     * It seems that the puzzles use a different definition of
     * "updating" the display than Rockbox does; by calling this
     * function, it tells us that it has either already drawn to the
     * updated area (as rockbox assumes), or that it WILL draw to the
     * said area in the future (in which case we will draw
     * nothing). Because we don't know which of these is the case, we
     * simply remember a rectangle that contains all the updated
     * regions and update it at the very end.
     */

    /* adapted from gtk.c */
    if (!need_draw_update || ud_l > x  ) ud_l = x;
    if (!need_draw_update || ud_r < x+w) ud_r = x+w;
    if (!need_draw_update || ud_u > y  ) ud_u = y;
    if (!need_draw_update || ud_d < y+h) ud_d = y+h;

    need_draw_update = true;
}

static void rb_start_draw(void *handle)
{
    (void) handle;

    /* ... mumble mumble ... not ... reentrant ... mumble mumble ... */

    need_draw_update = false;
    ud_l = 0;
    ud_r = LCD_WIDTH;
    ud_u = 0;
    ud_d = LCD_HEIGHT;
}

static void rb_end_draw(void *handle)
{
    (void) handle;

    if(debug_settings.highlight_cursor)
    {
        rb->lcd_set_foreground(LCD_RGBPACK(255,0,255));
        int x, y, w, h;
        midend_get_cursor_location(me, &x, &y, &w, &h);
        if(x >= 0)
            rb->lcd_drawrect(x, y, w, h);
    }

    /* we ignore the backend's redraw requests and just
     * unconditionally update everything */
#if 0
    if(!zoom_enabled)
    {
        LOGF("rb_end_draw");

        if(need_draw_update)
            rb->lcd_update_rect(MAX(0, ud_l), MAX(0, ud_u), MIN(LCD_WIDTH, ud_r - ud_l), MIN(LCD_HEIGHT, ud_d - ud_u));
    }
    else
    {
        /* stubbed */
    }
#endif
}

static void rb_status_bar(void *handle, const char *text)
{
    if(titlebar)
        sfree(titlebar);
    titlebar = dupstr(text);
    LOGF("game title is %s\n", text);
}

static void draw_title(bool clear_first)
{
    const char *base;
    if(titlebar)
        base = titlebar;
    else
        base = midend_which_game(me)->name;

    char str[128];
    rb->snprintf(str, sizeof(str), "%s%s", base, zoom_enabled ? (view_mode ? " (viewing)" : " (interaction)") : "");

    /* quick hack */
    bool orig_clipped = false;
    if(!zoom_enabled)
    {
        orig_clipped = clipped;
        if(orig_clipped)
            rb_unclip(NULL);
    }

    int w, h;
    rb->lcd_setfont(cur_font = FONT_UI);
    rb->lcd_getstringsize(str, &w, &h);

    rb->lcd_set_foreground(BG_COLOR);
    rb->lcd_fillrect(0, LCD_HEIGHT - h, clear_first ? LCD_WIDTH : w, h);

    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_putsxy(0, LCD_HEIGHT - h, str);

    if(!zoom_enabled)
    {
        if(orig_clipped)
            rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);
    }
}

#define MOUSE_W BMPWIDTH_puzzles_cursor
#define MOUSE_H BMPHEIGHT_puzzles_cursor

static blitter *mouse_bl = NULL;

static void clear_mouse(void)
{
    bool orig_clipped = clipped;
    if(!zoom_enabled)
    {
        if(orig_clipped)
            rb_unclip(NULL);
    }

    if(mouse_bl)
        rb_blitter_load(NULL, mouse_bl, BLITTER_FROMSAVED, BLITTER_FROMSAVED);

    if(!zoom_enabled)
    {
        if(orig_clipped)
            rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);
    }
}

static void draw_mouse(void)
{
    bool orig_clipped = clipped;
    if(!zoom_enabled)
    {
        if(orig_clipped)
            rb_unclip(NULL);
    }

    if(!mouse_bl)
        mouse_bl = rb_blitter_new(NULL, MOUSE_W, MOUSE_H);

    /* save area being covered (will be restored elsewhere) */
    rb_blitter_save(NULL, mouse_bl, mouse_x, mouse_y);

    rb->lcd_bitmap_transparent(puzzles_cursor, mouse_x, mouse_y, BMPWIDTH_puzzles_cursor, BMPHEIGHT_puzzles_cursor);

    if(!zoom_enabled)
    {
        if(orig_clipped)
            rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);
    }
}

/* doesn't work, disabled (can't find a good mechanism to check if a
 * glyph exists in a font) */
#if 0
/* See: https://www.chiark.greenend.org.uk/~sgtatham/puzzles/devel/drawing.html#drawing-text-fallback */
static char *rb_text_fallback(void *handle, const char *const *strings,
                              int nstrings)
{
    struct font *pf = rb->font_get(cur_font);

    for(int i = 0; i < nstrings; i++)
    {
        LOGF("checking alternative \"%s\"", strings[i]);
        const unsigned char *ptr = strings[i];
        unsigned short code;
        bool valid = true;

        while(*ptr)
        {
            ptr = rb->utf8decode(ptr, &code);

            if(!rb->font_get_bits(pf, code))
            {
                valid = false;
                break;
            }
        }

        if(valid)
            return dupstr(strings[i]);
    }

    /* shouldn't get here */
    return dupstr(strings[nstrings - 1]);
}
#endif

const drawing_api rb_drawing = {
    rb_draw_text,
    rb_draw_rect,
    rb_draw_line,
    rb_draw_poly,
    rb_draw_circle,
    rb_draw_update,
    rb_clip,
    rb_unclip,
    rb_start_draw,
    rb_end_draw,
    rb_status_bar,
    rb_blitter_new,
    rb_blitter_free,
    rb_blitter_save,
    rb_blitter_load,
    /* printing functions */
    NULL, NULL, NULL, NULL, NULL, NULL, /* {begin,end}_{doc,page,puzzle} */
    NULL, NULL,                         /* line_width, line_dotted */
    NULL, /* fall back to ASCII */
    NULL,
};

/** utility functions exported to puzzles code **/

void fatal(const char *fmt, ...)
{
    va_list ap;
    char buf[256];

    rb->splash(HZ, "FATAL");

    va_start(ap, fmt);
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    LOGF("%s", buf);
    rb->splash(HZ * 2, buf);

    if(rb->thread_self() == thread)
        rb->thread_exit();
    else
        exit(PLUGIN_ERROR);
}

void get_random_seed(void **randseed, int *randseedsize)
{
    *randseed = snew(long);
    long seed = *rb->current_tick;
    rb->memcpy(*randseed, &seed, sizeof(seed));
    *randseedsize = sizeof(seed);
}

static void timer_cb(void)
{
#if LCD_DEPTH < 24
    if(debug_settings.timerflash)
    {
        static bool what = false;
        what = !what;
        if(what)
            lcd_fb[0] = LCD_BLACK;
        else
            lcd_fb[0] = LCD_WHITE;
        rb->lcd_update();
    }
#endif

    LOGF("timer callback");
    midend_timer(me, ((float)(*rb->current_tick - last_tstamp) / (float)HZ) / debug_settings.slowmo_factor);
    last_tstamp = *rb->current_tick;
}

void activate_timer(frontend *fe)
{
    last_tstamp = *rb->current_tick;
    timer_on = true;
}

void deactivate_timer(frontend *fe)
{
    timer_on = false;
}

void frontend_default_color(frontend *fe, float *out)
{
    *out++ = BG_R;
    *out++ = BG_G;
    *out++ = BG_B;
}

/** frontend code -- mostly UI stuff **/

static void send_click(int button, bool release)
{
    int x = (zoom_enabled ? zoom_x : 0) + mouse_x,
        y = (zoom_enabled ? zoom_y : 0) + mouse_y;
    assert(LEFT_BUTTON + 6 == LEFT_RELEASE);

    midend_process_key(me, x, y, button);

    if(release)
        midend_process_key(me, x, y, button + 6);
}

/*
 * Numerical chooser ("spinbox")
 *
 * Let the user scroll through the options for the keys they can
 * press.
 */
static int choose_key(void)
{
    int options = 0;

    key_label *game_keys = midend_request_keys(me, &options);

    if(!game_keys || !options)
        return 0;

    int sel = 0;

    while(1)
    {
        if(timer_on)
            timer_cb();
        midend_process_key(me, 0, 0, game_keys[sel].button);
        midend_force_redraw(me);

        if(zoom_enabled)
            rb->lcd_bitmap_part(zoom_fb, zoom_x, zoom_y, STRIDE(SCREEN_MAIN, zoom_w, zoom_h),
                                0, 0, LCD_WIDTH, LCD_HEIGHT);

        rb->lcd_update();
        rb->yield();

        int button = rb->button_get_w_tmo(timer_on ? TIMER_INTERVAL : -1);
        switch(button)
        {
        case BTN_LEFT:
            if(--sel < 0)
                sel = options - 1;
            break;
        case BTN_RIGHT:
            if(++sel >= options)
                sel = 0;
            break;
        case BTN_PAUSE:
            return -1;
        case BTN_FIRE:
            free_keys(game_keys, options);

            /* the key has already been sent to the game, just return */
            return 0;
        }
    }
}

/* This function handles most user input. It has specific workarounds
 * and fixes for certain games to allow them to work well on
 * Rockbox. It will either return a positive value that can be passed
 * to the midend, or a negative flag value. `do_pausemenu' sets how
 * this function will handle BUTTON_PAUSE: if true, it will handle it
 * all, otherwise it will simply return -1 and let the caller do the
 * work (this is used for zoom mode). */
static int process_input(int tmo, bool do_pausemenu)
{
    LOGF("process_input start");
    LOGF("-------------------");
    int state = 0;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false); /* about to block for button input */
#endif

    int button = rb->button_get_w_tmo(tmo);

    /* weird stuff */
    exit_on_usb(button);

    /* See if the button is a long-press. */
    if(accept_input && (button == (BTN_FIRE | BUTTON_REPEAT)))
    {
        LOGF("button is long-press, ignoring subsequent input until release");
        /* Ignore repeated long presses. */
        accept_input = false;

        if(mouse_mode && input_settings.rclick_on_hold)
        {
            /* simulate right-click */
            LOGF("sending right click");
            send_click(RIGHT_BUTTON, true);
            return 0;
        }

        /* These games want a spacebar in the event of a long press. */
        if(!mouse_mode && input_settings.want_spacebar)
            return ' ';
    }

    button = rb->button_status();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    if(button == BTN_PAUSE)
    {
        last_keystate = 0;
        accept_input = true;

        if(do_pausemenu)
        {
            /* quick hack to preserve the clipping state */
            bool orig_clipped = clipped;
            if(orig_clipped)
                rb_unclip(NULL);

            int rc = pause_menu();

            if(orig_clipped)
                rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);

            return rc;
        }
        else
            return -1;
    }

    /* Mouse movement (if enabled). This goes here since none of the
     * following code is needed for mouse mode. */
    if(mouse_mode)
    {
        static int held_count = 0, v = 2;

        int dx = 0, dy = 0;

        if(button & BTN_UP)
            dy -= 1;
        if(button & BTN_DOWN)
            dy += 1;
        if(button & BTN_LEFT)
            dx -= 1;
        if(button & BTN_RIGHT)
            dx += 1;

        unsigned released = ~button & last_keystate,
                  pressed =  button & ~last_keystate;

        /* acceleration */
        if(button && button == last_keystate)
        {
            if(++held_count % 4 == 0 && v < 15)
                v++;
        }
        else
        {
            LOGF("no buttons pressed, or state changed");
            v = 1;
            held_count = 0;
        }

        mouse_x += dx * v;
        mouse_y += dy * v;

        /* clamp */
        /* The % operator with negative operands is messy; this is much
         * simpler. */
        bool clamped_x = false, clamped_y = false;

        if(mouse_x < 0)
            mouse_x = 0, clamped_x = true;
        if(mouse_y < 0)
            mouse_y = 0, clamped_y = true;

        if(mouse_x >= LCD_WIDTH)
            mouse_x = LCD_WIDTH - 1, clamped_x = true;
        if(mouse_y >= LCD_HEIGHT)
            mouse_y = LCD_HEIGHT - 1, clamped_y = true;

        if((clamped_x || clamped_y) && zoom_enabled) {
            if(clamped_x)
                zoom_x += dx * v;
            if(clamped_y)
                zoom_y += dy * v;
            zoom_clamp_panning();
        }

        /* clicking/dragging */
        /* rclick on hold requires that we fire left-click on a
         * release, otherwise it's impossible to distinguish the
         * two. */
        if(input_settings.rclick_on_hold)
        {
            if(accept_input && released == BTN_FIRE)
            {
                LOGF("sending left click");
                send_click(LEFT_BUTTON, true); /* right-click is handled earlier */
            }
        }
        else
        {
            if(pressed & BTN_FIRE) {
                send_click(LEFT_BUTTON, false);
                accept_input = false;
            }
            else if(released & BTN_FIRE)
                send_click(LEFT_RELEASE, false);
            else if(button & BTN_FIRE)
                send_click(LEFT_DRAG, false);
        }

        if(!button)
        {
            LOGF("all keys released, accepting further input");
            accept_input = true;
        }

        last_keystate = button;

        /* no buttons are sent to the midend in mouse mode */
        return 0;
    }

    /* These games require, for one reason or another, that events
     * fire upon buttons being released rather than when they are
     * pressed. For Inertia, it is because it needs to be able to
     * sense multiple simultaneous keypresses (to move diagonally),
     * and the others require a long press to map to a secondary
     * "action" key. */
    if(input_settings.falling_edge)
    {
        LOGF("received button 0x%08x", button);

        unsigned released = ~button & last_keystate;

        last_keystate = button;

        if(!button)
        {
            if(!accept_input)
            {
                LOGF("ignoring, all keys released but not accepting input before, can accept input later");
                accept_input = true;
                return 0;
            }
        }

        if(!released || !accept_input)
        {
            LOGF("released keys detected: 0x%08x", released);
            LOGF("ignoring, either no keys released or not accepting input");
            return 0;
        }

        if(button)
        {
            LOGF("ignoring input from now until all released");
            accept_input = false;
        }

        button |= released;
        LOGF("accepting event 0x%08x", button);
    }
    /* Ignore repeats in all games which are not Untangle. */
    else if(input_settings.ignore_repeats)
    {
        /* start accepting input again after a release */
        if(!button)
        {
            accept_input = true;
            return 0;
        }

        /* ignore repeats (in mouse mode, only ignore repeats of BTN_FIRE) */
        if(!accept_input)
            return 0;

        accept_input = false;
    }

    switch(button)
    {
    case BTN_UP:
        state = CURSOR_UP;
        break;
    case BTN_DOWN:
        state = CURSOR_DOWN;
        break;
    case BTN_LEFT:
        state = CURSOR_LEFT;
        break;
    case BTN_RIGHT:
        state = CURSOR_RIGHT;
        break;

        /* handle diagonals (mainly for Inertia) */
    case BTN_DOWN | BTN_LEFT:
#ifdef BTN_DOWN_LEFT
    case BTN_DOWN_LEFT:
#endif
        state = '1' | MOD_NUM_KEYPAD;
        break;
    case BTN_DOWN | BTN_RIGHT:
#ifdef BTN_DOWN_RIGHT
    case BTN_DOWN_RIGHT:
#endif
        state = '3' | MOD_NUM_KEYPAD;
        break;
    case BTN_UP | BTN_LEFT:
#ifdef BTN_UP_LEFT
    case BTN_UP_LEFT:
#endif
        state = '7' | MOD_NUM_KEYPAD;
        break;
    case BTN_UP | BTN_RIGHT:
#ifdef BTN_UP_RIGHT
    case BTN_UP_RIGHT:
#endif
        state = '9' | MOD_NUM_KEYPAD;
        break;

    case BTN_FIRE:
        if(input_settings.numerical_chooser)
        {
            if(choose_key() < 0)
            {
                if(do_pausemenu)
                {
                    /* quick hack to preserve the clipping state */
                    bool orig_clipped = clipped;
                    if(orig_clipped)
                        rb_unclip(NULL);

                    int rc = pause_menu();

                    if(orig_clipped)
                        rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);

                    last_keystate = 0;
                    accept_input = true;

                    return rc;
                }
                else
                    return -1;
            }
        }
        else
        {
            if(!strcmp("Fifteen", midend_which_game(me)->name))
                state = 'h'; /* hint */
            else
                state = CURSOR_SELECT;
        }
        break;

    default:
        break;
    }

    if(debug_settings.shortcuts)
    {
        static bool shortcuts_ok = true;
        switch(button)
        {
        case BTN_LEFT | BTN_FIRE:
            if(shortcuts_ok)
                midend_process_key(me, 0, 0, 'u');
            shortcuts_ok = false;
            break;
        case BTN_RIGHT | BTN_FIRE:
            if(shortcuts_ok)
                midend_process_key(me, 0, 0, 'r');
            shortcuts_ok = false;
            break;
        case 0:
            shortcuts_ok = true;
            break;
        default:
            break;
        }
    }

    LOGF("process_input done");
    LOGF("------------------");
    return state;
}

static void zoom_clamp_panning(void) {
    if(zoom_y < 0)
        zoom_y = 0;
    if(zoom_x < 0)
        zoom_x = 0;

    if(zoom_y + LCD_HEIGHT >= zoom_h)
        zoom_y = zoom_h - LCD_HEIGHT;
    if(zoom_x + LCD_WIDTH >= zoom_w)
        zoom_x = zoom_w - LCD_WIDTH;
}

static bool point_in_rect(int px, int py,
                          int rx, int ry,
                          int rw, int rh) {
    return (rx <= px && px < rx + rw) && (ry <= py && py < ry + rh);
}


static void zoom_center_on_cursor(void) {
    /* get cursor bounding rectangle */
    int x, y, w, h;

    midend_get_cursor_location(me, &x, &y, &w, &h);

    /* no cursor */
    if(x < 0)
        return;

    /* check if either of the top-left and bottom-right corners are
     * off-screen */
    bool off_screen = (!point_in_rect(x, y,         zoom_x, zoom_y, LCD_WIDTH, LCD_HEIGHT) ||
                       !point_in_rect(x + w, y + h, zoom_x, zoom_y, LCD_WIDTH, LCD_HEIGHT));

    if(off_screen || zoom_force_center)
    {
        /* if so, recenter */
        int cx, cy;
        cx = x + w / 2;
        cy = y + h / 2;

        bool x_pan = x < zoom_x || zoom_x + LCD_WIDTH <= x + w;
        if(x_pan || zoom_force_center)
            zoom_x = cx - LCD_WIDTH / 2;

        bool y_pan = y < zoom_y || zoom_y + LCD_HEIGHT <= y + h;
        if(y_pan || zoom_force_center)
            zoom_y = cy - LCD_HEIGHT / 2;

        zoom_clamp_panning();
    }
}

/* This function handles zoom mode, where the user can either pan
 * around a zoomed-in image or play a zoomed-in version of the game. */
static void zoom(void)
{
    rb->splash(0, "Please wait...");
    zoom_w = LCD_WIDTH * ZOOM_FACTOR, zoom_h = LCD_HEIGHT * ZOOM_FACTOR;

    zoom_clipu = 0;
    zoom_clipd = zoom_h;
    zoom_clipl = 0;
    zoom_clipr = zoom_w;

    midend_size(me, &zoom_w, &zoom_h, true);

    /* Allocating the framebuffer will mostly likely grab the
     * audiobuffer, which will make impossible to load new fonts, and
     * lead to zoomed puzzles being drawn with the default fallback
     * fonts. As a semi-workaround, we go ahead and load the biggest available
     * monospace and proportional fonts. */
    rb_setfont(FONT_FIXED, BUNDLE_MAX);
    rb_setfont(FONT_VARIABLE, BUNDLE_MAX);

    zoom_fb = smalloc(zoom_w * zoom_h * sizeof(fb_data));
    if(!zoom_fb)
    {
        rb->splash(HZ, "Out of memory. Cannot zoom in.");
        return;
    }

    /* set position */

    if(zoom_x < 0) {
        /* first run */
        zoom_x = zoom_w / 2 - LCD_WIDTH / 2;
        zoom_y = zoom_h / 2 - LCD_HEIGHT / 2;
    }

    zoom_clamp_panning();

    zoom_enabled = true;

    /* draws go to the zoom framebuffer */
    midend_force_redraw(me);

    rb->lcd_bitmap_part(zoom_fb, zoom_x, zoom_y, STRIDE(SCREEN_MAIN, zoom_w, zoom_h),
                        0, 0, LCD_WIDTH, LCD_HEIGHT);

    draw_title(false); /* false since we don't want to use more screen space than we need. */

    rb->lcd_update();

    /* Here's how this works: pressing select (or the target's
     * equivalent, it's whatever BTN_FIRE is) while in viewing mode
     * will toggle the mode to interaction mode. In interaction mode,
     * the buttons will behave as normal and be sent to the puzzle,
     * except for the pause/quit (BTN_PAUSE) button, which will return
     * to view mode. Finally, when in view mode, pause/quit will
     * return to the pause menu. */

    view_mode = true;

    /* pan around the image */
    while(1)
    {
        if(view_mode)
        {
            int button = rb->button_get_w_tmo(timer_on ? TIMER_INTERVAL : -1);

            exit_on_usb(button);

            switch(button)
            {
            case BTN_UP:
            case BTN_UP | BUTTON_REPEAT:
                zoom_y -= PAN_Y; /* clamped later */
                break;
            case BTN_DOWN:
            case BTN_DOWN | BUTTON_REPEAT:
                zoom_y += PAN_Y; /* clamped later */
                break;
            case BTN_LEFT:
            case BTN_LEFT | BUTTON_REPEAT:
                zoom_x -= PAN_X; /* clamped later */
                break;
            case BTN_RIGHT:
            case BTN_RIGHT | BUTTON_REPEAT:
                zoom_x += PAN_X; /* clamped later */
                break;
            case BTN_PAUSE:
                zoom_enabled = false;
                sfree(zoom_fb);
                fix_size();
                return;
            case BTN_FIRE | BUTTON_REL:
                /* state change to interaction mode */
                view_mode = false;
                break;
            default:
                break;
            }

            zoom_clamp_panning();

            if(timer_on)
                timer_cb();

            /* goes to zoom_fb */
            midend_redraw(me);

            rb->lcd_bitmap_part(zoom_fb, zoom_x, zoom_y, STRIDE(SCREEN_MAIN, zoom_w, zoom_h),
                                0, 0, LCD_WIDTH, LCD_HEIGHT);
            draw_title(false);
            rb->lcd_update();
            rb->yield();
        }
        else
        {
            /* The cursor is always in screenspace coordinates; when
             * zoomed, this means the mouse is always restricted to
             * the bounds of the physical display, not the virtual
             * zoom framebuffer. */
            if(mouse_mode)
                draw_mouse();

            rb->lcd_update();

            if(mouse_mode)
                clear_mouse();

            /* basically a copy-pasta'd main loop */
            int button = process_input(timer_on ? TIMER_INTERVAL : -1, false);

            exit_on_usb(button);

            if(button < 0)
            {
                view_mode = true;
                continue;
            }

            if(button)
                midend_process_key(me, 0, 0, button);

            if(timer_on)
                timer_cb();

            zoom_center_on_cursor();

            midend_redraw(me);

            /* blit */
            rb->lcd_bitmap_part(zoom_fb, zoom_x, zoom_y, STRIDE(SCREEN_MAIN, zoom_w, zoom_h),
                                0, 0, LCD_WIDTH, LCD_HEIGHT);

            draw_title(false);

            rb->yield();
        }
    }
}

/** settings/preset code */

static const char *config_choices_formatter(int sel, void *data, char *buf, size_t len)
{
    /* we can't rely on being called in any particular order */
    char *list = dupstr(data);
    char delimbuf[2] = { *list, 0 };
    char *save = NULL;
    char *str = rb->strtok_r(list, delimbuf, &save);
    for(int i = 0; i < sel; ++i)
        str = rb->strtok_r(NULL, delimbuf, &save);
    rb->snprintf(buf, len, "%s", str);
    sfree(list);
    return buf;
}

static int list_choose(const char *list_str, const char *title, int sel)
{
    char delim = *list_str;

    const char *ptr = list_str + 1;
    int n = 0;
    while(ptr)
    {
        n++;
        ptr = strchr(ptr + 1, delim);
    }

    struct gui_synclist list;

    rb->gui_synclist_init(&list, &config_choices_formatter, (void*)list_str, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, n);
    rb->gui_synclist_limit_scroll(&list, false);

    rb->gui_synclist_select_item(&list, sel);

    rb->gui_synclist_set_title(&list, (char*)title, NOICON);
    while (1)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if(rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;
        switch(button)
        {
        case ACTION_STD_OK:
            return rb->gui_synclist_get_sel_pos(&list);
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
            return -1;
        default:
            break;
        }
    }
}

static bool is_integer(const char *str)
{
    while(*str)
    {
        char c = *str++;
        if(!isdigit(c) && c != '-')
            return false;
    }
    return true;
}

static void int_chooser(config_item *cfgs, int idx, int val)
{
    config_item *cfg = cfgs + idx;
    int old_val = val;

    rb->snprintf(cfg->u.string.sval, MAX_STRLEN, "%d", val);

    rb->lcd_clear_display();

    while(1)
    {
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_puts(0, 0, cfg->name);
        rb->lcd_putsf(0, 1, "< %d >", val);
        rb->lcd_update();
        rb->lcd_set_foreground(ERROR_COLOR);

        int d = 0;
        int button = rb->button_get(true);
        switch(button)
        {
        case BTN_RIGHT:
        case BTN_RIGHT | BUTTON_REPEAT:
            d = 1;
            break;
        case BTN_LEFT:
        case BTN_LEFT | BUTTON_REPEAT:
            d = -1;
            break;
        case BTN_FIRE:
            /* config is already set */
            rb->lcd_scroll_stop();
            return;
        case BTN_PAUSE:
            if(val != old_val)
                rb->splash(HZ, "Canceled.");
            val = old_val;
            rb->snprintf(cfg->u.string.sval, MAX_STRLEN, "%d", val);
            rb->lcd_scroll_stop();
            return;
        }
        if(d)
        {

            const char *ret;
            for(int i = 0; i < CHOOSER_MAX_INCR; ++i)
            {
                val += d;
                rb->snprintf(cfg->u.string.sval, MAX_STRLEN, "%d", val);
                ret = midend_set_config(me, CFG_SETTINGS, cfgs);
                if(!ret)
                {
                    /* clear any error message */
                    rb->lcd_clear_display();
                    rb->lcd_scroll_stop();
                    break;
                }

            }

            /* failure */
            if(ret)
            {
                /* bright, annoying red */
                rb->lcd_set_foreground(ERROR_COLOR);
                rb->lcd_puts_scroll(0, 2, ret);

                /* reset value */
                val -= d * CHOOSER_MAX_INCR;
                rb->snprintf(cfg->u.string.sval, MAX_STRLEN, "%d", val);
                assert(!midend_set_config(me, CFG_SETTINGS, cfgs));
            }
        }
    }
}

/* return value is only meaningful when type == C_STRING, where it
 * indicates whether cfg->sval has been freed or otherwise altered */
static bool do_configure_item(config_item *cfgs, int idx)
{
    config_item *cfg = cfgs + idx;
    switch(cfg->type)
    {
    case C_STRING:
    {
        char *newstr = smalloc(MAX_STRLEN);

        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_set_background(LCD_BLACK);

        if(is_integer(cfg->u.string.sval))
        {
            int val = atoi(cfg->u.string.sval);

            /* we now free the original string and give int_chooser()
             * a clean buffer to work with */
            sfree(cfg->u.string.sval);
            cfg->u.string.sval = newstr;

            int_chooser(cfgs, idx, val);

            rb->lcd_set_foreground(LCD_WHITE);
            rb->lcd_set_background(LCD_BLACK);

            return true;
        }

        rb->strlcpy(newstr, cfg->u.string.sval, MAX_STRLEN);
        if(rb->kbd_input(newstr, MAX_STRLEN, NULL) < 0)
        {
            sfree(newstr);
            return false;
        }
        sfree(cfg->u.string.sval);
        cfg->u.string.sval = newstr;
        return true;
    }
    case C_BOOLEAN:
    {
        bool res = cfg->u.boolean.bval != 0;
        rb->set_bool(cfg->name, &res);

        /* seems to reset backdrop */
        rb->lcd_set_backdrop(NULL);

        cfg->u.boolean.bval = res;
        break;
    }
    case C_CHOICES:
    {
        int sel = list_choose(cfg->u.choices.choicenames, cfg->name, cfg->u.choices.selected);
        if(sel >= 0)
            cfg->u.choices.selected = sel;
        break;
    }
    default:
        fatal("");
        break;
    }
    return false;
}

const char *config_formatter(int sel, void *data, char *buf, size_t len)
{
    config_item *cfg = data;
    cfg += sel;
    rb->snprintf(buf, len, "%s", cfg->name);
    return buf;
}

static bool config_menu(void)
{
    char *title;
    config_item *config = midend_get_config(me, CFG_SETTINGS, &title);

    rb->lcd_setfont(cur_font = FONT_UI);

    bool success = false;

    if(!config)
    {
        goto done;
    }

    /* count */
    int n = 0;
    config_item *ptr = config;
    while(ptr->type != C_END)
    {
        n++;
        ptr++;
    }

    /* display a list */
    struct gui_synclist list;

    rb->gui_synclist_init(&list, &config_formatter, config, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, n);
    rb->gui_synclist_limit_scroll(&list, false);

    rb->gui_synclist_select_item(&list, 0);

    bool done = false;
    rb->gui_synclist_set_title(&list, title, NOICON);
    while (!done)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if(rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;
        switch(button)
        {
        case ACTION_STD_OK:
        {
            int pos = rb->gui_synclist_get_sel_pos(&list);

            /* store the initial state */
            config_item old;
            memcpy(&old, config + pos, sizeof(old));

            char *old_str = NULL;
            if(old.type == C_STRING)
                old_str = dupstr(old.u.string.sval);

            bool freed_str = do_configure_item(config, pos);
            const char *err = midend_set_config(me, CFG_SETTINGS, config);

            if(err)
            {
                rb->splash(HZ, err);

                /* restore old state */
                memcpy(config + pos, &old, sizeof(old));

                if(old.type == C_STRING && freed_str)
                    config[pos].u.string.sval = old_str;
            }
            else
            {
                if(old.type == C_STRING)
                {
                    /* success, and we duplicated the old string when
                     * we didn't need to, so free it now */
                    sfree(old_str);
                }
                success = true;
            }
            break;
        }
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
            done = true;
            break;
        default:
            break;
        }
    }

done:
    sfree(title);
    free_cfg(config);
    return success;
}

static const char *preset_formatter(int sel, void *data, char *buf, size_t len)
{
    struct preset_menu *menu = data;
    rb->snprintf(buf, len, "%s", menu->entries[sel].title);
    return buf;
}

/* main worker function */
/* returns the index of the selected item on success, -1 on failure */
static int do_preset_menu(struct preset_menu *menu, char *title, int selected)
{
    if(!menu->n_entries)
        return false;

    /* display a list */
    struct gui_synclist list;

    rb->gui_synclist_init(&list, &preset_formatter, menu, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, menu->n_entries);
    rb->gui_synclist_limit_scroll(&list, false);

    rb->gui_synclist_select_item(&list, selected);

    static char def[] = "Game Type";
    rb->gui_synclist_set_title(&list, title ? title : def, NOICON);
    while(1)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if(rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;
        switch(button)
        {
        case ACTION_STD_OK:
        {
            int sel = rb->gui_synclist_get_sel_pos(&list);
            struct preset_menu_entry *entry = menu->entries + sel;
            if(entry->params)
            {
                midend_set_params(me, entry->params);
                return sel;
            }
            else
            {
                /* recurse */
                if(do_preset_menu(entry->submenu, entry->title, 0)) /* select first one */
                    return sel;
            }
            break;
        }
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
            return -1;
        default:
            break;
        }
    }
}

/* Let user choose from game presets. Returns true if the user chooses
 * one (in which case the caller should start a new game. */
static bool presets_menu(void)
{
    /* figure out the index of the current preset
     * if it's in a submenu, give up and default to the first item */
    struct preset_menu *top = midend_get_presets(me, NULL);
    int sel = 0;
    for(int i = 0; i < top->n_entries; ++i)
    {
        if(top->entries[i].id == midend_which_preset(me))
        {
            sel = i;
            break;
        }
    }

    return do_preset_menu(midend_get_presets(me, NULL), NULL, sel) >= 0;
}

static void quick_help(void)
{
#if defined(FOR_REAL) && defined(DEBUG_MENU)
    if(++help_times >= 5)
    {
        rb->splash(HZ, "You are now a developer!");
        debug_mode = true;
    }
#endif

    rb->splash(0, quick_help_text);
    rb->button_get(true);
    return;
}

static void full_help(const char *name)
{
    unsigned old_bg = rb->lcd_get_background();

    bool orig_clipped = clipped;
    if(orig_clipped)
        rb_unclip(NULL);

    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    unload_fonts();
    rb->lcd_setfont(cur_font = FONT_UI);

    /* The help text is stored in compressed format in the help_text[]
     * array. display_text wants an array of pointers to
     * null-terminated words, so we create that here. */
    char *buf = smalloc(help_text_len);
    LZ4_decompress_tiny(help_text, buf, help_text_len);

    /* create the word_ptrs array to pass to display_text */
    char **word_ptrs = smalloc(sizeof(char*) * help_text_words);
    char **ptr = word_ptrs;
    bool last_was_null = false;

    *ptr++ = buf;

    for(int i = 1; i < help_text_len; ++i)
    {
        switch(buf[i])
        {
        case '\0':
            if(last_was_null)
            {
                /* newline */
                *ptr++ = buf + i;
            }
            else
                last_was_null = true;
            break;
        default:
            if(last_was_null)
                *ptr++ = buf + i;
            last_was_null = false;
            break;
        }
    }

    display_text(help_text_words, word_ptrs, help_text_style, NULL, true);

    sfree(buf);
    sfree(word_ptrs);

    rb->lcd_set_background(old_bg);

    if(orig_clipped)
        rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);
}

static void init_default_settings(void)
{
    debug_settings.slowmo_factor = 1;
    debug_settings.timerflash = false;
    debug_settings.clipoff = false;
    debug_settings.shortcuts = false;
    debug_settings.no_aa = false;
    debug_settings.polyanim = false;
    debug_settings.highlight_cursor = false;
}

#ifdef DEBUG_MENU
/* Useless debug code. Mostly a waste of space. */
static void bench_aa(void)
{
    rb->sleep(0);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    int next = *rb->current_tick + HZ;
    int i = 0;
    while(*rb->current_tick < next)
    {
        draw_antialiased_line(lcd_fb, LCD_WIDTH, LCD_HEIGHT, 0, 0, 20, 31);
        ++i;
    }
    rb->splashf(HZ, "%d AA lines/sec", i);
    next = *rb->current_tick + HZ;
    int j = 0;
    while(*rb->current_tick < next)
    {
        rb->lcd_drawline(0, 0, 20, 31);
        ++j;
    }
    rb->splashf(HZ, "%d normal lines/sec", j);
    rb->splashf(HZ, "Efficiency: %d%%", 100 * i / j);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

static void debug_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Debug Menu", NULL,
                        "Slowmo factor",
                        "Randomize colors",
                        "Toggle flash pixel on timer",
                        "Toggle clip",
                        "Toggle shortcuts",
                        "Toggle antialias",
                        "Benchmark antialias",
                        "Toggle show poly steps",
                        "Toggle mouse mode",
                        "Toggle spacebar on long click",
                        "Toggle send keys on release",
                        "Toggle ignore repeats",
                        "Toggle right-click on hold vs. dragging",
                        "Toggle highlight cursor region",
                        "Toggle force zoom on center",
                        "Back");
    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rb->set_int("Slowmo factor", "", UNIT_INT, &debug_settings.slowmo_factor, NULL, 1, 1, 15, NULL);
            break;
        case 1:
        {
            unsigned *ptr = colors;
            for(int i = 0; i < ncolors; ++i)
            {
                /* not seeded, who cares? */
                *ptr++ = LCD_RGBPACK(rb->rand()%255, rb->rand()%255, rb->rand()%255);
            }
            break;
        }
        case 2:
            debug_settings.timerflash ^= true;
            break;
        case 3:
            debug_settings.clipoff ^= true;
            break;
        case 4:
            debug_settings.shortcuts ^= true;
            break;
        case 5:
            debug_settings.no_aa ^= true;
            break;
        case 6:
            bench_aa();
            break;
        case 7:
            debug_settings.polyanim ^= true;
            break;
        case 8:
            mouse_mode ^= true;
            break;
        case 9:
            input_settings.want_spacebar ^= true;
            break;
        case 10:
            input_settings.falling_edge ^= true;
            break;
        case 11:
            input_settings.ignore_repeats ^= true;
            break;
        case 12:
            input_settings.rclick_on_hold ^= true;
            break;
        case 13:
            debug_settings.highlight_cursor ^= true;
            break;
        case 14:
            zoom_force_center ^= true;
            break;
        default:
            quit = true;
            break;
        }
    }
}
#endif

static int pausemenu_cb(int action,
                        const struct menu_item_ex *this_item,
                        struct gui_synclist *this_list)
{
    (void)this_list;
    int i = (intptr_t) this_item;
    if(action == ACTION_REQUEST_MENUITEM)
    {
        switch(i)
        {
        case 3:
            if(!midend_can_undo(me))
                return ACTION_EXIT_MENUITEM;
            break;
        case 4:
            if(!midend_can_redo(me))
                return ACTION_EXIT_MENUITEM;
            break;
        case 5:
            if(!midend_which_game(me)->can_solve)
                return ACTION_EXIT_MENUITEM;
            break;
        case 9:
            if(audiobuf_available)
                break;
            else
                return ACTION_EXIT_MENUITEM;
        case 10:
            if(!midend_get_presets(me, NULL)->n_entries)
                return ACTION_EXIT_MENUITEM;
            break;
        case 11:
#if defined(FOR_REAL) && defined(DEBUG_MENU)
            if(debug_mode)
                break;
            return ACTION_EXIT_MENUITEM;
#else
            break;
#endif
        case 12:
            if(!midend_which_game(me)->can_configure)
                return ACTION_EXIT_MENUITEM;
            break;
        default:
            break;
        }
    }
    return action;
}

static void clear_and_draw(void)
{
    rb->lcd_clear_display();

    midend_force_redraw(me);

    draw_title(true);

    if(mouse_mode)
        draw_mouse();

    rb->lcd_update();

    if(mouse_mode)
        clear_mouse();
}

static void reset_drawing(void)
{
    rb->lcd_set_viewport(NULL);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(BG_COLOR);
}

/* Make a new game, but tell the user through a splash so they don't
 * think we're locked up. Also performs new-game initialization
 * specific to Rockbox. */
static void new_game_notify(void)
{
    rb->splash(0, "Please wait...");
    midend_new_game(me);
    fix_size();
    rb->lcd_update();
}

static int pause_menu(void)
{
    MENUITEM_STRINGLIST(menu, menu_desc, pausemenu_cb,
                        "Resume Game",         // 0
                        "New Game",            // 1
                        "Restart Game",        // 2
                        "Undo",                // 3
                        "Redo",                // 4
                        "Solve",               // 5
                        "Zoom In",             // 6
                        "Quick Help",          // 7
                        "Extensive Help",      // 8
                        "Playback Control",    // 9
                        "Game Type",           // 10
                        "Debug Menu",          // 11
                        "Configure Game",      // 12
                        "Quit without Saving", // 13
                        "Quit");               // 14

#if defined(FOR_REAL) && defined(DEBUG_MENU)
    help_times = 0;
#endif

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            quit = true;
            break;
        case 1:
            new_game_notify();
            quit = true;
            break;
        case 2:
            midend_restart_game(me);
            fix_size();
            quit = true;
            break;
        case 3:
            if(!midend_can_undo(me))
                rb->splash(HZ, "Cannot undo.");
            else
                midend_process_key(me, 0, 0, 'u');
            quit = true;
            break;
        case 4:
            if(!midend_can_redo(me))
                rb->splash(HZ, "Cannot redo.");
            else
                midend_process_key(me, 0, 0, 'r');
            quit = true;
            break;
        case 5:
        {
            const char *msg = midend_solve(me);
            if(msg)
                rb->splash(HZ, msg);
            quit = true;
            break;
        }
        case 6:
            zoom();
            break;
        case 7:
            quick_help();
            break;
        case 8:
            full_help(midend_which_game(me)->name);
            break;
        case 9:
            playback_control(NULL);
            break;
        case 10:
            if(presets_menu())
            {
                new_game_notify();
                reset_drawing();
                clear_and_draw();
                quit = true;
            }
            break;
        case 11:
#ifdef DEBUG_MENU
            debug_menu();
#endif
            break;
        case 12:
            if(config_menu())
            {
                new_game_notify();
                reset_drawing();
                clear_and_draw();
                quit = true;
            }
            break;
        case 13:
            return -2;
        case 14:
            return -3;
        default:
            break;
        }
    }
    rb->lcd_set_background(BG_COLOR);
    rb->lcd_clear_display();
    midend_force_redraw(me);
    rb->lcd_update();
    return 0;
}

/* points to pluginbuf, used by rbmalloc.c */
/* useless side note: originally giant_buffer was a statically
 * allocated giant array (4096KB IIRC), hence its name. */
char *giant_buffer = NULL;
static size_t giant_buffer_len = 0; /* set on start */

static void fix_size(void)
{
    int w = LCD_WIDTH, h = LCD_HEIGHT, h_x;
    rb->lcd_setfont(cur_font = FONT_UI);
    rb->lcd_getstringsize("X", NULL, &h_x);
    h -= h_x;
    midend_size(me, &w, &h, true);
}

static void init_tlsf(void)
{
    /* reset tlsf by nuking the signature */
    /* will make any already-allocated memory point to garbage */
    memset(giant_buffer, 0, 4);
    init_memory_pool(giant_buffer_len, giant_buffer);
}

static bool read_wrapper(void *ptr, void *buf, int len)
{
    int fd = (int) ptr;
    return rb->read(fd, buf, len) == len;
}

static void write_wrapper(void *ptr, const void *buf, int len)
{
    int fd = (int) ptr;
    rb->write(fd, buf, len);
}

static void init_colors(void)
{
    float *floatcolors = midend_colors(me, &ncolors);

    /* convert them to packed RGB */
    colors = smalloc(ncolors * sizeof(unsigned));
    unsigned *ptr = colors;
    float *floatptr = floatcolors;
    for(int i = 0; i < ncolors; ++i)
    {
        int r = 255 * *(floatptr++);
        int g = 255 * *(floatptr++);
        int b = 255 * *(floatptr++);
        LOGF("color %d is %d %d %d", i, r, g, b);
        *ptr++ = LCD_RGBPACK(r, g, b);
    }
    sfree(floatcolors);
}

static bool string_in_list(const char *target, const char **list)
{
    /* list is terminated with NULL */
    const char *i;

    while((i = *list++))
    {
        if(!strcmp(target, i))
            return true;
    }

    return false;
}

/* this function sets game-specific input settings */
static void tune_input(const char *name)
{
    static const char *want_spacebar[] = {
        "Magnets",
        "Map",
        "Mines",
        "Palisade",
        "Rectangles",
        NULL
    };

    /* these get a spacebar on long click - you must also add to the
     * falling_edge list below! */
    input_settings.want_spacebar = string_in_list(name, want_spacebar);

    static const char *falling_edge[] = {
        "Inertia",
        "Magnets",
        "Map",
        "Mines",
        "Palisade",
        "Rectangles",
        NULL
    };

    /* wait until a key is released to send an action (useful for
     * chording in Inertia; must be enabled if the game needs a
     * spacebar) */
    input_settings.falling_edge = string_in_list(name, falling_edge);

    /* For want_spacebar to work, events must be sent on the falling
     * edge */
    assert(!(input_settings.want_spacebar && !input_settings.falling_edge));

    /* ignore repeated keypresses in all games but untangle (mouse
     * mode overrides this no matter what) */
    static const char *allow_repeats[] = {
        "Untangle",
        NULL
    };

    input_settings.ignore_repeats = !string_in_list(name, allow_repeats);

    /* disable right-click on hold if you want dragging in mouse
     * mode */
    static const char *no_rclick_on_hold[] = {
        "Map",
        "Signpost",
        "Untangle",
        NULL
    };

    input_settings.rclick_on_hold = !string_in_list(name, no_rclick_on_hold);

    static const char *mouse_games[] = {
        "Loopy",
        NULL
    };

    mouse_mode = string_in_list(name, mouse_games);

    static const char *number_chooser_games[] = {
        "Filling",
        "Keen",
        "Solo",
        "Towers",
        "Undead",
        "Unequal",
        NULL
    };

    input_settings.numerical_chooser = string_in_list(name, number_chooser_games);

    static const char *force_center_games[] = {
        "Inertia",
        NULL
    };
    zoom_force_center = string_in_list(name, force_center_games);
}

static const char *init_for_game(const game *gm, int load_fd)
{
    me = midend_new(NULL, gm, &rb_drawing, NULL);

    if(load_fd < 0)
    {
        new_game_notify();
    }
    else
    {
        const char *ret = midend_deserialize(me, read_wrapper, (void*) load_fd);
        if(ret)
            return ret;
    }

    fix_size();

    tune_input(gm->name);

    mouse_x = LCD_WIDTH / 2;
    mouse_y = LCD_HEIGHT / 2;

    init_colors();

    reset_drawing();

    return NULL;
}

static void shutdown_tlsf(void)
{
    memset(giant_buffer, 0, 4);
}

static void exit_handler(void)
{
#ifdef HAVE_SW_POWEROFF
    sw_poweroff_restore();
#endif

    unload_fonts();
    shutdown_tlsf();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

#ifdef FONT_CACHING
/* try loading the fonts indicated in the on-disk font table */
static void load_fonts(void)
{
    LOGF("loading cached fonts from disk");
    int fd = rb->open(FONT_TABLE, O_RDONLY);
    if(fd < 0)
        return;

    uint64_t fontmask = 0;
    while(1)
    {
        char linebuf[MAX_LINE], *ptr = linebuf;
        int len = rb->read_line(fd, linebuf, sizeof(linebuf));
        if(len <= 0)
            break;

        char *tok, *save;
        tok = rb->strtok_r(ptr, ":", &save);
        ptr = NULL;

        if(!strcmp(tok, midend_which_game(me)->name))
        {
            LOGF("successfully found game in table");
            uint32_t left, right;
            tok = rb->strtok_r(ptr, ":", &save);
            left = atoi(tok);
            tok = rb->strtok_r(ptr, ":", &save);
            right = atoi(tok);
            fontmask = ((uint64_t)left << 31) | right;
            break;
        }
    }

    /* nothing to do */
    if(!fontmask)
    {
        rb->close(fd);
        return;
    }

    /* loop through each bit of the mask and try loading the
       corresponding font */
    for(int i = 0; i < 2 * BUNDLE_COUNT; ++i)
    {
        if(fontmask & ((uint64_t)1 << i))
        {
            int size = (i > BUNDLE_COUNT  ? i - BUNDLE_COUNT : i) + BUNDLE_MIN;
            int type = i > BUNDLE_COUNT ? FONT_VARIABLE : FONT_FIXED;

            LOGF("loading font type %d, size %d", type, size);

            rb_setfont(type, size);
        }
    }

    rb->close(fd);
}

/* remember which fonts were loaded */
static void save_fonts(void)
{
#if 2*BUNDLE_COUNT > 62
#error too many fonts for 62-bit mask
#endif

    /* first assemble the bitmask */
    uint64_t fontmask = 0;
    for(int i = 0; i < 2 * BUNDLE_COUNT; ++i)
    {
        /* try loading if we attempted to load */
        if(loaded_fonts[i].status >= -2)
        {
            fontmask |= (uint64_t)1 << i;
        }
    }

    if(fontmask)
    {
        /* font table format is as follows:
         * [GAME NAME]:[32-halves of bit mask in decimal][newline]
         */
        int fd = rb->open(FONT_TABLE, O_RDONLY);
        int outfd = rb->open(FONT_TABLE ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(outfd < 0)
            return;

        uint64_t oldmask = 0;

        if(fd >= 0)
        {
            while(1)
            {
                char linebuf[MAX_LINE], *ptr = linebuf;
                char origbuf[MAX_LINE];
                int len = rb->read_line(fd, linebuf, sizeof(linebuf));
                if(len <= 0)
                    break;
                rb->memcpy(origbuf, linebuf, len);

                char *tok, *save;
                tok = rb->strtok_r(ptr, ":", &save);
                ptr = NULL;

                /* copy line if not matching */
                if(strcmp(tok, midend_which_game(me)->name) != 0)
                {
                    rb->write(outfd, origbuf, len);
                    rb->fdprintf(outfd, "\n");
                }
                else
                {
                    /* matching, remember the old mask */
                    assert(oldmask == 0);
                    uint32_t left, right;
                    tok = rb->strtok_r(ptr, ":", &save);
                    left = atoi(tok);
                    tok = rb->strtok_r(ptr, ":", &save);
                    right = atoi(tok);
                    oldmask = ((uint64_t)left << 31) | right;
                }
            }
            rb->close(fd);
        }
        uint64_t final = fontmask;
        if(n_fonts < MAX_FONTS)
            final |= oldmask;
        uint32_t left = final >> 31;
        uint32_t right = final & 0x7fffffff;
        rb->fdprintf(outfd, "%s:%u:%u\n", midend_which_game(me)->name, (unsigned)left, (unsigned)right);
        rb->close(outfd);
        rb->rename(FONT_TABLE ".tmp", FONT_TABLE);
    }
}
#endif

static void save_fname(char *buf)
{
    rb->snprintf(buf, MAX_PATH, "%s/sgt-%s.sav", PLUGIN_GAMES_DATA_DIR, thegame.htmlhelp_topic);
}

/* expects a totally free me* pointer */
static bool load_game(void)
{
    char fname[MAX_PATH];
    save_fname(fname);

    int fd = rb->open(fname, O_RDONLY);
    if(fd < 0)
        return false;

    rb->splash(0, "Loading...");

    char *game;
    const char *ret = identify_game(&game, read_wrapper, (void*)fd);

    if(!game && ret)
    {
        rb->splash(HZ, ret);
        rb->close(fd);
        return false;
    }
    else
    {
        /* seek to beginning */
        rb->lseek(fd, 0, SEEK_SET);

        if(!strcmp(game, thegame.name))
        {
            ret = init_for_game(&thegame, fd);
            if(ret)
            {
                rb->splash(HZ, ret);
                rb->close(fd);
                rb->remove(fname);
                return false;
            }
            rb->close(fd);
            rb->remove(fname);

            /* success */
            return true;
        }
        rb->splash(HZ, "Load failed.");

        /* clean up, even on failure */
        rb->close(fd);
        rb->remove(fname);

        return false;
    }
}

static void save_game(void)
{
    rb->splash(0, "Saving...");

    char fname[MAX_PATH];
    save_fname(fname);

    /* save game */
    int fd = rb->open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    midend_serialize(me, write_wrapper, (void*) fd);
    rb->close(fd);

#ifdef FONT_CACHING
    save_fonts();
#endif

    rb->lcd_update();
}

static int mainmenu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = (intptr_t) this_item;
    if(action == ACTION_REQUEST_MENUITEM)
    {
        switch(i)
        {
        case 0:
        case 7:
            if(!load_success)
                return ACTION_EXIT_MENUITEM;
            break;
        case 3:
            break;
        case 4:
            if(audiobuf_available)
                break;
            else
                return ACTION_EXIT_MENUITEM;
        case 5:
            if(!midend_get_presets(me, NULL)->n_entries)
                return ACTION_EXIT_MENUITEM;
            break;
        case 6:
            if(!midend_which_game(me)->can_configure)
                return ACTION_EXIT_MENUITEM;
            break;
        default:
            break;
        }
    }
    return action;
}

static void puzzles_main(void)
{
    rb_atexit(exit_handler);

#ifdef HAVE_SW_POWEROFF
    sw_poweroff_disable();
#endif

    init_default_settings();
    init_fonttab();

    load_success = load_game();

    if(!load_success)
    {
        /* our main menu expects a ready-to-use midend */
        init_for_game(&thegame, -1);
    }

#ifdef FONT_CACHING
    LOGF("loading fonts");
    load_fonts();
#endif

    /* must be done before any menu needs to be displayed */
    rb->snprintf(menu_desc, sizeof(menu_desc), "%s Menu", midend_which_game(me)->name);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* about to go to menu or button block */
    rb->cpu_boost(false);
#endif

#if defined(FOR_REAL) && defined(DEBUG_MENU)
    help_times = 0;
#endif

    MENUITEM_STRINGLIST(menu, menu_desc, mainmenu_cb,
                        "Resume Game",         // 0
                        "New Game",            // 1
                        "Quick Help",          // 2
                        "Extensive Help",      // 3
                        "Playback Control",    // 4
                        "Game Type",           // 5
                        "Configure Game",      // 6
                        "Quit without Saving", // 7
                        "Quit");               // 8

    bool quit = false;
    int sel = 0;

    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            /* Loaded. Run the game! */
            goto game_loop;
        case 1:
            if(!load_success)
            {
                /* Failed to load (so midend is already initialized
                 * with new game) */
                goto game_loop;
            }

            /* Otherwise we need to generate a new game. */
            quit = true;
            break;
        case 2:
            quick_help();
            break;
        case 3:
            full_help(midend_which_game(me)->name);
            break;
        case 4:
            playback_control(NULL);
            break;
        case 5:
            if(presets_menu())
            {
                new_game_notify();
                goto game_loop;
            }
            break;
        case 6:
            if(config_menu())
            {
                new_game_notify();
                goto game_loop;
            }
            break;
        case 8:
            if(load_success)
                save_game();
            /* fall through */
        case 7:
            /* we don't care about freeing anything because tlsf will
             * be wiped out the next time around */
            return;
        default:
            break;
        }
    }

    while(1)
    {
        init_for_game(&thegame, -1);
    game_loop:
        reset_drawing();
        clear_and_draw();

        last_keystate = 0;
        accept_input = true;

        while(1)
        {
            int button = process_input(timer_on ? TIMER_INTERVAL : -1, true);

            /* special codes are < 0 */
            if(button < 0)
            {
                rb_unclip(NULL);
                deactivate_timer(NULL);

                if(titlebar)
                {
                    sfree(titlebar);
                    titlebar = NULL;
                }

                switch(button)
                {
                case -1:
                    /* new game */
                    midend_free(me);
                    break;
                case -2:
                    /* quit without saving */
                    midend_free(me);
                    sfree(colors);
                    return;
                case -3:
                    /* save and quit */
                    save_game();
                    midend_free(me);
                    sfree(colors);
                    return;
                default:
                    break;
                }
            }

            /* we have a game input */
            if(button)
                midend_process_key(me, 0, 0, button);

            if(timer_on)
                timer_cb();

            midend_redraw(me);

            draw_title(true); /* will draw to fb */

            /* Blit mouse but immediately clear it. */
            if(mouse_mode)
                draw_mouse();

            rb->lcd_update();

            if(mouse_mode)
                clear_mouse();

            rb->yield();
        }
        sfree(colors);
    }
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* boost for init */
    rb->cpu_boost(true);
#endif

    giant_buffer = rb->plugin_get_buffer(&giant_buffer_len);
    init_tlsf();
    struct viewport *vp_main = rb->lcd_set_viewport(NULL);
    lcd_fb = vp_main->buffer->fb_ptr;

    if(!strcmp(thegame.name, "Solo"))
    {
        /* Solo needs a big stack */
        int stack_sz = 16 * DEFAULT_STACK_SIZE;
        uintptr_t old = (uintptr_t)smalloc(stack_sz);

        /* word alignment */
        long *stack = (long*)((char*)(((uintptr_t)old & (uintptr_t)(~0x3)) + 4));
        stack_sz -= ((char*)stack - (char*)old);

        thread = rb->create_thread(puzzles_main, stack, stack_sz, 0, "puzzles"
                                   IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
        rb->thread_wait(thread);
    }
    else
        puzzles_main();

    return PLUGIN_OK;
}
