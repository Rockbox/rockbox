/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2013 Andrew Ryabinin
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

#ifndef _PCM1792_H
#define _PCM1792_H

#define PCM1792_VOLUME_MIN -1270
#define PCM1792_VOLUME_MAX 0

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP)
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, PCM1792_VOLUME_MIN/10, PCM1792_VOLUME_MAX/10, 0)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 1, 0)

#define PCM1792_REG(x) (((x)&0x1f)<<8)

/**
 * Register #18
 */
/* Attenuation Load Control */
#define PCM1792_ATLD_OFF        (0<<7)
#define PCM1792_ATLD_ON         (1<<7)

/* Audio Interface Data Format */
#define PCM1792_FMT_16_RJ      (0<<4) /* 16-bit standard format, right-justified data */
#define PCM1792_FMT_20_RJ      (1<<4) /* 20-bit standard format, right-justified data */
#define PCM1792_FMT_24_RJ      (2<<4) /* 24-bit standard format, right-justified data */
#define PCM1792_FMT_24_MSB_I2S (3<<4) /* 24-bit MSB-first, left-justified format data */
#define PCM1792_FMT_16_I2S     (4<<4) /* 16-bit I2S format data */
#define PCM1792_FMT_24_I2S     (5<<4) /* 24-bit I2S format data */

/* Sampling Frequency Selection for the De-Emphasis Function */
#define PCM1792_DMF_DISABLE (0<<2)
#define PCM1792_DMF_48      (1<<2)
#define PCM1792_DMF_44      (2<<2)
#define PCM1792_DMF_32      (2<<2)

/* Digital De-Emphasis Control */
#define PCM1792_DME_OFF     (0<<1)
#define PCM1792_DME_ON      (1<<1)

/* Soft Mute Control */
#define PCM1792_MUTE_OFF (0<<0)
#define PCM1792_MUTE_ON  (1<<0)


/**
 * Register #19
 */
/* Output Phase Reversal */
#define PCM1792_REV_OFF  (0<<7)
#define PCM1792_REV_ON   (1<<7)

/* Attenuation Rate Select */
#define PCM1792_ATS_DIV1   (0<<5)
#define PCM1792_ATS_DIV2   (1<<5)
#define PCM1792_ATS_DIV4   (2<<5)
#define PCM1792_ATS_DIV8   (4<<5)

/* DAC Operation Control */
#define PCM1792_OPE_ON       (0<<4)
#define PCM1792_OPE_OFF      (1<<4)

/* Stereo DF Bypass Mode Select */
#define PCM1792_DFMS_MONO   (0<<2)
#define PCM1792_DFMS_STERO  (1<<2)

/* Digital Filter Rolloff Control */
#define PCM1792_FLT_SHARP   (0<<1)
#define PCM1792_FLT_SLOW    (1<<1)

/* Infinite Zero Detect Mute Control */
#define PCM1792_INZD_OFF    (0<<0)
#define PCM1792_INZD_ON     (1<<0)


/**
 * Register #20
 */
/* System Reset Control */
#define PCM1792_SRST_NORMAL  (0<<6)
#define PCM1792_SRST_RESET   (1<<6)

/* DSD Interface Mode Control */
#define PCM1792_DSD_OFF      (0<<5)
#define PCM1792_DSD_ON       (1<<5)

/* Digital Filter Bypass (or Through Mode) Control */
#define PCM1792_DFTH_ENABLE  (0<<4) /* Digital filter enabled */
#define PCM1792_DFTH_BYPASS  (1<<4) /* Digital filter bypassed
                                       for external digital filter */

/* Monaural Mode Selection */
#define PCM1792_STEREO       (0<<3)
#define PCM1792_MONO         (1<<3)

/* Channel Selection for Monaural Mode */
#define PCM1792_CHSL_L       (0<<2)
#define PCM1792_CHSL_R       (1<<2)

/* Delta-Sigma Oversampling Rate Selection */
#define PCM1792_OS_64        (0<<0)
#define PCM1792_OS_32        (1<<0)
#define PCM1792_OS_128       (2<<0)

/**
 * Register #21
 */
/* DSD Zero Output Enable */
#define PCM1792_DZ_DISABLE   (0<<1)
#define PCM1792_DZ_EVEN      (1<<1) /* Even pattern detect */
#define PCM1792_DZ_96H       (2<<1) /* 96h pattern detect */

/* PCM Zero Output Enable */
#define PCM1792_PCMZ_OFF     (0<<0)
#define PCM1792_PCMZ_ON      (1<<0)

void audiohw_mute(void);
void pcm1792_set_ml(const int);
void pcm1792_set_mc(const int);
void pcm1792_set_md(const int);
void pcm1792_set_ml_dir(const int);

#endif
