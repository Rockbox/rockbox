/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Tuner "middleware" for Silicon Labs SI4700 chip
 *
 * Copyright (C) 2008 Nils Wallménius
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

/* some models use the internal 32 kHz oscillator which needs special attention
   during initialisation, power-up and power-down.
*/
#if defined(SANSA_CLIP) || defined(SANSA_E200V2) || defined(SANSA_FUZE)
#define USE_INTERNAL_OSCILLATOR
#elif defined(TOSHIBA_GIGABEAT_S)
#define SI4700_GPIO_SETUP (SYSCONFIG1_GPIO1_HI_Z | \
                           SYSCONFIG1_GPIO2_HI_Z | \
                           SYSCONFIG1_GPIO3_MO_ST_I)
extern int si4700_st(void);
#endif

#ifndef SI4700_GPIO_SETUP
#define SI4700_GPIO_SETUP 0
#endif

#define SEEK_THRESHOLD 0x16

#define I2C_ADR 0x20

/** Registers and bits - "x" denotes Si4702/03 only (so they say) **/
#define DEVICEID    0x0
#define CHIPID      0x1
#define POWERCFG    0x2
#define CHANNEL     0x3
#define SYSCONFIG1  0x4
#define SYSCONFIG2  0x5
#define SYSCONFIG3  0x6
#define TEST1       0x7
#define TEST2       0x8
#define BOOTCONFIG  0x9
#define STATUSRSSI  0xA
#define READCHAN    0xB
#define RDSA        0xC /* x */
#define RDSB        0xD /* x */
#define RDSC        0xE /* x */
#define RDSD        0xF /* x */

/* DEVICEID (0x0) */
#define DEVICEID_PN         (0xf << 12)
    /* 0x01 = Si4700/01 */
    /* 0x01 = Si4702/03 */
#define DEVICEID_MFGID      (0xfff << 0)
    /* always 0x242 */

/* CHIPID (0x1) */

#if 0 /* Informational */
/* Si4700/01 */
#define CHIPID_REV          (0x3f << 10)
#define CHIPID_DEV          (0x1 << 9)
    /* 0 before powerup */
    /* 0 after powerup = Si4700 */
    /* 1 after powerup = Si4701 */
#define CHIPID_FIRMWARE     (0xff << 0)

/* Si4702/03 */
#define CHIPID_REV          (0x3f << 10)
#define CHIPID_DEV          (0xf << 6)
    /* 0000 before PU = Si4702 */
    /* 0001 after PU = Si4702 */
    /* 1000 before PU = Si4703 */
    /* 1001 after PU = Si4703 */
#define CHIPID_FIRMWARE     (0x3f << 0)
#endif /* 0 */

/* POWERCFG (0x2) */
#define POWERCFG_DSMUTE     (0x1 << 15)
#define POWERCFG_DMUTE      (0x1 << 14)
#define POWERCFG_MONO       (0x1 << 13)
#define POWERCFG_RDSM       (0x1 << 11) /* x */
#define POWERCFG_SKMODE     (0x1 << 10)
#define POWERCFG_SEEKUP     (0x1 <<  9)
#define POWERCFG_SEEK       (0x1 <<  8)
#define POWERCFG_DISABLE    (0x1 <<  6)
#define POWERCFG_ENABLE     (0x1 <<  0)

/* CHANNEL (0x3) */
#define CHANNEL_TUNE        (0x1 << 15)
#define CHANNEL_CHAN        (0x3ff << 0)
    #define CHANNEL_CHANw(x) ((x) & CHANNEL_CHAN)

/* SYSCONFIG1 (0x4) */
#define SYSCONFIG1_RDSIEN   (0x1 << 15) /* x */
#define SYSCONFIG1_STCIEN   (0x1 << 14)
#define SYSCONFIG1_RDS      (0x1 << 12) /* x */
#define SYSCONFIG1_DE       (0x1 << 11)
#define SYSCONFIG1_AGCD     (0x1 << 10)
#define SYSCONFIG1_BLNDADJ  (0x3 <<  6)
    #define SYSCONFIG1_BLNDADJ_31_39_RSSI (0x0 << 6)
    #define SYSCONFIG1_BLNDADJ_37_55_RSSI (0x1 << 6)
    #define SYSCONFIG1_BLNDADJ_19_37_RSSI (0x2 << 6)
    #define SYSCONFIG1_BLNDADJ_25_43_RSSI (0x3 << 6)
