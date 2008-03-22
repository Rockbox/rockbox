/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Oscilloscope, with many different modes of operation.
*
* Copyright (C) 2004-2006 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "helper.h"

#ifdef HAVE_LCD_BITMAP
#include "xlcd.h"
#include "configfile.h"

PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE     BUTTON_F1
#define OSCILLOSCOPE_ADVMODE      BUTTON_F2
#define OSCILLOSCOPE_ORIENTATION  BUTTON_F3
#define OSCILLOSCOPE_PAUSE        BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_DOWN

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE     BUTTON_F1
#define OSCILLOSCOPE_ADVMODE      BUTTON_F2
#define OSCILLOSCOPE_ORIENTATION  BUTTON_F3
#define OSCILLOSCOPE_PAUSE        BUTTON_SELECT
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE_PRE BUTTON_MENU
#define OSCILLOSCOPE_DRAWMODE     (BUTTON_MENU | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE      (BUTTON_MENU | BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION  (BUTTON_MENU | BUTTON_LEFT)
#define OSCILLOSCOPE_PAUSE        (BUTTON_MENU | BUTTON_OFF)
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define OSCILLOSCOPE_QUIT         BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE     BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE      BUTTON_MODE
#define OSCILLOSCOPE_ORIENTATION  BUTTON_REC
#define OSCILLOSCOPE_PAUSE        BUTTON_ON
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_DOWN
#define OSCILLOSCOPE_RC_QUIT      BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define OSCILLOSCOPE_QUIT         (BUTTON_SELECT | BUTTON_MENU)
#define OSCILLOSCOPE_DRAWMODE     (BUTTON_SELECT | BUTTON_PLAY)
#define OSCILLOSCOPE_ADVMODE      (BUTTON_SELECT | BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#define OSCILLOSCOPE_PAUSE        BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_SCROLL_FWD
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define OSCILLOSCOPE_QUIT         BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE     BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE      BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION  BUTTON_UP
#define OSCILLOSCOPE_PAUSE        BUTTON_A
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define OSCILLOSCOPE_QUIT         BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE     BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE      BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION  BUTTON_UP
#define OSCILLOSCOPE_PAUSE        BUTTON_REC
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_SCROLL_FWD
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define OSCILLOSCOPE_QUIT         BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE     BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE      BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION  BUTTON_UP
#define OSCILLOSCOPE_PAUSE        BUTTON_REC
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE BUTTON_SELECT
#define OSCILLOSCOPE_DRAWMODE     (BUTTON_SELECT | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE      BUTTON_REC
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_SELECT
#define OSCILLOSCOPE_ORIENTATION  (BUTTON_SELECT | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE        BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE BUTTON_REW
#define OSCILLOSCOPE_DRAWMODE     (BUTTON_REW | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE      BUTTON_FF
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_REW
#define OSCILLOSCOPE_ORIENTATION  (BUTTON_REW | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE        BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_SCROLL_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_BACK
#define OSCILLOSCOPE_DRAWMODE     BUTTON_PREV
#define OSCILLOSCOPE_ADVMODE      BUTTON_NEXT
#define OSCILLOSCOPE_ORIENTATION  BUTTON_MENU
#define OSCILLOSCOPE_PAUSE        BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define OSCILLOSCOPE_QUIT         BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE     BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE      BUTTON_MENU
#define OSCILLOSCOPE_ORIENTATION  BUTTON_PLAY
#define OSCILLOSCOPE_PAUSE        BUTTON_DISPLAY
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP       BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_RC_REC
#define OSCILLOSCOPE_DRAWMODE_PRE BUTTON_RC_MODE
#define OSCILLOSCOPE_DRAWMODE     (BUTTON_RC_MODE|BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE      BUTTON_RC_MENU
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_RC_MODE
#define OSCILLOSCOPE_ORIENTATION  (BUTTON_RC_MODE|BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE        BUTTON_RC_PLAY
#define OSCILLOSCOPE_SPEED_UP     BUTTON_RC_FF
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_RC_REW
#define OSCILLOSCOPE_VOL_UP       BUTTON_RC_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_RC_VOL_DOWN

