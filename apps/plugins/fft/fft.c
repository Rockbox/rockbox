/***************************************************************************
*             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Delyan Kratunov
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

#include "lib/helper.h"
#include "lib/xlcd.h"
#include "math.h"
#include "thread.h"

#ifndef HAVE_LCD_COLOR
#include "lib/grey.h"
#endif

PLUGIN_HEADER

#ifndef HAVE_LCD_COLOR
GREY_INFO_STRUCT
#endif

#if CONFIG_KEYPAD == ARCHOS_AV300_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_F3
#   define FFT_WINDOW     BUTTON_F1
#   define FFT_SCALE       BUTTON_UP
#   define FFT_QUIT     BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_REC
#   define FFT_WINDOW     BUTTON_SELECT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_QUIT     BUTTON_OFF

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   define MINESWP_SCROLLWHEEL
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#   define FFT_WINDOW     (BUTTON_SELECT | BUTTON_RIGHT)
#   define FFT_SCALE         BUTTON_MENU
#   define FFT_QUIT       (BUTTON_SELECT | BUTTON_MENU)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW     BUTTON_PLAY
#   define FFT_SCALE       BUTTON_UP
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW        BUTTON_A
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW        BUTTON_REC
#   define FFT_SCALE       BUTTON_UP
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#   define FFT_WINDOW     (BUTTON_SELECT | BUTTON_RIGHT)
#   define FFT_SCALE       BUTTON_UP
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW        BUTTON_REC
#   define FFT_SCALE         BUTTON_SELECT
#   define FFT_QUIT       BUTTON_POWER
#elif (CONFIG_KEYPAD == SANSA_M200_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW        BUTTON_DOWN
#   define FFT_SCALE         BUTTON_SELECT
#   define FFT_QUIT       BUTTON_POWER
#elif (CONFIG_KEYPAD == SANSA_CLIP_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW        BUTTON_HOME
#   define FFT_SCALE         BUTTON_SELECT
#   define FFT_QUIT       BUTTON_POWER

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_FF
#   define FFT_WINDOW       BUTTON_SCROLL_UP
#   define FFT_SCALE            BUTTON_REW
#   define FFT_QUIT             BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_MENU
#   define FFT_WINDOW     BUTTON_PREV
#   define FFT_SCALE            BUTTON_UP
#   define FFT_QUIT             BUTTON_BACK

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_PLAY
#   define FFT_WINDOW     BUTTON_SELECT
#   define FFT_SCALE            BUTTON_UP
#   define FFT_QUIT             BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#   define FFT_PREV_GRAPH       BUTTON_RC_REW
#   define FFT_NEXT_GRAPH       BUTTON_RC_FF
#   define FFT_ORIENTATION      BUTTON_RC_MODE
#   define FFT_WINDOW        BUTTON_RC_PLAY
#   define FFT_SCALE            BUTTON_RC_VOL_UP
#   define FFT_QUIT             BUTTON_RC_REC

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
#   define FFT_QUIT             BUTTON_POWER
#   define FFT_PREV_GRAPH       BUTTON_PLUS
#   define FFT_NEXT_GRAPH       BUTTON_MINUS

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_MENU
#   define FFT_WINDOW     BUTTON_SELECT
#   define FFT_SCALE            BUTTON_UP
#   define FFT_QUIT             BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_SELECT
#   define FFT_WINDOW     BUTTON_MENU
#   define FFT_SCALE            BUTTON_UP
#   define FFT_QUIT             BUTTON_POWER

#elif (CONFIG_KEYPAD == SAMSUNG_YH_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW        BUTTON_DOWN
#   define FFT_SCALE         BUTTON_FFWD
#   define FFT_QUIT       BUTTON_PLAY

#elif (CONFIG_KEYPAD == MROBE500_PAD)
#   define FFT_QUIT             BUTTON_POWER

#elif (CONFIG_KEYPAD == ONDAVX747_PAD)
#   define FFT_QUIT             BUTTON_POWER

#elif (CONFIG_KEYPAD == ONDAVX777_PAD)
#   define FFT_QUIT             BUTTON_POWER

#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
#   define FFT_PREV_GRAPH       BUTTON_PREV
#   define FFT_NEXT_GRAPH       BUTTON_NEXT
#   define FFT_ORIENTATION      BUTTON_MENU
#   define FFT_WINDOW           BUTTON_OK
#   define FFT_SCALE            BUTTON_PLAY
#   define FFT_QUIT             BUTTON_REC

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#   define FFT_PREV_GRAPH       BUTTON_PREV
#   define FFT_NEXT_GRAPH       BUTTON_NEXT
#   define FFT_ORIENTATION      BUTTON_REC
#   define FFT_WINDOW           BUTTON_SELECT
#   define FFT_SCALE            BUTTON_PLAY
#   define FFT_QUIT             (BUTTON_REC | BUTTON_PLAY)

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef FFT_PREV_GRAPH
#   define FFT_PREV_GRAPH     BUTTON_MIDLEFT
#endif
#ifndef FFT_NEXT_GRAPH
#   define FFT_NEXT_GRAPH    BUTTON_MIDRIGHT
#endif
#ifndef FFT_ORIENTATION
#   define FFT_ORIENTATION  BUTTON_CENTER
#endif
#ifndef FFT_WINDOW
#   define FFT_WINDOW        BUTTON_TOPLEFT
#endif
#ifndef FFT_SCALE
#   define FFT_SCALE       BUTTON_TOPRIGHT
#endif
#ifndef FFT_QUIT
#   define FFT_QUIT     BUTTON_BOTTOMLEFT
#endif
#endif

#ifdef HAVE_LCD_COLOR
#include "pluginbitmaps/fft_colors.h"
#endif

#include "kiss_fftr.h"
#include "_kiss_fft_guts.h" /* sizeof(struct kiss_fft_state) */
#include "const.h"

