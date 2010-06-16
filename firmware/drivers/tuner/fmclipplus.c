/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Tuner "middleware" for unidentified Silicon Labs chip present in some
 * Sansa Clip+ players
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
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h"
#include "fmradio_i2c.h" /* physical interface driver */

#define SEEK_THRESHOLD 0x10

#define I2C_ADR 0x20

/** Registers and bits **/
#define POWERCFG    0x2
#define CHANNEL     0x3
#define SYSCONFIG1  0x4
#define SYSCONFIG2  0x5
#define SYSCONFIG3  0x6

#define READCHAN    0xA
#define STATUSRSSI  0xB
#define IDENT       0xC


/* POWERCFG (0x2) */
#define POWERCFG_DMUTE      (0x1 << 14)
#define POWERCFG_MONO       (0x1 << 13)
#define POWERCFG_ENABLE     (0x1 <<  0)

/* CHANNEL (0x3) */
#define CHANNEL_CHAN        (0x3ff << 6)
    #define CHANNEL_CHANw(x) (((x) << 6) & CHANNEL_CHAN)
#define CHANNEL_TUNE        (0x1 << 4)
#define CHANNEL_BAND        (0x3 << 2)
    #define CHANNEL_BANDw(x)        (((x) << 2) & CHANNEL_BAND)
    #define CHANNEL_BANDr(x)        (((x) & CHANNEL_BAND) >> 2)
    #define CHANNEL_BAND_875_1080   (0x0 << 2) /* tenth-megahertz */
    #define CHANNEL_BAND_760_1080   (0x1 << 2)
    #define CHANNEL_BAND_760_900    (0x2 << 2)
#define CHANNEL_SPACE       (0x3 << 0)
    #define CHANNEL_SPACEw(x)  (((x) << 0) & CHANNEL_SPACE)
    #define CHANNEL_SPACEr(x)  (((x) & CHANNEL_SPACE) >> 0)
    #define CHANNEL_SPACE_200KHZ    (0x0 << 0)
    #define CHANNEL_SPACE_100KHZ    (0x1 << 0)
    #define CHANNEL_SPACE_50KHZ     (0x2 << 0)

/* SYSCONFIG1 (0x4) */
#define SYSCONFIG1_DE       (0x1 << 11)

/* READCHAN (0xA) */
#define READCHAN_READCHAN   (0x3ff << 0)
    #define READCHAN_READCHANr(x) (((x) & READCHAN_READCHAN) >> 0)
#define READCHAN_STC        (0x1 << 14)
#define READCHAN_ST         (0x1 << 10)

/* STATUSRSSI (0xB) */
#define STATUSRSSI_RSSI     (0x3F << 10)
    #define STATUSRSSI_RSSIr(x) (((x) & STATUSRSSI_RSSI) >> 10)
#define STATUSRSSI_AFCRL    (0x1 << 8)

static const uint16_t initvals[32] = {
    0x8110, 0x4580, 0xC401, 0x1B90, 
    0x0400, 0x866F, 0x8000, 0x4712, 
    0x5EC6, 0x0000, 0x406E, 0x2D80, 
    0x5803, 0x5804, 0x5804, 0x5804,
    
    0x0047, 0x9000, 0xF587, 0x0009, 
    0x00F1, 0x41C0, 0x41E0, 0x506F,
    0x5592, 0x007D, 0x10A0, 0x0780,
    0x311D, 0x4006, 0x1F9B, 0x4C2B
};

static bool tuner_present = false;
static int curr_frequency = 87500000; /* Current station frequency (HZ) */
static uint16_t cache[32];

/* reads <len> registers from radio at offset 0x0A into cache */
static void fmclipplus_read(int len)
{
    int i;
    unsigned char buf[64];
    unsigned char *ptr = buf;
    uint16_t data;
    
    fmradio_i2c_read(I2C_ADR, buf, len * 2);
    for (i = 0; i < len; i++) {
        data = ptr[0] << 8 | ptr[1];
        cache[(i + READCHAN) & 0x1F] = data;
        ptr += 2;
    }
}

/* writes <len> registers from cache to radio at offset 0x02 */
static void fmclipplus_write(int len)
{
    int i;
    unsigned char buf[64];
    unsigned char *ptr = buf;
    uint16_t data;

    for (i = 0; i < len; i++) {
        data = cache[(i + POWERCFG) & 0x1F];
        *ptr++ = (data >> 8) & 0xFF;
        *ptr++ = data & 0xFF;
    }
    fmradio_i2c_write(I2C_ADR, buf, len * 2);
}

static uint16_t fmclipplus_read_reg(int reg)
{
    fmclipplus_read(((reg - READCHAN) & 0x1F) + 1);
    return cache[reg];
}

static void fmclipplus_write_reg(int reg, uint16_t value)
{
    cache[reg] = value;
}

static void fmclipplus_write_cache(void)
{
    fmclipplus_write(5);
}

static void fmclipplus_write_masked(int reg, uint16_t bits, uint16_t mask)
{
    fmclipplus_write_reg(reg, (cache[reg] & ~mask) | (bits & mask));
}

static void fmclipplus_write_clear(int reg, uint16_t mask)
{
    fmclipplus_write_reg(reg, cache[reg] & ~mask);
}