#else
#error No keymap defined!
#endif

/* colours */
#if LCD_DEPTH > 1
#ifdef HAVE_LCD_COLOR
#define BACKG_COLOR  LCD_BLACK
#define GRAPH_COLOR  LCD_RGBPACK(128, 255, 0)
#define CURSOR_COLOR LCD_RGBPACK(255, 0, 0)
#else
#define BACKG_COLOR  LCD_WHITE
#define GRAPH_COLOR  LCD_BLACK
#define CURSOR_COLOR LCD_DARKGRAY
#endif
#endif

enum { DRAW_FILLED, DRAW_LINE, DRAW_PIXEL, MAX_DRAW };
enum { ADV_SCROLL, ADV_WRAP, MAX_ADV };
enum { OSC_HORIZ, OSC_VERT, MAX_OSC };

#define CFGFILE_VERSION 0     /* Current config file version */
#define CFGFILE_MINVERSION 0  /* Minimum config file version to accept */


#define MAX_PEAK 0x8000

#if defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
#define mas_codec_readreg(x) rand()%MAX_PEAK
#endif

/* global variables */

struct plugin_api* rb;     /* global api struct pointer */

/* settings */
struct osc_config {
    int delay;     /* in ticks */
    int draw;
    int advance;
    int orientation;
};

struct osc_config osc_disk = { 2, DRAW_FILLED, ADV_SCROLL, OSC_HORIZ };
struct osc_config osc;       /* running config */

static const char cfg_filename[] =  "oscilloscope.cfg";
static char *draw_str[3]        = { "filled", "line", "pixel" };
static char *advance_str[2]     = { "scroll", "wrap" };
static char *orientation_str[2] = { "horizontal", "vertical" };

struct configdata disk_config[] = {
   { TYPE_INT,  1, 99, &osc_disk.delay, "delay", NULL, NULL },
   { TYPE_ENUM, 0, MAX_DRAW, &osc_disk.draw, "draw", draw_str, NULL },
   { TYPE_ENUM, 0, MAX_ADV, &osc_disk.advance, "advance", advance_str, NULL },
   { TYPE_ENUM, 0, MAX_OSC, &osc_disk.orientation, "orientation", orientation_str, NULL }
};


long last_tick = 0;  /* time of last drawing */
int last_pos = 0;    /* last x or y drawing position. Reset for aspect switch. */
int last_left;       /* last channel values */
int last_right;

unsigned char message[16]; /* message to display */
bool displaymsg = false;
int font_height = 8;

/* implementation */

