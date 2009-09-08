/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$    
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
 * - Find someone who knows how recording actually works, and rewrite the 
 *   recording code to use proper, gapless recording with a callback function
 *   that provides new buffer, instead of stopping and restarting recording
 *   everytime the buffer is full
 * - Convert all floating point operations to fixed-point
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

PLUGIN_HEADER

/* First figure out what sample rate we're going to use */
#if (REC_SAMPR_CAPS & SAMPR_CAP_44)
#define SAMPLE_RATE     SAMPR_44
#elif (REC_SAMPR_CAPS & SAMPR_CAP_22)
#define SAMPLE_RATE     SAMPR_22
#elif (REC_SAMPR_CAPS & SAMPR_CAP_11)
#define SAMPLE_RATE     SAMPR_11
#endif

/* Some fixed point calculation stuff */
typedef int32_t fixed_data;
struct _fixed
{
    fixed_data a;
};
typedef struct _fixed fixed;
#define FIXED_PRECISION 18
#define FP_MAX ((fixed) {0x7fffffff})
#define FP_MIN ((fixed) {-0x80000000})
#define int2fixed(x)    ((fixed){(x) << FIXED_PRECISION})
#define int2mantissa(x) ((fixed){x})
#define fixed2int(x)    ((int)((x).a >> FIXED_PRECISION))
#define fixed2float(x)  (((float)(x).a) / ((float)(1 << FIXED_PRECISION)))
#define float2fixed(x) \
    ((fixed){(fixed_data)(x * (float)(1 << FIXED_PRECISION))})
/* I adapted these ones from the Rockbox fixed point library */
#define fp_mul(x, y) \
    ((fixed){(((int64_t)((x).a)) * ((int64_t)((y).a))) >> (FIXED_PRECISION)})
#define fp_div(x, y) \
    ((fixed){(((int64_t)((x).a)) << (FIXED_PRECISION)) / ((int64_t)((y).a))})
/* Operators for fixed point */
#define fp_add(x, y) ((fixed){(x).a + (y).a})
#define fp_sub(x, y) ((fixed){(x).a - (y).a})
#define fp_shl(x, y) ((fixed){(x).a << y})
#define fp_shr(x, y) ((fixed){(x).a >> y})
#define fp_neg(x)    ((fixed){-(x).a})
#define fp_gt(x, y)  ((x).a > (y).a)
#define fp_gte(x, y) ((x).a >= (y).a)
#define fp_lt(x, y)  ((x).a < (y).a)
#define fp_lte(x, y) ((x).a <= (y).a)
#define fp_sqr(x)    fp_mul((x), (x))
#define fp_equal(x, y) ((x).a == (y).a)
#define fp_round(x)  (fixed2int(fp_add((x), float2fixed(0.5))))
#define fp_data(x)   ((x).a)
#define fp_frac(x)   (fp_sub((x), int2fixed(fixed2int(x))))
#define FP_ZERO      ((fixed){0})
#define FP_LOW       ((fixed){1})

/* Some defines for converting between period and frequency  */
/* I introduce some divisors in this because the fixed point */
/* variables aren't big enough to hold higher than a certain */
/* vallue.  This loses a bit of precision but it means we    */
/* don't have to use 32.32 variables (yikes).                */
/* With an 18-bit decimal precision, the max value in the    */
/* integer part is 8192.  Divide 44100 by 7 and it'll fit in */
/* that variable.                                            */
#define fp_period2freq(x) fp_div(int2fixed(SAMPLE_RATE / 7), \
                                 fp_div((x),int2fixed(7)))
#define fp_freq2period(x) fp_period2freq(x)
#define period2freq(x)    (SAMPLE_RATE / (x))
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
/* It also determines latency -- if BUFFER_SIZE == SAMPLE_RATE then */
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
#define YIN_THRESHOLD float2fixed(0.05f)

/* How loud the audio has to be to start displaying pitch  */
/* Must be between 0 and 100                               */
#define VOLUME_THRESHOLD (50)
/* Change to AUDIO_SRC_LINEIN if you want to record from line-in */
#define INPUT_TYPE AUDIO_SRC_MIC

