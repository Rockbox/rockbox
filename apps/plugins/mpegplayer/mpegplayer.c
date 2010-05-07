/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mpegplayer main entrypoint and UI implementation
 *
 * Copyright (c) 2007 Michael Sevakis
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

/****************************************************************************
 * NOTES:
 * 
 * mpegplayer is structured as follows:
 * 
 *                       +-->Video Thread-->Video Output-->LCD
 *                       |
 * UI-->Stream Manager-->+-->Audio Thread-->PCM buffer--Audio Device
 *         |       |     |                        |     (ref. clock)
 *         |       |     +-->Buffer Thread        |
 *    Stream Data  |             |          (clock intf./
 *     Requests    |         File Cache      drift adj.)
 *                 |          Disk I/O
 *         Stream services
 *          (timing, etc.)
 * 
 * Thread list:
 *  1) The main thread - Handles user input, settings, basic playback control
 *     and USB connect.
 * 
 *  2) Stream Manager thread - Handles playback state, events from streams
 *     such as when a stream is finished, stream commands, PCM state. The
 *     layer in which this thread run also handles arbitration of data
 *     requests between the streams and the disk buffer. The actual specific
 *     transport layer code may get moved out to support multiple container
 *     formats.
 * 
 *  3) Buffer thread - Buffers data in the background, generates notifications
 *     to streams when their data has been buffered, and watches streams'
 *     progress to keep data available during playback. Handles synchronous
 *     random access requests when the file cache is missed.
 * 
 *  4) Video thread (running on the COP for PortalPlayer targets) - Decodes
 *     the video stream and renders video frames to the LCD. Handles
 *     miscellaneous video tasks like frame and thumbnail printing.
 * 
 *  5) Audio thread (running on the main CPU to maintain consistency with the
 *     audio FIQ hander on PP) - Decodes audio frames and places them into
 *     the PCM buffer for rendering by the audio device.
 * 
 * Streams are neither aware of one another nor care about one another. All
 * streams shall have their own thread (unless it is _really_ efficient to
 * have a single thread handle a couple minor streams). All coordination of
 * the streams is done through the stream manager. The clocking is controlled
 * by and exposed by the stream manager to other streams and implemented at
 * the PCM level.
 * 
 * Notes about MPEG files:
 * 
 * MPEG System Clock is 27MHz - i.e. 27000000 ticks/second.
 * 
 * FPS is represented in terms of a frame period - this is always an
 * integer number of 27MHz ticks.
 * 
 * e.g. 29.97fps (30000/1001) NTSC video has an exact frame period of
 * 900900 27MHz ticks.
 * 
 * In libmpeg2, info->sequence->frame_period contains the frame_period.
 * 
 * Working with Rockbox's 100Hz tick, the common frame rates would need
 * to be as follows (1):
 * 
 * FPS     | 27Mhz   | 100Hz          | 44.1KHz   | 48KHz
 * --------|-----------------------------------------------------------
 * 10*     | 2700000 | 10             | 4410      | 4800
 * 12*     | 2250000 |  8.3333        | 3675      | 4000
 * 15*     | 1800000 |  6.6667        | 2940      | 3200
 * 23.9760 | 1126125 |  4.170833333   | 1839.3375 | 2002
 * 24      | 1125000 |  4.166667      | 1837.5    | 2000
 * 25      | 1080000 |  4             | 1764      | 1920
 * 29.9700 |  900900 |  3.336667      | 1471,47   | 1601.6
 * 30      |  900000 |  3.333333      | 1470      | 1600
 * 
 * *Unofficial framerates
 * 
 * (1) But we don't really care since the audio clock is used anyway and has
 *     very fine resolution ;-)
 *****************************************************************************/
