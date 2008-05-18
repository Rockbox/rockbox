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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "helper.h"
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
#define MPEG_FF         BUTTON_DOWN

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
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_SCROLL_BACK
#define MPEG_VOLUP      BUTTON_SCROLL_FWD
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP
#define MPEG_RW         BUTTON_LEFT
#define MPEG_FF         BUTTON_RIGHT

#elif CONFIG_KEYPAD == MROBE500_PAD
#define MPEG_MENU       BUTTON_RC_HEART
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_TOUCHPAD
#define MPEG_VOLDOWN    BUTTON_RC_VOL_DOWN
#define MPEG_VOLUP      BUTTON_RC_VOL_UP
#define MPEG_RW         BUTTON_RC_REW
#define MPEG_FF         BUTTON_RC_FF

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

#elif CONFIG_KEYPAD == COWOND2_PAD
#define MPEG_MENU       (BUTTON_MENU|BUTTON_REL)
//#define MPEG_STOP       BUTTON_POWER
#define MPEG_VOLDOWN    BUTTON_MINUS
#define MPEG_VOLUP      BUTTON_PLUS

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHPAD
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
#define MPEG_VOLDOWN    BUTTON_TOPMIDDLE
#endif
#ifndef MPEG_VOLUP
#define MPEG_VOLUP      BUTTON_BOTTOMMIDDLE
#endif
#ifndef MPEG_RW
#define MPEG_RW         BUTTON_MIDLEFT
#endif
#ifndef MPEG_FF
#define MPEG_FF         BUTTON_MIDRIGHT
#endif
#endif

const struct plugin_api* rb;

CACHE_FUNCTION_WRAPPERS(rb);
ALIGN_BUFFER_WRAPPER(rb);

/* One thing we can do here for targets with remotes is having a display
 * always on the remote instead of always forcing a popup on the main display */

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP (TS_SECOND/2)
#define WVS_MIN_UPDATE_INTERVAL (HZ/2)

/* WVS status - same order as icon array */
enum wvs_status_enum
{
    WVS_STATUS_STOPPED = 0,
    WVS_STATUS_PAUSED,
    WVS_STATUS_PLAYING,
    WVS_STATUS_FF,
    WVS_STATUS_RW,
    WVS_STATUS_COUNT,
    WVS_STATUS_MASK = 0x7
};

enum wvs_bits
{
    WVS_REFRESH_DEFAULT    = 0x0000, /* Only refresh elements when due */
                                     /* Refresh the... */
    WVS_REFRESH_VOLUME     = 0x0001, /* ...volume display */
    WVS_REFRESH_TIME       = 0x0002, /* ...time display+progress */
    WVS_REFRESH_STATUS     = 0x0004, /* ...playback status icon */
    WVS_REFRESH_BACKGROUND = 0x0008, /* ...background (implies ALL) */
    WVS_REFRESH_VIDEO      = 0x0010, /* ...video image upon timeout */
    WVS_REFRESH_RESUME     = 0x0020, /* Resume playback upon timeout */
    WVS_NODRAW             = 0x8000, /* OR bitflag - don't draw anything */
    WVS_SHOW               = 0x4000, /* OR bitflag - show the WVS */
    WVS_HP_PAUSE           = 0x2000,
    WVS_HIDE               = 0x0000, /* hide the WVS (aid readability) */
    WVS_REFRESH_ALL        = 0x000f, /* Only immediate graphical elements */
};

/* Status icons selected according to font height */
extern const unsigned char mpegplayer_status_icons_8x8x1[];
extern const unsigned char mpegplayer_status_icons_12x12x1[];
extern const unsigned char mpegplayer_status_icons_16x16x1[];

/* Main border areas that contain WVS elements */
#define WVS_BDR_L 2
#define WVS_BDR_T 2
#define WVS_BDR_R 2
#define WVS_BDR_B 2

struct wvs
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

static struct wvs wvs;

static void wvs_show(unsigned show);

#ifdef LCD_LANDSCAPE
    #define _X (x + wvs.x)
    #define _Y (y + wvs.y)
    #define _W width
    #define _H height
#else
    #define _X (LCD_WIDTH - (y + wvs.y) - height)
    #define _Y (x + wvs.x)
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
 * The origin is the upper-left corner of the WVS area */
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
    int x = rc->l;
    int y = rc->t;
    int width = rc->r - rc->l;
    int height = rc->b - rc->t;
