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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "mas.h"
#include "thread.h"
#include "kernel.h"
#include "settings.h"
#include "lcd.h"
#include "widgets.h"
#include "wps-display.h"
#include "sprintf.h"
#include "button.h"
#include "system.h"
#include "font.h"
#include "icons.h"
#include "lang.h"
#include "peakmeter.h"

/* no inline in simulator mode */
#ifdef SIMULATOR
#define inline
#endif

/* buffer the read peak value */
static int peak_meter_max_l;
static int peak_meter_max_r;

/* point in time when peak_meter_max_x becomes invalid */
static long peak_meter_timeout_l;
static long peak_meter_timeout_r;

/* when true a clip has occurred */
static bool peak_meter_l_clip = false;
static bool peak_meter_r_clip = false;

/* point in time when peak_meter_x_oveflow becomes invalid */
static long peak_meter_clip_timeout_l;
static long peak_meter_clip_timeout_r;

static int  peak_meter_clip_hold;

/* specifies the value range in peak volume values */
static unsigned short peak_meter_range_min;
static unsigned short peak_meter_range_max;
static unsigned short peak_meter_range;

/* if set to true clip timeout is disabled */
static bool peak_meter_clip_eternal = false;

static bool peak_meter_use_dbfs = true;
static unsigned short db_min = 0;
static unsigned short db_max = 9000;
static unsigned short db_range = 9000;

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
#endif

#if CONFIG_HWCODEC == MASNONE
#define MAS_REG_DQPEAK_L 0
#define MAS_REG_DQPEAK_R 0
#endif

#ifndef SIMULATOR
static int peak_meter_src_l = MAS_REG_DQPEAK_L;
static int peak_meter_src_r = MAS_REG_DQPEAK_R;
#endif

/* temporarily en- / disables peak meter. This is
   especially for external applications to detect 
   if the peak_meter is in use and needs drawing at all */
bool peak_meter_enabled = true;

/*
bool peak_meter_use_thread = false;
static char peak_meter_stack[DEFAULT_STACK_SIZE];
*/
/* used in wps.c to set the display frame rate of the peak meter */
int peak_meter_fps = 20;

static int peak_meter_l;
static int peak_meter_r;

static int peak_meter_hold = 1;
static int peak_meter_release = 8;

/* debug only */
#ifdef PM_DEBUG
static int peek_calls = 0;

#define PEEKS_PER_DRAW_SIZE 40
static unsigned int peeks_per_redraw[PEEKS_PER_DRAW_SIZE];

#define TICKS_PER_DRAW_SIZE 20
static unsigned int ticks_per_redraw[TICKS_PER_DRAW_SIZE];
#endif

/* time out values for max */
static const long max_time_out[] = {
    0 * HZ, HZ / 5, 30, HZ / 2, HZ, 2 * HZ, 
    3 * HZ, 4 * HZ, 5 * HZ, 6 * HZ, 7 * HZ, 8 * HZ,
    9 * HZ, 10 * HZ, 15 * HZ, 20 * HZ, 30 * HZ, 60 * HZ
};

/* time out values for clip */
static const long clip_time_out[] = {
    0 * HZ, 1  * HZ, 2 * HZ, 3 * HZ, 4 * HZ, 5 * HZ, 
    6 * HZ, 7 * HZ, 8 * HZ, 9 * HZ, 10 * HZ, 15 * HZ,
    20 * HZ, 25 * HZ, 30 * HZ, 45 * HZ, 60 * HZ, 90 * HZ,
    120 * HZ, 180 * HZ, 300 * HZ, 600L * HZ, 1200L * HZ,
    2700L * HZ, 5400L * HZ
};

/* precalculated peak values that represent magical
   dBfs values. Used to draw the scale */
#define DB_SCALE_SRC_VALUES_SIZE 12
#if 0
static const int db_scale_src_values[DB_SCALE_SRC_VALUES_SIZE] = {
    32767, /*   0 db */
    23197, /* - 3 db */
    16422, /* - 6 db */
    11626, /* - 9 db */
    8231,  /* -12 db */
    4125,  /* -18 db */
    2067,  /* -24 db */
    1036,  /* -30 db */
    328,   /* -40 db */
    104,   /* -50 db */
    33,    /* -60 db */
    1,     /* -inf   */
};
#else
static const int db_scale_src_values[DB_SCALE_SRC_VALUES_SIZE] = {
    32752, /*   0 db */
    22784, /* - 3 db */
    14256, /* - 6 db */
    11752, /* - 9 db */
    9256,  /* -12 db */
    4256,  /* -18 db */
    2186,  /* -24 db */
    1186,  /* -30 db */
    373,   /* -40 db */
    102,   /* -50 db */
    33,    /* -60 db */
    0,     /* -inf   */
};
#endif

