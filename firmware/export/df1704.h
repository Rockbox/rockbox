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

#ifndef _DF1704_H
#define _DF1704_H

#define DF1704_VOLUME_MIN -1270
#define DF1704_VOLUME_MAX 0

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP)
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, DF1704_VOLUME_MIN/10, DF1704_VOLUME_MAX/10, 0)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 1, 0)

#define DF1704_MODE(x) (((x)&0x03)<<9)

/**
 * MODE0 register settings
 */
/* Left channel attenuation data load control */
#define DF1704_LDL_ON    (1<<8)
#define DF1704_LDL_OFF   (0<<8)

/**
 * MODE1 register settings
 */
/* Right channel attenuation data load control */
#define DF1704_LDR_ON    (1<<8)
#define DF1704_LDR_OFF   (0<<8)

/**
 * MODE2 register settings
 */
#define DF1704_MUTE_ON   (0<<0)
#define DF1704_MUTE_OFF  (1<<0)
/* Digital De-Emphasis */
#define DF1704_DEM_ON    (1<<1)
#define DF1704_DEM_OFF   (0<<1)
/* Input data format & word lengths */
#define DF1704_IW_16_I2S (0<<3)
#define DF1704_IW_24_I2S (1<<3)

#define DF1704_IW_16_RJ  (0<<3)
#define DF1704_IW_20_RJ  (1<<3)
#define DF1704_IW_24_RJ  (2<<3)
#define DF1704_IW_24_LJ  (3<<3)
/* Output data format & word lengths */
#define DF1704_OW_16     (0<<5)
#define DF1704_OW_18     (1<<5)
#define DF1704_OW_20     (2<<5)
#define DF1704_OW_24     (3<<5)

/**
 * MODE3 register settings
 */
#define DF1704_I2S_OFF   (0<<0)
#define DF1704_I2S_ON    (1<<0)
#define DF1704_LRP_L     (0<<1)
#define DF1704_LRP_H     (1<<1)
#define DF1704_ATC_ON    (1<<2)
#define DF1704_ATC_OFF   (0<<2)
#define DF1704_SRO_SHARP (0<<3)
#define DF1704_SRO_SLOW  (1<<3)
/* CLKO output frequency selection */
#define DF1704_CKO_FULL  (0<<5)
#define DF1704_CKO_HALF  (1<<5)
/* Sampling freq selection for the De-Emphasis */
#define DF1704_SF_44     (0<<6)
#define DF1704_SF_32     (3<<6)
#define DF1704_SF_48     (2<<6)

void audiohw_mute(void);
void df1704_set_ml(const int);
void df1704_set_mc(const int);
void df1704_set_md(const int);
void df1704_set_ml_dir(const int);
#endif
