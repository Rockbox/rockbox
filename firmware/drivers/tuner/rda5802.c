/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Tuner "middleware" for RDA5802 chip present in some Sansa Clip+ players
 *
 * Copyright (C) 2010 Bertrik Sikken
 * Copyright (C) 2008 Nils Wallménius (si4700 code that this was based on)
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
#include <string.h>
#include <stdlib.h>
#include "kernel.h"
#include "power.h"
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h"
#include "fmradio_i2c.h" /* physical interface driver */
#include "audio.h"
#include "backlight.h"

#define SEEK_THRESHOLD 0x16

#define I2C_ADR 0x20

/* define RSSI range */
#define RSSI_MIN 0
#define RSSI_MAX 70

/** Registers and bits **/
#define CHIPID      0x0
#define POWERCFG    0x2
#define CHANNEL     0x3
#define SYSCONFIG1  0x4
#define SYSCONFIG2  0x5
#define SYSCONFIG3  0x6
#define SYSCONFIG4  0x7
#define SYSCONFIG5  0x8
#define SYSCONFIG6  0x9 /* suspected not to exists */
#define READCHAN    0xA
#define STATUSRSSI  0xB


/* POWERCFG (0x2) */
#define POWERCFG_DMUTE      (0x1 << 14)
#define POWERCFG_MONO       (0x1 << 13)
#define POWERCFG_SOFT_RESET (0x1 <<  1)
#define POWERCFG_ENABLE     (0x1 <<  0)

/* CHANNEL (0x3) */
#define CHANNEL_CHAN        (0x3ff << 6)
    #define CHANNEL_CHANw(x) (((x) << 6) & CHANNEL_CHAN)
#define CHANNEL_TUNE        (0x1 << 4)
#define CHANNEL_BAND        (0x3 << 2)
    #define CHANNEL_BANDw(x)        (((x) << 2) & CHANNEL_BAND)
    #define CHANNEL_BANDr(x)        (((x) & CHANNEL_BAND) >> 2)
    #define CHANNEL_BAND_870_1080   (0x0) /* tenth-megahertz */
    #define CHANNEL_BAND_760_1080   (0x1)
    #define CHANNEL_BAND_760_900    (0x2)
    #define CHANNEL_BAND_650_760    (0x3)
#define CHANNEL_SPACE       (0x3 << 0)
    #define CHANNEL_SPACEw(x)  (((x) << 0) & CHANNEL_SPACE)
    #define CHANNEL_SPACEr(x)  (((x) & CHANNEL_SPACE) >> 0)
    #define CHANNEL_SPACE_100KHZ    (0x0)
    #define CHANNEL_SPACE_200KHZ    (0x1)
    #define CHANNEL_SPACE_50KHZ     (0x2)

/* SYSCONFIG1 (0x4) */
#define SYSCONFIG1_DE       (0x1 << 11)
#define SYSCONFIG1_SOFTMUTE_EN  (0x1 << 9)

/* SYSCONFIG2 (0x5) */
#define SYSCONFIG2_VOLUME   (0xF <<  0)

/* READCHAN (0xA) */
#define READCHAN_READCHAN   (0x3ff << 0)
    #define READCHAN_READCHANr(x) (((x) & READCHAN_READCHAN) >> 0)
#define READCHAN_STC        (0x1 << 14)
#define READCHAN_ST         (0x1 << 10)

/* STATUSRSSI (0xB) */
#define STATUSRSSI_RSSI     (0x7F << 9)
    #define STATUSRSSI_RSSIr(x) (((x) & STATUSRSSI_RSSI) >> 9)
#define STATUSRSSI_FM_TRUE  (0x1 << 8)

static bool tuner_present = false;

static uint16_t cache[16] = {
    [POWERCFG] =    0xC401, /* DHIZ, DMUTE, CLK_DIRECT_MODE, ENABLE */
    [CHANNEL] =     0x0000, /* - */
    [SYSCONFIG1] =  0x0200, /* SYSCONFIG1_SOFTMUTE_EN */
    [SYSCONFIG2] =  0x867F, /* INT_MODE (def), SEEKTH=1100b, LNA_PORT_SEL=LNAN,
                               LNA_ICSEL=3.0mA, VOLUME=max */
    [SYSCONFIG3] =  0x8000, /* RSVD */
    [SYSCONFIG4] =  0x4712, /* TH_SOFRBLEND=34dB, 65M_50M MODE=65~76MHz, RSVD, SEEK_TH_OLD=4, SOFTBLEND_EN */
    [SYSCONFIG5] =  0x5EC6, /* FREQ_DIRECT=24262kHz (unused) */
    [SYSCONFIG6] =  0x0000  /* undocumented */
};

