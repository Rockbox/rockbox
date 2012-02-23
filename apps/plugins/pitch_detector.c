/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008    Lechner Michael / smoking gnu
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * ----------------------------------------------------------------------------
 *
 * INTRODUCTION:
 * OK, this is an attempt to write an instrument tuner for rockbox.
 * It uses a Schmitt trigger algorithm, which I copied from
 * tuneit [ (c) 2004 Mario Lang <mlang@delysid.org> ], for detecting the
 * fundamental freqency of a sound. A FFT algorithm would be more accurate
 * but also much slower.
 *
 * TODO:
 * - Adapt the Yin FFT algorithm, which would reduce complexity from O(n^2)
 *   to O(nlogn), theoretically reducing latency by a factor of ~10. -David
 *
 * MAJOR CHANGES:
 * 08.03.2008        Started coding
 * 21.03.2008        Pitch detection works more or less
 *                   Button definitions for most targets added
 * 02.04.2008        Proper GUI added
 *                   Todo, Major Changes and Current Limitations added
 * 08.19.2009        Brought the code up to date with current plugin standards
 *                   Made it work more nicely with color, BW and grayscale
 *                   Changed pitch detection to use the Yin algorithm (better
 *                      detection, but slower -- would be ~4x faster with
 *                      fixed point math, I think).  Code was poached from the
 *                      Aubio sound processing library (aubio.org). -David
 * 08.31.2009        Lots of changes:
 *                   Added a menu to tweak settings
 *                   Converted everything to fixed point (greatly improving
 *                       latency)
 *                   Improved the display
 *                   Improved efficiency with judicious use of cpu_boost, the
 *                       backlight, and volume detection to limit unneeded
 *                       calculation
 *                   Fixed a problem that caused an octave-off error
 *                   -David
 * 05.14.2010        Multibuffer continuous recording with two buffers
 *
 *
 * CURRENT LIMITATIONS:
 * - No gapless recording.  Strictly speaking true gappless isn't possible,
 *   since the algorithm takes longer to calculate than the length of the
 *   sample, but latency could be improved a bit with proper use of the DMA
 *   recording functions.
 * - Due to how the Yin algorithm works, latency is higher for lower
 *   frequencies.
 */

#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "lib/picture.h"
#include "lib/helper.h"
#include "pluginbitmaps/pitch_notes.h"


/* Some fixed point calculation stuff */
typedef int32_t fixed;
#define FIXED_PRECISION 18
#define FP_MAX ((fixed) {0x7fffffff})
#define FP_MIN ((fixed) {-0x80000000})
#define int2fixed(x)    ((fixed)((x) << FIXED_PRECISION))
#define int2mantissa(x) ((fixed)(x))
#define fixed2int(x)    ((int)((x) >> FIXED_PRECISION))
#define fixed2float(x)  (((float)(x)) / ((float)(1 << FIXED_PRECISION)))
#define float2fixed(x) ((fixed)(x * (float)(1 << FIXED_PRECISION)))

/* I adapted these ones from the Rockbox fixed point library */
#define fp_mul(x, y) \
    ((fixed)((((int64_t)((x))) * ((int64_t)((y)))) >> (FIXED_PRECISION)))
#define fp_div(x, y) \
    ((fixed)((((int64_t)((x))) << (FIXED_PRECISION)) / ((int64_t)((y)))))
/* Operators for fixed point */
#define fp_add(x, y) ((fixed)((x) +  (y)))
#define fp_sub(x, y) ((fixed)((x) -  (y)))
#define fp_shl(x, y) ((fixed)((x) << (y)))
#define fp_shr(x, y) ((fixed)((x) >> (y)))
#define fp_neg(x)    ((fixed)(-(x)))
#define fp_gt(x, y)  ((x) >  (y))
#define fp_gte(x, y) ((x) >= (y))
#define fp_lt(x, y)  ((x) <  (y))
#define fp_lte(x, y) ((x) <= (y))
#define fp_sqr(x)    fp_mul((x), (x))
#define fp_equal(x, y) ((x) == (y))
#define fp_round(x)  (fixed2int(fp_add((x), float2fixed(0.5))))
#define fp_data(x)   (x)
#define fp_frac(x)   (fp_sub((x), int2fixed(fixed2int(x))))
#define FP_ZERO      ((fixed)0)
#define FP_LOW       ((fixed)2)

/* Some defines for converting between period and frequency  */

/* I introduce some divisors in this because the fixed point */
/* variables aren't big enough to hold higher than a certain */
/* value.  This loses a bit of precision but it means we     */
/* don't have to use 32.32 variables (yikes).                */
/* With an 18-bit decimal precision, the max value in the    */
/* integer part is 8192.  Divide 44100 by 7 and it'll fit in */
/* that variable.                                            */
#define fp_period2freq(x) fp_div(int2fixed(sample_rate / 7), \
                                 fp_div((x),int2fixed(7)))