void anim_horizontal(int cur_left, int cur_right)
{
    int cur_x, x;
    int left, right, dl, dr;
    long cur_tick = *rb->current_tick;
    long d = (cur_tick - last_tick) / osc.delay;
    bool full_update = false;

    if (d == 0)  /* too early, bail out */
        return;

    last_tick = cur_tick;

    if (d > HZ) /* first call or too much delay, (re)start */
    {
        last_left = cur_left;
        last_right = cur_right;
        return;
    }
    cur_x = last_pos + d;

    if (cur_x >= LCD_WIDTH)
    {
        if (osc.advance == ADV_SCROLL)
        {
            int shift = cur_x - (LCD_WIDTH-1);
            xlcd_scroll_left(shift);
            full_update = true;
            cur_x -= shift;
            last_pos -= shift;
        }
        else
        {
            cur_x -= LCD_WIDTH;
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    if (cur_x > last_pos)
    {
        rb->lcd_fillrect(last_pos + 1, 0, d, LCD_HEIGHT);
    }
    else
    {
        rb->lcd_fillrect(last_pos + 1, 0, LCD_WIDTH - last_pos, LCD_HEIGHT);
        rb->lcd_fillrect(0, 0, cur_x + 1, LCD_HEIGHT);
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);

    switch (osc.draw)
    {
        case DRAW_FILLED:
            left = last_left;
            right = last_right;
            dl = (cur_left  - left)  / d;
            dr = (cur_right - right) / d;

            for (x = last_pos + 1; d > 0; x++, d--)
            {
                if (x == LCD_WIDTH)
                    x = 0;

                left  += dl;
                right += dr;

                rb->lcd_vline(x, LCD_HEIGHT/2-1,
                              LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16));
                rb->lcd_vline(x, LCD_HEIGHT/2+1,
                              LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16));
            }
            break;

        case DRAW_LINE:
            if (cur_x > last_pos)
            {
                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * last_left) >> 16),
                    cur_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * cur_left) >> 16)
                );
                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * last_right) >> 16),
                    cur_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * cur_right) >> 16)
                );
            }
            else
            {
                left  = last_left 
                      + (LCD_WIDTH - last_pos) * (last_left - cur_left) / d;
                right = last_right 
                      + (LCD_WIDTH - last_pos) * (last_right - cur_right) / d;

                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * last_left) >> 16),
                    LCD_WIDTH, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16)
                );
                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * last_right) >> 16),
                    LCD_WIDTH, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16)
                );
                if (cur_x > 0)
                {
                    rb->lcd_drawline(
                        0, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16),
                        cur_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * cur_left) >> 16)
                    );
                    rb->lcd_drawline(
                        0, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16),
                        cur_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * cur_right) >> 16)
                    );
                }
            }
            break;
            
        case DRAW_PIXEL:
            left = last_left;
            right = last_right;
            dl = (cur_left - left) / d;
            dr = (cur_right - right) / d;

            for (x = last_pos + 1; d > 0; x++, d--)
            {
                if (x == LCD_WIDTH)
                    x = 0;

                left  += dl;
                right += dr;

                rb->lcd_drawpixel(x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16));
                rb->lcd_drawpixel(x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16));
            }
            break;

    }
    last_left  = cur_left;
    last_right = cur_right;
    
    if (displaymsg)
    {
        int width;
        
        rb->lcd_getstringsize(message, &width, NULL);
        last_pos -= width - 1;
        rb->lcd_putsxy(last_pos, 0, message);
        displaymsg = false;

        if (last_pos < 0)
            last_pos = 0;
    }

    if (full_update)
    {
        rb->lcd_update();
    }
    else
    {
#if LCD_DEPTH > 1                 /* cursor bar */    
        rb->lcd_set_foreground(CURSOR_COLOR);
        rb->lcd_vline(cur_x + 1, 0, LCD_HEIGHT-1); 
        rb->lcd_set_foreground(GRAPH_COLOR);
#else
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_vline(cur_x + 1, 0, LCD_HEIGHT-1);
        rb->lcd_set_drawmode(DRMODE_SOLID);
#endif

        if (cur_x > last_pos)
        {
            rb->lcd_update_rect(last_pos, 0, cur_x - last_pos + 2, LCD_HEIGHT);
        }
        else
        {
            rb->lcd_update_rect(last_pos, 0, LCD_WIDTH - last_pos, LCD_HEIGHT);
            rb->lcd_update_rect(0, 0, cur_x + 2, LCD_HEIGHT);
        }
    }
    last_pos = cur_x;
}