/* reads <len> registers from radio at offset 0x0A into cache */
static void rda5802_read(int len)
{
    int i;
    unsigned char buf[sizeof(cache)];
    unsigned char *ptr = buf;
    uint16_t data;
    
    fmradio_i2c_read(I2C_ADR, buf, len * 2);
    for (i = 0; i < len; i++) {
        data = ptr[0] << 8 | ptr[1];
        cache[READCHAN + i] = data;
        ptr += 2;
    }
}

/* writes <len> registers from cache to radio at offset 0x02 */
static void rda5802_write(int len)
{
    int i;
    unsigned char buf[sizeof(cache)];
    unsigned char *ptr = buf;
    uint16_t data;

    for (i = 0; i < len; i++) {
        data = cache[POWERCFG + i];
        *ptr++ = (data >> 8) & 0xFF;
        *ptr++ = data & 0xFF;
    }
    fmradio_i2c_write(I2C_ADR, buf, len * 2);
}

static uint16_t rda5802_read_reg(int reg)
{
    rda5802_read((reg - READCHAN) + 1);
    return cache[reg];
}

static void rda5802_write_reg(int reg, uint16_t value)
{
    cache[reg] = value;
}

static void rda5802_write_cache(void)
{
    rda5802_write(5);
}

static void rda5802_write_masked(int reg, uint16_t bits, uint16_t mask)
{
    rda5802_write_reg(reg, (cache[reg] & ~mask) | (bits & mask));
}

static void rda5802_write_clear(int reg, uint16_t mask)
{
    rda5802_write_reg(reg, cache[reg] & ~mask);
}

static void rda5802_write_set(int reg, uint16_t mask)
{
    rda5802_write_reg(reg, cache[reg] | mask);
}

static void rda5802_sleep(int snooze)
{
    if (snooze) {
        rda5802_write_clear(POWERCFG, POWERCFG_ENABLE);
    }
    else {
        tuner_power(true);
        rda5802_write_set(POWERCFG, POWERCFG_ENABLE);
    }
    rda5802_write_cache();
    if(snooze)
        tuner_power(false);
}

bool rda5802_detect(void)
{
    /* The RDA5802 has a weird wrap-around at 0x40. Upon initialisation, it will copy the Chip ID
     * to register 0xC but since this register is also used for RDS, we cannot rely on its content
     * until we softreset. But we cannot soft-reset until we confirm the tuner type... So we really
     * need to register 0, which means reading 0x40 registers.
     * NOTE: the datasheet says that it wraps around at 0x3A but this is wrong. */

    /* we want to read registers 0x00/0x01, aka 0x40/0x41 because of wrapping, tuner starts reading
     * at 0xA so we need to read 0x40 - 0xA + 2 registers, each register is two bytes. Thats
     * (0x40 - 0xA + 2) * 2 = 0x6e bytes */
    unsigned char buf[0x70];
    fmradio_i2c_read(I2C_ADR, buf, sizeof(buf));
    cache[CHIPID] = buf[0x6c] << 8 | buf[0x6d];
    cache[1] = buf[0x6e] << 8 | buf[0x6f]; /* unknown register, maybe firmware ID or related */

    return ((cache[CHIPID] & 0xFF00) == 0x5800);
 }

void rda5802_init(void)
{
    if (rda5802_detect()) {
        tuner_present = true;

        /* soft-reset */
        rda5802_write_set(POWERCFG, POWERCFG_SOFT_RESET);
        rda5802_write(1);
        rda5802_write_clear(POWERCFG, POWERCFG_SOFT_RESET);
        sleep(HZ * 10 / 1000);

        /* write initialisation values */
        rda5802_write(8);
        sleep(HZ * 70 / 1000);
    }
}

