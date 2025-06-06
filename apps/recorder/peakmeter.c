/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Philipp Pertermann
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
#include "config.h"
#include "thread.h"
#include "kernel.h"
#include "settings.h"
#include "storage.h"
#include "lcd.h"
#include "scrollbar.h"
#include "string.h"
#include "button.h"
#include "system.h"
#include "font.h"
#include "icons.h"
#include "lang.h"
#include "peakmeter.h"
#include "audio.h"
#include "screen_access.h"
#ifdef HAVE_BACKLIGHT
#include "backlight.h"
#endif
#include "action.h"

#include "pcm.h"
#include "pcm_mixer.h"

#ifdef HAVE_RECORDING
#include "pcm_record.h"
#endif

static bool pm_playback = true; /* selects between playback and recording peaks */

static struct meter_scales scales[NB_SCREENS];

/* Current values and cumulation */
static int pm_cur_left;        /* current values (last peak_meter_peek) */
static int pm_cur_right;
static int pm_max_left;        /* maximum values between peak meter draws */
static int pm_max_right;
#if defined(HAVE_AGC) || defined(HAVE_HISTOGRAM)
static int pm_peakhold_left;   /* max. peak values between peakhold calls */
static int pm_peakhold_right;  /* used for AGC and histogram display */
#endif
#ifdef HAVE_HISTOGRAM
static long next_histogram_update;
#endif

/* Clip hold */
static bool pm_clip_left = false;  /* when true a clip has occurred */
static bool pm_clip_right = false;
static long pm_clip_timeout_l;     /* clip hold timeouts */
static long pm_clip_timeout_r;

/* Temporarily en- / disables peak meter. This is especially for external
   applications to detect if the peak_meter is in use and needs drawing at all */
static bool peak_meter_enabled = true;
void peak_meter_enable(bool enable)
{
    peak_meter_enabled = enable;
}

/** Parameters **/
/* Range */
static unsigned short peak_meter_range_min;  /* minimum of range in samples */
static unsigned short peak_meter_range_max;  /* maximum of range in samples */
static unsigned short pm_range;       /* range width in samples */
static bool pm_use_dbfs = true;       /* true if peakmeter displays dBfs */
static bool level_check;              /* true if peeked at peakmeter before drawing */
static unsigned short pm_db_min = 0;      /* minimum of range in 1/100 dB */
static unsigned short pm_db_max = 9000;   /* maximum of range in 1/100 dB */
static unsigned short pm_db_range = 9000; /* range width in 1/100 dB */
/* Timing behaviour */
static int pm_peak_hold     = HZ / 5;  /* peak hold timeout ticks */
static int pm_peak_release  = 8;       /* peak release in units per read */
static int pm_clip_hold     = HZ * 60; /* clip hold timeout ticks */
static bool pm_clip_eternal = false;   /* true if clip timeout is disabled */

#ifdef HAVE_RECORDING
static unsigned short trig_strt_threshold;
static long trig_strt_duration;
static long trig_strt_dropout;
static unsigned short trig_stp_threshold;
static long trig_stp_hold;
static long trig_rstrt_gap;

/* point in time when the threshold was exceeded */
static long trig_hightime;

/* point in time when the volume fell below the threshold*/
static long trig_lowtime;

/* The output value of the trigger. See TRIG_XXX constants for valid values */
static int trig_status = TRIG_OFF;

static void (*trigger_listener)(int) = NULL;

/* clipping counter (only used for recording) */
static unsigned int pm_clipcount = 0;       /* clipping count */
static bool pm_clipcount_active = false;    /* counting or not */
#endif

/* debug only */
#ifdef PM_DEBUG
static int peek_calls = 0;

#define PEEKS_PER_DRAW_SIZE 40
static unsigned int peeks_per_redraw[PEEKS_PER_DRAW_SIZE];

#define TICKS_PER_DRAW_SIZE 20
static unsigned int ticks_per_redraw[TICKS_PER_DRAW_SIZE];
#endif

#if defined(HAVE_HISTOGRAM)
#define HIST_BUF_SIZE (LCD_WIDTH / 2)
static int hist_l = 0;
static int hist_r = 0;
static unsigned char history_l[HIST_BUF_SIZE];
static unsigned char history_r[HIST_BUF_SIZE];
#ifdef HAVE_LCD_COLOR
static const char hist_level_marks[6] = { 29, 26, 23, 17, 9, 2};
#endif
static int history_pos = 0;
#define HIST_W (LCD_WIDTH / 2)
#if LCD_DEPTH > 1
#ifdef HAVE_LCD_COLOR
#define LCD_BAL_L LCD_RGBPACK(0, 0, 255)
#define LCD_BAL_R LCD_RGBPACK(204, 0, 0)
#define LCD_HIST_OVER LCD_RGBPACK(204, 0, 0)
#define LCD_HIST_HI LCD_RGBPACK(255, 204, 0)
#define LCD_HIST_OK LCD_RGBPACK(51, 153, 0)
#else /* HAVE_LCD_COLOR */
#define LCD_BATT_OK LCD_BLACK
#define LCD_BATT_LO LCD_DARKGRAY
#define LCD_DISK_OK LCD_BLACK
#define LCD_DISK_LO LCD_DARKGRAY
#define LCD_HIST_OVER LCD_BLACK
#define LCD_HIST_OK LCD_DARKGRAY
#define LCD_BAL LCD_DARKGRAY
#endif /* HAVE_LCD_COLOR */
#else /* LCD_DEPTH > 1 */
#define LCD_HIST_OVER LCD_DEFAULT_FG
#define LCD_HIST_OK LCD_DEFAULT_FG
#define LCD_BAL LCD_DEFAULT_FG
#endif /* LCD_DEPTH > 1 */
#endif /* HAVE_HISTOGRAM */

static void peak_meter_draw(struct screen *display, struct meter_scales *meter_scales,
                            int x, int y, int width, int height);

/* precalculated peak values that represent magical
   dBfs values. Used to draw the scale */