#if (LCD_WIDTH < LCD_HEIGHT)
#define LCD_SIZE LCD_HEIGHT
#else
#define LCD_SIZE LCD_WIDTH
#endif

#if (LCD_SIZE < 512)
#define FFT_SIZE 2048 /* 512*4 */
#elif (LCD_SIZE < 1024)
#define FFT_SIZE 4096 /* 1024*4 */
#else
#define FFT_SIZE 8192 /* 2048*4 */
#endif

#define ARRAYSIZE_IN (FFT_SIZE)
#define ARRAYSIZE_OUT (FFT_SIZE/2)
#define ARRAYSIZE_PLOT (FFT_SIZE/4)
#define BUFSIZE_FFT (sizeof(struct kiss_fft_state)+sizeof(kiss_fft_cpx)*(FFT_SIZE-1))
#define BUFSIZE_FFTR (BUFSIZE_FFT+sizeof(struct kiss_fftr_state)+sizeof(kiss_fft_cpx)*(FFT_SIZE*3/2))
#define BUFSIZE BUFSIZE_FFTR
#define FFT_ALLOC kiss_fftr_alloc
#define FFT_FFT   kiss_fftr
#define FFT_CFG   kiss_fftr_cfg

#define __COEFF(type,size) type##_##size
#define _COEFF(x, y) __COEFF(x,y) /* force the preprocessor to evaluate FFT_SIZE) */
#define HANN_COEFF _COEFF(hann, FFT_SIZE)
#define HAMMING_COEFF _COEFF(hamming, FFT_SIZE)

/****************************** Globals ****************************/

static kiss_fft_scalar input[ARRAYSIZE_IN];
static kiss_fft_cpx output[ARRAYSIZE_OUT];
static int32_t plot[ARRAYSIZE_PLOT];
static char buffer[BUFSIZE];

#if LCD_DEPTH > 1 /* greyscale or color, enable spectrogram */
#define MODES_COUNT 3
#else
#define MODES_COUNT 2
#endif

const unsigned char* modes_text[] = { "Lines", "Bars", "Spectrogram" };
const unsigned char* scales_text[] = { "Linear scale", "Logarithmic scale" };
const unsigned char* window_text[] = { "Hamming window", "Hann window" };

struct mutex input_mutex;
bool input_thread_run = true;
bool input_thread_has_data = false;

struct {
    int32_t mode;
    bool logarithmic;
    bool orientation_vertical;
    int window_func;
    struct {
        int column;
        int row;
    } spectrogram;
    struct {
        bool orientation;
        bool mode;
        bool scale;
    } changed;
} graph_settings;

#define COLORS BMPWIDTH_fft_colors

/************************* End of globals *************************/

/************************* Math functions *************************/
#define QLOG_MAX 286286
#define QLIN_MAX 1534588906
#define QLN_10 float_q16(2.302585093)
#define LIN_MAX (QLIN_MAX >> 16)

/* Returns logarithmically scaled values in S15.16 format */
inline int32_t get_log_value(int32_t value)
{
    return Q16_DIV(fp16_log(value), QLN_10);
}