#include "plugin.h"
#include "mpegplayer.h"
#include "lib/helper.h"
#include "mpeg_settings.h"
#include "mpeg2.h"
#include "video_out.h"
#include "stream_thread.h"
#include "stream_mgr.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_MENU       BUTTON_MODE
#define MPEG_STOP       BUTTON_OFF
#define MPEG_PAUSE      BUTTON_ON
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MPEG_MENU       BUTTON_MENU
#define MPEG_PAUSE      (BUTTON_PLAY | BUTTON_REL)
#define MPEG_STOP       (BUTTON_PLAY | BUTTON_REPEAT)
#define MPEG_VOLDOWN    BUTTON_SCROLL_BACK
#define MPEG_VOLUP      BUTTON_SCROLL_FWD
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define MPEG_MENU       (BUTTON_REC | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_SELECT
#define MPEG_PAUSE2     BUTTON_A
#define MPEG_VOLDOWN    BUTTON_LEFT
#define MPEG_VOLUP      BUTTON_RIGHT
#define MPEG_VOLDOWN2   BUTTON_VOL_DOWN
#define MPEG_VOLUP2     BUTTON_VOL_UP
#define MPEG_RW         BUTTON_UP
#define MPEG_FF         BUTTON_DOWN

#define MPEG_RC_MENU    BUTTON_RC_DSP
#define MPEG_RC_STOP    (BUTTON_RC_PLAY | BUTTON_REPEAT)
#define MPEG_RC_PAUSE   (BUTTON_RC_PLAY | BUTTON_REL)
#define MPEG_RC_VOLDOWN BUTTON_RC_VOL_DOWN
#define MPEG_RC_VOLUP   BUTTON_RC_VOL_UP
#define MPEG_RC_RW      BUTTON_RC_REW
#define MPEG_RC_FF      BUTTON_RC_FF

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_SELECT
#define MPEG_PAUSE2     BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_LEFT
#define MPEG_VOLUP      BUTTON_RIGHT
#define MPEG_VOLDOWN2   BUTTON_VOL_DOWN
#define MPEG_VOLUP2     BUTTON_VOL_UP
#define MPEG_RW         BUTTON_UP
#define MPEG_RW2        BUTTON_PREV
#define MPEG_FF         BUTTON_DOWN
#define MPEG_FF2        BUTTON_NEXT
#define MPEG_SHOW_OSD   BUTTON_BACK

#define MPEG_RC_MENU    BUTTON_RC_DSP
#define MPEG_RC_STOP    (BUTTON_RC_PLAY | BUTTON_REPEAT)
#define MPEG_RC_PAUSE   (BUTTON_RC_PLAY | BUTTON_REL)
#define MPEG_RC_VOLDOWN BUTTON_RC_VOL_DOWN
#define MPEG_RC_VOLUP   BUTTON_RC_VOL_UP
#define MPEG_RC_RW      BUTTON_RC_REW
#define MPEG_RC_FF      BUTTON_RC_FF

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_MENU       BUTTON_LEFT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_SCROLL_DOWN
#define MPEG_VOLUP      BUTTON_SCROLL_UP
#define MPEG_RW         BUTTON_REW
#define MPEG_FF         BUTTON_FF

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_RIGHT
#define MPEG_VOLDOWN    BUTTON_SCROLL_BACK
#define MPEG_VOLUP      BUTTON_SCROLL_FWD
#define MPEG_RW         BUTTON_UP
#define MPEG_FF         BUTTON_DOWN

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       (BUTTON_HOME|BUTTON_REPEAT)
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_SCROLL_BACK
#define MPEG_VOLUP      BUTTON_SCROLL_FWD
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT


#elif CONFIG_KEYPAD == SANSA_C200_PAD || \
CONFIG_KEYPAD == SANSA_CLIP_PAD || \
CONFIG_KEYPAD == SANSA_M200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == MROBE500_PAD
#define MPEG_STOP       BUTTON_POWER

#define MPEG_RC_MENU    BUTTON_RC_HEART
#define MPEG_RC_STOP    BUTTON_RC_DOWN
#define MPEG_RC_PAUSE   BUTTON_RC_PLAY
#define MPEG_RC_VOLDOWN BUTTON_RC_VOL_DOWN
#define MPEG_RC_VOLUP   BUTTON_RC_VOL_UP
#define MPEG_RC_RW      BUTTON_RC_REW
#define MPEG_RC_FF      BUTTON_RC_FF

#elif CONFIG_KEYPAD == MROBE100_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define MPEG_MENU       BUTTON_RC_MENU
#define MPEG_STOP       BUTTON_RC_REC
#define MPEG_PAUSE      BUTTON_RC_PLAY
#define MPEG_VOLDOWN    BUTTON_RC_VOL_DOWN
#define MPEG_VOLUP      BUTTON_RC_VOL_UP
#define MPEG_RW         BUTTON_RC_REW
#define MPEG_FF         BUTTON_RC_FF

#elif CONFIG_KEYPAD == COWON_D2_PAD
#define MPEG_MENU       (BUTTON_MENU|BUTTON_REL)
//#define MPEG_STOP       BUTTON_POWER
#define MPEG_VOLDOWN    BUTTON_MINUS
#define MPEG_VOLUP      BUTTON_PLUS

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_STOP
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_VOLDOWN
#define MPEG_VOLUP      BUTTON_VOLUP
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_BACK
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_UP
#define MPEG_VOLUP      BUTTON_DOWN
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_SELECT
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP
#define MPEG_RW         BUTTON_UP
#define MPEG_FF         BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define MPEG_MENU       (BUTTON_MENU|BUTTON_REL)
//#define MPEG_STOP       BUTTON_POWER
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP

#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define MPEG_MENU       BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define MPEG_MENU       BUTTON_LEFT
#define MPEG_STOP       BUTTON_RIGHT
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP
#define MPEG_RW         BUTTON_REW
#define MPEG_FF         BUTTON_FFWD

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_REC
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP
#define MPEG_RW         BUTTON_PREV
#define MPEG_FF         BUTTON_NEXT

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_PAUSE      (BUTTON_PLAY | BUTTON_REL)
#define MPEG_STOP       (BUTTON_PLAY | BUTTON_REPEAT)
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP
#define MPEG_RW         BUTTON_PREV
#define MPEG_FF         BUTTON_NEXT

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef MPEG_MENU
#define MPEG_MENU      (BUTTON_TOPRIGHT|BUTTON_REL)
#endif
#ifndef MPEG_STOP
#define MPEG_STOP       BUTTON_TOPLEFT
#endif
#ifndef MPEG_PAUSE
#define MPEG_PAUSE      BUTTON_CENTER
#endif
#ifndef MPEG_VOLDOWN
#define MPEG_VOLDOWN    BUTTON_BOTTOMMIDDLE
#endif
#ifndef MPEG_VOLUP
#define MPEG_VOLUP      BUTTON_TOPMIDDLE
#endif
#ifndef MPEG_RW
#define MPEG_RW         BUTTON_MIDLEFT
#endif
#ifndef MPEG_FF
#define MPEG_FF         BUTTON_MIDRIGHT
#endif
#endif

/* One thing we can do here for targets with remotes is having a display
 * always on the remote instead of always forcing a popup on the main display */

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP (TS_SECOND/2)
#define OSD_MIN_UPDATE_INTERVAL (HZ/2)

/* OSD status - same order as icon array */
enum osd_status_enum
{
    OSD_STATUS_STOPPED = 0,
    OSD_STATUS_PAUSED,
    OSD_STATUS_PLAYING,
    OSD_STATUS_FF,
    OSD_STATUS_RW,
    OSD_STATUS_COUNT,
    OSD_STATUS_MASK = 0x7
};

enum osd_bits
{
    OSD_REFRESH_DEFAULT    = 0x0000, /* Only refresh elements when due */
                                     /* Refresh the... */
    OSD_REFRESH_VOLUME     = 0x0001, /* ...volume display */
    OSD_REFRESH_TIME       = 0x0002, /* ...time display+progress */
    OSD_REFRESH_STATUS     = 0x0004, /* ...playback status icon */
    OSD_REFRESH_BACKGROUND = 0x0008, /* ...background (implies ALL) */
    OSD_REFRESH_VIDEO      = 0x0010, /* ...video image upon timeout */
    OSD_REFRESH_RESUME     = 0x0020, /* Resume playback upon timeout */
    OSD_NODRAW             = 0x8000, /* OR bitflag - don't draw anything */
    OSD_SHOW               = 0x4000, /* OR bitflag - show the OSD */
    OSD_HP_PAUSE           = 0x2000,
    OSD_HIDE               = 0x0000, /* hide the OSD (aid readability) */
    OSD_REFRESH_ALL        = 0x000f, /* Only immediate graphical elements */
};

/* Status icons selected according to font height */
extern const unsigned char mpegplayer_status_icons_8x8x1[];
extern const unsigned char mpegplayer_status_icons_12x12x1[];
extern const unsigned char mpegplayer_status_icons_16x16x1[];

/* Main border areas that contain OSD elements */
#define OSD_BDR_L 2
#define OSD_BDR_T 2
#define OSD_BDR_R 2
#define OSD_BDR_B 2

struct osd
{
    long hide_tick;
    long show_for;
    long print_tick;
    long print_delay;
    long resume_tick;
    long resume_delay;
    long next_auto_refresh;
    int x;
    int y;
    int width;
    int height;
    unsigned fgcolor;
    unsigned bgcolor;
    unsigned prog_fillcolor;
    struct vo_rect update_rect;
    struct vo_rect prog_rect;
    struct vo_rect time_rect;
    struct vo_rect dur_rect;
    struct vo_rect vol_rect;
    const unsigned char *icons;
    struct vo_rect stat_rect;
    int status;
    uint32_t curr_time;
    unsigned auto_refresh;
    unsigned flags;
};

static struct osd osd;

static void osd_show(unsigned show);

#ifdef LCD_LANDSCAPE
    #define _X (x + osd.x)
    #define _Y (y + osd.y)
    #define _W width
    #define _H height
#else
    #define _X (LCD_WIDTH - (y + osd.y) - height)
    #define _Y (x + osd.x)
    #define _W height
    #define _H width
#endif

#ifdef HAVE_LCD_COLOR
/* Blend two colors in 0-100% (0-255) mix of c2 into c1 */
static unsigned draw_blendcolor(unsigned c1, unsigned c2, unsigned char amount)
{
    int r1 = RGB_UNPACK_RED(c1);
    int g1 = RGB_UNPACK_GREEN(c1);
    int b1 = RGB_UNPACK_BLUE(c1);

    int r2 = RGB_UNPACK_RED(c2);
    int g2 = RGB_UNPACK_GREEN(c2);
    int b2 = RGB_UNPACK_BLUE(c2);

    return LCD_RGBPACK(amount*(r2 - r1) / 255 + r1,
                       amount*(g2 - g1) / 255 + g1,
                       amount*(b2 - b1) / 255 + b1);
}
#endif

/* Drawing functions that operate rotated on LCD_PORTRAIT displays -
 * most are just wrappers of lcd_* functions with transforms applied.
 * The origin is the upper-left corner of the OSD area */
static void draw_update_rect(int x, int y, int width, int height)
{
    lcd_(update_rect)(_X, _Y, _W, _H);
}

static void draw_clear_area(int x, int y, int width, int height)
{
#ifdef HAVE_LCD_COLOR
    rb->screen_clear_area(rb->screens[SCREEN_MAIN], _X, _Y, _W, _H);
#else
    int oldmode = grey_get_drawmode();
    grey_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    grey_fillrect(_X, _Y, _W, _H);
    grey_set_drawmode(oldmode);
#endif
}

static void draw_clear_area_rect(const struct vo_rect *rc)
{
    draw_clear_area(rc->l, rc->t, rc->r - rc->l, rc->b - rc->t);
}

static void draw_fillrect(int x, int y, int width, int height)
{
    lcd_(fillrect)(_X, _Y, _W, _H);
}

static void draw_hline(int x1, int x2, int y)
{
#ifdef LCD_LANDSCAPE
    lcd_(hline)(x1 + osd.x, x2 + osd.x, y + osd.y);
#else
    y = LCD_WIDTH - (y + osd.y) - 1;
    lcd_(vline)(y, x1 + osd.x, x2 + osd.x);
#endif
}

static void draw_vline(int x, int y1, int y2)
{
#ifdef LCD_LANDSCAPE
    lcd_(vline)(x + osd.x, y1 + osd.y, y2 + osd.y);
#else
    y1 = LCD_WIDTH - (y1 + osd.y) - 1;
    y2 = LCD_WIDTH - (y2 + osd.y) - 1;
    lcd_(hline)(y1, y2, x + osd.x);
#endif
}

static void draw_scrollbar_draw(int x, int y, int width, int height,
                                uint32_t min, uint32_t max, uint32_t val)
{
    unsigned oldfg = lcd_(get_foreground)();

    draw_hline(x + 1, x + width - 2, y);
    draw_hline(x + 1, x + width - 2, y + height - 1);
    draw_vline(x, y + 1, y + height - 2);
    draw_vline(x + width - 1, y + 1, y + height - 2);

    val = muldiv_uint32(width - 2, val, max - min);
    val = MIN(val, (uint32_t)(width - 2));

    draw_fillrect(x + 1, y + 1, val, height - 2);

    lcd_(set_foreground)(osd.prog_fillcolor);

    draw_fillrect(x + 1 + val, y + 1, width - 2 - val, height - 2);

    lcd_(set_foreground)(oldfg);
}

static void draw_scrollbar_draw_rect(const struct vo_rect *rc, int min,
                                     int max, int val)
{
    draw_scrollbar_draw(rc->l, rc->t, rc->r - rc->l, rc->b - rc->t,
                        min, max, val);
}

#ifdef LCD_PORTRAIT
/* Portrait displays need rotated text rendering */

/* Limited function that only renders in DRMODE_FG and uses absolute screen
 * coordinates */
static void draw_oriented_mono_bitmap_part(const unsigned char *src,
                                           int src_x, int src_y,
                                           int stride, int x, int y,
                                           int width, int height)
{
    const unsigned char *src_end;
    fb_data *dst, *dst_end;
    unsigned fg_pattern, bg_pattern;

    if (x + width > SCREEN_WIDTH)
        width = SCREEN_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > SCREEN_HEIGHT)
        height = SCREEN_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    fg_pattern = rb->lcd_get_foreground();
    bg_pattern = rb->lcd_get_background();

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    dst = rb->lcd_framebuffer + (LCD_WIDTH - y) + x*LCD_WIDTH;
    do
    {
        const unsigned char *src_col = src++;
        unsigned data = *src_col >> src_y;
        int numbits = 8 - src_y;

        fb_data *dst_col = dst;
        dst_end = dst_col - height;
        dst += LCD_WIDTH;

        do
        {
            dst_col--;

            if (data & 1)
                *dst_col = fg_pattern;
#if 0
            else
                *dst_col = bg_pattern;
#endif
            data >>= 1;
            if (--numbits == 0) {
                src_col += stride;
                data = *src_col;
                numbits = 8;
            }
        }
        while (dst_col > dst_end);
    }
    while (src < src_end);
}

