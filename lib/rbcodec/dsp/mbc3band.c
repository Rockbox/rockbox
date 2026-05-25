/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * 3-band OTT multiband compressor for Rockbox
 * Fixed-point implementation for ARM targets (iPod Classic 6/7)
 *
 * Signal flow:
 *   Input → [HPF crossover1] → Band 2 (High)  ─┐
 *        → [LPF crossover1] → [HPF crossover2] → Band 1 (Mid)   ├→ mix → Output
 *                           → [LPF crossover2] → Band 0 (Low)  ─┘
 *
 * Crossover: Linkwitz-Riley 12dB/oct (cascaded Butterworth 2nd-order biquads)
 *
 * Per-band: OTT-style downward + upward compression with soft knee
 *
 * Copyright (C) 2024
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

#include "rbcodecconfig.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include <string.h>
#include "mbc3band.h"
#include "dsp_proc_entry.h"
#include "dsp_core.h"
#include "dsp_filter.h"
#include "dsp_sample_io.h"
#include "dsp_misc.h"
#include "platform.h"

#define UNITY (1L << 24)
#define MAX_CH 2

#define GAIN_TABLE_SIZE 66

struct band_coeff
{
    int32_t gain_table[GAIN_TABLE_SIZE];
    int32_t attca, attcb;
    int32_t rlsca, rlscb;
    int32_t makeup_gain;
    int32_t wet_mix;
    int32_t dry_mix;
    bool active;
};

struct band_state
{
    int32_t envelope[MAX_CH];
};

struct xover_pair
{
    struct dsp_filter f[2];
};

static struct mbc3band_settings curr_set;
static struct band_coeff band_coeffs[MBC_NUM_BANDS];
static struct band_state band_states[MBC_NUM_BANDS];

static struct xover_pair xover_lp;
static struct xover_pair xover_hp;
static struct xover_pair xover_lp2;
static struct xover_pair xover_hp2;

static int32_t output_gain;

static const int32_t db_table[64] = {
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
    0x008F82, 0x006AC1, 0x004699, 0x002305
};

static int32_t get_tc_coeff(int32_t rc_ms, int32_t fs)
{
    if (rc_ms <= 0)
        return UNITY;
    int32_t c = UNITY / fs;
    c *= 1152;
    c /= rc_ms;
    return c;
}

static FORCE_INLINE int32_t biquad_step(struct dsp_filter *f, int ch, int32_t x)
{
    int64_t acc = (int64_t)x * f->coefs[0];
    acc += (int64_t)f->history[ch][0] * f->coefs[1];
    acc += (int64_t)f->history[ch][1] * f->coefs[2];
    acc += (int64_t)f->history[ch][2] * f->coefs[3];
    acc += (int64_t)f->history[ch][3] * f->coefs[4];

    f->history[ch][1] = f->history[ch][0];
    f->history[ch][0] = x;
    f->history[ch][3] = f->history[ch][2];
    int32_t y = (int32_t)((acc << f->shift) >> 32);
    f->history[ch][2] = y;
    return y;
}

static void butterworth_coefs(unsigned long cutoff_phase, bool highpass,
                               struct dsp_filter *f)
{
    long cos_w0, sin_w0;
    sin_w0 = fp_sincos(cutoff_phase, &cos_w0);

    int32_t alpha = (int32_t)(((int64_t)sin_w0 * (int64_t)0x5A82799ALL) >> 31);
    int32_t cos_w0_s24 = cos_w0 >> 7;
    int32_t alpha_s24   = alpha >> 7;

    int32_t b0, b1, b2, a0, a1, a2;

    if (highpass)
    {
        int32_t hpc = (UNITY + cos_w0_s24) >> 1;
        b0 = hpc;
        b1 = -2 * hpc;
        b2 = hpc;
    }
    else
    {
        int32_t lpc = (UNITY - cos_w0_s24) >> 1;
        b0 = lpc;
        b1 = 2 * lpc;
        b2 = lpc;
    }

    a0 = UNITY + alpha_s24;
    a1 = -2 * cos_w0_s24;
    a2 = UNITY - alpha_s24;

    int32_t rcp_a0 = (int32_t)(((int64_t)1 << 55) / (int64_t)a0);

    f->coefs[0] = FRACMUL(b0, rcp_a0);
    f->coefs[1] = FRACMUL(b1, rcp_a0);
    f->coefs[2] = FRACMUL(b2, rcp_a0);
    f->coefs[3] = FRACMUL(-a1, rcp_a0);
    f->coefs[4] = FRACMUL(-a2, rcp_a0);
    f->shift = 6;
}