/* How many decimal places to display for the Hz value */
#define DISPLAY_HZ_PRECISION 100

typedef signed short audio_sample_type;
/* It's stereo, so make the buffer twice as big */
audio_sample_type audio_data[BUFFER_SIZE];
fixed yin_buffer[YIN_BUFFER_SIZE];
static int recording=0;

/* Frequencies of all the notes of the scale */
static const fixed freqs[12] = 
{
    float2fixed(440.0f),        /* A */
    float2fixed(466.1637615f),  /* A# */
    float2fixed(493.8833013f),  /* etc... */
    float2fixed(523.2511306f),
    float2fixed(554.3652620f),
    float2fixed(587.3295358f),
    float2fixed(622.2539674f),
    float2fixed(659.2551138f),
    float2fixed(698.4564629f),
    float2fixed(739.9888454f),
    float2fixed(783.9908720f),
    float2fixed(830.6093952f),
};
/* logarithm of all the notes of the scale */
static const fixed lfreqs[12] =
{
    float2fixed(8.781359714f),
    float2fixed(8.864693047f),
    float2fixed(8.948026380f),
    float2fixed(9.031359714f),
    float2fixed(9.114693047f),
    float2fixed(9.198026380f),
    float2fixed(9.281359714f),
    float2fixed(9.364693047f),
    float2fixed(9.448026380f),
    float2fixed(9.531359714f),
    float2fixed(9.614693047f),
    float2fixed(9.698026380f),
};

/* GUI */
static unsigned back_color, front_color;            
static int font_w, font_h;
static int bar_x_minus_50, bar_x_minus_20, bar_x_0, bar_x_20, bar_x_50;
static int letter_y;     /* y of the notes letters */
static int note_bar_y;   /* y of the yellow bars (under the notes) */
static int bar_h;        /* height of the yellow (note) and red (error) bars */
static int freq_y;       /* y of the line with the frequency */
static int error_ticks_y; /* y of the error values (-50, -20, ...) */
static int error_hline_y; /* y of the top line for error bar */
static int error_hline_y2;/* y of the bottom line for error bar */
static int error_bar_y;   /* y of the error bar */
static int error_bar_margin; /* distance between the error bar and the hline  */

static const char *english_notes[12] = {"A","A#","B","C","C#","D","D#","E",
                                        "F","F#", "G", "G#"};
static const char gui_letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', '#'};
static const char **notes = english_notes;

/*Settings for the plugin */

struct tuner_settings
{
    unsigned volume_threshold;
    unsigned record_gain;
    unsigned sample_size;
    unsigned lowest_freq;
    unsigned yin_threshold;
} tuner_settings;

/*=================================================================*/
/* MENU                                                            */
/*=================================================================*/

/* Keymaps */
const struct button_mapping* plugin_contexts[]={
    generic_actions,
    generic_increase_decrease,
    generic_directions,
#if NB_SCREENS == 2
    remote_directions
#endif
};
#define PLA_ARRAY_COUNT sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

fixed yin_threshold_table[] =
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

#define DEFAULT_YIN_THRESHOLD 5  /* 0.10 */

/* Option strings */
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

void set_min_freq(int new_freq)
{
    tuner_settings.sample_size = freq2period(new_freq) * 4;

    /* clamp the sample size between min and max */
    if(tuner_settings.sample_size <= SAMPLE_SIZE_MIN)
        tuner_settings.sample_size = SAMPLE_SIZE_MIN;
    else if(tuner_settings.sample_size >= BUFFER_SIZE)
        tuner_settings.sample_size = BUFFER_SIZE;
    /* sample size must be divisible by 4 */
    else if(tuner_settings.sample_size % 4 != 0)
        tuner_settings.sample_size += 4 - (tuner_settings.sample_size % 4);
}