static void draw_putsxy_oriented(int x, int y, const char *str)
{
    unsigned short ch;
    unsigned short *ucs;
    int ofs = MIN(x, 0);
    struct font* pf = rb->font_get(FONT_UI);

    ucs = rb->bidi_l2v(str, 1);

    x += osd.x;
    y += osd.y;

    while ((ch = *ucs++) != 0 && x < SCREEN_WIDTH)
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = rb->font_get_width(pf, ch);

        if (ofs > width) {
            ofs -= width;
            continue;
        }

        bits = rb->font_get_bits(pf, ch);

        draw_oriented_mono_bitmap_part(bits, ofs, 0, width, x, y,
                                       width - ofs, pf->height);

        x += width - ofs;
        ofs = 0;
    }
}
#else
static void draw_oriented_mono_bitmap_part(const unsigned char *src,
                                           int src_x, int src_y,
                                           int stride, int x, int y,
                                           int width, int height)
{
    int mode = lcd_(get_drawmode)();
    lcd_(set_drawmode)(DRMODE_FG);
    lcd_(mono_bitmap_part)(src, src_x, src_y, stride, x, y, width, height);
    lcd_(set_drawmode)(mode);
}

static void draw_putsxy_oriented(int x, int y, const char *str)
{
    int mode = lcd_(get_drawmode)();
    lcd_(set_drawmode)(DRMODE_FG);
    lcd_(putsxy)(x + osd.x, y + osd.y, str);
    lcd_(set_drawmode)(mode);
}
#endif /* LCD_PORTRAIT */

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
/* So we can refresh the overlay */
static void osd_lcd_enable_hook(void* param)
{
    (void)param;
    rb->queue_post(rb->button_queue, LCD_ENABLE_EVENT_1, 0);
}
#endif