#define SYSCONFIG1_GPIO3    (0x3 <<  4)
    #define SYSCONFIG1_GPIO3_HI_Z       (0x0 << 4)
    #define SYSCONFIG1_GPIO3_MO_ST_I    (0x1 << 4)
    #define SYSCONFIG1_GPIO3_LOW        (0x2 << 4)
    #define SYSCONFIG1_GPIO3_HI         (0x3 << 4)
#define SYSCONFIG1_GPIO2    (0x3 <<  2)
    #define SYSCONFIG1_GPIO2_HI_Z       (0x0 << 2)
    #define SYSCONFIG1_GPIO2_STC_RDS_I  (0x1 << 2)
    #define SYSCONFIG1_GPIO2_LOW        (0x2 << 2)
    #define SYSCONFIG1_GPIO2_HI         (0x3 << 2)
#define SYSCONFIG1_GPIO1    (0x3 <<  0)
    #define SYSCONFIG1_GPIO1_HI_Z       (0x0 << 0)
    #define SYSCONFIG1_GPIO1_LOW        (0x2 << 0)
    #define SYSCONFIG1_GPIO1_HI         (0x3 << 0)

/* SYSCONFIG2 (0x5) */
#define SYSCONFIG2_SEEKTH   (0xff << 8)
    #define SYSCONFIG2_SKEETHw(x) (((x) << 8) & SYSCONFIG2_SEEKTH)
#define SYSCONFIG2_BAND     (0x3 << 6)
    #define SYSCONFIG2_BANDw(x)   (((x) << 6) & SYSCONFIG2_BAND)
    #define SYSCONFIG2_BANDr(x)   (((x) & SYSCONFIG2_BAND) >> 6)
    #define SYSCONFIG2_BAND_875_1080    (0x0 << 6) /* tenth-megahertz */
    #define SYSCONFIG2_BAND_760_1080    (0x1 << 6)
    #define SYSCONFIG2_BAND_760_900     (0x2 << 6)
#define SYSCONFIG2_SPACE    (0x3 << 4)
    #define SYSCONFIG2_SPACEw(x)  (((x) << 4) & SYSCONFIG2_SPACE)
    #define SYSCONFIG2_SPACEr(x)  (((x) & SYSCONFIG2_SPACE) >> 4)
    #define SYSCONFIG2_SPACE_200KHZ     (0x0 << 4)
    #define SYSCONFIG2_SPACE_100KHZ     (0x1 << 4)
    #define SYSCONFIG2_SPACE_50KHZ      (0x2 << 4)
/* 4700/01            0000=mute,0001=-28dBFS..2dB steps..1111= +0dBFS */
/* 4702/03: VOLEXT=0: 0000=mute,0001=-28dBFS..2dB steps..1111= +0dBFS */
/*          VOLEXT=1: 0000=mute,0001=-58dBFS..2dB steps..1111=-30dBFS */
#define SYSCONFIG2_VOLUME   (0xf << 0)
    #define SYSCONFIG2_VOLUMEw(x) ((x) & SYSCONFIG2_VOLUME)

/* SYSCONFIG3 (0x6) */
#define SYSCONFIG3_SMUTER   (0x3 << 14)
    #define SYSCONFIG3_SMUTER_FASTEST   (0x0 << 14)
    #define SYSCONFIG3_SMUTER_FAST      (0x1 << 14)
    #define SYSCONFIG3_SMUTER_SLOW      (0x2 << 14)
    #define SYSCONFIG3_SMUTER_SLOWEST   (0x3 << 14)
#define SYSCONFIG3_SMUTEA   (0x3 << 12)
    #define SYSCONFIG3_SMUTEA_16DB      (0x0 << 12)
    #define SYSCONFIG3_SMUTEA_14DB      (0x1 << 12)
    #define SYSCONFIG3_SMUTEA_12DB      (0x2 << 12)
    #define SYSCONFIG3_SMUTEA_10DB      (0x3 << 12)
#define SYSCONFIG3_VOLEXT   (0x1 << 8) /* x */
#define SYSCONFIG3_SKSNR    (0xf << 4)
    #define SYSCONFIG3_SKSNRw(x) (((x) << 4) & SYSCONFIG3_SKSNR)
#define SYSCONFIG3_SKCNT    (0xf << 0)
    #define SYSCONFIG3_SKCNTw(x) (((x) << 0) & SYSCONFIG3_SKCNT)

/* TEST1 (0x7) */
/* 4700/01: 15=always 0, 13:0 = write with preexisting values! */
/* 4702/03:              13:0 = write with preexisting values! */
#define TEST1_XOSCEN        (0x1 << 15) /* x */
#define TEST1_AHIZEN        (0x1 << 14)

/* TEST2 (0x8) */
/* 15:0 = write with preexisting values! */

/* BOOTCONFIG (0x9) */
/* 15:0 = write with preexisting values! */

