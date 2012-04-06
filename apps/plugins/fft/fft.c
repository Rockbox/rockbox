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
#include "fracmul.h"
#ifndef HAVE_LCD_COLOR
#include "lib/grey.h"
#endif
#include "lib/mylcd.h"



#ifndef HAVE_LCD_COLOR
GREY_INFO_STRUCT
#endif

#if CONFIG_KEYPAD == ARCHOS_AV300_PAD
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_F3
#   define FFT_WINDOW       BUTTON_F1
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_QUIT         BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_REC
#   define FFT_WINDOW       BUTTON_SELECT
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_OFF

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   define MINESWP_SCROLLWHEEL
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#   define FFT_WINDOW       (BUTTON_SELECT | BUTTON_RIGHT)
#   define FFT_AMP_SCALE    BUTTON_MENU
#   define FFT_FREQ_SCALE   BUTTON_PLAY
#   define FFT_QUIT         (BUTTON_SELECT | BUTTON_MENU)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW       BUTTON_PLAY
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW       BUTTON_A
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW       BUTTON_REC
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#   define FFT_WINDOW       (BUTTON_SELECT | BUTTON_RIGHT)
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT     (BUTTON_HOME|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW       BUTTON_REC
#   define FFT_AMP_SCALE    BUTTON_SELECT
#   define FFT_QUIT         BUTTON_POWER
#elif (CONFIG_KEYPAD == SANSA_M200_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW       BUTTON_DOWN
#   define FFT_AMP_SCALE    BUTTON_SELECT
#   define FFT_QUIT         BUTTON_POWER
#elif (CONFIG_KEYPAD == SANSA_CLIP_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW       BUTTON_HOME
#   define FFT_AMP_SCALE    BUTTON_SELECT
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_FF
#   define FFT_WINDOW       BUTTON_SCROLL_UP
#   define FFT_AMP_SCALE    BUTTON_REW
#   define FFT_FREQ_SCALE   BUTTON_PLAY
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_MENU
#   define FFT_WINDOW       BUTTON_PREV
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_BACK

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_PLAY
#   define FFT_WINDOW       BUTTON_SELECT
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#   define FFT_PREV_GRAPH   BUTTON_RC_REW
#   define FFT_NEXT_GRAPH   BUTTON_RC_FF
#   define FFT_ORIENTATION  BUTTON_RC_MODE
#   define FFT_WINDOW       BUTTON_RC_PLAY
#   define FFT_AMP_SCALE    BUTTON_RC_VOL_UP
#   define FFT_QUIT         BUTTON_RC_REC

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
#   define FFT_QUIT         BUTTON_POWER
#   define FFT_PREV_GRAPH   BUTTON_PLUS
#   define FFT_NEXT_GRAPH   BUTTON_MINUS

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_MENU
#   define FFT_WINDOW       BUTTON_SELECT
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW       BUTTON_MENU
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_POWER

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#   define FFT_PREV_GRAPH   BUTTON_PREV
#   define FFT_NEXT_GRAPH   BUTTON_NEXT
#   define FFT_ORIENTATION  BUTTON_PLAY
#   define FFT_WINDOW       BUTTON_MENU
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_POWER

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#   define FFT_PREV_GRAPH   BUTTON_PREV
#   define FFT_NEXT_GRAPH   BUTTON_NEXT
#   define FFT_ORIENTATION  BUTTON_PLAY
#   define FFT_WINDOW       BUTTON_MENU
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == SAMSUNG_YH_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW       BUTTON_DOWN
#   define FFT_AMP_SCALE    BUTTON_FFWD
#   define FFT_QUIT         BUTTON_PLAY

#elif (CONFIG_KEYPAD == MROBE500_PAD)
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == ONDAVX747_PAD)
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == ONDAVX777_PAD)
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
#   define FFT_PREV_GRAPH   BUTTON_PREV
#   define FFT_NEXT_GRAPH   BUTTON_NEXT
#   define FFT_ORIENTATION  BUTTON_MENU
#   define FFT_WINDOW       BUTTON_OK
#   define FFT_AMP_SCALE    BUTTON_PLAY
#   define FFT_QUIT         BUTTON_REC

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#   define FFT_PREV_GRAPH   BUTTON_REW
#   define FFT_NEXT_GRAPH   BUTTON_FF
#   define FFT_ORIENTATION  BUTTON_REC
#   define FFT_WINDOW       BUTTON_FUNC
#   define FFT_AMP_SCALE    BUTTON_PLAY
#   define FFT_QUIT        (BUTTON_REC | BUTTON_PLAY)

#elif CONFIG_KEYPAD == MPIO_HD300_PAD
#   define FFT_PREV_GRAPH   BUTTON_REW
#   define FFT_NEXT_GRAPH   BUTTON_FF
#   define FFT_ORIENTATION  BUTTON_REC
#   define FFT_WINDOW       BUTTON_ENTER
#   define FFT_AMP_SCALE    BUTTON_PLAY
#   define FFT_QUIT        (BUTTON_REC | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_BACK
#   define FFT_WINDOW       BUTTON_SELECT
#   define FFT_AMP_SCALE    BUTTON_PLAYPAUSE
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_CONNECT_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW       BUTTON_VOL_DOWN
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YPR0_PAD
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_USER
#   define FFT_WINDOW       BUTTON_MENU
#   define FFT_AMP_SCALE    BUTTON_SELECT
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_QUIT         BUTTON_BACK