static const short db_scale_src_values[DB_SCALE_SRC_VALUES_SIZE] = {
    32736, /*   0 db */
    22752, /* - 3 db */
    16640, /* - 6 db */
    11648, /* - 9 db */
    8320,  /* -12 db */
    4364,  /* -18 db */
    2064,  /* -24 db */
    1194,  /* -30 db */
    363,   /* -40 db */
    101,   /* -50 db */
    34,    /* -60 db */
    0,     /* -inf   */
};

static int db_scale_count = DB_SCALE_SRC_VALUES_SIZE;

/**
 * Calculates dB Value for the peak meter, uses peak value as input
 * @param int sample - The input value
 *                    Make sure that 0 <= value < SAMPLE_RANGE
 *
 * @return int - The 2 digit fixed point result of the euation
 *               20 * log (sample / SAMPLE_RANGE) + 90
 *               Output range is 0-9000 (that is 0.0 - 90.0 dB).
 *               Normally 0dB is full scale, here it is shifted +90dB.
 *               The calculation is based on the results of a linear
 *               approximation tool written specifically for this problem
 *               by Andreas Zwirtes (radhard@gmx.de). The result has an
 *               accurracy of better than 2%.
 *
 *               The search tree does a binary search on the input value.
 *
 *               Marked "NO_INLINE" because GCC really wants to inline it
 *               in some cases.
 *.
 *               Improved by Jvo Studer for errors < 0.2dB for critical
 *               range of -12dB to 0dB (78.0 to 90.0dB).
 */

static NO_INLINE int calc_db (int isample)
{
    /* Search tree:
     *
     *            ________2308_______
     *       ___115___             _12932_
     *    __24__   __534__     _6394_   _22450_
     *   _5_   d0 d1     d2   d3    d4 d5     d6
     *  d7 d8
     */
    static const int16_t keys[] =
    {
            0, /* 1-based array */
         2308,
          115,
        12932,
           24,
          534,
         6394,
        22450,
            5,
    };

    /* To ensure no overflow in the computations throughout the operational
       range, M_FRAC_BITS must be 20 or less. The full range of isample for
       20 fractional bits without overflowing int32_t is [-4, 90903]. */
    #define M_FRAC_BITS 20
    #define M_FIXED(m)  ((int32_t)(((int64_t)(m) << M_FRAC_BITS) / 100))

    static const struct {
        int16_t istart;
        int16_t n;
        int32_t m;
    } data[] =
    {
        { .istart =    24, .n = 2858, .m = M_FIXED( 1498) },
        { .istart =   114, .n = 4207, .m = M_FIXED(  319) },
        { .istart =   588, .n = 5583, .m = M_FIXED(   69) },
        { .istart =  2608, .n = 6832, .m = M_FIXED(   21) },
        { .istart =  7000, .n = 7682, .m = M_FIXED(    9) },
        { .istart = 13000, .n = 8219, .m = M_FIXED(    5) },
        { .istart = 22636, .n = 8697, .m = M_FIXED(    3) },
        { .istart =     1, .n =   98, .m = M_FIXED(34950) },
        { .istart =     5, .n = 1496, .m = M_FIXED( 7168) },
    };

    int i = 1;

    /* Runs 3 times if isample >= 24, else 4 */
    while (i < 9) {
        i = 2*i + (isample >= keys[i]);
    }

    i -= 9;

    return data[i].n + (data[i].m * (isample - data[i].istart) >> M_FRAC_BITS);
}

/**
 * A helper function for peak_meter_db2sample. Don't call it separately but
 * use peak_meter_db2sample. If one or both of min and max are outside the
 * range 0 <= min (or max) < 8961 the behaviour of this function is
 * undefined. It may not return.
 * @param int min - The minimum of the value range that is searched.
 * @param int max - The maximum of the value range that is searched.
 * @param int db  - The value in dBfs * (-100) for which the according
 *                  minimal peak sample is searched.
 * @return int - A linear volume value with 0 <= value < MAX_PEAK
 */
static int db_to_sample_bin_search(int min, int max, int db)
{
    int test = min + (max - min) / 2;

    if (min < max) {
        if (calc_db(test) < db) {
            test = db_to_sample_bin_search(test, max, db);
        } else {
            if (calc_db(test-1) > db) {
                test = db_to_sample_bin_search(min, test, db);
            }
        }
    }
    return test;
}

/**
 * Converts a value representing dBfs to a linear
 * scaled volume info as it is used by the MAS.
 * An incredibly inefficiant function which is
 * the vague inverse of calc_db. This really
 * should be replaced by something better soon.
 *
 * @param int db - A dBfs * 100 value with
 *                 -9000 < value <= 0
 * @return int - The return value is in the range of
 *               0 <= return value < MAX_PEAK
 */
int peak_meter_db2sample(int db)
{
    int retval = 0;

    /* what is the maximum pseudo db value */
    int max_peak_db = calc_db(MAX_PEAK - 1);

    /* range check: db value to big */
    if (max_peak_db + db < 0) {
        retval = 0;
    }

    /* range check: db value too small */
    else if (max_peak_db + db >= max_peak_db) {
        retval = MAX_PEAK -1;
    }

    /* value in range: find the matching linear value */
    else {
        retval = db_to_sample_bin_search(0, MAX_PEAK, max_peak_db + db);

        /* as this is a dirty function anyway, we want to adjust the
           full scale hit manually to avoid users complaining that when
           they adjust maximum for 0 dBfs and display it in percent it
           shows 99%. That is due to precision loss and this is the
           optical fix */
    }

    return retval;
}

/**
 * Set the min value for restriction of the value range.
 * @param int newmin - depending whether dBfs is used
 * newmin is a value in dBfs * 100 or in linear percent values.
 * for dBfs: -9000 < newmin <= 0
 * for linear: 0 <= newmin <= 100
 */