#ifdef HAVE_LCD_COLOR
    rb->screen_clear_area(rb->screens[SCREEN_MAIN], _X, _Y, _W, _H);
#else
    int oldmode = grey_get_drawmode();
    grey_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    grey_fillrect(_X, _Y, _W, _H);
    grey_set_drawmode(oldmode);
#endif
}

static void draw_fillrect(int x, int y, int width, int height)
{
    lcd_(fillrect)(_X, _Y, _W, _H);
}

static void draw_hline(int x1, int x2, int y)
{
#ifdef LCD_LANDSCAPE
    lcd_(hline)(x1 + wvs.x, x2 + wvs.x, y + wvs.y);
#else
    y = LCD_WIDTH - (y + wvs.y) - 1;
    lcd_(vline)(y, x1 + wvs.x, x2 + wvs.x);
#endif
}

static void draw_vline(int x, int y1, int y2)
{
#ifdef LCD_LANDSCAPE
    lcd_(vline)(x + wvs.x, y1 + wvs.y, y2 + wvs.y);
#else
    y1 = LCD_WIDTH - (y1 + wvs.y) - 1;
    y2 = LCD_WIDTH - (y2 + wvs.y) - 1;
    lcd_(hline)(y1, y2, x + wvs.x);
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

    lcd_(set_foreground)(wvs.prog_fillcolor);

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

    x += wvs.x;
    y += wvs.y;

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
    lcd_(putsxy)(x + wvs.x, y + wvs.y, str);
    lcd_(set_drawmode)(mode);
}
#endif /* LCD_PORTRAIT */