bool main_menu(void)
{
    int selection=0;
    bool done = false;
    bool exit_tuner=false;
    int choice;

    MENUITEM_STRINGLIST(menu,"Tuner Settings",NULL,
                        "Return to Tuner",
                        "Volume Threshold",
                        "Listening Volume",
                        "Lowest Frequency",
                        "Algorithm Pickiness",
                        "Quit");

    while(!done)
    {
        choice = rb->do_menu(&menu, &selection, NULL, false);
        switch(choice)
        {
            case 1:
                rb->set_int("Volume Threshold", "%", UNIT_INT,
                               &tuner_settings.volume_threshold,
                               NULL, 5, 5, 95, NULL);
                break;
            case 2:
                rb->set_int("Listening Volume", "%", UNIT_INT,
                               &tuner_settings.record_gain,
                               NULL, 1, rb->sound_min(SOUND_MIC_GAIN), 
                               rb->sound_max(SOUND_MIC_GAIN), NULL);
                break;
            case 3:
                rb->set_int("Lowest Frequency", "Hz", UNIT_INT,
                               &tuner_settings.lowest_freq, set_min_freq, 1,
                               /* Range depends on the size of the buffer */
                               SAMPLE_RATE / (BUFFER_SIZE / 4), 
                               SAMPLE_RATE / (SAMPLE_SIZE_MIN / 4), NULL);
                break;
            case 4:
                rb->set_option(
                    "Algorithm Pickiness (Lower -> more discriminating)",
                    &tuner_settings.yin_threshold,
                    INT, yin_threshold_text, 
                    sizeof(yin_threshold_text) / 
                    sizeof(yin_threshold_text[0]), 
                    NULL);
                break;
            case 5:
                exit_tuner = true;
                done = true;
                break;
            case 0:
            default: /* Return to the tuner */
                done = true;
                break;

        }
    }
    return(exit_tuner);
}

/*=================================================================*/
/* Settings loading and saving(adapted from the clock plugin)      */
/*=================================================================*/

#define SETTINGS_FILENAME PLUGIN_APPS_DIR "/.pitch_settings"

enum message
{
    MESSAGE_LOADING,
    MESSAGE_LOADED,
    MESSAGE_ERRLOAD,
    MESSAGE_SAVING,
    MESSAGE_SAVED,
    MESSAGE_ERRSAVE
};

enum settings_file_status
{
    LOADED, ERRLOAD,
    SAVED, ERRSAVE
};

/* The settings as they exist on the hard disk, so that 
 * we can know at saving time if changes have been made */
struct tuner_settings hdd_tuner_settings;

/*---------------------------------------------------------------------*/

bool settings_needs_saving(struct tuner_settings* settings)
{
    return(rb->memcmp(settings, &hdd_tuner_settings, sizeof(*settings)));
}

/*---------------------------------------------------------------------*/

void tuner_settings_reset(struct tuner_settings* settings)
{
    settings->volume_threshold = VOLUME_THRESHOLD;
    settings->record_gain = rb->global_settings->rec_mic_gain;
    settings->sample_size = BUFFER_SIZE;
    settings->lowest_freq = period2freq(BUFFER_SIZE / 4);
    settings->yin_threshold = DEFAULT_YIN_THRESHOLD;
}

/*---------------------------------------------------------------------*/

enum settings_file_status tuner_settings_load(struct tuner_settings* settings,
                                              char* filename)
{
    int fd = rb->open(filename, O_RDONLY);
    if(fd >= 0){ /* does file exist? */
        /* basic consistency check */
        if(rb->filesize(fd) == sizeof(*settings)){
            rb->read(fd, settings, sizeof(*settings));
            rb->close(fd);
            rb->memcpy(&hdd_tuner_settings, settings, sizeof(*settings));
            return(LOADED);
        }
    }
    /* Initializes the settings with default values at least */
    tuner_settings_reset(settings);
    return(ERRLOAD);
}

/*---------------------------------------------------------------------*/

enum settings_file_status tuner_settings_save(struct tuner_settings* settings,
                                              char* filename)
{
    int fd = rb->creat(filename);
    if(fd >= 0){ /* does file exist? */
        rb->write (fd, settings, sizeof(*settings));
        rb->close(fd);
        return(SAVED);
    }
    return(ERRSAVE);
}