/* Apply window function to input
 * 0 - Hamming window
 * 1 - Hann window */
#define WINDOW_COUNT 2
void apply_window_func(char mode)
{
    switch(mode)
    {
        case 0: /* Hamming window */
        {
            size_t i;
            for (i = 0; i < ARRAYSIZE_IN; ++i)
            {
                input[i] = Q15_MUL(input[i] << 15, HAMMING_COEFF[i]) >> 15;
            } 
            break;
        }
        case 1: /* Hann window */
        {
            size_t i;
            for (i = 0; i < ARRAYSIZE_IN; ++i)
            {
                input[i] = Q15_MUL(input[i] << 15, HANN_COEFF[i]) >> 15;
            }
            break;
        }
    }
}

/* Calculates the magnitudes from complex numbers and returns the maximum */
int32_t calc_magnitudes(bool logarithmic)
{
    int64_t tmp;
    size_t i;

    int32_t max = -2147483647;
    
    /* Calculate the magnitude, discarding the phase.
     * The sum of the squares can easily overflow the 15-bit (s15.16)
     * requirement for fsqrt, so we scale the data down */
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        tmp = output[i].r * output[i].r + output[i].i * output[i].i;
        tmp <<= 16;

        tmp = fsqrt64(tmp, 16);

        if (logarithmic)
            tmp = get_log_value(tmp & 0x7FFFFFFF);

        plot[i] = tmp;

        if (plot[i] > max)
            max = plot[i];
    }
    return max;
}
/************************ End of math functions ***********************/

/********************* Plotting functions (modes) *********************/
void draw_lines_vertical(void);
void draw_lines_horizontal(void);
void draw_bars_vertical(void);
void draw_bars_horizontal(void);
void draw_spectrogram_vertical(void);
void draw_spectrogram_horizontal(void);