#elif (CONFIG_KEYPAD == HM60X_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW       (BUTTON_POWER|BUTTON_SELECT)
#   define FFT_QUIT         BUTTON_POWER

#elif (CONFIG_KEYPAD == HM801_PAD)
#   define FFT_PREV_GRAPH   BUTTON_LEFT
#   define FFT_NEXT_GRAPH   BUTTON_RIGHT
#   define FFT_AMP_SCALE    BUTTON_UP
#   define FFT_FREQ_SCALE   BUTTON_DOWN
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW       BUTTON_PLAY
#   define FFT_QUIT         BUTTON_POWER

#elif !defined(HAVE_TOUCHSCREEN)
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef FFT_PREV_GRAPH
#   define FFT_PREV_GRAPH   BUTTON_MIDLEFT
#endif
#ifndef FFT_NEXT_GRAPH
#   define FFT_NEXT_GRAPH   BUTTON_MIDRIGHT
#endif
#ifndef FFT_ORIENTATION
#   define FFT_ORIENTATION  BUTTON_CENTER
#endif
#ifndef FFT_WINDOW
#   define FFT_WINDOW       BUTTON_TOPLEFT
#endif
#ifndef FFT_AMP_SCALE
#   define FFT_AMP_SCALE    BUTTON_TOPRIGHT
#endif
#ifndef FFT_QUIT
#   define FFT_QUIT         BUTTON_BOTTOMLEFT
#endif
#endif /* HAVE_TOUCHSCREEN */

#ifdef HAVE_LCD_COLOR
#include "pluginbitmaps/fft_colors.h"
#endif

#include "kiss_fftr.h"
#include "_kiss_fft_guts.h" /* sizeof(struct kiss_fft_state) */
#include "const.h"

#define LCD_SIZE MAX(LCD_WIDTH, LCD_HEIGHT)

#if (LCD_SIZE <= 511)
#define FFT_SIZE 1024 /* 512*2 */
#elif (LCD_SIZE <= 1023)
#define FFT_SIZE 2048 /* 1024*2 */
#else
#define FFT_SIZE 4096 /* 2048*2 */
#endif

#define ARRAYLEN_IN (FFT_SIZE)
#define ARRAYLEN_OUT (FFT_SIZE)
#define ARRAYLEN_PLOT (FFT_SIZE/2-1) /* FFT is symmetric, ignore DC */
#define BUFSIZE_FFT (sizeof(struct kiss_fft_state)+sizeof(kiss_fft_cpx)*(FFT_SIZE-1))

#define __COEFF(type,size) type##_##size
#define _COEFF(x, y) __COEFF(x,y) /* force the preprocessor to evaluate FFT_SIZE) */
#define HANN_COEFF _COEFF(hann, FFT_SIZE)
#define HAMMING_COEFF _COEFF(hamming, FFT_SIZE)

/****************************** Globals ****************************/
/* cacheline-aligned buffers with COP, otherwise word-aligned */
/* CPU/COP only applies when compiled for more than one core */

#define CACHEALIGN_UP_SIZE(type, len) \
    (CACHEALIGN_UP((len)*sizeof(type) + (sizeof(type)-1)) / sizeof(type))
/* Shared */
/* COP + CPU PCM */
static kiss_fft_cpx input[CACHEALIGN_UP_SIZE(kiss_fft_scalar, ARRAYLEN_IN)]
                            CACHEALIGN_AT_LEAST_ATTR(4);
/* CPU+COP */
#if NUM_CORES > 1
/* Output queue indexes */
static volatile int output_head SHAREDBSS_ATTR = 0;
static volatile int output_tail SHAREDBSS_ATTR = 0;
/* The result is nfft/2 complex frequency bins from DC to Nyquist. */
static kiss_fft_cpx output[2][CACHEALIGN_UP_SIZE(kiss_fft_cpx, ARRAYLEN_OUT)]
                                SHAREDBSS_ATTR;
#else
/* Only one output buffer */
#define output_head 0
#define output_tail 0
/* The result is nfft/2 complex frequency bins from DC to Nyquist. */
static kiss_fft_cpx output[1][ARRAYLEN_OUT];
#endif

/* Unshared */
/* COP */
static kiss_fft_cfg fft_state SHAREDBSS_ATTR;
static char fft_buffer[CACHEALIGN_UP_SIZE(char, BUFSIZE_FFT)]
                CACHEALIGN_AT_LEAST_ATTR(4);
/* CPU */
static int32_t plot_history[ARRAYLEN_PLOT];
static int32_t plot[ARRAYLEN_PLOT];
static struct
{
    int16_t bin;   /* integer bin number */
    uint16_t frac; /* interpolation fraction */
} binlog[ARRAYLEN_PLOT] __attribute__((aligned(4)));