/*---------------------------------------------------------------------*/

void load_settings(void)
{
    tuner_settings_load(&tuner_settings, SETTINGS_FILENAME);

    rb->storage_sleep();
}

/*---------------------------------------------------------------------*/

void save_settings(void)
{
    if(!settings_needs_saving(&tuner_settings))
        return;

    tuner_settings_save(&tuner_settings, SETTINGS_FILENAME);
}

/*=================================================================*/
/* Binary Log                                                      */
/*=================================================================*/

/* Fixed-point log base 2*/
/* Adapted from python code at 
   http://en.wikipedia.org/wiki/Binary_logarithm#Algorithm
*/
fixed log(fixed inp)
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

/* The function name is pretty self-explaining ;) */
void print_int_xy(int x, int y, int v)
{
    char temp[20];
    
    rb->lcd_set_foreground(front_color);
    rb->snprintf(temp,20,"%d",v);
    rb->lcd_putsxy(x,y,temp);
    rb->lcd_update();
}

/* Print out the frequency etc - will be removed later on */
void print_str(char* s)
{
    rb->lcd_set_foreground(front_color);
    rb->lcd_putsxy(0, freq_y, s);
    rb->lcd_update();
}

/* What can I say? Read the function name... */
void print_char_xy(int x, int y, char c)
{
    char temp[2];
    
    temp[0]=c;
    temp[1]=0;
    rb->lcd_set_foreground(front_color);
    
    rb->lcd_putsxy(x, y, temp);
}

/* Draw the red bar and the white lines */
void draw_bar(fixed wrong_by_cents)
{ 
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(255,0,0));   /* Color screens */
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DARKGRAY);           /* Greyscale screens */
#else
    rb->lcd_set_foreground(LCD_BLACK);      /* Black and white screens */
#endif

    if (fp_gt(wrong_by_cents, FP_ZERO))
    {
        rb->lcd_fillrect(bar_x_0, error_bar_y, 
            fixed2int(fp_mul(wrong_by_cents, LCD_FACTOR)), bar_h);
    }
    else
    {
        rb->lcd_fillrect(bar_x_0 + fixed2int(fp_mul(wrong_by_cents,LCD_FACTOR)),
                         error_bar_y,
                         fixed2int(fp_mul(wrong_by_cents, LCD_FACTOR)) * -1, 
                         bar_h);
    }
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));   /* Color screens */
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_BLACK);      /* Greyscale screens */
#else
    rb->lcd_set_foreground(LCD_BLACK);      /* Black and white screens */
#endif

    rb->lcd_hline(0,LCD_WIDTH-1, error_hline_y);
    rb->lcd_hline(0,LCD_WIDTH-1, error_hline_y2);
    rb->lcd_vline(LCD_WIDTH / 2, error_hline_y, error_hline_y2);
    
    print_int_xy(bar_x_minus_50    , error_ticks_y, -50);
    print_int_xy(bar_x_minus_20    , error_ticks_y, -20);
    print_int_xy(bar_x_0           , error_ticks_y,   0);
    print_int_xy(bar_x_20          , error_ticks_y,  20);
    print_int_xy(bar_x_50          , error_ticks_y,  50);
}

/* Print the letters A-G and the # on the screen */
void draw_letters(void)
{
    int i;

    rb->lcd_set_foreground(front_color);
    
    for (i=0; i<8; i++)
    {
        print_char_xy(i*(LCD_WIDTH / 8 ) + font_w, letter_y, gui_letters[i]);
    }
}

/* Draw the yellow point(s) below the letters and the '#' */
void draw_points(const char *s)
{
    int i;
    
    i = s[0]-'A';
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,0));     /* Color screens */
#elif LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DARKGRAY);           /* Grey screens */
#else
    rb->lcd_set_foreground(LCD_BLACK);      /* Black and White screens */
#endif
    
    rb->lcd_fillrect(i*(LCD_WIDTH / 8 ) + font_w, note_bar_y, font_w, font_h);
    
    if (s[1] == '#')
        rb->lcd_fillrect(7*(LCD_WIDTH / 8 ) + font_w, note_bar_y, font_w, font_h);
}

