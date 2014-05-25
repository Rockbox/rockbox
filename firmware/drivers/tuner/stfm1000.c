/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Bertrik Sikken
 * Copyright (C) 2012 by Amaury Pouly
 * Based on linux driver
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
#include "system.h"
#include <string.h>
#include "kernel.h"
#include "power.h"
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h"
#include "fmradio_i2c.h" /* physical interface driver */
#include "stfm1000.h"
#include "stfm1000-regs.h"
#include "dri-imx233.h"

#include "stfm1000-tunetable.c"

static int rev_id;
static bool tuner_present = false;

#define STFM100_I2C_ADDR    0xc0

static int stfm1000_read(uint8_t reg, uint32_t *val)
{
    uint8_t buf[4];
    buf[0] = reg;
    int ret = fmradio_i2c_write(STFM100_I2C_ADDR, buf, 1);
    if(ret < 0) return ret;
    ret = fmradio_i2c_read(STFM100_I2C_ADDR, buf, 4);
    *val = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
    return ret;
}

static int stfm1000_write(uint8_t reg, uint32_t val)
{
    uint8_t buf[5];
    buf[0] = reg;
    buf[1] = val & 0xff; buf[2] = (val >> 8) & 0xff;
    buf[3] = (val >> 16) & 0xff; buf[4] = (val >> 24) & 0xff;
    return fmradio_i2c_write(STFM100_I2C_ADDR, buf, 5);
}

static int stfm1000_write_masked(int reg, int val, int mask)
{
    uint32_t tmp;

    stfm1000_read(reg, &tmp);
    stfm1000_write(reg, (val & mask) | (tmp & ~mask));
    return 0;
}

static int stfm1000_set_bits(int reg, uint32_t value)
{
    return stfm1000_write_masked(reg, value, value);
}

static int stfm1000_clear_bits(int reg, uint32_t value)
{
    return stfm1000_write_masked(reg, ~value, value);
}

static void stfm1000_set_region(int region)
{
    const struct fm_region_data *rd = &fm_region_data[region];

    if(rd->deemphasis == 50)
        stfm1000_set_bits(STFM1000_INITIALIZATION2, STFM1000_DEEMPH_50_75B);
    else
        stfm1000_clear_bits(STFM1000_INITIALIZATION2, STFM1000_DEEMPH_50_75B);
}


static int stfm1000_set_frequency(int freq)
{
    uint32_t freq100 = freq / 100;
    int if_freq = 0;
    int mix_reg = 1;
    switch(mix_reg)
    {
        case 0: if_freq = -2; break;
        case 1: if_freq = -1; break;
        case 2: if_freq =  0; break;
        case 3: if_freq =  1; break;
        case 4: if_freq =  2; break;
    }

    int fe_freq = freq100 + if_freq;

    /* clamp into range */
    if(fe_freq < STFM1000_FREQUENCY_100KHZ_MIN)
        fe_freq = STFM1000_FREQUENCY_100KHZ_MIN;
    else if(fe_freq > STFM1000_FREQUENCY_100KHZ_MAX)
        fe_freq = STFM1000_FREQUENCY_100KHZ_MAX;

    const struct stfm1000_tune1 * tp =
        &stfm1000_tune1_table[fe_freq - STFM1000_FREQUENCY_100KHZ_MIN];

    /* bits [14:0], [20:18] */
    uint32_t tune1 = (tp->tune1 & 0x7fff) | (mix_reg << 18);
    uint32_t sdnom = tp->sdnom;

    uint32_t agc1 = (rev_id == STFM1000_CHIP_REV_TA2) ? 0x0400 : 0x2200;

    int ret = stfm1000_write_masked(STFM1000_AGC_CONTROL1, agc1, 0x3f00);
    if(ret != 0)
        goto err;

    ret = stfm1000_write_masked(STFM1000_TUNE1, tune1, 0xFFFF7FFF); /* do not set bit-15 */
    if(ret != 0)
        goto err;

    ret = stfm1000_write(STFM1000_SDNOMINAL, sdnom);
    if(ret != 0)
        goto err;

    /* fix for seek-not-stopping on alternate tunings */
    ret = stfm1000_set_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
    if(ret != 0)
        goto err;

    ret = stfm1000_clear_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
    if(ret != 0)
        goto err;

    ret = stfm1000_set_bits(STFM1000_INITIALIZATION2, STFM1000_DRI_CLK_EN);
    if(ret != 0)
        goto err;

    int i2s_clock;
    /* 6MHz spur fix */
    if((freq100 >=  778 && freq100 <=  782) ||
            (freq100 >=  838 && freq100 <=  842) ||
            (freq100 >=  898 && freq100 <=  902) ||
            (freq100 >=  958 && freq100 <=  962) ||
            (freq100 >= 1018 && freq100 <= 1022) ||
            (freq100 >= 1078 && freq100 <= 1080))
        i2s_clock = 5;  /* 4.8MHz */
    else
        i2s_clock = 4;

    ret = stfm1000_write_masked(STFM1000_DATAPATH,
        STFM1000_SAI_CLK_DIV(i2s_clock), STFM1000_SAI_CLK_DIV_MASK);
    if(ret != 0)
        goto err;

    ret = stfm1000_set_bits(STFM1000_INITIALIZATION2, STFM1000_DRI_CLK_EN);
    if(ret != 0)
        goto err;

    if(tune1 & 0xf)
        ret = stfm1000_set_bits(STFM1000_CLK1, STFM1000_ENABLE_TAPDELAYFIX);
    else
        ret = stfm1000_clear_bits(STFM1000_CLK1, STFM1000_ENABLE_TAPDELAYFIX);

    if(ret != 0)
        goto err;

    int tune_cap = 4744806 - (4587 * freq100);
    if(tune_cap < 4)
        tune_cap = 4;
    ret = stfm1000_write_masked(STFM1000_LNA, STFM1000_ANTENNA_TUNECAP(tune_cap),
        STFM1000_ANTENNA_TUNECAP_MASK);
    if(ret != 0)
        goto err;

    stfm1000_set_bits(STFM1000_INITIALIZATION2, STFM1000_RDS_ENABLE);

    return 0;
err:
    return -1;
}