static void setup_xover(struct xover_pair *xp, int cutoff_hz,
                         unsigned long fs, bool is_lp)
{
    unsigned long phase = fp_div(cutoff_hz, fs, 32);
    butterworth_coefs(phase, !is_lp, &xp->f[0]);
    butterworth_coefs(phase, !is_lp, &xp->f[1]);
}

static void flush_xover(struct xover_pair *xp)
{
    filter_flush(&xp->f[0]);
    filter_flush(&xp->f[1]);
}

static void build_gain_table(struct band_coeff *bc,
                              const struct ott_band_settings *band,
                              int32_t fs)
{
    int threshold   = band->threshold;
    int ratio_down  = band->ratio_down;
    int ratio_up    = band->ratio_up;
    int knee_db     = band->knee_db;
    int makeup_d10  = band->makeup_gain_db;

    int32_t thresh_s16 = (int32_t)threshold << 16;
    int32_t knee_half  = (knee_db << 16) >> 1;

    int32_t down_off_0 = 0;
    if (ratio_down <= 0)
        down_off_0 = thresh_s16;
    else if (ratio_down > 100)
        down_off_0 = (int32_t)(((int64_t)thresh_s16 * (ratio_down - 100)) / ratio_down);

    for (int i = 0; i < 64; i++)
    {
        int32_t this_db = -db_table[i];
        int32_t gain_db = 0;

        int32_t knee_bottom = thresh_s16 - knee_half;
        int32_t knee_top    = thresh_s16 + knee_half;

        if (knee_db <= 0)
        {
            knee_bottom = thresh_s16;
            knee_top    = thresh_s16;
        }

        if (this_db >= knee_top)
        {
            if (ratio_down <= 0)
                gain_db = thresh_s16 - this_db;
            else if (ratio_down > 100)
            {
                int32_t excess = this_db - thresh_s16;
                int32_t reduction =
                    (int32_t)(((int64_t)excess * (ratio_down - 100)) / ratio_down);
                gain_db = -reduction;
            }
        }
        else if (this_db <= knee_bottom)
        {
            if (ratio_up > 0 && ratio_up < 100)
            {
                int32_t deficit = thresh_s16 - this_db;
                if (deficit > 0)
                    gain_db = (int32_t)(((int64_t)deficit * (100 - ratio_up)) / 100);
            }
        }
        else
        {
            int32_t gain_up = 0, gain_down = 0;
            if (ratio_up > 0 && ratio_up < 100)
            {
                int32_t def = thresh_s16 - knee_bottom;
                if (def > 0)
                    gain_up = (int32_t)(((int64_t)def * (100 - ratio_up)) / 100);
            }
            if (ratio_down > 100)
            {
                int32_t ex = knee_top - thresh_s16;
                gain_down = -(int32_t)(((int64_t)ex * (ratio_down - 100)) / ratio_down);
            }
            int32_t frac = this_db - knee_bottom;
            int32_t knee_range = knee_top - knee_bottom;
            if (knee_range > 0)
                gain_db = gain_up +
                    (int32_t)(((int64_t)(gain_down - gain_up) * frac) / knee_range);
        }

        if (gain_db > 24 << 16)
            gain_db = 24 << 16;

        bc->gain_table[i] = fp_factor(gain_db, 16) << 8;
    }

    bc->gain_table[64] = fp_factor(down_off_0, 16) << 8;

    {
        int32_t over_12 = 0xC0A8C;
        int32_t off_12 = 0;
        if (ratio_down <= 0)
            off_12 = thresh_s16 - over_12;
        else if (ratio_down > 100)
        {
            int32_t ex = over_12 - thresh_s16;
            if (ex > 0)
                off_12 = -(int32_t)(((int64_t)ex * (ratio_down - 100)) / ratio_down);
        }
        bc->gain_table[65] = fp_factor(off_12, 16) << 8;
    }

    int at_ms = band->attack_ms;
    int rl_ms = band->release_ms;

    int32_t a = (at_ms > 0) ? get_tc_coeff(at_ms, fs) : UNITY;
    int32_t r = get_tc_coeff(rl_ms, fs);

    bc->attca = a;
    bc->attcb = UNITY - a;
    bc->rlsca = r;
    bc->rlscb = UNITY - r;

    if (makeup_d10 > 0)
    {
        int32_t mg_db_int = makeup_d10 / 10;
        int32_t mg_db_frac = makeup_d10 % 10;
        int32_t mg_s16 = (mg_db_int << 16) + (mg_db_frac * 6554);
        bc->makeup_gain = fp_factor(mg_s16, 16) << 8;
    }
    else
    {
        bc->makeup_gain = UNITY;
    }

    int wet = band->wet_mix;
    if (wet < 0) wet = 0;
    if (wet > 100) wet = 100;
    bc->wet_mix = ((int64_t)wet * UNITY) / 100;
    bc->dry_mix = UNITY - bc->wet_mix;

    bc->active = (threshold < 0) && (ratio_down != 100 || ratio_up != 100);
}

