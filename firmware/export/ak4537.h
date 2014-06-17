/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Mark Arigo
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

#ifndef _AK4537_H
#define _AK4537_H


#define AUDIOHW_CAPS    (LIN_GAIN_CAP | MIC_GAIN_CAP)

/* Volume goes from -127.0 ... 0 dB in 0.5 dB increments */
AUDIOHW_SETTING(VOLUME, "dB",  0,  1, -128,   0, -25)
#ifdef HAVE_RECORDING
/* line input: -23 .. +12dB */
AUDIOHW_SETTING(LEFT_GAIN,  "dB", 0,  1, -23, 12, 0)
AUDIOHW_SETTING(RIGHT_GAIN, "dB", 0,  1, -23, 12, 0)
/* mic gain: +15dB fixed +20dB switchable mic preamp gain
   and the line stage of -23..+12dB make a total range of -8..+47dB */
AUDIOHW_SETTING(MIC_GAIN,   "dB", 0,  1,   -8,   47,   20)

void audiohw_set_recsrc(int source);
#endif /* HAVE_RECORDING */

#define AKC_NUM_REGS        0x11

/* Common register bits */

/* Power Management 1 */
#define AK4537_PM1          0x00
#define PMADL       (1 << 0)
#define PMMICL      (1 << 1)
#define PMIPGL      (1 << 2)
#define PMMO        (1 << 3)
#define PMLO        (1 << 4)
#define PMBPM       (1 << 5)
#define PMBPS       (1 << 6)
#define PMVCM       (1 << 7)

/* Power Management 2 */
#define AK4537_PM2          0x01
#define PMDAC       (1 << 0)
#define PMHPR       (1 << 1)
#define PMHPL       (1 << 2)
#define PMSPK       (1 << 3)
#define SPKG        (1 << 4)
#define PMPLL       (1 << 5)
#define PMXTL       (1 << 6)
#define MCLKPD      (1 << 7)

/* Signal Select 1 */
#define AK4537_SIGSEL1      0x02
#define MOUT2       (1 << 0)
#define ALCS        (1 << 1)
#define BPMSP       (1 << 2)
#define BPSSP       (1 << 3)
#define MICM        (1 << 4)
#define DAMO        (1 << 5)
#define PSMO        (1 << 6)
#define MOGN        (1 << 7)

/* Signal Select 2 */
#define AK4537_SIGSEL2      0x03
#define HPR         (1 << 0)
#define HPL         (1 << 1)
#define BPMHP       (1 << 2)
#define BPSHP       (1 << 3)
#define MICL        (1 << 4)
#define PSLO        (1 << 6)
#define DAHS        (1 << 7)

/* Mode Control 1 */
#define AK4537_MODE1        0x04
#define DIF_MASK    (3 << 0)
#define BICK_MASK   (1 << 2)
#define MCKO_EN     (1 << 3)
#define MCKO_MASK   (3 << 4)
#define MCKI_MASK   (3 << 6)

/* Mode Control 2 */
#define AK4537_MODE2        0x05
#define SPPS        (1 << 0)
#define LOOP        (1 << 1)
#define HPM         (1 << 2)
#define FS_MASK     (7 << 5)

/* DAC Control */
#define AK4537_DAC          0x06
#define DEM_MASK    (3 << 0)
#define BST_MASK    (3 << 2)
#define DATTC       (1 << 4)
#define SMUTE       (1 << 5)
#define TM_MASK     (3 << 6)

/* MIC Control */
#define AK4537_MIC          0x07
#define MGAIN       (1 << 0)
#define MSEL        (1 << 1)
#define MICAD       (1 << 2)
#define MPWRI       (1 << 3)
#define MPWRE       (1 << 4)
#define IPGAC       (1 << 5)

/* Timer Select */
#define AK4537_TIMER        0x08
#define LTM_MASK    (3 << 0)
#define WTM_MASK    (3 << 2)
#define ZTM_MASK    (3 << 4)
#define ZTM1        (1 << 5)
#define ROTM        (1 << 6)

/* ALC Mode Control 1 */
#define AK4537_ALC1         0x09
#define LMTH        (1 << 0)
#define RATT        (1 << 1)
#define LMAT_MASK   (3 << 2)
#define ZELM        (1 << 4)
#define ALC1        (1 << 5)
#define ALC2        (1 << 6)

/* ALC Mode Control 2 */
#define AK4537_ALC2         0x0a

/* Lch Input PGA Control */
#define AK4537_IPGAL        0x0b

/* Lch Digital ATT Control */
#define AK4537_ATTL         0x0c

/* Rch Digital ATT Control */
#define AK4537_ATTR         0x0d

/* Volume Control */
#define AK4537_VOLUME       0x0e
#define ATTS_MASK   (7 << 4)
#define ATTRM       (1 << 7)

/* Rch Input PGA Control */
#define AK4537_IPGAR        0x0f

/* Power Management 3 */
#define AK4537_PM3          0x10
#define PMADR       (1 << 0)
#define PMMICR      (1 << 1)
#define PMIPGR      (1 << 2)
#define INR         (1 << 3)
#define INL         (1 << 4)

/* Sampling frequency (PLL mode) */
#define AKC_PLL_8000HZ      (7 << 5)
#define AKC_PLL_11025HZ     (2 << 5)
#define AKC_PLL_16000HZ     (6 << 5)
#define AKC_PLL_22050HZ     (1 << 5)
#define AKC_PLL_24000HZ     (5 << 5)
#define AKC_PLL_32000HZ     (4 << 5)
#define AKC_PLL_44100HZ     (0 << 5)
#define AKC_PLL_48000HZ     (3 << 5)

/* MCKI input frequency (PLL mode) */
#define MCKI_PLL_12288KHZ   (0 << 6)
#define MCKI_PLL_11289KHZ   (1 << 6)
#define MCKI_PLL_12000KHZ   (2 << 6)

/* MCKO frequency (PLL mode, MCKO bit = 1) */
#define MCKO_PLL_256FS      (0 << 4)
#define MCKO_PLL_128FS      (1 << 4)
#define MCKO_PLL_64FS       (2 << 4)
#define MCKO_PLL_32FS       (3 << 4)

/* BICK frequency */
#define BICK_64FS           (0 << 2)
#define BICK_32FS           (1 << 2)

/* Audio interface format */
#define DIF_MSB_LSB         (0 << 0)
#define DIF_MSB_MSB         (1 << 0)
#define DIF_I2S             (2 << 0)

/* Low frequency boost control */
#define BST_OFF             (0 << 2)
#define BST_MIN             (1 << 2)
#define BST_MID             (2 << 2)
#define BST_MAX             (3 << 2)

#endif /* _AK4537_H */