static void peak_meter_set_min(int newmin)
{
    if (pm_use_dbfs) {
        peak_meter_range_min = peak_meter_db2sample(newmin);

    } else {
        if (newmin < peak_meter_range_max) {
            peak_meter_range_min = newmin * MAX_PEAK / 100;
        }
    }

    pm_range = peak_meter_range_max - peak_meter_range_min;

    /* Avoid division by zero. */
    if (pm_range == 0) {
        pm_range = 1;
    }

    pm_db_min = calc_db(peak_meter_range_min);
    pm_db_range = pm_db_max - pm_db_min;

    FOR_NB_SCREENS(i)
        scales[i].db_scale_valid = false;
}

/**
 * Returns the minimum value of the range the meter
 * displays. If the scale is set to dBfs it returns
 * dBfs values * 100 or linear percent values.
 * @return: using dBfs : -9000 < value <= 0
 *          using linear scale: 0 <= value <= 100
 */
int peak_meter_get_min(void)
{
    int retval = 0;
    if (pm_use_dbfs) {
        retval = calc_db(peak_meter_range_min) - calc_db(MAX_PEAK - 1);
    } else {
        retval = peak_meter_range_min * 100 / MAX_PEAK;
    }
    return retval;
}

/**
 * Set the max value for restriction of the value range.
 * @param int newmax - depending wether dBfs is used
 * newmax is a value in dBfs * 100 or in linear percent values.
 * for dBfs: -9000 < newmax <= 0
 * for linear: 0 <= newmax <= 100
 */
static void peak_meter_set_max(int newmax)
{
    if (pm_use_dbfs) {
        peak_meter_range_max = peak_meter_db2sample(newmax);
    } else {
        if (newmax > peak_meter_range_min) {
            peak_meter_range_max = newmax * MAX_PEAK / 100;
        }
    }

    pm_range = peak_meter_range_max - peak_meter_range_min;

    /* Avoid division by zero. */
    if (pm_range == 0) {
        pm_range = 1;
    }

    pm_db_max = calc_db(peak_meter_range_max);
    pm_db_range = pm_db_max - pm_db_min;

    FOR_NB_SCREENS(i)
        scales[i].db_scale_valid = false;
}

/**
 * Returns the minimum value of the range the meter
 * displays. If the scale is set to dBfs it returns
 * dBfs values * 100 or linear percent values
 * @return: using dBfs : -9000 < value <= 0
 *          using linear scale: 0 <= value <= 100
 */
int peak_meter_get_max(void)
{
    int retval = 0;
    if (pm_use_dbfs) {
        retval = calc_db(peak_meter_range_max) - calc_db(MAX_PEAK - 1);
    } else {
        retval = peak_meter_range_max * 100 / MAX_PEAK;
    }
    return retval;
}

/**
 * Returns whether the meter is currently displaying dBfs or percent values.
 * @return bool - true if the meter is displaying dBfs
                  false if the meter is displaying percent values.
 */
bool peak_meter_get_use_dbfs(void)
{
    return pm_use_dbfs;
}

/**
 * Specifies whether the values displayed are scaled
 * as dBfs or as linear percent values.
 * @param use - set to true for dBfs,
 *              set to false for linear scaling in percent
 */
void peak_meter_set_use_dbfs(bool use)
{
    pm_use_dbfs = use;
    FOR_NB_SCREENS(i)
        scales[i].db_scale_valid = false;
}

/**
 * Initialize the range of the meter. Only values
 * that are in the range of [range_min ... range_max]
 * are displayed.
 * @param bool dbfs - set to true for dBfs,
 *                    set to false for linear scaling in percent
 * @param int range_min - Specifies the lower value of the range.
 *                        Pass a value dBfs * 100 when dbfs is set to true.
 *                        Pass a percent value when dbfs is set to false.
 * @param int range_max - Specifies the upper value of the range.
 *                        Pass a value dBfs * 100 when dbfs is set to true.
 *                        Pass a percent value when dbfs is set to false.
 */
void peak_meter_init_range( bool dbfs, int range_min, int range_max)
{
    pm_use_dbfs = dbfs;
    peak_meter_set_min(range_min);
    peak_meter_set_max(range_max);
}

/**
 * Initialize the peak meter with all relevant values concerning times.
 * @param int release - Set the maximum amount of pixels the meter is allowed
 *                      to decrease with each redraw
 * @param int hold_ms - Select the time in ms for the time the peak indicator
 *                      is reset after a peak occurred.
 * @param int clip_hold_sec - Select the time in seconds for the time the peak
 *                            indicator is reset after a peak occurred.
 */
void peak_meter_init_times(int release, int hold_ms, int clip_hold_sec)
{
    pm_peak_hold = hold_ms/(1000UL/HZ); /* convert ms to ticks */
    pm_peak_release = release;
    pm_clip_hold = HZ * clip_hold_sec;
}

#ifdef HAVE_RECORDING
/**
 * Enable/disable clip counting
 */
void pm_activate_clipcount(bool active)
{
    pm_clipcount_active = active;
}

/**
 * Get clipping counter value
 */
int pm_get_clipcount(void)
{
    return pm_clipcount;
}

/**
 * Set clipping counter to zero (typically at start of recording or playback)
 */
void pm_reset_clipcount(void)
{
    pm_clipcount = 0;
}
#endif

/**
 * Set the source of the peak meter to playback or to
 * record.
 * @param: bool playback - If true playback peak meter is used.
 *         If false recording peak meter is used.
 */
void peak_meter_playback(bool playback)
{
    pm_playback = playback;
    /* reset the scales just in case recording and playback
       use different viewport sizes. Normally we should be checking viewport
       sizes every time but this will do for now */
    FOR_NB_SCREENS(i)
            scales[i].db_scale_valid = false;
}

#ifdef HAVE_RECORDING
static void set_trig_status(int new_state)
{
    if (trig_status != new_state) {
        trig_status = new_state;
        if (trigger_listener != NULL) {
            trigger_listener(trig_status);
        }
    }
}

#endif

/**
 * Reads peak values from the MAS, and detects clips. The
 * values are stored in pm_max_left pm_max_right for later
 * evauluation. Consecutive calls to peak_meter_peek detect
 * that ocurred. This function could be used by a thread for
 * busy reading the MAS.
 */