void anim_vertical(int cur_left, int cur_right)
{
    int cur_y, y;
    int left, right, dl, dr;
    long cur_tick = *rb->current_tick;
    long d = (cur_tick - last_tick) / osc.delay;
    bool full_update = false;

    if (d == 0)  /* too early, bail out */
        return;

    last_tick = cur_tick;

    if (d > HZ) /* first call or too much delay, (re)start */
    {
        last_left = cur_left;
        last_right = cur_right;
        return;
    }
    cur_y = last_pos + d;

    if (cur_y >= LCD_HEIGHT)
    {
        if (osc.advance == ADV_SCROLL)
        {
            int shift = cur_y - (LCD_HEIGHT-1);
            xlcd_scroll_up(shift);
            full_update = true;
            cur_y -= shift;
            last_pos -= shift;
        }
        else 
        {
            cur_y -= LCD_HEIGHT;
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    if (cur_y > last_pos)
    {
        rb->lcd_fillrect(0, last_pos + 1, LCD_WIDTH, d);
    }
    else
    {
        rb->lcd_fillrect(0, last_pos + 1, LCD_WIDTH, LCD_HEIGHT - last_pos);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, cur_y + 1);
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);

    switch (osc.draw)
    {
        case DRAW_FILLED:
            left = last_left;
            right = last_right;
            dl = (cur_left  - left)  / d;
            dr = (cur_right - right) / d;

            for (y = last_pos + 1; d > 0; y++, d--)
            {
                if (y == LCD_HEIGHT)
                    y = 0;

                left  += dl;
                right += dr;

                rb->lcd_hline(LCD_WIDTH/2-1,
                              LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), y);
                rb->lcd_hline(LCD_WIDTH/2+1,
                              LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), y);
            }
            break;

        case DRAW_LINE:
            if (cur_y > last_pos)
            {
                rb->lcd_drawline(
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * last_left) >> 16), last_pos,
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * cur_left) >> 16), cur_y
                );
                rb->lcd_drawline(
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * last_right) >> 16), last_pos,
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * cur_right) >> 16), cur_y
                );
            }
            else
            {
                left  = last_left 
                      + (LCD_HEIGHT - last_pos) * (last_left - cur_left) / d;
                right = last_right
                      + (LCD_HEIGHT - last_pos) * (last_right - cur_right) / d;

                rb->lcd_drawline(
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * last_left) >> 16), last_pos,
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), LCD_HEIGHT
                );
                rb->lcd_drawline(
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * last_right) >> 16), last_pos,
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), LCD_HEIGHT
                );
                if (cur_y > 0)
                {
                    rb->lcd_drawline(
                        LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), 0,
                        LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * cur_left) >> 16), cur_y
                    );
                    rb->lcd_drawline(
                        LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), 0,
                        LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * cur_right) >> 16), cur_y
                    );
                }
            }
            break;
            
        case DRAW_PIXEL:
            left = last_left;
            right = last_right;
            dl = (cur_left - left) / d;
            dr = (cur_right - right) / d;

            for (y = last_pos + 1; d > 0; y++, d--)
            {
                if (y == LCD_HEIGHT)
                    y = 0;

                left  += dl;
                right += dr;

                rb->lcd_drawpixel(LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), y);
                rb->lcd_drawpixel(LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), y);
            }
            break;

    }
    last_left  = cur_left;
    last_right = cur_right;
    
    if (displaymsg)
    {
        last_pos -= font_height - 1;
        rb->lcd_putsxy(0, last_pos, message);
        displaymsg = false;

        if (last_pos < 0)
            last_pos = 0;
    }

    if (full_update)
    {
        rb->lcd_update();
    }
    else
    {
#if LCD_DEPTH > 1               /* cursor bar */
        rb->lcd_set_foreground(CURSOR_COLOR);
        rb->lcd_hline(0, LCD_WIDTH-1, cur_y + 1); 
        rb->lcd_set_foreground(GRAPH_COLOR);
#else
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_hline(0, LCD_WIDTH-1, cur_y + 1);
        rb->lcd_set_drawmode(DRMODE_SOLID);
#endif

        if (cur_y > last_pos)
        {
            rb->lcd_update_rect(0, last_pos, LCD_WIDTH, cur_y - last_pos + 2);
        }
        else
        {
            rb->lcd_update_rect(0, last_pos, LCD_WIDTH, LCD_HEIGHT - last_pos);
            rb->lcd_update_rect(0, 0, LCD_WIDTH, cur_y + 2);
        }
    }
    last_pos = cur_y;
}