static FORCE_INLINE int32_t get_band_gain(struct band_coeff *bc,
                                           int32_t sample,
                                           int frac_bits)
{
    const int off = frac_bits - 15;
    if (off > 0)
        sample >>= off;
    else if (off < 0)
        sample <<= -off;

    if (sample <= 0)
        return bc->gain_table[0];

    if (sample < (1 << 15))
    {
        int idx = sample >> 9;
        int32_t rem = (sample & 0x1FF) << 22;
        int32_t g0 = bc->gain_table[idx];
        int32_t g1 = bc->gain_table[idx + 1];
        int32_t diff = g0 - g1;
        return g0 - (int32_t)(((int64_t)rem * (int64_t)diff) >> 31);
    }

    if (sample < (1 << 17))
    {
        int32_t frac = ((sample - (1 << 15)) / 3) << 16;
        int32_t g64 = bc->gain_table[64];
        int32_t g65 = bc->gain_table[65];
        int32_t diff = g64 - g65;
        return g64 - (int32_t)(((int64_t)frac * (int64_t)diff) >> 31);
    }

    return -1;
}

static FORCE_INLINE int32_t apply_band_compressor(
    struct band_coeff *bc, struct band_state *bs,
    int32_t sample, int ch, int frac_bits)
{
    int32_t target_gain = get_band_gain(bc, sample, frac_bits);
    int32_t *env = &bs->envelope[ch];

    if (target_gain <= *env && target_gain > 0)
        *env = FRACMUL_SHL(*env, bc->attcb, 7)
             + FRACMUL_SHL(target_gain, bc->attca, 7);
    else
        *env = FRACMUL_SHL(*env, bc->rlscb, 7)
             + FRACMUL_SHL(target_gain, bc->rlsca, 7);

    int32_t gain = FRACMUL_SHL(*env, bc->makeup_gain, 7);
    int32_t wet_gain = FRACMUL_SHL(bc->wet_mix, gain, 7);
    return bc->dry_mix + wet_gain;
}

static FORCE_INLINE int32_t compress_band(
    struct band_coeff *bc, struct band_state *bs,
    int32_t x, int ch, int frac_bits)
{
    int32_t abs_x = (x < 0) ? -(x + 1) : x;
    int32_t gain = apply_band_compressor(bc, bs, abs_x, ch, frac_bits);
    return FRACMUL_SHL(x, gain, 7);
}

static void mbc3band_process(struct dsp_proc_entry *this,
                              struct dsp_buffer **buf_p)
{
    (void)this;
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;
    int32_t *out_l = buf->p32[0];
    int32_t *out_r = buf->p32[1];
    const int num_ch = buf->format.num_channels;
    const int frac_bits = buf->format.frac_bits;
    int mode = curr_set.mode;

    struct band_coeff *bc[3];
    struct band_state *bs[3];
    for (int b = 0; b < MBC_NUM_BANDS; b++)
    {
        bc[b] = &band_coeffs[b];
        bs[b] = &band_states[b];
    }

    for (int s = 0; s < count; s++)
    {
        int32_t L = out_l[s];
        int32_t R = (num_ch > 1) ? out_r[s] : L;

        int32_t outL = 0, outR = 0;

        if (mode == MBC_MODE_1BAND)
        {
            outL = compress_band(bc[MBC_BAND_MID], bs[MBC_BAND_MID],
                                 L, 0, frac_bits);
            outR = compress_band(bc[MBC_BAND_MID], bs[MBC_BAND_MID],
                                 R, (num_ch > 1) ? 1 : 0, frac_bits);
        }
        else if (mode == MBC_MODE_2BAND)
        {
            for (int ch = 0; ch < num_ch; ch++)
            {
                int32_t x = (ch == 0) ? L : R;

                int32_t lp = biquad_step(&xover_lp.f[0], ch, x);
                lp = biquad_step(&xover_lp.f[1], ch, lp);
                int32_t hp = biquad_step(&xover_hp.f[0], ch, x);
                hp = biquad_step(&xover_hp.f[1], ch, hp);

                int32_t low  = compress_band(bc[MBC_BAND_LOW], bs[MBC_BAND_LOW],
                                              lp, ch, frac_bits);
                int32_t high = compress_band(bc[MBC_BAND_HIGH], bs[MBC_BAND_HIGH],
                                              hp, ch, frac_bits);

                if (ch == 0) outL = low + high;
                else         outR = low + high;
            }
            if (num_ch == 1) outR = outL;
        }
        else
        {
            for (int ch = 0; ch < num_ch; ch++)
            {
                int32_t x = (ch == 0) ? L : R;

                int32_t lp1 = biquad_step(&xover_lp.f[0], ch, x);
                lp1 = biquad_step(&xover_lp.f[1], ch, lp1);
                int32_t hp1 = biquad_step(&xover_hp.f[0], ch, x);
                hp1 = biquad_step(&xover_hp.f[1], ch, hp1);

                int32_t lp2 = biquad_step(&xover_lp2.f[0], ch, hp1);
                lp2 = biquad_step(&xover_lp2.f[1], ch, lp2);
                int32_t hp2 = biquad_step(&xover_hp2.f[0], ch, hp1);
                hp2 = biquad_step(&xover_hp2.f[1], ch, hp2);

                int32_t lo = compress_band(bc[0], bs[0], lp1, ch, frac_bits);
                int32_t mi = compress_band(bc[1], bs[1], lp2, ch, frac_bits);
                int32_t hi = compress_band(bc[2], bs[2], hp2, ch, frac_bits);

                if (ch == 0) outL = lo + mi + hi;
                else         outR = lo + mi + hi;
            }
            if (num_ch == 1) outR = outL;
        }

        if (output_gain != UNITY)
        {
            outL = FRACMUL_SHL(outL, output_gain, 7);
            outR = FRACMUL_SHL(outR, output_gain, 7);
        }

        out_l[s] = outL;
        if (num_ch > 1)
            out_r[s] = outR;
    }
}