void draw(const unsigned char* message)
{
    static uint32_t show_message = 0;
    static unsigned char* last_message = 0;

    static char last_mode = 0;
    static bool last_orientation = true, last_scale = true;

    if (message != 0)
    {
        last_message = (unsigned char*) message;
        show_message = 5;
    }

    if(last_mode != graph_settings.mode)
    {
        last_mode = graph_settings.mode;
        graph_settings.changed.mode = true;
    }
    if(last_scale != graph_settings.logarithmic)
    {
        last_scale = graph_settings.logarithmic;
        graph_settings.changed.scale = true;
    }
    if(last_orientation != graph_settings.orientation_vertical)
    {
        last_orientation = graph_settings.orientation_vertical;
        graph_settings.changed.orientation = true;
    }
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#else
    grey_set_foreground(GREY_BLACK);
    grey_set_background(GREY_WHITE);
#endif

    switch (graph_settings.mode)
    {
        default:
        case 0: {

#ifdef HAVE_LCD_COLOR
            rb->lcd_clear_display();
#else
            grey_clear_display();
#endif

            if (graph_settings.orientation_vertical)
                draw_lines_vertical();
            else
                draw_lines_horizontal();
            break;
        }
        case 1: {

#ifdef HAVE_LCD_COLOR
            rb->lcd_clear_display();
#else
            grey_clear_display();
#endif

            if(graph_settings.orientation_vertical)
                draw_bars_vertical();
            else
                draw_bars_horizontal();

            break;
        }
        case 2: {
            if(graph_settings.orientation_vertical)
                draw_spectrogram_vertical();
            else
                draw_spectrogram_horizontal();
            break;
        }
    }

    if (show_message > 0)
    {
		/* We have a message to show */

        int x, y;
#ifdef HAVE_LCD_COLOR
        rb->lcd_getstringsize(last_message, &x, &y);
#else
        grey_getstringsize(last_message, &x, &y);
#endif
		/* x and y give the size of the box for the popup */
        x += 6; /* 3 px of horizontal padding and */
        y += 4; /* 2 px of vertical padding */

        /* In vertical spectrogram mode, leave space for the popup
		 * before actually drawing it (if space is needed) */
        if(graph_settings.mode == 2 &&
		   graph_settings.orientation_vertical &&
           graph_settings.spectrogram.column > LCD_WIDTH-x-2) 
		{
#ifdef HAVE_LCD_COLOR
			xlcd_scroll_left(graph_settings.spectrogram.column -
							 (LCD_WIDTH - x - 1));
#else
			grey_scroll_left(graph_settings.spectrogram.column -
							 (LCD_WIDTH - x - 1));
#endif
			graph_settings.spectrogram.column = LCD_WIDTH - x - 2;
		}

#ifdef HAVE_LCD_COLOR
        rb->lcd_set_foreground(LCD_DARKGRAY);
        rb->lcd_fillrect(LCD_WIDTH-1-x, 0, LCD_WIDTH-1, y);

        rb->lcd_set_foreground(LCD_DEFAULT_FG);
        rb->lcd_set_background(LCD_DARKGRAY);
        rb->lcd_putsxy(LCD_WIDTH-1-x+3, 2, last_message);
        rb->lcd_set_background(LCD_DEFAULT_BG);
#else
        grey_set_foreground(GREY_LIGHTGRAY);
        grey_fillrect(LCD_WIDTH-1-x, 0, LCD_WIDTH-1, y);

        grey_set_foreground(GREY_BLACK);
        grey_set_background(GREY_LIGHTGRAY);
        grey_putsxy(LCD_WIDTH-1-x+3, 2, last_message);
        grey_set_background(GREY_WHITE);
#endif

        show_message--;
    }
    else if(last_message != 0)
    {
		if(graph_settings.mode != 2)
		{
			/* These modes clear the screen themselves */
			last_message = 0;
		}
		else /* Spectrogram mode - need to erase the popup */
		{
			int x, y;
#ifdef HAVE_LCD_COLOR
			rb->lcd_getstringsize(last_message, &x, &y);
#else
			grey_getstringsize(last_message, &x, &y);
#endif
			/* Recalculate the size */
			x += 6; /* 3 px of horizontal padding and */
			y += 4; /* 2 px of vertical padding */

			if(!graph_settings.orientation_vertical)
			{
				/* In horizontal spectrogram mode, just scroll up by Y lines */
#ifdef HAVE_LCD_COLOR
				xlcd_scroll_up(y);
#else
				grey_scroll_up(y);
#endif
				graph_settings.spectrogram.row -= y;
				if(graph_settings.spectrogram.row < 0)
					graph_settings.spectrogram.row = 0;
			}
			else
			{
				/* In vertical spectrogram mode, erase the popup */
#ifdef HAVE_LCD_COLOR
				rb->lcd_set_foreground(LCD_DEFAULT_BG);
				rb->lcd_fillrect(LCD_WIDTH-2-x, 0, LCD_WIDTH-1, y);
				rb->lcd_set_foreground(LCD_DEFAULT_FG);
#else
				grey_set_foreground(GREY_WHITE);
				grey_fillrect(LCD_WIDTH-2-x, 0, LCD_WIDTH-1, y);
				grey_set_foreground(GREY_BLACK);
#endif
			}
			
			last_message = 0;
		}
    }
#ifdef HAVE_LCD_COLOR
    rb->lcd_update();
#else
    grey_update();
#endif

    graph_settings.changed.mode =  false;
    graph_settings.changed.orientation = false;
    graph_settings.changed.scale = false;
}

void draw_lines_vertical(void)
{
    static int32_t max = 0, vfactor = 0, vfactor_count = 0;
    static const int32_t hfactor =
            Q16_DIV(LCD_WIDTH << 16, (ARRAYSIZE_PLOT) << 16),
            bins_per_pixel = (ARRAYSIZE_PLOT) / LCD_WIDTH;
    static bool old_scale = true;

    if (old_scale != graph_settings.logarithmic)
        old_scale = graph_settings.logarithmic, max = 0; /* reset the graph on scaling mode change */

    int32_t new_max = calc_magnitudes(graph_settings.logarithmic);

    if (new_max > max)
    {
        max = new_max;
        vfactor = Q16_DIV(LCD_HEIGHT << 16, max); /* s15.16 */
        vfactor_count = Q16_DIV(vfactor, bins_per_pixel << 16); /* s15.16 */
    }

    if (new_max == 0 || max == 0) /* nothing to draw */
        return;

    /* take the average of neighboring bins
     * if we have to scale the graph horizontally */
    int64_t bins_avg = 0;
    bool draw = true;
    int32_t i;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        int32_t x = 0, y = 0;

        x = Q16_MUL(hfactor, i << 16) >> 16;
        //x = (x + (1 << 15)) >> 16;

        if (hfactor < 65536) /* hfactor < 0, graph compression */
        {
            draw = false;
            bins_avg += plot[i];

            /* fix the division by zero warning:
             * bins_per_pixel is zero when the graph is expanding;
             * execution won't even reach this point - this is a dummy constant
             */
            const int32_t div = bins_per_pixel > 0 ? bins_per_pixel : 1;
            if ((i + 1) % div == 0)
            {
                y = Q16_MUL(vfactor_count, bins_avg) >> 16;

                bins_avg = 0;
                draw = true;
            }
        }
        else
        {
            y = Q16_MUL(vfactor, plot[i]) >> 16;
            draw = true;
        }

        if (draw)
        {
#ifdef HAVE_LCD_COLOR
			rb->lcd_vline(x, LCD_HEIGHT-1, LCD_HEIGHT-y-1);
#else
			grey_vline(x, LCD_HEIGHT-1, LCD_HEIGHT-y-1);
#endif
        }
    }
}