static void wvs_backlight_on_video_mode(bool video_on)
{
    if (video_on) {
        /* Turn off backlight timeout */
        /* backlight control in lib/helper.c */
        backlight_force_on(rb);
    } else {
        /* Revert to user's backlight settings */
        backlight_use_settings(rb);
    }
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
static void wvs_backlight_brightness_video_mode(bool video_on)
{
    if (settings.backlight_brightness < 0)
        return;

    mpeg_backlight_update_brightness(
        video_on ? settings.backlight_brightness : -1);
}
#else
#define wvs_backlight_brightness_video_mode(video_on)
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

static void wvs_text_init(void)
{
    struct hms hms;
    char buf[32];
    int phys;
    int spc_width;

    lcd_(setfont)(FONT_UI);

    wvs.x = 0;
    wvs.width = SCREEN_WIDTH;

    vo_rect_clear(&wvs.time_rect);
    vo_rect_clear(&wvs.stat_rect);
    vo_rect_clear(&wvs.prog_rect);
    vo_rect_clear(&wvs.vol_rect);

    ts_to_hms(stream_get_duration(), &hms);
    hms_format(buf, sizeof (buf), &hms);
    lcd_(getstringsize)(buf, &wvs.time_rect.r, &wvs.time_rect.b);

    /* Choose well-sized bitmap images relative to font height */
    if (wvs.time_rect.b < 12) {
        wvs.icons = mpegplayer_status_icons_8x8x1;
        wvs.stat_rect.r = wvs.stat_rect.b = 8;
    } else if (wvs.time_rect.b < 16) {
        wvs.icons = mpegplayer_status_icons_12x12x1;
        wvs.stat_rect.r = wvs.stat_rect.b = 12;
    } else {
        wvs.icons = mpegplayer_status_icons_16x16x1;
        wvs.stat_rect.r = wvs.stat_rect.b = 16;
    }

    if (wvs.stat_rect.b < wvs.time_rect.b) {
        vo_rect_offset(&wvs.stat_rect, 0,
                       (wvs.time_rect.b - wvs.stat_rect.b) / 2 + WVS_BDR_T);
        vo_rect_offset(&wvs.time_rect, WVS_BDR_L, WVS_BDR_T);
    } else {
        vo_rect_offset(&wvs.time_rect, WVS_BDR_L,
                       wvs.stat_rect.b - wvs.time_rect.b + WVS_BDR_T);
        vo_rect_offset(&wvs.stat_rect, 0, WVS_BDR_T);
    }

    wvs.dur_rect = wvs.time_rect;

    phys = rb->sound_val2phys(SOUND_VOLUME, rb->sound_min(SOUND_VOLUME));
    rb->snprintf(buf, sizeof(buf), "%d%s", phys,
                 rb->sound_unit(SOUND_VOLUME));

    lcd_(getstringsize)(" ", &spc_width, NULL);
    lcd_(getstringsize)(buf, &wvs.vol_rect.r, &wvs.vol_rect.b);

    wvs.prog_rect.r = SCREEN_WIDTH - WVS_BDR_L - spc_width -
                           wvs.vol_rect.r - WVS_BDR_R;
    wvs.prog_rect.b = 3*wvs.stat_rect.b / 4;
    vo_rect_offset(&wvs.prog_rect, wvs.time_rect.l,
                   wvs.time_rect.b);

    vo_rect_offset(&wvs.stat_rect,
                   (wvs.prog_rect.r + wvs.prog_rect.l - wvs.stat_rect.r) / 2,
                   0);

    vo_rect_offset(&wvs.dur_rect,
                   wvs.prog_rect.r - wvs.dur_rect.r, 0);

    vo_rect_offset(&wvs.vol_rect, wvs.prog_rect.r + spc_width,
                   (wvs.prog_rect.b + wvs.prog_rect.t - wvs.vol_rect.b) / 2);

    wvs.height = WVS_BDR_T + MAX(wvs.prog_rect.b, wvs.vol_rect.b) -
                    MIN(wvs.time_rect.t, wvs.stat_rect.t) + WVS_BDR_B;

#ifdef HAVE_LCD_COLOR
    wvs.height = ALIGN_UP(wvs.height, 2);
#endif
    wvs.y = SCREEN_HEIGHT - wvs.height;

    lcd_(setfont)(FONT_SYSFIXED);
}

static void wvs_init(void)
{
    wvs.flags = 0;
    wvs.show_for = HZ*4;
    wvs.print_delay = 75*HZ/100;
    wvs.resume_delay = HZ/2;
#ifdef HAVE_LCD_COLOR
    wvs.bgcolor = LCD_RGBPACK(0x73, 0x75, 0xbd);
    wvs.fgcolor = LCD_WHITE;
    wvs.prog_fillcolor = LCD_BLACK;
#else
    wvs.bgcolor = GREY_LIGHTGRAY;
    wvs.fgcolor = GREY_BLACK;
    wvs.prog_fillcolor = GREY_WHITE;
#endif
    wvs.curr_time = 0;
    wvs.status = WVS_STATUS_STOPPED;
    wvs.auto_refresh = WVS_REFRESH_TIME;
    wvs.next_auto_refresh = *rb->current_tick;
    wvs_text_init();
}

static void wvs_schedule_refresh(unsigned refresh)
{
    long tick = *rb->current_tick;

    if (refresh & WVS_REFRESH_VIDEO)
        wvs.print_tick = tick + wvs.print_delay;

    if (refresh & WVS_REFRESH_RESUME)
        wvs.resume_tick = tick + wvs.resume_delay;

    wvs.auto_refresh |= refresh;
}

static void wvs_cancel_refresh(unsigned refresh)
{
    wvs.auto_refresh &= ~refresh;
}

/* Refresh the background area */
static void wvs_refresh_background(void)
{
    char buf[32];
    struct hms hms;

    unsigned bg = lcd_(get_background)();
    lcd_(set_drawmode)(DRMODE_SOLID | DRMODE_INVERSEVID);

#ifdef HAVE_LCD_COLOR
    /* Draw a "raised" area for our graphics */
    lcd_(set_background)(draw_blendcolor(bg, DRAW_WHITE, 192));
    draw_hline(0, wvs.width, 0);

    lcd_(set_background)(draw_blendcolor(bg, DRAW_WHITE, 80));
    draw_hline(0, wvs.width, 1);

    lcd_(set_background)(draw_blendcolor(bg, DRAW_BLACK, 48));
    draw_hline(0, wvs.width, wvs.height-2);

    lcd_(set_background)(draw_blendcolor(bg, DRAW_BLACK, 128));
    draw_hline(0, wvs.width, wvs.height-1);

    lcd_(set_background)(bg);
    draw_clear_area(0, 2, wvs.width, wvs.height - 4);
#else
    /* Give contrast with the main background */
    lcd_(set_background)(GREY_WHITE);
    draw_hline(0, wvs.width, 0);

    lcd_(set_background)(GREY_DARKGRAY);
    draw_hline(0, wvs.width, wvs.height-1);

    lcd_(set_background)(bg);
    draw_clear_area(0, 1, wvs.width, wvs.height - 2);
#endif

    vo_rect_set_ext(&wvs.update_rect, 0, 0, wvs.width, wvs.height);
    lcd_(set_drawmode)(DRMODE_SOLID);

    if (stream_get_duration() != INVALID_TIMESTAMP) {
        /* Draw the movie duration */
        ts_to_hms(stream_get_duration(), &hms);
        hms_format(buf, sizeof (buf), &hms);
        draw_putsxy_oriented(wvs.dur_rect.l, wvs.dur_rect.t, buf);
    }
    /* else don't know the duration */
}

/* Refresh the current time display + the progress bar */
static void wvs_refresh_time(void)
{
    char buf[32];
    struct hms hms;

    uint32_t duration = stream_get_duration();

    draw_scrollbar_draw_rect(&wvs.prog_rect, 0, duration,
                             wvs.curr_time);

    ts_to_hms(wvs.curr_time, &hms);
    hms_format(buf, sizeof (buf), &hms);

    draw_clear_area_rect(&wvs.time_rect);
    draw_putsxy_oriented(wvs.time_rect.l, wvs.time_rect.t, buf);    

    vo_rect_union(&wvs.update_rect, &wvs.update_rect,
                  &wvs.prog_rect);
    vo_rect_union(&wvs.update_rect, &wvs.update_rect,
                  &wvs.time_rect);
}

/* Refresh the volume display area */
static void wvs_refresh_volume(void)
{
    char buf[32];
    int width;

    int volume = rb->global_settings->volume;
    rb->snprintf(buf, sizeof (buf), "%d%s",
                 rb->sound_val2phys(SOUND_VOLUME, volume),
                 rb->sound_unit(SOUND_VOLUME));
    lcd_(getstringsize)(buf, &width, NULL);

    /* Right-justified */
    draw_clear_area_rect(&wvs.vol_rect);
    draw_putsxy_oriented(wvs.vol_rect.r - width, wvs.vol_rect.t, buf);

    vo_rect_union(&wvs.update_rect, &wvs.update_rect, &wvs.vol_rect);
}

/* Refresh the status icon */
static void wvs_refresh_status(void)
{
    int icon_size = wvs.stat_rect.r - wvs.stat_rect.l;

    draw_clear_area_rect(&wvs.stat_rect);

#ifdef HAVE_LCD_COLOR
    /* Draw status icon with a drop shadow */
    unsigned oldfg = lcd_(get_foreground)();
    int i = 1;

    lcd_(set_foreground)(draw_blendcolor(lcd_(get_background)(),
                         DRAW_BLACK, 96));

    while (1)
    {
        draw_oriented_mono_bitmap_part(wvs.icons,
                                       icon_size*wvs.status,
                                       0,
                                       icon_size*WVS_STATUS_COUNT,
                                       wvs.stat_rect.l + wvs.x + i,
                                       wvs.stat_rect.t + wvs.y + i,
                                       icon_size, icon_size);

        if (--i < 0)
            break;

        lcd_(set_foreground)(oldfg);
    }

    vo_rect_union(&wvs.update_rect, &wvs.update_rect, &wvs.stat_rect);
#else
    draw_oriented_mono_bitmap_part(wvs.icons,
                                   icon_size*wvs.status,
                                   0,
                                   icon_size*WVS_STATUS_COUNT,
                                   wvs.stat_rect.l + wvs.x,
                                   wvs.stat_rect.t + wvs.y,
                                   icon_size, icon_size);
    vo_rect_union(&wvs.update_rect, &wvs.update_rect, &wvs.stat_rect);
#endif
}

/* Update the current status which determines which icon is displayed */
static bool wvs_update_status(void)
{
    int status;

    switch (stream_status())
    {
    default:
        status = WVS_STATUS_STOPPED;
        break;
    case STREAM_PAUSED:
        /* If paused with a pending resume, coerce it to WVS_STATUS_PLAYING */
        status = (wvs.auto_refresh & WVS_REFRESH_RESUME) ?
            WVS_STATUS_PLAYING : WVS_STATUS_PAUSED;
        break;
    case STREAM_PLAYING:
        status = WVS_STATUS_PLAYING;
        break;
    }

    if (status != wvs.status) {
        /* A refresh is needed */
        wvs.status = status;
        return true;
    }

    return false;
}

/* Update the current time that will be displayed */
static void wvs_update_time(void)
{
    uint32_t start;
    wvs.curr_time = stream_get_seek_time(&start);
    wvs.curr_time -= start;
}

/* Refresh various parts of the WVS - showing it if it is hidden */
static void wvs_refresh(int hint)
{
    long tick;
    unsigned oldbg, oldfg;

    tick = *rb->current_tick;

    if (hint == WVS_REFRESH_DEFAULT) {
        /* The default which forces no updates */

        /* Make sure Rockbox doesn't turn off the player because of
           too little activity */
        if (wvs.status == WVS_STATUS_PLAYING)
            rb->reset_poweroff_timer();

        /* Redraw the current or possibly extract a new video frame */
        if ((wvs.auto_refresh & WVS_REFRESH_VIDEO) &&
            TIME_AFTER(tick, wvs.print_tick)) {
            wvs.auto_refresh &= ~WVS_REFRESH_VIDEO;
            stream_draw_frame(false);
        }

        /* Restart playback if the timout was reached */
        if ((wvs.auto_refresh & WVS_REFRESH_RESUME) &&
            TIME_AFTER(tick, wvs.resume_tick)) {
            wvs.auto_refresh &= ~(WVS_REFRESH_RESUME | WVS_REFRESH_VIDEO);
            stream_resume();
        }

        /* If not visible, return */
        if (!(wvs.flags & WVS_SHOW))
            return;

        /* Hide if the visibility duration was reached */
        if (TIME_AFTER(tick, wvs.hide_tick)) {
            wvs_show(WVS_HIDE);
            return;
        }
    } else {
        /* A forced update of some region */

        /* Show if currently invisible */
        if (!(wvs.flags & WVS_SHOW)) {
            /* Avoid call back into this function - it will be drawn */
            wvs_show(WVS_SHOW | WVS_NODRAW);
            hint = WVS_REFRESH_ALL;
        }

        /* Move back timeouts for frame print and hide */
        wvs.print_tick = tick + wvs.print_delay;
        wvs.hide_tick = tick + wvs.show_for;
    }

    if (TIME_AFTER(tick, wvs.next_auto_refresh)) {
        /* Refresh whatever graphical elements are due automatically */
        wvs.next_auto_refresh = tick + WVS_MIN_UPDATE_INTERVAL;

        if (wvs.auto_refresh & WVS_REFRESH_STATUS) {
            if (wvs_update_status())
                hint |= WVS_REFRESH_STATUS;
        }

        if (wvs.auto_refresh & WVS_REFRESH_TIME) {
            wvs_update_time();
            hint |= WVS_REFRESH_TIME;
        }
    }

    if (hint == 0)
        return; /* No drawing needed */

    /* Set basic drawing params that are used. Elements that perform variations
     * will restore them. */
    oldfg = lcd_(get_foreground)();
    oldbg = lcd_(get_background)();

    lcd_(setfont)(FONT_UI);
    lcd_(set_foreground)(wvs.fgcolor);
    lcd_(set_background)(wvs.bgcolor);

    vo_rect_clear(&wvs.update_rect);

    if (hint & WVS_REFRESH_BACKGROUND) {
        wvs_refresh_background();
        hint |= WVS_REFRESH_ALL; /* Requires a redraw of everything */
    }

    if (hint & WVS_REFRESH_TIME) {
        wvs_refresh_time();
    }

    if (hint & WVS_REFRESH_VOLUME) {
        wvs_refresh_volume();
    }

    if (hint & WVS_REFRESH_STATUS) {
        wvs_refresh_status();
    }

    /* Go back to defaults */
    lcd_(setfont)(FONT_SYSFIXED);
    lcd_(set_foreground)(oldfg);
    lcd_(set_background)(oldbg);

    /* Update the dirty rectangle */
    vo_lock();

    draw_update_rect(wvs.update_rect.l,
                     wvs.update_rect.t,
                     wvs.update_rect.r - wvs.update_rect.l,
                     wvs.update_rect.b - wvs.update_rect.t);

    vo_unlock();
}

/* Show/Hide the WVS */
static void wvs_show(unsigned show)
{
    if (((show ^ wvs.flags) & WVS_SHOW) == 0)
    {
        if (show & WVS_SHOW) {
            wvs.hide_tick = *rb->current_tick + wvs.show_for;
        }
        return;
    }

    if (show & WVS_SHOW) {
        /* Clip away the part of video that is covered */
        struct vo_rect rc = { 0, 0, SCREEN_WIDTH, wvs.y };

        wvs.flags |= WVS_SHOW;

        if (wvs.status != WVS_STATUS_PLAYING) {
            /* Not playing - set brightness to mpegplayer setting */
            wvs_backlight_brightness_video_mode(true);
        }

        stream_vo_set_clip(&rc);

        if (!(show & WVS_NODRAW))
            wvs_refresh(WVS_REFRESH_ALL);
    } else {
        /* Uncover clipped video area and redraw it */
        wvs.flags &= ~WVS_SHOW;

        draw_clear_area(0, 0, wvs.width, wvs.height);

        if (!(show & WVS_NODRAW)) {
            vo_lock();
            draw_update_rect(0, 0, wvs.width, wvs.height);
            vo_unlock();

            stream_vo_set_clip(NULL);
            stream_draw_frame(false);
        } else {
            stream_vo_set_clip(NULL);
        }

        if (wvs.status != WVS_STATUS_PLAYING) {
            /* Not playing - restore backlight brightness */
            wvs_backlight_brightness_video_mode(false);
        }
    }
}

/* Set the current status - update screen if specified */
static void wvs_set_status(int status)
{
    bool draw = (status & WVS_NODRAW) == 0;

    status &= WVS_STATUS_MASK;

    if (wvs.status != status) {

        wvs.status = status;

        if (draw)
            wvs_refresh(WVS_REFRESH_STATUS);
    }
}

/* Get the current status value */
static int wvs_get_status(void)
{
    return wvs.status & WVS_STATUS_MASK;
}

/* Handle Fast-forward/Rewind keys using WPS settings (and some nicked code ;) */
static uint32_t wvs_ff_rw(int btn, unsigned refresh)
{
    unsigned int step = TS_SECOND*rb->global_settings->ff_rewind_min_step;
    const long ff_rw_accel = rb->global_settings->ff_rewind_accel;
    long accel_tick = *rb->current_tick + ff_rw_accel*HZ;
    uint32_t start;
    uint32_t time = stream_get_seek_time(&start);
    const uint32_t duration = stream_get_duration();
    unsigned int max_step = 0;
    uint32_t ff_rw_count = 0;
    unsigned status = wvs.status;

    wvs_cancel_refresh(WVS_REFRESH_VIDEO | WVS_REFRESH_RESUME |
                       WVS_REFRESH_TIME);

    time -= start; /* Absolute clock => stream-relative */

    switch (btn)
    {
    case MPEG_FF:
#ifdef MPEG_RC_FF
    case MPEG_RC_FF:
#endif
        wvs_set_status(WVS_STATUS_FF);
        break;
    case MPEG_RW:
#ifdef MPEG_RC_RW
    case MPEG_RC_RW:
#endif
        wvs_set_status(WVS_STATUS_RW);
        break;
    default:
        btn = -1;
    }

    btn |= BUTTON_REPEAT;

    while (1)
    {
        long tick = *rb->current_tick;
        stream_keep_disk_active();

        switch (btn)
        {
        case BUTTON_NONE:
            wvs_refresh(WVS_REFRESH_DEFAULT);
            break;

        case MPEG_FF | BUTTON_REPEAT:
        case MPEG_RW | BUTTON_REPEAT:
#ifdef MPEG_RC_FF
        case MPEG_RC_FF | BUTTON_REPEAT:
        case MPEG_RC_RW | BUTTON_REPEAT:
#endif
            break;

        case MPEG_FF | BUTTON_REL:
        case MPEG_RW | BUTTON_REL:
#ifdef MPEG_RC_FF
        case MPEG_RC_FF | BUTTON_REL:
        case MPEG_RC_RW | BUTTON_REL:
#endif
            if (wvs.status == WVS_STATUS_FF)
                time += ff_rw_count;
            else if (wvs.status == WVS_STATUS_RW)
                time -= ff_rw_count;

            /* Fall-through */
        case -1:
        default:
            wvs_schedule_refresh(refresh);
            wvs_set_status(status);
            wvs_schedule_refresh(WVS_REFRESH_TIME);
            return time;
        }

        if (wvs.status == WVS_STATUS_FF) {
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

        if (ff_rw_accel != 0 && TIME_AFTER(tick, accel_tick)) { 
            step *= 2;
            accel_tick = tick + ff_rw_accel*HZ; 
        }

        if (wvs.status == WVS_STATUS_FF) {
            if (duration - time <= ff_rw_count)
                ff_rw_count = duration - time;

            wvs.curr_time = time + ff_rw_count;
        } else {
            if (time <= ff_rw_count)
                ff_rw_count = time;

            wvs.curr_time = time - ff_rw_count;
        }

        wvs_refresh(WVS_REFRESH_TIME);

        btn = rb->button_get_w_tmo(WVS_MIN_UPDATE_INTERVAL);
    }
}

static int wvs_status(void)
{
    int status = stream_status();

    /* Coerce to STREAM_PLAYING if paused with a pending resume */
    if (status == STREAM_PAUSED) {
        if (wvs.auto_refresh & WVS_REFRESH_RESUME)
            status = STREAM_PLAYING;
    }

    return status;
}

/* Change the current audio volume by a specified amount */
static void wvs_set_volume(int delta)
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
    wvs_refresh(WVS_REFRESH_VOLUME);
}

/* Begin playback at the specified time */
static int wvs_play(uint32_t time)
{
    int retval;

    wvs_cancel_refresh(WVS_REFRESH_VIDEO | WVS_REFRESH_RESUME);

    retval = stream_seek(time, SEEK_SET);

    if (retval >= STREAM_OK) {
        wvs_backlight_on_video_mode(true);
        wvs_backlight_brightness_video_mode(true);
        stream_show_vo(true);
        retval = stream_play();

        if (retval >= STREAM_OK)
            wvs_set_status(WVS_STATUS_PLAYING | WVS_NODRAW);
    }

    return retval;
}

/* Halt playback - pause engine and return logical state */
static int wvs_halt(void)
{
    int status = stream_pause();

    /* Coerce to STREAM_PLAYING if paused with a pending resume */
    if (status == STREAM_PAUSED) {
        if (wvs_get_status() == WVS_STATUS_PLAYING)
            status = STREAM_PLAYING;
    }

    /* Cancel some auto refreshes - caller will restart them if desired */
    wvs_cancel_refresh(WVS_REFRESH_VIDEO | WVS_REFRESH_RESUME);

    /* No backlight fiddling here - callers does the right thing */

    return status;
}

/* Pause playback if playing */
static int wvs_pause(void)
{
    unsigned refresh = wvs.auto_refresh;
    int status = wvs_halt();

    if (status == STREAM_PLAYING && (refresh & WVS_REFRESH_RESUME)) {
        /* Resume pending - change to a still video frame update */
        wvs_schedule_refresh(WVS_REFRESH_VIDEO);
    }

    wvs_set_status(WVS_STATUS_PAUSED);

    wvs_backlight_on_video_mode(false);
    /* Leave brightness alone and restore it when WVS is hidden */

    return status;
}

/* Resume playback if halted or paused */
static void wvs_resume(void)
{
    /* Cancel video and resume auto refresh - the resyc when starting
     * playback will perform those tasks */
    wvs_backlight_on_video_mode(true);
    wvs_backlight_brightness_video_mode(true);
    wvs_cancel_refresh(WVS_REFRESH_VIDEO | WVS_REFRESH_RESUME);
    wvs_set_status(WVS_STATUS_PLAYING);
    stream_resume();
}

/* Stop playback - remember the resume point if not closed */
static void wvs_stop(void)
{
    uint32_t resume_time;

    wvs_cancel_refresh(WVS_REFRESH_VIDEO | WVS_REFRESH_RESUME);
    wvs_set_status(WVS_STATUS_STOPPED | WVS_NODRAW);
    wvs_show(WVS_HIDE | WVS_NODRAW);

    stream_stop();

    resume_time = stream_get_resume_time();

    if (resume_time != INVALID_TIMESTAMP)
        settings.resume_time = resume_time;

    wvs_backlight_on_video_mode(false);
    wvs_backlight_brightness_video_mode(false);
}

/* Perform a seek if seeking is possible for this stream - if playing, a delay
 * will be inserted before restarting in case the user decides to seek again */
static void wvs_seek(int btn)
{
    int status;
    unsigned refresh;
    uint32_t time;

    if (!stream_can_seek())
        return;

    /* Halt playback - not strictly nescessary but nice */
    status = wvs_halt();

    if (status == STREAM_STOPPED)
        return;

    wvs_show(WVS_SHOW);

    if (status == STREAM_PLAYING)
        refresh = WVS_REFRESH_RESUME; /* delay resume if playing */
    else
        refresh = WVS_REFRESH_VIDEO;  /* refresh if paused */

    /* Obtain a new playback point */
    time = wvs_ff_rw(btn, refresh);

    /* Tell engine to resume at that time */
    stream_seek(time, SEEK_SET);
}

#ifdef HAVE_HEADPHONE_DETECTION
/* Handle SYS_PHONE_PLUGGED/UNPLUGGED */
static void wvs_handle_phone_plug(bool inserted)
{
    if (rb->global_settings->unplug_mode == 0)
        return;

    /* Wait for any incomplete state transition to complete first */
    stream_wait_status();

    int status = wvs_status();

    if (inserted) {
        if (rb->global_settings->unplug_mode > 1) {
            if (status == STREAM_PAUSED) {
                wvs_resume();
            }
        }
    } else {
        if (status == STREAM_PLAYING) {
            wvs_pause();

            if (stream_can_seek() && rb->global_settings->unplug_rw) {
                stream_seek(-rb->global_settings->unplug_rw*TS_SECOND,
                            SEEK_CUR);
                wvs_schedule_refresh(WVS_REFRESH_VIDEO);
                /* Update time display now */
                wvs_update_time();
                wvs_refresh(WVS_REFRESH_TIME);
            }
        }
    }
}
#endif

static void button_loop(void)
{
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_clear_display();
    rb->lcd_update();

    wvs_init();

    /* Start playback at the specified starting time */
    if (wvs_play(settings.resume_time) < STREAM_OK) {
        rb->splash(HZ*2, "Playback failed");
        return;
    }

    /* Gently poll the video player for EOS and handle UI */
    while (stream_status() != STREAM_STOPPED)
    {
        int button;

        mpeg_menu_sysevent_clear();
        button = rb->button_get_w_tmo(WVS_MIN_UPDATE_INTERVAL);

        button = mpeg_menu_sysevent_callback(button, -1);

        switch (button)
        {
        case BUTTON_NONE:
        {
            wvs_refresh(WVS_REFRESH_DEFAULT);
            continue;
            } /* BUTTON_NONE: */

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
            wvs_set_volume(+1);
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
            wvs_set_volume(-1);
            break;
            } /* MPEG_VOLDOWN*: */

        case MPEG_MENU:
#ifdef MPEG_RC_MENU
        case MPEG_RC_MENU:
#endif
        {
            int state = wvs_halt(); /* save previous state */
            int result;

            /* Hide video output */
            wvs_show(WVS_HIDE | WVS_NODRAW);
            stream_show_vo(false);
            wvs_backlight_brightness_video_mode(false);

            result = mpeg_menu(0);

            /* The menu can change the font, so restore */
            rb->lcd_setfont(FONT_SYSFIXED);

            switch (result)
            {
            case MPEG_MENU_QUIT:
                wvs_stop();
                break;

            default:
                /* If not stopped, show video again */
                if (state != STREAM_STOPPED) {
                    wvs_show(WVS_SHOW);
                    stream_show_vo(true);
                }

                /* If stream was playing, restart it */
                if (state == STREAM_PLAYING) {
                    wvs_resume();
                }
                break;
            }
            break;
            } /* MPEG_MENU: */

        case MPEG_STOP:
#ifdef MPEG_RC_STOP
        case MPEG_RC_STOP:
#endif
        case ACTION_STD_CANCEL:
        {
            wvs_stop();
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
            int status = wvs_status();

            if (status == STREAM_PLAYING) {
                /* Playing => Paused */
                wvs_pause();
            }
            else if (status == STREAM_PAUSED) {
                /* Paused => Playing */
                wvs_resume();
            }

            break;
            } /* MPEG_PAUSE*: */

        case MPEG_RW:
        case MPEG_FF:
#ifdef MPEG_RC_RW
        case MPEG_RC_RW:
        case MPEG_RC_FF:
#endif
        {
            wvs_seek(button);
            break;
            } /* MPEG_RW: MPEG_FF: */

#ifdef HAVE_HEADPHONE_DETECTION
        case SYS_PHONE_PLUGGED:
        case SYS_PHONE_UNPLUGGED:
        {
            wvs_handle_phone_plug(button == SYS_PHONE_PLUGGED);
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

    wvs_stop();

    rb->lcd_setfont(FONT_UI);
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    int status = PLUGIN_ERROR; /* assume failure */
    int result;
    int err;
    const char *errstring;

    if (parameter == NULL) {
        /* No file = GTFO */
        api->splash(HZ*2, "No File");
        return PLUGIN_ERROR;
    }

    /* Disable all talking before initializing IRAM */
    api->talk_disable(true);

    /* Initialize IRAM - stops audio and voice as well */
    PLUGIN_IRAM_INIT(api)

    rb = api;

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

            save_settings();  /* Save settings (if they have changed) */
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

            rb->splash(HZ*2, errstring, err);
        }
    }

    stream_exit();

    rb->talk_disable(false);
    return status;
}