void stfm1000_dbg_info(struct stfm1000_dbg_info *nfo)
{
    memset(nfo, 0, sizeof(struct stfm1000_dbg_info));
    stfm1000_read(STFM1000_TUNE1, &nfo->tune1);
    stfm1000_read(STFM1000_SDNOMINAL, &nfo->sdnominal);
    stfm1000_read(STFM1000_PILOTTRACKING, &nfo->pilottracking);
    stfm1000_read(STFM1000_RSSI_TONE, &nfo->rssi_tone);
    stfm1000_read(STFM1000_PILOTCORRECTION, &nfo->pilotcorrection);
    stfm1000_read(STFM1000_CHIPID, &nfo->chipid);
    for(int i = 0; i < 6; i++)
        stfm1000_read(STFM1000_INITIALIZATION1 + i * 4, &nfo->initialization[i]);
}

static void stfm1000_dp_enable(bool enable)
{
    if(enable)
    {
        stfm1000_set_bits(STFM1000_DATAPATH, STFM1000_DP_EN);
        sleep(1);
        stfm1000_set_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
        stfm1000_clear_bits(STFM1000_AGC_CONTROL1, STFM1000_B2_BYPASS_AGC_CTL);
        stfm1000_clear_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
    }
    else
    {
        stfm1000_set_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
        stfm1000_clear_bits(STFM1000_DATAPATH, STFM1000_DP_EN);
        stfm1000_set_bits(STFM1000_AGC_CONTROL1, STFM1000_B2_BYPASS_AGC_CTL);
        stfm1000_clear_bits(STFM1000_PILOTTRACKING, STFM1000_B2_PILOTTRACKING_EN);
        stfm1000_clear_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
    }
}

static void stfm1000_dri_enable(bool enable)
{
    if(enable)
        stfm1000_set_bits(STFM1000_DATAPATH, STFM1000_SAI_EN);
    else
        stfm1000_clear_bits(STFM1000_DATAPATH, STFM1000_SAI_EN);
}