void peak_meter_peek(void)
{
    int left, right;
#ifdef HAVE_RECORDING
    bool was_clipping = pm_clip_left || pm_clip_right;
#endif
   /* read current values */
    if (pm_playback)
    {
        static struct pcm_peaks chan_peaks; /* *MUST* be static */
        mixer_channel_calculate_peaks(PCM_MIXER_CHAN_PLAYBACK,
                                      &chan_peaks);
        pm_cur_left = chan_peaks.left;
        pm_cur_right = chan_peaks.right;
    }
#ifdef HAVE_RECORDING
    else
        pcm_calculate_rec_peaks(&pm_cur_left, &pm_cur_right);
#endif
    left  = pm_cur_left;
    right = pm_cur_right;

    /* check for clips
       An clip is assumed when two consecutive readouts
       of the volume are at full scale. This is proven
       to be inaccurate in both ways: it may detect clips
       when no clip occurred and it may fail to detect
       a real clip. For software codecs, the peak is already
       the max of a bunch of samples, so use one max value
       or you fail to detect clipping! */
    if (left == MAX_PEAK - 1) {
        pm_clip_left = true;
        pm_clip_timeout_l = current_tick + pm_clip_hold;
    }

    if (right == MAX_PEAK - 1) {
        pm_clip_right = true;
        pm_clip_timeout_r = current_tick + pm_clip_hold;
    }

#ifdef HAVE_RECORDING
    if(!was_clipping && (pm_clip_left || pm_clip_right))
    {
        if(pm_clipcount_active)
            pm_clipcount++;
    }
#endif

    /* peaks are searched -> we have to find the maximum. When
       many calls of peak_meter_peek the maximum value will be
       stored in pm_max_xxx. This maximum is reset by the
       functions peak_meter_read_x. */
    pm_max_left = MAX(pm_max_left, left);
    pm_max_right = MAX(pm_max_right, right);

#ifdef HAVE_RECORDING
    /* Ignore any unread peakmeter data */
#define MAX_DROP_TIME HZ/7 /* this value may need tweaking. Increase if you are
                              getting trig events when you shouldn't with
                              trig_stp_hold = 0 */
    if (!trig_stp_hold)
        trig_stp_hold = MAX_DROP_TIME;

    switch (trig_status) {
        case TRIG_READY:
            /* no more changes, if trigger was activated as release trigger */
            /* threshold exceeded? */
            if ((left > trig_strt_threshold)
                || (right > trig_strt_threshold)) {
                    /* reset trigger duration */
                    trig_hightime = current_tick;

                    /* reset dropout duration */
                    trig_lowtime = current_tick;

                if (trig_strt_duration)
                    set_trig_status(TRIG_STEADY);
                else
                    /* if trig_duration is set to 0 the user wants to start
                     recording immediately */
                    set_trig_status(TRIG_GO);
            }
            break;

        case TRIG_STEADY:
        case TRIG_RETRIG:
            /* trigger duration exceeded */
            if (current_tick - trig_hightime > trig_strt_duration) {
                set_trig_status(TRIG_GO);
            } else {
                /* threshold exceeded? */
                if ((left > trig_strt_threshold)
                    || (right > trig_strt_threshold)) {
                    /* reset lowtime */
                    trig_lowtime = current_tick;
                }
                /* volume is below threshold */
                else {
                    /* dropout occurred? */
                    if (current_tick - trig_lowtime > trig_strt_dropout){
                        if (trig_status == TRIG_STEADY){
                            set_trig_status(TRIG_READY);
                        }
                        /* trig_status == TRIG_RETRIG */
                        else {
                            /* the gap has already expired */
                            trig_lowtime = current_tick - trig_rstrt_gap - 1;
                            set_trig_status(TRIG_POSTREC);
                        }
                    }
                }
            }
            break;

        case TRIG_GO:
        case TRIG_CONTINUE:
            /* threshold exceeded? */
            if ((left > trig_stp_threshold)
                || (right > trig_stp_threshold)) {
                /* restart hold time countdown */
                trig_lowtime = current_tick;
            } else if (current_tick - trig_lowtime > MAX_DROP_TIME){
                set_trig_status(TRIG_POSTREC);
                trig_hightime = current_tick;
            }
            break;

        case TRIG_POSTREC:
            /* gap time expired? */
            if (current_tick - trig_lowtime > trig_rstrt_gap){
                /* start threshold exceeded? */
                if ((left > trig_strt_threshold)
                    || (right > trig_strt_threshold)) {

                    set_trig_status(TRIG_RETRIG);
                    trig_hightime = current_tick;
                    trig_lowtime = current_tick;
                }
                else

                /* stop threshold exceeded */
                if ((left > trig_stp_threshold)
                    || (right > trig_stp_threshold)) {
                    if (current_tick - trig_hightime > trig_stp_hold){
                        trig_lowtime = current_tick;
                        set_trig_status(TRIG_CONTINUE);
                    } else {
                        trig_lowtime = current_tick - trig_rstrt_gap - 1;
                    }
                }

                /* below any threshold */
                else {
                    if (current_tick - trig_lowtime > trig_stp_hold){
                        set_trig_status(TRIG_READY);
                    } else {
                        trig_hightime = current_tick;
                    }
                }
            }

            /* still within the gap time */
            else {
                /* stop threshold exceeded */
                if ((left > trig_stp_threshold)
                    || (right > trig_stp_threshold)) {
                    set_trig_status(TRIG_CONTINUE);
                    trig_lowtime = current_tick;
                }

                /* hold time expired */
                else if (current_tick - trig_lowtime > trig_stp_hold){
                    trig_hightime = current_tick;
                    trig_lowtime = current_tick;
                    set_trig_status(TRIG_READY);
                }
            }
            break;
    }
    /* restore stop hold value */
    if (trig_stp_hold == MAX_DROP_TIME)
        trig_stp_hold = 0;
#endif
    /* check levels next time peakmeter drawn */
    level_check = true;
#ifdef PM_DEBUG
    peek_calls++;
#endif
}