#define fp_freq2period(x) fp_period2freq(x)
#define period2freq(x)    (sample_rate / (x))
#define freq2period(x)    period2freq(x)

#define sqr(x)    ((x)*(x))

/* Some constants for tuning */
#define A_FREQ          float2fixed(440.0f)
#define D_NOTE          float2fixed(1.059463094359f)
#define LOG_D_NOTE      float2fixed(1.0f/12.0f)
#define D_NOTE_SQRT     float2fixed(1.029302236643f)
#define LOG_2           float2fixed(1.0f)

/* The recording buffer size */
/* This is how much is sampled at a time. */
/* It also determines latency -- if BUFFER_SIZE == sample_rate then */
/*   there'll be one sample per second, or a latency of one second. */
/* Furthermore, the lowest detectable frequency will be about twice */
/*   the number of reads per second                                 */
/* If we ever switch to Yin FFT algorithm then this needs to be
   a power of 2 */
#define BUFFER_SIZE 4096
#define SAMPLE_SIZE 4096
#define SAMPLE_SIZE_MIN 1024
#define YIN_BUFFER_SIZE (BUFFER_SIZE / 4)

#define LCD_FACTOR (fp_div(int2fixed(LCD_WIDTH), int2fixed(100)))
/* The threshold for the YIN algorithm */
#define DEFAULT_YIN_THRESHOLD 5  /* 0.10 */
static const fixed yin_threshold_table[] IDATA_ATTR =
{
    float2fixed(0.01),
    float2fixed(0.02),
    float2fixed(0.03),
    float2fixed(0.04),
    float2fixed(0.05),
    float2fixed(0.10),
    float2fixed(0.15),
    float2fixed(0.20),
    float2fixed(0.25),
    float2fixed(0.30),
    float2fixed(0.35),
    float2fixed(0.40),
    float2fixed(0.45),
    float2fixed(0.50),
};

/* Structure for the reference frequency (frequency of A)
 * It's used for scaling the frequency before finding out
 * the note. The frequency is scaled in a way that the main
 * algorithm can assume the frequency of A to be 440 Hz.
 */
static const struct
{
    const int frequency;  /* Frequency in Hz */
    const fixed ratio;    /* 440/frequency   */
    const fixed logratio; /* log2(factor)    */
}  freq_A[] =
{
    {435, float2fixed(1.011363636), float2fixed( 0.016301812)},
    {436, float2fixed(1.009090909), float2fixed( 0.013056153)},
    {437, float2fixed(1.006818182), float2fixed( 0.009803175)},
    {438, float2fixed(1.004545455), float2fixed( 0.006542846)},
    {439, float2fixed(1.002272727), float2fixed( 0.003275132)},
    {440, float2fixed(1.000000000), float2fixed( 0.000000000)},
    {441, float2fixed(0.997727273), float2fixed(-0.003282584)},
    {442, float2fixed(0.995454545), float2fixed(-0.006572654)},
    {443, float2fixed(0.993181818), float2fixed(-0.009870244)},
    {444, float2fixed(0.990909091), float2fixed(-0.013175389)},
    {445, float2fixed(0.988636364), float2fixed(-0.016488123)},
};

/* Index of the entry for 440 Hz in the table (default frequency for A) */
#define DEFAULT_FREQ_A 5
#define NUM_FREQ_A (sizeof(freq_A)/sizeof(freq_A[0]))

/* How loud the audio has to be to start displaying pitch  */
/* Must be between 0 and 100                               */
#define VOLUME_THRESHOLD (50)

/* Change to AUDIO_SRC_LINEIN if you want to record from line-in */
#ifdef HAVE_MIC_IN
#define INPUT_TYPE AUDIO_SRC_MIC
#else
#define INPUT_TYPE AUDIO_SRC_LINEIN
#endif

/* How many decimal places to display for the Hz value */
#define DISPLAY_HZ_PRECISION 100

/* Where to put the various GUI elements */
static int note_y;
static int bar_grad_y;
#define LCD_RES_MIN (LCD_HEIGHT < LCD_WIDTH ? LCD_HEIGHT : LCD_WIDTH)
#define BAR_PADDING (LCD_RES_MIN / 32)
#define BAR_Y       (LCD_HEIGHT * 3 / 4)
#define BAR_HEIGHT  (LCD_RES_MIN / 4 - BAR_PADDING)
#define BAR_HLINE_Y (BAR_Y - BAR_PADDING)
#define BAR_HLINE_Y2 (BAR_Y + BAR_HEIGHT + BAR_PADDING - 1)
#define HZ_Y        0
#define GRADUATION  10       /* Subdivisions of the whole 100-cent scale */

/* Bitmaps for drawing the note names.  These need to have height
   <= (bar_grad_y - note_y), or 15/32 * LCD_HEIGHT
 */