enum fft_window_func
{
    FFT_WF_FIRST = 0,
    FFT_WF_HAMMING = 0,
    FFT_WF_HANN,
};
#define FFT_WF_COUNT (FFT_WF_HANN+1)

enum fft_display_mode
{
    FFT_DM_FIRST = 0,
    FFT_DM_LINES = 0,
    FFT_DM_BARS,
    FFT_DM_SPECTROGRAPH,
};
#define FFT_DM_COUNT (FFT_DM_SPECTROGRAPH+1)

static const unsigned char* const modes_text[FFT_DM_COUNT] =
{ "Lines", "Bars", "Spectrogram" };

static const unsigned char* const amp_scales_text[2] =
{ "Linear amplitude", "Logarithmic amplitude" };

static const unsigned char* const freq_scales_text[2] =
{ "Linear frequency", "Logarithmic frequency" };

static const unsigned char* const window_text[FFT_WF_COUNT] =
{ "Hamming window", "Hann window" };

static struct {
    bool orientation_vertical;
    enum fft_display_mode mode;
    bool logarithmic_amp;
    bool logarithmic_freq;
    enum fft_window_func window_func;
    int spectrogram_pos; /* row or column - only used by one at a time */
    union
    {
        struct
        {
            bool orientation : 1;
            bool mode        : 1;
            bool amp_scale   : 1;
            bool freq_scale  : 1;
            bool window_func : 1;
            bool do_clear    : 1;
        };
        bool clear_all; /* Write 'false' to clear all above */
    } changed;
} graph_settings SHAREDDATA_ATTR = 
{
     /* Defaults */
    .orientation_vertical = true,
    .mode                 = FFT_DM_LINES,
    .logarithmic_amp      = true,
    .logarithmic_freq     = true,
    .window_func          = FFT_WF_HAMMING,
    .spectrogram_pos      = 0,
    .changed              = { .clear_all = false },
};

#ifdef HAVE_LCD_COLOR
#define SHADES BMPWIDTH_fft_colors
#define SPECTROGRAPH_PALETTE(index) (fft_colors[index])
#else
#define SHADES 256
#define SPECTROGRAPH_PALETTE(index) (255 - (index))
#endif

/************************* End of globals *************************/

/************************* Math functions *************************/

/* Based on feeding-in a 0db sinewave at FS/4 */
#define QLOG_MAX 0x0009154B
/* fudge it a little or it's not very visbile */
#define QLIN_MAX (0x00002266 >> 1)

/* Apply window function to input */
static void apply_window_func(enum fft_window_func mode)
{
    int i;

    switch(mode)
    {
        case FFT_WF_HAMMING:
            for(i = 0; i < ARRAYLEN_IN; ++i)
            {
                input[i].r = (input[i].r * HAMMING_COEFF[i] + 16384) >> 15;
            } 
            break;

        case FFT_WF_HANN:
            for(i = 0; i < ARRAYLEN_IN; ++i)
            {
                input[i].r = (input[i].r * HANN_COEFF[i] + 16384) >> 15;
            }
            break;
    }
}

/* Calculates the magnitudes from complex numbers and returns the maximum */
static int32_t calc_magnitudes(bool logarithmic_amp)
{
    /* A major assumption made when calculating the Q*MAX constants 
     * is that the maximum magnitude is 29 bits long. */
    uint32_t max = 0;
    kiss_fft_cpx *this_output = output[output_head] + 1; /* skip DC */
    int i;

    /* Calculate the magnitude, discarding the phase. */
    for(i = 0; i < ARRAYLEN_PLOT; ++i)
    {
        int32_t re = this_output[i].r;
        int32_t im = this_output[i].i;

        uint32_t tmp = re*re + im*im;

        if(tmp > 0)
        {
            if(tmp > 0x7FFFFFFF) /* clip */
            {
                tmp = 0x7FFFFFFF; /* if our assumptions are correct,
                                     this should never happen. It's just
                                     a safeguard. */
            }

            if(logarithmic_amp)
            {
                if(tmp < 0x8000) /* be more precise */
                {
                    /* ln(x ^ .5) = .5*ln(x) */
                    tmp = fp16_log(tmp << 16) >> 1;
                }
                else
                {
                    tmp = isqrt(tmp); /* linear scaling, nothing
                                         bad should happen */
                    tmp = fp16_log(tmp << 16); /* the log function
                                                  expects s15.16 values */
                }
            }
            else
            {
                tmp = isqrt(tmp); /* linear scaling, nothing
                                     bad should happen */
            }
        }

        /* Length 2 moving average - last transform and this one */
        tmp = (plot_history[i] + tmp) >> 1;
        plot[i] = tmp;
        plot_history[i] = tmp;

        if(tmp > max)
            max = tmp;
    }

    return max;
}

/* Move plot bins into a logarithmic scale by sliding them towards the
 * Nyquist bin according to the translation in the binlog array. */