void draw_lines_horizontal(void)
{
    static int max = 0;

    static const int32_t vfactor =
            Q16_DIV(LCD_HEIGHT << 16, (ARRAYSIZE_PLOT) << 16),
            bins_per_pixel = (ARRAYSIZE_PLOT) / LCD_HEIGHT;

    if (graph_settings.changed.scale)
        max = 0; /* reset the graph on scaling mode change */

    int32_t new_max = calc_magnitudes(graph_settings.logarithmic);

    if (new_max > max)
        max = new_max;

    if (new_max == 0 || max == 0) /* nothing to draw */
        return;

    int32_t hfactor;

    hfactor = Q16_DIV((LCD_WIDTH - 1) << 16, max); /* s15.16 */

    /* take the average of neighboring bins
     * if we have to scale the graph horizontally */
    int64_t bins_avg = 0;
    bool draw = true;
    int32_t i;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        int32_t x = 0, y = 0;

        y = Q16_MUL(vfactor, i << 16) + (1 << 15);
        y >>= 16;

        if (vfactor < 65536) /* vfactor < 0, graph compression */
        {
            draw = false;
            bins_avg += plot[i];

            /* fix the division by zero warning:
             * bins_per_pixel is zero when the graph is expanding;
             * execution won't even reach this point - this is a dummy constant
             */
            const int32_t div = bins_per_pixel > 0 ? bins_per_pixel : 1;
            if ((i + 1) % div == 0)
            {
                bins_avg = Q16_DIV(bins_avg, div << 16);
                x = Q16_MUL(hfactor, bins_avg) >> 16;

                bins_avg = 0;
                draw = true;
            }
        }
        else
        {
            y = Q16_MUL(hfactor, plot[i]) >> 16;
            draw = true;
        }

        if (draw)
        {
#ifdef HAVE_LCD_COLOR
			rb->lcd_hline(0, x, y);
#else
			grey_hline(0, x, y);
#endif
        }
    }
}

void draw_bars_vertical(void)
{
    static const unsigned int bars = 20, border = 2, items = ARRAYSIZE_PLOT
            / bars, width = (LCD_WIDTH - ((bars - 1) * border)) / bars;

    calc_magnitudes(graph_settings.logarithmic);

    uint64_t bars_values[bars], bars_max = 0, avg = 0;
    unsigned int i, bars_idx = 0;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        avg += plot[i];
        if ((i + 1) % items == 0)
        {
            /* Calculate the average value and keep the fractional part
             * for some added precision */
            avg = Q16_DIV(avg, items << 16);
            bars_values[bars_idx] = avg;

            if (bars_values[bars_idx] > bars_max)
                bars_max = bars_values[bars_idx];

            bars_idx++;
            avg = 0;
        }
    }

    if(bars_max == 0) /* nothing to draw */
        return;

    /* Give the graph some headroom */
    bars_max = Q16_MUL(bars_max, float_q16(1.1));

    uint64_t vfactor = Q16_DIV(LCD_HEIGHT << 16, bars_max);

    for (i = 0; i < bars; ++i)
    {
        int x = (i) * (border + width);
        int y;
        y = Q16_MUL(vfactor, bars_values[i]) + (1 << 15);
        y >>= 16;
#ifdef HAVE_LCD_COLOR
        rb->lcd_fillrect(x, LCD_HEIGHT - y - 1, width, y);
#else
        grey_fillrect(x, LCD_HEIGHT - y - 1, width, y);
#endif
    }
}