/**
 * Reads out the peak volume of the left channel.
 * @return int - The maximum value that has been detected
 * since the last call of peak_meter_read_l. The value
 * is in the range 0 <= value < MAX_PEAK.
 */
static int peak_meter_read_l(void)
{
    /* pm_max_left contains the maximum of all peak values that were read
       by peak_meter_peek since the last call of peak_meter_read_l */
    int retval;

    retval = pm_max_left;

#if defined(HAVE_HISTOGRAM) || defined(HAVE_AGC)
    /* store max peak value for peak_meter_get_peakhold_x readout */
    pm_peakhold_left = MAX(pm_max_left, pm_peakhold_left);
#endif
#ifdef PM_DEBUG
    peek_calls = 0;
#endif
    /* reset pm_max_left so that subsequent calls of peak_meter_peek don't
       get fooled by an old maximum value */
    pm_max_left = pm_cur_left;

    return retval;
}

/**
 * Reads out the peak volume of the right channel.
 * @return int - The maximum value that has been detected
 * since the last call of peak_meter_read_l. The value
 * is in the range 0 <= value < MAX_PEAK.
 */
static int peak_meter_read_r(void)
{
    /* peak_meter_r contains the maximum of all peak values that were read
       by peak_meter_peek since the last call of peak_meter_read_r */
    int retval;

    retval = pm_max_right;

#if defined(HAVE_HISTOGRAM) || defined(HAVE_AGC)
    /* store max peak value for peak_meter_get_peakhold_x readout */
    pm_peakhold_right = MAX(pm_max_right, pm_peakhold_right);
#endif
#ifdef PM_DEBUG
    peek_calls = 0;
#endif
    /* reset pm_max_right so that subsequent calls of peak_meter_peek don't
       get fooled by an old maximum value */
    pm_max_right = pm_cur_right;

    return retval;
}

#if defined(HAVE_AGC) || defined(HAVE_HISTOGRAM)
/**
 * Reads out the current peak-hold values since the last call.
 * This is used by the histogram feature in the recording screen.
 * Values are in the range 0 <= peak_x < MAX_PEAK. MAX_PEAK is typ 32767.
 */
void peak_meter_get_peakhold(int *peak_left, int *peak_right)
{
    if (peak_left)
        *peak_left  = pm_peakhold_left;
    if (peak_right)
        *peak_right = pm_peakhold_right;

#ifdef HAVE_HISTOGRAM
    if (*peak_left > hist_l)
        hist_l = *peak_left;
    if (*peak_right > hist_r)
        hist_r = *peak_right;
#endif

    pm_peakhold_left  = 0;
    pm_peakhold_right = 0;
}
#endif

/**
 * Reset the detected clips. This method is for
 * use by the user interface.
 * @param int unused - This parameter was added to
 * make the function compatible with set_int
 */
void peak_meter_set_clip_hold(int time)
{
    pm_clip_left = false;
    pm_clip_right = false;
    pm_clip_eternal = (time > 0) ? false : true;
}

/**
 * Scales a peak value as read from the MAS to the range of meterwidth.
 * The scaling is performed according to the scaling method (dBfs / linear)
 * and the range (peak_meter_range_min .. peak_meter_range_max).
 * @param unsigned short val - The volume value. Range: 0 <= val < MAX_PEAK
 * @param int meterwidht - The widht of the meter in pixel
 * @return unsigned short - A value 0 <= return value <= meterwidth
 */
unsigned short peak_meter_scale_value(unsigned short val, int meterwidth)
{
    int retval;

    if (val <= peak_meter_range_min) {
        return 0;
    }

    if (val >= peak_meter_range_max) {
        return meterwidth;
    }

    retval = val;

    /* different scaling is used for dBfs and linear percent */
    if (pm_use_dbfs) {

        /* scale the samples dBfs */
        retval  = (calc_db(retval) - pm_db_min) * meterwidth / pm_db_range;
    }

    /* Scale for linear percent display */
    else
    {
        /* scale the samples */
        retval  = ((retval - peak_meter_range_min) * meterwidth)
                  / pm_range;
    }
    return retval;
}
void peak_meter_screen(struct screen *display, int x, int y, int height)
{
    peak_meter_draw(display, &scales[display->screen_type], x, y,
                        display->getwidth() - x, height);
}

/* sets *left and *right to the current *unscaled* values */
void peak_meter_current_vals(int *left, int *right)
{
    static int left_level = 0, right_level = 0;
    if (level_check){
        /* only read the volume info from MAS if peek since last read*/
        left_level  = peak_meter_read_l();
        right_level = peak_meter_read_r();
        level_check = false;
    }
    *left = left_level;
    *right = right_level;
}

/**
 * Draws a peak meter in the specified size at the specified position.
 * @param int x - The x coordinate.
 *                Make sure that 0 <= x and x + width < display->getwidth()
 * @param int y - The y coordinate.
 *                Make sure that 0 <= y and y + height < display->getheight()
 * @param int width - The width of the peak meter. Note that for display
 *                    of clips a 3 pixel wide area is used ->
 *                    width > 3
 * @param int height - The height of the peak meter. height > 3
 */