static bool mbc3band_update(struct dsp_config *dsp,
                             const struct mbc3band_settings *settings)
{
    if (!settings->enabled)
        return false;

    int32_t fs = dsp_get_output_frequency(dsp);
    if (fs <= 0)
        return false;

    curr_set = *settings;

    static const int ratio_down_map[] = {100, 200, 400, 600, 1000, 0};
    static const int ratio_up_map[]   = {100, 25, 50, 75};

    for (int b = 0; b < MBC_NUM_BANDS; b++)
    {
        int rd = curr_set.bands[b].ratio_down;
        curr_set.bands[b].ratio_down = (rd >= 0 && rd < 6)
            ? ratio_down_map[rd] : 100;
        int ru = curr_set.bands[b].ratio_up;
        curr_set.bands[b].ratio_up = (ru >= 0 && ru < 4)
            ? ratio_up_map[ru] : 100;
    }

    if (settings->mode >= MBC_MODE_2BAND)
    {
        setup_xover(&xover_lp, settings->crossover_low, fs, true);
        setup_xover(&xover_hp, settings->crossover_low, fs, false);
    }

    if (settings->mode >= MBC_MODE_3BAND)
    {
        setup_xover(&xover_lp2, settings->crossover_high, fs, true);
        setup_xover(&xover_hp2, settings->crossover_high, fs, false);
    }

    for (int b = 0; b < MBC_NUM_BANDS; b++)
        build_gain_table(&band_coeffs[b], &settings->bands[b], fs);

    if (settings->output_gain != 0)
    {
        int32_t db_d10 = settings->output_gain;
        int32_t db_int  = db_d10 / 10;
        int32_t db_frac = db_d10 % 10;
        int32_t db_s16 = (db_int << 16) + (db_frac * 6554);
        output_gain = fp_factor(db_s16, 16) << 8;
    }
    else
    {
        output_gain = UNITY;
    }

    return true;
}

static intptr_t mbc3band_configure(struct dsp_proc_entry *this,
                                    struct dsp_config *dsp,
                                    unsigned int setting,
                                    intptr_t value)
{
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value != 0)
            break;
        this->process = mbc3band_process;
        mbc3band_update(dsp, &curr_set);
        /* fall through */

    case DSP_RESET:
    case DSP_FLUSH:
        for (int b = 0; b < MBC_NUM_BANDS; b++)
            for (int ch = 0; ch < MAX_CH; ch++)
                band_states[b].envelope[ch] = UNITY;
        if (curr_set.mode >= MBC_MODE_2BAND)
        {
            flush_xover(&xover_lp);
            flush_xover(&xover_hp);
        }
        if (curr_set.mode >= MBC_MODE_3BAND)
        {
            flush_xover(&xover_lp2);
            flush_xover(&xover_hp2);
        }
        break;

    case DSP_SET_OUT_FREQUENCY:
    case DSP_SET_FREQUENCY:
        mbc3band_update(dsp, &curr_set);
        break;
    }

    return 0;
}

void dsp_set_mbc3band(const struct mbc3band_settings *settings)
{
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    bool enable = mbc3band_update(dsp, settings);
    dsp_proc_enable(dsp, DSP_PROC_MBC3BAND, enable);
    dsp_proc_activate(dsp, DSP_PROC_MBC3BAND, true);
}

DSP_PROC_DB_ENTRY(MBC3BAND, mbc3band_configure);