static void osd_backlight_on_video_mode(bool video_on)
{
    if (video_on) {
        /* Turn off backlight timeout */
        /* backlight control in lib/helper.c */
        backlight_force_on();
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        rb->remove_event(LCD_EVENT_ACTIVATION, osd_lcd_enable_hook);
#endif
    } else {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        rb->add_event(LCD_EVENT_ACTIVATION, false, osd_lcd_enable_hook);
#endif
        /* Revert to user's backlight settings */
        backlight_use_settings();
    }
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
static void osd_backlight_brightness_video_mode(bool video_on)
{
    if (settings.backlight_brightness < 0)
        return;

    mpeg_backlight_update_brightness(
        video_on ? settings.backlight_brightness : -1);
}
#else
#define osd_backlight_brightness_video_mode(video_on)
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

static void osd_text_init(void)
{
    struct hms hms;
    char buf[32];
    int phys;
    int spc_width;

    lcd_(setfont)(FONT_UI);

    osd.x = 0;
    osd.width = SCREEN_WIDTH;

    vo_rect_clear(&osd.time_rect);
    vo_rect_clear(&osd.stat_rect);
    vo_rect_clear(&osd.prog_rect);
    vo_rect_clear(&osd.vol_rect);

    ts_to_hms(stream_get_duration(), &hms);
    hms_format(buf, sizeof (buf), &hms);
    lcd_(getstringsize)(buf, &osd.time_rect.r, &osd.time_rect.b);

    /* Choose well-sized bitmap images relative to font height */
    if (osd.time_rect.b < 12) {
        osd.icons = mpegplayer_status_icons_8x8x1;
        osd.stat_rect.r = osd.stat_rect.b = 8;
    } else if (osd.time_rect.b < 16) {
        osd.icons = mpegplayer_status_icons_12x12x1;
        osd.stat_rect.r = osd.stat_rect.b = 12;
    } else {
        osd.icons = mpegplayer_status_icons_16x16x1;
        osd.stat_rect.r = osd.stat_rect.b = 16;
    }

    if (osd.stat_rect.b < osd.time_rect.b) {
        vo_rect_offset(&osd.stat_rect, 0,
                       (osd.time_rect.b - osd.stat_rect.b) / 2 + OSD_BDR_T);
        vo_rect_offset(&osd.time_rect, OSD_BDR_L, OSD_BDR_T);
    } else {
        vo_rect_offset(&osd.time_rect, OSD_BDR_L,
                       osd.stat_rect.b - osd.time_rect.b + OSD_BDR_T);
        vo_rect_offset(&osd.stat_rect, 0, OSD_BDR_T);
    }

    osd.dur_rect = osd.time_rect;

    phys = rb->sound_val2phys(SOUND_VOLUME, rb->sound_min(SOUND_VOLUME));
    rb->snprintf(buf, sizeof(buf), "%d%s", phys,
                 rb->sound_unit(SOUND_VOLUME));

    lcd_(getstringsize)(" ", &spc_width, NULL);
    lcd_(getstringsize)(buf, &osd.vol_rect.r, &osd.vol_rect.b);

    osd.prog_rect.r = SCREEN_WIDTH - OSD_BDR_L - spc_width -
                           osd.vol_rect.r - OSD_BDR_R;
    osd.prog_rect.b = 3*osd.stat_rect.b / 4;
    vo_rect_offset(&osd.prog_rect, osd.time_rect.l,
                   osd.time_rect.b);

    vo_rect_offset(&osd.stat_rect,
                   (osd.prog_rect.r + osd.prog_rect.l - osd.stat_rect.r) / 2,
                   0);

    vo_rect_offset(&osd.dur_rect,
                   osd.prog_rect.r - osd.dur_rect.r, 0);

    vo_rect_offset(&osd.vol_rect, osd.prog_rect.r + spc_width,
                   (osd.prog_rect.b + osd.prog_rect.t - osd.vol_rect.b) / 2);

    osd.height = OSD_BDR_T + MAX(osd.prog_rect.b, osd.vol_rect.b) -
                    MIN(osd.time_rect.t, osd.stat_rect.t) + OSD_BDR_B;

#ifdef HAVE_LCD_COLOR
    osd.height = ALIGN_UP(osd.height, 2);
#endif
    osd.y = SCREEN_HEIGHT - osd.height;

    lcd_(setfont)(FONT_SYSFIXED);
}

static void osd_init(void)
{
    osd.flags = 0;
    osd.show_for = HZ*4;
    osd.print_delay = 75*HZ/100;
    osd.resume_delay = HZ/2;
#ifdef HAVE_LCD_COLOR
    osd.bgcolor = LCD_RGBPACK(0x73, 0x75, 0xbd);
    osd.fgcolor = LCD_WHITE;
    osd.prog_fillcolor = LCD_BLACK;
#else
    osd.bgcolor = GREY_LIGHTGRAY;
    osd.fgcolor = GREY_BLACK;
    osd.prog_fillcolor = GREY_WHITE;
#endif
    osd.curr_time = 0;
    osd.status = OSD_STATUS_STOPPED;
    osd.auto_refresh = OSD_REFRESH_TIME;
    osd.next_auto_refresh = *rb->current_tick;
    osd_text_init();
}

static void osd_schedule_refresh(unsigned refresh)
{
    long tick = *rb->current_tick;

    if (refresh & OSD_REFRESH_VIDEO)
        osd.print_tick = tick + osd.print_delay;

    if (refresh & OSD_REFRESH_RESUME)
        osd.resume_tick = tick + osd.resume_delay;

    osd.auto_refresh |= refresh;
}

static void osd_cancel_refresh(unsigned refresh)
{
    osd.auto_refresh &= ~refresh;
}

/* Refresh the background area */
static void osd_refresh_background(void)
{
    char buf[32];
    struct hms hms;

    unsigned bg = lcd_(get_background)();
    lcd_(set_drawmode)(DRMODE_SOLID | DRMODE_INVERSEVID);

#ifdef HAVE_LCD_COLOR
    /* Draw a "raised" area for our graphics */
    lcd_(set_background)(draw_blendcolor(bg, DRAW_WHITE, 192));
    draw_hline(0, osd.width, 0);

    lcd_(set_background)(draw_blendcolor(bg, DRAW_WHITE, 80));
    draw_hline(0, osd.width, 1);

    lcd_(set_background)(draw_blendcolor(bg, DRAW_BLACK, 48));
    draw_hline(0, osd.width, osd.height-2);

    lcd_(set_background)(draw_blendcolor(bg, DRAW_BLACK, 128));
    draw_hline(0, osd.width, osd.height-1);

    lcd_(set_background)(bg);
    draw_clear_area(0, 2, osd.width, osd.height - 4);
#else
    /* Give contrast with the main background */
    lcd_(set_background)(GREY_WHITE);
    draw_hline(0, osd.width, 0);

    lcd_(set_background)(GREY_DARKGRAY);
    draw_hline(0, osd.width, osd.height-1);

    lcd_(set_background)(bg);
    draw_clear_area(0, 1, osd.width, osd.height - 2);
#endif

    vo_rect_set_ext(&osd.update_rect, 0, 0, osd.width, osd.height);
    lcd_(set_drawmode)(DRMODE_SOLID);

    if (stream_get_duration() != INVALID_TIMESTAMP) {
        /* Draw the movie duration */
        ts_to_hms(stream_get_duration(), &hms);
        hms_format(buf, sizeof (buf), &hms);
        draw_putsxy_oriented(osd.dur_rect.l, osd.dur_rect.t, buf);
    }
    /* else don't know the duration */
}

/* Refresh the current time display + the progress bar */
static void osd_refresh_time(void)
{
    char buf[32];
    struct hms hms;

    uint32_t duration = stream_get_duration();

    draw_scrollbar_draw_rect(&osd.prog_rect, 0, duration,
                             osd.curr_time);

    ts_to_hms(osd.curr_time, &hms);
    hms_format(buf, sizeof (buf), &hms);

    draw_clear_area_rect(&osd.time_rect);
    draw_putsxy_oriented(osd.time_rect.l, osd.time_rect.t, buf);

    vo_rect_union(&osd.update_rect, &osd.update_rect,
                  &osd.prog_rect);
    vo_rect_union(&osd.update_rect, &osd.update_rect,
                  &osd.time_rect);
}

/* Refresh the volume display area */
static void osd_refresh_volume(void)
{
    char buf[32];
    int width;

    int volume = rb->global_settings->volume;
    rb->snprintf(buf, sizeof (buf), "%d%s",
                 rb->sound_val2phys(SOUND_VOLUME, volume),
                 rb->sound_unit(SOUND_VOLUME));
    lcd_(getstringsize)(buf, &width, NULL);

    /* Right-justified */
    draw_clear_area_rect(&osd.vol_rect);
    draw_putsxy_oriented(osd.vol_rect.r - width, osd.vol_rect.t, buf);

    vo_rect_union(&osd.update_rect, &osd.update_rect, &osd.vol_rect);
}

/* Refresh the status icon */
static void osd_refresh_status(void)
{
    int icon_size = osd.stat_rect.r - osd.stat_rect.l;

    draw_clear_area_rect(&osd.stat_rect);

#ifdef HAVE_LCD_COLOR
    /* Draw status icon with a drop shadow */
    unsigned oldfg = lcd_(get_foreground)();
    int i = 1;

    lcd_(set_foreground)(draw_blendcolor(lcd_(get_background)(),
                         DRAW_BLACK, 96));

    while (1)
    {
        draw_oriented_mono_bitmap_part(osd.icons,
                                       icon_size*osd.status,
                                       0,
                                       icon_size*OSD_STATUS_COUNT,
                                       osd.stat_rect.l + osd.x + i,
                                       osd.stat_rect.t + osd.y + i,
                                       icon_size, icon_size);

        if (--i < 0)
            break;

        lcd_(set_foreground)(oldfg);
    }

    vo_rect_union(&osd.update_rect, &osd.update_rect, &osd.stat_rect);
#else
    draw_oriented_mono_bitmap_part(osd.icons,
                                   icon_size*osd.status,
                                   0,
                                   icon_size*OSD_STATUS_COUNT,
                                   osd.stat_rect.l + osd.x,
                                   osd.stat_rect.t + osd.y,
                                   icon_size, icon_size);
    vo_rect_union(&osd.update_rect, &osd.update_rect, &osd.stat_rect);
#endif
}

/* Update the current status which determines which icon is displayed */
static bool osd_update_status(void)
{
    int status;

    switch (stream_status())
    {
    default:
        status = OSD_STATUS_STOPPED;
        break;
    case STREAM_PAUSED:
        /* If paused with a pending resume, coerce it to OSD_STATUS_PLAYING */
        status = (osd.auto_refresh & OSD_REFRESH_RESUME) ?
            OSD_STATUS_PLAYING : OSD_STATUS_PAUSED;
        break;
    case STREAM_PLAYING:
        status = OSD_STATUS_PLAYING;
        break;
    }

    if (status != osd.status) {
        /* A refresh is needed */
        osd.status = status;
        return true;
    }

    return false;
}

/* Update the current time that will be displayed */
static void osd_update_time(void)
{
    uint32_t start;
    osd.curr_time = stream_get_seek_time(&start);
    osd.curr_time -= start;
}

/* Refresh various parts of the OSD - showing it if it is hidden */
static void osd_refresh(int hint)
{
    long tick;
    unsigned oldbg, oldfg;

    tick = *rb->current_tick;

    if (hint == OSD_REFRESH_DEFAULT) {
        /* The default which forces no updates */

        /* Make sure Rockbox doesn't turn off the player because of
           too little activity */
        if (osd.status == OSD_STATUS_PLAYING)
            rb->reset_poweroff_timer();

        /* Redraw the current or possibly extract a new video frame */
        if ((osd.auto_refresh & OSD_REFRESH_VIDEO) &&
            TIME_AFTER(tick, osd.print_tick)) {
            osd.auto_refresh &= ~OSD_REFRESH_VIDEO;
            stream_draw_frame(false);
        }

        /* Restart playback if the timout was reached */
        if ((osd.auto_refresh & OSD_REFRESH_RESUME) &&
            TIME_AFTER(tick, osd.resume_tick)) {
            osd.auto_refresh &= ~(OSD_REFRESH_RESUME | OSD_REFRESH_VIDEO);
            stream_resume();
        }

        /* If not visible, return */
        if (!(osd.flags & OSD_SHOW))
            return;

        /* Hide if the visibility duration was reached */
        if (TIME_AFTER(tick, osd.hide_tick)) {
            osd_show(OSD_HIDE);
            return;
        }
    } else {
        /* A forced update of some region */

        /* Show if currently invisible */
        if (!(osd.flags & OSD_SHOW)) {
            /* Avoid call back into this function - it will be drawn */
            osd_show(OSD_SHOW | OSD_NODRAW);
            hint = OSD_REFRESH_ALL;
        }

        /* Move back timeouts for frame print and hide */
        osd.print_tick = tick + osd.print_delay;
        osd.hide_tick = tick + osd.show_for;
    }

    if (TIME_AFTER(tick, osd.next_auto_refresh)) {
        /* Refresh whatever graphical elements are due automatically */
        osd.next_auto_refresh = tick + OSD_MIN_UPDATE_INTERVAL;

        if (osd.auto_refresh & OSD_REFRESH_STATUS) {
            if (osd_update_status())
                hint |= OSD_REFRESH_STATUS;
        }

        if (osd.auto_refresh & OSD_REFRESH_TIME) {
            osd_update_time();
            hint |= OSD_REFRESH_TIME;
        }
    }

    if (hint == 0)
        return; /* No drawing needed */

    /* Set basic drawing params that are used. Elements that perform variations
     * will restore them. */
    oldfg = lcd_(get_foreground)();
    oldbg = lcd_(get_background)();

    lcd_(setfont)(FONT_UI);
    lcd_(set_foreground)(osd.fgcolor);
    lcd_(set_background)(osd.bgcolor);

    vo_rect_clear(&osd.update_rect);

    if (hint & OSD_REFRESH_BACKGROUND) {
        osd_refresh_background();
        hint |= OSD_REFRESH_ALL; /* Requires a redraw of everything */
    }

    if (hint & OSD_REFRESH_TIME) {
        osd_refresh_time();
    }

    if (hint & OSD_REFRESH_VOLUME) {
        osd_refresh_volume();
    }

    if (hint & OSD_REFRESH_STATUS) {
        osd_refresh_status();
    }

    /* Go back to defaults */
    lcd_(setfont)(FONT_SYSFIXED);
    lcd_(set_foreground)(oldfg);
    lcd_(set_background)(oldbg);

    /* Update the dirty rectangle */
    vo_lock();

    draw_update_rect(osd.update_rect.l,
                     osd.update_rect.t,
                     osd.update_rect.r - osd.update_rect.l,
                     osd.update_rect.b - osd.update_rect.t);

    vo_unlock();
}

/* Show/Hide the OSD */
static void osd_show(unsigned show)
{
    if (((show ^ osd.flags) & OSD_SHOW) == 0)
    {
        if (show & OSD_SHOW) {
            osd.hide_tick = *rb->current_tick + osd.show_for;
        }
        return;
    }

    if (show & OSD_SHOW) {
        /* Clip away the part of video that is covered */
        struct vo_rect rc = { 0, 0, SCREEN_WIDTH, osd.y };

        osd.flags |= OSD_SHOW;

        if (osd.status != OSD_STATUS_PLAYING) {
            /* Not playing - set brightness to mpegplayer setting */
            osd_backlight_brightness_video_mode(true);
        }

        stream_vo_set_clip(&rc);

        if (!(show & OSD_NODRAW))
            osd_refresh(OSD_REFRESH_ALL);
    } else {
        /* Uncover clipped video area and redraw it */
        osd.flags &= ~OSD_SHOW;

        draw_clear_area(0, 0, osd.width, osd.height);

        if (!(show & OSD_NODRAW)) {
            vo_lock();
            draw_update_rect(0, 0, osd.width, osd.height);
            vo_unlock();

            stream_vo_set_clip(NULL);
            stream_draw_frame(false);
        } else {
            stream_vo_set_clip(NULL);
        }

        if (osd.status != OSD_STATUS_PLAYING) {
            /* Not playing - restore backlight brightness */
            osd_backlight_brightness_video_mode(false);
        }
    }
}

/* Set the current status - update screen if specified */
static void osd_set_status(int status)
{
    bool draw = (status & OSD_NODRAW) == 0;

    status &= OSD_STATUS_MASK;

    if (osd.status != status) {

        osd.status = status;

        if (draw)
            osd_refresh(OSD_REFRESH_STATUS);
    }
}

/* Get the current status value */
static int osd_get_status(void)
{
    return osd.status & OSD_STATUS_MASK;
}

/* Handle Fast-forward/Rewind keys using WPS settings (and some nicked code ;) */
static uint32_t osd_ff_rw(int btn, unsigned refresh)
{
    unsigned int step = TS_SECOND*rb->global_settings->ff_rewind_min_step;
    const long ff_rw_accel = (rb->global_settings->ff_rewind_accel + 3);
    uint32_t start;
    uint32_t time = stream_get_seek_time(&start);
    const uint32_t duration = stream_get_duration();
    unsigned int max_step = 0;
    uint32_t ff_rw_count = 0;
    unsigned status = osd.status;

    osd_cancel_refresh(OSD_REFRESH_VIDEO | OSD_REFRESH_RESUME |
                       OSD_REFRESH_TIME);

    time -= start; /* Absolute clock => stream-relative */

    switch (btn)
    {
    case MPEG_FF:
#ifdef MPEG_FF2
    case MPEG_FF2:
#endif
#ifdef MPEG_RC_FF
    case MPEG_RC_FF:
#endif
        if (!(btn & BUTTON_REPEAT))
            osd_set_status(OSD_STATUS_FF);
        btn = MPEG_FF | BUTTON_REPEAT; /* simplify code below */
        break;
    case MPEG_RW:
#ifdef MPEG_RW2
    case MPEG_RW2:
#endif
#ifdef MPEG_RC_RW
    case MPEG_RC_RW:
#endif
        if (!(btn & BUTTON_REPEAT))
            osd_set_status(OSD_STATUS_RW);
        btn = MPEG_RW | BUTTON_REPEAT; /* simplify code below */
        break;
    default:
        btn = -1;
    }

    while (1)
    {
        stream_keep_disk_active();

        switch (btn)
        {
        case BUTTON_NONE:
            osd_refresh(OSD_REFRESH_DEFAULT);
            break;

        case MPEG_FF | BUTTON_REPEAT:
        case MPEG_RW | BUTTON_REPEAT:
#ifdef MPEG_FF2
        case MPEG_FF2 | BUTTON_REPEAT:
#endif
#ifdef MPEG_RW2
        case MPEG_RW2 | BUTTON_REPEAT:
#endif
#ifdef MPEG_RC_FF
        case MPEG_RC_FF | BUTTON_REPEAT:
        case MPEG_RC_RW | BUTTON_REPEAT:
#endif
            break;

        case MPEG_FF | BUTTON_REL:
        case MPEG_RW | BUTTON_REL:
#ifdef MPEG_FF2
        case MPEG_FF2 | BUTTON_REL:
#endif
#ifdef MPEG_RW2
        case MPEG_RW2 | BUTTON_REL:
#endif
#ifdef MPEG_RC_FF
        case MPEG_RC_FF | BUTTON_REL:
        case MPEG_RC_RW | BUTTON_REL:
#endif
            if (osd.status == OSD_STATUS_FF)
                time += ff_rw_count;
            else if (osd.status == OSD_STATUS_RW)
                time -= ff_rw_count;

            /* Fall-through */
        case -1:
        default:
            osd_schedule_refresh(refresh);
            osd_set_status(status);
            osd_schedule_refresh(OSD_REFRESH_TIME);
            return time;
        }

        if (osd.status == OSD_STATUS_FF) {
            /* fast forwarding, calc max step relative to end */
            max_step = muldiv_uint32(duration - (time + ff_rw_count),
                                     FF_REWIND_MAX_PERCENT, 100);
        } else {
            /* rewinding, calc max step relative to start */
            max_step = muldiv_uint32(time - ff_rw_count,
                                     FF_REWIND_MAX_PERCENT, 100);
        }

        max_step = MAX(max_step, MIN_FF_REWIND_STEP);

        if (step > max_step)
            step = max_step;

        ff_rw_count += step;

        /* smooth seeking by multiplying step by: 1 + (2 ^ -accel) */
        step += step >> ff_rw_accel;

        if (osd.status == OSD_STATUS_FF) {
            if (duration - time <= ff_rw_count)
                ff_rw_count = duration - time;

            osd.curr_time = time + ff_rw_count;
        } else {
            if (time <= ff_rw_count)
                ff_rw_count = time;

            osd.curr_time = time - ff_rw_count;
        }

        osd_refresh(OSD_REFRESH_TIME);

        btn = rb->button_get_w_tmo(OSD_MIN_UPDATE_INTERVAL);
    }
}

static int osd_status(void)
{
    int status = stream_status();

    /* Coerce to STREAM_PLAYING if paused with a pending resume */
    if (status == STREAM_PAUSED) {
        if (osd.auto_refresh & OSD_REFRESH_RESUME)
            status = STREAM_PLAYING;
    }

    return status;
}

/* Change the current audio volume by a specified amount */
static void osd_set_volume(int delta)
{
    int vol = rb->global_settings->volume;
    int limit;

    vol += delta;

    if (delta < 0) {
        /* Volume down - clip to lower limit */
        limit = rb->sound_min(SOUND_VOLUME);
        if (vol < limit)
            vol = limit;
    } else {
        /* Volume up - clip to upper limit */
        limit = rb->sound_max(SOUND_VOLUME);
        if (vol > limit)
            vol = limit;
    }

    /* Sync the global settings */
    if (vol != rb->global_settings->volume) {
        rb->sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
    }

    /* Update the volume display */
    osd_refresh(OSD_REFRESH_VOLUME);
}

/* Begin playback at the specified time */
static int osd_play(uint32_t time)
{
    int retval;

    osd_cancel_refresh(OSD_REFRESH_VIDEO | OSD_REFRESH_RESUME);

    retval = stream_seek(time, SEEK_SET);

    if (retval >= STREAM_OK) {
        osd_backlight_on_video_mode(true);
        osd_backlight_brightness_video_mode(true);
        stream_show_vo(true);
        retval = stream_play();

        if (retval >= STREAM_OK)
            osd_set_status(OSD_STATUS_PLAYING | OSD_NODRAW);
    }

    return retval;
}

/* Halt playback - pause engine and return logical state */
static int osd_halt(void)
{
    int status = stream_pause();

    /* Coerce to STREAM_PLAYING if paused with a pending resume */
    if (status == STREAM_PAUSED) {
        if (osd_get_status() == OSD_STATUS_PLAYING)
            status = STREAM_PLAYING;
    }

    /* Cancel some auto refreshes - caller will restart them if desired */
    osd_cancel_refresh(OSD_REFRESH_VIDEO | OSD_REFRESH_RESUME);

    /* No backlight fiddling here - callers does the right thing */

    return status;
}

/* Pause playback if playing */
static int osd_pause(void)
{
    unsigned refresh = osd.auto_refresh;
    int status = osd_halt();

    if (status == STREAM_PLAYING && (refresh & OSD_REFRESH_RESUME)) {
        /* Resume pending - change to a still video frame update */
        osd_schedule_refresh(OSD_REFRESH_VIDEO);
    }

    osd_set_status(OSD_STATUS_PAUSED);

    osd_backlight_on_video_mode(false);
    /* Leave brightness alone and restore it when OSD is hidden */

    return status;
}

/* Resume playback if halted or paused */
static void osd_resume(void)
{
    /* Cancel video and resume auto refresh - the resyc when starting
     * playback will perform those tasks */
    osd_backlight_on_video_mode(true);
    osd_backlight_brightness_video_mode(true);
    osd_cancel_refresh(OSD_REFRESH_VIDEO | OSD_REFRESH_RESUME);
    osd_set_status(OSD_STATUS_PLAYING);
    stream_resume();
}

/* Stop playback - remember the resume point if not closed */
static void osd_stop(void)
{
    uint32_t resume_time;

    osd_cancel_refresh(OSD_REFRESH_VIDEO | OSD_REFRESH_RESUME);
    osd_set_status(OSD_STATUS_STOPPED | OSD_NODRAW);
    osd_show(OSD_HIDE | OSD_NODRAW);

    stream_stop();

    resume_time = stream_get_resume_time();

    if (resume_time != INVALID_TIMESTAMP)
        settings.resume_time = resume_time;

    osd_backlight_on_video_mode(false);
    osd_backlight_brightness_video_mode(false);
}

/* Perform a seek if seeking is possible for this stream - if playing, a delay
 * will be inserted before restarting in case the user decides to seek again */
static void osd_seek(int btn)
{
    int status;
    unsigned refresh;
    uint32_t time;

    if (!stream_can_seek())
        return;

    /* Halt playback - not strictly nescessary but nice */
    status = osd_halt();

    if (status == STREAM_STOPPED)
        return;

    osd_show(OSD_SHOW);

    if (status == STREAM_PLAYING)
        refresh = OSD_REFRESH_RESUME; /* delay resume if playing */
    else
        refresh = OSD_REFRESH_VIDEO;  /* refresh if paused */

    /* Obtain a new playback point */
    time = osd_ff_rw(btn, refresh);

    /* Tell engine to resume at that time */
    stream_seek(time, SEEK_SET);
}

#ifdef HAVE_HEADPHONE_DETECTION
/* Handle SYS_PHONE_PLUGGED/UNPLUGGED */
static void osd_handle_phone_plug(bool inserted)
{
    if (rb->global_settings->unplug_mode == 0)
        return;

    /* Wait for any incomplete state transition to complete first */
    stream_wait_status();

    int status = osd_status();

    if (inserted) {
        if (rb->global_settings->unplug_mode > 1) {
            if (status == STREAM_PAUSED) {
                osd_resume();
            }
        }
    } else {
        if (status == STREAM_PLAYING) {
            osd_pause();

            if (stream_can_seek() && rb->global_settings->unplug_rw) {
                stream_seek(-rb->global_settings->unplug_rw*TS_SECOND,
                            SEEK_CUR);
                osd_schedule_refresh(OSD_REFRESH_VIDEO);
                /* Update time display now */
                osd_update_time();
                osd_refresh(OSD_REFRESH_TIME);
            }
        }
    }
}
#endif

static void button_loop(void)
{
    rb->lcd_setfont(FONT_SYSFIXED);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    rb->lcd_update();

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_YUV)
    rb->lcd_set_mode(LCD_MODE_YUV);
#endif

    osd_init();

    /* Start playback at the specified starting time */
    if (osd_play(settings.resume_time) < STREAM_OK) {
        rb->splash(HZ*2, "Playback failed");
        return;
    }

    /* Gently poll the video player for EOS and handle UI */
    while (stream_status() != STREAM_STOPPED)
    {
        int button;

        mpeg_menu_sysevent_clear();
        button = rb->button_get_w_tmo(OSD_MIN_UPDATE_INTERVAL/2);

        button = mpeg_menu_sysevent_callback(button, NULL);

        switch (button)
        {
        case BUTTON_NONE:
        {
            osd_refresh(OSD_REFRESH_DEFAULT);
            continue;
            } /* BUTTON_NONE: */

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        case LCD_ENABLE_EVENT_1:
        {
            /* Draw the current frame if prepared already */
            stream_draw_frame(true);
            break;
            } /* LCD_ENABLE_EVENT_1: */
#endif

        case MPEG_VOLUP:
        case MPEG_VOLUP|BUTTON_REPEAT:
#ifdef MPEG_VOLUP2
        case MPEG_VOLUP2:
        case MPEG_VOLUP2|BUTTON_REPEAT:
#endif
#ifdef MPEG_RC_VOLUP
        case MPEG_RC_VOLUP:
        case MPEG_RC_VOLUP|BUTTON_REPEAT:
#endif
        {
            osd_set_volume(+1);
            break;
            } /* MPEG_VOLUP*: */

        case MPEG_VOLDOWN:
        case MPEG_VOLDOWN|BUTTON_REPEAT:
#ifdef MPEG_VOLDOWN2
        case MPEG_VOLDOWN2:
        case MPEG_VOLDOWN2|BUTTON_REPEAT:
#endif
#ifdef MPEG_RC_VOLDOWN
        case MPEG_RC_VOLDOWN:
        case MPEG_RC_VOLDOWN|BUTTON_REPEAT:
#endif
        {
            osd_set_volume(-1);
            break;
            } /* MPEG_VOLDOWN*: */

        case MPEG_MENU:
#ifdef MPEG_RC_MENU
        case MPEG_RC_MENU:
#endif
        {
            int state = osd_halt(); /* save previous state */
            int result;

            /* Hide video output */
            osd_show(OSD_HIDE | OSD_NODRAW);
            stream_show_vo(false);
            osd_backlight_brightness_video_mode(false);

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_YUV)
            rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

            result = mpeg_menu();

            /* The menu can change the font, so restore */
            rb->lcd_setfont(FONT_SYSFIXED);
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(LCD_WHITE);
            rb->lcd_set_background(LCD_BLACK);
#endif
            rb->lcd_clear_display();
            rb->lcd_update();

            switch (result)
            {
            case MPEG_MENU_QUIT:
                osd_stop();
                break;

            default:
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_YUV)
                rb->lcd_set_mode(LCD_MODE_YUV);
#endif
                /* If not stopped, show video again */
                if (state != STREAM_STOPPED) {
                    osd_show(OSD_SHOW);
                    stream_show_vo(true);
                }

                /* If stream was playing, restart it */
                if (state == STREAM_PLAYING) {
                    osd_resume();
                }
                break;
            }
            break;
            } /* MPEG_MENU: */

#ifdef MPEG_SHOW_OSD
        case MPEG_SHOW_OSD:
        case MPEG_SHOW_OSD | BUTTON_REPEAT:
            /* Show if not visible */
            osd_show(OSD_SHOW);
            /* Make sure it refreshes */
            osd_refresh(OSD_REFRESH_DEFAULT);
            break;
#endif

        case MPEG_STOP:
#ifdef MPEG_RC_STOP
        case MPEG_RC_STOP:
#endif
        case ACTION_STD_CANCEL:
        {
            osd_stop();
            break;
            } /* MPEG_STOP: */

        case MPEG_PAUSE:
#ifdef MPEG_PAUSE2
        case MPEG_PAUSE2:
#endif
#ifdef MPEG_RC_PAUSE
        case MPEG_RC_PAUSE:
#endif
        {
            int status = osd_status();

            if (status == STREAM_PLAYING) {
                /* Playing => Paused */
                osd_pause();
            }
            else if (status == STREAM_PAUSED) {
                /* Paused => Playing */
                osd_resume();
            }

            break;
            } /* MPEG_PAUSE*: */

        case MPEG_RW:
        case MPEG_FF:
#ifdef MPEG_RW2
        case MPEG_RW2:
#endif
#ifdef MPEG_FF2
        case MPEG_FF2:
#endif
#ifdef MPEG_RC_RW
        case MPEG_RC_RW:
        case MPEG_RC_FF:
#endif
        {
            osd_seek(button);
            break;
            } /* MPEG_RW: MPEG_FF: */

#ifdef HAVE_HEADPHONE_DETECTION
        case SYS_PHONE_PLUGGED:
        case SYS_PHONE_UNPLUGGED:
        {
            osd_handle_phone_plug(button == SYS_PHONE_PLUGGED);
            break;
            } /* SYS_PHONE_*: */
#endif

        default:
        {
            rb->default_event_handler(button);
            break;
            } /* default: */
        }

        rb->yield();
    } /* end while */

    osd_stop();

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    /* Be sure hook is removed before exiting since the stop will put it
     * back because of the backlight restore. */
    rb->remove_event(LCD_EVENT_ACTIVATION, osd_lcd_enable_hook);