static int stfm1000_set_channel_filter(void)
{
    /* get near channel amplitude */
    int ret = stfm1000_write_masked(STFM1000_INITIALIZATION3,
        STFM1000_B2_NEAR_CHAN_MIX(0x01),
        STFM1000_B2_NEAR_CHAN_MIX_MASK);
    if(ret != 0)
        return ret;

    sleep(10 * HZ / 1000); /* wait for the signal quality to settle */

    int bypass_setting = 0;

#if 0
    ret = stfm1000_read(STFM1000_SIGNALQUALITY, &tmp);
    if(ret != 0)
        return ret;

    sig_qual = (tmp & STFM1000_NEAR_CHAN_AMPLITUDE_MASK) >>
        STFM1000_NEAR_CHAN_AMPLITUDE_SHIFT;

    /* check near channel amplitude vs threshold */
    if(sig_qual < stfm1000->adj_chan_th)
    {
        /* get near channel amplitude again */
        ret = stfm1000_write_masked(stfm1000, STFM1000_INITIALIZATION3,
            STFM1000_B2_NEAR_CHAN_MIX(0x05),
            STFM1000_B2_NEAR_CHAN_MIX_MASK);
        if(ret != 0)
            return ret;

        msleep(10); /* wait for the signal quality to settle */

        ret = stfm1000_read(stfm1000, STFM1000_SIGNALQUALITY, &tmp);
        if(ret != 0)
            return ret;

        sig_qual = (tmp & STFM1000_NEAR_CHAN_AMPLITUDE_MASK) >>
            STFM1000_NEAR_CHAN_AMPLITUDE_SHIFT;

        if(sig_qual < stfm1000->adj_chan_th)
            bypass_setting = 2;
    }
#endif
    /* set filter settings */
    ret = stfm1000_write_masked(STFM1000_INITIALIZATION1,
        STFM1000_B2_BYPASS_FILT(bypass_setting),
        STFM1000_B2_BYPASS_FILT_MASK);

    return ret;
}

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

static int stfm1000_track_pilot(void)
{
    static const struct
    {
        int delay;
        uint32_t value;
    }track_table[] =
    {
        { .delay = 10, .value = 0x81b6 },
        { .delay =  6, .value = 0x82a5 },
        { .delay =  6, .value = 0x8395 },
        { .delay =  8, .value = 0x8474 },
        { .delay = 20, .value = 0x8535 },
        { .delay = 50, .value = 0x8632 },
        { .delay =  0, .value = 0x8810 },
    };

    for(unsigned i = 0; i < ARRAY_SIZE(track_table); i++)
    {
        int ret = stfm1000_write(STFM1000_PILOTTRACKING, track_table[i].value);
        if(ret != 0)
            return ret;

        if(i < ARRAY_SIZE(track_table) - 1)      /* last one no delay */
            sleep(track_table[i].delay * HZ / 1000);
    }

    return 0;
}

static int stfm1000_optimise_channel(void)
{
    int ret;

      ret = stfm1000_set_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
      if(ret != 0)
            return ret;

      ret = stfm1000_write(STFM1000_PILOTTRACKING,
            STFM1000_B2_PILOTTRACKING_EN |
            STFM1000_B2_PILOTLPF_TIMECONSTANT(0x01) |
            STFM1000_B2_PFDSCALE(0x0B) |
            STFM1000_B2_PFDFILTER_SPEEDUP(0x06));     /* 0x000081B6 */
      if(ret != 0)
            return ret;

      ret = stfm1000_set_channel_filter();
      if(ret != 0)
            return ret;
#if 0
      ret = SD_Look_For_Pilot(stfm1000);
      if (ret != 0)
            return ret;
#endif
//      if (stfm1000->pilot_present) {
            ret = stfm1000_track_pilot();
            if (ret != 0)
                  return ret;
//      }

      ret = stfm1000_clear_bits(STFM1000_DATAPATH, STFM1000_DB_ACCEPT);
      if(ret != 0)
            return ret;

      return 0;
}