static void logarithmic_plot_translate(void)
{
    int i;

    for(i = ARRAYLEN_PLOT-1; i > 0; --i)
    {
        int bin;
        int s = binlog[i].bin;
        int e = binlog[i-1].bin;
        int frac = binlog[i].frac;

        bin = plot[s];

        if(frac)
        {
            /* slope < 1, Interpolate stretched bins (linear for now) */
            int diff = plot[s+1] - bin;

            do
            {
                plot[i] = bin + FRACMUL(frac << 15, diff);
                frac = binlog[--i].frac;
            }
            while(frac);
        }
        else
        {
            /* slope > 1, Find peak of two or more bins */
            while(--s > e)
            {
                int val = plot[s];

                if (val > bin)
                    bin = val;
            }
        }

        plot[i] = bin;
    }
}

/* Calculates the translation for logarithmic plot bins */
static void logarithmic_plot_init(void)
{
    int i, j;
    /*
     * log: y = round(n * ln(x) / ln(n))
     * anti: y = round(exp(x * ln(n) / n))
     */
    j = fp16_log((ARRAYLEN_PLOT - 1) << 16);
    for(i = 0; i < ARRAYLEN_PLOT; ++i)
    {
        binlog[i].bin = (fp16_exp(i * j / (ARRAYLEN_PLOT - 1)) + 32768) >> 16;
    }

    /* setup fractions for interpolation of stretched bins */
    for(i = 0; i < ARRAYLEN_PLOT-1; i = j)
    {
        j = i + 1;

        /* stop when we have two different values */
        while(binlog[j].bin == binlog[i].bin)
            j++; /* if here, local slope of curve is < 1 */

        if(j > i + 1)
        {
            /* distribute pieces evenly over stretched interval */
            int diff = j - i;
            int x = 0;
            do
            {
                binlog[i].frac = (x++ << 16) / diff;
            }
            while(++i < j);
        }
    }
}

/************************ End of math functions ***********************/

/********************* Plotting functions (modes) *********************/
static void draw_lines_vertical(void);
static void draw_lines_horizontal(void);
static void draw_bars_vertical(void);
static void draw_bars_horizontal(void);
static void draw_spectrogram_vertical(void);
static void draw_spectrogram_horizontal(void);

#define COLOR_DEFAULT_FG    MYLCD_DEFAULT_FG
#define COLOR_DEFAULT_BG    MYLCD_DEFAULT_BG

#ifdef HAVE_LCD_COLOR
#define COLOR_MESSAGE_FRAME LCD_RGBPACK(0xc6, 0x00, 0x00)
#define COLOR_MESSAGE_BG    LCD_BLACK
#define COLOR_MESSAGE_FG    LCD_WHITE
#else
#define COLOR_MESSAGE_FRAME GREY_DARKGRAY
#define COLOR_MESSAGE_BG    GREY_WHITE
#define COLOR_MESSAGE_FG    GREY_BLACK
#endif

#define POPUP_HPADDING      3 /* 3 px of horizontal padding and */
#define POPUP_VPADDING      2 /* 2 px of vertical padding */

static void draw_message_string(const unsigned char *message, bool active)
{
    int x, y;
    mylcd_getstringsize(message, &x, &y);

    /* x and y give the size of the box for the popup */
    x += POPUP_HPADDING*2;
    y += POPUP_VPADDING*2;

    /* In vertical spectrogram mode, leave space for the popup
     * before actually drawing it (if space is needed) */
    if(active &&
       graph_settings.mode == FFT_DM_SPECTROGRAPH &&
       graph_settings.orientation_vertical &&
       graph_settings.spectrogram_pos >= LCD_WIDTH - x) 
    {
        mylcd_scroll_left(graph_settings.spectrogram_pos -
                          LCD_WIDTH + x);
        graph_settings.spectrogram_pos = LCD_WIDTH - x - 1;
    }

    mylcd_set_foreground(COLOR_MESSAGE_FRAME);
    mylcd_fillrect(LCD_WIDTH - x, 0, LCD_WIDTH - 1, y);

    mylcd_set_foreground(COLOR_MESSAGE_FG);
    mylcd_set_background(COLOR_MESSAGE_BG);
    mylcd_putsxy(LCD_WIDTH - x + POPUP_HPADDING,
                 POPUP_VPADDING, message);
    mylcd_set_foreground(COLOR_DEFAULT_FG);
    mylcd_set_background(COLOR_DEFAULT_BG);
}

