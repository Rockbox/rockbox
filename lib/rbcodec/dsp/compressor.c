/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Jeffrey Goode
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
#include <stdbool.h>
#include <sys/types.h>
#include "fixedpoint.h"
#include "fracmul.h"
#include <string.h>

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"
#include "dsp_proc_entry.h"
#include "compressor.h"

static struct compressor_settings curr_set; /* Cached settings */

static int32_t comp_rel_slope IBSS_ATTR;   /* S7.24 format */
static int32_t comp_makeup_gain IBSS_ATTR; /* S7.24 format */
static int32_t comp_curve[66] IBSS_ATTR;   /* S7.24 format */
static int32_t release_gain IBSS_ATTR;     /* S7.24 format */

#define UNITY (1L << 24)                   /* unity gain in S7.24 format */

/** COMPRESSOR UPDATE
 *  Called via the menu system to configure the compressor process */
static bool compressor_update(const struct compressor_settings *settings)
{
    /* make settings values useful */
    int  threshold  = settings->threshold;
    bool auto_gain  = settings->makeup_gain == 1;
    static const int comp_ratios[] = { 2, 4, 6, 10, 0 };
    int  ratio      = comp_ratios[settings->ratio];
    bool soft_knee  = settings->knee == 1;
    int  release    = settings->release_time * NATIVE_FREQUENCY / 1000;

    bool changed = false;
    bool active  = threshold < 0;

    if (memcmp(settings, &curr_set, sizeof (curr_set)))
    {
        /* Compressor settings have changed since last call */
        changed = true;
            
#if defined(ROCKBOX_HAS_LOGF) && defined(LOGF_ENABLE)
        if (settings->threshold != curr_set.threshold)
        {
            logf("   Compressor Threshold: %d dB\tEnabled: %s",
                 threshold, active ? "Yes" : "No");
        }

        if (settings->makeup_gain != curr_set.makeup_gain)
        {
            logf("   Compressor Makeup Gain: %s",
                 auto_gain ? "Auto" : "Off");
         }

        if (settings->ratio != cur_set.ratio)
        {
            if (ratio)
                { logf("   Compressor Ratio: %d:1", ratio); }
            else
                { logf("   Compressor Ratio: Limit"); }
        }

        if (settings->knee != cur_set.knee)
        {
            logf("   Compressor Knee: %s", soft_knee?"Soft":"Hard");
        }

        if (settings->release_time != cur_set.release_time)
        {
            logf("   Compressor Release: %d", release);
        }
#endif

        curr_set = *settings;
    }

    if (!changed || !active)
        return active;

    /* configure variables for compressor operation */
    static const int32_t db[] = {
        /* positive db equivalents in S15.16 format */
        0x000000, 0x241FA4, 0x1E1A5E, 0x1A94C8,
        0x181518, 0x1624EA, 0x148F82, 0x1338BD,
        0x120FD2, 0x1109EB, 0x101FA4, 0x0F4BB6,
        0x0E8A3C, 0x0DD840, 0x0D3377, 0x0C9A0E,
        0x0C0A8C, 0x0B83BE, 0x0B04A5, 0x0A8C6C,
        0x0A1A5E, 0x09ADE1, 0x094670, 0x08E398,
        0x0884F6, 0x082A30, 0x07D2FA, 0x077F0F,
        0x072E31, 0x06E02A, 0x0694C8, 0x064BDF,
        0x060546, 0x05C0DA, 0x057E78, 0x053E03,
        0x04FF5F, 0x04C273, 0x048726, 0x044D64,
        0x041518, 0x03DE30, 0x03A89B, 0x037448,
        0x03412A, 0x030F32, 0x02DE52, 0x02AE80,
        0x027FB0, 0x0251D6, 0x0224EA, 0x01F8E2,
        0x01CDB4, 0x01A359, 0x0179C9, 0x0150FC,
        0x0128EB, 0x010190, 0x00DAE4, 0x00B4E1,
        0x008F82, 0x006AC1, 0x004699, 0x002305};

    struct curve_point
    {
        int32_t db;     /* S15.16 format */
        int32_t offset; /* S15.16 format */
    } db_curve[5];

    /** Set up the shape of the compression curve first as decibel
        values */
    /* db_curve[0] = bottom of knee
               [1] = threshold
               [2] = top of knee
               [3] = 0 db input
               [4] = ~+12db input (2 bits clipping overhead) */

    db_curve[1].db = threshold << 16;
    if (soft_knee)
    {
        /* bottom of knee is 3dB below the threshold for soft knee*/
        db_curve[0].db = db_curve[1].db - (3 << 16);
        /* top of knee is 3dB above the threshold for soft knee */
        db_curve[2].db = db_curve[1].db + (3 << 16);
        if (ratio)
            /* offset = -3db * (ratio - 1) / ratio */
            db_curve[2].offset = (int32_t)((long long)(-3 << 16)
                * (ratio - 1) / ratio);
        else
            /* offset = -3db for hard limit */
            db_curve[2].offset = (-3 << 16);
    }
    else
    {
        /* bottom of knee is at the threshold for hard knee */
        db_curve[0].db = threshold << 16;
        /* top of knee is at the threshold for hard knee */
        db_curve[2].db = threshold << 16;
        db_curve[2].offset = 0;
    }

    /* Calculate 0db and ~+12db offsets */
    db_curve[4].db = 0xC0A8C; /* db of 2 bits clipping */
    if (ratio)
    {
        /* offset = threshold * (ratio - 1) / ratio */
        db_curve[3].offset = (int32_t)((long long)(threshold << 16)
            * (ratio - 1) / ratio);
        db_curve[4].offset = (int32_t)((long long)-db_curve[4].db
            * (ratio - 1) / ratio) + db_curve[3].offset;
    }
    else
    {
        /* offset = threshold for hard limit */
        db_curve[3].offset = (threshold << 16);
        db_curve[4].offset = -db_curve[4].db + db_curve[3].offset;
    }

    /** Now set up the comp_curve table with compression offsets in the
        form of gain factors in S7.24 format */
    /* comp_curve[0] is 0 (-infinity db) input */
    comp_curve[0] = UNITY;
    /* comp_curve[1 to 63] are intermediate compression values 
       corresponding to the 6 MSB of the input values of a non-clipped
       signal */
    for (int i = 1; i < 64; i++)
    {
        /* db constants are stored as positive numbers;
           make them negative here */
        int32_t this_db = -db[i];

        /* no compression below the knee */
        if (this_db <= db_curve[0].db)
            comp_curve[i] = UNITY;

        /* if soft knee and below top of knee,
           interpolate along soft knee slope */
        else if (soft_knee && (this_db <= db_curve[2].db))
            comp_curve[i] = fp_factor(fp_mul(
                ((this_db - db_curve[0].db) / 6),
                db_curve[2].offset, 16), 16) << 8;

        /* interpolate along ratio slope above the knee */
        else
            comp_curve[i] = fp_factor(fp_mul(
                fp_div((db_curve[1].db - this_db), db_curve[1].db, 16),
                db_curve[3].offset, 16), 16) << 8;
    }
    /* comp_curve[64] is the compression level of a maximum level,
       non-clipped signal */
    comp_curve[64] = fp_factor(db_curve[3].offset, 16) << 8;

    /* comp_curve[65] is the compression level of a maximum level,
       clipped signal */
    comp_curve[65] = fp_factor(db_curve[4].offset, 16) << 8;

#if defined(ROCKBOX_HAS_LOGF) && defined(LOGF_ENABLE)
    logf("\n   *** Compression Offsets ***");
    /* some settings for display only, not used in calculations */
    db_curve[0].offset = 0;
    db_curve[1].offset = 0;
    db_curve[3].db = 0;

    for (int i = 0; i <= 4; i++)
    {
        logf("Curve[%d]: db: % 6.2f\toffset: % 6.2f", i,
            (float)db_curve[i].db / (1 << 16),
            (float)db_curve[i].offset / (1 << 16));
    }

    logf("\nGain factors:");
    for (int i = 1; i <= 65; i++)
    {
        DEBUGF("%02d: %.6f  ", i, (float)comp_curve[i] / UNITY);
        if (i % 4 == 0) DEBUGF("\n");
    }
    DEBUGF("\n");
#endif

    /* if using auto peak, then makeup gain is max offset -
       .1dB headroom */
    comp_makeup_gain = auto_gain ?
        fp_factor(-(db_curve[3].offset) - 0x199A, 16) << 8 : UNITY;
    logf("Makeup gain:\t%.6f", (float)comp_makeup_gain / UNITY);

    /* calculate per-sample gain change a rate of 10db over release time
     */
    comp_rel_slope = 0xAF0BB2 / release;
    logf("Release slope:\t%.6f", (float)comp_rel_slope / UNITY);

    release_gain = UNITY;
    return active;
}