void draw_bars_horizontal(void)
{
    static const unsigned int bars = 14, border = 3, items = ARRAYSIZE_PLOT
            / bars, height = (LCD_HEIGHT - ((bars - 1) * border)) / bars;

    calc_magnitudes(graph_settings.logarithmic);

    int64_t bars_values[bars], bars_max = 0, avg = 0;
    unsigned int i, bars_idx = 0;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        avg += plot[i];
        if ((i + 1) % items == 0)
        {
            /* Calculate the average value and keep the fractional part
             * for some added precision */
            avg = Q16_DIV(avg, items << 16); /* s15.16 */
            bars_values[bars_idx] = avg;

            if (bars_values[bars_idx] > bars_max)
                bars_max = bars_values[bars_idx];

            bars_idx++;
            avg = 0;
        }
    }

    if(bars_max == 0) /* nothing to draw */
        return;

    /* Give the graph some headroom */
    bars_max = Q16_MUL(bars_max, float_q16(1.1));

    int64_t hfactor = Q16_DIV(LCD_WIDTH << 16, bars_max);

    for (i = 0; i < bars; ++i)
    {
        int y = (i) * (border + height);
        int x;
        x = Q16_MUL(hfactor, bars_values[i]) + (1 << 15);
        x >>= 16;

#ifdef HAVE_LCD_COLOR
        rb->lcd_fillrect(0, y, x, height);
#else
        grey_fillrect(0, y, x, height);
#endif
    }
}

void draw_spectrogram_vertical(void)
{
    const int32_t scale_factor = ARRAYSIZE_PLOT / LCD_HEIGHT
#ifdef HAVE_LCD_COLOR
        ,colors_per_val_log = Q16_DIV((COLORS-1) << 16, QLOG_MAX),
        colors_per_val_lin = Q16_DIV((COLORS-1) << 16, QLIN_MAX)
#else
		,grey_vals_per_val_log = Q16_DIV(255 << 16, QLOG_MAX),
		grey_vals_per_val_lin = Q16_DIV(255 << 16, QLIN_MAX)
#endif
		;

    const int32_t remaining_div =
        (ARRAYSIZE_PLOT-scale_factor*LCD_HEIGHT) > 0 ?
        ( Q16_DIV((scale_factor*LCD_HEIGHT) << 16,
                  (ARRAYSIZE_PLOT-scale_factor*LCD_HEIGHT) << 16)
         + (1<<15) ) >> 16 : 0;

    calc_magnitudes(graph_settings.logarithmic);
    if(graph_settings.changed.mode || graph_settings.changed.orientation)
    {
        graph_settings.spectrogram.column = 0;
#ifdef HAVE_LCD_COLOR
        rb->lcd_clear_display();
#else
        grey_clear_display();
#endif
    }

    int i, y = LCD_HEIGHT-1, count = 0, rem_count = 0;
    uint64_t avg = 0;
    bool added_extra_value = false;
    for(i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        if(plot[i] > 0)
            avg += plot[i];
        ++count;
        ++rem_count;

        /* Kinda hacky - due to the rounding in scale_factor, we try to
         * uniformly interweave the extra values in our calculations */
        if(remaining_div > 0 && rem_count >= remaining_div &&
                i < (ARRAYSIZE_PLOT-1))
        {
            ++i;
            if(plot[i] > 0)
                avg += plot[i];
            rem_count = 0;
            added_extra_value = true;
        }

        if(count >= scale_factor)
        {
            if(added_extra_value)
                { ++count; added_extra_value = false; }

            int32_t color;

            avg = Q16_DIV(avg, count << 16);

#ifdef HAVE_LCD_COLOR
            if(graph_settings.logarithmic)
                color = Q16_MUL(avg, colors_per_val_log) >> 16;
            else
                color = Q16_MUL(avg, colors_per_val_lin) >> 16;
            if(color >= COLORS) /* TODO These happen because we don't normalize the values to be above 1 and log() returns negative numbers. I think. */
                color = COLORS-1;
            else if (color < 0)
                color = 0;

#else
			if(graph_settings.logarithmic)
				color = Q16_MUL(avg, grey_vals_per_val_log) >> 16;
			else
				color = Q16_MUL(avg, grey_vals_per_val_lin) >> 16;
            if(color > 255) 
                color = 255;
            else if (color < 0)
                color = 0;
#endif

#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(fft_colors[color]);
            rb->lcd_drawpixel(graph_settings.spectrogram.column, y);
#else
            grey_set_foreground(255 - color);
            grey_drawpixel(graph_settings.spectrogram.column, y);
#endif

            y--;

            avg = 0;
            count = 0;
        }
        if(y < 0)
            break;
    }
    if(graph_settings.spectrogram.column != LCD_WIDTH-1)
        graph_settings.spectrogram.column++;
    else
#ifdef HAVE_LCD_COLOR
        xlcd_scroll_left(1);
#else
        grey_scroll_left(1);
#endif
}