static void draw(const unsigned char* message)
{
    static long show_message_tick = 0;
    static const unsigned char* last_message = 0;

    if(message != NULL)
    {
        last_message = message;
        show_message_tick = (*rb->current_tick + HZ) | 1;
    }

    /* maybe take additional actions depending upon the changed setting */
    if(graph_settings.changed.orientation)
    {
       graph_settings.changed.amp_scale = true;
       graph_settings.changed.do_clear = true;
    }

    if(graph_settings.changed.mode)
    {
        graph_settings.changed.amp_scale = true;
        graph_settings.changed.do_clear = true;
    }

    if(graph_settings.changed.amp_scale)
        memset(plot_history, 0, sizeof (plot_history));

    if(graph_settings.changed.freq_scale)
        graph_settings.changed.freq_scale = true;

    mylcd_set_foreground(COLOR_DEFAULT_FG);
    mylcd_set_background(COLOR_DEFAULT_BG);

    switch (graph_settings.mode)
    {
        default:
        case FFT_DM_LINES: {

            mylcd_clear_display();

            if (graph_settings.orientation_vertical)
                draw_lines_vertical();
            else
                draw_lines_horizontal();
            break;
        }
        case FFT_DM_BARS: {

            mylcd_clear_display();

            if(graph_settings.orientation_vertical)
                draw_bars_vertical();
            else
                draw_bars_horizontal();

            break;
        }
        case FFT_DM_SPECTROGRAPH: {

            if(graph_settings.changed.do_clear)
            {
                graph_settings.spectrogram_pos = 0;
                mylcd_clear_display();
            }

            if(graph_settings.orientation_vertical)
                draw_spectrogram_vertical();
            else
                draw_spectrogram_horizontal();
            break;
        }
    }

    if(show_message_tick != 0)
    {
        if(TIME_BEFORE(*rb->current_tick, show_message_tick))
        {
    		/* We have a message to show */
            draw_message_string(last_message, true);
        }
        else
        {
            /* Stop drawing message */
            show_message_tick = 0;
        }
    }
    else if(last_message != NULL)
    {
        if(graph_settings.mode == FFT_DM_SPECTROGRAPH)
        {
    		/* Spectrogram mode - need to erase the popup */
			int x, y;
			mylcd_getstringsize(last_message, &x, &y);
			/* Recalculate the size */
			x += POPUP_HPADDING*2;
			y += POPUP_VPADDING*2;

			if(!graph_settings.orientation_vertical)
			{
				/* In horizontal spectrogram mode, just scroll up by Y lines */
				mylcd_scroll_up(y);
				graph_settings.spectrogram_pos -= y;
				if(graph_settings.spectrogram_pos < 0)
					graph_settings.spectrogram_pos = 0;
			}
			else
			{
				/* In vertical spectrogram mode, erase the popup */
                mylcd_set_foreground(COLOR_DEFAULT_BG);
				mylcd_fillrect(graph_settings.spectrogram_pos + 1, 0,
                               LCD_WIDTH, y);
                mylcd_set_foreground(COLOR_DEFAULT_FG);
			}
        }
        /* else These modes clear the screen themselves */
			
        last_message = NULL;
    }

    mylcd_update();

    graph_settings.changed.clear_all = false;
}

static void draw_lines_vertical(void)
{
    static int max = 0;

#if LCD_WIDTH < ARRAYLEN_PLOT /* graph compression */
    const int offset = 0;
    const int plotwidth = LCD_WIDTH;
#else
    const int offset = (LCD_HEIGHT - ARRAYLEN_PLOT) / 2;
    const int plotwidth = ARRAYLEN_PLOT;
#endif

    int this_max;
    int i, x;

    if(graph_settings.changed.amp_scale)
        max = 0; /* reset the graph on scaling mode change */

    this_max = calc_magnitudes(graph_settings.logarithmic_amp);

    if(this_max == 0)
    {
        mylcd_hline(0, LCD_WIDTH - 1, LCD_HEIGHT - 1); /* Draw all "zero" */
        return;
    }

    if(graph_settings.logarithmic_freq)
        logarithmic_plot_translate();

    /* take the maximum of neighboring bins if we have to scale the graph
     * horizontally */
    if(LCD_WIDTH < ARRAYLEN_PLOT) /* graph compression */
    {
        int bins_acc = LCD_WIDTH / 2;
        int bins_max = 0;
        
        i = 0, x = 0;

        for(;;)
        {
            int bin = plot[i++];

            if(bin > bins_max)
                bins_max = bin;

            bins_acc += LCD_WIDTH;

            if(bins_acc >= ARRAYLEN_PLOT)
            {
                plot[x] = bins_max;
                
                if(bins_max > max)
                    max = bins_max;

                if(++x >= LCD_WIDTH)
                    break;

                bins_acc -= ARRAYLEN_PLOT;
                bins_max = 0;
            }
        }
    }
    else
    {
        if(this_max > max)
            max = this_max;
    }

    for(x = 0; x < plotwidth; ++x)
    {
        int h = LCD_HEIGHT*plot[x] / max;
        mylcd_vline(x + offset, LCD_HEIGHT - h, LCD_HEIGHT-1);
    }
}