static void rda5802_set_frequency(int freq)
{
    int i;
    uint16_t readchan;
    static const int spacings[] = {100000, 200000, 50000, 50000};
    static const int bandstart[] = {87000000, 76000000, 76000000, 65000000};

    /* calculate channel number */
    int start = bandstart[CHANNEL_BANDr(cache[CHANNEL])];
    int space = spacings[CHANNEL_SPACEr(cache[CHANNEL])];
    int chan = (freq - start) / space;

    for (i = 0; i < 5; i++) {
        /* tune and wait a bit */
        rda5802_write_masked(CHANNEL, CHANNEL_CHANw(chan) | CHANNEL_TUNE,
                                CHANNEL_CHAN | CHANNEL_TUNE);
        rda5802_write_cache();
        sleep(HZ * 70 / 1000);
        rda5802_write_clear(CHANNEL, CHANNEL_TUNE);
        rda5802_write_cache();
        
        /* check if tuning was successful */
        readchan = rda5802_read_reg(READCHAN);
        if (readchan & READCHAN_STC) {
            if (READCHAN_READCHANr(readchan) == chan) {
                break;
            }
        }
    }
}

static int rda5802_tuned(void)
{
    /* Primitive tuning check: sufficient level and AFC not railed */
    uint16_t status = rda5802_read_reg(STATUSRSSI);
    if (STATUSRSSI_RSSIr(status) >= SEEK_THRESHOLD &&
        (status & STATUSRSSI_FM_TRUE)) {
        return 1;
    }

    return 0;
}

static void rda5802_set_region(int region)
{
    const struct fm_region_data *rd = &fm_region_data[region];

    int band = (rd->freq_min == 76000000) ?
               CHANNEL_BAND_760_900 : CHANNEL_BAND_870_1080;
    int deemphasis = (rd->deemphasis == 50) ? SYSCONFIG1_DE : 0;
    int space = (rd->freq_step == 50000) ?
                CHANNEL_SPACE_50KHZ : CHANNEL_SPACE_100KHZ;

    rda5802_write_masked(SYSCONFIG1, deemphasis, SYSCONFIG1_DE);
    rda5802_write_masked(CHANNEL, CHANNEL_BANDw(band), CHANNEL_BAND);
    rda5802_write_masked(CHANNEL, CHANNEL_SPACEw(space), CHANNEL_SPACE);
    rda5802_write_cache();
}

static bool rda5802_st(void)
{
    return (rda5802_read_reg(READCHAN) & READCHAN_ST);
}

static int rda5802_rssi(void)
{
    uint16_t status = rda5802_read_reg(STATUSRSSI);
    return STATUSRSSI_RSSIr(status);
}

/* tuner abstraction layer: set something to the tuner */
int rda5802_set(int setting, int value)
{
    switch (setting) {
    case RADIO_SLEEP:
        rda5802_sleep(value);
        break;

    case RADIO_FREQUENCY:
        rda5802_set_frequency(value);
        break;

    case RADIO_SCAN_FREQUENCY:
        rda5802_set_frequency(value);
        return rda5802_tuned();

    case RADIO_MUTE:
        rda5802_write_masked(POWERCFG, value ? 0 : POWERCFG_DMUTE,
                                POWERCFG_DMUTE);
        rda5802_write_cache();
        break;

    case RADIO_REGION:
        rda5802_set_region(value);
        break;

    case RADIO_FORCE_MONO:
        rda5802_write_masked(POWERCFG, value ? POWERCFG_MONO : 0,
                            POWERCFG_MONO);
        rda5802_write_cache();
        break;

    default:
        return -1;
    }

    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int rda5802_get(int setting)
{
    int val = -1; /* default for unsupported query */

    switch (setting) {
    case RADIO_PRESENT:
        val = tuner_present ? 1 : 0;
        break;

    case RADIO_TUNED:
        val = ((audio_status() & AUDIO_STATUS_RECORD) || !is_backlight_on(true)) ? 1 : rda5802_tuned();
        break;

    case RADIO_STEREO:
        val = ((audio_status() & AUDIO_STATUS_RECORD) || !is_backlight_on(true)) ? 1 : rda5802_st();
        break;

    case RADIO_RSSI:
        val = ((audio_status() & AUDIO_STATUS_RECORD) || !is_backlight_on(true)) ? RSSI_MAX : rda5802_rssi();
        break;

    case RADIO_RSSI_MIN:
        val = RSSI_MIN;
        break;

    case RADIO_RSSI_MAX:
        val = RSSI_MAX;
        break;
    }

    return val;
}

void rda5802_dbg_info(struct rda5802_dbg_info *nfo)
{
    rda5802_read(6);
    memcpy(nfo->regs, cache, sizeof (nfo->regs));
}

