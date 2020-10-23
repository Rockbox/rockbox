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
#include "lib/pluginlib_exit.h"
#include "lib/configfile.h"
#include "lib/xlcd.h"
#include "math.h"
#include "fracmul.h"
#ifndef HAVE_LCD_COLOR
#include "lib/grey.h"
#endif
#include "lib/mylcd.h"
#include "lib/osd.h"

#ifndef HAVE_LCD_COLOR
GREY_INFO_STRUCT
#endif

#include "lib/pluginlib_actions.h"

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
#define FFT_PREV_GRAPH   PLA_LEFT
#define FFT_NEXT_GRAPH   PLA_RIGHT
#define FFT_ORIENTATION  PLA_CANCEL
#define FFT_WINDOW       PLA_SELECT
#define FFT_AMP_SCALE    PLA_UP
#define FFT_FREQ_SCALE   PLA_DOWN
#define FFT_QUIT         PLA_EXIT

#ifdef HAVE_LCD_COLOR
#include "pluginbitmaps/fft_colors.h"
#endif

#include "kiss_fftr.h"
#include "_kiss_fft_guts.h" /* sizeof(struct kiss_fft_state) */
#include "const.h"


/******************************* FFT globals *******************************/

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
#define BUFSIZE_FFT (sizeof(struct kiss_fft_state)+\
                     sizeof(kiss_fft_cpx)*(FFT_SIZE-1))

#define __COEFF(type,size) type##_##size
#define _COEFF(x, y) __COEFF(x,y) /* force CPP evaluation of FFT_SIZE */
#define HANN_COEFF _COEFF(hann, FFT_SIZE)
#define HAMMING_COEFF _COEFF(hamming, FFT_SIZE)

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
static uint32_t linf_magnitudes[ARRAYLEN_PLOT]; /* ling freq bin plot */
static uint32_t logf_magnitudes[ARRAYLEN_PLOT]; /* log freq plot output */
static uint32_t *plot;                          /* use this to plot */
static struct
{
    int16_t bin;   /* integer bin number */
    uint16_t frac; /* interpolation fraction */
} binlog[ARRAYLEN_PLOT] __attribute__((aligned(4)));

/**************************** End of FFT globals ***************************/


/********************************* Settings ********************************/

enum fft_orientation
{
    FFT_MIN_OR = 0,
    FFT_OR_VERT = 0, /* Amplitude vertical, frequency horizontal * */
    FFT_OR_HORZ,     /* Amplitude horizontal, frequency vertical */
    FFT_MAX_OR,
};

enum fft_display_mode
{
    FFT_MIN_DM = 0,
    FFT_DM_LINES = 0,   /* Bands are displayed as single-pixel lines * */
    FFT_DM_BARS,        /* Bands are combined into wide bars */
    FFT_DM_SPECTROGRAM, /* Band amplitudes are denoted by color */
    FFT_MAX_DM,
};

enum fft_amp_scale
{
    FFT_MIN_AS = 0,
    FFT_AS_LOG = 0, /* Amplitude is plotted on log scale * */
    FFT_AS_LIN,     /* Amplitude is plotted on linear scale */
    FFT_MAX_AS,
};

enum fft_freq_scale
{
    FFT_MIN_FS = 0,
    FFT_FS_LOG = 0, /* Frequency is plotted on log scale * */
    FFT_FS_LIN,     /* Frequency is plotted on linear scale */
    FFT_MAX_FS
};

enum fft_window_func
{
    FFT_MIN_WF = 0,
    FFT_WF_HAMMING = 0, /* Hamming window applied to each input frame * */
    FFT_WF_HANN,        /* Hann window applied to each input frame */
    FFT_MAX_WF,
};

static struct fft_config
{
    int orientation;
    int drawmode;
    int amp_scale;
    int freq_scale;
    int window_func;
} fft_disk =
{
     /* Defaults */
    .orientation = FFT_OR_VERT,
    .drawmode    = FFT_DM_LINES,
    .amp_scale   = FFT_AS_LOG,
    .freq_scale  = FFT_FS_LOG,
    .window_func = FFT_WF_HAMMING,
};