#define NUM_NOTE_IMAGES  9
#define NOTE_INDEX_A     0
#define NOTE_INDEX_B     1
#define NOTE_INDEX_C     2
#define NOTE_INDEX_D     3
#define NOTE_INDEX_E     4
#define NOTE_INDEX_F     5
#define NOTE_INDEX_G     6
#define NOTE_INDEX_SHARP 7
#define NOTE_INDEX_FLAT  8

static const struct picture note_bitmaps =
{
    pitch_notes,
    BMPWIDTH_pitch_notes,
    BMPHEIGHT_pitch_notes,
    BMPHEIGHT_pitch_notes/NUM_NOTE_IMAGES
};


static unsigned int sample_rate;
static int audio_head = 0; /* which of the two buffers to use? */
static volatile int audio_tail = 0; /* which of the two buffers to record? */
/* It's stereo, so make the buffer twice as big */
#ifndef SIMULATOR
static int16_t audio_data[2][BUFFER_SIZE] MEM_ALIGN_ATTR;
static fixed yin_buffer[YIN_BUFFER_SIZE] IBSS_ATTR;
#ifdef PLUGIN_USE_IRAM
static int16_t iram_audio_data[BUFFER_SIZE] IBSS_ATTR;
#else
#define iram_audio_data audio_data[audio_head]
#endif
#endif

/* Notes within one (reference) scale */
static const struct
{
    const char *name;    /* Name of the note, e.g. "A#" */
    const fixed freq;    /* Note frequency, Hz          */
    const fixed logfreq; /* log2(frequency)             */
} notes[] =
{
    {"A" , float2fixed(440.0000000f), float2fixed(8.781359714f)},
    {"A#", float2fixed(466.1637615f), float2fixed(8.864693047f)},
    {"B" , float2fixed(493.8833013f), float2fixed(8.948026380f)},
    {"C" , float2fixed(523.2511306f), float2fixed(9.031359714f)},
    {"C#", float2fixed(554.3652620f), float2fixed(9.114693047f)},
    {"D" , float2fixed(587.3295358f), float2fixed(9.198026380f)},
    {"D#", float2fixed(622.2539674f), float2fixed(9.281359714f)},
    {"E" , float2fixed(659.2551138f), float2fixed(9.364693047f)},
    {"F" , float2fixed(698.4564629f), float2fixed(9.448026380f)},
    {"F#", float2fixed(739.9888454f), float2fixed(9.531359714f)},
    {"G" , float2fixed(783.9908720f), float2fixed(9.614693047f)},
    {"G#", float2fixed(830.6093952f), float2fixed(9.698026380f)},
};

/* GUI */
#if LCD_DEPTH > 1
static unsigned front_color;
#endif
static int font_w,font_h;
static int bar_x_0;
static int lbl_x_minus_50, lbl_x_minus_20, lbl_x_0, lbl_x_20, lbl_x_50;

/* Settings for the plugin */
static struct tuner_settings
{
    unsigned volume_threshold;
    unsigned record_gain;
    unsigned sample_size;
    unsigned lowest_freq;
    unsigned yin_threshold;
    int      freq_A;        /* Index of the frequency of A */
    bool     use_sharps;
    bool     display_hz;
    int      key_transposition; /* Which note to display as 'C'. */
        /* 0=C, 1=D-flat, 2=D, ..., 11=B. This is useful if you  */
        /* use a transposing instrument. In that case, this      */
        /* setting tells which 'real' note is played by the      */
        /* instrument if you play a written 'C'. Thus, this      */
        /* setting is the number of semitones from the real 'C'  */
        /* up to the 'instrument key'.                           */
} settings;

/* By default, the real 'C' is displayed as 'C' */
#define DEFAULT_KEY_TRANSPOSITION 0


/*=================================================================*/
/* Settings loading and saving(adapted from the clock plugin)      */
/*=================================================================*/

#define SETTINGS_FILENAME PLUGIN_APPS_DATA_DIR "/.pitch_detector_settings"

/* The settings as they exist on the hard disk, so that
 * we can know at saving time if changes have been made */
static struct tuner_settings hdd_settings;

/*---------------------------------------------------------------------*/

static bool settings_needs_saving(void)
{
    return(rb->memcmp(&settings, &hdd_settings, sizeof(settings)));
}

/*---------------------------------------------------------------------*/

static void tuner_settings_reset(void)
{
    settings = (struct tuner_settings) {
        .volume_threshold = VOLUME_THRESHOLD,
        .record_gain = rb->global_settings->rec_mic_gain,
        .sample_size = BUFFER_SIZE,
        .lowest_freq = period2freq(BUFFER_SIZE / 4),
        .yin_threshold = DEFAULT_YIN_THRESHOLD,
        .freq_A = DEFAULT_FREQ_A,
        .use_sharps = true,
        .display_hz = false,
        .key_transposition = DEFAULT_KEY_TRANSPOSITION,
    };
}

/*---------------------------------------------------------------------*/