static void draw_lines_horizontal(void)
{
    static int max = 0;

#if LCD_WIDTH < ARRAYLEN_PLOT /* graph compression */
    const int offset = 0;
    const int plotwidth = LCD_HEIGHT;
#else
    const int offset = (LCD_HEIGHT - ARRAYLEN_PLOT) / 2;
    const int plotwidth = ARRAYLEN_PLOT;
#endif

    int this_max;
    int y;

    if(graph_settings.changed.amp_scale)
        max = 0; /* reset the graph on scaling mode change */

    this_max = calc_magnitudes(graph_settings.logarithmic_amp);

    if(this_max == 0)
    {
        mylcd_vline(0, 0, LCD_HEIGHT-1); /* Draw all "zero" */
        return;
    }

    if(graph_settings.logarithmic_freq)
        logarithmic_plot_translate();

    /* take the maximum of neighboring bins if we have to scale the graph
     * horizontally */
    if(LCD_HEIGHT < ARRAYLEN_PLOT) /* graph compression */
    {
        int bins_acc = LCD_HEIGHT / 2;
        int bins_max = 0;
        int i = 0;

        y = 0;

        for(;;)
        {
            int bin = plot[i++];

            if (bin > bins_max)
                bins_max = bin;

            bins_acc += LCD_HEIGHT;

            if(bins_acc >= ARRAYLEN_PLOT)
            {
                plot[y] = bins_max;

                if(bins_max > max)
                    max = bins_max;

                if(++y >= LCD_HEIGHT)
                    break;

                bins_acc -= ARRAYLEN_PLOT;
                bins_max = 0;
            }
        }
    }
    else
    {
        if(this_max > max)
            max = this_max;
    }

    for(y = 0; y < plotwidth; ++y)
    {
        int w = LCD_WIDTH*plot[y] / max;
        mylcd_hline(0, w - 1, y + offset);
    }
}

static void draw_bars_vertical(void)
{
    static int max = 0;

#if LCD_WIDTH < LCD_HEIGHT
    const int bars = 15;
#else
    const int bars = 20;
#endif
    const int border = 2;
    const int barwidth = LCD_WIDTH / (bars + border);
    const int width = barwidth - border;
    const int offset = (LCD_WIDTH - bars*barwidth) / 2;

    if(graph_settings.changed.amp_scale)
        max = 0; /* reset the graph on scaling mode change */

    mylcd_hline(0, LCD_WIDTH-1, LCD_HEIGHT-1); /* Draw baseline */

    if(calc_magnitudes(graph_settings.logarithmic_amp) == 0)
        return; /* nothing more to draw */

    if(graph_settings.logarithmic_freq)
        logarithmic_plot_translate();

    int bins_acc = bars / 2;
    int bins_max = 0;
    int x = 0, i = 0;

    for(;;)
    {
        int bin = plot[i++];

        if(bin > bins_max)
            bins_max = bin;

        bins_acc += bars;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            plot[x] = bins_max;

            if(bins_max > max)
                max = bins_max;

            if(++x >= bars)
                break;

            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }

    for(i = 0, x = offset; i < bars; ++i, x += barwidth)
    {
        int h = LCD_HEIGHT * plot[i] / max;
        mylcd_fillrect(x, LCD_HEIGHT - h, width, h - 1);
    }
}

static void draw_bars_horizontal(void)
{
    static int max = 0;

#if LCD_WIDTH < LCD_HEIGHT
    const int bars = 20;
#else
    const int bars = 15;
#endif
    const int border = 2;
    const int barwidth = LCD_HEIGHT / (bars + border);
    const int height = barwidth - border;
    const int offset = (LCD_HEIGHT - bars*barwidth) / 2;

    if(graph_settings.changed.amp_scale)
        max = 0; /* reset the graph on scaling mode change */

    mylcd_vline(0, 0, LCD_HEIGHT-1); /* Draw baseline */

    if(calc_magnitudes(graph_settings.logarithmic_amp) == 0)
        return; /* nothing more to draw */

    if(graph_settings.logarithmic_freq)
        logarithmic_plot_translate();

    int bins_acc = bars / 2;
    int bins_max = 0;
    int y = 0, i = 0;

    for(;;)
    {
        int bin = plot[i++];

        if (bin > bins_max)
            bins_max = bin;

        bins_acc += bars;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            plot[y] = bins_max;

            if(bins_max > max)
                max = bins_max;

            if(++y >= bars)
                break;

            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }

    for(i = 0, y = offset; i < bars; ++i, y += barwidth)
    {
        int w = LCD_WIDTH * plot[i] / max;
        mylcd_fillrect(1, y, w, height);
    }
}

static void draw_spectrogram_vertical(void)
{
    const int32_t scale_factor = MIN(LCD_HEIGHT, ARRAYLEN_PLOT);

    calc_magnitudes(graph_settings.logarithmic_amp);

    if(graph_settings.logarithmic_freq)
        logarithmic_plot_translate();

    int bins_acc = scale_factor / 2;
    int bins_max = 0;
    int y = 0, i = 0;

    for(;;)
    {
        int bin = plot[i++];

        if(bin > bins_max)
            bins_max = bin;

        bins_acc += scale_factor;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            unsigned index;

            if(graph_settings.logarithmic_amp)
                index = (SHADES-1)*bins_max / QLOG_MAX;
            else
                index = (SHADES-1)*bins_max / QLIN_MAX;

            /* These happen because we exaggerate the graph a little for
             * linear mode */
            if(index >= SHADES)
                index = SHADES-1;

            mylcd_set_foreground(SPECTROGRAPH_PALETTE(index));
            mylcd_drawpixel(graph_settings.spectrogram_pos,
                            scale_factor-1 - y);

            if(++y >= scale_factor)
                break;

            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }

    if(graph_settings.spectrogram_pos < LCD_WIDTH-1)
        graph_settings.spectrogram_pos++;
    else
        mylcd_scroll_left(1);
}