static int db_scale_count = DB_SCALE_SRC_VALUES_SIZE;

/* if db_scale_valid is false the content of 
   db_scale_lcd_coord needs recalculation */
static bool db_scale_valid = false;

/* contains the lcd x coordinates of the magical
   scale values in db_scale_src_values */
static int db_scale_lcd_coord[sizeof db_scale_src_values / sizeof (int)];


/**
 * Calculates dB Value for the peak meter, uses peak value as input
 * @param int sample - The input value 
 *                    Make sure that 0 <= value < SAMPLE_RANGE
 *
 * @return int - The 2 digit fixed comma result of the euation
 *               20 * log (sample / SAMPLE_RANGE) + 90
 *               Output range is 0-8961 (that is 0,0 - 89,6 dB).
 *               Normally 0dB is full scale, here it is shifted +90dB.
 *               The calculation is based on the results of a linear
 *               approximation tool written specifically for this problem
 *               by Andreas Zwirtes (radhard@gmx.de). The result hat an
 *               accurracy of better than 2%. It is highly runtime optimized,
 *               the cascading if-clauses do an successive approximation on
 *               the input value. This avoids big lookup-tables and
 *               for-loops.
 */

int calc_db (int isample) {
    /* return n+m*(isample-istart)/100 */
    int n;
    long m;
    int istart;

    /* Range 1-4 */
    if (isample < 119) {

        /* Range 1-2 */
        if (isample < 5) {

            /* Range 1 */
            if (isample < 1) {
                istart = 0;
                n = 0;
                m = 5900;
            }

            /* Range 2 */
            else {
                istart = 1;
                n = 59;
                m = 34950;
            }
        }

        /* Range 3-4 */
        else {

            /* Range 3 */
            if (isample < 24) {
                istart = 5;
                n = 1457;
                m = 7168;
            }

            /* Range 4 */
            else {
                istart = 24;
                n = 2819;
                m = 1464;
            }
        }
    }

    /* Range 5-8 */
    else {

        /* Range 5-6 */
        if (isample < 2918) {

            /* Range 5 */
            if (isample < 592) {
                istart = 119;
                n = 4210;
                m = 295;
            }

            /* Range 6 */
            else {
                istart = 592;
                n = 5605;
                m = 60;
            }
        }

        /* Range 7-8 */
        else {

            /* Range 7 */
            if (isample < 15352) {
                istart = 2918;
                n = 7001;
                m = 12;
            }

            /* Range 8 */
            else {
                istart = 15352;
                n = 8439;
                m = 3;
            }
        }
    }
   
    return n + (m * (long)(isample - istart)) / 100L;
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
static int db_to_sample_bin_search(int min, int max, int db){
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
int peak_meter_db2sample(int db) {
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
 * @param int newmin - depending wether dBfs is used
 * newmin is a value in dBfs * 100 or in linear percent values.
 * for dBfs: -9000 < newmin <= 0
 * for linear: 0 <= newmin <= 100
 */
void peak_meter_set_min(int newmin) {
    if (peak_meter_use_dbfs) {
        peak_meter_range_min = peak_meter_db2sample(newmin);

    } else {
        if (newmin < peak_meter_range_max) {
            peak_meter_range_min = newmin * MAX_PEAK / 100;
        }
    }

    peak_meter_range = peak_meter_range_max - peak_meter_range_min;

    db_min = calc_db(peak_meter_range_min);
    db_range = db_max - db_min;
    db_scale_valid = false;
}

/**
 * Returns the minimum value of the range the meter 
 * displays. If the scale is set to dBfs it returns
 * dBfs values * 100 or linear percent values.
 * @return: using dBfs : -9000 < value <= 0
 *          using linear scale: 0 <= value <= 100
 */
int peak_meter_get_min(void) {
    int retval = 0;
    if (peak_meter_use_dbfs) {
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
void peak_meter_set_max(int newmax) {
    if (peak_meter_use_dbfs) {
        peak_meter_range_max = peak_meter_db2sample(newmax);
    } else {
        if (newmax > peak_meter_range_min) {
            peak_meter_range_max = newmax * MAX_PEAK / 100;
        }
    }

    peak_meter_range = peak_meter_range_max - peak_meter_range_min;

    db_max = calc_db(peak_meter_range_max);
    db_range = db_max - db_min;
    db_scale_valid = false;
}

/**
 * Returns the minimum value of the range the meter 
 * displays. If the scale is set to dBfs it returns
 * dBfs values * 100 or linear percent values
 * @return: using dBfs : -9000 < value <= 0
 *          using linear scale: 0 <= value <= 100
 */
int peak_meter_get_max(void) {
    int retval = 0;
    if (peak_meter_use_dbfs) {
        retval = calc_db(peak_meter_range_max) - calc_db(MAX_PEAK - 1);
    } else {
        retval = peak_meter_range_max * 100 / MAX_PEAK;
    }
    return retval;
}

/**
 * Returns 1 if the meter currently is
 * displaying dBfs values, 0 if the meter
 * displays percent values.
 * @return int - returns 0 or 1.
 */
int peak_meter_get_use_dbfs(void) {
    return peak_meter_use_dbfs ? 1 : 0;
}

/**
 * Specifies wether the values displayed are scaled
 * as dBfs or as linear percent values.
 * @param int - Set to 0 for linear percent scale. Any other value
 *              switches on dBfs.
 */
void peak_meter_set_use_dbfs(int use){
    peak_meter_use_dbfs = ((use & 1) == 1);
    db_scale_valid = false;
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
    peak_meter_use_dbfs = dbfs;
    peak_meter_set_min(range_min);
    peak_meter_set_max(range_max);
}

/**
 * Initialize the peak meter with all relevant values concerning times.
 * @param int release - Set the maximum amount of pixels the meter is allowed
 *                      to decrease with each redraw
 * @param int hold - Select the time preset for the time the peak indicator 
 *                   is reset after a peak occurred. The preset values are 
 *                   stored in max_time_out.
 * @param int clip_hold - Select the time preset for the time the peak 
 *                        indicator is reset after a peak occurred. The preset 
 *                        values are stored in clip_time_out.
 */
void peak_meter_init_times(int release, int hold, int clip_hold) {
    peak_meter_hold = hold;
    peak_meter_release = release;
    peak_meter_clip_hold = clip_hold;
}

/**
 * Set the source of the peak meter to playback or to 
 * record.
 * @param: bool playback - If true playback peak meter is used.
 *         If false recording peak meter is used.
 */
void peak_meter_playback(bool playback)
{
#ifdef SIMULATOR
    (void)playback;
#elif CONFIG_HWCODEC == MASNONE
/* FIX: not for the sw-based ones yes */
#else
    if (playback) {
        peak_meter_src_l = MAS_REG_DQPEAK_L;
        peak_meter_src_r = MAS_REG_DQPEAK_R;
    } else {
        peak_meter_src_l = MAS_REG_QPEAK_L;
        peak_meter_src_r = MAS_REG_QPEAK_R;
    }
#endif
}

#ifdef HAVE_RECORDING
static void set_trig_status(int new_state) {
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
 * values are stored in peak_meter_l peak_meter_r for later 
 * evauluation. Consecutive calls to peak_meter_peek detect 
 * that ocurred. This function could be used by a thread for
 * busy reading the MAS.
 */
inline void peak_meter_peek(void)
{
#ifdef SIMULATOR
    int left = 8000;
    int right = 9000;
#elif CONFIG_HWCODEC == MASNONE
    /* FIX */
    int left = 9000;
    int right = 8000;
#else
   /* read the peak values */
    int left  = mas_codec_readreg(peak_meter_src_l);
    int right = mas_codec_readreg(peak_meter_src_r);
#endif

    /* check for clips
       An clip is assumed when two consecutive readouts
       of the volume are at full scale. This is proven
       to be inaccurate in both ways: it may detect clips
       when no clip occurred and it may fail to detect
       a real clip. */
    if ((left  == peak_meter_l) && 
        (left  == MAX_PEAK - 1)) {
        peak_meter_l_clip = true;
        peak_meter_clip_timeout_l = 
            current_tick + clip_time_out[peak_meter_clip_hold];
    }

    if ((right == peak_meter_r) && 
        (right == MAX_PEAK - 1)) {
        peak_meter_r_clip = true;
        peak_meter_clip_timeout_r = 
            current_tick + clip_time_out[peak_meter_clip_hold];
    }

#ifdef HAVE_RECORDING
    switch (trig_status) {
        case TRIG_READY:
            /* no more changes, if trigger was activated as release trigger */
            /* threshold exceeded? */
            if ((left > trig_strt_threshold) || (right > trig_strt_threshold)) {
                if (trig_strt_duration) {
                    /* reset trigger duration */
                    trig_hightime = current_tick;

                    /* reset dropout duration */
                    trig_lowtime = current_tick;

                    /* if trig_duration is set to 0 the user wants to start
                     recording immediately */
                    set_trig_status(TRIG_STEADY);
                } else {
                    set_trig_status(TRIG_GO);
                }
            }
            break;

        case TRIG_STEADY:
        case TRIG_RETRIG:
            /* trigger duration exceeded */
            if (current_tick - trig_hightime > trig_strt_duration) {
                set_trig_status(TRIG_GO);
            } else {
                /* threshold exceeded? */
                if ((left > trig_strt_threshold) ||
                    (right > trig_strt_threshold)) {
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
            if ((left > trig_stp_threshold) || (right > trig_stp_threshold)) {
                /* restart hold time countdown */
                trig_lowtime = current_tick;
            } else {
                set_trig_status(TRIG_POSTREC);
                trig_hightime = current_tick;
            }
            break;

        case TRIG_POSTREC:
            /* gap time expired? */
            if (current_tick - trig_lowtime > trig_rstrt_gap){
                /* start threshold exceeded? */
                if ((left > trig_strt_threshold) ||
                    (right > trig_strt_threshold)) {

                    set_trig_status(TRIG_RETRIG);
                    trig_hightime = current_tick;
                }
                else

                /* stop threshold exceeded */
                if ((left > trig_stp_threshold) ||
                    (right > trig_stp_threshold)) {
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
                if ((left > trig_stp_threshold) ||
                    (right > trig_stp_threshold)) {
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
#endif

    /* peaks are searched -> we have to find the maximum. When
       many calls of peak_meter_peek the maximum value will be
       stored in peak_meter_x. This maximum is reset by the
       functions peak_meter_read_x. */
    peak_meter_l = MAX(peak_meter_l, left);
    peak_meter_r = MAX(peak_meter_r, right);

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
static int peak_meter_read_l (void)
{
    /* peak_meter_l contains the maximum of
       all peak values that were read by peak_meter_peek 
       since the last call of peak_meter_read_r */
    int retval = peak_meter_l;
#ifdef PM_DEBUG
    peek_calls = 0;
#endif

#ifdef SIMULATOR
    peak_meter_l = 8000;
#elif CONFIG_HWCODEC == MASNONE
    /* FIX */
    peak_meter_l = 8000;
#else
    /* reset peak_meter_l so that subsequent calls of
       peak_meter_peek doesn't get fooled by an old
       maximum value */
    peak_meter_l = mas_codec_readreg(peak_meter_src_l);
#endif
    return retval;
}

/**
 * Reads out the peak volume of the right channel.
 * @return int - The maximum value that has been detected
 * since the last call of peak_meter_read_l. The value
 * is in the range 0 <= value < MAX_PEAK.
 */
static int peak_meter_read_r (void) {
    /* peak_meter_r contains the maximum of
       all peak values that were read by peak_meter_peek 
       since the last call of peak_meter_read_r */
    int retval = peak_meter_r;
#ifdef PM_DEBUG
    peek_calls = 0;
#endif

#ifdef SIMULATOR
    peak_meter_l = 8000;
#elif CONFIG_HWCODEC == MASNONE
    /* FIX */
    peak_meter_r = 8000;
#else
    /* reset peak_meter_r so that subsequent calls of
       peak_meter_peek doesn't get fooled by an old
       maximum value */
    peak_meter_r = mas_codec_readreg(peak_meter_src_r);
#endif
    return retval;
}

/**
 * Reset the detected clips. This method is for
 * use by the user interface.
 * @param int unused - This parameter was added to
 * make the function compatible with set_int
 */
void peak_meter_set_clip_hold(int time) {
    peak_meter_clip_eternal = false;

    if (time <= 0) {
        peak_meter_l_clip = false;
        peak_meter_r_clip = false;
        peak_meter_clip_eternal = true;
    }
}

/**
 * Scales a peak value as read from the MAS to the range of meterwidth.
 * The scaling is performed according to the scaling method (dBfs / linear)
 * and the range (peak_meter_range_min .. peak_meter_range_max).
 * @param unsigned short val - The volume value. Range: 0 <= val < MAX_PEAK
 * @param int meterwidht - The widht of the meter in pixel
 * @return unsigned short - A value 0 <= return value <= meterwidth
 */
unsigned short peak_meter_scale_value(unsigned short val, int meterwidth){
    int retval;

    if (val <= peak_meter_range_min) {
        return 0;
    }

    if (val >= peak_meter_range_max) {
        return meterwidth;
    }

    retval = val;

    /* different scaling is used for dBfs and linear percent */
    if (peak_meter_use_dbfs) {

        /* scale the samples dBfs */
        retval  = (calc_db(retval) - db_min) * meterwidth / db_range;
    }

    /* Scale for linear percent display */
    else
    {
        /* scale the samples */
        retval  = ((retval - peak_meter_range_min) * meterwidth) 
                  / peak_meter_range;
    }
    return retval;
}


/**
 * Draws a peak meter in the specified size at the specified position.
 * @param int x - The x coordinate. 
 *                Make sure that 0 <= x and x + width < LCD_WIDTH
 * @param int y - The y coordinate. 
 *                Make sure that 0 <= y and y + height < LCD_HEIGHT
 * @param int width - The width of the peak meter. Note that for display
 *                    of clips a 3 pixel wide area is used ->
 *                    width > 3
 * @param int height - The height of the peak meter. height > 3
 */
void peak_meter_draw(int x, int y, int width, int height) {
    int left = 0, right = 0;
    static int last_left = 0, last_right = 0;
    int meterwidth = width - 3;
    int i;

#ifdef PM_DEBUG
    static long pm_tick = 0;
    int tmp = peek_calls;
#endif

    /* if disabled only draw the peak meter */
    if (peak_meter_enabled) {

        /* read the volume info from MAS */
        left  = peak_meter_read_l(); 
        right = peak_meter_read_r(); 
        /*peak_meter_peek();*/

        /* scale the samples dBfs */
        left  = peak_meter_scale_value(left, meterwidth);
        right = peak_meter_scale_value(right, meterwidth);
    
        /* if the scale has changed -> recalculate the scale 
           (The scale becomes invalid when the range changed.) */
        if (!db_scale_valid){

            if (peak_meter_use_dbfs) {
                db_scale_count = DB_SCALE_SRC_VALUES_SIZE;
                for (i = 0; i < db_scale_count; i++){
                    /* find the real x-coords for predefined interesting
                       dBfs values. These only are recalculated when the
                       scaling of the meter changed. */
                        db_scale_lcd_coord[i] = 
                            peak_meter_scale_value(
                                db_scale_src_values[i], 
                                meterwidth - 1);
                } 
            }

            /* when scaling linear we simly make 10% steps */
            else {
                db_scale_count = 10;
                for (i = 0; i < db_scale_count; i++) {
                    db_scale_lcd_coord[i] = 
                        (i * (MAX_PEAK / 10) - peak_meter_range_min) *
                        meterwidth / peak_meter_range;
                }
            }

            /* mark scale valid to avoid recalculating dBfs values
               of the scale. */
            db_scale_valid = true;
        }

        /* apply release */
        left  = MAX(left , last_left  - peak_meter_release);
        right = MAX(right, last_right - peak_meter_release);

        /* reset max values after timeout */
        if (TIME_AFTER(current_tick, peak_meter_timeout_l)){
            peak_meter_max_l = 0;
        }

        if (TIME_AFTER(current_tick, peak_meter_timeout_r)){
            peak_meter_max_r = 0;
        }

        if (!peak_meter_clip_eternal) {
            if (peak_meter_l_clip && 
                TIME_AFTER(current_tick, peak_meter_clip_timeout_l)){
                peak_meter_l_clip = false;
            }

            if (peak_meter_r_clip && 
                TIME_AFTER(current_tick, peak_meter_clip_timeout_r)){
                peak_meter_r_clip = false;
            }
        }

        /* check for new max values */
        if (left > peak_meter_max_l) {
            peak_meter_max_l = left - 1;
            peak_meter_timeout_l = 
                current_tick + max_time_out[peak_meter_hold];
        }

        if (right > peak_meter_max_r) {
            peak_meter_max_r = right - 1;
            peak_meter_timeout_r = 
                current_tick + max_time_out[peak_meter_hold];
        }
    }

    /* draw the peak meter */
    lcd_clearrect(x, y, width, height);

    /* draw left */
    lcd_fillrect (x, y, left, height / 2 - 2 );
    if (peak_meter_max_l > 0) {
        lcd_drawline(x + peak_meter_max_l, y, 
                     x + peak_meter_max_l, y + height / 2 - 2 );
    }
    if (peak_meter_l_clip) {
        lcd_fillrect(x + meterwidth, y, 3, height / 2 - 1);
    }

    /* draw right */
    lcd_fillrect(x, y + height / 2 + 1, right, height / 2 - 2);
    if (peak_meter_max_r > 0) {
        lcd_drawline( x + peak_meter_max_r, y + height / 2, 
                      x + peak_meter_max_r, y + height - 2);
    }
    if (peak_meter_r_clip) {
        lcd_fillrect(x + meterwidth, y + height / 2, 3, height / 2 - 1);
    }

    /* draw scale end */
    lcd_drawline(x + meterwidth, y,
                 x + meterwidth, y + height - 2);
    
    /* draw dots for scale marks */
    for (i = 0; i < db_scale_count; i++) {
        /* The x-coordinates of interesting scale mark points 
           have been calculated before */
        lcd_invertpixel(db_scale_lcd_coord[i], y + height / 2 - 1);
    }

#ifdef HAVE_RECORDING
    if (trig_status != TRIG_OFF) {
        int start_trigx, stop_trigx, ycenter;

        ycenter = y + height / 2;
        /* display threshold value */
        start_trigx = x+peak_meter_scale_value(trig_strt_threshold,meterwidth);
        lcd_drawline(start_trigx, ycenter - 2, start_trigx, ycenter);
        start_trigx ++;
        if (start_trigx < LCD_WIDTH) lcd_drawpixel(start_trigx, ycenter - 1);

        stop_trigx = x + peak_meter_scale_value(trig_stp_threshold,meterwidth);
        lcd_drawline(stop_trigx, ycenter - 2, stop_trigx, ycenter);
        if (stop_trigx > 0) lcd_drawpixel(stop_trigx - 1, ycenter - 1);
    }
#endif

#ifdef PM_DEBUG
    /* display a bar to show how many calls to peak_meter_peek 
       have ocurred since the last display */
    lcd_invertrect(x, y, tmp, 3);

    if (tmp < PEEKS_PER_DRAW_SIZE) {
        peeks_per_redraw[tmp]++;
    }

    tmp = current_tick - pm_tick;
    if (tmp < TICKS_PER_DRAW_SIZE ){
        ticks_per_redraw[tmp] ++;
    }

    /* display a bar to show how many ticks have passed since 
       the last redraw */
    lcd_invertrect(x, y + height / 2, current_tick - pm_tick, 2);
    pm_tick = current_tick;
#endif

    last_left = left;
    last_right = right;
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
void peak_meter_trigger(bool on) {
    /* don't use set_trigger here as that would fire an undesired event */
    trig_status = on ? TRIG_READY : TRIG_OFF;
}

/**
 * Registers the listener function that listenes on trig_status changes.
 * @param listener - The function that is called with each change of
 *                   trig_status. May be set to NULL if no callback is desired.
 */
void peak_meter_set_trigger_listener(void (*listener)(int status)) {
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
int peak_meter_trigger_status(void) {
   return trig_status; /* & TRIG_PIT_MASK;*/
}

void peak_meter_draw_trig(int xpos, int ypos) {
    int x = xpos + ICON_PLAY_STATE_WIDTH + 1;
    switch (trig_status) {
        long time_left;

        case TRIG_READY:
            scrollbar(x, ypos + 1, TRIGBAR_WIDTH, TRIG_HEIGHT - 2,
                      TRIGBAR_WIDTH, 0, 0, HORIZONTAL);
            lcd_bitmap(bitmap_icons_7x8[Icon_Stop], xpos, ypos,
                       ICON_PLAY_STATE_WIDTH, STATUSBAR_HEIGHT, false);
            break;

        case TRIG_STEADY:
        case TRIG_RETRIG:
            time_left = trig_strt_duration - (current_tick - trig_hightime);
            time_left = time_left * TRIGBAR_WIDTH / trig_strt_duration;
            scrollbar(x, ypos + 1, TRIGBAR_WIDTH, TRIG_HEIGHT - 2,
                      TRIGBAR_WIDTH, 0, TRIGBAR_WIDTH - time_left, HORIZONTAL);
            lcd_bitmap(bitmap_icons_7x8[Icon_Stop], xpos, ypos,
                       ICON_PLAY_STATE_WIDTH, STATUSBAR_HEIGHT, false);
            break;

        case TRIG_GO:
        case TRIG_CONTINUE:
            scrollbar(x, ypos + 1, TRIGBAR_WIDTH, TRIG_HEIGHT - 2,
                      TRIGBAR_WIDTH, TRIGBAR_WIDTH, TRIGBAR_WIDTH, HORIZONTAL);
            lcd_bitmap(bitmap_icons_7x8[Icon_Record],
                       TRIG_WIDTH - ICON_PLAY_STATE_WIDTH, ypos,
                       ICON_PLAY_STATE_WIDTH, STATUSBAR_HEIGHT, false);
            break;

        case TRIG_POSTREC:
            time_left = trig_stp_hold - (current_tick - trig_lowtime);
            time_left = time_left * TRIGBAR_WIDTH / trig_stp_hold;
            scrollbar(x, ypos + 1, TRIGBAR_WIDTH, TRIG_HEIGHT - 2,
                      TRIGBAR_WIDTH, time_left, TRIGBAR_WIDTH, HORIZONTAL);
            lcd_bitmap(bitmap_icons_7x8[Icon_Record],
                       TRIG_WIDTH - ICON_PLAY_STATE_WIDTH, ypos,
                       ICON_PLAY_STATE_WIDTH, STATUSBAR_HEIGHT, false);
            break;
    }

}
#endif

int peak_meter_draw_get_btn(int x, int y, int width, int height)
{
    int button;
    long next_refresh = current_tick;
    long next_big_refresh = current_tick + HZ / 10;
    button = BUTTON_NONE;
    while (!TIME_AFTER(current_tick, next_big_refresh)) {
        button = button_get(false);
        if (button != BUTTON_NONE) {
            break;
        }
        peak_meter_peek();
        yield();

        if (TIME_AFTER(current_tick, next_refresh)) {
            peak_meter_draw(x, y, width, height);
            lcd_update_rect(x, y, width, height);
            next_refresh = current_tick + HZ / peak_meter_fps;
        }
    }
    return button;
}

#ifdef PM_DEBUG
static void peak_meter_clear_histogram(void) {
    int i = 0;
    for (i = 0; i < TICKS_PER_DRAW_SIZE; i++) {
        ticks_per_redraw[i] = (unsigned int)0;
    }

    for (i = 0; i < PEEKS_PER_DRAW_SIZE; i++) {
        peeks_per_redraw[i] = (unsigned int)0;
    }
}

bool peak_meter_histogram(void) {
    int i;
    int btn = BUTTON_NONE;
    while ((btn & BUTTON_OFF) != BUTTON_OFF )
    {
        unsigned int max = 0;
        int y = 0;
        int x = 0;
        lcd_clear_display();

        for (i = 0; i < PEEKS_PER_DRAW_SIZE; i++) {
            max = MAX(max, peeks_per_redraw[i]);
        }

        for (i = 0; i < PEEKS_PER_DRAW_SIZE; i++) {
            x = peeks_per_redraw[i] * (LCD_WIDTH - 1)/ max;
            lcd_drawline(0, y + i, x, y + i);
        }

        y = PEEKS_PER_DRAW_SIZE + 1;
        max = 0;

        for (i = 0; i < TICKS_PER_DRAW_SIZE; i++) {
            max = MAX(max, ticks_per_redraw[i]);
        }

        for (i = 0; i < TICKS_PER_DRAW_SIZE; i++) {
            x = ticks_per_redraw[i] * (LCD_WIDTH - 1)/ max;
            lcd_drawline(0, y + i, x, y + i);
        }
        lcd_update();
        
        btn = button_get(true);
        if (btn == BUTTON_PLAY) {
            peak_meter_clear_histogram();
        }
    }
    return false;
}
#endif