#endif

    rb->lcd_setfont(FONT_UI);
}

enum plugin_status plugin_start(const void* parameter)
{
    int status = PLUGIN_ERROR; /* assume failure */
    int result;
    int err;
    const char *errstring;

    if (parameter == NULL) {
        /* No file = GTFO */
        rb->splash(HZ*2, "No File");
        return PLUGIN_ERROR;
    }

    /* Disable all talking before initializing IRAM */
    rb->talk_disable(true);

    /* Initialize IRAM - stops audio and voice as well */
    PLUGIN_IRAM_INIT(rb)

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif

    rb->lcd_clear_display();
    rb->lcd_update();

    if (stream_init() < STREAM_OK) {
        DEBUGF("Could not initialize streams\n");
    } else {
        rb->splash(0, "Loading...");
        init_settings((char*)parameter);

        err = stream_open((char *)parameter);

        if (err >= STREAM_OK) {
            /* start menu */
            rb->lcd_clear_display();
            rb->lcd_update();
            result = mpeg_start_menu(stream_get_duration());

            if (result != MPEG_START_QUIT) {
                /* Enter button loop and process UI */
                button_loop();
            }

            stream_close();

            rb->lcd_clear_display();
            rb->lcd_update();

            save_settings();
            status = PLUGIN_OK;

            mpeg_menu_sysevent_handle();
        } else {
            DEBUGF("Could not open %s\n", (char*)parameter);
            switch (err)
            {
            case STREAM_UNSUPPORTED:
                errstring = "Unsupported format";
                break;
            default:
                errstring = "Error opening file: %d";
            }

            rb->splashf(HZ*2, errstring, err);
        }
    }

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_YUV)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

    stream_exit();

    rb->talk_disable(false);
    return status;
}