/** GET COMPRESSION GAIN
 *  Returns the required gain factor in S7.24 format in order to compress the
 *  sample in accordance with the compression curve.  Always 1 or less.
 */
static inline int32_t get_compression_gain(struct sample_format *format,
                                           int32_t sample)
{
    const int frac_bits_offset = format->frac_bits - 15;
    
    /* sample must be positive */
    if (sample < 0)
        sample = -(sample + 1);
        
    /* shift sample into 15 frac bit range */
    if (frac_bits_offset > 0)
        sample >>= frac_bits_offset;
    if (frac_bits_offset < 0)
        sample <<= -frac_bits_offset;
    
    /* normal case: sample isn't clipped */
    if (sample < (1 << 15))
    {
        /* index is 6 MSB, rem is 9 LSB */
        int index = sample >> 9;
        int32_t rem = (sample & 0x1FF) << 22;
        
        /* interpolate from the compression curve:
            higher gain - ((rem / (1 << 31)) * (higher gain - lower gain)) */
        return comp_curve[index] - (FRACMUL(rem,
            (comp_curve[index] - comp_curve[index + 1])));
    }
    /* sample is somewhat clipped, up to 2 bits of overhead */
    if (sample < (1 << 17))
    {
        /* straight interpolation:
            higher gain - ((clipped portion of sample * 4/3
            / (1 << 31)) * (higher gain - lower gain)) */
        return comp_curve[64] - (FRACMUL(((sample - (1 << 15)) / 3) << 16,
            (comp_curve[64] - comp_curve[65])));
    }
    
    /* sample is too clipped, return invalid value */
    return -1;
}