static void peak_meter_draw(struct screen *display, struct meter_scales *scales,
                         int x, int y, int width, int height)
{
    int left_level = 0, right_level = 0;
    int left = 0, right = 0;
    int meterwidth = width - 3;
    int i, delta;
#if defined(HAVE_REMOTE_LCD) && !defined (ROCKBOX_HAS_LOGF)
    static long peak_release_tick[2] = {0,0};
    int screen_nr = display->screen_type == SCREEN_MAIN ? 0 : 1;
#else
    static long peak_release_tick = 0;
#endif

#ifdef PM_DEBUG
    static long pm_tick = 0;
    int tmp = peek_calls;
#endif

    /* if disabled only draw the peak meter */
    if (peak_meter_enabled) {


        peak_meter_current_vals(&left_level, &right_level);

        /* scale the samples dBfs */
        left  = peak_meter_scale_value(left_level, meterwidth);
        right = peak_meter_scale_value(right_level, meterwidth);

         /*if the scale has changed -> recalculate the scale
           (The scale becomes invalid when the range changed.) */
        if (!scales->db_scale_valid){

            if (pm_use_dbfs) {
                db_scale_count = DB_SCALE_SRC_VALUES_SIZE;
                for (i = 0; i < db_scale_count; i++){
                    /* find the real x-coords for predefined interesting
                       dBfs values. These only are recalculated when the
                       scaling of the meter changed. */
                        scales->db_scale_lcd_coord[i] =
                            peak_meter_scale_value(
                                db_scale_src_values[i],
                                meterwidth - 1);
                }
            }

            /* when scaling linear we simly make 10% steps */
            else {
                db_scale_count = 10;
                for (i = 0; i < db_scale_count; i++) {
                    scales->db_scale_lcd_coord[i] =
                        (i * (MAX_PEAK / 10) - peak_meter_range_min) *
                        meterwidth / pm_range;
                }
            }

            /* mark scale valid to avoid recalculating dBfs values
               of the scale. */
            scales->db_scale_valid = true;
        }

        /* apply release */
#if defined(HAVE_REMOTE_LCD) && !defined (ROCKBOX_HAS_LOGF)
        delta = current_tick - peak_release_tick[screen_nr];
        peak_release_tick[screen_nr] = current_tick;
#else
        delta = current_tick - peak_release_tick;
        peak_release_tick = current_tick;
#endif
        left  = MAX(left , scales->last_left  - delta * pm_peak_release);
        right = MAX(right, scales->last_right - delta * pm_peak_release);

        /* reset max values after timeout */
        if (TIME_AFTER(current_tick, scales->pm_peak_timeout_l)){
            scales->pm_peak_left = 0;
        }

        if (TIME_AFTER(current_tick, scales->pm_peak_timeout_r)){
            scales->pm_peak_right = 0;
        }

        if (!pm_clip_eternal) {
            if (pm_clip_left &&
                TIME_AFTER(current_tick, pm_clip_timeout_l)){
                pm_clip_left = false;
            }

            if (pm_clip_right &&
                TIME_AFTER(current_tick, pm_clip_timeout_r)){
                pm_clip_right = false;
            }
        }

        /* check for new max values */
        if (left > scales->pm_peak_left) {
            scales->pm_peak_left = left - 1;
            scales->pm_peak_timeout_l = current_tick + pm_peak_hold;
        }

        if (right > scales->pm_peak_right) {
            scales->pm_peak_right = right - 1;
            scales->pm_peak_timeout_r = current_tick + pm_peak_hold;
        }
    }

    /* draw the peak meter */
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(x, y, width, height);
    display->set_drawmode(DRMODE_SOLID);

    /* draw left */
    display->fillrect (x, y, left, height / 2 - 2 );
    if (scales->pm_peak_left > 0) {
        display->vline(x + scales->pm_peak_left, y, y + height / 2 - 2 );
    }
    if (pm_clip_left) {
        display->fillrect(x + meterwidth, y, 3, height / 2 - 1);
    }

    /* draw right */
    display->fillrect(x, y + height / 2 + 1, right, height / 2 - 2);
    if (scales->pm_peak_right > 0) {
        display->vline( x + scales->pm_peak_right, y + height / 2, y + height - 2);
    }
    if (pm_clip_right) {
        display->fillrect(x + meterwidth, y + height / 2, 3, height / 2 - 1);
    }

    /* draw scale end */
    display->vline(x + meterwidth, y, y + height - 2);

    /* draw dots for scale marks */
    for (i = 0; i < db_scale_count; i++) {
        /* The x-coordinates of interesting scale mark points
           have been calculated before */
        display->drawpixel(x + scales->db_scale_lcd_coord[i],
                           y + height / 2 - 1);
    }

#ifdef HAVE_RECORDING

#ifdef HAVE_BACKLIGHT
    /* cliplight */
    if ((pm_clip_left || pm_clip_right) &&
        global_settings.cliplight
        && !pm_playback
	    )
    {
        /* if clipping, cliplight setting on and in recording screen */
        if (global_settings.cliplight <= 2)
        {
            /* turn on main unit light if setting set to main or both*/
            backlight_on();
        }
#ifdef HAVE_REMOTE_LCD
        if (global_settings.cliplight >= 2)
        {
            /* turn remote light unit on if setting set to remote or both */
            remote_backlight_on();
        }
#endif /* HAVE_REMOTE_LCD */
    }
#endif /* HAVE_BACKLIGHT */

    if (trig_status != TRIG_OFF) {
        int start_trigx, stop_trigx, ycenter;

        display->set_drawmode(DRMODE_SOLID);
        ycenter = y + height / 2;
        /* display threshold value */
        start_trigx = x+peak_meter_scale_value(trig_strt_threshold,meterwidth);
        display->vline(start_trigx, ycenter - 2, ycenter);
        start_trigx ++;
        if (start_trigx < display->getwidth() ) display->drawpixel(start_trigx,
                                                                   ycenter - 1);

        stop_trigx = x + peak_meter_scale_value(trig_stp_threshold,meterwidth);
        display->vline(stop_trigx, ycenter - 2, ycenter);
        if (stop_trigx > 0) display->drawpixel(stop_trigx - 1, ycenter - 1);
    }
#endif /*HAVE_RECORDING*/

#ifdef PM_DEBUG
    /* display a bar to show how many calls to peak_meter_peek
       have ocurred since the last display */
    display->set_drawmode(DRMODE_COMPLEMENT);
    display->fillrect(x, y, tmp, 3);

    if (tmp < PEEKS_PER_DRAW_SIZE) {
        peeks_per_redraw[tmp]++;
    }

    tmp = current_tick - pm_tick;
    if (tmp < TICKS_PER_DRAW_SIZE ){
        ticks_per_redraw[tmp] ++;
    }

    /* display a bar to show how many ticks have passed since
       the last redraw */
    display->fillrect(x, y + height / 2, current_tick - pm_tick, 2);
    pm_tick = current_tick;
#endif

    scales->last_left = left;
    scales->last_right = right;

    display->set_drawmode(DRMODE_SOLID);
}