#define CFGFILE_VERSION    0
#define CFGFILE_MINVERSION 0

static const char cfg_filename[] =  "fft.cfg";
static struct configdata disk_config[] =
{
   { TYPE_ENUM, FFT_MIN_OR, FFT_MAX_OR,
     { .int_p = &fft_disk.orientation }, "orientation",
        (char * []){ [FFT_OR_VERT] = "vertical",
                     [FFT_OR_HORZ] = "horizontal" } },
   { TYPE_ENUM, FFT_MIN_DM, FFT_MAX_DM,
     { .int_p = &fft_disk.drawmode }, "drawmode",
        (char * []){ [FFT_DM_LINES]       = "lines",
                     [FFT_DM_BARS]        = "bars",
                     [FFT_DM_SPECTROGRAM] = "spectrogram" } },
   { TYPE_ENUM, FFT_MIN_AS, FFT_MAX_AS,
     { .int_p = &fft_disk.amp_scale }, "amp scale",
        (char * []){ [FFT_AS_LOG] = "logarithmic",
                     [FFT_AS_LIN] = "linear" } },
   { TYPE_ENUM, FFT_MIN_FS, FFT_MAX_FS,
     { .int_p = &fft_disk.freq_scale }, "freq scale",
        (char * []){ [FFT_FS_LOG] = "logarithmic",
                     [FFT_FS_LIN] = "linear" } },
   { TYPE_ENUM, FFT_MIN_WF, FFT_MAX_WF,
     { .int_p = &fft_disk.window_func }, "window function",
        (char * []){ [FFT_WF_HAMMING] = "hamming",
                     [FFT_WF_HANN]    = "hann" } },
};

/* Hint flags for setting changes */
enum fft_setting_flags
{
    FFT_SETF_OR = 1 << 0,
    FFT_SETF_DM = 1 << 1,
    FFT_SETF_AS = 1 << 2,
    FFT_SETF_FS = 1 << 3,
    FFT_SETF_WF = 1 << 4,
    FFT_SETF_ALL = 0x1f
};

/***************************** End of settings *****************************/


/**************************** Operational data *****************************/

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

#define FFT_OSD_MARGIN_SIZE 1

#define FFT_PERIOD  (HZ/50) /* How fast to try to go */

/* Based on feeding-in a 0db sinewave at FS/4 */
#define QLOG_MAX 0x0009154B
/* Fudge it a little or it's not very visbile */
#define QLIN_MAX (0x00002266 >> 1)

static struct fft_config fft;
typedef void (* fft_drawfn_t)(unsigned, unsigned);
static fft_drawfn_t fft_drawfn = NULL; /* plotting function */
static int fft_spectrogram_pos = -1; /* row or column - only used by one at a time */
static uint32_t fft_graph_scale = 0; /* max level over time, for scaling display */
static int fft_message_id = -1; /* current message id displayed */
static char fft_osd_message[32]; /* current message string displayed */
static long fft_next_frame_tick = 0; /* next tick to attempt drawing */

#ifdef HAVE_LCD_COLOR
#define SHADES BMPWIDTH_fft_colors
#define SPECTROGRAPH_PALETTE(index) (fft_colors[index])
#else
#define SHADES 256
#define SPECTROGRAPH_PALETTE(index) (255 - (index))
#endif

/************************* End of operational data *************************/


/***************************** Math functions ******************************/

/* Apply window function to input */
static void apply_window_func(enum fft_window_func mode)
{
    static const int16_t * const coefs[] =
    {
        [FFT_WF_HAMMING] = HAMMING_COEFF,
        [FFT_WF_HANN]    = HANN_COEFF,
    };

    const int16_t * const c = coefs[mode];

    for(int i = 0; i < ARRAYLEN_IN; ++i)
        input[i].r = (input[i].r * c[i] + 16384) >> 15;
}