static void load_settings(void)
{
    int fd = rb->open(SETTINGS_FILENAME, O_RDONLY);
    if(fd < 0){ /* file doesn't exist */
        /* Initializes the settings with default values at least */
        tuner_settings_reset();
        return;
    }

    /* basic consistency check */
    if(rb->filesize(fd) == sizeof(settings)){
        rb->read(fd, &settings, sizeof(settings));
        rb->memcpy(&hdd_settings, &settings, sizeof(settings));
    }
    else{
        tuner_settings_reset();
    }

    rb->close(fd);
}

/*---------------------------------------------------------------------*/

static void save_settings(void)
{
    if(!settings_needs_saving())
        return;

    int fd = rb->creat(SETTINGS_FILENAME, 0666);
    if(fd >= 0){ /* does file exist? */
        rb->write (fd, &settings, sizeof(settings));
        rb->close(fd);
    }
}

/*=================================================================*/
/* MENU                                                            */
/*=================================================================*/

/* Keymaps */
const struct button_mapping* plugin_contexts[]={
    pla_main_ctx,
#if NB_SCREENS == 2
    pla_remote_ctx,
#endif
};
#define PLA_ARRAY_COUNT sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

/* Option strings */

/* This has to match yin_threshold_table */
static const struct opt_items yin_threshold_text[] =
{
    { "0.01", -1 },
    { "0.02", -1 },
    { "0.03", -1 },
    { "0.04", -1 },
    { "0.05", -1 },
    { "0.10", -1 },
    { "0.15", -1 },
    { "0.20", -1 },
    { "0.25", -1 },
    { "0.30", -1 },
    { "0.35", -1 },
    { "0.40", -1 },
    { "0.45", -1 },
    { "0.50", -1 },
};

static const struct opt_items accidental_text[] =
{
    { "Flat", -1 },
    { "Sharp", -1 },
};

static const struct opt_items transpose_text[] =
{
    { "C (Concert Pitch)", -1 },
    { "D-flat", -1 },
    { "D", -1 },
    { "E-flat", -1 },
    { "E", -1 },
    { "F", -1 },
    { "G-flat", -1 },
    { "G", -1 },
    { "A-flat", -1 },
    { "A", -1 },
    { "B-flat", -1 },
    { "B", -1 },
};

static void set_min_freq(int new_freq)
{
    settings.sample_size = freq2period(new_freq) * 4;

    /* clamp the sample size between min and max */
    if(settings.sample_size <= SAMPLE_SIZE_MIN)
        settings.sample_size = SAMPLE_SIZE_MIN;
    else if(settings.sample_size >= BUFFER_SIZE)
        settings.sample_size = BUFFER_SIZE;

    /* sample size must be divisible by 4 - round up */
    settings.sample_size = (settings.sample_size + 3) & ~3;
}

/* Displays the menu. Returns true iff the user selects 'quit'. */
static bool main_menu(void)
{
    int selection = 0;
    bool done = false;
    bool exit_tuner = false;
    int choice;
    int freq_val;
    bool reset;

    backlight_use_settings();
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    rb->cancel_cpu_boost();
#endif

    MENUITEM_STRINGLIST(menu,"Tuner Settings",NULL,
                        "Return to Tuner",
                        "Volume Threshold",
                        "Listening Volume",
                        "Lowest Frequency",
                        "Algorithm Pickiness",
                        "Accidentals",
                        "Key Transposition",
                        "Display Frequency (Hz)",
                        "Frequency of A (Hz)",
                        "Reset Settings",
                        "Quit");

    while(!done)
    {
        choice = rb->do_menu(&menu, &selection, NULL, false);
        switch(choice)
        {
            case 1:
                rb->set_int("Volume Threshold", "%", UNIT_INT,
                               &settings.volume_threshold,
                               NULL, 5, 5, 95, NULL);
                break;
            case 2:
                rb->set_int("Listening Volume", "%", UNIT_INT,
                               &settings.record_gain,
                               NULL, 1, rb->sound_min(SOUND_MIC_GAIN),
                               rb->sound_max(SOUND_MIC_GAIN), NULL);
                break;
            case 3:
                rb->set_int("Lowest Frequency", "Hz", UNIT_INT,
                               &settings.lowest_freq, set_min_freq, 1,
                               /* Range depends on the size of the buffer */
                               sample_rate / (BUFFER_SIZE / 4),
                               sample_rate / (SAMPLE_SIZE_MIN / 4), NULL);
                break;
            case 4:
                rb->set_option(
                    "Algorithm Pickiness (Lower -> more discriminating)",
                    &settings.yin_threshold,
                    INT, yin_threshold_text,
                    sizeof(yin_threshold_text) / sizeof(yin_threshold_text[0]),
                    NULL);
                break;
            case 5:
                rb->set_option("Display Accidentals As",
                               &settings.use_sharps,
                               BOOL, accidental_text, 2, NULL);
                break;
            case 6:
                rb->set_option("Key Transposition",
                              &settings.key_transposition,
                              INT, transpose_text, 12, NULL);
                break;
            case 7:
                rb->set_bool("Display Frequency (Hz)",
                             &settings.display_hz);
                break;
            case 8:
                freq_val = freq_A[settings.freq_A].frequency;
                rb->set_int("Frequency of A (Hz)",
                    "Hz", UNIT_INT, &freq_val, NULL,
                    1, freq_A[0].frequency, freq_A[NUM_FREQ_A-1].frequency,
                    NULL);
                settings.freq_A = freq_val - freq_A[0].frequency;
                break;
            case 9:
                reset = false;
                rb->set_bool("Reset Tuner Settings?", &reset);
                if (reset)
                    tuner_settings_reset();
                break;
            case 10:
                exit_tuner = true;
                done = true;
                break;
            case 0:
            default:
                /* Return to the tuner */
                done = true;
                break;
        }
    }

    backlight_ignore_timeout();
    return exit_tuner;
}