void cleanup(void *parameter)
{
    (void)parameter;
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button, vol;
    int lastbutton = BUTTON_NONE;
    bool exit = false;
    bool paused = false;
    bool tell_speed;

    (void)parameter;
    rb = api;

    xlcd_init(rb);
    configfile_init(rb);

    configfile_load(cfg_filename, disk_config,
                    sizeof(disk_config) / sizeof(disk_config[0]),
                    CFGFILE_MINVERSION);
    rb->memcpy(&osc, &osc_disk, sizeof(osc));   /* copy to running config */

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(GRAPH_COLOR);
    rb->lcd_set_background(BACKG_COLOR);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_clear_display();
    rb->lcd_update();
#endif

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

    rb->lcd_getstringsize("A", NULL, &font_height);

    while (!exit)
    {
        if (!paused)
        {
            int left, right;

            rb->sleep(MAX(last_tick + osc.delay - *rb->current_tick - 1, 0));

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
            left = rb->mas_codec_readreg(0xC);
            right = rb->mas_codec_readreg(0xD);
#elif (CONFIG_CODEC == SWCODEC)
            rb->pcm_calculate_peaks(&left, &right);
#endif
            if (osc.orientation == OSC_HORIZ)
                anim_horizontal(left, right);
            else
                anim_vertical(left, right);
        }

        tell_speed = false;
        button = rb->button_get(paused);
        switch (button)
        {
#ifdef OSCILLOSCOPE_RC_QUIT
            case OSCILLOSCOPE_RC_QUIT:
#endif
            case OSCILLOSCOPE_QUIT:
                exit = true;
                break;

            case OSCILLOSCOPE_ADVMODE:
                if (++osc.advance >= MAX_ADV)
                    osc.advance = 0;
                break;

            case OSCILLOSCOPE_DRAWMODE:
#ifdef OSCILLOSCOPE_DRAWMODE_PRE
                if (lastbutton != OSCILLOSCOPE_DRAWMODE_PRE)
                    break;
#endif
                if (++osc.draw >= MAX_DRAW)
                    osc.draw = 0;
                break;
                
            case OSCILLOSCOPE_ORIENTATION:
#ifdef OSCILLOSCOPE_ORIENTATION_PRE
                if (lastbutton != OSCILLOSCOPE_ORIENTATION_PRE)
                    break;
#endif
                if (++osc.orientation >= MAX_OSC)
                    osc.orientation = 0;
                last_pos = 0;
                last_tick = 0;
                displaymsg = false;
                rb->lcd_clear_display();
                rb->lcd_update();
                break;

            case OSCILLOSCOPE_PAUSE:
                paused = !paused;
                last_tick = 0;
                break;
                
            case OSCILLOSCOPE_SPEED_UP:
            case OSCILLOSCOPE_SPEED_UP | BUTTON_REPEAT:
                if (osc.delay > 1)
                {
                    osc.delay--;
                    tell_speed = true;
                }
                break;
                
            case OSCILLOSCOPE_SPEED_DOWN:
            case OSCILLOSCOPE_SPEED_DOWN | BUTTON_REPEAT:
                osc.delay++;
                tell_speed = true;
                break;

            case OSCILLOSCOPE_VOL_UP:
            case OSCILLOSCOPE_VOL_UP | BUTTON_REPEAT:
                vol = rb->global_settings->volume;
                if (vol < rb->sound_max(SOUND_VOLUME))
                {
                    vol++;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }
                break;

            case OSCILLOSCOPE_VOL_DOWN:
            case OSCILLOSCOPE_VOL_DOWN | BUTTON_REPEAT:
                vol = rb->global_settings->volume;
                if (vol > rb->sound_min(SOUND_VOLUME))
                {
                    vol--;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }
                break;

            default:
                if (rb->default_event_handler_ex(button, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
            
        if (tell_speed)
        {
            rb->snprintf(message, sizeof(message), "%s%d", 
                        (osc.orientation == OSC_VERT) ? "Speed: " : "",
                        100 / osc.delay);
            displaymsg = true;
        }
    }
    cleanup(NULL);
    if (rb->memcmp(&osc, &osc_disk, sizeof(osc))) /* save settings if changed */
    {
        rb->memcpy(&osc_disk, &osc, sizeof(osc));
        configfile_save(cfg_filename, disk_config,
                        sizeof(disk_config) / sizeof(disk_config[0]),
                        CFGFILE_VERSION);
    }
    return PLUGIN_OK;
}
#endif /* HAVE_LCD_BITMAP */