/* Calculate how wrong the note is and draw the GUI */
void display_frequency (fixed freq)                    
{                                                     
    fixed ldf, mldf;
    fixed lfreq, nfreq;
    int i, note = 0;
    bool draw_freq;
    char str_buf[30];

    if (fp_lt(freq, FP_LOW))
        freq = FP_LOW;
    lfreq = log(freq);

    /* Get the frequency to within the range of our reference table */
    while (fp_lt(lfreq, fp_sub(lfreqs[0], fp_shr(LOG_D_NOTE, 1))))
        lfreq = fp_add(lfreq, LOG_2);
    while (fp_gte(lfreq, fp_sub(fp_add(lfreqs[0], LOG_2), 
           fp_shr(LOG_D_NOTE, 1))))
        lfreq = fp_sub(lfreq, LOG_2);
    mldf = LOG_D_NOTE;
    for (i=0; i<12; i++) 
    {
        ldf = fp_gt(fp_sub(lfreq,lfreqs[i]), FP_ZERO) ? 
                 fp_sub(lfreq,lfreqs[i]) : fp_neg(fp_sub(lfreq,lfreqs[i]));
        if (fp_lt(ldf, mldf)) 
        {
              mldf = ldf;
              note = i;
        }
    }
    nfreq = freqs[note];
    while (fp_gt(fp_div(nfreq, freq), D_NOTE_SQRT)) 
        nfreq = fp_shr(nfreq, 1);
    while (fp_gt(fp_div(freq, nfreq), D_NOTE_SQRT))
    {
        nfreq = fp_shl(nfreq, 1);
    }
    
    ldf=fp_mul(int2fixed(1200), log(fp_div(freq,nfreq)));

    draw_freq = (fp_round(freq) != 0);
    if (draw_freq)
    { 
        rb->snprintf(str_buf,30, "%s : %d cents (%d.%dHz)", 
                     notes[note], fp_round(ldf) ,fixed2int(freq),
                     fp_round(fp_mul(fp_frac(freq), 
                                     int2fixed(DISPLAY_HZ_PRECISION))));
    }
    else
    {
        ldf = FP_ZERO;      /* prevents red bar at -32 cents when freq= 0*/
    }

    rb->lcd_clear_display();
    draw_bar(ldf);                /* The red bar */
    draw_letters();               /* The A-G letters and the # */
    if (draw_freq)
    {
        draw_points(notes[note]);     /* The yellow point(s) */
        print_str(str_buf);
    }
    rb->lcd_update();
}

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
unsigned vec_min_elem(fixed *s, unsigned buflen) 
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


fixed aubio_quadfrac(fixed s0, fixed s1, fixed s2, fixed pf) 
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

fixed vec_quadint_min(fixed *x, unsigned bufsize, unsigned pos, unsigned span)
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

fixed pitchyin(audio_sample_type *input, fixed *yin)
{
    fixed retval;
    unsigned j,tau = 0;
    int period;
    unsigned yin_size = tuner_settings.sample_size / 4;

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
                            yin_threshold_table[tuner_settings.yin_threshold])
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

uint32_t buffer_magnitude(audio_sample_type *input)
{
    unsigned n;
    uint64_t tally = 0;

    /* Operate on only one channel of the stereo signal */
    for(n = 0; n < tuner_settings.sample_size; n+=2)
    {
        tally += (uint64_t)input[n] * (uint64_t)input[n];
    }

    tally /= tuner_settings.sample_size / 2;

    /* now tally holds the average of the squares of all the samples */
    /* It must be between 0 and 0x7fff^2, so it fits in 32 bits      */
    return (uint32_t)tally;
}

/* Stop the recording when the buffer is full */
int recording_callback(int status)
{
    (void) status;
    
    rb->pcm_stop_recording();
    recording=0;
    return -1;  
}

