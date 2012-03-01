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
#include "lib/pluginlib_actions.h"

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
#define SCANRATE_QUIT            PLA_EXIT
#define SCANRATE_QUIT2           PLA_CANCEL
#define SCANRATE_FASTINC         PLA_UP
#define SCANRATE_FASTINC_REPEAT  PLA_UP_REPEAT
#define SCANRATE_FASTDEC         PLA_DOWN
#define SCANRATE_FASTDEC_REPEAT  PLA_DOWN_REPEAT

#ifdef HAVE_SCROLLWHEEL
#define SCANRATE_INC             PLA_SCROLL_FWD
#define SCANRATE_INC_REPEAT      PLA_SCROLL_FWD_REPEAT
#define SCANRATE_DEC             PLA_SCROLL_BACK
#define SCANRATE_DEC_REPEAT      PLA_SCROLL_BACK_REPEAT
#else
#define SCANRATE_INC             PLA_RIGHT
#define SCANRATE_INC_REPEAT      PLA_RIGHT_REPEAT
#define SCANRATE_DEC             PLA_LEFT
#define SCANRATE_DEC_REPEAT      PLA_LEFT_REPEAT
#endif /*HAVE_SCROLLWHEEL*/

/* Default refresh rates in 1/10 Hz */
#if defined ARCHOS_RECORDER   || defined ARCHOS_FMRECORDER  \
 || defined ARCHOS_RECORDERV2 || defined ARCHOS_ONDIOFM     \
 || defined ARCHOS_ONDIOSP
#define DEFAULT_SCAN_RATE 670
#elif defined IAUDIO_M3
#define DEFAULT_SCAN_RATE 1500
#define HORIZ_SCAN /* LCD controller updates the panel sideways */
#define NEED_BOOST
#elif defined MPIO_HD200
#define DEFAULT_SCAN_RATE 1460
#define NEED_BOOST
#elif defined MPIO_HD300
#define DEFAULT_SCAN_RATE 730
#elif defined IAUDIO_M5
#define DEFAULT_SCAN_RATE 730
#elif defined IPOD_1G2G
#define DEFAULT_SCAN_RATE 960
#elif defined IPOD_MINI2G  || defined IPOD_MINI  \
   || defined IPOD_3G      || defined IPOD_4G
#define DEFAULT_SCAN_RATE 870
#elif defined IRIVER_H100_SERIES
#define DEFAULT_SCAN_RATE 700
#elif defined SANSA_CLIP
#define DEFAULT_SCAN_RATE 780
#elif defined SAMSUNG_YH920
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
    rb->timer_register(1, NULL, TIMER_FREQ * 5 / scan_rate,
                       timer_isr IF_COP(, CPU));

    while (!done)
    {
        if (change)
        {
            /* The actual frequency is twice the displayed value */
            rb->timer_set_period(TIMER_FREQ * 5 / scan_rate);
            rb->lcd_putsxyf(TEXT_X, TEXT_Y, "f: %d.%d Hz", scan_rate / 10,
                         scan_rate % 10);
            need_refresh = true;
            change = false;
        }
        button = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts,
                          ARRAYLEN(plugin_contexts));
        switch (button)
        {
          case SCANRATE_FASTINC:
          case SCANRATE_FASTINC_REPEAT:
            scan_rate += 10;
            change = true;
            break;

          case SCANRATE_FASTDEC:
          case SCANRATE_FASTDEC_REPEAT:
            scan_rate -= 10;
            change = true;
            break;

          case SCANRATE_INC:
          case SCANRATE_INC_REPEAT:
            scan_rate++;
            change = true;
            break;

          case SCANRATE_DEC:
          case SCANRATE_DEC_REPEAT:
            scan_rate--;
            change = true;
            break;

          case SCANRATE_QUIT:
          case SCANRATE_QUIT2:
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
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    return plugin_main();
}