static void draw_spectrogram_horizontal(void)
{
    const int32_t scale_factor = MIN(LCD_WIDTH, ARRAYLEN_PLOT);

    calc_magnitudes(graph_settings.logarithmic_amp);

    if(graph_settings.logarithmic_freq)
        logarithmic_plot_translate();

    int bins_acc = scale_factor / 2;
    int bins_max = 0;
    int x = 0, i = 0;

    for(;;)
    {
        int bin = plot[i++];

        if(bin > bins_max)
            bins_max = bin;

        bins_acc += scale_factor;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            unsigned index;

            if(graph_settings.logarithmic_amp)
                index = (SHADES-1)*bins_max / QLOG_MAX;
            else
                index = (SHADES-1)*bins_max / QLIN_MAX;

            /* These happen because we exaggerate the graph a little for
             * linear mode */
            if(index >= SHADES)
                index = SHADES-1;

            mylcd_set_foreground(SPECTROGRAPH_PALETTE(index));
            mylcd_drawpixel(x, graph_settings.spectrogram_pos);

            if(++x >= scale_factor)
                break;

            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }

    if(graph_settings.spectrogram_pos < LCD_HEIGHT-1)
        graph_settings.spectrogram_pos++;
    else
        mylcd_scroll_up(1);
}

/********************* End of plotting functions (modes) *********************/

/****************************** FFT functions ********************************/
static bool is_playing(void)
{
    return rb->mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) == CHANNEL_PLAYING;
}

/** functions use in single/multi configuration **/
static inline bool fft_init_fft_lib(void)
{
    size_t size = sizeof(fft_buffer);
    fft_state = kiss_fft_alloc(FFT_SIZE, 0, fft_buffer, &size);

    if(fft_state == NULL)
    {
        DEBUGF("needed data: %i", (int) size);
        return false;
    }

    return true;
}

static inline bool fft_get_fft(void)
{
    int count;
    const int16_t *value =
        rb->mixer_channel_get_buffer(PCM_MIXER_CHAN_PLAYBACK, &count);
    /* This block can introduce discontinuities in our data. Meaning, the
     * FFT will not be done a continuous segment of the signal. Which can
     * be bad. Or not.
     * 
     * Anyway, this is a demo, not a scientific tool. If you want accuracy,
     * do a proper spectrum analysis.*/

    /* there are cases when we don't have enough data to fill the buffer */
    if(count != ARRAYLEN_IN)
    {
        if(count < ARRAYLEN_IN)
            return false;

        count = ARRAYLEN_IN;  /* too much - limit */
    }

    int fft_idx = 0; /* offset in 'input' */

    do
    {
        kiss_fft_scalar left = *value++;
        kiss_fft_scalar right = *value++;
        input[fft_idx].r = (left + right) >> 1; /* to mono */
    } while (fft_idx++, --count > 0);

    apply_window_func(graph_settings.window_func);

    rb->yield();

    kiss_fft(fft_state, input, output[output_tail]);

    rb->yield();

    return true;
}

#if NUM_CORES > 1
/* use a worker thread if there is another processor core */
static volatile bool fft_thread_run SHAREDDATA_ATTR = false;
static unsigned long fft_thread;

static long fft_thread_stack[CACHEALIGN_UP(DEFAULT_STACK_SIZE*4/sizeof(long))]
                             CACHEALIGN_AT_LEAST_ATTR(4);

static void fft_thread_entry(void)
{
    if (!fft_init_fft_lib())
    {
        output_tail = -1; /* tell that we bailed */
        fft_thread_run = true;
        return;
    }

    fft_thread_run = true;

    while(fft_thread_run)
	{
        if (!is_playing())
        {
            rb->sleep(HZ/5);
            continue;
        }

        if (!fft_get_fft())
        {
            rb->sleep(0);    /* not enough - ease up */
            continue;
        }

        /* write back output for other processor and invalidate for next frame read */
        rb->commit_discard_dcache();

        int new_tail = output_tail ^ 1;

        /* if full, block waiting until reader has freed a slot */
        while(fft_thread_run)
        {
            if(new_tail != output_head)
            {
                output_tail = new_tail;
                break;
            }

            rb->sleep(0);
        }
	}
}

static bool fft_have_fft(void)
{
    return output_head != output_tail;
}

/* Call only after fft_have_fft() has returned true */
static inline void fft_free_fft_output(void)
{
    output_head ^= 1; /* finished with this */
}