#ifdef HAVE_RECORDING
/**
 * Defines the parameters of the trigger. After these parameters are defined
 * the trigger can be started either by peak_meter_attack_trigger or by
 * peak_meter_release_trigger. Note that you can pass either linear (%) or
 * logarithmic (db) values to the thresholds. Positive values are intepreted as
 * percent (0 is 0% .. 100 is 100%). Negative values are interpreted as db.
 * To avoid ambiguosity of the value 0 the negative values are shifted by -1.
 * Thus -75 is -74db .. -1 is 0db.
 * @param start_threshold - The threshold used for attack trigger. Negative
 *                          values are interpreted as db -1, positive as %.
 * @param start_duration - The minimum time span within which start_threshold
 *                         must be exceeded to fire the attack trigger.
 * @param start_dropout - The maximum time span the level may fall below
 *                        start_threshold without releasing the attack trigger.
 * @param stop_threshold - The threshold the volume must fall below to release
 *                         the release trigger.Negative values are
 *                         interpreted as db -1, positive as %.
 * @param stop_hold - The minimum time the volume must fall below the
 *                    stop_threshold to release the trigger.
 * @param
 */
void peak_meter_define_trigger(
    int start_threshold,
    long start_duration,
    long start_dropout,
    int stop_threshold,
    long stop_hold_time,
    long restart_gap
    )
{
    if (start_threshold < 0) {
        /* db */
        if (start_threshold < -89) {
            trig_strt_threshold = 0;
        } else {
            trig_strt_threshold =peak_meter_db2sample((start_threshold+1)*100);
        }
    } else {
        /* linear percent */
        trig_strt_threshold = start_threshold * MAX_PEAK / 100;
    }
    trig_strt_duration = start_duration;
    trig_strt_dropout = start_dropout;
    if (stop_threshold < 0) {
        /* db */
        trig_stp_threshold = peak_meter_db2sample((stop_threshold + 1) * 100);
    } else {
        /* linear percent */
        trig_stp_threshold = stop_threshold * MAX_PEAK / 100;
    }
    trig_stp_hold = stop_hold_time;
    trig_rstrt_gap = restart_gap;
}

/**
 * Enables or disables the trigger.
 * @param on - If true the trigger is turned on.
 */
void peak_meter_trigger(bool on)
{
    /* don't use set_trigger here as that would fire an undesired event */
    trig_status = on ? TRIG_READY : TRIG_OFF;
}

/**
 * Registers the listener function that listenes on trig_status changes.
 * @param listener - The function that is called with each change of
 *                   trig_status. May be set to NULL if no callback is desired.
 */
void peak_meter_set_trigger_listener(void (*listener)(int status))
{
    trigger_listener = listener;
}

/**
 * Fetches the status of the trigger.
 * TRIG_OFF: the trigger is inactive
 * TRIG_RELEASED:  The volume level is below the threshold
 * TRIG_ACTIVATED: The volume level has exceeded the threshold, but the trigger
 *                 hasn't been fired yet.
 * TRIG_FIRED:     The volume exceeds the threshold
 *
 * To activate the trigger call either peak_meter_attack_trigger or
 * peak_meter_release_trigger. To turn the trigger off call
 * peak_meter_trigger_off.
 */
int peak_meter_trigger_status(void)
{
   return trig_status; /* & TRIG_PIT_MASK;*/
}

void peak_meter_draw_trig(int xpos[], int ypos[],
                          int trig_width[], int nb_screens)
{
    int barstart[NB_SCREENS];
    int barend[NB_SCREENS];
    int icon;
    int ixpos[NB_SCREENS];
    int trigbar_width[NB_SCREENS];

    FOR_NB_SCREENS(i)
        trigbar_width[i] = (trig_width[i] - (2 * (ICON_PLAY_STATE_WIDTH + 1)));

    switch (trig_status) {

        case TRIG_READY:
            FOR_NB_SCREENS(i){
                barstart[i] = 0;
                barend[i] = 0;
            }
            icon = Icon_Stop;
            FOR_NB_SCREENS(i)
                ixpos[i] = xpos[i];
            break;

        case TRIG_STEADY:
        case TRIG_RETRIG:
            FOR_NB_SCREENS(i)
            {
                barstart[i] = 0;
                barend[i] = (trig_strt_duration == 0) ? trigbar_width[i] :
                              trigbar_width[i] *
                             (current_tick - trig_hightime) / trig_strt_duration;
            }
            icon = Icon_Stop;
            FOR_NB_SCREENS(i)
                ixpos[i] = xpos[i];
            break;

        case TRIG_GO:
        case TRIG_CONTINUE:
            FOR_NB_SCREENS(i)
            {
                barstart[i] = trigbar_width[i];
                barend[i] = trigbar_width[i];
            }
            icon = Icon_Record;
            FOR_NB_SCREENS(i)
                ixpos[i] = xpos[i]+ trig_width[i] - ICON_PLAY_STATE_WIDTH;
            break;

        case TRIG_POSTREC:
            FOR_NB_SCREENS(i)
            {
                barstart[i] = (trig_stp_hold == 0) ? 0 :
                               trigbar_width[i] - trigbar_width[i] *
                              (current_tick - trig_lowtime) / trig_stp_hold;
                barend[i] = trigbar_width[i];
            }
            icon = Icon_Record;
            FOR_NB_SCREENS(i)
                ixpos[i] = xpos[i] + trig_width[i] - ICON_PLAY_STATE_WIDTH;
            break;

        default:
            return;
    }

    for(int i = 0; i < nb_screens; i++)
    {
        gui_scrollbar_draw(&screens[i], xpos[i] + ICON_PLAY_STATE_WIDTH + 1,
                               ypos[i] + 1, trigbar_width[i], TRIG_HEIGHT - 2,
                               trigbar_width[i], barstart[i], barend[i],
                               HORIZONTAL);

        screens[i].mono_bitmap(bitmap_icons_7x8[icon], ixpos[i], ypos[i],
                        ICON_PLAY_STATE_WIDTH, SB_ICON_HEIGHT);
    }
}
#endif