/** DSP interface **/

/** SET COMPRESSOR
 *  Enable or disable the compressor based upon the settings
 */
void dsp_set_compressor(const struct compressor_settings *settings)
{
    /* enable/disable the compressor depending upon settings */
    bool enable = compressor_update(settings);
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    dsp_proc_enable(dsp, DSP_PROC_COMPRESSOR, enable);
    dsp_proc_activate(dsp, DSP_PROC_COMPRESSOR, true);
}

/** COMPRESSOR PROCESS
 *  Changes the gain of the samples according to the compressor curve
 */
static void compressor_process(struct dsp_proc_entry *this,
                               struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;
    int32_t *in_buf[2] = { buf->p32[0], buf->p32[1] };
    const int num_chan = buf->format.num_channels;

    while (count-- > 0)
    {
        /* use lowest (most compressed) gain factor of the output buffer
           sample pair for both samples (mono is also handled correctly here)
         */
        int32_t sample_gain = UNITY;
        for (int ch = 0; ch < num_chan; ch++)
        {
            int32_t this_gain = get_compression_gain(&buf->format, *in_buf[ch]);
            if (this_gain < sample_gain)
                sample_gain = this_gain;
        }
        
        /* perform release slope; skip if no compression and no release slope
         */
        if ((sample_gain != UNITY) || (release_gain != UNITY))
        {
            /* if larger offset than previous slope, start new release slope
             */
            if ((sample_gain <= release_gain) && (sample_gain > 0))
            {
                release_gain = sample_gain;
            }
            else
            /* keep sloping towards unity gain (and ignore invalid value) */
            {
                release_gain += comp_rel_slope;
                if (release_gain > UNITY)
                {
                    release_gain = UNITY;
                }
            }
        }
        
        /* total gain factor is the product of release gain and makeup gain,
           but avoid computation if possible */
        int32_t total_gain = ((release_gain == UNITY) ? comp_makeup_gain :
            (comp_makeup_gain == UNITY) ? release_gain :
                FRACMUL_SHL(release_gain, comp_makeup_gain, 7));
        
        /* Implement the compressor: apply total gain factor (if any) to the
           output buffer sample pair/mono sample */
        if (total_gain != UNITY)
        {
            for (int ch = 0; ch < num_chan; ch++)
            {
                *in_buf[ch] = FRACMUL_SHL(total_gain, *in_buf[ch], 7);
            }
        }
        in_buf[0]++;
        in_buf[1]++;
    }

    (void)this;
}

/* DSP message hook */
static intptr_t compressor_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     unsigned int setting,
                                     intptr_t value)
{
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value != 0)
            break; /* Already enabled */
        this->process[0] = compressor_process;
    case DSP_RESET:
    case DSP_FLUSH:
        release_gain = UNITY;
        break;
    }

    return 1;
    (void)dsp;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    COMPRESSOR,
    compressor_configure);