/*=================================================================*/
/* Binary Log                                                      */
/*=================================================================*/

/* Fixed-point log base 2*/
/* Adapted from python code at
   http://en.wikipedia.org/wiki/Binary_logarithm#Algorithm
*/
static fixed log(fixed inp)
{
    fixed x = inp;
    fixed fp = int2fixed(1);
    fixed res = int2fixed(0);

    if(fp_lte(x, FP_ZERO))
    {
        return FP_MIN;
    }

    /* Integer part*/
    /* while x<1 */
    while(fp_lt(x, int2fixed(1)))
    {
        res = fp_sub(res, int2fixed(1));
        x = fp_shl(x, 1);
    }
    /* while x>=2 */
    while(fp_gte(x, int2fixed(2)))
    {
        res = fp_add(res, int2fixed(1));
        x = fp_shr(x, 1);
    }

    /* Fractional part */
    /* while fp > 0 */
    while(fp_gt(fp, FP_ZERO))
    {
        fp = fp_shr(fp, 1);
        x = fp_mul(x, x);
        /* if x >= 2 */
        if(fp_gte(x, int2fixed(2)))
        {
            x = fp_shr(x, 1);
            res = fp_add(res, fp);
        }
    }

    return res;
}

/*=================================================================*/
/* GUI Stuff                                                       */
/*=================================================================*/

/* Draw the note bitmap */
static void draw_note(const char *note)
{
    int i;
    int note_x = (LCD_WIDTH - BMPWIDTH_pitch_notes) / 2;
    int accidental_index = NOTE_INDEX_SHARP;

    i = note[0]-'A';

    if(note[1] == '#')
    {
        if(!(settings.use_sharps))
        {
            i = (i + 1) % 7;
            accidental_index = NOTE_INDEX_FLAT;
        }

        vertical_picture_draw_sprite(rb->screens[0],
                                     &note_bitmaps,
                                     accidental_index,
                                     LCD_WIDTH / 2,
                                     note_y);
        note_x = LCD_WIDTH / 2 - BMPWIDTH_pitch_notes;
    }

    vertical_picture_draw_sprite(rb->screens[0], &note_bitmaps, i,
                                 note_x,
                                 note_y);
}

/* Draw the red bar and the white lines */
static void draw_bar(fixed wrong_by_cents)
{
    unsigned n;
    int x;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255)); /* Color screens */
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_BLACK);      /* Greyscale screens */
#endif

    rb->lcd_hline(0,LCD_WIDTH-1, BAR_HLINE_Y);
    rb->lcd_hline(0,LCD_WIDTH-1, BAR_HLINE_Y2);

    /* Draw graduation lines on the off-by readout */
    for(n = 0; n <= GRADUATION; n++)
    {
        x = (LCD_WIDTH * n + GRADUATION / 2) / GRADUATION;
        if (x >= LCD_WIDTH)
            x = LCD_WIDTH - 1;
        rb->lcd_vline(x, BAR_HLINE_Y, BAR_HLINE_Y2);
    }

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(front_color);
#endif
    rb->lcd_putsxyf(lbl_x_minus_50    ,bar_grad_y, "%d", -50);
    rb->lcd_putsxyf(lbl_x_minus_20    ,bar_grad_y, "%d", -20);
    rb->lcd_putsxyf(lbl_x_0           ,bar_grad_y, "%d",   0);
    rb->lcd_putsxyf(lbl_x_20          ,bar_grad_y, "%d",  20);
    rb->lcd_putsxyf(lbl_x_50          ,bar_grad_y, "%d",  50);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(255,0,0));   /* Color screens */
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DARKGRAY);           /* Greyscale screens */
#endif

    if (fp_gt(wrong_by_cents, FP_ZERO))
    {
        rb->lcd_fillrect(bar_x_0, BAR_Y,
            fixed2int(fp_mul(wrong_by_cents, LCD_FACTOR)), BAR_HEIGHT);
    }
    else
    {
        rb->lcd_fillrect(bar_x_0 + fixed2int(fp_mul(wrong_by_cents,LCD_FACTOR)),
                         BAR_Y,
                         fixed2int(fp_mul(wrong_by_cents, LCD_FACTOR)) * -1,
                         BAR_HEIGHT);
    }
}