static bool fft_init_fft(void)
{
    /* create worker thread - on the COP for dual-core targets */
    fft_thread = rb->create_thread(fft_thread_entry,
            fft_thread_stack, sizeof(fft_thread_stack), 0, "fft output thread"
            IF_PRIO(, PRIORITY_USER_INTERFACE+1) IF_COP(, COP));

    if(fft_thread == 0)
    {
        rb->splash(HZ, "FFT thread failed create");
        return false;
    }

    /* wait for it to indicate 'ready' */
    while(fft_thread_run == false)
        rb->sleep(0);

    if(output_tail == -1)
    {
        /* FFT thread bailed-out like The Fed */
        rb->thread_wait(fft_thread);
        rb->splash(HZ, "FFT thread failed to init");
        return false;
    }

    return true;
}

static void fft_close_fft(void)
{
    /* Handle our FFT thread. */
    fft_thread_run = false;
    rb->thread_wait(fft_thread);
    rb->commit_discard_dcache();
}
#else /* NUM_CORES == 1 */
/* everything serialize on single-core and FFT gets to use IRAM main stack if
 * target uses IRAM */
static bool fft_have_fft(void)
{
    return is_playing() && fft_get_fft();
}

static inline void fft_free_fft_output(void)
{
    /* nothing to do */
}

static bool fft_init_fft(void)
{
    return fft_init_fft_lib();
}

static inline void fft_close_fft(void)
{
    /* nothing to do */
}
#endif /* NUM_CORES */
/*************************** End of FFT functions ****************************/

enum plugin_status plugin_start(const void* parameter)
{
    /* Defaults */
    bool run = true;
    bool showing_warning = false;

    if (!fft_init_fft())
        return PLUGIN_ERROR;

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
        fft_close_fft();
        return PLUGIN_ERROR;
    }
    grey_show(true); 
#endif

    logarithmic_plot_init();

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    mylcd_clear_display();
    mylcd_update();
#endif
    backlight_ignore_timeout();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    while (run)
    {
        /* Unless otherwise specified, HZ/50 is around the window length
         * and quite fast. We want to be done with drawing by this time. */
        long next_frame_tick = *rb->current_tick + HZ/50;
        int button;

        while (!fft_have_fft())
		{
            int timeout;

            if(!is_playing())
            {
                showing_warning = true;
                mylcd_clear_display();
                draw_message_string("No audio playing", false);
                mylcd_update();
                timeout = HZ/5;
            }
            else
            {
                if(showing_warning)
                {
                    showing_warning = false;
                    mylcd_clear_display();
                    mylcd_update();
                }

                timeout = HZ/100; /* 'till end of curent tick, don't use 100% CPU */
            }

            /* Make sure the FFT has produced something before doing anything
             * but watching for buttons. Music might not be playing or things
             * just aren't going well for picking up buffers so keys are
             * scanned to avoid lockup.  */
             button = rb->button_get_w_tmo(timeout);
             if (button != BUTTON_NONE)
                 goto read_button;
		}

	    draw(NULL);

        fft_free_fft_output(); /* COP only */

        long tick = *rb->current_tick;
        if(TIME_BEFORE(tick, next_frame_tick))
        {
            tick = next_frame_tick - tick;
        }
        else
        {
            rb->yield(); /* tmo = 0 won't yield */
            tick = 0;
        }

		button = rb->button_get_w_tmo(tick);
    read_button:
        switch (button)
        {
            case FFT_QUIT:
                run = false;
                break;
            case FFT_PREV_GRAPH: {
                if (graph_settings.mode-- <= FFT_DM_FIRST)
                    graph_settings.mode = FFT_DM_COUNT-1;
                graph_settings.changed.mode = true;
                draw(modes_text[graph_settings.mode]);
                break;
            }
            case FFT_NEXT_GRAPH: {
                if (++graph_settings.mode >= FFT_DM_COUNT)
                    graph_settings.mode = FFT_DM_FIRST;
                graph_settings.changed.mode = true;
                draw(modes_text[graph_settings.mode]);
                break;
            }
            case FFT_WINDOW: {
                if(++graph_settings.window_func >= FFT_WF_COUNT)
                    graph_settings.window_func = FFT_WF_FIRST;
                graph_settings.changed.window_func = true;
                draw(window_text[graph_settings.window_func]);
                break;
            }
            case FFT_AMP_SCALE: {
                graph_settings.logarithmic_amp = !graph_settings.logarithmic_amp;
                graph_settings.changed.amp_scale = true;
                draw(amp_scales_text[graph_settings.logarithmic_amp ? 1 : 0]);
                break;
            }
#ifdef FFT_FREQ_SCALE /* 'Till all keymaps are defined */
            case FFT_FREQ_SCALE: {
                graph_settings.logarithmic_freq = !graph_settings.logarithmic_freq;
                graph_settings.changed.freq_scale = true;
                draw(freq_scales_text[graph_settings.logarithmic_freq ? 1 : 0]);
                break;
            }
#endif
            case FFT_ORIENTATION: {
                graph_settings.orientation_vertical =
                    !graph_settings.orientation_vertical;
                graph_settings.changed.orientation = true;
                draw(NULL);
                break;
            }
            default: {
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
            }

        }
    }

    fft_close_fft();	

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
#ifndef HAVE_LCD_COLOR
    grey_release();
#endif
    backlight_use_settings();
    return PLUGIN_OK;
    (void)parameter;
}