/* The main program loop */
void record_and_get_pitch(void)
{
    int quit=0, button;
    bool redraw = true;
    /* For tracking the latency */
    /*
    long timer;
    char debug_string[20];
    */
    fixed period;
    bool waiting = false;

    while(!quit) 
    {
                                        /* Start recording */
        rb->pcm_record_data(recording_callback, (void *) audio_data, 
                            (size_t) tuner_settings.sample_size * 
                                     sizeof(audio_sample_type));
        recording=1;    
        
        while (recording && !quit)      /* wait for the buffer to be filled */
        {        
            rb->yield();
            button=pluginlib_getaction(0, plugin_contexts, PLA_ARRAY_COUNT);
            switch(button) 
            {
                case PLA_QUIT:
                    quit=true;
                    rb->yield();
                    break;
                
                case PLA_MENU:
                    if(main_menu())
                        quit=true;
                    rb->yield();
                    redraw = true;
                    break;
                
                default:
                    rb->yield();
                
                break;
            }
        } 
        
        /* Let's keep track of how long this takes */
        /* timer = *(rb->current_tick); */

        if(!quit)
        {
            /* Only do the heavy lifting if the volume is high enough */
            if(buffer_magnitude(audio_data) > 
                sqr(tuner_settings.volume_threshold * 
                    rb->sound_max(SOUND_MIC_GAIN)))
            {
                if(waiting)
                {
    #ifdef HAVE_ADJUSTABLE_CPU_FREQ
                    rb->cpu_boost(true);
    #endif
                    waiting = false;
                }
                
                rb->backlight_on();
                redraw = false;

                /* This returns the period of the detected pitch in samples */
                period = pitchyin(audio_data, yin_buffer);
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
    #ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost(false);
    #endif
                /*rb->backlight_off();*/
                display_frequency(FP_ZERO);
            }

            /*rb->snprintf(debug_string, 20, "%x,%x", 
                           audio_data[50], audio_data[52]);
            rb->lcd_putsxy(0, 40, debug_string);
            rb->lcd_update();
            */

            /* Print out how long it took to find the pitch */
            /*
            rb->snprintf(debug_string, 20, "latency: %ld", 
                         *(rb->current_tick) - timer);
            rb->lcd_putsxy(0, 40, debug_string);
            rb->lcd_update();
            */
        }
    }
    rb->pcm_close_recording();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

/* Init recording, tuning, and GUI */
void init_everything(void)
{    
    load_settings();

    /* --------- Init the audio recording ----------------- */
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);  
    rb->audio_set_input_source(INPUT_TYPE, SRCF_RECORDING); 

    /* set to maximum gain */
    rb->audio_set_recording_gain(tuner_settings.record_gain,
                                 tuner_settings.record_gain, 
                                 AUDIO_GAIN_MIC);

    rb->pcm_set_frequency(SAMPLE_RATE);
    rb->pcm_apply_settings();
    
    rb->pcm_init_recording();
    
    /* GUI */
    back_color  = rb->lcd_get_background();
    front_color = rb->lcd_get_foreground();
    rb->lcd_getstringsize("X", &font_w, &font_h);
    
    bar_x_minus_50 = 0;
    bar_x_minus_20 = (LCD_WIDTH / 2) - 
                     fixed2int(fp_mul(LCD_FACTOR, int2fixed(20))) - font_w;
    bar_x_0  = LCD_WIDTH / 2; 
    bar_x_20 = (LCD_WIDTH / 2) + 
               fixed2int(fp_mul(LCD_FACTOR, int2fixed(20))) - font_w; 
    bar_x_50 = LCD_WIDTH - 2 * font_w;
    
    letter_y = 10;
    note_bar_y = letter_y + font_h;
    bar_h = font_h;
    freq_y = note_bar_y + bar_h + 3;
    error_ticks_y = freq_y + font_h + 8;
    error_hline_y = error_ticks_y + font_h + 2;
    error_bar_margin = 2;
    error_bar_y = error_hline_y + error_bar_margin;
    error_hline_y2 = error_bar_y + bar_h + error_bar_margin;
}


enum plugin_status plugin_start(const void* parameter) NO_PROF_ATTR
{
    (void)parameter;
  
    init_everything();
    record_and_get_pitch();
    save_settings();

    return 0;
}