/* Calculate how wrong the note is and draw the GUI */
static void display_frequency (fixed freq)
{
    fixed ldf, mldf;
    fixed lfreq, nfreq;
    fixed orig_freq;
    int i, note = 0;

    if (fp_lt(freq, FP_LOW))
        freq = FP_LOW;

    /* We calculate the frequency and its log as if */
    /* the reference frequency of A were 440 Hz.    */
    orig_freq = freq;
    lfreq = fp_add(log(freq), freq_A[settings.freq_A].logratio);
    freq = fp_mul(freq, freq_A[settings.freq_A].ratio);

    /* This calculates a log freq offset for note A */
    /* Get the frequency to within the range of our reference table, */
    /* i.e. into the right octave.                                   */
    while (fp_lt(lfreq, fp_sub(notes[0].logfreq, fp_shr(LOG_D_NOTE, 1))))
        lfreq = fp_add(lfreq, LOG_2);
    while (fp_gte(lfreq, fp_sub(fp_add(notes[0].logfreq, LOG_2),
           fp_shr(LOG_D_NOTE, 1))))
        lfreq = fp_sub(lfreq, LOG_2);
    mldf = LOG_D_NOTE;
    for (i=0; i<12; i++)
    {
        ldf = fp_gt(fp_sub(lfreq,notes[i].logfreq), FP_ZERO) ?
                 fp_sub(lfreq,notes[i].logfreq) : fp_neg(fp_sub(lfreq,notes[i].logfreq));
        if (fp_lt(ldf, mldf))
        {
            mldf = ldf;
            note = i;
        }
    }
    nfreq = notes[note].freq;
    while (fp_gt(fp_div(nfreq, freq), D_NOTE_SQRT))
        nfreq = fp_shr(nfreq, 1);

    while (fp_gt(fp_div(freq, nfreq), D_NOTE_SQRT))
        nfreq = fp_shl(nfreq, 1);

    ldf = fp_mul(int2fixed(1200), log(fp_div(freq,nfreq)));

    rb->lcd_clear_display();
    draw_bar(ldf);                /* The red bar */
    if(fp_round(freq) != 0)
    {
        /* Raise the displayed pitch an octave minus key_transposition */
        /* semitones, effectively lowering it.  Note that the pitch    */
        /* displayed alongside the frequency is unaffected.            */
        int transposition = 12 - settings.key_transposition;

        draw_note(notes[(note + transposition) % 12].name);

        if(settings.display_hz)
        {
#if LCD_DEPTH > 1
            rb->lcd_set_foreground(front_color);
#endif
            rb->lcd_putsxyf(0, HZ_Y, "%s : %d cents (%d.%02dHz)",
                         notes[note].name, fp_round(ldf) ,fixed2int(orig_freq),
                         fp_round(fp_mul(fp_frac(orig_freq),
                                         int2fixed(DISPLAY_HZ_PRECISION))));
        }
    }
    rb->lcd_update();
}

#ifndef SIMULATOR
/*-----------------------------------------------------------------------
 * Functions for the Yin algorithm
 *
 * These were all adapted from the versions in Aubio v0.3.2
 * Here's what the Aubio documentation has to say:
 *
 * This algorithm was developped by A. de Cheveigne and H. Kawahara and
 * published in:
 *
 * de Cheveign?, A., Kawahara, H. (2002) "YIN, a fundamental frequency
 * estimator for speech and music", J. Acoust. Soc. Am. 111, 1917-1930.
 *
 * see http://recherche.ircam.fr/equipes/pcm/pub/people/cheveign.html
-------------------------------------------------------------------------*/

/* Find the index of the minimum element of an array of floats */
static unsigned vec_min_elem(fixed *s, unsigned buflen)
{
    unsigned j, pos=0.0f;
    fixed tmp = s[0];
    for (j=0; j < buflen; j++)
    {
        if(fp_gt(tmp, s[j]))
        {
            pos = j;
            tmp = s[j];
        }
    }
    return pos;
}


static inline fixed aubio_quadfrac(fixed s0, fixed s1, fixed s2, fixed pf)
{
    /* Original floating point version: */
    /* tmp = s0 + (pf/2.0f) * (pf * ( s0 - 2.0f*s1 + s2 ) -
             3.0f*s0 + 4.0f*s1 - s2);*/
    /* Converted to explicit operator precedence: */
    /* tmp = s0 + ((pf/2.0f) * ((((pf * ((s0 - (2*s1)) + s2)) -
            (3*s0)) + (4*s1)) - s2)); */

    /* I made it look like this so I could easily track the precedence and */
    /* make sure it matched the original expression                        */
    /* Oy, this is when I really wish I could do C++ operator overloading  */
    fixed tmp = fp_add
                (
                  s0,
                  fp_mul
                  (
                    fp_shr(pf, 1),
                    fp_sub
                    (
                      fp_add
                      (
                        fp_sub
                        (
                          fp_mul
                          (
                            pf,
                            fp_add
                            (
                              fp_sub
                              (
                                s0,
                                fp_shl(s1, 1)
                              ),
                              s2
                            )
                          ),
                          fp_mul
                          (
                            float2fixed(3.0f),
                            s0
                          )
                        ),
                        fp_shl(s1, 2)
                      ),
                      s2
                    )
                  )
                );
    return tmp;
}