void draw_spectrogram_horizontal(void)
{
    const int32_t scale_factor = ARRAYSIZE_PLOT / LCD_WIDTH
#ifdef HAVE_LCD_COLOR
        ,colors_per_val_log = Q16_DIV((COLORS-1) << 16, QLOG_MAX),
        colors_per_val_lin = Q16_DIV((COLORS-1) << 16, QLIN_MAX)
#else
		,grey_vals_per_val_log = Q16_DIV(255 << 16, QLOG_MAX),
		grey_vals_per_val_lin = Q16_DIV(255 << 16, QLIN_MAX)
#endif
		;

    const int32_t remaining_div =
            (ARRAYSIZE_PLOT-scale_factor*LCD_WIDTH) > 0 ?
            ( Q16_DIV((scale_factor*LCD_WIDTH) << 16,
                      (ARRAYSIZE_PLOT-scale_factor*LCD_WIDTH) << 16)
             + (1<<15) ) >> 16 : 0;

    calc_magnitudes(graph_settings.logarithmic);
    if(graph_settings.changed.mode || graph_settings.changed.orientation)
    {
        graph_settings.spectrogram.row = 0;
#ifdef HAVE_LCD_COLOR
        rb->lcd_clear_display();
#else
        grey_clear_display();
#endif
    }

    int i, x = 0, count = 0, rem_count = 0;
    uint64_t avg = 0;
    bool added_extra_value = false;
    for(i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        if(plot[i] > 0)
            avg += plot[i];
        ++count;
        ++rem_count;

        /* Kinda hacky - due to the rounding in scale_factor, we try to
         * uniformly interweave the extra values in our calculations */
        if(remaining_div > 0 && rem_count >= remaining_div &&
                i < (ARRAYSIZE_PLOT-1))
        {
            ++i;
            if(plot[i] > 0)
                avg += plot[i];
            rem_count = 0;
            added_extra_value = true;
        }

        if(count >= scale_factor)
        {
            if(added_extra_value)
                { ++count; added_extra_value = false; }

            int32_t color;

            avg = Q16_DIV(avg, count << 16);

#ifdef HAVE_LCD_COLOR
            if(graph_settings.logarithmic)
                color = Q16_MUL(avg, colors_per_val_log) >> 16;
            else
                color = Q16_MUL(avg, colors_per_val_lin) >> 16;
            if(color >= COLORS) /* TODO same as _vertical */
                color = COLORS-1;
            else if (color < 0)
                color = 0;

#else
			if(graph_settings.logarithmic)
				color = Q16_MUL(avg, grey_vals_per_val_log) >> 16;
			else
				color = Q16_MUL(avg, grey_vals_per_val_lin) >> 16;
            if(color > 255) 
				color = 255;
            else if (color < 0)
                color = 0;
#endif

#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(fft_colors[color]);
            rb->lcd_drawpixel(x, graph_settings.spectrogram.row);
#else
            grey_set_foreground(255 - color);
            grey_drawpixel(x, graph_settings.spectrogram.row);
#endif

            x++;

            avg = 0;
            count = 0;
        }
        if(x >= LCD_WIDTH)
            break;
    }
    if(graph_settings.spectrogram.row != LCD_HEIGHT-1)
        graph_settings.spectrogram.row++;
    else
#ifdef HAVE_LCD_COLOR
        xlcd_scroll_up(1);
#else
        grey_scroll_up(1);
#endif
}

/********************* End of plotting functions (modes) *********************/

static long thread_stack[DEFAULT_STACK_SIZE/sizeof(long)];
void input_thread_entry(void)
{
	kiss_fft_scalar * value;
	kiss_fft_scalar left;
	int count;
	int idx = 0; /* offset in the buffer */
	int fft_idx = 0; /* offset in input */
	while(true)
	{
		rb->mutex_lock(&input_mutex);
		if(!input_thread_run)
			rb->thread_exit();

		value = (kiss_fft_scalar*) rb->pcm_get_peak_buffer(&count);
		
		if (value == 0 || count == 0)
		{
			rb->mutex_unlock(&input_mutex);
			rb->yield();
			continue;
			/* This block can introduce discontinuities in our data. Meaning, the FFT
			 * will not be done a continuous segment of the signal. Which can be bad. Or not.
			 * 
			 * Anyway, this is a demo, not a scientific tool. If you want accuracy, do a proper
			 * spectrum analysis.*/
		}
		else
		{
			idx = fft_idx = 0;
			do
			{
				left = *(value + idx);
				idx += 2;

				input[fft_idx] = left;
				fft_idx++;
				input[fft_idx] = 0;
				fft_idx++;

				if (fft_idx == ARRAYSIZE_IN)
					break;
			} while (idx < count);
		}
		if(fft_idx == ARRAYSIZE_IN) /* there are cases when we don't have enough data to fill the buffer */
		    input_thread_has_data = true;	
		
		rb->mutex_unlock(&input_mutex);
		rb->yield();
	}
}


enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;
    if ((rb->audio_status() & AUDIO_STATUS_PLAY) == 0)
    {
        rb->splash(HZ * 2, "No track playing. Exiting..");
        return PLUGIN_OK;
    }
#ifndef HAVE_LCD_COLOR
    unsigned char *gbuf;
    size_t  gbuf_size = 0;
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the greyscale buffer.*/
    if (!grey_init(gbuf, gbuf_size, GREY_ON_COP | GREY_BUFFERED,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Couldn't init greyscale display");
        return PLUGIN_ERROR;
    }
    grey_show(true); 
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    backlight_force_on();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    rb->mutex_init(&input_mutex);

    /* Defaults */
    bool run = true;
    graph_settings.mode = 0;
    graph_settings.logarithmic = true;
    graph_settings.orientation_vertical = true;
    graph_settings.window_func = 0;
    graph_settings.changed.mode = false;
    graph_settings.changed.scale = false;
    graph_settings.changed.orientation = false;
    graph_settings.spectrogram.row = 0;
    graph_settings.spectrogram.column = 0;

    bool changed_window = false;

    size_t size = sizeof(buffer);
    FFT_CFG state = FFT_ALLOC(FFT_SIZE, 0, buffer, &size);

    if (state == 0)
    {
        DEBUGF("needed data: %i", (int) size);
        return PLUGIN_ERROR;
    }
	
	unsigned int input_thread = rb->create_thread(&input_thread_entry, thread_stack, sizeof(thread_stack), 0, "fft input thread" IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
	rb->yield();
    while (run)
    {
		rb->mutex_lock(&input_mutex);
		if(!input_thread_has_data)
		{
			/* Make sure the input thread has started before doing anything else */
			rb->mutex_unlock(&input_mutex);
			rb->yield();
			continue;
		}
		apply_window_func(graph_settings.window_func);
		FFT_FFT(state, input, output);

		if(changed_window)
		{
			draw(window_text[graph_settings.window_func]);
			changed_window = false;
		}
		else
			draw(0);

		input_thread_has_data = false;
        rb->mutex_unlock(&input_mutex);
		rb->yield();

		int button = rb->button_get(false);
        switch (button)
        {
            case FFT_QUIT:
                run = false;
                break;
            case FFT_PREV_GRAPH: {
                graph_settings.mode--;
                if (graph_settings.mode < 0)
                    graph_settings.mode = MODES_COUNT-1;
                draw(modes_text[graph_settings.mode]);
                break;
            }
            case FFT_NEXT_GRAPH: {
                graph_settings.mode++;
                if (graph_settings.mode >= MODES_COUNT)
                    graph_settings.mode = 0;
                draw(modes_text[graph_settings.mode]);
                break;
            }
            case FFT_WINDOW: {
                changed_window = true;
                graph_settings.window_func ++;
                if(graph_settings.window_func >= WINDOW_COUNT)
                    graph_settings.window_func = 0;
                break;
            }
            case FFT_SCALE: {
                graph_settings.logarithmic = !graph_settings.logarithmic;
                draw(scales_text[graph_settings.logarithmic ? 1 : 0]);
                break;
            }
            case FFT_ORIENTATION: {
                graph_settings.orientation_vertical = !graph_settings.orientation_vertical;
                draw(0);
                break;
            }
            default: {
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
            }

        }
    }
	
	/* Handle our input thread. We haven't yield()'d since our last mutex_unlock, so we know we have the mutex */
	rb->mutex_lock(&input_mutex);
	input_thread_run = false;
	rb->mutex_unlock(&input_mutex);
	rb->thread_wait(input_thread);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
#ifndef HAVE_LCD_COLOR
    grey_release();
#endif
    backlight_use_settings();
    return PLUGIN_OK;
}