/* Calculates the magnitudes from complex numbers and returns the maximum */
static unsigned calc_magnitudes(enum fft_amp_scale scale)
{
    /* A major assumption made when calculating the Q*MAX constants
     * is that the maximum magnitude is 29 bits long. */
    unsigned this_max = 0;
    kiss_fft_cpx *this_output = output[output_head] + 1; /* skip DC */

    /* Calculate the magnitude, discarding the phase. */
    for(int i = 0; i < ARRAYLEN_PLOT; ++i)
    {
        int32_t re = this_output[i].r;
        int32_t im = this_output[i].i;

        uint32_t d = re*re + im*im;

        if(d > 0)
        {
            if(d > 0x7FFFFFFF) /* clip */
            {
                d = 0x7FFFFFFF; /* if our assumptions are correct,
                                   this should never happen. It's just
                                   a safeguard. */
            }

            if(scale == FFT_AS_LOG)
            {
                if(d < 0x8000) /* be more precise */
                {
                    /* ln(x ^ .5) = .5*ln(x) */
                    d = fp16_log(d << 16) >> 1;
                }
                else
                {
                    d = fp_sqrt(d, 0); /* linear scaling, nothing
                                          bad should happen */
                    d = fp16_log(d << 16); /* the log function
                                              expects s15.16 values */
                }
            }
            else
            {
                d = fp_sqrt(d, 0); /* linear scaling, nothing
                                      bad should happen */
            }
        }

        /* Length 2 moving average - last transform and this one */
        linf_magnitudes[i] = (linf_magnitudes[i] + d) >> 1;

        if(d > this_max)
            this_max = d;
    }

    return this_max;
}

/* Move plot bins into a logarithmic scale by sliding them towards the
 * Nyquist bin according to the translation in the binlog array. */
static void log_plot_translate(void)
{
    for(int i = ARRAYLEN_PLOT-1; i > 0; --i)
    {
        int s = binlog[i].bin;
        int e = binlog[i-1].bin;
        unsigned frac = binlog[i].frac;

        int bin = linf_magnitudes[s];

        if(frac)
        {
            /* slope < 1, Interpolate stretched bins (linear for now) */
            int diff = linf_magnitudes[s+1] - bin;

            do
            {
                logf_magnitudes[i] = bin + FRACMUL(frac << 15, diff);
                frac = binlog[--i].frac;
            }
            while(frac);
        }
        else
        {
            /* slope > 1, Find peak of two or more bins */
            while(--s > e)
            {
                int val = linf_magnitudes[s];

                if (val > bin)
                    bin = val;
            }
        }

        logf_magnitudes[i] = bin;
    }
}