#define QUADINT_STEP float2fixed(1.0f/200.0f)

static fixed ICODE_ATTR vec_quadint_min(fixed *x, unsigned bufsize, unsigned pos, unsigned span)
{
    fixed res, frac, s0, s1, s2;
    fixed exactpos = int2fixed(pos);
    /* init resold to something big (in case x[pos+-span]<0)) */
    fixed resold = FP_MAX;

    if ((pos > span) && (pos < bufsize-span))
    {
        s0 = x[pos-span];
        s1 = x[pos]     ;
        s2 = x[pos+span];
        /* increase frac */
        for (frac = float2fixed(0.0f);
             fp_lt(frac, float2fixed(2.0f));
             frac = fp_add(frac, QUADINT_STEP))
        {
            res = aubio_quadfrac(s0, s1, s2, frac);
            if (fp_lt(res, resold))
            {
                resold = res;
            }
            else
            {
                /* exactpos += (frac-QUADINT_STEP)*span - span/2.0f; */
                exactpos = fp_add(exactpos,
                                  fp_sub(
                                    fp_mul(
                                      fp_sub(frac, QUADINT_STEP),
                                      int2fixed(span)
                                    ),
                                    int2fixed(span)
                                  )
                                 );
                break;
            }
        }
    }
    return exactpos;
}

/* Calculate the period of the note in the
     buffer using the YIN algorithm */
/* The yin pointer is just a buffer that the algorithm uses as a work
     space.  It needs to be half the length of the input buffer. */

static fixed ICODE_ATTR pitchyin(int16_t *input, fixed *yin)
{
    fixed retval;
    unsigned j,tau = 0;
    int period;
    unsigned yin_size = settings.sample_size / 4;

    fixed tmp = FP_ZERO, tmp2 = FP_ZERO;
    yin[0] = int2fixed(1);
    for (tau = 1; tau < yin_size; tau++)
    {
        yin[tau] = FP_ZERO;
        for (j = 0; j < yin_size; j++)
        {
            tmp = fp_sub(int2mantissa(input[2 * j]),
                         int2mantissa(input[2 * (j + tau)]));
            yin[tau] = fp_add(yin[tau], fp_mul(tmp, tmp));
        }
        tmp2 = fp_add(tmp2, yin[tau]);
        if(!fp_equal(tmp2, FP_ZERO))
        {
            yin[tau] = fp_mul(yin[tau], fp_div(int2fixed(tau), tmp2));
        }
        period = tau - 3;
        if(tau > 4 && fp_lt(yin[period],
                            yin_threshold_table[settings.yin_threshold])
                   && fp_lt(yin[period], yin[period+1]))
        {
            retval = vec_quadint_min(yin, yin_size, period, 1);
            return retval;
        }
    }
    retval =  vec_quadint_min(yin, yin_size,
                             vec_min_elem(yin, yin_size), 1);
    return retval;
    /*return FP_ZERO;*/
}

/*-----------------------------------------------------------------*/

static uint32_t ICODE_ATTR buffer_magnitude(int16_t *input)
{
    unsigned n;
    uint64_t tally = 0;
    const unsigned size = settings.sample_size;

    /* Operate on only one channel of the stereo signal */
    for(n = 0; n < size; n+=2)
    {
        int s = input[n];
        tally += s * s;
    }

    tally /= size / 2;

    /* now tally holds the average of the squares of all the samples */
    /* It must be between 0 and 0x7fff^2, so it fits in 32 bits      */
    return (uint32_t)tally;
}

/* Stop the recording when the buffer is full */
static void recording_callback(void **start, size_t *size)
{
    int tail = audio_tail ^ 1;

    /* Do not overrun the reader. Reuse current buffer if full. */
    if (tail != audio_head)
        audio_tail = tail;

    /* Always record full buffer, even if not required */
    *start = audio_data[tail];
    *size = BUFFER_SIZE * sizeof (int16_t);
}
#endif /* SIMULATOR */

/* Start recording */
static void record_data(void)
{
#ifndef SIMULATOR
    /* Always record full buffer, even if not required */
    rb->pcm_record_data(recording_callback, NULL,
                        audio_data[audio_tail],
                        BUFFER_SIZE * sizeof (int16_t));
#endif
}