/* STATUSRSSI (0xA) */
#define STATUSRSSI_RDSR     (0x1 << 15) /* x */
#define STATUSRSSI_STC      (0x1 << 14)
#define STATUSRSSI_SFBL     (0x1 << 13)
#define STATUSRSSI_AFCRL    (0x1 << 12)
#define STATUSRSSI_RDSS     (0x1 << 11) /* x */
#define STATUSRSSI_BLERA    (0x3 <<  9) /* x */
#define STATUSRSSI_ST       (0x1 <<  8)
#define STATUSRSSI_RSSI     (0xff << 0)
    #define STATUSRSSI_RSSIr(x) ((x) & 0xff)

/* READCHAN (0xB) */
#define READCHAN_BLERB      (0x3 << 14) /* x */
#define READCHAN_BLERC      (0x3 << 12) /* x */
#define READCHAN_BLERD      (0x3 << 10) /* x */
#define READCHAN_READCHAN   (0x3ff << 0)

/* RDSA-D (0xC-0xF) */
/* 4702/03: RDS Block A-D data */

static bool tuner_present = false;
static int curr_frequency = 87500000; /* Current station frequency (HZ) */
static uint16_t cache[16];

/* reads <len> registers from radio at offset 0x0A into cache */
static void si4700_read(int len)
{
    int i;
    unsigned char buf[32];
    unsigned char *ptr = buf;
    uint16_t data;
    
    fmradio_i2c_read(I2C_ADR, buf, len * 2);
    for (i = 0; i < len; i++) {
        data = ptr[0] << 8 | ptr[1];
        cache[(i + STATUSRSSI) & 0xF] = data;
        ptr += 2;
    }
}

/* writes <len> registers from cache to radio at offset 0x02 */
static void si4700_write(int len)
{
    int i;
    unsigned char buf[32];
    unsigned char *ptr = buf;
    uint16_t data;

    for (i = 0; i < len; i++) {
        data = cache[(i + POWERCFG) & 0xF];
        *ptr++ = (data >> 8) & 0xFF;
        *ptr++ = data & 0xFF;
    }
    fmradio_i2c_write(I2C_ADR, buf, len * 2);
}

/* Hide silly, wrapped and continuous register reading and make interface
 * appear sane and normal. This also makes the driver compatible with
 * using the 3-wire interface. */
static uint16_t si4700_read_reg(int reg)
{
    si4700_read(((reg - STATUSRSSI) & 0xF) + 1);
    return cache[reg];
}

static void si4700_write_reg(int reg, uint16_t value)
{
    cache[reg] = value;
    si4700_write(((reg - POWERCFG) & 0xF) + 1);
}

static void si4700_write_masked(int reg, uint16_t bits, uint16_t mask)
{
    si4700_write_reg(reg, (cache[reg] & ~mask) | (bits & mask));
}

static void si4700_write_set(int reg, uint16_t mask)
{
    si4700_write_reg(reg, cache[reg] | mask);
}

static void si4700_write_clear(int reg, uint16_t mask)
{
    si4700_write_reg(reg, cache[reg] & ~mask);
}

#if (SI4700_GPIO_SETUP & SYSCONFIG1_GPIO3) != SYSCONFIG1_GPIO3_MO_ST_I
/* Poll i2c for the stereo status */
static inline int si4700_st(void)
{
    return (si4700_read_reg(STATUSRSSI) & STATUSRSSI_ST) >> 8;
}
#endif

void si4700_init(void)
{
    tuner_power(true);

    /* read all registers */
    si4700_read(16);
    
    /* check device id */
    if (cache[DEVICEID] == 0x1242)
    {
        tuner_present = true;

#ifdef USE_INTERNAL_OSCILLATOR
        /* enable the internal oscillator */
        si4700_write_set(TEST1, TEST1_XOSCEN);
        sleep(HZ/2);
#endif    
    }

    tuner_power(false);
}

static void si4700_sleep(int snooze)
{
    if (snooze)
    {
        /** power down **/
        /* ENABLE high, DISABLE high */
        si4700_write_set(POWERCFG,
                         POWERCFG_DISABLE | POWERCFG_ENABLE);
        /* Bits self-clear once placed in powerdown. */
        cache[POWERCFG] &= ~(POWERCFG_DISABLE | POWERCFG_ENABLE);
    }
    else
    {
        /** power up **/
        /* ENABLE high, DISABLE low */
        si4700_write_masked(POWERCFG, POWERCFG_ENABLE,
                            POWERCFG_DISABLE | POWERCFG_ENABLE);
        sleep(110 * HZ / 1000);

        /* init register cache */
        si4700_read(16);

#if SI4700_GPIO_SETUP != 0
        si4700_write_masked(SYSCONFIG1, SI4700_GPIO_SETUP, 
                            SYSCONFIG1_GPIO1 | SYSCONFIG1_GPIO2 |
                            SYSCONFIG1_GPIO3);
#endif
        si4700_write_masked(SYSCONFIG2,
                            SYSCONFIG2_SKEETHw(SEEK_THRESHOLD) |
                            SYSCONFIG2_VOLUMEw(0xF),
                            SYSCONFIG2_VOLUME | SYSCONFIG2_SEEKTH);
    }
}