int peak_meter_draw_get_btn(int action_context, int x[], int y[],
                            int height[], int nb_screens,
                            struct viewport vps[])
{
    int button = BUTTON_NONE;
    long next_refresh = current_tick;
    long next_big_refresh = current_tick + HZ / 10;
    int i;
    bool highperf = false;
    bool dopeek = true;

    while (TIME_BEFORE(current_tick, next_big_refresh)) {
        button = get_action(action_context, TIMEOUT_NOBLOCK);
        if (button != BUTTON_NONE) {
            break;
        }
        if (dopeek) {          /* Peek only once per refresh when disk is */
            peak_meter_peek(); /* spinning, but as often as possible */
            dopeek = highperf; /* otherwise. */
            yield();
        } else {
            sleep(0);          /* Sleep until end of current tick. */
        }
        if (TIME_AFTER(current_tick, next_refresh)) {
            for(i = 0; i < nb_screens; i++)
            {
                screens[i].set_viewport(&vps[i]);
                peak_meter_screen(&screens[i], x[i], y[i], height[i]);
                screens[i].update_viewport_rect(x[i], y[i],
                                                screens[i].getwidth() - x[i],
                                                height[i]);
            }
            next_refresh += HZ / PEAK_METER_FPS;
            dopeek = true;
        }
    }

    return button;
}

#ifdef PM_DEBUG
static void peak_meter_clear_histogram(void)
{
    int i = 0;
    for (i = 0; i < TICKS_PER_DRAW_SIZE; i++) {
        ticks_per_redraw[i] = (unsigned int)0;
    }

    for (i = 0; i < PEEKS_PER_DRAW_SIZE; i++) {
        peeks_per_redraw[i] = (unsigned int)0;
    }
}

bool peak_meter_histogram(void)
{
    int i;
    int btn = BUTTON_NONE;
    while ((btn & BUTTON_OFF) != BUTTON_OFF )
    {
        unsigned int max = 0;
        int y = 0;
        int x = 0;
        screens[0].clear_display();

        for (i = 0; i < PEEKS_PER_DRAW_SIZE; i++) {
            max = MAX(max, peeks_per_redraw[i]);
        }

        for (i = 0; i < PEEKS_PER_DRAW_SIZE; i++) {
            x = peeks_per_redraw[i] * (LCD_WIDTH - 1)/ max;
            screens[0].hline(0, x, y + i);
        }

        y = PEEKS_PER_DRAW_SIZE + 1;
        max = 0;

        for (i = 0; i < TICKS_PER_DRAW_SIZE; i++) {
            max = MAX(max, ticks_per_redraw[i]);
        }

        for (i = 0; i < TICKS_PER_DRAW_SIZE; i++) {
            x = ticks_per_redraw[i] * (LCD_WIDTH - 1)/ max;
            screens[0].hline(0, x, y + i);
        }
        screens[0].update();

        btn = button_get(true);
        if (btn == BUTTON_PLAY) {
            peak_meter_clear_histogram();
        }
    }
    return false;
}
#endif

#ifdef HAVE_HISTOGRAM
void histogram_init()
{
    /* get update interval, clear buffer, reset drawing position */
    memset(history_l, 0, sizeof(unsigned char)*HIST_BUF_SIZE);
    memset(history_r, 0, sizeof(unsigned char)*HIST_BUF_SIZE);
    next_histogram_update = current_tick +
                                      (global_settings.histogram_interval * HZ);
}

void histogram_draw(int x1, int x2, int y1, int y2, int width, int height)
{
    int i, j;
    if (current_tick >= next_histogram_update)
    {
        /* fill history buffer */
        history_l[history_pos] = hist_l * height / 32767;
        history_r[history_pos] = hist_r * height / 32767;
        history_pos = (history_pos + 1) % HIST_BUF_SIZE;
        history_l[history_pos] = history_r[history_pos] = 0;
        history_l[(history_pos + 1) % HIST_BUF_SIZE] = 0;
        history_r[(history_pos + 1) % HIST_BUF_SIZE] = 0;
        hist_l = 0;
        hist_r = 0;
        next_histogram_update = current_tick +
                                      (global_settings.histogram_interval * HZ);
    }
    lcd_set_drawmode(DRMODE_SOLID);
    lcd_drawrect(x1, y1, width, height);
    lcd_drawrect(x2, y2, width, height);
    lcd_set_drawmode(DRMODE_FG);

    j = history_pos;
    for (i = width-2; i >= 0; i--)
    {
        j--;
        if(j<0)
            j = HIST_BUF_SIZE-1;
        if (history_l[j])
        {
            if (history_l[j] == height)
                lcd_set_foreground(LCD_HIST_OVER);
#ifdef HAVE_LCD_COLOR
            else if (history_l[j] > hist_level_marks[1])
                lcd_set_foreground(LCD_HIST_HI);
#endif
            else
                lcd_set_foreground(LCD_HIST_OK);
            lcd_vline(x1 + i, y1 + height - 2, y1 + height - history_l[j]);
        }
        if (history_r[j])
        {
            if (history_r[j] == height)
                lcd_set_foreground(LCD_HIST_OVER);
#ifdef HAVE_LCD_COLOR
            else if (history_r[j] > hist_level_marks[1])
                lcd_set_foreground(LCD_HIST_HI);
#endif
            else
                lcd_set_foreground(LCD_HIST_OK);
            lcd_vline(x2 + i, y2 + height - 2, y2 + height - history_r[j]);
        }
    }
    lcd_set_foreground(
#ifdef HAVE_LCD_COLOR
    global_settings.fg_color);
#else
    LCD_DEFAULT_FG);
#endif
}
#endif /* HAVE_HISTOGRAM */