/* The main program loop */
static void record_and_get_pitch(void)
{
    int quit=0, button;
#ifndef SIMULATOR
    bool redraw = true;
#endif
    /* For tracking the latency */
    /*
    long timer;
    char debug_string[20];
    */
#ifndef SIMULATOR
    fixed period;
    bool waiting = false;
#else
    audio_tail = 1;
#endif

    backlight_ignore_timeout();

    record_data();

    while(!quit)
    {
        while (audio_head == audio_tail && !quit)      /* wait for the buffer to be filled */
        {
            button=pluginlib_getaction(HZ/100, plugin_contexts, PLA_ARRAY_COUNT);

            switch(button)
            {
                case PLA_EXIT:
                    quit=true;
                    break;

                case PLA_CANCEL:
                    rb->pcm_stop_recording();
                    quit = main_menu();
                    if(!quit)
                    {
#ifndef SIMULATOR
                        redraw = true;
#endif
                        record_data();
                    }
                    break;
            }
        }

        if(!quit)
        {
#ifndef SIMULATOR
            /* Only do the heavy lifting if the volume is high enough */
            if(buffer_magnitude(audio_data[audio_head]) >
                sqr(settings.volume_threshold *
                    rb->sound_max(SOUND_MIC_GAIN)))
            {
                waiting = false;
                redraw = false;

            #ifdef HAVE_SCHEDULER_BOOSTCTRL
                rb->trigger_cpu_boost();
            #endif
#ifdef PLUGIN_USE_IRAM
                rb->memcpy(iram_audio_data, audio_data[audio_head],
                           settings.sample_size * sizeof (int16_t));
#endif
                /* This returns the period of the detected pitch in samples */
                period = pitchyin(iram_audio_data, yin_buffer);
                /* Hz = sample rate / period */
                if(fp_gt(period, FP_ZERO))
                {
                    display_frequency(fp_period2freq(period));
                }
                else
                {
                    display_frequency(FP_ZERO);
                }
            }
            else if(redraw || !waiting)
            {
                waiting = true;
                redraw = false;
                display_frequency(FP_ZERO);
            #ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cancel_cpu_boost();
            #endif
            }

            /* Move to next buffer if not empty (but empty *shouldn't* happen
             * here). */
            if (audio_head != audio_tail)
                audio_head ^= 1;
#else /* SIMULATOR */
            /* Display a preselected frequency */
            display_frequency(int2fixed(445));
#endif
        }
    }
    rb->pcm_close_recording();
    rb->pcm_set_frequency(HW_SAMPR_RESET | SAMPR_TYPE_REC);
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    rb->cancel_cpu_boost();
#endif

    backlight_use_settings();
}

/* Init recording, tuning, and GUI */
static void init_everything(void)
{
    /* Disable all talking before initializing IRAM */
    rb->talk_disable(true);

    load_settings();
    rb->storage_sleep();

    /* Stop all playback */
    rb->plugin_get_audio_buffer(NULL);

    /* --------- Init the audio recording ----------------- */
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
    rb->audio_set_input_source(INPUT_TYPE, SRCF_RECORDING);

    /* set to maximum gain */
    rb->audio_set_recording_gain(settings.record_gain,
                                 settings.record_gain,
                                 AUDIO_GAIN_MIC);

    /* Highest C on piano is approx 4.186 kHz, so we need just over
     * 8.372 kHz to pass it. */
    sample_rate = rb->round_value_to_list32(9000, rb->rec_freq_sampr,
                                            REC_NUM_FREQ, false);
    sample_rate = rb->rec_freq_sampr[sample_rate];
    rb->pcm_set_frequency(sample_rate | SAMPR_TYPE_REC);
    rb->pcm_init_recording();

    /* avoid divsion by zero */
    if(settings.lowest_freq == 0)
        settings.lowest_freq = period2freq(BUFFER_SIZE / 4);

    /* GUI */
#if LCD_DEPTH > 1
    front_color = rb->lcd_get_foreground();
#endif
    rb->lcd_getstringsize("X", &font_w, &font_h);

    bar_x_0  = LCD_WIDTH / 2;
    lbl_x_minus_50 = 0;
    lbl_x_minus_20 = (LCD_WIDTH / 2) -
                     fixed2int(fp_mul(LCD_FACTOR, int2fixed(20))) - font_w;
    lbl_x_0  = (LCD_WIDTH - font_w) / 2;
    lbl_x_20 = (LCD_WIDTH / 2) +
               fixed2int(fp_mul(LCD_FACTOR, int2fixed(20))) - font_w;
    lbl_x_50 = LCD_WIDTH - 2 * font_w;

    bar_grad_y = BAR_Y - BAR_PADDING - font_h;
    /* Put the note right between the top and bottom text elements */
    note_y = ((font_h + bar_grad_y - note_bitmaps.slide_height) / 2);

    rb->talk_disable(false);
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    init_everything();
    record_and_get_pitch();
    save_settings();

    return PLUGIN_OK;
}