void stfm1000_init(void)
{
    uint32_t val;

    /* get revision id */
    stfm1000_read(STFM1000_CHIPID, &val);
    rev_id = val & 0xFF;

    /* send TB2 init sequence */
    stfm1000_write(STFM1000_REF, 0x00200000);
    sleep(20 * HZ / 1000);
    stfm1000_write(STFM1000_DATAPATH, 0x00010210);
    stfm1000_write(STFM1000_TUNE1, 0x0004CF01);
    stfm1000_write(STFM1000_SDNOMINAL, 0x1C5EBCF0);
    stfm1000_write(STFM1000_PILOTTRACKING, 0x000001B6);
    stfm1000_write(STFM1000_INITIALIZATION1, 0x9fb80008);
    stfm1000_write(STFM1000_INITIALIZATION2, 0x8516e444 | STFM1000_DEEMPH_50_75B);
    stfm1000_write(STFM1000_INITIALIZATION3, 0x1402190b);
    stfm1000_write(STFM1000_INITIALIZATION4, 0x525bf052);
    stfm1000_write(STFM1000_INITIALIZATION5, 0x1000d106);
    stfm1000_write(STFM1000_INITIALIZATION6, 0x000062cb);
    stfm1000_write(STFM1000_AGC_CONTROL1, 0x1BCB2202);
    stfm1000_write(STFM1000_AGC_CONTROL2, 0x000020F0);
    stfm1000_write(STFM1000_CLK1, 0x10000000);
    stfm1000_write(STFM1000_CLK1, 0x20000000);
    stfm1000_write(STFM1000_CLK1, 0x00000000);
    stfm1000_write(STFM1000_CLK2, 0x7f000000);
    stfm1000_write(STFM1000_REF, 0x00B8222D);
    stfm1000_write(STFM1000_CLK1, 0x30000000);
    stfm1000_write(STFM1000_CLK1, 0x30002000);
    stfm1000_write(STFM1000_CLK1, 0x10002000);
    stfm1000_write(STFM1000_LNA, 0x0D080009);
    sleep(10 * HZ / 1000);
    stfm1000_write(STFM1000_MIXFILT, 0x00008000);
    stfm1000_write(STFM1000_MIXFILT, 0x00000000);
    stfm1000_write(STFM1000_MIXFILT, 0x00007205);
    stfm1000_write(STFM1000_ADC, 0x001B3282);
    stfm1000_write(STFM1000_ATTENTION, 0x0000003F);

    stfm1000_dp_enable(true);
}

static void stfm1000_sleep(bool sleep)
{
    (void)sleep;
    /* no implementation yet */
}

static int stfm1000_rssi(void)
{
    uint32_t rssi_dc_est;
    int rssi_mantissa, rssi_exponent, rssi_decoded;
    int prssi;
    int rssi_log;

    stfm1000_read(STFM1000_RSSI_TONE, &rssi_dc_est);

    rssi_mantissa = (rssi_dc_est & 0xffe0) >> 5;    /* 11Msb */
    rssi_exponent = rssi_dc_est & 0x001f;     /* 5 lsb */
    rssi_decoded = (uint32_t)rssi_mantissa << rssi_exponent;

    /* Convert Rsst to 10log(Rssi) */
    for(prssi = 20; prssi > 0; prssi--)
        if(rssi_decoded >= (1 << prssi))
            break;

    rssi_log = (3 * rssi_decoded >> prssi) + (3 * prssi - 3);

    /* clamp to positive */
    if(rssi_log < 0)
        rssi_log = 0;

    /* Compensate for errors in truncation/approximation by adding 1 */
    rssi_log++;

    return rssi_log;
}

static bool stfm1000_tuned(void)
{
    uint32_t tmp;
    int pilot;

    /* get pilot */
    stfm1000_read(STFM1000_PILOTCORRECTION, &tmp);
    pilot = (tmp & STFM1000_PILOTEST_TB2_MASK) >> STFM1000_PILOTEST_TB2_SHIFT;

    /* check pilot and signal strength */
    return (pilot >= 0x1E) && (pilot < 0x80) && (stfm1000_rssi() > 28);
}

int stfm1000_set(int setting, int value)
{
    int val = 0;

    switch (setting)
    {
    case RADIO_SLEEP:
        stfm1000_sleep(value);
        break;

    case RADIO_FREQUENCY:
        stfm1000_dri_enable(false);
        imx233_dri_enable(false);
        imx233_dri_enable(true);
        stfm1000_dri_enable(true);
        stfm1000_set_frequency(value / 1000);
        stfm1000_optimise_channel();
        break;
    case RADIO_SCAN_FREQUENCY:
        stfm1000_set_frequency(value / 1000);
        val = stfm1000_tuned();
        break;
    case RADIO_MUTE:
        /* no implementation yet */
        break;
    case RADIO_REGION:
        stfm1000_set_region(value);
        break;
    case RADIO_FORCE_MONO:
        /* no implementation yet */
        break;
    default:
        val = -1;
        break;
    }

    return val;
}

int stfm1000_get(int setting)
{
    int val = -1; /* default for unsupported query */

    switch (setting)
    {
    case RADIO_PRESENT:
        val = true;
        break;
    case RADIO_TUNED:
        val = stfm1000_tuned();
        break;
    case RADIO_STEREO:
        val = 0;
        break;
    case RADIO_RSSI:
        val = stfm1000_rssi();
        break;
    case RADIO_RSSI_MIN:
        val = 0;
        break;
    case RADIO_RSSI_MAX:
        val = 70;
        break;
    }

    return val;
}