static void si4700_set_frequency(int freq)
{
    static const unsigned int spacings[3] =
    {
          200000, /* SYSCONFIG2_SPACE_200KHZ */
          100000, /* SYSCONFIG2_SPACE_100KHZ */
           50000, /* SYSCONFIG2_SPACE_50KHZ  */
    };
    static const unsigned int bands[3] =
    {
        87500000, /* SYSCONFIG2_BAND_875_1080 */
        76000000, /* SYSCONFIG2_BAND_760_1080 */
        76000000, /* SYSCONFIG2_BAND_760_900  */
    };

    /* check BAND and spacings */
    int space = SYSCONFIG2_SPACEr(cache[SYSCONFIG2]);
    int band = SYSCONFIG2_BANDr(cache[SYSCONFIG2]);
    int chan = (freq - bands[band]) / spacings[space];

    curr_frequency = freq;

    si4700_write_reg(CHANNEL, CHANNEL_CHANw(chan) | CHANNEL_TUNE);

    do
    {
        /* tuning should be done within 60 ms according to the datasheet */
        sleep(HZ * 60 / 1000);
    }
    while ((si4700_read_reg(STATUSRSSI) & STATUSRSSI_STC) == 0); /* STC high? */

    si4700_write_clear(CHANNEL, CHANNEL_TUNE); /* Set TUNE low */
}

static int si4700_tuned(void)
{
    /* Primitive tuning check: sufficient level and AFC not railed */
    uint16_t status = si4700_read_reg(STATUSRSSI);
    if (STATUSRSSI_RSSIr(status) >= SEEK_THRESHOLD &&
        (status & STATUSRSSI_AFCRL) == 0)
        return 1;

    return 0;
}

static void si4700_set_region(int region)
{
    const struct si4700_region_data *rd = &si4700_region_data[region];
    uint16_t bandspacing = SYSCONFIG2_BANDw(rd->band) |
                           SYSCONFIG2_SPACEw(rd->spacing);
    uint16_t oldbs = cache[SYSCONFIG2] & (SYSCONFIG2_BAND | SYSCONFIG2_SPACE);

    si4700_write_masked(SYSCONFIG1,
                        rd->deemphasis ? SYSCONFIG1_DE : 0,
                        SYSCONFIG1_DE);
    si4700_write_masked(SYSCONFIG2, bandspacing,
                        SYSCONFIG2_BAND | SYSCONFIG2_SPACE);

    /* Retune if this region change would change the channel number. */
    if (oldbs != bandspacing)
        si4700_set_frequency(curr_frequency);
}

/* tuner abstraction layer: set something to the tuner */
int si4700_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            si4700_sleep(value);
            break;

        case RADIO_FREQUENCY:
            si4700_set_frequency(value);
            break;

        case RADIO_SCAN_FREQUENCY:
            si4700_set_frequency(value);
            return si4700_tuned();

        case RADIO_MUTE:
            si4700_write_masked(POWERCFG, value ? 0 : POWERCFG_DMUTE,
                                POWERCFG_DMUTE);
            break;

        case RADIO_REGION:
            si4700_set_region(value);
            break;

        case RADIO_FORCE_MONO:
            si4700_write_masked(POWERCFG, value ? POWERCFG_MONO : 0,
                                POWERCFG_MONO);
            break;

        default:
            return -1;
    }

    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int si4700_get(int setting)
{
    int val = -1; /* default for unsupported query */

    switch(setting)
    {
        case RADIO_PRESENT:
            val = tuner_present ? 1 : 0;
            break;

        case RADIO_TUNED:
            val = si4700_tuned();
            break;

        case RADIO_STEREO:
            val = si4700_st();
            break;
    }

    return val;
}

void si4700_dbg_info(struct si4700_dbg_info *nfo)
{
    memset(nfo->regs, 0, sizeof (nfo->regs));

    if (tuner_powered())
    {
        si4700_read(16);
        memcpy(nfo->regs, cache, sizeof (nfo->regs));
    }
}

