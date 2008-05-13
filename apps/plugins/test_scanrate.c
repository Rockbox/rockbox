/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2006 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH < 4) && !defined(SIMULATOR)

PLUGIN_HEADER

#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == ONDIO_PAD) \
 || (CONFIG_KEYPAD == IRIVER_H100_PAD)
#define SCANRATE_DONE    BUTTON_OFF
#define SCANRATE_FASTINC BUTTON_UP
#define SCANRATE_FASTDEC BUTTON_DOWN
#define SCANRATE_INC     BUTTON_RIGHT
#define SCANRATE_DEC     BUTTON_LEFT

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD) \
 || (CONFIG_KEYPAD == MROBE100_PAD)
#define SCANRATE_DONE    BUTTON_POWER
#define SCANRATE_FASTINC BUTTON_UP
#define SCANRATE_FASTDEC BUTTON_DOWN
#define SCANRATE_INC     BUTTON_RIGHT
#define SCANRATE_DEC     BUTTON_LEFT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define SCANRATE_DONE    BUTTON_RC_REC
#define SCANRATE_FASTINC BUTTON_RC_VOL_UP
#define SCANRATE_FASTDEC BUTTON_RC_VOL_DOWN
#define SCANRATE_INC     BUTTON_RC_FF
#define SCANRATE_DEC     BUTTON_RC_REW

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define SCANRATE_DONE    BUTTON_MENU
#define SCANRATE_FASTINC BUTTON_SCROLL_FWD
#define SCANRATE_FASTDEC BUTTON_SCROLL_BACK
#define SCANRATE_INC     BUTTON_RIGHT
#define SCANRATE_DEC     BUTTON_LEFT

#endif

/* Default refresh rates in 1/10 Hz */
#if defined ARCHOS_RECORDER   || defined ARCHOS_FMRECORDER  \
 || defined ARCHOS_RECORDERV2 || defined ARCHOS_ONDIOFM     \
 || defined ARCHOS_ONDIOSP
#define DEFAULT_SCAN_RATE 670
#elif defined IAUDIO_M3
#define DEFAULT_SCAN_RATE 1500
#define HORIZ_SCAN /* LCD controller updates the panel sideways */
#define NEED_BOOST
#elif defined IAUDIO_M5
#define DEFAULT_SCAN_RATE 730
#elif defined IPOD_1G2G
#define DEFAULT_SCAN_RATE 960
#elif defined IPOD_MINI2G  || defined IPOD_MINI  \
   || defined IPOD_3G      || defined IPOD_4G
#define DEFAULT_SCAN_RATE 870
#elif defined IRIVER_H100_SERIES
#define DEFAULT_SCAN_RATE 700
#else
#define DEFAULT_SCAN_RATE 700
#warning Generic default scanrate
#endif

#ifdef HORIZ_SCAN
#define TEXT_X 0
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define BUF_WIDTH  ((LCD_WIDTH+7)/8)
#define BUF_HEIGHT (LCD_HEIGHT/4)
#define TEXT_Y BUF_HEIGHT
#else
#define BUF_WIDTH  (LCD_WIDTH)
#define BUF_HEIGHT (LCD_HEIGHT/8/4)
#define TEXT_Y (BUF_HEIGHT*8)
#endif
#else /* !HORIZ_SCAN */
#define TEXT_Y 0
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define BUF_WIDTH  ((LCD_WIDTH+7)/8/4)
#define BUF_HEIGHT LCD_HEIGHT
#define TEXT_X (BUF_WIDTH*8)
#else
#define BUF_WIDTH  (LCD_WIDTH/4)
#define BUF_HEIGHT (LCD_HEIGHT/8)
#define TEXT_X BUF_WIDTH
#endif
#endif /* !HORIZ_SCAN */

#if defined(CPU_PP) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
#define NEED_BOOST
#endif

static const struct plugin_api* rb;
static unsigned char bitbuffer[2][BUF_HEIGHT][BUF_WIDTH];
static int curbuf = 0;
static int scan_rate = DEFAULT_SCAN_RATE;
static bool need_refresh = false;

static void timer_isr(void)
{
    rb->lcd_blit_mono(bitbuffer[curbuf][0], 0, 0, BUF_WIDTH, BUF_HEIGHT, BUF_WIDTH);
    curbuf = (curbuf + 1) & 1;
    if (need_refresh)
    {
        rb->lcd_update_rect(TEXT_X, TEXT_Y, LCD_WIDTH-TEXT_X, 8);
        need_refresh = false;
    }
}

int plugin_main(void)
{
    unsigned char buf[32];
    int button;
    bool done = false;
    bool change = true;
    
    rb->lcd_setfont(FONT_SYSFIXED);

    rb->lcd_putsxy(TEXT_X, TEXT_Y+12, "Adjust Frequ.");
    rb->lcd_putsxy(TEXT_X, TEXT_Y+20, "so the block");
    rb->lcd_putsxy(TEXT_X, TEXT_Y+28, "stops moving.");
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) \
 || (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    rb->lcd_putsxy(TEXT_X, TEXT_Y+40, "Scroll: Coarse");
#else
    rb->lcd_putsxy(TEXT_X, TEXT_Y+40, "U/D: Coarse");
#endif
    rb->lcd_putsxy(TEXT_X, TEXT_Y+48, "L/R: Fine");
    rb->lcd_update();

    rb->memset(bitbuffer[0], 0, sizeof(bitbuffer[0]));
    rb->memset(bitbuffer[1], 0xff, sizeof(bitbuffer[1]));
#ifdef NEED_BOOST
    rb->cpu_boost(true);
#endif
    /* The actual frequency is twice the displayed value */
    rb->timer_register(1, NULL, TIMER_FREQ * 5 / scan_rate, 1,
                       timer_isr IF_COP(, CPU));

    while (!done)
    {
        if (change)
        {
            /* The actual frequency is twice the displayed value */
            rb->timer_set_period(TIMER_FREQ * 5 / scan_rate);
            rb->snprintf(buf, sizeof(buf), "f: %d.%d Hz", scan_rate / 10,
                         scan_rate % 10);
            rb->lcd_putsxy(TEXT_X, TEXT_Y, buf);
            need_refresh = true;
            change = false;
        }
        button = rb->button_get(true);
        switch (button)
        {
          case SCANRATE_FASTINC:
          case SCANRATE_FASTINC|BUTTON_REPEAT:
            scan_rate += 10;
            change = true;
            break;

          case SCANRATE_FASTDEC:
          case SCANRATE_FASTDEC|BUTTON_REPEAT:
            scan_rate -= 10;
            change = true;
            break;

          case SCANRATE_INC:
          case SCANRATE_INC|BUTTON_REPEAT:
            scan_rate++;
            change = true;
            break;

          case SCANRATE_DEC:
          case SCANRATE_DEC|BUTTON_REPEAT:
            scan_rate--;
            change = true;
            break;

          case SCANRATE_DONE:
            done = true;
            break;
        }
    }
    rb->timer_unregister();
#ifdef NEED_BOOST
    rb->cpu_boost(false);
#endif

    rb->lcd_setfont(FONT_UI);

    return PLUGIN_OK;
}


/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    (void)parameter;
    rb = api;
    return plugin_main();
}

#endif
