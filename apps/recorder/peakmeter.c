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
#include "mas.h"
#include "thread.h"
#include "kernel.h"
#include "settings.h"
#include "lcd.h"
#include "wps-display.h"
#include "sprintf.h"
#include "button.h"
#include "system.h"
#include "font.h"
#include "icons.h"
#include "lang.h"
#include "peakmeter.h"

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

/* if set to true clip timeout is disabled */
static bool peak_meter_clip_eternal = false;

#ifndef SIMULATOR
static int peak_meter_src_l = MAS_REG_DQPEAK_L;
static int peak_meter_src_r = MAS_REG_DQPEAK_R;
#endif

/* temporarily en- / disables peak meter. This is
   especially for external applications to detect 
   if the peak_meter is in use and needs drawing at all */
bool peak_meter_enabled = true;

static int peak_meter_l;
static int peak_meter_r;

/* time out values for max */
static long max_time_out[] = {
    0 * HZ, HZ / 5, 30, HZ / 2, HZ, 2 * HZ, 
    3 * HZ, 4 * HZ, 5 * HZ, 6 * HZ, 7 * HZ, 8 * HZ,
    9 * HZ, 10 * HZ, 15 * HZ, 20 * HZ, 30 * HZ, 60 * HZ
};

/* time out values for clip */
static long clip_time_out[] = {
    0 * HZ, 1  * HZ, 2 * HZ, 3 * HZ, 4 * HZ, 5 * HZ, 
    6 * HZ, 7 * HZ, 8 * HZ, 9 * HZ, 10 * HZ, 15 * HZ,
    20 * HZ, 25 * HZ, 30 * HZ, 45 * HZ, 60 * HZ, 90 * HZ,
    120 * HZ, 180 * HZ, 300 * HZ, 600 * HZ, 1200 * HZ,
    2700 * HZ, 5400 * HZ
};

/**
 * Set the source of the peak meter to playback or to 
 * record.
 * @param: bool playback - If true playback peak meter is used.
 *         If false recording peak meter is used.
 */
void peak_meter_playback(bool playback) {
#ifdef SIMULATOR
    (void)playback;
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

/**
 * Reads peak values from the MAS, and detects clips. The 
 * values are stored in peak_meter_l peak_meter_r for later 
 * evauluation. Consecutive calls to peak_meter_peek detect 
 * that ocurred. This function could be used by a thread for
 * busy reading the MAS.
 */
void peak_meter_peek(void) {
#ifdef SIMULATOR
    int left = 8000;
    int right = 9000;
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
            current_tick + clip_time_out[global_settings.peak_meter_clip_hold];
    }

    if ((right == peak_meter_r) && 
        (right == MAX_PEAK - 1)) {
        peak_meter_r_clip = true;
        peak_meter_clip_timeout_r = 
            current_tick + clip_time_out[global_settings.peak_meter_clip_hold];
    }

    /* peaks are searched -> we have to find the maximum */
    peak_meter_l = MAX(peak_meter_l, left);
    peak_meter_r = MAX(peak_meter_r, right);
}

/**
 * Reads out the peak volume of the left channel.
 * @return int - The maximum value that has been detected
 * since the last call of peak_meter_read_l. The value
 * is in the range 0 <= value < MAX_PEAK.
 */
static int peak_meter_read_l (void) {
    int retval = peak_meter_l;
#ifdef SIMULATOR
    peak_meter_l = 8000;
#else
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
    int retval = peak_meter_r;
#ifdef SIMULATOR
    peak_meter_l = 8000;
#else
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

    /* if disabled only draw the peak meter */
    if (peak_meter_enabled) {
        /* read the volume info from MAS */
        left  = peak_meter_read_l(); 
        right = peak_meter_read_r(); 
        peak_meter_peek();


        /* scale the samples */
        left  /= (MAX_PEAK / meterwidth);
        right /= (MAX_PEAK / meterwidth);

        /* apply release */
        left  = MAX(left , last_left  - global_settings.peak_meter_release);
        right = MAX(right, last_right - global_settings.peak_meter_release);

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
                current_tick + max_time_out[global_settings.peak_meter_hold];
        }

        if (right > peak_meter_max_r) {
            peak_meter_max_r = right - 1;
            peak_meter_timeout_r = 
                current_tick + max_time_out[global_settings.peak_meter_hold];
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

    /* draw scale */
    lcd_drawline(x + meterwidth, y,
                 x + meterwidth, y + height - 2);
    for (i = 0; i < 10; i++) {
        lcd_invertpixel(x + meterwidth * i / 10, y + height / 2 - 1);
    }

    last_left = left;
    last_right = right;
}