/* Calculates the translation for logarithmic plot bins */
static void logarithmic_plot_init(void)
{
    /*
     * log: y = round(n * ln(x) / ln(n))
     * anti: y = round(exp(x * ln(n) / n))
     */
    int j = fp16_log((ARRAYLEN_PLOT - 1) << 16);
    for(int i = 0; i < ARRAYLEN_PLOT; ++i)
    {
        binlog[i].bin = (fp16_exp(i * j / (ARRAYLEN_PLOT - 1)) + 32768) >> 16;
    }

    /* setup fractions for interpolation of stretched bins */
    for(int i = 0; i < ARRAYLEN_PLOT-1; i = j)
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

/************************** End of math functions **************************/


/*********************** Plotting functions (modes) ************************/

static void draw_lines_vertical(unsigned this_max, unsigned graph_max)
{
#if LCD_WIDTH < ARRAYLEN_PLOT /* graph compression */
    const int offset = 0;
    const int plotwidth = LCD_WIDTH;
#else
    const int offset = (LCD_HEIGHT - ARRAYLEN_PLOT) / 2;
    const int plotwidth = ARRAYLEN_PLOT;
#endif

    mylcd_clear_display();

    if(this_max == 0)
    {
        mylcd_hline(0, LCD_WIDTH - 1, LCD_HEIGHT - 1); /* Draw all "zero" */
        return;
    }

    /* take the maximum of neighboring bins if we have to scale down the
     * graph horizontally */
    if(LCD_WIDTH < ARRAYLEN_PLOT) /* graph compression */
    {
        int bins_acc = LCD_WIDTH / 2;
        unsigned bins_max = 0;

        for(int i = 0, x = 0; i < ARRAYLEN_PLOT; ++i)
        {
            unsigned bin = plot[i];

            if(bin > bins_max)
                bins_max = bin;

            bins_acc += LCD_WIDTH;

            if(bins_acc >= ARRAYLEN_PLOT)
            {
                int h = LCD_HEIGHT*bins_max / graph_max;
                mylcd_vline(x, LCD_HEIGHT - h, LCD_HEIGHT-1);

                x++;
                bins_acc -= ARRAYLEN_PLOT;
                bins_max = 0;
            }
        }
    }
    else
    {
        for(int i = 0; i < plotwidth; ++i)
        {
            int h = LCD_HEIGHT*plot[i] / graph_max;
            mylcd_vline(i + offset, LCD_HEIGHT - h, LCD_HEIGHT-1);
        }
    }
}

static void draw_lines_horizontal(unsigned this_max, unsigned graph_max)
{
#if LCD_WIDTH < ARRAYLEN_PLOT /* graph compression */
    const int offset = 0;
    const int plotwidth = LCD_HEIGHT;
#else
    const int offset = (LCD_HEIGHT - ARRAYLEN_PLOT) / 2;
    const int plotwidth = ARRAYLEN_PLOT;
#endif

    mylcd_clear_display();

    if(this_max == 0)
    {
        mylcd_vline(0, 0, LCD_HEIGHT-1); /* Draw all "zero" */
        return;
    }

    /* take the maximum of neighboring bins if we have to scale the graph
     * horizontally */
    if(LCD_HEIGHT < ARRAYLEN_PLOT) /* graph compression */
    {
        int bins_acc = LCD_HEIGHT / 2;
        unsigned bins_max = 0;

        for(int i = 0, y = 0; i < ARRAYLEN_PLOT; ++i)
        {
            unsigned bin = plot[i];

            if(bin > bins_max)
                bins_max = bin;

            bins_acc += LCD_HEIGHT;

            if(bins_acc >= ARRAYLEN_PLOT)
            {
                int w = LCD_WIDTH*bins_max / graph_max;
                mylcd_hline(0, w - 1, y);

                y++;
                bins_acc -= ARRAYLEN_PLOT;
                bins_max = 0;
            }
        }
    }
    else
    {
        for(int i = 0; i < plotwidth; ++i)
        {
            int w = LCD_WIDTH*plot[i] / graph_max;
            mylcd_hline(0, w - 1, i + offset);
        }
    }
}

static void draw_bars_vertical(unsigned this_max, unsigned graph_max)
{
#if LCD_WIDTH < LCD_HEIGHT
    const int bars = 15;
#else
    const int bars = 20;
#endif
    const int border = 2;
    const int barwidth = LCD_WIDTH / (bars + border);
    const int width = barwidth - border;
    const int offset = (LCD_WIDTH - bars*barwidth + border) / 2;

    mylcd_clear_display();
    mylcd_hline(0, LCD_WIDTH-1, LCD_HEIGHT-1); /* Draw baseline */

    if(this_max == 0)
        return; /* nothing more to draw */

    int bins_acc = bars / 2;
    unsigned bins_max = 0;

    for(int i = 0, x = offset;; ++i)
    {
        unsigned bin = plot[i];

        if(bin > bins_max)
            bins_max = bin;

        bins_acc += bars;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            int h = LCD_HEIGHT*bins_max / graph_max;
            mylcd_fillrect(x, LCD_HEIGHT - h, width, h - 1);

            if(i >= ARRAYLEN_PLOT-1)
                break;

            x += barwidth;
            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }
}

static void draw_bars_horizontal(unsigned this_max, unsigned graph_max)
{
#if LCD_WIDTH < LCD_HEIGHT
    const int bars = 20;
#else
    const int bars = 15;
#endif
    const int border = 2;
    const int barwidth = LCD_HEIGHT / (bars + border);
    const int height = barwidth - border;
    const int offset = (LCD_HEIGHT - bars*barwidth + border) / 2;

    mylcd_clear_display();
    mylcd_vline(0, 0, LCD_HEIGHT-1); /* Draw baseline */

    if(this_max == 0)
        return; /* nothing more to draw */

    int bins_acc = bars / 2;
    unsigned bins_max = 0;

    for(int i = 0, y = offset;; ++i)
    {
        unsigned bin = plot[i];

        if(bin > bins_max)
            bins_max = bin;

        bins_acc += bars;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            int w = LCD_WIDTH*bins_max / graph_max;
            mylcd_fillrect(1, y, w, height);

            if(i >= ARRAYLEN_PLOT-1)
                break;

            y += barwidth;
            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }
}

static void draw_spectrogram_vertical(unsigned this_max, unsigned graph_max)
{
    const int scale_factor = MIN(LCD_HEIGHT, ARRAYLEN_PLOT);

    if(fft_spectrogram_pos < LCD_WIDTH-1)
        fft_spectrogram_pos++;
    else
        mylcd_scroll_left(1);

    int bins_acc = scale_factor / 2;
    unsigned bins_max = 0;

    for(int i = 0, y = LCD_HEIGHT-1;; ++i)
    {
        unsigned bin = plot[i];

        if(bin > bins_max)
            bins_max = bin;

        bins_acc += scale_factor;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            unsigned index = (SHADES-1)*bins_max / graph_max;
            unsigned color;

            /* These happen because we exaggerate the graph a little for
             * linear mode */
            if(index >= SHADES)
                index = SHADES-1;

            color = FB_UNPACK_SCALAR_LCD(SPECTROGRAPH_PALETTE(index));
            mylcd_set_foreground(color);
            mylcd_drawpixel(fft_spectrogram_pos, y);

            if(--y < 0)
                break;

            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }

    (void)this_max;
}

static void draw_spectrogram_horizontal(unsigned this_max, unsigned graph_max)
{
    const int scale_factor = MIN(LCD_WIDTH, ARRAYLEN_PLOT);

    if(fft_spectrogram_pos < LCD_HEIGHT-1)
        fft_spectrogram_pos++;
    else
        mylcd_scroll_up(1);

    int bins_acc = scale_factor / 2;
    unsigned bins_max = 0;

    for(int i = 0, x = 0;; ++i)
    {
        unsigned bin = plot[i];

        if(bin > bins_max)
            bins_max = bin;

        bins_acc += scale_factor;

        if(bins_acc >= ARRAYLEN_PLOT)
        {
            unsigned index = (SHADES-1)*bins_max / graph_max;
            unsigned color;

            /* These happen because we exaggerate the graph a little for
             * linear mode */
            if(index >= SHADES)
                index = SHADES-1;

            color = FB_UNPACK_SCALAR_LCD(SPECTROGRAPH_PALETTE(index));
            mylcd_set_foreground(color);
            mylcd_drawpixel(x, fft_spectrogram_pos);

            if(++x >= LCD_WIDTH)
                break;

            bins_acc -= ARRAYLEN_PLOT;
            bins_max = 0;
        }
    }

    (void)this_max;
}

/******************** End of plotting functions (modes) ********************/


/***************************** FFT functions *******************************/

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

    apply_window_func(fft.window_func);

    rb->yield();

    kiss_fft(fft_state, input, output[output_tail]);

    rb->yield();

    return true;
}