static void fmclipplus_sleep(int snooze)
{
    if (snooze) {
        fmclipplus_write_masked(POWERCFG, 0, 0xFF);
    }
    else {
        fmclipplus_write_masked(POWERCFG, 1, 0xFF);
    }
    fmclipplus_write_cache();
}

bool fmclipplus_detect(void)
{
    return ((fmclipplus_read_reg(IDENT) & 0xFF00) == 0x5800);
 }

void fmclipplus_init(void)
{
    if (fmclipplus_detect()) {
        tuner_present = true;

        // send pre-initialisation value
        fmclipplus_write_reg(POWERCFG, 0x200);
        fmclipplus_write(2);
        sleep(HZ * 10 / 100);

        // write initialisation values
        memcpy(cache, initvals, sizeof(cache));
        fmclipplus_write(32);
        sleep(HZ * 70 / 1000);
    }
}

static void fmclipplus_set_frequency(int freq)
{
    int i;

    /* check BAND and spacings */
    fmclipplus_read_reg(STATUSRSSI);
    int start = CHANNEL_BANDr(cache[CHANNEL]) & 1 ? 76000000 : 87000000;
    int chan = (freq - start) / 50000;

    curr_frequency = freq;

    for (i = 0; i < 5; i++) {
        /* tune and wait a bit */
        fmclipplus_write_masked(CHANNEL, CHANNEL_CHANw(chan) | CHANNEL_TUNE,
                                CHANNEL_CHAN | CHANNEL_TUNE);
        fmclipplus_write_cache();
        sleep(HZ * 70 / 1000);
        fmclipplus_write_clear(CHANNEL, CHANNEL_TUNE);
        fmclipplus_write_cache();
        
        /* check if tuning was successful */
        fmclipplus_read_reg(STATUSRSSI);
        if (cache[READCHAN] & READCHAN_STC) {
            if (READCHAN_READCHANr(cache[READCHAN]) == chan) {
                break;
            }
        }
    }
}

static int fmclipplus_tuned(void)
{
    /* Primitive tuning check: sufficient level and AFC not railed */
    uint16_t status = fmclipplus_read_reg(STATUSRSSI);
    if (STATUSRSSI_RSSIr(status) >= SEEK_THRESHOLD &&
        (status & STATUSRSSI_AFCRL) == 0) {
        return 1;
    }

    return 0;
}

static void fmclipplus_set_region(int region)
{
    const struct fmclipplus_region_data *rd = &fmclipplus_region_data[region];
    uint16_t bandspacing = CHANNEL_BANDw(rd->band) |
                           CHANNEL_SPACEw(CHANNEL_SPACE_50KHZ);
    uint16_t oldbs = cache[CHANNEL] & (CHANNEL_BAND | CHANNEL_SPACE);

    fmclipplus_write_masked(SYSCONFIG1, rd->deemphasis ? SYSCONFIG1_DE : 0,
                        SYSCONFIG1_DE);
    fmclipplus_write_masked(CHANNEL, bandspacing, CHANNEL_BAND | CHANNEL_SPACE);
    fmclipplus_write_cache();

    /* Retune if this region change would change the channel number. */
    if (oldbs != bandspacing) {
        fmclipplus_set_frequency(curr_frequency);
    }
}

static bool fmclipplus_st(void)
{
    return (fmclipplus_read_reg(READCHAN) & READCHAN_ST);
}

/* tuner abstraction layer: set something to the tuner */
int fmclipplus_set(int setting, int value)
{
    switch (setting) {
    case RADIO_SLEEP:
        if (value != 2) {
            fmclipplus_sleep(value);
        }
        break;

    case RADIO_FREQUENCY:
        fmclipplus_set_frequency(value);
        break;

    case RADIO_SCAN_FREQUENCY:
        fmclipplus_set_frequency(value);
        return fmclipplus_tuned();

    case RADIO_MUTE:
        fmclipplus_write_masked(POWERCFG, value ? 0 : POWERCFG_DMUTE,
                                POWERCFG_DMUTE);
        fmclipplus_write_masked(SYSCONFIG1, (3 << 9), (3 << 9));
        fmclipplus_write_masked(SYSCONFIG2, (0xF << 0), (0xF << 0));
        fmclipplus_write_cache();
        break;

    case RADIO_REGION:
        fmclipplus_set_region(value);
        break;

    case RADIO_FORCE_MONO:
        fmclipplus_write_masked(POWERCFG, value ? POWERCFG_MONO : 0,
                            POWERCFG_MONO);
        fmclipplus_write_cache();
        break;

    default:
        return -1;
    }

    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int fmclipplus_get(int setting)
{
    int val = -1; /* default for unsupported query */

    switch (setting) {
    case RADIO_PRESENT:
        val = tuner_present ? 1 : 0;
        break;

    case RADIO_TUNED:
        val = fmclipplus_tuned();
        break;

    case RADIO_STEREO:
        val = fmclipplus_st();
        break;
    }

    return val;
}

void fmclipplus_dbg_info(struct fmclipplus_dbg_info *nfo)
{
    fmclipplus_read(32);
    memcpy(nfo->regs, cache, sizeof (nfo->regs));
}