#if NUM_CORES > 1
/* use a worker thread if there is another processor core */
static volatile bool fft_thread_run SHAREDDATA_ATTR = false;
static unsigned long fft_thread = 0;

static long fft_thread_stack[CACHEALIGN_UP(DEFAULT_STACK_SIZE*4/sizeof(long))]
                             CACHEALIGN_AT_LEAST_ATTR(4);

static void fft_thread_entry(void)
{
    if(!fft_init_fft_lib())
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

        /* write back output for other processor and invalidate for next
           frame read */
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

/************************** End of FFT functions ***************************/


/****************************** OSD functions ******************************/

/* Format a message to display */
static void fft_osd_format_message(enum fft_setting_flags id)
{
    const char *msg = "";

    switch (id)
    {
    case FFT_SETF_DM:
        msg = (const char * [FFT_MAX_DM]) {
                [FFT_DM_LINES]       = "Lines",
                [FFT_DM_BARS]        = "Bars",
                [FFT_DM_SPECTROGRAM] = "Spectrogram",
            }[fft.drawmode];
        break;

    case FFT_SETF_WF:
        msg = (const char * [FFT_MAX_WF]) {
                [FFT_WF_HAMMING] = "Hamming window",
                [FFT_WF_HANN]    = "Hann window",
            }[fft.window_func];
        break;

    case FFT_SETF_AS:
        msg = (const char * [FFT_MAX_AS]) {
                [FFT_AS_LOG] = "Logarithmic amplitude",
                [FFT_AS_LIN] = "Linear amplitude"
            }[fft.amp_scale];
        break;

    case FFT_SETF_FS:
        msg = (const char * [FFT_MAX_FS]) {
                [FFT_FS_LOG] = "Logarithmic frequency",
                [FFT_FS_LIN] = "Linear frequency",
            }[fft.freq_scale];
        break;

    case FFT_SETF_OR:
        rb->snprintf(fft_osd_message, sizeof (fft_osd_message),
                     (const char * [FFT_MAX_OR]) {
                        [FFT_OR_VERT] = "Vertical %s",
                        [FFT_OR_HORZ] = "Horizontal %s",
                     }[fft.orientation],
                     (const char * [FFT_MAX_DM]) {
                        [FFT_DM_LINES ... FFT_DM_BARS] = "amplitude",
                        [FFT_DM_SPECTROGRAM] = "frequency"
                     }[fft.drawmode]);
        return;

#if 0
    /* Pertentially */
    case FFT_SETF_VOLUME:
        rb->snprintf(fft_osd_message, sizeof (fft_osd_message),
                     "Volume: %d%s",
                     rb->sound_val2phys(SOUND_VOLUME, global_settings.volume),
                     rb->sound_unit(SOUND_VOLUME));
        return;
#endif

    default:
        break;
    }

    /* Default action: copy string */
    rb->strlcpy(fft_osd_message, msg, sizeof (fft_osd_message));
}

static void fft_osd_draw_cb(int x, int y, int width, int height)
{
#if LCD_DEPTH > 1
    mylcd_set_foreground(COLOR_MESSAGE_FG);
    mylcd_set_background(COLOR_MESSAGE_BG);
#endif
#if FFT_OSD_MARGIN_SIZE != 0
    mylcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    mylcd_fillrect(1, 1, width - 2, height - 2);
    mylcd_set_drawmode(DRMODE_SOLID);
#endif
    mylcd_putsxy(1+FFT_OSD_MARGIN_SIZE, 1+FFT_OSD_MARGIN_SIZE,
                 fft_osd_message);
#if LCD_DEPTH > 1
    mylcd_set_foreground(COLOR_MESSAGE_FRAME);
#endif

    mylcd_drawrect(0, 0, width, height);

    (void)x; (void)y;
}

static void fft_osd_show_message(enum fft_setting_flags id)
{
    fft_osd_format_message(id);

    if(!myosd_enabled())
        return;

    int width, height;
    int maxwidth, maxheight;

    mylcd_set_viewport(myosd_get_viewport());
    myosd_get_max_dims(&maxwidth, &maxheight);
    mylcd_setfont(FONT_UI);
    mylcd_getstringsize(fft_osd_message, &width, &height);
    mylcd_set_viewport(NULL);

    width += 2 + 2*FFT_OSD_MARGIN_SIZE;
    if(width > maxwidth)
        width = maxwidth;

    height += 2 + 2*FFT_OSD_MARGIN_SIZE;
    if(height > maxheight)
        height = maxheight;

    bool drawn = myosd_update_pos((LCD_WIDTH - width) / 2,
                                  (LCD_HEIGHT - height) / 2,
                                  width, height);

    myosd_show(OSD_SHOW | (drawn ? 0 : OSD_UPDATENOW));
}

static void fft_popupmsg(enum fft_setting_flags id)
{
    fft_message_id = id;
}

/************************** End of OSD functions ***************************/


static void fft_setting_update(unsigned which)
{
    static fft_drawfn_t fft_drawfns[FFT_MAX_DM][FFT_MAX_OR] =
    {
        [FFT_DM_LINES] =
        {
            [FFT_OR_HORZ] = draw_lines_horizontal,
            [FFT_OR_VERT] = draw_lines_vertical,
        },
        [FFT_DM_BARS] =
        {
            [FFT_OR_HORZ] = draw_bars_horizontal,
            [FFT_OR_VERT] = draw_bars_vertical,
        },
        [FFT_DM_SPECTROGRAM] =
        {
            [FFT_OR_HORZ] = draw_spectrogram_horizontal,
            [FFT_OR_VERT] = draw_spectrogram_vertical,
        },
    };

    if(which & (FFT_SETF_DM | FFT_SETF_OR))
    {
        fft_drawfn = fft_drawfns[fft.drawmode]
                                [fft.orientation];

        if(fft.drawmode == FFT_DM_SPECTROGRAM)
        {
            fft_spectrogram_pos = -1;
            myosd_lcd_update_prepare();
            mylcd_clear_display();
            myosd_lcd_update();
        }
    }

    if(which & (FFT_SETF_DM | FFT_SETF_AS))
    {
        if(fft.drawmode == FFT_DM_SPECTROGRAM)
        {
            fft_graph_scale = fft.amp_scale == FFT_AS_LIN ?
                QLIN_MAX : QLOG_MAX;
        }
        else
        {
            fft_graph_scale = 0;
        }
    }

    if(which & FFT_SETF_FS)
    {
        plot = fft.freq_scale == FFT_FS_LIN ?
                linf_magnitudes : logf_magnitudes;
    }

    if(which & FFT_SETF_AS)
    {
        memset(linf_magnitudes, 0, sizeof (linf_magnitudes));
        memset(logf_magnitudes, 0, sizeof (logf_magnitudes));
    }
}

static long fft_draw(void)
{
    long tick = *rb->current_tick;

    if(fft_message_id != -1)
    {
        /* Show a new message */
        fft_osd_show_message((enum fft_setting_flags)fft_message_id);
        fft_message_id = -1;
    }
    else
    {
        /* Monitor OSD timeout */
        myosd_monitor_timeout();
    }

    if(TIME_BEFORE(tick, fft_next_frame_tick))
        return fft_next_frame_tick - tick; /* Too early */

    unsigned this_max;

    if(!fft_have_fft())
    {
        if(is_playing())
            return HZ/100;

        /* All magnitudes == 0 thus this_max == 0 */
        for(int i = 0; i < ARRAYLEN_PLOT; i++)
            linf_magnitudes[i] >>= 1; /* decay */

        this_max = 0;
    }
    else
    {
        this_max = calc_magnitudes(fft.amp_scale);

        fft_free_fft_output(); /* COP only */

        if(fft.drawmode != FFT_DM_SPECTROGRAM &&
           this_max > fft_graph_scale)
        {
            fft_graph_scale = this_max;
        }
    }

    if (fft.freq_scale == FFT_FS_LOG)
        log_plot_translate();

    myosd_lcd_update_prepare();

    mylcd_set_foreground(COLOR_DEFAULT_FG);
    mylcd_set_background(COLOR_DEFAULT_BG);

    fft_drawfn(this_max, fft_graph_scale);

    myosd_lcd_update();

    fft_next_frame_tick = tick + FFT_PERIOD;
    return fft_next_frame_tick - *rb->current_tick;
}

static void fft_osd_init(void *buf, size_t bufsize)
{
    int width, height;
    mylcd_setfont(FONT_UI);
    mylcd_getstringsize("M", NULL, &height);
    width = LCD_WIDTH;
    height += 2 + 2*FFT_OSD_MARGIN_SIZE;
    myosd_init(OSD_INIT_MAJOR_HEIGHT | OSD_INIT_MINOR_MAX, buf, bufsize,
               fft_osd_draw_cb, &width, &height, NULL);
    myosd_set_timeout(HZ);
}

static void fft_cleanup(void)
{
    myosd_destroy();

    fft_close_fft();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cancel_cpu_boost();
#endif
#ifndef HAVE_LCD_COLOR
    grey_release();
#endif
#ifdef HAVE_BACKLIGHT
    backlight_use_settings();
#endif

    /* save settings if changed */
    if (rb->memcmp(&fft, &fft_disk, sizeof(fft)))
    {
        fft_disk = fft;
        configfile_save(cfg_filename, disk_config, ARRAYLEN(disk_config),
                        CFGFILE_VERSION);
    }
}

static bool fft_setup(void)
{
    atexit(fft_cleanup);

    configfile_load(cfg_filename, disk_config, ARRAYLEN(disk_config),
                    CFGFILE_MINVERSION);
    fft = fft_disk; /* copy to running config */

    if(!fft_init_fft())
        return false;

    /* get the remainder of the plugin buffer for OSD and perhaps
       greylib */
    size_t bufsize = 0;
    unsigned char *buf = rb->plugin_get_buffer(&bufsize);

#ifndef HAVE_LCD_COLOR
    /* initialize the greyscale buffer.*/
    long grey_size;
    if(!grey_init(buf, bufsize, GREY_ON_COP | GREY_BUFFERED,
                  LCD_WIDTH, LCD_HEIGHT, &grey_size))
    {
        rb->splash(HZ, "Couldn't init greyscale display");
        return false;
    }

    grey_show(true);

    buf += grey_size;
    bufsize -= grey_size;
#endif /* !HAVE_LCD_COLOR */

    fft_osd_init(buf, bufsize);

#if LCD_DEPTH > 1
    myosd_lcd_update_prepare();
    rb->lcd_set_backdrop(NULL);
    mylcd_clear_display();
    myosd_lcd_update();
#endif
#ifdef HAVE_BACKLIGHT
    backlight_ignore_timeout();
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->trigger_cpu_boost();
#endif

    logarithmic_plot_init();
    fft_setting_update(FFT_SETF_ALL);
    fft_next_frame_tick = *rb->current_tick;

    return true;
}

enum plugin_status plugin_start(const void* parameter)
{
    bool run = true;

    if(!fft_setup())
        return PLUGIN_ERROR;

    while(run)
    {
        long delay = fft_draw();

        if(delay <= 0)
        {
            delay = 0;
            rb->yield(); /* tmo = 0 won't yield */
        }

        int button = pluginlib_getaction(TIMEOUT_NOBLOCK, plugin_contexts, ARRAYLEN(plugin_contexts));

        switch (button)
        {
            case FFT_QUIT:
                run = false;
                break;

            case FFT_ORIENTATION:
                if (++fft.orientation >= FFT_MAX_OR)
                    fft.orientation = FFT_MIN_OR;

                fft_setting_update(FFT_SETF_OR);
                fft_popupmsg(FFT_SETF_OR);
                break;

            case FFT_PREV_GRAPH:
                if (fft.drawmode-- <= FFT_MIN_DM)
                    fft.drawmode = FFT_MAX_DM-1;

                fft_setting_update(FFT_SETF_DM);
                fft_popupmsg(FFT_SETF_DM);
                break;

            case FFT_NEXT_GRAPH:
                if (++fft.drawmode >= FFT_MAX_DM)
                    fft.drawmode = FFT_MIN_DM;

                fft_setting_update(FFT_SETF_DM);
                fft_popupmsg(FFT_SETF_DM);
                break;

            case FFT_AMP_SCALE:
                if (++fft.amp_scale >= FFT_MAX_AS)
                    fft.amp_scale = FFT_MIN_AS;

                fft_setting_update(FFT_SETF_AS);
                fft_popupmsg(FFT_SETF_AS);
                break;

#ifdef FFT_FREQ_SCALE /* 'Till all keymaps are defined */
            case FFT_FREQ_SCALE:
                if (++fft.freq_scale >= FFT_MAX_FS)
                    fft.freq_scale = FFT_MIN_FS;

                fft_setting_update(FFT_SETF_FS);
                fft_popupmsg(FFT_SETF_FS);
                break;
#endif
            case FFT_WINDOW:
                if(++fft.window_func >= FFT_MAX_WF)
                    fft.window_func = FFT_MIN_WF;

                fft_setting_update(FFT_SETF_WF);
                fft_popupmsg(FFT_SETF_WF);
                break;

            default:
                exit_on_usb(button);
                break;
        }
    }

    return PLUGIN_OK;
    (void)parameter;
}
